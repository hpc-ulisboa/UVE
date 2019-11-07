
#include "base/chunk_generator.hh"
#include "debug/UVESE.hh"
#include "uve_simobjs/uve_streaming_engine.hh"

SEprocessing::SEprocessing(UVEStreamingEngineParams *params,
                UVEStreamingEngine *_parent) : SimObject(params),
                tlb(params->tlb),
                offsetMask(mask(floorLog2(RiscvISA::PageBytes))),
                cacheLineSize(params->system->cacheLineSize())
                {
    parent = _parent;
    iterQueue.fill(new SEIter());
    ssidArray.fill(-1);
};

void SEprocessing::tick(){
    // DPRINTF(JMDEVEL, "SEprocessing::Tick()\n");

    uint16_t max_advance_size;
    pID++;
    if (pID > 31) pID = 0;
    uint8_t auxID = pID + 1;
    if (auxID > 31) auxID = 0;

    max_advance_size = parent->ld_fifo.getAvailableSpace(auxID);
    while (iterQueue[auxID]->empty() || max_advance_size <= 0) {
        // One passage determines if no stream is configured
        if (auxID == pID) {
            // DPRINTF(JMDEVEL, "Blanck Tick\n");
            return;
        }
        auxID++;
        if (auxID > 31) auxID = 0;
        max_advance_size = parent->ld_fifo.getAvailableSpace(auxID);
    }

    DPRINTF(JMDEVEL, "Settled on auxID:%d\n", auxID);
    pID = auxID;
    SERequestInfo request_info = iterQueue[pID]->advance(max_advance_size);
    DPRINTF(UVESE, "Memory Request[%d]: [%d->%d] w(%d)\n",
            request_info.sequence_number, request_info.initial_offset,
            request_info.final_offset, request_info.width);
    if (request_info.status == SEIterationStatus::Ended)
        DPRINTF(UVESE,"Iteration on stream %d ended!\n", pID);
    //Create and send request to memory
    emitRequest(request_info);
    return;
}

void
SEprocessing::emitRequest(SERequestInfo info){
    this->schedule(new RequestExecuteEvent(this, info),
        parent->nextCycle());
}

void
SEprocessing::executeRequest(SERequestInfo info){

    DPRINTF(UVESE,"Execute Memory Request[%d]: [%d->%d] w(%d)\n",
    info.sequence_number, info.initial_offset,
    info.final_offset, info.width );
    uint64_t size = info.final_offset- info.initial_offset;
    accessMemory(info.initial_offset, size, info.sid, info.sequence_number,
        BaseTLB::Mode::Read, new uint8_t[size], info.tc);
}

