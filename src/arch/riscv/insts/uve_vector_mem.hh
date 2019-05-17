
#ifndef __ARCH_RISCV_UVE_VECTOR_MEM_HH__
#define __ARCH_RISCV_UVE_VECTOR_MEM_HH__

#include "arch/riscv/insts/static_inst.hh"
#include "debug/UVEMem.hh"

namespace RiscvISA
{

class UveBaseMemLoad : public RiscvStaticInst
{
  protected:
    uint8_t VDest;
    uint8_t RS1;
    uint8_t RS2;

    Request::Flags memAccessFlags;

    UveBaseMemLoad(const char *mnem, ExtMachInst _machInst, OpClass __opClass,
                   uint8_t _VDest, uint8_t _RS1, uint8_t _RS2) :
          RiscvStaticInst(mnem, _machInst, __opClass),
          VDest(_VDest), RS1(_RS1), RS2(_RS2)
    {
    DPRINTF(UVEMem,
    "UveBaseMemLoad constructor (%s) executed: VDest(%d), RS1(%d), RS2(%d)\n",
    mnem, VDest, RS1, RS2);
    }

    std::string generateDisassembly(Addr pc, const SymbolTable *symtab) const;
};

class UveBaseMemStore : public RiscvStaticInst
{
  protected:
    uint8_t VSrc1;
    uint8_t RS1;
    uint8_t RS2;

    Request::Flags memAccessFlags;

    UveBaseMemStore(const char *mnem, ExtMachInst _machInst, OpClass __opClass,
                   uint8_t _VSrc1, uint8_t _RS1, uint8_t _RS2) :
          RiscvStaticInst(mnem, _machInst, __opClass),
          VSrc1(_VSrc1), RS1(_RS1), RS2(_RS2)
    {
      DPRINTF(UVEMem,
      "UveBaseMemStore constructor executed: VSrc1(%d), RS1(%d), RS2(%d)\n",
      VSrc1, RS1, RS2);
    }

    std::string generateDisassembly(Addr pc, const SymbolTable *symtab) const;
};


class UveBaseVectorDup : public RiscvStaticInst
{
  protected:
    uint8_t VDest;
    uint8_t RS1;
    uint8_t PS2;

    Request::Flags memAccessFlags;

    UveBaseVectorDup(const char *mnem, ExtMachInst _machInst,
      OpClass __opClass, uint8_t _VDest, uint8_t _RS1, uint8_t _PS2) :
          RiscvStaticInst(mnem, _machInst, __opClass),
          VDest(_VDest), RS1(_RS1), PS2(_PS2)
    {
    DPRINTF(UVEMem,
    "UveBaseMemDup constructor (%s) executed: VDest(%d), RS1(%d), PS2(%d)\n",
    mnem, VDest, RS1, PS2);
    }

    std::string generateDisassembly(Addr pc, const SymbolTable *symtab) const;
};



}  // namespace RiscvISA

#endif  // __ARCH_RISCV_UVE_VECTOR_MEM_HH__
