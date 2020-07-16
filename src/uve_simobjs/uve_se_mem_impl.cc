
#include "base/chunk_generator.hh"
#include "debug/UVESE.hh"
#include "uve_simobjs/uve_streaming_engine.hh"

SEprocessing::SEprocessing(UVEStreamingEngineParams *params,
                           UVEStreamingEngine *_parent)
    : SimObject(params),
      tlb(params->tlb),
      offsetMask(mask(floorLog2(RiscvISA::PageBytes))),
      cacheLineSize(params->system->cacheLineSize()),
      write_boss(),
      outstanding_errors(32),
      outstanding_requests(32) {
    parent = _parent;
    write_boss.set_owner(this);
    for (int i = 0; i < 32; i++) {
        iterQueue[i] = new SEIter();
    }
    load_pID = 0;
    store_pID = 0;
    load_pID--;
    store_pID--;
    ssidArray.fill(-1);
};

void
SEprocessing::tick_load() {
    uint16_t max_advance_size = 0;
    load_pID++;
    if (load_pID > 31) load_pID = 0;
    uint8_t load_auxID = load_pID + 1;
    if (load_auxID > 31) load_auxID = 0;

    while (1) {
        if (load_auxID == load_pID) {
            // Found no candidate
            return;
        }
        if (!iterQueue[load_auxID]->is_load()) {
            // Not a load -> next stream
            load_auxID++;
            if (load_auxID > 31) load_auxID = 0;
        } else {
            max_advance_size = parent->ld_fifo.getAvailableSpace(load_auxID);
            if (max_advance_size > 0 && !iterQueue[load_auxID]->empty()) {
                // We find a candidate if it is supposed to advance: has space
                // and is configured
                break;
            }
            load_auxID++;
            if (load_auxID > 31) load_auxID = 0;
        }
    }

    DPRINTF(JMDEVEL, PR_INFO("Settled on load_auxID:%d advance_size(%d B)\n"),
            load_auxID, max_advance_size / 8);
    parent->streamProcessingCycles[load_auxID]++;
    load_pID = load_auxID;
    SERequestInfo request_info =
        iterQueue[load_pID]->advance(max_advance_size);
    if (request_info.stop_reason == DimensionSwap) {
        asm("nop\n\t");
    }
    DPRINTF(UVESE, PR_ANN("iterator sid:(%d) ssid:(%d): %s\n"), load_pID,
            request_info.sequence_number, iterQueue[load_pID]->print_state());
    DPRINTF(UVESE, PR_INFO(
            "Memory Request[%s][%d] sz[%d B]: [%d->%d] w(%d)  StopReason:%s\n")
            , request_info.mode == StreamMode::load ? "load" : "store",
            request_info.sequence_number,
            (request_info.final_offset - request_info.initial_offset),
            request_info.initial_offset, request_info.final_offset,
            request_info.width,
            (request_info.stop_reason == StopReason::DimensionSwap
                 ? "Dimension Swap"
                 : (request_info.stop_reason == StopReason::BufferFull
                        ? "Buffer Full"
                        : "End or Strided")));
    if (request_info.status == SEIterationStatus::Ended)
        DPRINTF(UVESE, PR_WARN("Iteration on stream %d ended!\n"), load_pID);
    // Create and send request to memory
    emitRequest(request_info);
};

void
SEprocessing::tick_store() {
    // JMFIXME: 512 should be the width of an element.. Can also be the size of
    // a store fifo.. Never more
    uint16_t max_advance_size = 512;
    store_pID++;
    if (store_pID > 31) store_pID = 0;
    uint8_t store_auxID = store_pID + 1;
    if (store_auxID > 31) store_auxID = 0;

    while (1) {
        if (store_auxID == store_pID) {
            // Found no candidate
            return;
        }
        if (iterQueue[store_auxID]->is_load()) {
            // Not a store -> next stream
            store_auxID++;
            if (store_auxID > 31) store_auxID = 0;
        } else {
            // max_advance_size =
            // parent->st_fifo.getAvailableSpace(store_auxID); if
            // (max_advance_size > 0 && !iterQueue[store_auxID]->empty()) {
            if (!iterQueue[store_auxID]->empty()) {
                // We find a candidate if it is supposed to advance: has space
                // and is configured
                break;
            }
            store_auxID++;
            if (store_auxID > 31) store_auxID = 0;
        }
    }

    DPRINTF(JMDEVEL, PR_ANN("Settled on store_auxID:%d\n"), store_auxID);
    parent->streamProcessingCycles[store_auxID]++;
    store_pID = store_auxID;
    SERequestInfo request_info =
        iterQueue[store_pID]->advance(max_advance_size);
    DPRINTF(UVESE, PR_INFO("Memory Request[%s][%d]: [%d->%d] w(%d)\n"),
            request_info.mode == StreamMode::load ? "load" : "store",
            request_info.sequence_number, request_info.initial_offset,
            request_info.final_offset, request_info.width);
    if (request_info.status == SEIterationStatus::Ended)
        DPRINTF(UVESE, PR_WARN("Iteration on stream %d ended!\n"), store_pID);
    //Create and send request to memory
    emitRequest(request_info);
};

