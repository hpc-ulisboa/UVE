
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
      registerBuffer(32),
      speculationPointer(32) {
    dcachePort = &(cpu_ptr->getDataPort());
    if (params->streamEngine.size() == 1) {
        engine = params->streamEngine[0];
    } else {
        engine = nullptr;
    }
    SEInterface<Impl>::singleton = this;

    for (int i = 0; i < 32; i++) {
        registerBuffer[i] = new RegBufferList();
        speculationPointer[i] = SpecBufferIter(registerBuffer[i]);
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
    //JMFIXME: For now leave as this, but this must me addressed
}

template <class Impl>
bool
SEInterface<Impl>::sendCommand(SECommand cmd){
    return engine->recvCommand(cmd);
}

template <class Impl>
bool
SEInterface<Impl>::fetch(StreamID sid, TheISA::VecRegContainer **cnt) {
    auto renamed = stream_rename.getStream(sid);
    assert(renamed.first || "Rename Should be true at this point");
    return engine->ld_fifo.fetch(renamed.second, cnt);
}

template <class Impl>
bool
SEInterface<Impl>::isReady(StreamID sid) {
    auto renamed = stream_rename.getStream(sid);
    assert(renamed.first || "Rename Should be true at this point");
    return engine->ld_fifo.ready(renamed.second);
}

template <class Impl>
void
SEInterface<Impl>::sendData(int physIdx, TheISA::VecRegContainer *cnt) {
    // DPRINTF(JMDEVEL, "Send Data Here: idx: %d, cnt: %s\n", physIdx,
    //         cnt->print());
    // // Get PhysRegIdPtr from index
    // PhysRegIdPtr physReg =
    //     cpu->getRenameCpuPtr()->get_reg_id_by_index(physIdx);
    // // Set data in reg file
    // cpu->setVecReg(physReg, *cnt);
    // // Wake Dependents and set scoreboard
    // iewStage->instQueue.wakeDependents(physReg);
}

template <class Impl>
void
SEInterface<Impl>::_signalEngineReady(CallbackInfo info) {
    // DPRINTF(JMDEVEL, "Send Data Here: idx: %d, cnt: %s\n", physIdx,
    //         cnt->print());
    // Get PhysRegIdPtr from index
    // Get Streamed Registers
    for (int i = 0; i < info.psids_size; i++) {
        StreamID psid = info.psids[i];

        auto phys_lookup_result = stream_rename.reverseLookup(psid);
        assert(phys_lookup_result.first ||
               "Reverse lookup should always be true");

        auto regs = consumeOnBuffer(phys_lookup_result.second);
        if (regs.second == NULL) continue;
        // Ask Engine for the data
        auto lookup_result = getStream(regs.first.index());
        CoreContainer *cnt = engine->getData(lookup_result);
        assert(cnt->is_streaming());
        // Set data in reg file
        cpu->setVecReg(regs.second, *cnt);
        // Wake Dependents and set scoreboard
        iewStage->instQueue.wakeDependents(regs.second);
        return;
    }
    assert(false || "Code should be unreacheable");
}

#endif  // __CPU_O3_SE_INTERFACE_IMPL_HH__