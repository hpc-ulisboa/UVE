#include "uve_simobjs/uve_streaming_engine.hh"

UVEStreamingEngine::UVEStreamingEngine(UVEStreamingEngineParams* params)
    : ClockedObject(params),
      memoryPortLoad(params->name + ".mem_side_load", this),
      memoryPortStore(params->name + ".mem_side_store", this),
      memCore(params, this),
      confCore(params, this),
      ld_fifo(params),
      st_fifo(params),
      confAddr(params->start_addr),
      confSize(32),
      cycler(0) {
    callback = nullptr;
}

void
UVEStreamingEngine::init(){
    if (!memoryPortLoad.isConnected())
        panic("Memory port load of %s not connected to anything!", name());
    if (!memoryPortStore.isConnected())
        panic("Memory port store of %s not connected to anything!", name());
    ClockedObject::init();
    ld_fifo.init();
    st_fifo.init();
}


UVEStreamingEngine*
UVEStreamingEngineParams::create()
{
  return new UVEStreamingEngine(this);
}

AddrRangeList
UVEStreamingEngine::getAddrRanges() const
{
    assert(confSize != 0);
    AddrRangeList ranges;
    ranges.push_back(RangeSize(confAddr, confSize));
    return ranges;
}

void
UVEStreamingEngine::sendRangeChange() const
{
  // confPort.sendRangeChange();
}

Port& UVEStreamingEngine::getPort(const std::string& if_name,
  PortID idx = InvalidPortID)
{
    if (if_name == "mem_side_load") {
        return memoryPortLoad;
    } else if (if_name == "mem_side_store") {
        return memoryPortStore;
    } else {
        return ClockedObject::getPort(if_name, idx);
    }
}

SmartReturn
UVEStreamingEngine::recvCommand(SECommand cmd, InstSeqNum sn) {
    return confCore.recvCommand(cmd, sn);
}
SmartReturn
UVEStreamingEngine::addStreamConfig(StreamID sid, InstSeqNum sn) {
    return confCore.addCmd(sid, sn);
}
SmartReturn
UVEStreamingEngine::squashStreamConfig(StreamID sid, InstSeqNum sn) {
    return confCore.squashCmd(sid, sn);
}

bool
UVEStreamingEngine::MemSidePort::recvTimingResp(PacketPtr pkt)
{
    // Just forward to the Engine.
    // return owner->handleResponse(pkt);
    if (pkt->isWrite()) {
        DPRINTF(JMDEVEL, "MemSidePort received write pkt for Paddr %p\n",
                pkt->req->getPaddr());
        delete pkt;
        return true;
    }
    if (pkt->isInvalidate()) {
        DPRINTF(JMDEVEL, "MemSidePort received invalidate pkt for Paddr %p\n",
                pkt->req->getPaddr());
        delete pkt;
        return true;
    }
    if (pkt->req->hasVaddr()){
      DPRINTF(JMDEVEL, "MemSidePort received pkt for Vaddr %p :Paddr %p\n",
                          pkt->req->getVaddr(), pkt->req->getPaddr());
    }
    else {
      DPRINTF(JMDEVEL, "MemSidePort received pkt for Paddr %p\n",
                          pkt->req->getPaddr());
    }
    owner->memCore.recvData(pkt);
    // delete pkt->req;
    delete pkt;
    return true;
}

void
UVEStreamingEngine::MemSidePort::recvRangeChange()
{
    owner->sendRangeChange();
    return;
}

void
UVEStreamingEngine::tick(){
  memCore.tick();
  CallbackInfo res;
  res.is_clear = false;
  if (ld_fifo.tick(&res)) {
      // auto data_vec = ld_fifo.get_data();
      signal_cpu(res);
      // for (auto elem : data_vec) {
      //     send_data_to_sei(elem.first, elem.second);
      // }
  }
  memQueueDepthLoad.sample(getMemPort(true /*Load*/)->getTransmitListSize());
  memQueueDepthStore.sample(
      getMemPort(false /*Store*/)->getTransmitListSize());
}