void
SEprocessing::tick() {
    // DPRINTF(JMDEVEL, "SEprocessing::Tick()\n");
    tick_load();
    tick_store();
    write_boss.tick();
    return;
}

SmartReturn
SEprocessing::clear(StreamID sid) {
    if (iterQueue[sid]->ended().isOk()) {
        Cycles _time = Cycles(curTick() - iterQueue[sid]->time());
        parent->sampleStreamExecutionCycles(sid, _time);
        parent->incStreamCompleted(sid);
    } else
        parent->incStreamSquashed(sid);
    delete iterQueue[sid];
    iterQueue[sid] = new SEIter();
    this->ssidArray[sid] = -1;
    write_boss.squash(sid);

    outstanding_errors[sid] = outstanding_requests[sid];
    outstanding_requests[sid].empty();

    return SmartReturn::ok();
}

void
SEprocessing::emitRequest(SERequestInfo info){
    // this->schedule(new RequestExecuteEvent(this, info),
    //     parent->nextCycle());
    executeRequest(info);
}

void
SEprocessing::executeRequest(SERequestInfo info){

    DPRINTF(UVESE,PR_ANN("Execute Memory Request[%d]: [%d->%d] w(%d)\n"),
    info.sequence_number, info.initial_offset,
    info.final_offset, info.width );
    uint64_t size = info.final_offset- info.initial_offset;
    if (info.mode == StreamMode::load) {
        accessMemory(info.initial_offset, size, info.sid, info.sequence_number,
                     BaseTLB::Mode::Read, nullptr, info.tc);
    } else {
        write_boss.queueMemoryWrite(info);
    }
}

void
SEprocessing::accessMemory(Addr addr, int size, StreamID sid, SubStreamID ssid,
                           BaseTLB::Mode mode, uint8_t *data,
                           ThreadContext *tc) {
    Addr paddr;
    if (mode == BaseTLB::Mode::Write ||
        !iterQueue[sid]->translatePaddr(&paddr)) {
        //Create new virtual request if:
            //Translation is needed
        RequestPtr req = std::make_shared<Request>();
        req->setVirt(0,addr,size,0,0,0);
        req->setStreamId(sid);
        req->setSubStreamId(ssid);
        // req->setFlags(Request::PREFETCH);
        DPRINTF(JMDEVEL, PR_DBG("Translating for addr %#x\n"),
                     req->getVaddr());

        // Check if the data is only in one page:
        if (!this->isSinglePage(addr, size)) {
            // In this case we need to split the request in two
            RequestPtr req1, req2;
            req->splitOnVaddr(this->splitAddressOnPage(addr, size), req1,
                              req2);

            WholeTranslationState *state =
                new WholeTranslationState(req, req1, req2, data, NULL, mode);
            DataTranslation<SEprocessing *> *trans1 =
                new DataTranslation<SEprocessing *>(this, state, 0);
            DataTranslation<SEprocessing *> *trans2 =
                new DataTranslation<SEprocessing *>(this, state, 1);

            tlb->translateTiming(req1, tc, trans1, mode);
            tlb->translateTiming(req2, tc, trans2, mode);
        } else {
            WholeTranslationState *state =
                new WholeTranslationState(req, data, NULL, mode);
            DataTranslation<SEprocessing *> *translation =
                new DataTranslation<SEprocessing *>(this, state);

            // This callsback finishTranslation;
            tlb->translateTiming(req, tc, translation, mode);
        }

    }
    else {
        //Create physical request
        RequestPtr req = std::make_shared<Request>(paddr, size, 0, 0);
        req->setStreamId(sid);
        req->setSubStreamId(ssid);
        // req->setFlags(Request::PREFETCH);
        sendDataRequest(req, data, mode == BaseTLB::Read);
        iterQueue[sid]->setPaddr(paddr, false);
    }
}


