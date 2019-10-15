#include "uve_simobjs/uve_ff_load.hh"

UVELoadFifo::UVELoadFifo(UVEStreamingEngineParams *params)
    : SimObject(params),
      fifos(32, StreamFifo(params->width)),
      cacheLineSize(params->system->cacheLineSize()) {
    for (int i = 0; i < fifos.size(); i++) {
        fifos[i].set_owner(this);
    }
}

void
UVELoadFifo::init(){
    SimObject::init();
}

bool
UVELoadFifo::tick() {
    // Check if fifo is ready and answer true if it is
    for (auto fifo : fifos) {
        if (fifo.ready()) {
            return true;
        }
    }
    return false;
}

std::vector<std::pair<int, CoreContainer *>>
UVELoadFifo::get_data() {
    // Check if fifo is ready and if so, organize data to the cpu;
    std::vector<std::pair<int, CoreContainer *>> data_vector;
    for (auto &fifo : fifos) {
        if (fifo.ready()) {
            int physIdx = -1;
            auto entry = fifo.get(&physIdx);
            data_vector.push_back(
                std::make_pair(physIdx, new CoreContainer(entry)));
        }
    }
    return data_vector;
}

CoreContainer *
UVELoadFifo::getData(int sid, PhysRegIndex physIdx) {
    // Check if fifo is ready and if so, organize data to the cpu;
    if (fifos[sid].ready(physIdx)) {
        auto entry = fifos[sid].get(&physIdx);
        return new CoreContainer(entry);
    } else {
        return new CoreContainer();
    }
}

