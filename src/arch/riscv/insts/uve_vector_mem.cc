
#include "arch/riscv/insts/uve_vector_mem.hh"

#include <sstream>
#include <string>

#include "arch/riscv/utility.hh"

using namespace std;

namespace RiscvISA
{

std::string
UveMemLoad::generateDisassembly(Addr pc, const SymbolTable *symtab) const
{
    stringstream ss;
    ss << mnemonic << getWidthRepr() << getSbitRepr() << " " <<
    registerName(_destRegIdx[0]) << ", " << registerName(_srcRegIdx[0]) << 
    ", " << registerName(_srcRegIdx[1]);
    return ss.str();
}

std::string
UveMemStore::generateDisassembly(Addr pc, const SymbolTable *symtab) const
{
    stringstream ss;
    ss << mnemonic << getWidthRepr() << getSbitRepr() << " " <<
    registerName(_destRegIdx[0]) << ", " << registerName(_srcRegIdx[0]) << 
    ", " << registerName(_srcRegIdx[1]);
    return ss.str();
}

}  // namespace RiscvISA
