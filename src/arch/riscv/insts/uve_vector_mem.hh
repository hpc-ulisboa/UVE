
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
    uint8_t PSrc2;

    Request::Flags memAccessFlags;

    UveBaseVectorDup(const char *mnem, ExtMachInst _machInst,
      OpClass __opClass, uint8_t _VDest, uint8_t _RS1, uint8_t _PSrc2) :
          RiscvStaticInst(mnem, _machInst, __opClass),
          VDest(_VDest), RS1(_RS1), PSrc2(_PSrc2)
    {
    DPRINTF(UVEMem,
    "UveBaseVectorDup constructor (%s) executed: VDest(%d),"
    " RS1(%d), PSrc2(%d)\n",
    mnem, VDest, RS1, PSrc2);
    }

    std::string generateDisassembly(Addr pc, const SymbolTable *symtab) const;
};


class UveBaseVectorMove : public RiscvStaticInst
{
  protected:
    uint8_t VDest;
    uint8_t VSrc1;
    uint8_t PSrc2;

    Request::Flags memAccessFlags;

    UveBaseVectorMove(const char *mnem, ExtMachInst _machInst,
      OpClass __opClass, uint8_t _VDest, uint8_t _VSrc1, uint8_t _PSrc2) :
          RiscvStaticInst(mnem, _machInst, __opClass),
          VDest(_VDest), VSrc1(_VSrc1), PSrc2(_PSrc2)
    {
    DPRINTF(UVEMem,
    "UveBaseVectorMove constructor (%s) executed: VDest(%d),"
    " VSrc1(%d), PSrc2(%d)\n",
    mnem, VDest, VSrc1, PSrc2);
    }

    std::string generateDisassembly(Addr pc, const SymbolTable *symtab) const;
};


class UveBaseVectorMoveVS : public RiscvStaticInst
{
  protected:
    uint8_t RD;
    uint8_t VSrc1;
    uint8_t PSrc2;

    Request::Flags memAccessFlags;

    UveBaseVectorMoveVS(const char *mnem, ExtMachInst _machInst,
      OpClass __opClass, uint8_t _RD, uint8_t _VSrc1, uint8_t _PSrc2) :
          RiscvStaticInst(mnem, _machInst, __opClass),
          RD(_RD), VSrc1(_VSrc1), PSrc2(_PSrc2)
    {
    DPRINTF(UVEMem,
    "UveBaseVectorMoveVS constructor (%s) executed: RD(%d),"
    " VSrc1(%d), PSrc2(%d)\n",
    mnem, RD, VSrc1, PSrc2);
    }

    std::string generateDisassembly(Addr pc, const SymbolTable *symtab) const;
};

class UveBaseVectorConv : public RiscvStaticInst
{
  protected:
    uint8_t VDest;
    uint8_t VSrc1;

    Request::Flags memAccessFlags;

    UveBaseVectorConv(const char *mnem, ExtMachInst _machInst,
      OpClass __opClass, uint8_t _VDest, uint8_t _VSrc1) :
          RiscvStaticInst(mnem, _machInst, __opClass),
          VDest(_VDest), VSrc1(_VSrc1)
    {
    DPRINTF(UVEMem,
    "UveBaseVectorConv constructor (%s) executed: VDest(%d),"
    " VSrc1(%d)\n",
    mnem, VDest, VSrc1);
    }

    std::string generateDisassembly(Addr pc, const SymbolTable *symtab) const;
};


}  // namespace RiscvISA

#endif  // __ARCH_RISCV_UVE_VECTOR_MEM_HH__
