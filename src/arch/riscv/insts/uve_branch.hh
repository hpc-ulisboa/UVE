
#ifndef __ARCH_RISCV_UVE_BRANCH_HH__
#define __ARCH_RISCV_UVE_BRANCH_HH__

#include "arch/riscv/insts/static_inst.hh"
#include "debug/UVEBranch.hh"

namespace RiscvISA {

    class UveBaseStreamBranch : public RiscvStaticInst {
       protected:
        uint8_t VSrc1;
        uint64_t imm;

        Request::Flags memAccessFlags;

       public:
        UveBaseStreamBranch(const char *mnem, ExtMachInst _machInst,
                            OpClass __opClass, uint8_t _VSrc1, uint64_t _imm)
            : RiscvStaticInst(mnem, _machInst, __opClass),
              VSrc1(_VSrc1),
              imm(_imm) {}

        std::string generateDisassembly(Addr pc,
                                        const SymbolTable *symtab) const;
        uint8_t getUVEStream() const { return VSrc1; }

        RiscvISA::PCState branchTarget(
            const RiscvISA::PCState &branchPC) const override;

        using StaticInst::branchTarget;
    };
}  // namespace RiscvISA

#endif  // __ARCH_RISCV_UVE_BRANCH_HH__