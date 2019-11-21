
#ifndef __CPU_O3_SE_INTERFACE_HH__
#define __CPU_O3_SE_INTERFACE_HH__

#include <unordered_set>

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
    void freeStream(StreamID sid) {
        pool.push(sid);
        auto result = reverseLookup(sid);
        if (result.first) {
            map[result.second].used = false;
        }
        DPRINTF(UVESEI, "Freed Stream %d\n", sid);
    }

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

    void _signalEngineReady(CallbackInfo info);

    static void signalEngineReady(CallbackInfo info) {
        SEInterface<Impl>::singleton->_signalEngineReady(info);
    }

    bool sendCommand(SECommand cmd);

    bool isReady(StreamID sid);

    bool fetch(StreamID sid, TheISA::VecRegContainer **cnt);

    void tick() { engine->tick(); }

    // JMFIXME: Remove never used
    bool isFinished(StreamID asid) {
        StreamID sid = getAssertedStream(asid);
        return engine->isFinished(sid).isOk();
    }

    // JMFIXME: Remove never used
    void setCompleted(InstSeqNum seqNum, bool Complete) {
        UVECondLookup[seqNum] = Complete;
    }

    // JMFIXME: Remove never used
    bool isComplete(InstSeqNum seqNum) { return UVECondLookup.at(seqNum); }

    void squashToBuffer(const RegId &arch, PhysRegIdPtr phys, InstSeqNum sn) {
        if (!isInstMeaningful(sn)) return;
        StreamID sid = getStream(arch.index());

        if (!registerBuffer[sid]->empty()) {
            if (std::get<1>(registerBuffer[sid]->back()) == phys &&
                std::get<0>(registerBuffer[sid]->back()) == arch) {
                registerBuffer[sid]->pop_back();
                speculationPointer[sid]--;
                auto lookup_stream = stream_rename.getStream(arch.index());
                if (engine->shouldSquash(lookup_stream.second).isOk()) {
                    SmartReturn result = engine->squash(lookup_stream.second);
                    if (result.isNok() || result.isError())
                        panic("Error" + result.estr());
                    DPRINTF(
                        UVERename,
                        "SquashToBuffer: %d, %p. SeqNum[%d] \t Result:%s\n",
                        arch.index(), phys, sn, result.str());
                } else {
                    DPRINTF(UVERename,
                            "SquashToBufferOnly: %d, %p. SeqNum[%d]\n",
                            arch.index(), phys, sn);
                }
            }
        }
    }

    void squashDestToBuffer(const RegId &arch, PhysRegIdPtr phys,
                            InstSeqNum sn) {
        if (!isInstMeaningful(sn)) return;
        if (isInstStreamConfig(sn)) {
            StreamID sid = getStream(arch.index());

            // Set the old renames
            auto result = stream_rename.undo(sid);
            if (result.first) {
                // registerBuffer[result.first]->clear();
                // speculationPointer[result.first].clear();
                stream_rename.freeStream(result.first);
                DPRINTF(UVERename, "SquashDestToBuffer: %d, %p. SeqNum[%d]\n",
                        result.first, phys, sn);
            }
        } else {
            squashToBuffer(arch, phys, sn);
        }
    }

    void squashStreamConfig(StreamID asid, InstSeqNum sn) {
        if (isInstMeaningful(sn) && isInstStreamConfig(sn)) {
            StreamID sid = getStreamConfigSid(sn);
            stream_rename.freeStream(sid);
            retireStreamConfigInst(sn);
            DPRINTF(UVERename, "SquashStreamConfig: %d. SeqNum[%d]\n", sid,
                    sn);
        }
    }

    void commitToBuffer(const RegId &arch, PhysRegIdPtr phys, InstSeqNum sn) {
        if (!isInstMeaningful(sn)) return;
        retireStreamConfigInst(sn);

        StreamID sid = getStream(arch.index());

        auto bufferEnd = registerBuffer[sid]->back();
        if (std::get<2>(bufferEnd)) {
            if (std::get<1>(bufferEnd) == phys &&
                std::get<0>(bufferEnd) == arch) {
                registerBuffer[sid]->pop_back();
                speculationPointer[sid]--;
                auto lookup_stream =
                    stream_rename.getStream(std::get<0>(bufferEnd).index());
                SmartReturn result = engine->commit(lookup_stream.second);
                DPRINTF(UVERename,
                        "CommitToBuffer: %d, %p. SeqNum[%d]. \t Result:%s\n",
                        sid, phys, sn, result.str());
                if (result.isEnd()) {
                    clearBuffer(lookup_stream.second);
                }
                retireMeaningfulInst(sn);
                if (result.isNok() || result.isError())
                    panic("Error" + result.estr());
            }
        }
    }
    void addToBuffer(const RegId &arch, PhysRegIdPtr phys, InstSeqNum sn) {
        insertMeaningfulInst(sn);
        StreamID sid = getAssertedStream(arch.index());

        DPRINTF(UVERename, "AddToBuffer: %d, %p. SeqNum[%d]\n", sid, phys, sn);
        if (registerBuffer[sid]->empty()) {
            speculationPointer[sid]++;
            // Also update the fifo speculationPointer due to desyncronization
            // with This one
            engine->synchronizeLists(sid);
        }
        // // JMFIXME: Get around for bad squashing (I think im not handling
        // all
        // // squash cases)
        // if (!registerBuffer[sid]->empty()) {
        //     auto frnt = registerBuffer[sid]->front();
        //     if (std::get<0>(frnt) == arch && std::get<1>(frnt) == phys) {
        //         return;
        //     }
        // }
        registerBuffer[sid]->push_front(std::make_tuple(arch, phys, false));
    }

    void markOnBuffer(const RegId &arch, PhysRegIdPtr phys, InstSeqNum sn) {
        if (!isInstMeaningful(sn)) return;
        // Here we use getStream, because every register is called in the
        // parent instruction
        StreamID sid = getStream(arch.index());
        // DPRINTF(UVERename, "MarkOnBuffer: %p\n",phys);
        RegBufferIter it = registerBuffer[sid]->begin();
        while (it != registerBuffer[sid]->end()) {
            if (phys->index() == std::get<1>(*it)->index()) {
                std::get<2>(*it) = true;
            }
            it++;
        }
    }

    std::pair<RegId, PhysRegIdPtr> consumeOnBuffer(StreamID psid) {
        // Consume on buffer does not need stream lookup.
        // Sid is already physical

        if (!registerBuffer[psid]->empty() &&
            speculationPointer[psid].valid()) {
            if (std::get<2>(*speculationPointer[psid])) {
                auto retval =
                    std::make_pair(std::get<0>(*speculationPointer[psid]),
                                   std::get<1>(*speculationPointer[psid]));
                DPRINTF(UVERename, "ConsumeOnBuffer: %d, %p\n", psid,
                        std::get<1>(*speculationPointer[psid]));
                speculationPointer[psid]++;

                return retval;
            }
            return std::make_pair(RegId(), (PhysRegIdPtr)NULL);
        }
        return std::make_pair(RegId(), (PhysRegIdPtr)NULL);
    }

    std::pair<bool, StreamID> initializeStream(StreamID sid, InstSeqNum sn) {
        // Add inst sequence number to hash map
        insertMeaningfulInst(sn);
        auto result = stream_rename.renameStream(sid);
        insertStreamConfigInst(sn, result.second);
        DPRINTF(UVERename,
                "Initialized Stream [%d], renamed to sid[%d] with inst "
                "SeqNum[%d]\n",
                sid, result.second, sn);
        return result;
    }

    bool isStream(StreamID sid) { return stream_rename.getStream(sid).first; }

    StreamID getAssertedStream(StreamID asid) {
        auto rename = stream_rename.getStream(asid);
        if (!rename.first) panic("Fatal Error in Stream Rename.");
        return rename.second;
    }

    StreamID getStream(StreamID asid) {
        return stream_rename.getStream(asid).second;
    }

    bool isInstMeaningful(InstSeqNum sn) {
        if (inst_validator.find(sn) == inst_validator.end())
            return false;
        else
            return true;
    }

    bool isInstStreamConfig(InstSeqNum sn) {
        if (stream_config_validator.find(sn) == stream_config_validator.end())
            return false;
        else
            return true;
    }

    void retireStreamConfigInst(InstSeqNum sn) {
        stream_config_validator.erase(sn);
    }

    void insertStreamConfigInst(InstSeqNum sn, StreamID psid) {
        stream_config_validator.insert({sn, psid});
    }

    StreamID getStreamConfigSid(InstSeqNum sn) {
        auto search = stream_config_validator.find(sn);
        return search->second;
    }

    void retireMeaningfulInst(InstSeqNum sn) { inst_validator.erase(sn); }

    void insertMeaningfulInst(InstSeqNum sn) { inst_validator.insert(sn); }

   private:
    void clearBuffer(StreamID sid) {
        // Sid must be a real stream. Not a rename alias.
        // Release stream rename and clear rename
        stream_rename.freeStream(sid);
        // Clear register buffer
        registerBuffer[sid]->clear();

        DPRINTF(UVERename, "Cleared Stream %d\n", sid);
    }
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
    using InstSeqNumSet = std::unordered_set<uint64_t>;
    using StreamConfigHashMap = std::unordered_map<uint64_t, StreamID>;

    std::vector<RegBufferList *> registerBuffer;
    std::vector<SpecBufferIter> speculationPointer;

    StreamRename stream_rename;
    InstSeqNumSet inst_validator;
    StreamConfigHashMap stream_config_validator;
};

#endif  //__CPU_O3_SE_INTERFACE_HH__