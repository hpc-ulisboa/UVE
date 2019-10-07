
#include "arch/riscv/insts/uve_branch.hh"

#include <sstream>
#include <string>

#include "arch/riscv/utility.hh"

using namespace std;

namespace RiscvISA {

    std::string UveBaseStreamBranch::generateDisassembly(
        Addr pc, const SymbolTable *symtab) const {
        stringstream ss;
        ss << mnemonic << registerName(_destRegIdx[0]) << ", " << (pc + imm);
        return ss.str();
    }
}  // namespace RiscvISA