void
SEprocessing::finishTranslation(WholeTranslationState *state)
{
    if (state->getFault() != NoFault) {
        // JMFIXME: Must continue with the processing. Executing with
        // speculation can give rise to page faults
        // panic("Page fault in SEprocessing. Addr: %#x, Name: %s\n",
        //       state->mainReq->getVaddr(), state->getFault()->name());
        DPRINTF(JMDEVEL, PR_ERR(
                "Page fault found, prooceding. For now doing nothing.\n"));
        delete state;
        return;
    }

    //Save the physical addr in the iterator
    auto sid = state->mainReq->streamId();

    if (!state->isSplit) {
        DPRINTF(JMDEVEL, PR_INFO("Got response for translation. %#x -> %#x\n"),
                state->mainReq->getVaddr(), state->mainReq->getPaddr());
        sendDataRequest(state->mainReq, state->data,
                        state->mode == BaseTLB::Read);
        iterQueue[sid]->setPaddr(state->mainReq->getPaddr(), false);
    } else {
        DPRINTF(JMDEVEL, PR_INFO(
                "Got response for split translation. 1(%#x -> %#x) 2(%#x -> "
                "%#x)\n"),
                state->sreqLow->getVaddr(), state->sreqLow->getPaddr(),
                state->sreqHigh->getVaddr(), state->sreqHigh->getPaddr());
        sendSplitDataRequest(state->sreqLow, state->sreqHigh, state->mainReq,
                             state->data, state->mode == BaseTLB::Read);
        iterQueue[sid]->setPaddr(state->sreqHigh->getPaddr(), true,
                                 state->sreqHigh->getVaddr());
    }

    delete state;
}

void
SEprocessing::sendDataRequest(RequestPtr ireq, uint8_t *data, bool read,
                              bool split_not_last) {
    DPRINTF(JMDEVEL, PR_INFO("Sending request for addr [P%#x]\n"),
     ireq->getPaddr());

    RequestPtr req = NULL;
    auto sid = ireq->streamId();
    auto width = iterQueue[sid]->getCompressedWidth();
    bool ended = iterQueue[sid]->ended().isOk();

    for (ChunkGenerator gen(ireq->getPaddr(), ireq->getSize(), cacheLineSize);
         !gen.done(); gen.next()) {
        // JMNOTE: We need to make sure that a page is not being crossed
        // If so: Go back with the iteration: For that the next cycle takes
        // care of that

        Request::FlagsType flags = 0;  // = Request::PREFETCH;
        req = std::make_shared<Request>(gen.addr(), gen.size(), flags, 0);
        SubStreamID old_ssid = ireq->substreamId();
        SubStreamID ssid = ++ssidArray[sid];
        req->setStreamId(sid);
        req->setSubStreamId(ssid);

        // if (!parent->ld_fifo.full(sid)){
        bool last_packet = (ended && gen.last() && !split_not_last);
        if (last_packet) DPRINTF(JMDEVEL, PR_WARN("Last packet for sid (%d)\n")
                                , sid);
        if (read)
            parent->ld_fifo.reserve(sid, ssid, gen.size(), width, last_packet);

        req->taskId(ContextSwitchTaskId::DMA);
        PacketPtr pkt =
            read ? Packet::createRead(req) : Packet::createWrite(req);

        // Increment the data pointer on each request
        uint8_t *data_blk = new uint8_t[gen.size()];
        if (!read) {
            memcpy(data_blk, data + gen.complete(), gen.size());
        }
        pkt->dataDynamic(data_blk);

        if (read)
            DPRINTF(JMDEVEL, PR_INFO(
                    "--[%s] Queuing for addr: %#x size: %d ssid[%d]\n"),
                    read ? "Read" : "Write", gen.addr(), gen.size(), ssid);
        else
            DPRINTF(JMDEVEL, PR_INFO(
                    "--[%s] Queuing for addr: %#x size: %d ssid[%d] old[%d]\n")
                    , read ? "Read" : "Write", gen.addr(), gen.size(), ssid,
                    old_ssid);
        parent->getMemPort(read /*read=>Load/!Write*/)
            ->schedTimingReq(pkt, parent->nextAvailableCycle());
        parent->numStreamMemAccesses[sid]++;

        if (read) outstanding_requests[sid].insert(ssid);
        if (!read) {
            if (last_packet) {
                if (write_boss.is_end(sid, old_ssid))
                    parent->endStreamStore(sid);
            }
        }
    }
}

