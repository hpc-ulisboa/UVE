
#ifndef __ARCH_RISCV_UVE_ARITHMETIC_HH__
#define __ARCH_RISCV_UVE_ARITHMETIC_HH__

#include "arch/riscv/insts/static_inst.hh"
#include "debug/UVEArith.hh"

namespace RiscvISA
{

class UveBaseThreeVecPredicated : public RiscvStaticInst
{
  protected:
    uint8_t VDest;
    uint8_t VSrc1;
    uint8_t VSrc2;
    uint8_t PSrc3;

    Request::Flags memAccessFlags;

    UveBaseThreeVecPredicated(const char *mnem, ExtMachInst _machInst,
      OpClass __opClass, uint8_t _VDest, uint8_t _VSrc1, uint8_t _VSrc2,
      uint8_t _PSrc3) :
          RiscvStaticInst(mnem, _machInst, __opClass),
          VDest(_VDest), VSrc1(_VSrc1), VSrc2(_VSrc2), PSrc3(_PSrc3)
    {
    DPRINTF(UVEMem,
    "UveBaseThreeVecPredicated constructor (%s) executed: VDest(%d), VSrc1(%d)"
    "VSrc2(%d) PSrc3(%d)\n",
    mnem, VDest, VSrc1, VSrc2, PSrc3);
    }

    std::string generateDisassembly(Addr pc, const SymbolTable *symtab) const;
};

class UveBaseTwoVecPredicated : public RiscvStaticInst
{
  protected:
    uint8_t VDest;
    uint8_t VSrc1;
    uint8_t VSrc2;
    uint8_t PSrc3;

    Request::Flags memAccessFlags;

    UveBaseTwoVecPredicated(const char *mnem, ExtMachInst _machInst,
        OpClass __opClass, uint8_t _VDest, uint8_t _VSrc1, uint8_t _PSrc3) :
          RiscvStaticInst(mnem, _machInst, __opClass),
          VDest(_VDest), VSrc1(_VSrc1), PSrc3(_PSrc3)
    {
    DPRINTF(UVEMem,
    "UveBaseTwoVecPredicated constructor (%s) executed: VDest(%d), VSrc1(%d)"
    "PSrc3(%d)\n",
    mnem, VDest, VSrc1, PSrc3);
    }

    std::string generateDisassembly(Addr pc, const SymbolTable *symtab) const;
};

class UveBaseTwoVec2ScaPredicated : public RiscvStaticInst
{
  protected:
    uint8_t RD;
    uint8_t VSrc1;
    uint8_t VSrc2;
    uint8_t PSrc3;

    Request::Flags memAccessFlags;

    UveBaseTwoVec2ScaPredicated(const char *mnem, ExtMachInst _machInst,
        OpClass __opClass, uint8_t _RD, uint8_t _VSrc1, uint8_t _PSrc3) :
          RiscvStaticInst(mnem, _machInst, __opClass),
          RD(_RD), VSrc1(_VSrc1), PSrc3(_PSrc3)
    {
    DPRINTF(UVEMem,
    "UveBaseTwoVecPredicated constructor (%s) executed: RD(%d), VSrc1(%d)"
    "PSrc3(%d)\n",
    mnem, RD, VSrc1, PSrc3);
    }

    std::string generateDisassembly(Addr pc, const SymbolTable *symtab) const;
};

class UveBaseVecScaPredicated : public RiscvStaticInst
{
  protected:
    uint8_t VDest;
    uint8_t VSrc1;
    uint8_t RS2;
    uint8_t PSrc3;

    Request::Flags memAccessFlags;

    UveBaseVecScaPredicated(const char *mnem, ExtMachInst _machInst,
      OpClass __opClass, uint8_t _VDest, uint8_t _VSrc1, uint8_t _RS2,
      uint8_t _PSrc3) :
          RiscvStaticInst(mnem, _machInst, __opClass),
          VDest(_VDest), VSrc1(_VSrc1), RS2(_RS2), PSrc3(_PSrc3)
    {
    DPRINTF(UVEMem,
    "UveBaseVecScaPredicated constructor (%s) executed: VDest(%d), VSrc1(%d)"
    "RS2(%d) PSrc3(%d)\n",
    mnem, VDest, VSrc1, RS2, PSrc3);
    }

    std::string generateDisassembly(Addr pc, const SymbolTable *symtab) const;
};

}  // namespace RiscvISA

#endif  // __ARCH_RISCV_UVE_ARITHMETIC_HH__
