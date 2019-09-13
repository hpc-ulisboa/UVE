
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

    UveBaseStream(const char *mnem, ExtMachInst _machInst, OpClass __opClass,
                  uint8_t _VDest, uint8_t _VSrc1, uint8_t _VSrc2,
                  uint8_t _VSrc3)
        : RiscvStaticInst(mnem, _machInst, __opClass),
          VDest(_VDest),
          VSrc1(_VSrc1),
          VSrc2(_VSrc2),
          VSrc3(_VSrc3) {
      DPRINTF(dUVEStream,
              "UveBaseStream constructor (%s) executed: VDest(%d), VSrc1(%d)"
              "VSrc2(%d) VSrc3(%d)\n",
              mnem, VDest, VSrc1, VSrc2, VSrc3);
    }

    std::string generateDisassembly(Addr pc, const SymbolTable *symtab) const;
    uint8_t getStreamRegister() const override { return VDest; }
  };
}  // namespace RiscvISA

#endif  // __ARCH_RISCV_UVE_STREAM_HH__
