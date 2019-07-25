
#include "debug/UVESC.hh"
#include "uve_simobjs/uve_streaming_engine.hh"

SEcontroller::SEcontroller(UVEStreamingEngineParams *params,
                            UVEStreamingEngine *_parent)
{
    parent = _parent;
}

Tick SEcontroller::read(PacketPtr pkt){
    return Tick(20);
}

Tick SEcontroller::write(PacketPtr pkt){
    return Tick(20);
}