/*
 * Copyright (c) 2016 RISC-V Foundation
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
 * Authors: Alec Roelke
 */
#include "arch/riscv/isa.hh"

#include <ctime>
#include <set>
#include <sstream>

#include "arch/riscv/registers.hh"
#include "base/bitfield.hh"
#include "cpu/base.hh"
#include "debug/RiscvMisc.hh"
#include "params/RiscvISA.hh"
#include "sim/core.hh"
#include "sim/pseudo_inst.hh"

namespace RiscvISA
{

ISA::ISA(Params *p) : SimObject(p)
{
    miscRegFile.resize(NumMiscRegs);

    //JMNOTE:uveVl Added system cast to grab the value of uveVl
    system = dynamic_cast<RiscvSystem *>(p->system);

    if (system) {
        uveVl = system->uveVl();
    } else {
        uveVl = p->uve_vl_se;
    }

    clear();
}

const RiscvISAParams *
ISA::params() const
{
    return dynamic_cast<const Params *>(_params);
}

void ISA::clear()
{
    std::fill(miscRegFile.begin(), miscRegFile.end(), 0);

    miscRegFile[MISCREG_PRV] = PRV_M;
    miscRegFile[MISCREG_ISA] = (2ULL << MXL_OFFSET) | 0x14112D;
    miscRegFile[MISCREG_VENDORID] = 0;
    miscRegFile[MISCREG_ARCHID] = 0;
    miscRegFile[MISCREG_IMPID] = 0;
    miscRegFile[MISCREG_STATUS] = (2ULL << UXL_OFFSET) | (2ULL << SXL_OFFSET) |
                                  (1ULL << FS_OFFSET);
    miscRegFile[MISCREG_MCOUNTEREN] = 0x7;
    miscRegFile[MISCREG_SCOUNTEREN] = 0x7;

    //JMNOTE:uveVl Added UVE CSR to here. It's value comes
    // from the parameters of the simulation
    miscRegFile[MISCREG_UVEVS] = uveVl;
    miscRegFile[MISCREG_UVEVT] = 0;
    miscRegFile[MISCREG_UVEVI0] = 0;
    miscRegFile[MISCREG_UVEVI1] = 0;
    miscRegFile[MISCREG_UVEVI2] = 0;
    miscRegFile[MISCREG_UVEVI3] = 0;
    miscRegFile[MISCREG_UVEVI4] = 0;
    miscRegFile[MISCREG_UVEVI5] = 0;
    miscRegFile[MISCREG_UVEVI6] = 0;
    miscRegFile[MISCREG_UVEVI7] = 0;

}

bool
ISA::hpmCounterEnabled(int misc_reg) const
{
    int hpmcounter = misc_reg - MISCREG_CYCLE;
    if (hpmcounter < 0 || hpmcounter > 31)
        panic("Illegal HPM counter %d\n", hpmcounter);
    int counteren;
    switch (readMiscRegNoEffect(MISCREG_PRV)) {
      case PRV_M:
        return true;
      case PRV_S:
        counteren = MISCREG_MCOUNTEREN;
        break;
      case PRV_U:
        counteren = MISCREG_SCOUNTEREN;
        break;
      default:
        panic("Unknown privilege level %d\n", miscRegFile[MISCREG_PRV]);
        return false;
    }
    return (miscRegFile[counteren] & (1ULL << (hpmcounter))) > 0;
}

RegVal
ISA::readMiscRegNoEffect(int misc_reg) const
{
    if (misc_reg > NumMiscRegs || misc_reg < 0) {
        // Illegal CSR
        panic("Illegal CSR index %#x\n", misc_reg);
        return -1;
    }
    DPRINTF(RiscvMisc, "Reading MiscReg %d: %#llx.\n", misc_reg,
            miscRegFile[misc_reg]);
    return miscRegFile[misc_reg];
}

RegVal
ISA::readMiscReg(int misc_reg, ThreadContext *tc)
{
    switch (misc_reg) {
      case MISCREG_HARTID:
        return tc->contextId();
      case MISCREG_CYCLE:
        if (hpmCounterEnabled(MISCREG_CYCLE)) {
            DPRINTF(RiscvMisc, "Cycle counter at: %llu.\n",
                    tc->getCpuPtr()->curCycle());
            return tc->getCpuPtr()->curCycle();
        } else {
            warn("Cycle counter disabled.\n");
            return 0;
        }
      case MISCREG_TIME:
        if (hpmCounterEnabled(MISCREG_TIME)) {
            DPRINTF(RiscvMisc, "Wall-clock counter at: %llu.\n",
                    std::time(nullptr));
            return std::time(nullptr);
        } else {
            warn("Wall clock disabled.\n");
            return 0;
        }
      case MISCREG_INSTRET:
        if (hpmCounterEnabled(MISCREG_INSTRET)) {
            DPRINTF(RiscvMisc, "Instruction counter at: %llu.\n",
                    tc->getCpuPtr()->totalInsts());
            return tc->getCpuPtr()->totalInsts();
        } else {
            warn("Instruction counter disabled.\n");
            return 0;
        }
      case MISCREG_IP:
        return tc->getCpuPtr()->getInterruptController(tc->threadId())
                              ->readIP();
      case MISCREG_IE:
        return tc->getCpuPtr()->getInterruptController(tc->threadId())
                              ->readIE();
      default:
        // Try reading HPM counters
        // As a placeholder, all HPM counters are just cycle counters
        if (misc_reg >= MISCREG_HPMCOUNTER03 &&
                misc_reg <= MISCREG_HPMCOUNTER31) {
            if (hpmCounterEnabled(misc_reg)) {
                DPRINTF(RiscvMisc, "HPM counter %d: %llu.\n",
                        misc_reg - MISCREG_CYCLE, tc->getCpuPtr()->curCycle());
                return tc->getCpuPtr()->curCycle();
            } else {
                warn("HPM counter %d disabled.\n", misc_reg - MISCREG_CYCLE);
                return 0;
            }
        }
        return readMiscRegNoEffect(misc_reg);
    }
}

void
ISA::setMiscRegNoEffect(int misc_reg, RegVal val)
{
    if (misc_reg > NumMiscRegs || misc_reg < 0) {
        // Illegal CSR
        panic("Illegal CSR index %#x\n", misc_reg);
    }
    DPRINTF(RiscvMisc, "Setting MiscReg %d to %#x.\n", misc_reg, val);
    miscRegFile[misc_reg] = val;
}

void
ISA::setMiscReg(int misc_reg, RegVal val, ThreadContext *tc)
{
    if (misc_reg >= MISCREG_CYCLE && misc_reg <= MISCREG_HPMCOUNTER31) {
        // Ignore writes to HPM counters for now
        warn("Ignoring write to %s.\n", CSRData.at(misc_reg).name);
    } else {
        switch (misc_reg) {
          case MISCREG_IP:
            return tc->getCpuPtr()->getInterruptController(tc->threadId())
                                  ->setIP(val);
          case MISCREG_IE:
            return tc->getCpuPtr()->getInterruptController(tc->threadId())
                                  ->setIE(val);
          default:
            setMiscRegNoEffect(misc_reg, val);
        }
    }
}


//JMNOTE: Set UVE Vector Type
void
ISA::setUveVecType(ThreadContext *tc, uint8_t vector_register_id,
    uint8_t width)
{
    auto vector_type = tc->readMiscReg(MISCREG_UVEVT);
    uint8_t shift_amt = vector_register_id * 2;

    vector_type &= ~(0b11 << shift_amt);
    vector_type |= width << shift_amt;
    DPRINTF(UVEMem, "Set Uve Type: V(%d) Requested(%d) Final(%#x)\n",
        vector_register_id, width,vector_type);

    tc->setMiscReg(MISCREG_UVEVT,vector_type);
}

//JMNOTE: Set UVE Vector Type
// template <class WidthSize>
void
ISA::setUvePVecType(ThreadContext *tc, uint8_t vector_register_id,
    uint8_t width)
{
    auto vector_type = tc->readMiscReg(MISCREG_UVEPVT);
    uint8_t shift_amt = vector_register_id * 2;

    vector_type &= ~(0b11 << shift_amt);
    vector_type |= width << shift_amt;
    DPRINTF(UVEMem, "Set Uve PredicateType: P(%d) Requested(%d) Final(%#x)\n",
        vector_register_id, width,vector_type);

    tc->setMiscReg(MISCREG_UVEPVT,vector_type);
}

uint8_t
ISA::getUveVecType(ThreadContext *tc, uint8_t vector_register_id)
{
    auto vector_type = tc->readMiscReg(MISCREG_UVEVT);
    uint8_t shift_amt = vector_register_id * 2;

    vector_type &= 0b11 << shift_amt;

    return vector_type >> shift_amt;
}

uint8_t
ISA::getUvePVecType(ThreadContext *tc, uint8_t vector_register_id)
{
    auto vector_type = tc->readMiscReg(MISCREG_UVEPVT);
    uint8_t shift_amt = vector_register_id * 2;

    vector_type &= 0b11 << shift_amt;

    return vector_type >> shift_amt;

//JMNOTE: Set UVE Vector Valid Index
void
ISA::setUveValidIndex(ThreadContext *tc, uint8_t vector_register_id,
    uint16_t valid_index)
{
    //4 is number of vec reg per misc_reg
    uint8_t misc_reg_index = vector_register_id / 4;
    uint8_t shift_amt = (vector_register_id % 4) * 16;

    auto vector_index_info = tc->readMiscReg(MISCREG_UVEVI0 + misc_reg_index);
    vector_index_info &= ~(0xFFFF << shift_amt);
    vector_index_info |= ((uint64_t)valid_index) << shift_amt;
    DPRINTF(UVEMem, "Set Uve Valid: V(%d) Requested(%dby) Final(%#x) in"
        " Reg(%d)\n",vector_register_id, valid_index,vector_index_info,
        misc_reg_index);

    tc->setMiscReg(MISCREG_UVEVI0 + misc_reg_index, vector_index_info);
}

uint16_t
ISA::getUveValidIndex(ThreadContext *tc, uint8_t vector_register_id)
{
    //4 is number of vec reg per misc_reg
    uint8_t misc_reg_index = vector_register_id / 4;
    uint8_t shift_amt = (vector_register_id % 4) * 16;

    auto vector_index_info = tc->readMiscReg(MISCREG_UVEVI0 + misc_reg_index);
    vector_index_info &= (0xFFFFLL << shift_amt);
    DPRINTF(UVEMem, "Get Uve Valid: V(%d) Got(%dby) in Reg(%d)\n",
        vector_register_id,(((uint64_t)vector_index_info) >> shift_amt),
            misc_reg_index);
    return (((uint64_t)vector_index_info) >> shift_amt);
}

// JMNOTE: Get UVE Vector Length in bytes
int
ISA::getCurUveVecLen() const
{
    uint64_t len = miscRegFile[MISCREG_UVEVS];
    return len >> 3;
}


} //JMNOTE: End namespace Riscv

RiscvISA::ISA *
RiscvISAParams::create()
{
    return new RiscvISA::ISA(this);
}
