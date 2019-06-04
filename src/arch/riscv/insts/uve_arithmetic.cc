
#include "arch/riscv/insts/uve_arithmetic.hh"

#include <sstream>
#include <string>

#include "arch/riscv/utility.hh"

using namespace std;

namespace RiscvISA
{

std::string
UveBaseThreeVecPredicated::generateDisassembly(Addr pc,
    const SymbolTable *symtab)
const
{
    stringstream ss;
    ss << mnemonic << getFPSignedRepr() << "\t" <<
    registerName(_destRegIdx[0]) << ", " << registerName(_srcRegIdx[0]) <<
    ", " << registerName(_srcRegIdx[1]) << ", " << registerName(_srcRegIdx[2]);
    return ss.str();
}

std::string
UveBaseTwoVecPredicated::generateDisassembly(Addr pc,
    const SymbolTable *symtab)
const
{
    stringstream ss;
    ss << mnemonic << getFPSignedRepr() << "\t" <<
    registerName(_destRegIdx[0]) << ", " << registerName(_srcRegIdx[0]) <<
    ", " << registerName(_srcRegIdx[2]);
    return ss.str();
}

std::string
UveBaseVecScaPredicated::generateDisassembly(Addr pc,
    const SymbolTable *symtab)
const
{
    stringstream ss;
    ss << mnemonic << getFPSignedRepr() << "\t" <<
    registerName(_destRegIdx[0]) << ", " << registerName(_srcRegIdx[0]) <<
    ", " << registerName(_srcRegIdx[1]) << ", " << registerName(_srcRegIdx[2]);
    return ss.str();
}

}  // namespace RiscvISA
