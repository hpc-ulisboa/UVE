
#include "arch/riscv/insts/uve_predicate.hh"

#include <sstream>
#include <string>

#include "arch/riscv/utility.hh"

using namespace std;

namespace RiscvISA
{

std::string
UveBasePredTwoVecPredicated::generateDisassembly(Addr pc,
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
UveBasePredVecScaPredicated::generateDisassembly(Addr pc,
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
UveBasePredUnaryPredicated::generateDisassembly(Addr pc,
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
UveBasePredOneVecPredicated::generateDisassembly(Addr pc,
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
UveBasePredOnePVecPredicated::generateDisassembly(Addr pc,
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
UveBasePredOnePVec::generateDisassembly(Addr pc,
    const SymbolTable *symtab)
const
{
    stringstream ss;
    ss << mnemonic << getFPSignedRepr() << "\t" <<
    registerName(_destRegIdx[0]) << ", " << registerName(_srcRegIdx[0]);
    return ss.str();
}

}  // namespace RiscvISA
