
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
            {}

#endif //__CPU_O3_SE_INTERFACE_IMPL_HH__