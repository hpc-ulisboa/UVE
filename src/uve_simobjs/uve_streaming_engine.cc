#include "uve_simobjs/uve_streaming_engine.hh"

UVEStreamingEngine::UVEStreamingEngine(UVEStreamingEngineParams *params) :
  ClockedObject(params),
  memoryPort(params->name + ".mem_side", this), memCore(params, this),
  confCore(params, this), confAddr(params->start_addr), confSize(32)
{
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

Tick UVEStreamingEngine::read(PacketPtr pkt){
  return 1;
}

Tick UVEStreamingEngine::write(PacketPtr pkt){
  return 1;
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

bool
UVEStreamingEngine::recvCommand(SECommand cmd)
{
  confCore.recvCommand(cmd);
  return true;
}

bool
UVEStreamingEngine::MemSidePort::recvTimingResp(PacketPtr pkt)
{
    // Just forward to the Engine.
    // return owner->handleResponse(pkt);
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
}