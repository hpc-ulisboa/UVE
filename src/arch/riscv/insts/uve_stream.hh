
#ifndef __ARCH_RISCV_UVE_STREAM_HH__
#define __ARCH_RISCV_UVE_STREAM_HH__

#include "arch/riscv/insts/static_inst.hh"
#include "debug/dUVEStream.hh"

namespace RiscvISA {

  class UveBaseStream : public RiscvStaticInst {
   protected:
    uint8_t VDest;
    uint8_t VSrc1;
    uint8_t VSrc2;
    uint8_t VSrc3;
    uint8_t PSid;

    UveBaseStream(const char *mnem, ExtMachInst _machInst, OpClass __opClass,
                  uint8_t _VDest, uint8_t _VSrc1, uint8_t _VSrc2,
                  uint8_t _VSrc3)
        : RiscvStaticInst(mnem, _machInst, __opClass),
          VDest(_VDest),
          VSrc1(_VSrc1),
          VSrc2(_VSrc2),
          VSrc3(_VSrc3),
          PSid(0) {
        DPRINTF(dUVEStream,
                "UveBaseStream constructor (%s) executed: VDest(%d), VSrc1(%d)"
                "VSrc2(%d) VSrc3(%d)\n",
                mnem, VDest, VSrc1, VSrc2, VSrc3);
    }

    std::string generateDisassembly(Addr pc, const SymbolTable *symtab) const;
    uint8_t getStreamRegister() const override { return VDest; }
    uint8_t isStreamLoad() const override { return bits(machInst, 14) == 1; }
    uint8_t isEndAppendStream() const override {
        return bits(machInst, 26) == 0;
    }
    void setPhysStream(uint8_t sid) override { PSid = sid; }
  };

  class UveBaseModifierStream : public RiscvStaticInst {
   protected:
    uint8_t VDest;
    uint8_t RS1;
    uint8_t target;
    uint8_t behaviour;
    uint8_t RS3;
    uint8_t PSid;

    UveBaseModifierStream(const char *mnem, ExtMachInst _machInst,
                  OpClass __opClass, uint8_t _VDest, uint8_t _RS1,
                  uint8_t _target, uint8_t _behaviour, uint8_t _RS3)
        : RiscvStaticInst(mnem, _machInst, __opClass),
          VDest(_VDest),
          RS1(_RS1),
          target(_target),
          behaviour(_behaviour),
          RS3(_RS3),
          PSid(0) {
        DPRINTF(dUVEStream,
                "UveBaseModifierStream constructor (%s) executed: VDest(%d),"
                "RS1(%d), target(%d), behaviour(%d), RS3(%d)\n",
                mnem, VDest, RS1, target, behaviour, RS3);
    }

    std::string generateDisassembly(Addr pc, const SymbolTable *symtab) const;
    uint8_t getStreamRegister() const override { return VDest; }
    uint8_t isStreamLoad() const override { return bits(machInst, 14) == 1; }
    uint8_t isEndAppendStream() const override {
        return bits(machInst, 26) == 0;
    }
    void setPhysStream(uint8_t sid) override { PSid = sid; }
  };
}  // namespace RiscvISA

#endif  // __ARCH_RISCV_UVE_STREAM_HH__
