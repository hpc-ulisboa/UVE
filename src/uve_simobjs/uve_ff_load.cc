#include "uve_simobjs/uve_ff_load.hh"

UVELoadFifo::UVELoadFifo(UVEStreamingEngineParams *params)
    : SimObject(params),
      fifos(32),
      cacheLineSize(params->system->cacheLineSize()),
      confParams(params),
      fifo_depth(confParams->fifo_depth) {
    for (int i = 0; i < fifos.size(); i++) {
        fifos[i] = new StreamFifo(confParams->width, confParams->fifo_depth,
                                  confParams->max_request_size, i,
                                  true /*is of load type*/);
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
        if (fifos[i]->ready().isOk()) {
            info->psids[++k] = i;
        }
        fifo_occupancy[i].sample(fifos[i]->entries());
    }
    if (k != -1) {
        info->psids_size = k + 1;
        return true;
    }
    return false;
}

SmartReturn
UVELoadFifo::getData(StreamID sid) {
    // Check if fifo is ready and if so, organize data to the cpu;
    if (fifos[sid]->ready().isOk()) {
        auto entry = fifos[sid]->get();
        entry_time[sid].sample(Cycles(curTick() - entry.time()));
        return SmartReturn::ok((void *)new CoreContainer(entry));
    } else {
        return SmartReturn::nok();
    }
}

void
UVELoadFifo::synchronizeLists(StreamID sid) {
    fifos[sid]->synchronizeLists();
}

void
UVELoadFifo::reserve(StreamID sid, SubStreamID ssid, uint8_t size,
                     uint8_t width, bool dim_end,
                     bool * end_status) {
    // Reserve space in fifo. This reserve comes from the engine
    SmartReturn result = fifos[sid]->full();
    if (result.isError() || result.isTrue()) {
        std::stringstream s;
        s << "Error trying to reserve on fifo " << (int)sid
          << " with no space available. " << (result.isError() ? " Error" : "")
          << "\n";
        panic(s.str());
    }
    fifos[sid]->insert(size, ssid, width, dim_end, end_status);
}

SmartReturn
UVELoadFifo::insert(StreamID sid, SubStreamID ssid, CoreContainer data) {
    //If not empty try and merge
    //  Not Succeded: Create new entry
    //      If full: Warn no more space
    //      Else: Create new entry
    fifos[sid]->empty().NASSERT();

    if (fifos[sid]->merge_data(ssid, data.raw_ptr<uint8_t>()).isOk()) {
        return SmartReturn::ok();
    }
    return SmartReturn::nok();
}

SmartReturn
UVELoadFifo::full(StreamID sid) {
    return fifos[sid]->full();
}

SmartReturn
UVELoadFifo::ready(StreamID sid) {
    return fifos[sid]->ready();
}

SmartReturn
UVELoadFifo::squash(StreamID sid) {
    return fifos[sid]->squash();
}

SmartReturn
UVELoadFifo::shouldSquash(StreamID sid) {
    return fifos[sid]->shouldSquash();
}

SmartReturn
UVELoadFifo::commit(StreamID sid) {
    return fifos[sid]->commit();
}

SmartReturn
UVELoadFifo::clear(StreamID sid) {
    delete fifos[sid];
    fifos[sid] = new StreamFifo(confParams->width, confParams->fifo_depth,
                                confParams->max_request_size, sid);

    return SmartReturn::ok();
}

SmartReturn
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
UVELoadFifo::regStats() {
    SimObject::regStats();
    using namespace Stats;
    // Cycles at 0, 25, 50, 75, 90, 100. %
    fifo_occupancy.init(32, 0, fifo_depth, 1)
        .name(name() + ".load.occupancy")
        .desc("Occupancy of the fifo for stream x.");
    // Cycles per entry (avg, max, min)
    entry_time.init(32, 0, 1000, 100)
        .name(name() + ".load.entry_time")
        .desc("Time a entry is in the fifo x, in cycles");
}

