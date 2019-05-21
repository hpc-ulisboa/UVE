/*
 * Copyright (c) 2015 RISC-V Foundation
 * Copyright (c) 2016 The University of Virginia
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Authors: Maxwell Walter
 *          Alec Roelke
 */

#include "arch/riscv/insts/static_inst.hh"

#include "arch/riscv/types.hh"
#include "cpu/static_inst.hh"

namespace RiscvISA
{

void
RiscvMicroInst::advancePC(PCState &pcState) const
{
    if (flags[IsLastMicroop]) {
        pcState.uEnd();
    } else {
        pcState.uAdvance();
    }
}

//JMNOTE: Functions for generating dissassembly
std::string
RiscvStaticInst::getWidthRepr() const {
    int8_t width = bits(machInst,13,12);
    std::string width_repr = "";
    switch(width){
        case 0:
            width_repr = ".b";
            break;
        case 1:
            width_repr = ".h";
            break;
        case 2:
            width_repr = ".w";
            break;
        case 3:
            width_repr = ".d";
            break;
    }
    return width_repr;
}

std::string
RiscvStaticInst::getUpdateRepr() const {
    int8_t u_bit = bits(machInst,14);
    std::string u_bit_repr = "";
    switch(u_bit){
        case 0:
            u_bit_repr = "";
            break;
        case 1:
            u_bit_repr = ".u";
            break;
    }
    return u_bit_repr;
}

std::string
RiscvStaticInst::getSbitRepr() const {
    int8_t s_bit = bits(machInst,25);
    std::string s_bit_repr = "";
    switch(s_bit){
        case 0:
            s_bit_repr = "";
            break;
        case 1:
            s_bit_repr = ".s";
            break;
    }
    return s_bit_repr;
}


} // namespace RiscvISA