void
SEprocessing::sendSplitDataRequest(const RequestPtr &req1,
                                   const RequestPtr &req2,
                                   const RequestPtr &req, uint8_t *data,
                                   bool read) {
    DPRINTF(JMDEVEL, PR_INFO("Sending split request for addrs [P%#x] & "
                            "[P%#x]\n"), req1->getPaddr(), req2->getPaddr());

    sendDataRequest(req1, data, read, true);
    sendDataRequest(req2, data + req1->getSize(), read, false);
}

template <typename T>
std::string
v_print(T data, int size = -1) {
    if (size == -1) {
        return data.print();
    }
    std::stringstream stm;
    stm << "[";

    for (int i = 0; i<size; i++){
        stm << "0x"<< std::hex << (uint64_t)data[i] <<" ";
    }

    stm << "]";
    return stm.str();
}

void
SEprocessing::recvData(PacketPtr pkt){
    RiscvISA::VecRegContainer memData;
    memData.zero();
    StreamID sid = pkt->req->streamId();
    uint64_t ssid = pkt->req->substreamId();

    pkt->writeData(memData.raw_ptr<uint8_t>());
    //Need to know width and destination stream..
    std::stringstream str;

    SEIterPtr iter = iterQueue[sid];
    // Special case for in-fligth transactions on canceled streams
    if (outstanding_errors[sid].find(ssid)
            != outstanding_errors[sid].end()){
        DPRINTF(
            JMDEVEL, PR_WARN(
            "Iteration Object with sid %d and ssid %d was invalid. "
            "In-fligth transaction detected.\n"),
            sid, ssid);
        outstanding_errors[sid].erase(ssid);
        return;
    }
    if (iter->empty() && iter->ended().isNok()) {
        DPRINTF(
            JMDEVEL, PR_WARN(
            "Iteration Object with sid %d was empty. In-fligth transaction "
            "detected.\n"),
            sid);
        return;
    }

    if (outstanding_requests[sid].find(ssid)
            != outstanding_requests[sid].end()){

        outstanding_requests[sid].erase(ssid);

        uint8_t width = iter->getWidth();
        int size = pkt->getSize()/width;

        switch(width){
            case 1:
                DPRINTF(JMDEVEL, PR_ANN("Data(%d:%d) w(%d)\n%s\n")
                        , sid, ssid, width,
                        v_print(memData.as<uint8_t>(), size));
                break;
            case 2:
                DPRINTF(JMDEVEL, PR_ANN("Data(%d:%d) w(%d)\n%s\n"),
                        sid, ssid, width,
                        v_print(memData.as<uint16_t>(), size));
                break;
            case 4:
                DPRINTF(JMDEVEL, PR_ANN("Data(%d:%d) w(%d)\n%s\n"),
                        sid, ssid, width,
                        v_print(memData.as<uint32_t>(), size));
                break;
            default:
                DPRINTF(JMDEVEL, PR_ANN("Data(%d:%d) w(%d)\n%s\n"),
                        sid, ssid, width,
                        v_print(memData.as<uint64_t>(), size));
        }
        //Insert data into fifo
        parent->ld_fifo.insert(sid, ssid, memData);
        Tick start_time = pkt->req->time();
        Tick end_time = curTick();
        parent->memRequestCycles[sid].sample(Cycles(end_time - start_time));
        parent->numStreamMemBytes[sid] += pkt->getSize();
        return;
    }

    DPRINTF(
            JMDEVEL, PR_ERR(
            "Iteration Object with sid %d was not matched."
            " Something strange occured.\n"),
            sid);
    return;
}

void
RequestExecuteEvent::process(){
    callee->executeRequest(info);
}

