
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

typedef struct stream_rename_map {
    StreamID renamed;
    bool used;
    StreamID last_rename;
    bool last_used;
} StreamRenameMap;

class StreamRename {
   private:
    StreamRenameMap makeSRM() {
        StreamRenameMap srm;
        srm.used = false;
        srm.renamed = 0;
        srm.last_rename = 0;
        srm.last_used = false;
        return srm;
    }
    using Arch2PhysMap = std::vector<StreamRenameMap>;
    Arch2PhysMap map;
    using FreePool = std::queue<StreamID>;
    FreePool pool;

   public:
    StreamRename() : map(32, makeSRM()) {
        for (int i = 0; i < 32; i++) {
            freeStream(i);
        }
    };
    ~StreamRename(){};

   private:
    StreamID getFreeStream() {
        auto retval = pool.front();
        pool.pop();
        return retval;
    }
    bool anyStream() { return pool.size() != 0; }
    void freeStream(StreamID sid) { pool.push(sid); }

   public:
    std::pair<bool, StreamID> renameStream(StreamID sid) {
        if (anyStream()) {
            auto available_stream = getFreeStream();
            map[sid].last_used = map[sid].used;
            map[sid].last_rename = map[sid].renamed;
            map[sid].used = true;
            map[sid].renamed = available_stream;
            DPRINTF(UVESEI, "Rename: Stream %d was renamed to %d\n", sid,
                    available_stream);
            return std::make_pair(true, available_stream);
        } else {
            DPRINTF(
                UVESEI,
                "Rename: Stream %d was not renamed, no streams available.\n",
                sid);
            return std::make_pair(false, (StreamID)0);
        }
    }

    std::pair<bool, StreamID> getStream(StreamID sid) {
        if (map[sid].used)
            DPRINTF(UVESEI, "Lookup: Stream %d points to %d.\n", sid,
                    map[sid].renamed);
        return std::make_pair(map[sid].used, map[sid].renamed);
    }
};

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

    void squashToBuffer(const RegId &arch, PhysRegIdPtr phys) {
        if (isStream(arch.index()))
            squashedBuffer.push_front(std::make_tuple(arch, phys, true));
    }
    void commitToBuffer(PhysRegIdPtr phys) {
        if (!physRegBuffer.empty()) physRegBuffer.pop_back();
    }
    void addToBuffer(const RegId &arch, PhysRegIdPtr phys) {
        physRegBuffer.push_front(std::make_tuple(arch, phys, false));
        specRegBuffer.push_front(std::make_tuple(arch, phys, false));
        // make sure not to repeat
        physRegBuffer.unique();
        specRegBuffer.unique();
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

    std::tuple<RegId, PhysRegIdPtr, bool> consumeOnBuffer() {
        if (!squashedBuffer.empty()) {
            bool success = false;
            auto bk = squashedBuffer.back();
            auto stream = std::get<0>(bk);
            squashedBuffer.pop_back();
            auto it = specRegBuffer.end();
            while (it != specRegBuffer.begin()) {
                it--;
                if (std::get<0>(*it) == stream && std::get<2>(*it)) {
                    success = true;
                    break;
                }
            }
            if (!success)
                return std::make_tuple(RegId(), (PhysRegIdPtr)NULL, false);
        }

        auto bk = specRegBuffer.back();
        if (std::get<2>(bk)) {
            auto retval =
                std::make_tuple(std::get<0>(bk), std::get<1>(bk), false);
            specRegBuffer.pop_back();
            return retval;
        }
        return std::make_tuple(RegId(), (PhysRegIdPtr)NULL, false);
    }

    void undoConsumeOnBuffer(std::tuple<RegId, PhysRegIdPtr, bool> elem) {
        squashedBuffer.push_back(elem);
    }

    std::pair<bool, StreamID> markConfigStream(StreamID sid) {
        return stream_rename.renameStream(sid);
    }

    bool isStream(StreamID sid) { return stream_rename.getStream(sid).first; }

    StreamID getStream(StreamID sid) {
        return stream_rename.getStream(sid).second;
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
        specRegBuffer, squashedBuffer;

    StreamRename stream_rename;
};

#endif  //__CPU_O3_SE_INTERFACE_HH__