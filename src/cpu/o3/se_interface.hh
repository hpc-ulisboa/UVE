
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

class RegBufferCont {
   public:
    RegId areg/*Arch Reg*/;
    PhysRegIdPtr preg /*Phys Reg*/;
    bool prog /*Progress Control*/;
    int ssid /*SSID*/;
    bool sqct /*Squash Control*/;

    RegBufferCont(RegId arch, PhysRegIdPtr phys){
        areg = arch;
        preg = phys;
        prog = false;
        ssid = 0;
        sqct = false;
    };
    RegBufferCont(RegId arch, PhysRegIdPtr phys, int ssids){
        areg = arch;
        preg = phys;
        prog = false;
        ssid = ssids;
        sqct = false;
    };

    ~RegBufferCont() {};
};

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
    using FreePool = std::deque<StreamID>;
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
        pool.pop_front();
        return retval;
    }
    StreamID getFreeStream(uint64_t sid) {
        auto it = pool.begin();
        while (it != pool.end()){
            if (*it == sid) pool.erase(it);
            it++;
        }
        return sid;
    }

    bool anyStream() { return pool.size() != 0; }

    std::tuple<bool, StreamID, StreamID> renameStream(StreamID sid, bool load,
                                                    bool rename){
        if (anyStream()) {
            if(rename) {
                auto available_stream = getFreeStream();
                map[sid].last_used = map[sid].used;
                map[sid].last_rename = map[sid].renamed;
                map[sid].used = true;
                map[sid].renamed = available_stream;
                map[sid].direction = load ? StreamMode::load : StreamMode::store;
                DPRINTF(UVESEI, "Rename: Stream %d was renamed to %d\n", sid,
                        available_stream);
                return std::make_tuple(true, available_stream,
                map[sid].last_rename);
            } else {
                auto available_stream = getFreeStream(sid);
                map[sid].last_used = map[sid].used;
                map[sid].last_rename = map[sid].renamed;
                map[sid].used = true;
                map[sid].renamed = available_stream;
                map[sid].direction = load ? StreamMode::load : StreamMode::store;
                DPRINTF(UVESEI, "Rename: Stream %d was renamed to %d\n", sid,
                        available_stream);
                return std::make_tuple(true, available_stream,
                map[sid].last_rename);
            }
        } else {
            DPRINTF(
                UVESEI,
                "Rename: Stream %d was not renamed, no streams available.\n",
                sid);
            return std::make_tuple(false, (StreamID)0, (StreamID)0);
        }
    }

    std::pair<bool, StreamID> getStream(StreamID sid) {
        // if (map[sid].used)
        //     DPRINTF(UVESEI, "Lookup: Stream %d points to %d.\n", sid,
        //             map[sid].renamed);
        return std::make_pair(map[sid].used, map[sid].renamed);
    }

    bool availableStream(){
        std::stringstream stro;
        FreePool tmp_q = pool; //copy the original queue to a temporary queue
        uint64_t q_element = 0;
        while (!tmp_q.empty())
        {
            q_element = (uint64_t)tmp_q.front();
            stro << q_element <<" ,";
            tmp_q.pop_front();
        } 
        DPRINTF(UVESEI, "Free Streams Status s(%d): %s\n", pool.size(),
                                                        stro.str().c_str());
        return pool.size() > 0;
    }


    bool availableStream(uint64_t sid){
        std::stringstream stro;
        bool can_rename = false;
        FreePool tmp_q = pool; //copy the original queue to a temporary queue
        uint64_t q_element = 0;
        while (!tmp_q.empty())
        {
            q_element = (uint64_t)tmp_q.front();
            if(q_element == sid) can_rename = true;
            tmp_q.pop_front();
        }
        return can_rename;
    }
   public:
    void freeStream(StreamID sid) {
        pool.push_front(sid);
        auto result = reverseLookup(sid);
        if (result.first) {
            map[result.second].used = false;
        }
        DPRINTF(UVESEI, "Freed Stream %d\n", sid);
    }

   public:
    bool canRenameStream(){
        return availableStream();
    }
    bool canRenameStream(uint64_t sid){
        return availableStream(sid);
    }

    std::tuple<bool, StreamID, StreamID> renameStreamLoad(StreamID sid) {
        return renameStream(sid, true, true);
    }
    std::tuple<bool, StreamID, StreamID> renameStreamStore(StreamID sid) {
        return renameStream(sid, false, true);
    }
    std::tuple<bool, StreamID, StreamID> nrenameStreamLoad(StreamID sid) {
        return renameStream(sid, true, false);
    }
    std::tuple<bool, StreamID, StreamID> nrenameStreamStore(StreamID sid) {
        return renameStream(sid, false, false);
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

    bool rollback(StreamID sid) {
        if (!map[sid].last_used) {
            pool.push_front(map[sid].renamed);
            map[sid].used = false;
            map[sid].renamed = 0;
            return false;
        }
        pool.push_front(map[sid].renamed);

        map[sid].used = map[sid].last_used;
        map[sid].renamed = map[sid].last_rename;
        
        map[sid].last_used = false;
        map[sid].last_rename = 0;
        
        return true;
    }

    std::string print(){
        std::stringstream stro;
        FreePool tmp_q = pool; //copy the original queue to a temporary queue
        uint64_t q_element = 0;
        stro << "Free Streams:";
        while (!tmp_q.empty())
        {
            q_element = (uint64_t)tmp_q.front();
            stro << q_element <<" ,";
            tmp_q.pop_front();
        }
        stro << "|  Status: ";
        for(int i = 0; i < map.size(); i ++){
            stro << "(" << i <<  "): " << ((int)map[i].renamed) << 
                (map[i].used ? "[U]:" : ":") <<
                ((int)map[i].last_rename) << (map[i].last_used ? "[U]" : "")
                << ", ";
        }
        return stro.str();
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
    bool sendCommand(SECommand cmd, InstSeqNum sn);
    bool isReady(StreamID sid);

    void tick();

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

    bool isRenameActive(){
        return engine->isRenameActive();
    }

   private:  // Private Methods
    void clearBuffer(StreamID sid) {
        // Sid must be a real stream. Not a rename alias.
        // Release stream rename and clear rename
        stream_rename.freeStream(sid);
        // Clear register buffer
        registerBufferLoad[sid]->clear();
        registerBufferLoadStatus[sid] = false;
        registerBufferLoadOutstanding[sid] = 0;
        std::stringstream stro;
        for( int i = 0; i < registerBufferLoadStatus.size(); i++){
            if(registerBufferLoadStatus[i] == true){
                stro << i << "("<< registerBufferLoadOutstanding[i] <<")" <<" ,";   
            }
        }
        DPRINTF(JMDEVEL, "del-registerBufferLoad Status: removed(%d) %s\n", sid, stro.str().c_str() );


        DPRINTF(UVERename, "Cleared Stream %d\n", sid);
    }
    void clearStoreBuffer(StreamID sid) {
        // Sid must be a real stream. Not a rename alias.
        // Release stream rename and clear rename
        stream_rename.freeStream(sid);
        // Clear register buffer
        registerBufferStore[sid]->clear();

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
                stream_rename.freeStream(result.second);
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

    /* True if any stream is available for configuration
        Args: None
        Returns: bool (true if any stream is available)
    */
    bool isStreamAvailable(){
        return stream_rename.canRenameStream();
    }
    bool isStreamAvailable(uint64_t sid){
        return stream_rename.canRenameStream(sid);
    }

    std::tuple<bool, StreamID, StreamID> initializeStreamConfig(StreamID sid,
                                                     InstSeqNum sn,
                                                     bool load) {
        // This method is only called on stream configs
        // Add inst sequence number to hash map
        insertMeaningfulInst(sn);
        std::tuple<bool, StreamID, StreamID> result;
        if(engine->isRenameActive()){
            if (load) {
                result = stream_rename.renameStreamLoad(sid);
            } else {
                result = stream_rename.renameStreamStore(sid);
            }
        } else {
            if (load) {
                result = stream_rename.nrenameStreamLoad(sid);
            } else {
                result = stream_rename.nrenameStreamStore(sid);
            }
        }
        insertStreamConfigInst(sn, std::get<1>(result));
        engine->addStreamConfig(std::get<1>(result), sn);
        DPRINTF(UVERename,
                "Initialized Stream [%d], renamed to sid[%d] (last:%d) "
                "with inst SeqNum[%d]\n",
                sid, std::get<1>(result), std::get<2>(result), sn);
        return result;
    }

    std::pair<bool, StreamID> getStreamConfigSequence(StreamID sid,
                                                      InstSeqNum sn) {
        // This method is only called on stream configs
        // Add inst sequence number to hash map
        std::pair<bool, StreamID> result;

        result = stream_rename.getStreamConfig(sid);
        engine->addStreamConfig(result.second, sn);
        DPRINTF(UVERename,
                "Initialization Sequence Stream [%d], looked up sid[%d]\n",
                sid, result.second);
        return result;
    }
    void squashStreamConfig(StreamID asid, InstSeqNum sn) {
        if (isInstMeaningful(sn) && isInstStreamConfig(sn)) {
            StreamID sid = getStreamConfigSid(sn);
            DPRINTF(JMDEVEL,"StreamRename: %s\n",
                    stream_rename.print().c_str());
            stream_rename.rollback(asid);
            retireStreamConfigInst(sn);
            engine->squashStreamConfig(sid, sn);
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
    StreamID addToBufferLoad(const RegId &arch, PhysRegIdPtr phys,
                            InstSeqNum sn) {
        insertMeaningfulInst(sn);
        StreamID sid = getAssertedStreamLoad(arch.index());

        DPRINTF(UVERename, "addToBufferLoad: %d, %p. SeqNum[%d]\n", sid, phys,
                sn);
        if (registerBufferLoad[sid]->empty()) {
            speculationPointerLoad[sid]++;
            // Also update the fifo speculation Pointer Load due to
            // desyncronization with This one
            engine->synchronizeLoadLists(sid);
        }
        registerBufferLoad[sid]->push_front(
            RegBufferCont(arch, phys, sid));

        if(registerBufferLoadStatus[sid] == true)
            registerBufferLoadOutstanding[sid] ++;
        return sid;
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
            if (phys->index() == it->preg->index()) {
                it->prog = true;
            }
            it++;
        }
    }
    std::tuple<RegId, PhysRegIdPtr, StreamID> consumeOnBufferLoad(StreamID psid) {
        // Consume on buffer does not need stream lookup.
        // Sid is already physical

        if (!registerBufferLoad[psid]->empty() &&
            speculationPointerLoad[psid].valid()) {
            if (speculationPointerLoad[psid]->prog) {
                auto retval =
                    std::make_tuple(speculationPointerLoad[psid]->areg,
                                   speculationPointerLoad[psid]->preg,
                                   speculationPointerLoad[psid]->ssid);
                speculationPointerLoad[psid]->sqct = true;
                DPRINTF(UVERename, "consumeOnBufferLoad: %d, %p, (cons)%d\n",
                        psid,
                        speculationPointerLoad[psid]->preg,
                        speculationPointerLoad[psid]->sqct);
                speculationPointerLoad[psid]++;

                return retval;
            }
            return std::make_tuple(RegId(), (PhysRegIdPtr) nullptr, 0);
        }
        return std::make_tuple(RegId(), (PhysRegIdPtr) nullptr, 0);
    }
    void squashToBufferLoad(const RegId &arch, PhysRegIdPtr phys,
                            InstSeqNum sn) {
        
        if (!isInstMeaningful(sn)) return;
        StreamID sid = getStreamLoad(arch.index());
        DPRINTF(UVERename, "squashToBufferLoadTry: %d, %p. SeqNum[%d] \n",
                arch.index(), phys, sn);

        std::stringstream sout;

        if (!registerBufferLoad[sid]->empty()) {
            if (registerBufferLoad[sid]->front().preg == phys &&
                registerBufferLoad[sid]->front().areg == arch) {
                bool already_consumed =
                    registerBufferLoad[sid]->front().sqct;
                

                auto it = registerBufferLoad[sid]->begin();
                while (it != registerBufferLoad[sid]->end()) {
                    sout << "(" << it->areg << "," <<
                                   it->preg << "," <<
                                   it->prog << "," <<
                                   it->ssid << "," <<
                                   it->sqct << ");  ";
                    it++;
                }

                DPRINTF(UVERename, "registerBufferLoad[%d]: %s\n", 
                                    sid ,sout.str().c_str() );

                registerBufferLoad[sid]->pop_front();
                auto lookup_stream = stream_rename.getStreamLoad(arch.index());
                if (already_consumed &&
                    engine->shouldSquashLoad(lookup_stream.second).isOk()) {
                    //JMFIXME: If squash targets a last: remove last flag.
                    SmartReturn result =
                        engine->squashLoad(lookup_stream.second);
                    if (result.isNok() || result.isError()) panic("Error");
                    DPRINTF(UVERename,
                            "squashToBufferLoad: %d, %p. SeqNum[%d] \t "
                            "Result:%s\n",
                            arch.index(), phys, sn,
                            result.isOk() ? "Ok" : "Nok");
                } else {
                    DPRINTF(UVERename,
                            "squashToBufferLoadOnly: %d, %p. SeqNum[%d]\n",
                            arch.index(), phys, sn);
                }
            }
        }
    }
    void commitToBufferLoad(const RegId &arch, PhysRegIdPtr phys,
                            InstSeqNum sn, uint64_t sid) {
        if (!isInstMeaningful(sn)) return;
        // JMFIXME: Not sure if this does anything. Maybe it should be done
        // in a specific method for stream config
        retireStreamConfigInst(sn);
        DPRINTF(UVERename, "CommitToBufferLoadTry: %d, %p. SeqNum[%d]. \n",
                sid, phys, sn);

        std::stringstream sout;
        auto it = registerBufferLoad[sid]->begin();
        while (it != registerBufferLoad[sid]->end()) {
            sout << "(" << it->areg << "," <<
                            it->preg << "," <<
                            it->prog << "," <<
                            it->ssid << "," <<
                            it->sqct << ");  ";
            it++;
        }
        DPRINTF(UVERename, "registerBufferLoad[%d]: %s\n",
                sid, sout.str().c_str() );
        
        auto bufferEnd = registerBufferLoad[sid]->back();
        if (bufferEnd.prog) {
            if (bufferEnd.preg == phys &&
                bufferEnd.areg == arch) {
                registerBufferLoad[sid]->pop_back();
                speculationPointerLoad[sid]--;
                SmartReturn result = engine->commitLoad(sid);
                DPRINTF(
                    UVERename,
                    "commitToBufferLoad: %d, %p. SeqNum[%d]. \t "
                    "Result:%s\n",
                    sid, phys, sn,
                    result.isOk() ? "Ok" : (result.isEnd() ? "End" : "Nok"));
                if (result.isEnd()) {
                    clearBuffer(sid);
                }
                retireMeaningfulInst(sn);
                if (result.isNok() || result.isError()) panic("Error");
            }
        } else {
            //Try the last rename (this can happen due to a bad rename)
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
            RegBufferCont(arch, phys, ssid));
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
                if (iter->preg == phys && iter->areg == arch) {
                    DPRINTF(UVERename,
                            "fillOnBufferStore: %d, %p. SeqNum[%d]\n", sid,
                            phys, sn);
                    SubStreamID ssid = iter->ssid;
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
            if (registerBufferStore[sid]->front().preg == phys &&
                registerBufferStore[sid]->front().areg == arch) {
                SubStreamID ssid =
                    registerBufferStore[sid]->front().ssid;
                registerBufferStore[sid]->pop_front();
                auto lookup_stream =
                    stream_rename.getStreamStore(arch.index());
                if (engine->shouldSquashStore(lookup_stream.second).isOk()) {
                    SmartReturn result =
                        engine->squashStore(lookup_stream.second, ssid);
                    if (result.isNok() || result.isError()) panic("Error");
                    DPRINTF(UVERename,
                            "squashToBufferStore: %d, %p. SeqNum[%d] \t "
                            "Result:%s\n",
                            arch.index(), phys, sn,
                            result.isOk() ? "Ok" : "Nok");
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
        if (bufferEnd.preg == phys && bufferEnd.areg == arch) {
            SubStreamID ssid = bufferEnd.ssid;
            registerBufferStore[sid]->pop_back();
            speculationPointerStore[sid]--;
            auto lookup_stream =
                stream_rename.getStreamStore(bufferEnd.areg.index());
            SmartReturn result =
                engine->commitStore(lookup_stream.second, ssid);
            DPRINTF(UVERename,
                    "commitToBufferStore: %d, %p. SeqNum[%d]. \t "
                    "Result:%s\n",
                    sid, phys, sn, result.isOk() ? "Ok" : "Nok");
            retireMeaningfulInst(sn);
            if (result.isNok() || result.isError()) panic("Error");
        }
    }
    bool checkStoreOccupancy(const RegId &arch) {
        StreamID psid = getStreamStore(arch.index());
        bool res =
            engine->st_fifo.fifo_depth > registerBufferStore[psid]->size();
        bool res2 = engine->st_fifo.full(psid).isNok();
        return res && res2;
    }
    void clearStoreStream(StreamID sid) { clearStoreBuffer(sid); }

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
        std::list<RegBufferCont>;
    /***Squash Control and Progress Control***
     *            Init   Mark    Cons    Comm
     * Progress     0     1        1       1
     * Squash       0     0        1       1
     */
    using RegBufferIter = RegBufferList::iterator;
    using SpecBufferIter = DumbIterator<RegBufferList>;
    using InstSeqNumSet = std::unordered_map<uint64_t, uint8_t>;
    using StreamConfigHashMap = std::unordered_map<uint64_t, StreamID>;

    std::vector<RegBufferList *> registerBufferLoad, registerBufferStore;
    std::vector<SpecBufferIter> speculationPointerLoad,
        speculationPointerStore;
    std::vector<bool> registerBufferLoadStatus;
    std::vector<int> registerBufferLoadOutstanding;

    StreamRename stream_rename;
    InstSeqNumSet inst_validator;
    StreamConfigHashMap stream_config_validator;
};

#endif  //__CPU_O3_SE_INTERFACE_HH__