void
SEprocessing::MemoryWriteHandler::tick() {
    // Each tick it tries to write data into memory
    SmartReturn res = consume_data();
    // 2.1. If there is data to write
    if (res.isOk()) {
        // Exc: If partial data is available, no need for new addr
        if (has_partial_data) {
            res = write_mem_partial();
            if (res.isNok()) {
                // Another partial request is ongoing
                return;
            }
            return;
        }
        // 2.2. Get address
        res = consume_addr_unit();
        if (res.isOk()) {
            // 2.3. Send memory request
            res = write_mem((SERequestInfo *)res.getData());
            if (res.isNok()) {
                // No sufficient data for addr size
                return;
            }
        } else {
            // No addr available, try next cycle
            return;
        }
    } else {
        // No data available, try next cycle
        return;
    }
};

SmartReturn
SEprocessing::MemoryWriteHandler::consume_data() {
    // General consume, handles the data comming from fifo and the in-fligth
    // data
    if (has_data_block) {
        return SmartReturn::ok();
    } else {
        SmartReturn res = owner->parent->st_fifo.ready();
        if (res.isOk()) {
            StreamID *sid = ((StreamID *)res.getData());
            res = owner->parent->st_fifo.getData(*sid);
            free(sid);
            if (res.isOk()) {
                data_block = (CoreContainer *)res.getData();
                has_data_block = true;
                data_block_index = 0;
                return SmartReturn::ok();
            }
        }
        return SmartReturn::nok();
    }
}

SmartReturn
SEprocessing::MemoryWriteHandler::consume_data_unit(uint64_t sz) {
    // Size always in bytes
    // We only get here when we have data in a block
    SmartReturn::compare(has_data_block).ASSERT();

    // Get a data unit of size sz from the data block
    // Check if sz is smaller than available data size
    if (sz <= data_block->get_valid() - data_block_index) {
        uint8_t *data = (uint8_t *)malloc(sz * sizeof(uint8_t));
        memcpy(data, data_block->raw_ptr<uint8_t>() + data_block_index, sz);
        // Handle remains
        data_block_index += sz;
        owner->DEBUG_PRINT( PR_INFO(
            "data_block: has(%s) idx(%d) valid(%d) || req:sz(%d)\n"),
            has_data_block ? "T" : "F", data_block_index,
            data_block->get_valid(), sz);
        if (data_block_index >= data_block->get_valid()) {
            owner->DEBUG_PRINT( PR_INFO(
                "Data unit has same size as requested.. has_data_block = "
                "false\n"));
            has_data_block = false;
            delete data_block;
        }
        return SmartReturn::ok((void *)data);
    } else {
        // requested size is bigger than current available data
        uint64_t transaction_size = data_block->get_valid() - data_block_index;
        // need to delay transaction to next cycle
        partial_data = (uint8_t *)malloc(transaction_size * sizeof(uint8_t));
        memcpy(partial_data, data_block->raw_ptr<uint8_t>() + data_block_index,
               transaction_size);
        // Handle remains
        data_block_index += transaction_size;
        partial_data_size = transaction_size;
        has_partial_data = true;
        has_data_block = false;
        delete data_block;
        return SmartReturn::nok();
    }
}

SmartReturn
SEprocessing::MemoryWriteHandler::write_mem(SERequestInfo *info) {
    // Size always in bytes
    // We only get here when we have data in a block
    SmartReturn::compare(has_data_block).ASSERT();

    // Get last address size
    uint64_t sz = info->final_offset - info->initial_offset;

    uint8_t *mem_block = (uint8_t *)malloc(sz * sizeof(uint8_t));

    SmartReturn res = consume_data_unit(sz);
    if (res.isOk()) {
        uint8_t *new_rq_block = (uint8_t *)res.getData();
        memcpy(mem_block, new_rq_block, sz);
        free(new_rq_block);
        // Request is complete: issue to memory
        if (info->status == SEIterationStatus::Ended) {
            add_end(info->sid, info->sequence_number);
        }
        owner->DEBUG_PRINT(PR_INFO("Writing request, sid(%d) ssid(%d) "
                           "sz(%d)\n"),
                           info->sid, info->sequence_number, sz);
        owner->accessMemory(info->initial_offset, sz, info->sid,
                            info->sequence_number, BaseTLB::Mode::Write,
                            mem_block, info->tc);
        delete info;
        return SmartReturn::ok();
    } else {
        // Confirm that partial request happened
        assert(has_partial_data);
        delayed_address = info;
        owner->DEBUG_PRINT( PR_INFO(
            "Data was not sufficient for request, retrying next "
            "cycle, sid(%d) "
            "ssid(%d) new_sz(%d)\n"),
            delayed_address->sid, delayed_address->sequence_number,
            partial_data_size);
        return SmartReturn::nok();
    }
}

