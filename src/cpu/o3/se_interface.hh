
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
    StreamMode direction;
} StreamRenameMap;

class StreamRename {
   private:
    StreamRenameMap makeSRM() {
        StreamRenameMap srm;
        srm.used = false;
        srm.renamed = 0;
        srm.last_rename = 0;
        srm.last_used = false;
        srm.direction = StreamMode::load;
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
    std::pair<bool, StreamID> renameStream(StreamID sid, bool load) {
        if (anyStream()) {
            auto available_stream = getFreeStream();
            map[sid].last_used = map[sid].used;
            map[sid].last_rename = map[sid].renamed;
            map[sid].used = true;
            map[sid].renamed = available_stream;
            map[sid].direction = load ? StreamMode::load : StreamMode::store;
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
        // if (map[sid].used)
        //     DPRINTF(UVESEI, "Lookup: Stream %d points to %d.\n", sid,
        //             map[sid].renamed);
        return std::make_pair(map[sid].used, map[sid].renamed);
    }

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
    std::pair<bool, StreamID> renameStreamLoad(StreamID sid) {
        return renameStream(sid, true);
    }
    std::pair<bool, StreamID> renameStreamStore(StreamID sid) {
        return renameStream(sid, false);
    }

    std::pair<bool, StreamID> getStreamStore(StreamID sid) {
        auto res = getStream(sid);
        if (res.first) {
            if (map[sid].direction == StreamMode::store) {
                return res;
            }
        }
        return std::make_pair(false, sid);
    }

    std::pair<bool, StreamID> getStreamLoad(StreamID sid) {
        auto res = getStream(sid);
        if (res.first) {
            if (map[sid].direction == StreamMode::load) {
                return res;
            }
        }
        return std::make_pair(false, sid);
    }

    std::pair<bool, StreamID> getStreamConfig(StreamID sid) {
        return getStream(sid);
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
    void _signalEngineReady(CallbackInfo info);
    static void signalEngineReady(CallbackInfo info) {
        SEInterface<Impl>::singleton->_signalEngineReady(info);
    }

    bool recvTimingResp(PacketPtr pkt);
    void recvTimingSnoopReq(PacketPtr pkt);
    void recvReqRetry();
    bool sendCommand(SECommand cmd);
    bool isReady(StreamID sid);
    bool fetch(StreamID sid, TheISA::VecRegContainer **cnt);

    void tick() { engine->tick(); }

    bool isStreamLoad(StreamID sid) {
        return stream_rename.getStreamLoad(sid).first;
    }
    bool isStreamStore(StreamID sid) {
        return stream_rename.getStreamStore(sid).first;
    }
    StreamID getAssertedStreamLoad(StreamID asid) {
        auto rename = stream_rename.getStreamLoad(asid);
        if (!rename.first) panic("Fatal Error in Stream Rename.");
        return rename.second;
    }
    StreamID getAssertedStreamStore(StreamID asid) {
        auto rename = stream_rename.getStreamStore(asid);
        if (!rename.first) panic("Fatal Error in Stream Rename.");
        return rename.second;
    }
    StreamID getStreamLoad(StreamID asid) {
        return stream_rename.getStreamLoad(asid).second;
    }
    StreamID getStreamStore(StreamID asid) {
        return stream_rename.getStreamStore(asid).second;
    }
    StreamID getStreamConfig(StreamID asid) {
        return stream_rename.getStreamConfig(asid).second;
    }

   private:  // Private Methods
    void clearBuffer(StreamID sid) {
        // Sid must be a real stream. Not a rename alias.
        // Release stream rename and clear rename
        stream_rename.freeStream(sid);
        // Clear register buffer
        registerBufferLoad[sid]->clear();

        DPRINTF(UVERename, "Cleared Stream %d\n", sid);
    }

   public:  // Common FIFO methods
    void squashDestToBuffer(const RegId &arch, PhysRegIdPtr phys,
                            InstSeqNum sn) {
        // This method is called from the history buffer rewind. This means
        // that we can get any type of stream. Be it config, load or store.
        if (!isInstMeaningful(sn)) return;
        if (isInstStreamConfig(sn)) {
            StreamID sid = getStreamConfig(arch.index());

            // Set the old renames
            auto result = stream_rename.undo(sid);
            if (result.first) {
                // registerBufferLoad[result.first]->clear();
                // speculationPointerLoad[result.first].clear();
                stream_rename.freeStream(result.first);
                DPRINTF(UVERename, "SquashDestToBuffer: %d, %p. SeqNum[%d]\n",
                        result.first, phys, sn);
            }
        } else {
            // squash can be both store and load. Let the methods solve this
            // problem themselfs
            if (isStreamLoad(arch.index())) squashToBufferLoad(arch, phys, sn);
            if (isStreamStore(arch.index()))
                squashToBufferStore(arch, phys, sn);
        }
    }
    void retireMeaningfulInst(InstSeqNum sn) {
        auto inst = inst_validator.find(sn);
        if (inst != inst_validator.end()) {
            if (inst->second > 0) {
                inst->second--;
            } else
                inst_validator.erase(sn);
        }
    }
    void insertMeaningfulInst(InstSeqNum sn) {
        auto inst = inst_validator.find(sn);
        if (inst == inst_validator.end()) {
            inst_validator.insert({sn, 0});
        } else
            inst->second++;
    }
    bool isInstMeaningful(InstSeqNum sn) {
        if (inst_validator.find(sn) == inst_validator.end())
            return false;
        else
            return true;
    }

   public:  // Config stream methods
    std::pair<bool, StreamID> initializeStreamConfig(StreamID sid,
                                                     InstSeqNum sn,
                                                     bool load) {
        // This method is only called on stream configs
        // Add inst sequence number to hash map
        insertMeaningfulInst(sn);
        std::pair<bool, StreamID> result;
        if (load) {
            result = stream_rename.renameStreamLoad(sid);
        } else {
            result = stream_rename.renameStreamStore(sid);
        }
        insertStreamConfigInst(sn, result.second);
        DPRINTF(UVERename,
                "Initialized Stream [%d], renamed to sid[%d] with inst "
                "SeqNum[%d]\n",
                sid, result.second, sn);
        return result;
    }
    void squashStreamConfig(StreamID asid, InstSeqNum sn) {
        if (isInstMeaningful(sn) && isInstStreamConfig(sn)) {
            StreamID sid = getStreamConfigSid(sn);
            stream_rename.freeStream(sid);
            retireStreamConfigInst(sn);
            engine->endStreamFromSquash(sid);
            DPRINTF(UVERename, "SquashStreamConfig: %d. SeqNum[%d]\n", sid,
                    sn);
        }
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

   public:  // Load FIFO methods
    bool isLoadStream(StreamID sid) {
        return stream_rename.getStreamLoad(sid).first;
    }
    void addToBufferLoad(const RegId &arch, PhysRegIdPtr phys, InstSeqNum sn) {
        insertMeaningfulInst(sn);
        StreamID sid = getAssertedStreamLoad(arch.index());

        DPRINTF(UVERename, "addToBufferLoad: %d, %p. SeqNum[%d]\n", sid, phys,
                sn);
        if (registerBufferLoad[sid]->empty()) {
            speculationPointerLoad[sid]++;
            // Also update the fifo speculationPointerLoad due to
            // desyncronization with This one
            engine->synchronizeLoadLists(sid);
        }
        registerBufferLoad[sid]->push_front(
            std::make_tuple(arch, phys, false, 0));
    }
    void markOnBufferLoad(const RegId &arch, PhysRegIdPtr phys,
                          InstSeqNum sn) {
        if (!isInstMeaningful(sn)) return;
        // Here we use getStream, because every register is called in the
        // parent instruction
        StreamID sid = getStreamLoad(arch.index());
        // DPRINTF(UVERename, "markOnBufferLoad: %p\n",phys);
        RegBufferIter it = registerBufferLoad[sid]->begin();
        while (it != registerBufferLoad[sid]->end()) {
            if (phys->index() == std::get<1>(*it)->index()) {
                std::get<2>(*it) = true;
            }
            it++;
        }
    }
    std::pair<RegId, PhysRegIdPtr> consumeOnBufferLoad(StreamID psid) {
        // Consume on buffer does not need stream lookup.
        // Sid is already physical

        if (!registerBufferLoad[psid]->empty() &&
            speculationPointerLoad[psid].valid()) {
            if (std::get<2>(*speculationPointerLoad[psid])) {
                auto retval =
                    std::make_pair(std::get<0>(*speculationPointerLoad[psid]),
                                   std::get<1>(*speculationPointerLoad[psid]));
                DPRINTF(UVERename, "consumeOnBufferLoad: %d, %p\n", psid,
                        std::get<1>(*speculationPointerLoad[psid]));
                speculationPointerLoad[psid]++;

                return retval;
            }
            return std::make_pair(RegId(), (PhysRegIdPtr)NULL);
        }
        return std::make_pair(RegId(), (PhysRegIdPtr)NULL);
    }
    void squashToBufferLoad(const RegId &arch, PhysRegIdPtr phys,
                            InstSeqNum sn) {
        if (!isInstMeaningful(sn)) return;
        StreamID sid = getStreamLoad(arch.index());
        DPRINTF(UVERename, "squashToBufferLoadTry: %d, %p. SeqNum[%d] \n",
                arch.index(), phys, sn);

        if (!registerBufferLoad[sid]->empty()) {
            if (std::get<1>(registerBufferLoad[sid]->front()) == phys &&
                std::get<0>(registerBufferLoad[sid]->front()) == arch) {
                registerBufferLoad[sid]->pop_front();
                auto lookup_stream = stream_rename.getStreamLoad(arch.index());
                if (engine->shouldSquashLoad(lookup_stream.second).isOk()) {
                    SmartReturn result =
                        engine->squashLoad(lookup_stream.second);
                    if (result.isNok() || result.isError())
                        panic("Error" + result.estr());
                    DPRINTF(UVERename,
                            "squashToBufferLoad: %d, %p. SeqNum[%d] \t "
                            "Result:%s\n",
                            arch.index(), phys, sn, result.str());
                } else {
                    DPRINTF(UVERename,
                            "squashToBufferLoadOnly: %d, %p. SeqNum[%d]\n",
                            arch.index(), phys, sn);
                }
            }
        }
    }
    void commitToBufferLoad(const RegId &arch, PhysRegIdPtr phys,
                            InstSeqNum sn) {
        if (!isInstMeaningful(sn)) return;
        // JMFIXME: Not sure if this does anything. Maybe it should be done
        // in a specific method for stream config
        retireStreamConfigInst(sn);

        StreamID sid = getStreamLoad(arch.index());
        DPRINTF(UVERename, "CommitToBufferLoadTry: %d, %p. SeqNum[%d]. \n",
                sid, phys, sn);
        auto bufferEnd = registerBufferLoad[sid]->back();
        if (std::get<2>(bufferEnd)) {
            if (std::get<1>(bufferEnd) == phys &&
                std::get<0>(bufferEnd) == arch) {
                registerBufferLoad[sid]->pop_back();
                speculationPointerLoad[sid]--;
                auto lookup_stream = stream_rename.getStreamLoad(
                    std::get<0>(bufferEnd).index());
                SmartReturn result = engine->commitLoad(lookup_stream.second);
                DPRINTF(UVERename,
                        "commitToBufferLoad: %d, %p. SeqNum[%d]. \t "
                        "Result:%s\n",
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
    bool checkLoadOccupancy(const RegId &arch) {
        StreamID psid = getStreamLoad(arch.index());
        return engine->ld_fifo.fifo_depth > registerBufferLoad[psid]->size();
    }

   public:  // Store FIFO Methods
    bool isStoreStream(StreamID sid) {
        return stream_rename.getStreamStore(sid).first;
    }
    void addToBufferStore(const RegId &arch, PhysRegIdPtr phys,
                          InstSeqNum sn) {
        insertMeaningfulInst(sn);
        StreamID sid = getAssertedStreamStore(arch.index());

        DPRINTF(UVERename, "addToBufferStore: %d, %p. SeqNum[%d]\n", sid, phys,
                sn);
        if (registerBufferStore[sid]->empty()) {
            speculationPointerStore[sid]++;
            // Also update the fifo speculationPointerLoad due to
            // desyncronization with this one
            engine->synchronizeStoreLists(sid);
        }
        SubStreamID ssid = engine->reserveStoreEntry(sid);
        registerBufferStore[sid]->push_front(
            std::make_tuple(arch, phys, false, ssid));
    }
    bool fillOnBufferStore(const RegId &arch, PhysRegIdPtr phys,
                           CoreContainer val, InstSeqNum sn) {
        // Fills the fifo entry with the data coming from the cpu
        // Sid is already physical
        if (!isInstMeaningful(sn)) return false;

        DPRINTF(UVERename, "fillToBufferStoreTry: %d, %p. SeqNum[%d] \n",
                arch.index(), phys, sn);
        StreamID sid = getStreamStore(arch.index());
        if (!registerBufferStore[sid]->empty()) {
            auto iter = registerBufferStore[sid]->begin();
            while (iter != registerBufferStore[sid]->end()) {
                if (std::get<1>(*iter) == phys && std::get<0>(*iter) == arch) {
                    DPRINTF(UVERename,
                            "fillOnBufferStore: %d, %p. SeqNum[%d]\n", sid,
                            phys, sn);
                    SubStreamID ssid = std::get<3>(*iter);
                    engine->setDataStore(sid, ssid, val);
                    return true;
                }
                iter++;
            }
        }
        return false;
    }

    void squashToBufferStore(const RegId &arch, PhysRegIdPtr phys,
                             InstSeqNum sn) {
        if (!isInstMeaningful(sn)) return;
        StreamID sid = getStreamStore(arch.index());

        DPRINTF(UVERename, "squashToBufferStoreTry: %d, %p. SeqNum[%d] \n",
                arch.index(), phys, sn);
        if (!registerBufferStore[sid]->empty()) {
            if (std::get<1>(registerBufferStore[sid]->front()) == phys &&
                std::get<0>(registerBufferStore[sid]->front()) == arch) {
                SubStreamID ssid =
                    std::get<3>(registerBufferStore[sid]->front());
                registerBufferStore[sid]->pop_front();
                auto lookup_stream =
                    stream_rename.getStreamStore(arch.index());
                if (engine->shouldSquashStore(lookup_stream.second).isOk()) {
                    SmartReturn result =
                        engine->squashStore(lookup_stream.second, ssid);
                    if (result.isNok() || result.isError())
                        panic("Error" + result.estr());
                    DPRINTF(UVERename,
                            "squashToBufferStore: %d, %p. SeqNum[%d] \t "
                            "Result:%s\n",
                            arch.index(), phys, sn, result.str());
                } else {
                    DPRINTF(UVERename,
                            "squashToBufferStoreOnly: %d, %p. SeqNum[%d]\n",
                            arch.index(), phys, sn);
                }
            }
        }
    }
    void commitToBufferStore(const RegId &arch, PhysRegIdPtr phys,
                             InstSeqNum sn) {
        if (!isInstMeaningful(sn)) return;
        StreamID sid = getStreamStore(arch.index());

        DPRINTF(UVERename, "commitToBufferStoreTry: %d, %p. SeqNum[%d] \n",
                arch.index(), phys, sn);
        auto bufferEnd = registerBufferStore[sid]->back();
        if (std::get<1>(bufferEnd) == phys && std::get<0>(bufferEnd) == arch) {
            SubStreamID ssid = std::get<3>(bufferEnd);
            registerBufferStore[sid]->pop_back();
            speculationPointerStore[sid]--;
            auto lookup_stream =
                stream_rename.getStreamStore(std::get<0>(bufferEnd).index());
            SmartReturn result =
                engine->commitStore(lookup_stream.second, ssid);
            DPRINTF(UVERename,
                    "commitToBufferStore: %d, %p. SeqNum[%d]. \t "
                    "Result:%s\n",
                    sid, phys, sn, result.str());
            if (result.isEnd()) {
                clearBuffer(lookup_stream.second);
            }
            retireMeaningfulInst(sn);
            if (result.isNok() || result.isError())
                panic("Error" + result.estr());
        }
    }
    bool checkStoreOccupancy(const RegId &arch) {
        StreamID psid = getStreamStore(arch.index());
        bool res =
            engine->st_fifo.fifo_depth > registerBufferStore[psid]->size();
        bool res2 = engine->st_fifo.full(psid).isNok();
        return res && res2;
    }

   private:  // Data
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

    using RegBufferList =
        std::list<std::tuple<RegId, PhysRegIdPtr, bool, int>>;
    using RegBufferIter = RegBufferList::iterator;
    using SpecBufferIter = DumbIterator<RegBufferList>;
    using InstSeqNumSet = std::unordered_map<uint64_t, uint8_t>;
    using StreamConfigHashMap = std::unordered_map<uint64_t, StreamID>;

    std::vector<RegBufferList *> registerBufferLoad, registerBufferStore;
    std::vector<SpecBufferIter> speculationPointerLoad,
        speculationPointerStore;

    StreamRename stream_rename;
    InstSeqNumSet inst_validator;
    StreamConfigHashMap stream_config_validator;
};

#endif  //__CPU_O3_SE_INTERFACE_HH__