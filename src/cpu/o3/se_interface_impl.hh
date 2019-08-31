
#ifndef __CPU_O3_SE_INTERFACE_IMPL_HH__
#define __CPU_O3_SE_INTERFACE_IMPL_HH__

#include "base/logging.hh"
#include "cpu/o3/se_interface.hh"
#include "params/DerivO3CPU.hh"

using namespace std;

template <class Impl>
SEInterface<Impl>::SEInterface(O3CPU *cpu_ptr, Decode *dec_ptr, IEW *iew_ptr,
                Commit *cmt_ptr, DerivO3CPUParams *params)
            : cpu(cpu_ptr), decStage(dec_ptr), iewStage(iew_ptr),
                cmtStage(cmt_ptr)
            {
                dcachePort = &(cpu_ptr->getDataPort());
                if( params->streamEngine.size() == 1 ){
                    engine = params->streamEngine[0];
                }
                else{
                    engine = nullptr;
                }
            }

template<class Impl>
void
SEInterface<Impl>::startupComponent()
{
    //JMNOTE: Now that we have the port, get the address range.
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

// template <class Impl>
// void
// SEInterface<Impl>::configureStream(Stream stream, Dimension dim)
// {
//     //Create Request for Physical device (l.427 request.hh)
//         //Address: StreamID + BaseAddress
//     Addr req_addr = sengine_addr_range.start() + stream.getID();
//     Flags req_flags = Request::Flags();
//     MasterID req_mid = cpu->dataMasterId();

//     //There are no memflags.. just create an empty Flags object
//     //JMFIXME: REQUEST for initiateAcc is created in lsq_impl.hh@pushRequest or dma_device
//     Request request = Request(req_addr, 1, req_flags, req_mid);
//     //Create Packet
//     PacketPtr _packet = Packet::createWrite(request);
//     //Send Packet
// }


#endif // __CPU_O3_SE_INTERFACE_IMPL_HH__