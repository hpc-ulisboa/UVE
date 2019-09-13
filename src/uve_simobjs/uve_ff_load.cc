#include "uve_simobjs/uve_ff_load.hh"

UVELoadFifo::UVELoadFifo(UVEStreamingEngineParams *params) :
  SimObject(params), fifos(), cacheLineSize(params->system->cacheLineSize())
{
    fifos.fill(StreamFifo());
    for (int i=0; i<fifos.size(); i++)
    {
        fifos[i].set_owner(this);
    }
}

void
UVELoadFifo::init(){
    SimObject::init();
}

void
UVELoadFifo::tick(){

}

void
UVELoadFifo::reserve(StreamID sid, uint32_t ssid, uint8_t size, uint8_t width,
                    bool last = false){
    // Reserve space in fifo. This reserve comes from the engine
    assert(!fifos[sid].full());
    fifos[sid].insert(size, ssid, width, last);
}

bool
UVELoadFifo::insert(StreamID sid, uint32_t ssid, CoreContainer data){
    //If not empty try and merge
    //  Not Succeded: Create new entry
    //      If full: Warn no more space
    //      Else: Create new entry
    assert(!fifos[sid].empty());

    if (fifos[sid].merge_data(ssid, data.raw_ptr<uint8_t>())){
        return true;
    }
    return false;
}

bool
UVELoadFifo::fetch(StreamID sid, SmartContainer * cnt){
    assert( cnt == nullptr );
    if (fifos[sid].ready()){
        cnt = new SmartContainer(to_smart(fifos[sid].get()));
        return true;
    }
    else {
        cnt = nullptr;
        return false;
    }
}

bool
UVELoadFifo::full(StreamID sid){
    return fifos[sid].full();
}

bool
UVELoadFifo::reserve(StreamID sid, int physIdx) {
    // Reserve space in fifo. This reserve comes from the cpu (rename phase)
    assert(!fifos[sid].full());
    fifos[sid].insert(physIdx);
}

bool
UVELoadFifo::ready(StreamID sid, int physIdx) {
    return fifos[sid].ready(physIdx);
}

SmartContainer
UVELoadFifo::to_smart(FifoEntry entry) {
    SmartContainer ret;
    ret.first = entry.as<uint8_t>();
    ret.second = entry.getEnable();
    return ret;
}

void
StreamFifo::insert(uint16_t size, uint16_t ssid, uint8_t width,
                 bool last = false){
    //Dont just push back, need to interpolate;
    uint16_t id = 0;
    //Offset sets the beggining of the data pointer; 0 in new entries;
    //Incremented by the size of the last reserve in already present entries
    uint16_t offset = 0;
    //First insertion case:
    if (this->empty()){
        //Create new entry
        this->push_back(FifoEntry(width));
        //Reserve space in entry
        assert(this->back().reserve(size, last));
        //First entry was created, set id to 0;
        this->map.push_back(create_MS(id,size,offset));
        return;
    }
    //Get last entry: this should be the place where to put data;
    //This insert is called in order
    if (this->back().complete()){
        assert(!this->full());
        //Create new entry due to the last one being complete
        this->push_back(FifoEntry(width));
        //New entry was created, increment id;
        id = this->map.back().id + 1;
        //Here we absolutelly need to be able to reserve
        assert(this->back().reserve(size, last));

        this->map.push_back(create_MS(id + 1, size, offset));
        return;
    }
    //Reserve the space
    bool result = back().reserve(size, last);
    if (!result){
        //Means that reserve is bigger than space:
        //Create new entry
        this->push_back(FifoEntry(width));
        //New entry was created, increment id;
        id = this->map.back().id + 1;

        assert(this->back().reserve(size, last));

        this->map.push_back(create_MS(id + 1,size, offset));
        return;
    }
    else {
        //No new entry was created, mantain id, but increase offset
        id = this->map.back().id;
        offset = this->map.back().offset + this->map.back().size;
        this->map.push_back(create_MS(id,size,offset));
        return;
    }
}

bool
StreamFifo::merge_data(uint16_t ssid, uint8_t * data){
    assert(!empty());
    //Get corresponding fifo entry
    assert(!this->map[ssid].used);
    auto id = this->map[ssid].id;
    auto offset = this->map[ssid].offset;
    auto size = this->map[ssid].size;
    auto entry = &this->at(id);
    entry->merge_data(data, offset, size);
    auto it = this->map.begin() + ssid;
    it->used = true;
    return true;
}

FifoEntry
StreamFifo::get() {
    assert(!empty());

    FifoEntry first_entry = front();
    erase(begin());
    return first_entry;
}

bool
StreamFifo::ready() {
    if (empty()) return false;
    FifoEntry first_entry = front();
    return first_entry.ready();
}

bool
StreamFifo::ready(int physIdx) {
    if (empty()) return false;

    // Search for the physIdx in function
    return true;
}

bool
StreamFifo::full() {
    //Real size is the number of fully complete entries
    return this->real_size() == this->max_size;
}

bool
StreamFifo::empty() {
    return this->size() == 0;
}

uint16_t
StreamFifo::real_size() {
    uint16_t size = 0;
    auto it = this->begin();
    while (it!=this->end()){
        if (it->complete()) size ++;
        it++;
    }
    return size;
}

void
StreamFifo::insert(int physIdx) {}

void
FifoEntry::merge_data(uint8_t * data, uint16_t offset, uint16_t _size) {
    auto my_data = this->raw_ptr<uint8_t>();
    for (int i=0; i<size; i++){
        (my_data+offset)[i] =  data[i];
    }
    csize += _size;
    if (csize == size){
        cstate = States::Complete;
    }
    else cstate = States::NotComplete;
}

bool
FifoEntry::reserve(uint16_t _size, bool last = false) {
    //In this case we are complete and this reservation does not belong here:
    //Send false;
    assert(rstate != States::Complete);
    if (size + _size > SIZE){
        rstate = States::Complete;
        return false;
    }
    // Completion on point, the reservation belongs here
    else if (size + _size == SIZE){
        rstate = States::Complete;
        size += _size;
        return true;
    }
    else{
        if (last){
            rstate = States::Complete;
            size += _size;
            return true;
        }
        rstate = States::NotComplete;
        size += _size;
        return true;
    }

}