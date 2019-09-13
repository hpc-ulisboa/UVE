
#ifndef __CPU_O3_SE_INTERFACE_HH__
#define __CPU_O3_SE_INTERFACE_HH__

#include "cpu/utils.hh"
#include "debug/UVESEI.hh"
#include "mem/port.hh"
#include "sim/sim_object.hh"
#include "uve_simobjs/utils.hh"
#include "uve_simobjs/uve_streaming_engine.hh"

#define MaximumStreams 32

class DerivO3CPUParams;

template <typename Impl>
class SEInterface {
   public:
    typedef typename Impl::O3CPU O3CPU;
    typedef typename Impl::CPUPol::IEW IEW;
    typedef typename Impl::CPUPol::Decode Decode;
    typedef typename Impl::CPUPol::Commit Commit;

    SEInterface(O3CPU *cpu_ptr, Decode *dec_ptr, IEW *iew_ptr, Commit *cmt_ptr,
                DerivO3CPUParams *params);
    ~SEInterface(){};

    void startupComponent();

    bool recvTimingResp(PacketPtr pkt);
    void recvTimingSnoopReq(PacketPtr pkt);
    void recvReqRetry();

    // void configureStream(Stream stream, Dimension dim);

    bool sendCommand(SECommand cmd);

    bool reserve(StreamID sid, PhysRegIndex idx);

    bool isReady(StreamID sid, PhysRegIndex idx);

    void tick() { engine->tick(); }

   private:
    /** Pointers for parent and sibling structures. */
    O3CPU *cpu;
    Decode *decStage;
    IEW *iewStage;
    Commit *cmtStage;

    /* Pointer for communication port */
    MasterPort *dcachePort;

    /* Pointer for engine */
    UVEStreamingEngine *engine;

    /* Addr range */
    AddrRange sengine_addr_range;
};

#endif  //__CPU_O3_SE_INTERFACE_HH__