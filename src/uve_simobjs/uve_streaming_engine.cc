#include "uve_simobjs/uve_streaming_engine.hh"

UVEStreamingEngine::UVEStreamingEngine(UVEStreamingEngineParams* params)
    : ClockedObject(params),
      memoryPort(params->name + ".mem_side", this),
      memCore(params, this),
      confCore(params, this),
      ld_fifo(params),
      st_fifo(params),
      confAddr(params->start_addr),
      confSize(32) {
    ld_fifo.init();
    st_fifo.init();
    // memCore->setConfCore(confCore);
    // confCore->setMemCore(memCore);
    // confPort = new PioPort(this);
    // memoryPort = new MemSidePort(params->name + ".mem_side", this);
}

void
UVEStreamingEngine::init(){
    if (!memoryPort.isConnected())
        panic("Memory port of %s not connected to anything!", name());
    ClockedObject::init();
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
  if (if_name == "mem_side") {
      return memoryPort;
  } else {
      return ClockedObject::getPort(if_name, idx);
  }
}

SmartReturn
UVEStreamingEngine::recvCommand(SECommand cmd) {
    return confCore.recvCommand(cmd);
}

bool
UVEStreamingEngine::MemSidePort::recvTimingResp(PacketPtr pkt)
{
    // Just forward to the Engine.
    // return owner->handleResponse(pkt);
    if (pkt->isWrite()) {
        DPRINTF(JMDEVEL, "MemSidePort received write pkt for Paddr %p\n",
                pkt->req->getPaddr());
        return true;
    }
    if (pkt->isInvalidate()) {
        DPRINTF(JMDEVEL, "MemSidePort received invalidate pkt for Paddr %p\n",
                pkt->req->getPaddr());
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
  if (ld_fifo.tick(&res)) {
      // auto data_vec = ld_fifo.get_data();
      signal_cpu(res);
      // for (auto elem : data_vec) {
      //     send_data_to_sei(elem.first, elem.second);
      // }
  }
}

void
UVEStreamingEngine::regStats(){
  ClockedObject::regStats();
}

void
UVEStreamingEngine::set_callback(void (*_callback)(CallbackInfo info)) {
    DPRINTF(JMDEVEL, "Configuring Callback\n");
    callback = _callback;
}

SmartReturn
UVEStreamingEngine::squashLoad(uint16_t sid) {
    return ld_fifo.squash(sid);
}

SmartReturn
UVEStreamingEngine::shouldSquashLoad(uint16_t sid) {
    return ld_fifo.shouldSquash(sid);
}

SmartReturn
UVEStreamingEngine::commitLoad(uint16_t sid) {
    auto result = ld_fifo.commit(sid);
    if (result.isEnd()) {
        // Time to eliminate this sid
        auto clear_result = endStreamLoad(sid);
        return clear_result.isOk()
                   ? SmartReturn::end()
                   : SmartReturn::error("Could not clear stream footprint");
    }
    return result;
}

void
UVEStreamingEngine::synchronizeLoadLists(uint16_t sid) {
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
UVEStreamingEngine::getDataLoad(uint16_t sid) {
    return ld_fifo.getData(sid);
}

SmartReturn
UVEStreamingEngine::squashStore(uint16_t sid, uint16_t ssid) {
    return st_fifo.squash(sid, ssid);
}

SmartReturn
UVEStreamingEngine::shouldSquashStore(uint16_t sid) {
    return st_fifo.shouldSquash(sid);
}

SmartReturn
UVEStreamingEngine::commitStore(uint16_t sid, uint16_t ssid) {
    auto result = st_fifo.commit(sid, ssid);
    if (result.isEnd()) {
        // Time to eliminate this sid
        auto clear_result = endStreamStore(sid);
        return clear_result.isOk()
                   ? SmartReturn::end()
                   : SmartReturn::error("Could not clear stream footprint");
    }
    return result;
}

void
UVEStreamingEngine::synchronizeStoreLists(uint16_t sid) {
    st_fifo.synchronizeLists(sid);
    return;
}

void
UVEStreamingEngine::setDataStore(uint16_t sid, uint16_t ssid,
                                 CoreContainer val) {
    st_fifo.insert_data(sid, ssid, val);
}

SmartReturn
UVEStreamingEngine::endStreamStore(StreamID sid) {
    SmartReturn result = SmartReturn::ok();
    // Clear Fifo
    result = st_fifo.clear(sid);
    if (!result.isOk()) return result;
    // Clear Processing
    result = memCore.clear(sid);

    return result;
}
