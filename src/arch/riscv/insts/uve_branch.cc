
#include "arch/riscv/insts/uve_branch.hh"

#include <sstream>
#include <string>

#include "arch/riscv/utility.hh"

using namespace std;

namespace RiscvISA {

    std::string UveBaseStreamBranch::generateDisassembly(
        Addr pc, const SymbolTable *symtab) const {
        stringstream ss;
        ss << mnemonic << " " << registerName(_srcRegIdx[0]) << ", "
           << std::hex << (int64_t)(pc + imm);
        return ss.str();
    }
}  // namespace RiscvISA