void
SEprocessing::accessMemory(Addr addr, int size, int sid, int ssid,
                        BaseTLB::Mode mode, uint8_t *data,
                        ThreadContext * tc)
{
    Addr paddr;
    if (!iterQueue[sid]->translatePaddr(&paddr)){
        //Create new virtual request if:
            //Translation is needed
        RequestPtr req = std::make_shared<Request>();
        req->setVirt(0,addr,size,0,0,0);
        req->setStreamId(sid);
        req->setSubStreamId(ssid);
        DPRINTF(JMDEVEL, "Translating for addr %#x\n", req->getVaddr());

        // Check if the data is only in one page:
        if (!this->isSinglePage(addr, size)) {
            // In this case we need to split the request in two
            RequestPtr req1, req2;
            req->splitOnVaddr(this->splitAddressOnPage(addr, size), req1,
                              req2);

            WholeTranslationState *state = new WholeTranslationState(
                req, req1, req2, new uint8_t[size], NULL, mode);
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
        sendData(req, data, mode == BaseTLB::Read);
        iterQueue[sid]->setPaddr(paddr, false);
    }
}


void
SEprocessing::finishTranslation(WholeTranslationState *state)
{
    if (state->getFault() != NoFault) {
        panic("Page fault in SEprocessing. Addr: %#x, Name: %s\n",
              state->mainReq->getVaddr(), state->getFault()->name());
    }

    //Save the physical addr in the iterator
    auto sid = state->mainReq->streamId();

    if (!state->isSplit) {
        DPRINTF(JMDEVEL, "Got response for translation. %#x -> %#x\n",
                state->mainReq->getVaddr(), state->mainReq->getPaddr());
        sendData(state->mainReq, state->data, state->mode == BaseTLB::Read);
        iterQueue[sid]->setPaddr(state->mainReq->getPaddr(), false);
    } else {
        DPRINTF(JMDEVEL,
                "Got response for split translation. 1(%#x -> %#x) 2(%#x -> "
                "%#x)\n",
                state->sreqLow->getVaddr(), state->sreqLow->getPaddr(),
                state->sreqHigh->getVaddr(), state->sreqHigh->getPaddr());
        sendSplitData(state->sreqLow, state->sreqHigh, state->mainReq,
                      state->data, state->mode == BaseTLB::Read);
        iterQueue[sid]->setPaddr(state->sreqHigh->getPaddr(), true,
                                 state->sreqHigh->getVaddr());
    }

    delete state;
}

void
SEprocessing::sendData(RequestPtr ireq, uint8_t *data, bool read,
                       bool split_not_last) {
    DPRINTF(JMDEVEL, "Sending request for addr [P%#x]\n", ireq->getPaddr());

    RequestPtr req = NULL;
    auto sid = ireq->streamId();
    auto width = iterQueue[sid]->getCompressedWidth();
    bool ended = iterQueue[sid]->ended();

    for (ChunkGenerator gen(ireq->getPaddr(), ireq->getSize(), cacheLineSize);
        !gen.done(); gen.next()) {

        //JMTODO: We need to make sure that a page is not being crossed
        //If so: Go back with the iteration: For that the next cycle takes
        // care of that

        //JMTODO: Here we need to check FIFO for ocuppancy
        //And guarantee that it is not full yet

        req = std::make_shared<Request>(
            gen.addr(), gen.size(), 0, 0);

        auto ssid = ++ssidArray[sid];
        req->setStreamId(sid);
        req->setSubStreamId(ssid);

        // if (!parent->ld_fifo.full(sid)){
        bool last_packet = (ended && gen.last() && !split_not_last);
        if (last_packet) DPRINTF(JMDEVEL, "Last packet for sid (%d)\n", sid);
        parent->ld_fifo.reserve(sid, ssid, gen.size(), width, last_packet);
        // }
        // else {
        //     //JMTODO: Take action on reversing the processing
        // }

        req->taskId(ContextSwitchTaskId::DMA);
        PacketPtr pkt = read ? Packet::createRead(req) :
                               Packet::createWrite(req);

        // Increment the data pointer on a write
        if (data)
            pkt->dataStatic(data + gen.complete());

        DPRINTF(JMDEVEL, "--Queuing for addr: %#x size: %d ssid[%d]\n",
                gen.addr(), gen.size(), ssid);
        parent->getMemPort()->schedTimingReq(pkt,
                                parent->nextAvailableCycle());
    }
}

void
SEprocessing::sendSplitData(const RequestPtr &req1, const RequestPtr &req2,
                            const RequestPtr &req, uint8_t *data, bool read) {
    DPRINTF(JMDEVEL, "Sending split request for addrs [P%#x] & [P%#x]\n",
            req1->getPaddr(), req2->getPaddr());

    sendData(req1, data, read, true);
    sendData(req2, data + req1->getSize(), read, false);
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
    uint8_t width = iter->getWidth();
    int size = pkt->getSize()/width;

    switch(width){
        case 1:
            DPRINTF(JMDEVEL, "Data(%d:%d) w(%d)\n%s\n", sid, ssid, width,
                    v_print(memData.as<uint8_t>(), size));
            break;
        case 2:
            DPRINTF(JMDEVEL, "Data(%d:%d) w(%d)\n%s\n", sid, ssid, width,
                    v_print(memData.as<uint16_t>(), size));
            break;
        case 4:
            DPRINTF(JMDEVEL, "Data(%d:%d) w(%d)\n%s\n", sid, ssid, width,
                    v_print(memData.as<uint32_t>(), size));
            break;
        default:
            DPRINTF(JMDEVEL, "Data(%d:%d) w(%d)\n%s\n", sid, ssid, width,
                    v_print(memData.as<uint64_t>(), size));
    }
    //Insert data into fifo
    parent->ld_fifo.insert(sid, ssid, memData);
}

void
RequestExecuteEvent::process(){
    callee->executeRequest(info);
}