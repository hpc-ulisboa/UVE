
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
    static SEInterface *singleton;

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

    void sendData(int physIdx, TheISA::VecRegContainer *cnt);
    void _signalEngineReady();

    static void sendDataToCPU(int physIdx, TheISA::VecRegContainer *cnt) {
        SEInterface<Impl>::singleton->sendData(physIdx, cnt);
    }

    static void signalEngineReady() {
        SEInterface<Impl>::singleton->_signalEngineReady();
    }

    // void configureStream(Stream stream, Dimension dim);

    bool sendCommand(SECommand cmd);

    bool reserve(StreamID sid, PhysRegIndex idx);

    bool isReady(StreamID sid, PhysRegIndex idx);

    bool fetch(StreamID sid, TheISA::VecRegContainer **cnt);

    void tick() { engine->tick(); }

    void squash(StreamID sid, int regidx) { engine->squash(sid, regidx); }

    bool isFinished(StreamID sid) { return engine->isFinished(sid); }

    void setCompleted(InstSeqNum seqNum, bool Complete) {
        UVECondLookup[seqNum] = Complete;
    }

    bool isComplete(InstSeqNum seqNum) { return UVECondLookup.at(seqNum); }

    void squashToBuffer(PhysRegIndex idx) {
        // panic("Not implemented\n");
    }
    void commitToBuffer(PhysRegIdPtr phys) {
        if (!physRegBuffer.empty()) physRegBuffer.pop_back();
    }
    void addToBuffer(const RegId &arch, PhysRegIdPtr phys) {
        physRegBuffer.push_front(std::make_tuple(arch, phys, false));
        specRegBuffer.push_front(std::make_tuple(arch, phys, false));
    }

    void markOnBuffer(PhysRegIdPtr phys) {
        auto it = specRegBuffer.begin();

        while (it != specRegBuffer.end()) {
            if (phys->index() == std::get<1>(*it)->index()) {
                std::get<2>(*it) = true;
            }
            it++;
        }
    }

    std::pair<RegId, PhysRegIdPtr> consumeOnBuffer() {
        auto bk = specRegBuffer.back();
        if (std::get<2>(bk)) {
            auto retval = std::make_pair(std::get<0>(bk), std::get<1>(bk));
            specRegBuffer.pop_back();
            return retval;
        }
        return std::make_pair(RegId(), (PhysRegIdPtr)NULL);
    }

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

    /* Completion Lookup Table (Updated in rename phase) */
    std::map<InstSeqNum, bool> UVECondLookup;

    std::list<std::tuple<RegId, PhysRegIdPtr, bool>> physRegBuffer,
        specRegBuffer;
};

#endif  //__CPU_O3_SE_INTERFACE_HH__