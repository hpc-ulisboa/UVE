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

bool
UVELoadFifo::insert(StreamID sid, uint32_t ssid, CoreContainer data,
                    uint8_t size, uint8_t width){
    //If empty put
    //If not empty try and merge
    //  Not Succeded: Create new entry
    //      If full: Warn no more space
    //      Else: Create new entry

    FifoEntry entry = FifoEntry(data, size, ssid, width);
    if (fifos[sid].empty()){
        fifos[sid].insert(entry);
    }
    else {
        if (!fifos[sid].merge(entry)){
            if (fifos[sid].full()){return false;}
            else {
                fifos[sid].insert(entry);
            }
        }
    }
    return true;
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

SmartContainer
UVELoadFifo::to_smart(FifoEntry entry){
    SmartContainer ret;
    ret.first = entry.as<uint8_t>();
    ret.second = entry.getEnable();
    return ret;
}

void
StreamFifo::insert(FifoEntry entry){
    push_back(entry);
}

bool
StreamFifo::merge(FifoEntry new_entry){
    assert(!empty());
    uint16_t packets_per_entry = new_entry.SIZE / owner->cacheLineSize;
    uint16_t entry_id = new_entry.getSsid() / packets_per_entry;
    uint16_t entry_offset = new_entry.getSsid() % packets_per_entry;
    //Get corresponding fifo entry
    auto entry = this->at(entry_id);
    entry.merge_entry(new_entry, entry_offset);
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
    return first_entry.complete();
}

bool
StreamFifo::full() {
    return this->size() == this->max_size;
}

bool
StreamFifo::empty() {
    return this->size() == 0;
}

void
FifoEntry::merge_entry(FifoEntry entry, uint16_t offset) {
    auto iptr = this->raw_ptr<uint8_t>();
    auto nptr = entry.raw_ptr<uint8_t>();
    for (int i = 0; i < offset; i++){iptr ++;}
    for (int i = 0; i < entry.size; i++){
        *iptr = *nptr;
        iptr++; nptr++;
    }
}