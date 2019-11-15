
#ifndef __CPU_O3_SE_INTERFACE_HH__
#define __CPU_O3_SE_INTERFACE_HH__

#include "cpu/utils.hh"
#include "debug/UVERename.hh"
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

   public:
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

    std::pair<bool, StreamID> reverseLookup(StreamID psid) {
        for (int sid = 0; sid < 32; sid++) {
            if (map[sid].used && map[sid].renamed == psid) {
                return std::make_pair(true, sid);
            }
        }
        return std::make_pair(false, 0);
    }

    std::pair<bool, StreamID> undo(StreamID sid) {
        if (!map[sid].last_used) return std::make_pair(false, 0);
        map[sid].last_used = false;
        return std::make_pair(true, map[sid].last_rename);
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
    void _signalEngineReady(CallbackInfo info);

    static void sendDataToCPU(int physIdx, TheISA::VecRegContainer *cnt) {
        SEInterface<Impl>::singleton->sendData(physIdx, cnt);
    }

    static void signalEngineReady(CallbackInfo info) {
        SEInterface<Impl>::singleton->_signalEngineReady(info);
    }

    // void configureStream(Stream stream, Dimension dim);

    bool sendCommand(SECommand cmd);

    bool isReady(StreamID sid);

    bool fetch(StreamID sid, TheISA::VecRegContainer **cnt);

    void tick() { engine->tick(); }

    bool isFinished(StreamID sid) { return engine->isFinished(sid); }

    void setCompleted(InstSeqNum seqNum, bool Complete) {
        UVECondLookup[seqNum] = Complete;
    }

    bool isComplete(InstSeqNum seqNum) { return UVECondLookup.at(seqNum); }

    void squashToBuffer(const RegId &arch, PhysRegIdPtr phys) {
        StreamID sid = arch.index();
        if (!registerBuffer[sid]->empty()) {
            if (std::get<1>(registerBuffer[sid]->back()) == phys &&
                std::get<0>(registerBuffer[sid]->back()) == arch) {
                DPRINTF(UVERename, "SquashToBuffer: %d, %p\n", arch.index(),
                        phys);
                registerBuffer[sid]->pop_back();
                speculationPointer[sid]--;
                auto lookup_stream = stream_rename.getStream(arch.index());
                engine->squash(lookup_stream.second);
            }
        }
    }

    void squashDestToBuffer(const RegId &arch, PhysRegIdPtr phys) {
        // Set the old renames
        StreamID sid = arch.index();
        auto result = stream_rename.undo(sid);
        if (result.first) {
            // registerBuffer[result.first]->clear();
            // speculationPointer[result.first].clear();
            stream_rename.freeStream(result.first);
            DPRINTF(UVERename, "SquashDestToBuffer: %d, %p\n", result.first,
                    phys);
        }
    }

    void commitToBuffer(const RegId &arch, PhysRegIdPtr phys) {
        StreamID sid = arch.index();
        auto bufferEnd = registerBuffer[sid]->back();
        if (std::get<2>(bufferEnd)) {
            if (std::get<1>(bufferEnd) == phys &&
                std::get<0>(bufferEnd) == arch) {
                DPRINTF(UVERename, "CommitToBuffer: %d, %p\n", sid, phys);
                registerBuffer[sid]->pop_back();
                speculationPointer[sid]--;
                auto lookup_stream =
                    stream_rename.getStream(std::get<0>(bufferEnd).index());
                engine->commit(lookup_stream.second);
            }
        }
    }
    void addToBuffer(const RegId &arch, PhysRegIdPtr phys) {
        StreamID sid = arch.index();
        DPRINTF(UVERename, "AddToBuffer: %d, %p\n", sid, phys);
        if (registerBuffer[sid]->empty()) speculationPointer[sid]++;
        registerBuffer[sid]->push_front(std::make_tuple(arch, phys, false));
    }

    void markOnBuffer(StreamID sid, PhysRegIdPtr phys) {
        // DPRINTF(UVERename, "MarkOnBuffer: %p\n",phys);
        RegBufferIter it = registerBuffer[sid]->begin();
        while (it != registerBuffer[sid]->end()) {
            if (phys->index() == std::get<1>(*it)->index()) {
                std::get<2>(*it) = true;
            }
            it++;
        }
    }

    std::pair<RegId, PhysRegIdPtr> consumeOnBuffer(StreamID sid) {
        if (!registerBuffer[sid]->empty() && speculationPointer[sid].valid()) {
            if (std::get<2>(*speculationPointer[sid])) {
                auto retval =
                    std::make_pair(std::get<0>(*speculationPointer[sid]),
                                   std::get<1>(*speculationPointer[sid]));
                DPRINTF(UVERename, "ConsumeOnBuffer: %d, %p\n", sid,
                        std::get<1>(*speculationPointer[sid]));
                speculationPointer[sid]++;

                return retval;
            }
            return std::make_pair(RegId(), (PhysRegIdPtr)NULL);
        }
        return std::make_pair(RegId(), (PhysRegIdPtr)NULL);
        }

        std::pair<bool, StreamID> markConfigStream(StreamID sid) {
            return stream_rename.renameStream(sid);
        }

        bool isStream(StreamID sid) {
            return stream_rename.getStream(sid).first;
        }

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

        using RegBufferList = std::list<std::tuple<RegId, PhysRegIdPtr, bool>>;
        using RegBufferIter = RegBufferList::iterator;
        using SpecBufferIter = DumbIterator<RegBufferList>;

        std::vector<RegBufferList *> registerBuffer;
        std::vector<SpecBufferIter> speculationPointer;

        StreamRename stream_rename;
};

#endif  //__CPU_O3_SE_INTERFACE_HH__