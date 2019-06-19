
#ifndef __ARCH_RISCV_UVE_PREDICATE_HH__
#define __ARCH_RISCV_UVE_PREDICATE_HH__

#include "arch/riscv/insts/static_inst.hh"
#include "debug/UVEPredic.hh"

namespace RiscvISA
{

class UveBasePredTwoVecPredicated : public RiscvStaticInst
{
  protected:
    uint8_t PDest;
    uint8_t VSrc1;
    uint8_t VSrc2;
    uint8_t PSrc3;

    Request::Flags memAccessFlags;

    UveBasePredTwoVecPredicated(const char *mnem, ExtMachInst _machInst,
      OpClass __opClass, uint8_t _PDest, uint8_t _VSrc1, uint8_t _VSrc2,
      uint8_t _PSrc3) :
          RiscvStaticInst(mnem, _machInst, __opClass),
          PDest(_PDest), VSrc1(_VSrc1), VSrc2(_VSrc2), PSrc3(_PSrc3)
    {
    DPRINTF(UVEMem,
    "UveBasePredTwoVecPredicated constructor (%s) executed: PDest(%d),"
    "VSrc1(%d) VSrc2(%d) PSrc3(%d)\n",
    mnem, PDest, VSrc1, VSrc2, PSrc3);
    }

    std::string generateDisassembly(Addr pc, const SymbolTable *symtab) const;
};

class UveBasePredVecScaPredicated : public RiscvStaticInst
{
  protected:
    uint8_t PDest;
    uint8_t VSrc1;
    uint8_t RS2;
    uint8_t PSrc3;

    Request::Flags memAccessFlags;

    UveBasePredVecScaPredicated(const char *mnem, ExtMachInst _machInst,
      OpClass __opClass, uint8_t _PDest, uint8_t _VSrc1, uint8_t _RS2,
      uint8_t _PSrc3) :
          RiscvStaticInst(mnem, _machInst, __opClass),
          PDest(_PDest), VSrc1(_VSrc1), RS2(_RS2), PSrc3(_PSrc3)
    {
    DPRINTF(UVEMem,
    "UveBasePredVecScaPredicated constructor (%s) executed: PDest(%d),"
    "VSrc1(%d) RS2(%d) PSrc3(%d)\n",
    mnem, PDest, VSrc1, RS2, PSrc3);
    }

    std::string generateDisassembly(Addr pc, const SymbolTable *symtab) const;
};

class UveBasePredUnaryPredicated : public RiscvStaticInst
{
  protected:
    uint8_t PDest;
    uint8_t PSrc3;

    Request::Flags memAccessFlags;

    UveBasePredUnaryPredicated(const char *mnem, ExtMachInst _machInst,
      OpClass __opClass, uint8_t _PDest, uint8_t _PSrc3) :
          RiscvStaticInst(mnem, _machInst, __opClass),
          PDest(_PDest), PSrc3(_PSrc3)
    {
    DPRINTF(UVEMem,
    "UveBasePredUnaryPredicated constructor (%s) executed: PDest(%d),"
    "PSrc3(%d)\n",
    mnem, PDest, PSrc3);
    }

    std::string generateDisassembly(Addr pc, const SymbolTable *symtab) const;
};

class UveBasePredOneVecPredicated : public RiscvStaticInst
{
  protected:
    uint8_t PDest;
    uint8_t VSrc1;
    uint8_t PSrc3;

    Request::Flags memAccessFlags;

    UveBasePredOneVecPredicated(const char *mnem, ExtMachInst _machInst,
      OpClass __opClass, uint8_t _PDest, uint8_t _VSrc1, uint8_t _PSrc3) :
          RiscvStaticInst(mnem, _machInst, __opClass),
          PDest(_PDest), VSrc1(_VSrc1), PSrc3(_PSrc3)
    {
    DPRINTF(UVEMem,
    "UveBasePredOneVecPredicated constructor (%s) executed: PDest(%d),"
    "VSrc1(%d) PSrc3(%d)\n",
    mnem, PDest, VSrc1, PSrc3);
    }

    std::string generateDisassembly(Addr pc, const SymbolTable *symtab) const;
};

class UveBasePredOnePVecPredicated : public RiscvStaticInst
{
  protected:
    uint8_t PDest;
    uint8_t PSrc1;
    uint8_t PSrc3;

    Request::Flags memAccessFlags;

    UveBasePredOnePVecPredicated(const char *mnem, ExtMachInst _machInst,
      OpClass __opClass, uint8_t _PDest, uint8_t _PSrc1, uint8_t _PSrc3) :
          RiscvStaticInst(mnem, _machInst, __opClass),
          PDest(_PDest), PSrc1(_PSrc1), PSrc3(_PSrc3)
    {
    DPRINTF(UVEMem,
    "UveBasePredOnePVecPredicated constructor (%s) executed: PDest(%d),"
    "PSrc1(%d) PSrc3(%d)\n",
    mnem, PDest, PSrc1, PSrc3);
    }

    std::string generateDisassembly(Addr pc, const SymbolTable *symtab) const;
};

class UveBasePredOnePVec : public RiscvStaticInst
{
  protected:
    uint8_t PDest;
    uint8_t PSrc1;

    Request::Flags memAccessFlags;

    UveBasePredOnePVec(const char *mnem, ExtMachInst _machInst,
      OpClass __opClass, uint8_t _PDest, uint8_t _PSrc1) :
          RiscvStaticInst(mnem, _machInst, __opClass),
          PDest(_PDest), PSrc1(_PSrc1)
    {
    DPRINTF(UVEMem,
    "UveBasePredOnePVec constructor (%s) executed: PDest(%d),"
    "PSrc1(%d)\n",
    mnem, PDest, PSrc1);
    }

    std::string generateDisassembly(Addr pc, const SymbolTable *symtab) const;
};




}  // namespace RiscvISA

#endif  // __ARCH_RISCV_UVE_PREDICATE_HH__
