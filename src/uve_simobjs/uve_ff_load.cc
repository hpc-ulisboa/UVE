#include "uve_simobjs/uve_ff_load.hh"

UVELoadFifo::UVELoadFifo(UVEStreamingEngineParams *params)
    : SimObject(params),
      fifos(32),
      cacheLineSize(params->system->cacheLineSize()),
      confParams(params) {
    for (int i = 0; i < fifos.size(); i++) {
        fifos[i] = new StreamFifo(confParams->width, confParams->fifo_depth,
                                  confParams->max_request_size, i);
        fifos[i]->set_owner(this);
    }
}

void
UVELoadFifo::init(){
    SimObject::init();
}

bool
UVELoadFifo::tick(CallbackInfo *info) {
    // Check if fifo is ready and answer true if it is
    int8_t k = -1;
    for (int i = 0; i < fifos.size(); i++) {
        if (fifos[i]->ready()) {
            info->psids[++k] = i;
        }
    }
    if (k != -1) {
        info->psids_size = k + 1;
        return true;
    }
    return false;
}

CoreContainer *
UVELoadFifo::getData(int sid) {
    // Check if fifo is ready and if so, organize data to the cpu;
    if (fifos[sid]->ready()) {
        auto entry = fifos[sid]->get();
        return new CoreContainer(entry);
    } else {
        return new CoreContainer();
    }
}

void
UVELoadFifo::reserve(StreamID sid, uint32_t ssid, uint8_t size, uint8_t width,
                     bool last = false) {
    // Reserve space in fifo. This reserve comes from the engine
    assert(!fifos[sid]->full());
    fifos[sid]->insert(size, ssid, width, last);
}

bool
UVELoadFifo::insert(StreamID sid, uint32_t ssid, CoreContainer data){
    //If not empty try and merge
    //  Not Succeded: Create new entry
    //      If full: Warn no more space
    //      Else: Create new entry
    assert(!fifos[sid]->empty());

    if (fifos[sid]->merge_data(ssid, data.raw_ptr<uint8_t>())) {
        return true;
    }
    return false;
}