void
StreamFifo::insert(uint16_t size, SubStreamID ssid, uint8_t width,
                   bool dim_end, bool * end_status) {
    //Dont just push back, need to separate data into containers;
    status = SEIterationStatus::Configured;
    uint16_t id = 0;
    //Offset sets the beggining of the data pointer; 0 in new entries;
    //Incremented by the size of the last reserve in already present entries
    uint16_t offset = 0;
    //First insertion case:
    if (this->empty().isOk()) {
        //Create new entry
        fifo_container->push_front(FifoEntry(width, config_size, my_id, ssid));
        speculationPointer++;
        DPRINTF(UVEFifo,
                "Inserted on back of fifo[%d] ssid[%d]: Was empty. Id is %d. "
                "Speculation Pointer targeting ssid[%d]. DimEnd:%c\n",
                my_id, ssid, 0, speculationPointer->get_ssid(),
                dim_end ? 'T' : 'F');
        //Reserve space in entry
        assert(fifo_container->front().reserve(&size, end_status, dim_end));
        DPRINTF(UVEFifo, "%s\n", print_fifo());
        //First entry was created, set id to 0;
        this->map.push_back(create_MS(id,size,offset));
        return;
    }
    //Get last entry: this should be the place where to put data;
    //This insert is called in order
    if (fifo_container->front().complete()) {
        this->full().NASSERT();
        // Create new entry due to the last one being complete
        fifo_container->push_front(FifoEntry(width, config_size, my_id, ssid));
        //New entry was created, increment id;
        id = this->map.back().id + 1;
        //Here we absolutelly need to be able to reserve
        assert(fifo_container->front().reserve(&size, end_status, dim_end));

        this->map.push_back(create_MS(id, size, offset));
        DPRINTF(UVEFifo,
                "Inserted on back of fifo[%d] ssid[%d]. Id is %d. "
                "Size[%d].Speculation Pointer targeting ssid[%d]. DimEnd:%c\n",
                my_id, ssid, id, fifo_container->size(),
                speculationPointer->get_ssid(), dim_end ? 'T' : 'F');
        DPRINTF(UVEFifo, "%s\n", print_fifo());
        return;
    }
    // Reserve the space -> When returns false the size is updated to the
    // remaining size
    uint16_t changeable_size = size;
    bool result = fifo_container->front().reserve(&changeable_size, end_status,
                                                 dim_end);
    if (!result) {
        // Means that reserve is bigger than available space:
        // Create new entry
        fifo_container->push_front(FifoEntry(width, config_size, my_id, ssid));
        // Restore pointer
        // New entry was created, increment id;
        if (this->map.back().split) {
            id = this->map.back().id2;
            offset = this->map.back().offset2 + this->map.back().size2;
        } else {
            id = this->map.back().id;
            offset = this->map.back().offset + this->map.back().size;
        }
        assert(fifo_container->front().reserve(&changeable_size, end_status,
                                                dim_end));

        this->map.push_back(create_MS(id, id + 1, size - changeable_size,
                                      changeable_size, offset, 0));
        DPRINTF(
            UVEFifo,
            "Inserted on back of fifo[%d] ssid[%d]. Id is %d. Splited "
            "Insertion. Size[%d]. Speculation Pointer targeting ssid[%d]."
            "DimEnd:%c\n",
            my_id, ssid, id, fifo_container->size(),
            speculationPointer->get_ssid(), dim_end ? 'T' : 'F');
        DPRINTF(UVEFifo, "%s\n", print_fifo());
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
        DPRINTF(
            UVEFifo,
            "Inserted on same entry of fifo[%d] ssid[%d]. Id is %d. Splited "
            "Insertion. Size[%d]. Speculation Pointer targeting ssid[%d]."
            "DimEnd:%c\n",
            my_id, ssid, id, fifo_container->size(),
            speculationPointer->get_ssid(), dim_end ? 'T' : 'F');
        return;
    }
}

SmartReturn
StreamFifo::merge_data(SubStreamID ssid, uint8_t *data) {
    empty().NASSERT();
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

    DPRINTF(UVEFifo, "Data Merged on fifo[%d] ssid[%d]. Id is %d. %s\n", my_id,
            ssid, mapping.id, mapping.split ? "Splited Merge" : "");
    DPRINTF(UVEFifo, "%s\n", print_fifo());
    return SmartReturn::ok();
}

