
#ifndef __ARCH_RISCV_UVE_VECTOR_MEM_HH__
#define __ARCH_RISCV_UVE_VECTOR_MEM_HH__

#include "arch/riscv/insts/static_inst.hh"

namespace RiscvISA
{

class UveMem : public RiscvStaticInst
{
  protected:
    IntRegIndex dest;
    IntRegIndex base;
    IntRegIndex size;

    Request::Flags memAccessFlags;

    UveMem(const char *mnem, ExtMachInst _machInst, OpClass __opClass,
                   IntRegIndex _dest, IntRegIndex _gp, IntRegIndex _base,
                   IntRegIndex _size)
        : RiscvStaticInst(mnem, _machInst, __opClass),
          dest(_dest), base(_base), size(_size)
    {}

    std::string generateDisassembly(Addr pc, const SymbolTable *symtab) const;
};

}  // namespace RiscvISA

#endif  // __ARCH_RISCV_UVE_VECTOR_MEM_HH__
