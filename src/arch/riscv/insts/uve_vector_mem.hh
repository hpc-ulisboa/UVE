
#ifndef __ARCH_RISCV_UVE_VECTOR_MEM_HH__
#define __ARCH_RISCV_UVE_VECTOR_MEM_HH__

#include "arch/riscv/insts/static_inst.hh"
#include "debug/UVEMem"

namespace RiscvISA
{

class UveMemLoad : public RiscvStaticInst
{
  protected:
    uint8_t VDest;
    uint8_t RS1;
    uint8_t RS2;

    Request::Flags memAccessFlags;

    UveMemLoad(const char *mnem, ExtMachInst _machInst, OpClass __opClass,
                   uint8_t _VDest, uint8_t _RS1, uint8_t _RS2) : 
          RiscvStaticInst(mnem, _machInst, __opClass),
          VDest(_VDest), RS1(_RS1), RS2(_RS2)
    {DPRINTF(UVEMem, "UveMemLoad constructor executed: VDest(%d), RS1(%d), RS2(%d)",VDest, RS1, RS2)}

    std::string generateDisassembly(Addr pc, const SymbolTable *symtab) const;
};

class UveMemStore : public RiscvStaticInst
{
  protected:
    uint8_t VSrc1;
    uint8_t RS1;
    uint8_t RS2;

    Request::Flags memAccessFlags;

    UveMemStore(const char *mnem, ExtMachInst _machInst, OpClass __opClass,
                   uint8_t _VSrc1, uint8_t _RS1, uint8_t _RS2) :
          RiscvStaticInst(mnem, _machInst, __opClass),
          VSrc1(_VSrc1), RS1(_RS1), RS2(_RS2)
    {}

    std::string generateDisassembly(Addr pc, const SymbolTable *symtab) const;
};


}  // namespace RiscvISA

#endif  // __ARCH_RISCV_UVE_VECTOR_MEM_HH__