void
UVEStreamingEngine::regStats(){
    ClockedObject::regStats();

    using namespace Stats;

    // Number of times a stream was configured - Scalar
    numStreamConfig.init(confSize)
        .name(name() + ".numStreamConfig")
        .desc("Number of times a stream was configured");
    // "" Squashed - Scalar
    numStreamSquashed.init(confSize)
        .name(name() + ".numStreamSquashed")
        .desc("Number of times a stream was squashed");
    // "" Completed - Scalar
    numStreamCompleted.init(confSize)
        .name(name() + ".numStreamCompleted")
        .desc("Number of times a stream was completed successfully");
    // Memory accesses - Scalar
    numStreamMemAccesses.init(confSize)
        .name(name() + ".numStreamMemAccesses")
        .desc("Number of memory accesses");
    // Read bytes - Scalar
    numStreamMemBytes.init(confSize)
        .name(name() + ".numStreamMemBytes")
        .desc("Number of memory bytes transacted");
    // Cycles between start and completion - Distribution
    streamExecutionCycles.init(confSize, 0, 2048, 1024)
        .name(name() + ".streamExecutionCycles")
        .desc("Cycles the stream took to successfully complete");
    // Cycles of stream processing - Scalar
    streamProcessingCycles.init(confSize)
        .name(name() + ".streamProcessingCycles")
        .desc("Number of cycles the stream was being processed");
    // Mem queue depth (max, avg) - Distribution
    memQueueDepthLoad.init(0, 200, 50)
        .name(name() + ".memQueueDepthLoad")
        .desc("Memory request queue depth");
    memQueueDepthStore.init(0, 200, 50)
        .name(name() + ".memQueueDepthStore")
        .desc("Memory request queue depth");
    // Cycles mem request took (avg, min, max) - Distribution
    memRequestCycles.init(32, 0, 100, 10)
        .name(name() + ".memRequestCycles")
        .desc("Cycles a memory request took");

    ld_fifo.regStats();
    st_fifo.regStats();
}

void
UVEStreamingEngine::set_callback(void (*_callback)(CallbackInfo info)) {
    DPRINTF(JMDEVEL, "Configuring Callback\n");
    callback = _callback;
}

SmartReturn
UVEStreamingEngine::squashLoad(StreamID sid) {
    return ld_fifo.squash(sid);
}

SmartReturn
UVEStreamingEngine::shouldSquashLoad(StreamID sid) {
    return ld_fifo.shouldSquash(sid);
}

SmartReturn
UVEStreamingEngine::commitLoad(StreamID sid) {
    auto result = ld_fifo.commit(sid);
    if (result.isEnd()) {
        // Time to eliminate this sid
        auto clear_result = endStreamLoad(sid);
        return clear_result.isOk() ? SmartReturn::end() : SmartReturn::error();
    }
    return result;
}

void
UVEStreamingEngine::synchronizeLoadLists(StreamID sid) {
    ld_fifo.synchronizeLists(sid);
    return;
}

SmartReturn
UVEStreamingEngine::endStreamLoad(StreamID sid) {
    SmartReturn result = SmartReturn::ok();
    // Clear Fifo
    result = ld_fifo.clear(sid);
    if (!result.isOk()) return result;
    // Clear Processing
    result = memCore.clear(sid);

    return result;
}

SmartReturn
UVEStreamingEngine::getDataLoad(StreamID sid) {
    return ld_fifo.getData(sid);
}

SmartReturn
UVEStreamingEngine::squashStore(StreamID sid, SubStreamID ssid) {
    return st_fifo.squash(sid, ssid);
}

SmartReturn
UVEStreamingEngine::shouldSquashStore(StreamID sid) {
    return st_fifo.shouldSquash(sid);
}

SmartReturn
UVEStreamingEngine::commitStore(StreamID sid, SubStreamID ssid) {
    auto result = st_fifo.commit(sid, ssid);
    // Should only eliminate store stream when last piece of data was sent to
    // memory
    // if (result.isEnd()) {
    //     // Time to eliminate this sid
    //     auto clear_result = endStreamStore(sid);
    //     return clear_result.isOk() ? SmartReturn::end() :
    //     SmartReturn::error();
    // }
    return result;
}

void
UVEStreamingEngine::synchronizeStoreLists(StreamID sid) {
    st_fifo.synchronizeLists(sid);
    return;
}

void
UVEStreamingEngine::setDataStore(StreamID sid, SubStreamID ssid,
                                 CoreContainer val) {
    st_fifo.insert_data(sid, ssid, val);
}

SmartReturn
UVEStreamingEngine::endStreamStore(StreamID sid) {
    SmartReturn result = SmartReturn::ok();
    // Clear Stream in SEInterface
    DPRINTF(JMDEVEL, "Ending StreamStore: sid %d\n", sid);
    CallbackInfo info;
    info.is_clear = true;
    info.clear_sid = sid;
    callback(info);
    // Clear Fifo
    result = st_fifo.clear(sid);
    if (!result.isOk()) return result;
    // Clear Processing
    result = memCore.clear(sid);
    return result;
}

SmartReturn
UVEStreamingEngine::endStreamFromSquash(StreamID sid) {
    return memCore.clear(sid).AND(ld_fifo.clear(sid)).AND(st_fifo.clear(sid));
}