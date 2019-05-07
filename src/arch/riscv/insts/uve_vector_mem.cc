
#include "arch/riscv/insts/uve_vector_mem.hh"

#include <sstream>
#include <string>

#include "arch/riscv/insts/bitfields.hh"
#include "arch/riscv/insts/static_inst.hh"
#include "arch/riscv/utility.hh"
#include "cpu/static_inst.hh"

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
