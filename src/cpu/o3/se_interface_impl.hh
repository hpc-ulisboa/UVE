
#ifndef __CPU_O3_SE_INTERFACE_IMPL_HH__
#define __CPU_O3_SE_INTERFACE_IMPL_HH__

#include "base/logging.hh"
#include "cpu/o3/se_interface.hh"
#include "params/DerivO3CPU.hh"

using namespace std;

template <class Impl>
SEInterface<Impl>::SEInterface(O3CPU *cpu_ptr, Decode *dec_ptr, IEW *iew_ptr,
                               Commit *cmt_ptr, DerivO3CPUParams *params)
    : cpu(cpu_ptr),
      decStage(dec_ptr),
      iewStage(iew_ptr),
      cmtStage(cmt_ptr),
      UVECondLookup(),
      registerBufferLoad(32),
      registerBufferStore(32),
      speculationPointerLoad(32),
      speculationPointerStore(32),
      registerBufferLoadStatus(32),
      registerBufferLoadOutstanding(32),
      inst_validator() {
    dcachePort = &(cpu_ptr->getDataPort());
    if (params->streamEngine.size() == 1) {
        engine = params->streamEngine[0];
    } else {
        engine = nullptr;
    }
    SEInterface<Impl>::singleton = this;

    for (int i = 0; i < 32; i++) {
        registerBufferLoad[i] = new RegBufferList();
        speculationPointerLoad[i] = SpecBufferIter(registerBufferLoad[i]);
        registerBufferStore[i] = new RegBufferList();
        speculationPointerStore[i] = SpecBufferIter(registerBufferStore[i]);
        registerBufferLoadStatus[i] = false;
        registerBufferLoadOutstanding[i] = 0;
    }
}

template<class Impl>
void
SEInterface<Impl>::startupComponent()
{
    // JMNOTE: Now that we have the port, get the address range.
    AddrRangeList sengine_addrs = dcachePort->getAddrRanges();
    Addr max_start_addr = 0;
    Addr max_end_addr = 0;
    for (auto const& e: sengine_addrs)
    {
        if (max_start_addr < e.start()) {
            max_start_addr = e.start();
            max_end_addr = e.end();
        }
    }
    sengine_addr_range = AddrRange(max_start_addr,max_end_addr);
    DPRINTF(JMDEVEL, "SEI: Address list is %s\n",
             sengine_addr_range.to_string());

    DPRINTF(JMDEVEL, "Calling\n");
    engine->set_callback(&signalEngineReady);
}

template <class Impl>
bool
SEInterface<Impl>::recvTimingResp(PacketPtr pkt)
{
    if (sengine_addr_range.contains(pkt->getAddr())) {
        //JMTODO: Handle the request, it is directed
        DPRINTF(UVESEI, "recvTimingResp addr[%#x]\n",pkt->getAddr());
        return true;
    }
    else {
        return iewStage->ldstQueue.recvTimingResp(pkt);
    }
}

template <class Impl>
void
SEInterface<Impl>::recvTimingSnoopReq(PacketPtr pkt)
{
    if (sengine_addr_range.contains(pkt->getAddr())) {
        //JMTODO: Handle the request, it is directed
        DPRINTF(UVESEI, "recvTimingSnoopResp addr[%#x]\n",pkt->getAddr());
        return;
    }
    else {
        return iewStage->ldstQueue.recvTimingSnoopReq(pkt);
    }
}

template <class Impl>
void
SEInterface<Impl>::recvReqRetry()
{
    iewStage->ldstQueue.recvReqRetry();
}

template <class Impl>
bool
SEInterface<Impl>::sendCommand(SECommand cmd, InstSeqNum sn) {
    SmartReturn result = engine->recvCommand(cmd, sn);
    if (result.isError()) panic("Error");
    return result.isOk();
}

template <class Impl>
bool
SEInterface<Impl>::isReady(StreamID sid) {
    auto renamed = stream_rename.getStreamLoad(sid);
    assert(renamed.first || "Rename Should be true at this point");
    SmartReturn result = engine->ld_fifo.ready(renamed.second);
    if (result.isError()) panic("Error");
    return result.isOk();
}

template <class Impl>
void
SEInterface<Impl>::_signalEngineReady(CallbackInfo info) {
    // DPRINTF(JMDEVEL, "Send Data Here: idx: %d, cnt: %s\n", physIdx,
    //         cnt->print());
    // Get PhysRegIdPtr from index
    // Get Streamed Registers
    if (info.is_clear) {
        clearStoreStream(info.clear_sid);
        return;
    }

    for (int i = 0; i < info.psids_size; i++) {
        StreamID psid = info.psids[i];

        // DPRINTF(JMDEVEL, "Trying to consume on signal(%d)\n", psid );

        auto regs = consumeOnBufferLoad(psid);
        // DPRINTF(JMDEVEL, "Consumed on signal(%d): %d, %p, sid(%d) \n", psid,
        //                         std::get<0>(regs),std::get<1>(regs),
        //                         std::get<2>(regs) );

        if (std::get<1>(regs) == nullptr) continue;
        // Ask Engine for the data
        SmartReturn result = engine->getDataLoad(std::get<2>(regs));
        DPRINTF(JMDEVEL, "Consumed on signal(%d):Data: (%d) %s \n", psid,
                                std::get<2>(regs),
                                result.isOk() ? "OK" : "NOK" );
        if (result.isNok()) continue;
        CoreContainer *cnt = (CoreContainer *)result.getData();
        // assert(cnt->is_streaming());
        // Set data in reg file
        cpu->setVecReg(std::get<1>(regs), *cnt);

        // If the container marks the end of a transaction: mark the
        // registerBufferLoad as ended
        if(cnt->is_last(DimensionHop::last)){
            registerBufferLoadStatus[psid] = true; 
            std::stringstream stro;
            for( int i = 0; i < registerBufferLoadStatus.size(); i++){
                if(registerBufferLoadStatus[i] == true){
                    stro << i << " ,";   
                }
            }
            DPRINTF(JMDEVEL, "registerBufferLoad Status: new(%d) %s\n", psid, stro.str().c_str() );
        } 

        delete cnt;
        // Wake Dependents and set scoreboard
        iewStage->instQueue.wakeDependents(std::get<1>(regs));
        return;
    }
    assert(false || "Code should be unreacheable");
}


template <class Impl>
void SEInterface<Impl>::tick() { 
    std::tuple<RegId, PhysRegIdPtr,StreamID> regis;
    
    engine->tick();
    //Solve the outstanding requests issue:
#if defined (UVE_OUTSTANDING_DETECTION) 
    for(int i = 0; i < registerBufferLoadStatus.size(); i++){
        if (registerBufferLoadStatus[i] == true &&
                    registerBufferLoadOutstanding[i] > 0 ){
            
            DPRINTF(JMDEVEL, "Detecting Outstanding (%d)\n", i);
            regis = consumeOnBufferLoad(i);
            if (std::get<1>(regis) == nullptr) continue;
            iewStage->instQueue.wakeDependents(std::get<1>(regis));
            registerBufferLoadOutstanding[i] --;
            if (registerBufferLoadOutstanding[i] < 0) 
                registerBufferLoadOutstanding[i] = 0;
            DPRINTF(JMDEVEL, "Outstanding (%d) with phys(%p); Sent ghost vector\n", i, std::get<1>(regis));
        }
    }
#endif
}

#endif  // __CPU_O3_SE_INTERFACE_IMPL_HH__