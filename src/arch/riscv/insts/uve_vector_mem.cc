
#include "arch/riscv/insts/uve_vector_mem.hh"

namespace RiscvISA
{

std::string
UveMem::generateDisassembly(Addr pc, const SymbolTable *symtab) const
{
    stringstream ss;
    //JMTODO: Update with the s bit
    ss << mnemonic << '.' << getWidthRepr() << " " <<
    registerName(_destRegIdx[0]) << ", " << registerName(_srcRegIdx[0]) << 
    ", " << registerName(_srcRegIdx[1]);
    return ss.str();
}

}  // namespace RiscvISA