SmartReturn
StreamFifo::merge_data_store(SubStreamID ssid, uint8_t *data, uint16_t valid) {
    empty().NASSERT();
    // Get corresponding fifo entry
    auto it = this->map.begin() + ssid;
    auto mapping = *it;
    assert(!mapping.used);

    auto list_it = fifo_container->rbegin();
    advance(list_it, mapping.id);
    list_it->merge_data(data, mapping.offset, MIN(mapping.size, valid));

    if (mapping.split) {
        list_it = fifo_container->rbegin();
        advance(list_it, mapping.id2);
        list_it->merge_data(data + mapping.size, mapping.offset2,
                            MIN(mapping.size2, valid - mapping.size));
    }

    it->used = true;

    DPRINTF(UVEFifo,
            "Data Merged on fifo[%d] ssid[%d] size[%d]. Id is %d. %s\n", my_id,
            ssid, valid, mapping.id, mapping.split ? "Splited Merge" : "");
    DPRINTF(UVEFifo, "%s\n", print_fifo());
    return SmartReturn::ok();
}

FifoEntry
StreamFifo::get() {
    empty().NASSERT();

    FifoEntry specEntry = *speculationPointer;
    speculationPointer++;
    DPRINTF(UVEFifo,
            "Get of fifo[%d]. Size[%d]. elements(%d) Speculation Pointer targeting "
            "ssid[%d]. %s\n",
            my_id, fifo_container->size(), specEntry.getSize(), speculationPointer->get_ssid(),
            specEntry.is_last(DimensionHop::last) ? "Last Get." : "");
    DPRINTF(UVEFifo, "%s\n", print_fifo());

    if (specEntry.is_last(DimensionHop::last)) {
        // Set state as complete
        status = SEIterationStatus::Ended;
    }
    return specEntry;
}

SmartReturn
StreamFifo::ready() {
    if (empty().isOk()) return SmartReturn::nok();
    if (!speculationPointer.valid()) SmartReturn::nok();
    FifoEntry first_entry = *speculationPointer;
    return SmartReturn::compare(first_entry.ready());
}

void
StreamFifo::synchronizeLists() {
    speculationPointer.forceUpdate();
}

SmartReturn
StreamFifo::commit() {
    if (empty().isOk()) return SmartReturn::error();

    auto elem = fifo_container->back();

    fifo_container->pop_back();
    speculationPointer--;
    DPRINTF(UVEFifo,
            "Commit on fifo[%d] ssid[%d]. Size[%d]. Speculation Pointer "
            "targeting ssid[%d]. %s\n",
            my_id, elem.get_ssid(), fifo_container->size(),
            speculationPointer->get_ssid(),
            elem.is_last(DimensionHop::last) ? "Last Commit." : "");
    DPRINTF(UVEFifo, "%s\n", print_fifo());
    // Decrease by 1 all the id pointers in the map
    for (auto t = map.begin(); t != map.end(); ++t) {
        t->id = t->id <= 0 ? 0 : t->id - 1;
        if (t->split) t->id2 = t->id2 <= 0 ? 0 : t->id2 - 1;
    }

    if (elem.is_last(DimensionHop::last)) {
        // Stream Ended
        // Return End Code
        return SmartReturn::end();
    }

    return SmartReturn::ok();
}

SmartReturn
StreamFifo::squash() {
    if (empty().isOk()) return SmartReturn::error();
    speculationPointer--;
    DPRINTF(UVEFifo,
            "Squash on fifo[%d]. Size[%d]. Pointer Targeting ssid[%d].\n",
            my_id, fifo_container->size(), (*speculationPointer).get_ssid());
    DPRINTF(UVEFifo, "%s\n", print_fifo());
    return SmartReturn::ok();
}

