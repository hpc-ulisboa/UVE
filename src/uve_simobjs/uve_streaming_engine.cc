#include "uve_simobjs/uve_streaming_engine.hh"

UVEStreamingEngine::UVEStreamingEngine(UVEStreamingEngineParams *params) :
  ClockedObject(params), memoryPort(params->name + ".mem_side", this),
  confPort(params->name + ".cpu_side", this), confCore(params, this),
  confAddr(params->start_addr), confSize(32)
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
    if (!confPort.isConnected())
        panic("Conf Pio port of %s not connected to anything!", name());
    ClockedObject::init();
}


UVEStreamingEngine*
UVEStreamingEngineParams::create()
{
  return new UVEStreamingEngine(this);
}

Tick UVEStreamingEngine::read(PacketPtr pkt){
  return confCore.read(pkt);
}

Tick UVEStreamingEngine::write(PacketPtr pkt){
  return confCore.write(pkt);
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
  confPort.sendRangeChange();
}

Port& UVEStreamingEngine::getPort(const std::string& if_name,
  PortID idx = InvalidPortID)
{
  if (if_name == "mem_side") {
      return memoryPort;
  } else if (if_name == "cpu_side"){
      return confPort;
  } else {
      return ClockedObject::getPort(if_name, idx);
  }
}

void
UVEStreamingEngine::MemSidePort::sendPacket(PacketPtr pkt)
{
    // Note: This flow control is very simple since the Engine is blocking.

    // panic_if(blockedPacket != nullptr, "Should never try to send if
    //blocked!");

    // // If we can't send the packet across the port, store it for later.
    // if (!sendTimingReq(pkt)) {
    //     blockedPacket = pkt;
    // }
}

bool
UVEStreamingEngine::MemSidePort::recvTimingResp(PacketPtr pkt)
{
    // Just forward to the Engine.
    // return owner->handleResponse(pkt);
    return true;
}

void
UVEStreamingEngine::MemSidePort::recvReqRetry()
{
    // // We should have a blocked packet if this function is called.
    // assert(blockedPacket != nullptr);

    // // Grab the blocked packet.
    // PacketPtr pkt = blockedPacket;
    // blockedPacket = nullptr;

    // // Try to resend it. It's possible that it fails again.
    // sendPacket(pkt);
}

void
UVEStreamingEngine::MemSidePort::recvRangeChange()
{
    owner->sendRangeChange();
    return;
}

Tick
UVEStreamingEngine::CpuSidePort::recvAtomic(PacketPtr pkt)
{
    // technically the packet only reaches us after the header delay,
    // and typically we also need to deserialise any payload
    DPRINTF(JMDEVEL, "CpuSidePort::recvAtomic executing\n");

    Tick receive_delay = pkt->headerDelay + pkt->payloadDelay;
    pkt->headerDelay = pkt->payloadDelay = 0;

    const Tick delay(pkt->isRead() ? device->read(pkt) : device->write(pkt));
    assert(pkt->isResponse() || pkt->isError());
    return delay + receive_delay;
}

AddrRangeList
UVEStreamingEngine::CpuSidePort::getAddrRanges() const
{
    return device->getAddrRanges();
}