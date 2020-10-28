
#include "arch/riscv/insts/uve_stream.hh"

#include <sstream>
#include <string>

#include "arch/riscv/utility.hh"

using namespace std;

namespace RiscvISA
{

std::string
UveBaseStream::generateDisassembly(Addr pc,
    const SymbolTable *symtab)
const
{
    stringstream ss;
    ss << mnemonic << "\t"
       << "u" << (uint32_t)this->getStreamRegister() << ", "
       << registerName(_srcRegIdx[0]) << ", " << registerName(_srcRegIdx[1])
       << ", " << registerName(_srcRegIdx[2]);
    return ss.str();
}
std::string
UveBaseModifierStream::generateDisassembly(Addr pc,
    const SymbolTable *symtab)
const
{
    stringstream ss;
    ss << mnemonic << "\t"
       << "u" << (uint32_t)this->getStreamRegister() << ", "
       << registerName(_srcRegIdx[0]) << registerName(_srcRegIdx[1]);
    return ss.str();
}

}  // namespace RiscvISA