bool
UVELoadFifo::fetch(StreamID sid, CoreContainer **cnt) {
    if (fifos[sid]->ready()) {
        // cnt = new SmartContainer(to_smart(fifos[sid].get()));
        auto entry = fifos[sid]->get();
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
    return fifos[sid]->full();
}

bool
UVELoadFifo::ready(StreamID sid) {
    return fifos[sid]->ready();
}

bool
UVELoadFifo::squash(StreamID sid) {
    return fifos[sid]->squash();
}

bool
UVELoadFifo::commit(StreamID sid) {
    return fifos[sid]->commit();
}

void
UVELoadFifo::clear(StreamID sid) {
    delete fifos[sid];
    fifos[sid] = new StreamFifo(confParams->width, confParams->fifo_depth,
                                confParams->max_request_size, sid);
    fifos[sid]->set_owner(this);
    return;
}

bool
UVELoadFifo::isFinished(StreamID sid) {
    // JMFIXME: Probably will not work (use start and end counter, keep state
    // of configuration start and end)
    return fifos[sid]->complete();
}

uint16_t
UVELoadFifo::getAvailableSpace(StreamID sid) {
    // JMFIXME: Probably will not work (use start and end counter, keep state
    // of configuration start and end)
    return fifos[sid]->availableSpace();
}

void
StreamFifo::insert(uint16_t size, uint16_t ssid, uint8_t width,
                   bool last = false) {
    //Dont just push back, need to interpolate;
    status = SEIterationStatus::Configured;
    uint16_t id = 0;
    //Offset sets the beggining of the data pointer; 0 in new entries;
    //Incremented by the size of the last reserve in already present entries
    uint16_t offset = 0;
    //First insertion case:
    if (this->empty()){
        //Create new entry
        fifo_container->push_front(FifoEntry(width, config_size));
        speculationPointer++;
        DPRINTF(UVEFifo, "Inserted on back of fifo[%d] ssid[%d]: Was empty\n",
                my_id, ssid);
        //Reserve space in entry
        assert(fifo_container->front().reserve(&size, last));
        //First entry was created, set id to 0;
        this->map.push_back(create_MS(id,size,offset));
        return;
    }
    //Get last entry: this should be the place where to put data;
    //This insert is called in order
    if (fifo_container->front().complete()) {
        assert(!this->full());
        //Create new entry due to the last one being complete
        fifo_container->push_front(FifoEntry(width, config_size));
        DPRINTF(UVEFifo, "Inserted on back of fifo[%d] ssid[%d]. Size[%d]\n",
                my_id, ssid, fifo_container->size());
        //New entry was created, increment id;
        id = this->map.back().id + 1;
        //Here we absolutelly need to be able to reserve
        assert(fifo_container->front().reserve(&size, last));

        this->map.push_back(create_MS(id, size, offset));
        return;
    }
    // Reserve the space -> When returns false the size is updated to the
    // remaining size
    uint16_t changeable_size = size;
    bool result = fifo_container->front().reserve(&changeable_size, last);
    if (!result) {
        // Means that reserve is bigger than available space:
        // Create new entry
        fifo_container->push_front(FifoEntry(width, config_size));
        DPRINTF(UVEFifo, "Inserted on back of fifo[%d] ssid[%d]. Size[%d]\n",
                my_id, ssid, fifo_container->size());
        // Restore pointer
        // New entry was created, increment id;
        if (this->map.back().split) {
            id = this->map.back().id2;
            offset = this->map.back().offset2 + this->map.back().size2;
        } else {
            id = this->map.back().id;
            offset = this->map.back().offset + this->map.back().size;
        }
        assert(fifo_container->front().reserve(&changeable_size, last));

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
    auto it = this->map.begin() + ssid;
    auto mapping = *it;
    assert(!mapping.used);

    auto list_it = fifo_container->rbegin();
    advance(list_it, mapping.id);
    list_it->merge_data(data, mapping.offset, mapping.size);

    if (mapping.split) {
        list_it = fifo_container->rbegin();
        advance(list_it, mapping.id2);
        list_it->merge_data(data + mapping.size, mapping.offset2,
                            mapping.size2);
    }

    it->used = true;

    return true;
}

FifoEntry
StreamFifo::get() {
    assert(!empty());

    FifoEntry specEntry = *speculationPointer;
    speculationPointer++;
    DPRINTF(UVEFifo, "Get of fifo[%d]. Size[%d]\n", my_id,
            fifo_container->size());
    if (specEntry.is_last()) {
        // Set state as complete
        status = SEIterationStatus::Ended;
    }
    return specEntry;
}

bool
StreamFifo::ready() {
    if (empty()) return false;
    if (!speculationPointer.valid()) return false;
    FifoEntry first_entry = *speculationPointer;
    return first_entry.ready();
}

bool
StreamFifo::commit() {
    if (empty()) return false;

    auto elem = fifo_container->back();

    fifo_container->pop_back();
    DPRINTF(UVEFifo, "Commit on fifo[%d]. Size[%d]\n", my_id,
            fifo_container->size());
    speculationPointer--;
    // Decrease by 1 all the id pointers in the map
    for (auto t = map.begin(); t != map.end(); ++t) {
        t->id = t->id <= 0 ? 0 : t->id - 1;
        if (t->split) t->id2 = t->id2 <= 0 ? 0 : t->id2 - 1;
    }

    if (elem.is_last()) {
        // Stream Ended
        // Return End Code
        return true;
    }

    return true;
}

bool
StreamFifo::squash() {
    if (empty()) return false;
    speculationPointer--;
    DPRINTF(UVEFifo, "Squash on fifo[%d]. Size[%d]\n", my_id,
            fifo_container->size());
    return true;
}

bool
StreamFifo::full() {
    //Real size is the number of fully complete entries
    return this->real_size() == this->max_size;
}

bool
StreamFifo::empty() {
    return fifo_container->size() == 0;
}

uint16_t
StreamFifo::real_size() {
    uint16_t size = 0;
    auto it = fifo_container->begin();
    while (it != fifo_container->end()) {
        if (it->complete()) size ++;
        it++;
    }
    return size;
}

void
FifoEntry::merge_data(uint8_t * data, uint16_t offset, uint16_t _size) {
    auto my_data = this->raw_ptr<uint8_t>();
    for (int i = 0; i < _size; i++) {
        (my_data+offset)[i] =  data[i];
    }
    csize += _size;
    assert(!(csize > size));
    if (csize == size && rstate == States::Complete) {
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