SmartReturn
StreamFifo::shouldSquash() {
    bool spec_iter_out_of_bounds = !speculationPointer.validOnDecrement();
    //JMNOTE: If the fifo is for store, ignore the speculationPointer.
    if (!load_nstore) spec_iter_out_of_bounds = false;
    if (empty().isOk() || spec_iter_out_of_bounds)
        return SmartReturn::nok();
    else
        return SmartReturn::ok();
}

SmartReturn
StreamFifo::full() {
    //Real size is the number of fully complete entries
    return SmartReturn::compare(this->real_size() == this->total_size);
}

SmartReturn
StreamFifo::empty() {
    return SmartReturn::compare(fifo_container->size() == 0);
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

SmartReturn
StreamFifo::storeReady() {
    // Checks if the last element of the fifo is ready to go
    if (fifo_container->empty()) return SmartReturn::nok();
    return SmartReturn::compare(fifo_container->back().is_ready_to_commit());
}

SmartReturn
StreamFifo::storeSquash(SubStreamID ssid) {
    // Squashes (removes) entry pointed by ssid
    auto iter = fifo_container->begin();
    while (iter != fifo_container->end()) {
        if (iter->get_ssid() == ssid) {
            // Remove entry
            fifo_container->erase(iter);
            speculationPointer--;
            DPRINTF(UVEFifo, "%s\n", print_fifo());
            return SmartReturn::ok();
        }
        iter++;
    }
    return SmartReturn::nok();
}

SmartReturn
StreamFifo::storeCommit(SubStreamID ssid) {
    // Marks the entry pointed by ssid as ready
    auto iter = fifo_container->rbegin();
    while (iter != fifo_container->rend()) {
        if (iter->get_ssid() == ssid) {
            iter->set_ready_to_commit();
            DPRINTF(UVEFifo, "%s\n", print_fifo());
            return SmartReturn::ok();
        }
        iter++;
    }
    return SmartReturn::nok();
}

SmartReturn
StreamFifo::storeDiscard(SubStreamID ssid) {
    if (empty().isOk()) return SmartReturn::error();

    auto elem = fifo_container->back();

    fifo_container->pop_back();
    speculationPointer--;
    // Decrease by 1 all the id pointers in the map
    for (auto t = map.begin(); t != map.end(); ++t) {
        t->id = t->id <= 0 ? 0 : t->id - 1;
        if (t->split) t->id2 = t->id2 <= 0 ? 0 : t->id2 - 1;
    }

    if (elem.is_last(DimensionHop::last)) {
        // Stream Ended
        // Return End Code
        return SmartReturn::end();
    }

    return SmartReturn::ok();
}

FifoEntry
StreamFifo::storeGet() {
    empty().NASSERT();

    FifoEntry entry = fifo_container->back();
    storeDiscard(entry.get_ssid());
    SmartReturn::compare(entry.is_ready_to_commit()).ASSERT();

    DPRINTF(UVEFifo, "Store Get of fifo[%d]. Size[%d]. SSID[%d] %s\n", my_id,
            fifo_container->size(), entry.get_ssid(),
            entry.is_last(DimensionHop::last) ? "Last Get." : "");
    DPRINTF(UVEFifo, "%s\n", print_fifo());
    return entry;
}

void
FifoEntry::merge_data(uint8_t *data, uint16_t offset, uint16_t _size) {
    auto my_data = this->raw_ptr<uint8_t>();
    for (uint16_t i = 0; i < _size; i++) {
        (my_data+offset)[i] =  data[i];
    }
    csize += _size;
    assert(!(csize > size));
    set_valid(MIN(csize, size));
    if (csize == size && rstate == States::Complete) {
        cstate = States::Complete;
    }
    else cstate = States::NotComplete;
}

bool
FifoEntry::reserve(uint16_t *_size, bool * end_status, bool dim_end) {
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
        for(int i = DimensionHop::first; i != DimensionHop::dh_size; i++){
            DimensionHop hop = static_cast<DimensionHop>(i);
            set_last(hop, end_status[i]);
        }
        return true;
    }
    else{
        if (dim_end){
            for(int i = DimensionHop::first; i != DimensionHop::dh_size; i++){
                DimensionHop hop = static_cast<DimensionHop>(i);
                set_last(hop, end_status[i]);
            }
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