SmartReturn
SEprocessing::MemoryWriteHandler::write_mem_partial() {
    // Size always in bytes
    // We only get here when we have data in a block
    SmartReturn::compare(has_data_block && has_partial_data).ASSERT();

    // Get last address in delayed_address
    uint64_t sz =
        delayed_address->final_offset - delayed_address->initial_offset;

    uint8_t *mem_block = (uint8_t *)malloc(sz * sizeof(uint8_t));
    memcpy(mem_block, partial_data, partial_data_size);
    uint64_t new_rq_sz = sz - partial_data_size;
    free(partial_data);
    has_partial_data = false;
    uint64_t data_offset = partial_data_size;

    SmartReturn res = consume_data_unit(new_rq_sz);
    if (res.isOk()) {
        uint8_t *new_rq_block = (uint8_t *)res.getData();
        memcpy(mem_block + partial_data_size, new_rq_block, new_rq_sz);
        free(new_rq_block);
        // Request is complete: issue to memory
        if (delayed_address->status == SEIterationStatus::Ended) {
            add_end(delayed_address->sid, delayed_address->sequence_number);
        }
        owner->DEBUG_PRINT( PR_INFO(
            "Writing partial request, sid(%d) ssid(%d) sz(%d)\n"),
            delayed_address->sid, delayed_address->sequence_number, sz);
        owner->accessMemory(
            delayed_address->initial_offset, sz, delayed_address->sid,
            delayed_address->sequence_number, BaseTLB::Mode::Write, mem_block,
            delayed_address->tc);
        delete delayed_address;
        return SmartReturn::ok();
    } else {
        // Something wrong happened...Maybe requested size still too big.
        if (has_partial_data) {
            // Not touch delayed_address
            // Merge old data in mem_nlock with new data in partial_data.
            // Need to move mem_block's to beggining of partial_data
            uint8_t *aux_block = (uint8_t *)malloc(
                (data_offset + partial_data_size) * sizeof(uint8_t));
            memcpy(aux_block, mem_block, data_offset);
            memcpy(aux_block + data_offset, partial_data, partial_data_size);
            free(partial_data);
            free(mem_block);
            partial_data_size = data_offset + partial_data_size;
            partial_data = aux_block;
            owner->DEBUG_PRINT( PR_WARN(
                "Data was not sufficient for partial request, retrying next "
                "cycle, sid(%d) "
                "ssid(%d) new_sz(%d)\n"),
                delayed_address->sid, delayed_address->sequence_number,
                partial_data_size);
        } else {
            assert(false && "Unnexpected error in write_mem_partial.");
        }
        return SmartReturn::nok();
    }
}

SmartReturn
SEprocessing::MemoryWriteHandler::consume_addr_unit() {
    // Check if there is an address in the queue, return it
    if (!addr_queue.empty()) {
        auto _back = addr_queue.back();
        while (_back.first == -1) {
            delete _back.second;
            addr_queue.pop_back();
            _back = addr_queue.back();
        }
        addr_queue.pop_back();
        owner->DEBUG_PRINT(PR_ANN("ConsumeAQ %s\n"), print_addr_queue());
        return SmartReturn::ok((void *)_back.second);
    } else
        return SmartReturn::nok();
}

void
SEprocessing::MemoryWriteHandler::squash(StreamID sid) {
    auto iter = addr_queue.begin();
    while (iter != addr_queue.end()) {
        if (iter->first == sid) iter->first = -1;
        iter++;
    }
    owner->DEBUG_PRINT(PR_INFO("SquashAQ %s\n"), print_addr_queue());
}

void
SEprocessing::MemoryWriteHandler::queueMemoryWrite(SERequestInfo info) {
    addr_queue.push_front(
        std::make_pair((StreamID)info.sid, new SERequestInfo(info)));
    owner->DEBUG_PRINT(PR_INFO("QueuedAQ %s\n"), print_addr_queue());
}