void
UVELoadFifo::reserve(StreamID sid, uint32_t ssid, uint8_t size, uint8_t width,
                     bool last = false) {
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
UVELoadFifo::fetch(StreamID sid, CoreContainer **cnt) {
    if (fifos[sid].ready()){
        // cnt = new SmartContainer(to_smart(fifos[sid].get()));
        auto entry = fifos[sid].get();
        *cnt = new CoreContainer(entry);
        return true;
    }
    else {
        *cnt = nullptr;
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
    return true;
}

bool
UVELoadFifo::ready(StreamID sid, int physIdx) {
    return fifos[sid].ready(physIdx);
}

bool
UVELoadFifo::squash(StreamID sid, int physIdx) {
    // return fifos[sid].squash(physIdx);
    for (auto &fifo : fifos) {
        fifo.squash(physIdx);
    }
    return true;
}

bool
UVELoadFifo::isFinished(StreamID sid) {
    // JMFIXME: Probably will not work (use start and end counter, keep state
    // of configuration start and end)
    return fifos[sid].complete();
}

void
StreamFifo::insert(uint16_t size, uint16_t ssid, uint8_t width,
                 bool last = false){
    //Dont just push back, need to interpolate;
    status = SEIterationStatus::Configured;
    uint16_t id = 0;
    //Offset sets the beggining of the data pointer; 0 in new entries;
    //Incremented by the size of the last reserve in already present entries
    uint16_t offset = 0;
    //First insertion case:
    if (this->empty()){
        //Create new entry
        this->push_back(FifoEntry(width, config_size));
        //Reserve space in entry
        assert(this->back().reserve(&size, last));
        //First entry was created, set id to 0;
        this->map.push_back(create_MS(id,size,offset));
        return;
    }
    //Get last entry: this should be the place where to put data;
    //This insert is called in order
    if (this->back().complete()){
        assert(!this->full());
        //Create new entry due to the last one being complete
        this->push_back(FifoEntry(width, config_size));
        //New entry was created, increment id;
        id = this->map.back().id + 1;
        //Here we absolutelly need to be able to reserve
        assert(this->back().reserve(&size, last));

        this->map.push_back(create_MS(id, size, offset));
        return;
    }
    // Reserve the space -> When returns false the size is updated to the
    // remaining size
    uint16_t changeable_size = size;
    bool result = back().reserve(&changeable_size, last);
    if (!result) {
        // Means that reserve is bigger than available space:
        // Create new entry
        this->push_back(FifoEntry(width, config_size));
        //New entry was created, increment id;
        if (this->map.back().split) {
            id = this->map.back().id2;
            offset = this->map.back().offset2 + this->map.back().size2;
        } else {
            id = this->map.back().id;
            offset = this->map.back().offset + this->map.back().size;
        }
        assert(this->back().reserve(&changeable_size, last));

        this->map.push_back(create_MS(id, id + 1, size - changeable_size,
                                      changeable_size, offset, 0));
        return;
    }
    else {
        //No new entry was created, mantain id, but increase offset
        if (this->map.back().split) {
            id = this->map.back().id2;
            offset = this->map.back().offset2 + this->map.back().size2;
        } else {
            id = this->map.back().id;
            offset = this->map.back().offset + this->map.back().size;
        }
        this->map.push_back(create_MS(id, size, offset));
        return;
    }
}

bool
StreamFifo::merge_data(uint16_t ssid, uint8_t * data){
    assert(!empty());
    //Get corresponding fifo entry
    FifoEntry *entry;
    auto it = this->map.begin() + ssid;
    auto mapping = *it;
    assert(!mapping.used);

    entry = &this->at(mapping.id);
    entry->merge_data(data, mapping.offset, mapping.size);

    if (mapping.split) {
        entry = &this->at(mapping.id2);
        entry->merge_data(data + mapping.size, mapping.offset2, mapping.size2);
    }

    it->used = true;

    return true;
}

FifoEntry
StreamFifo::get() {
    assert(!empty());

    FifoEntry first_entry = front();
    erase(begin());
    // Decrease by 1 all the id pointers in the map
    for (auto t = map.begin(); t != map.end(); ++t) {
        if (!t->used) {
            t->id--;
            if (t->split) t->id2--;
        }
    }
    physRegStack.pop_front();
    if (first_entry.is_last()) {
        // Set state as complete
        status = SEIterationStatus::Ended;
    }
    return first_entry;
}

FifoEntry
StreamFifo::get(int *physidx) {
    assert(!empty());

    FifoEntry first_entry = front();
    erase(begin());
    // Decrease by 1 all the id pointers in the map
    for (auto t = map.begin(); t != map.end(); ++t) {
        if (!t->used) {
            t->id--;
            if (t->split) t->id2--;
        }
    }
    *physidx = physRegStack.front();
    physRegStack.pop_front();
    if (first_entry.is_last()) {
        // Set state as complete
        status = SEIterationStatus::Ended;
    }
    return first_entry;
}

bool
StreamFifo::ready() {
    if (empty()) return false;
    if (physRegStack.empty()) return false;
    FifoEntry first_entry = front();
    return first_entry.ready();
}

bool
StreamFifo::ready(int physIdx) {
    assert(!physRegStack.empty());

    if (empty()) return false;
    // Search for the physIdx
    auto top_reg = physRegStack.front();
    if (top_reg == physIdx && ready())
        return true;
    else
        return false;
}

bool
StreamFifo::squash(int physIdx) {
    if (empty()) return false;
    if (physRegStack.empty()) return false;
    // Search for the physIdx
    int regIdx = -1;
    auto it = physRegStack.end();
    it--;

    while (it != physRegStack.begin()) {
        regIdx = *it;
        if (regIdx == physIdx) {
            physRegStack.erase(it);
            return true;
        }
        it--;
    }
    return false;
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
StreamFifo::insert(int physIdx) {
    // Insert in the stack of physical registers
    physRegStack.push_back(physIdx);
}

void
FifoEntry::merge_data(uint8_t * data, uint16_t offset, uint16_t _size) {
    auto my_data = this->raw_ptr<uint8_t>();
    for (int i = 0; i < _size; i++) {
        (my_data+offset)[i] =  data[i];
    }
    csize += _size;
    assert(!(csize > size));
    if (csize == size){
        cstate = States::Complete;
    }
    else cstate = States::NotComplete;
}

bool
FifoEntry::reserve(uint16_t *_size, bool last = false) {
    // In this case we are complete it must be a bug
    assert(rstate != States::Complete);
    // Reservation do not fit entirelly
    // Must reserve for what fits and the callee must send the remaining to the
    // next entry
    // Send false and update the _size value with the remaining space
    if (size + *_size > config_size) {
        rstate = States::Complete;
        *_size -= config_size - size;
        size = config_size;
        set_valid(size);
        return false;
    }
    // Completion on point, the reservation belongs here
    else if (size + *_size == config_size) {
        rstate = States::Complete;
        size += *_size;
        set_valid(size);
        set_last(last);
        return true;
    }
    else{
        if (last){
            set_last(true);
            rstate = States::Complete;
            size += *_size;
            set_valid(size);
            return true;
        }
        rstate = States::NotComplete;
        size += *_size;
        return true;
    }

}