# Copyright (c) 2012 The Regents of The University of Michigan
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met: redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer;
# redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution;
# neither the name of the copyright holders nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# Authors: ICS-FORTH, Polydoros Petrakis <ppetrak@ics.forth.gr>
# Authors: ICS-FORTH, Vassilis Papaefstathiou <papaef@ics.forth.gr>
# Based on previous model for ARM Cortex A72 provided by Adria Armejach (BSC)
# and information from the following links regarding Cortex-A76:
# https://en.wikichip.org/wiki/arm_holdings/microarchitectures/cortex-a76
# https://www.anandtech.com/show/12785/\
#    arm-cortex-a76-cpu-unveiled-7nm-powerhouse/2
# https://www.anandtech.com/show/12785/\
#        arm-cortex-a76-cpu-unveiled-7nm-powerhouse/3

from __future__ import print_function
from __future__ import absolute_import

from m5.objects import *
import m5
m5.util.addToPath('../../')
from common.Caches import *

#from common import CpuConfig
#from common import MemConfig

# Simple ALU Instructions have a latency of 1
class ARM_Cortex_A76_Simple_Int(FUDesc):
    opList = [ OpDesc(opClass='IntAlu', opLat=1) ]
    count = 3

# Complex ALU instructions have a variable latencies
class ARM_Cortex_A76_Complex_Int(FUDesc):
    opList = [ OpDesc(opClass='IntMult', opLat=3, pipelined=True),
               OpDesc(opClass='IntDiv', opLat=12, pipelined=False),
               OpDesc(opClass='IprAccess', opLat=3, pipelined=True) ]
    count = 2

# Floating point and SIMD instructions
class ARM_Cortex_A76_FP(FUDesc):
    opList = [ OpDesc(opClass='SimdAdd', opLat=4),
               OpDesc(opClass='SimdAddAcc', opLat=4),
               OpDesc(opClass='SimdAlu', opLat=4),
               OpDesc(opClass='SimdCmp', opLat=4),
               OpDesc(opClass='SimdCvt', opLat=3),
               OpDesc(opClass='SimdMisc', opLat=3),
               OpDesc(opClass='SimdMult',opLat=5),
               OpDesc(opClass='SimdMultAcc',opLat=5),
               OpDesc(opClass='SimdShift',opLat=3),
               OpDesc(opClass='SimdShiftAcc', opLat=3),
               OpDesc(opClass='SimdDiv', opLat=9, pipelined=False),
               OpDesc(opClass='SimdSqrt', opLat=9),
               OpDesc(opClass='SimdFloatAdd',opLat=5),
               OpDesc(opClass='SimdFloatAlu',opLat=5),
               OpDesc(opClass='SimdFloatCmp', opLat=3),
               OpDesc(opClass='SimdFloatCvt', opLat=3),
               OpDesc(opClass='SimdFloatDiv', opLat=3),
               OpDesc(opClass='SimdFloatMisc', opLat=3),
               OpDesc(opClass='SimdFloatMult', opLat=3),
               OpDesc(opClass='SimdFloatMultAcc',opLat=5),
               OpDesc(opClass='SimdFloatSqrt', opLat=9),
               OpDesc(opClass='SimdReduceAdd'),
               OpDesc(opClass='SimdReduceAlu'),
               OpDesc(opClass='SimdReduceCmp'),
               OpDesc(opClass='SimdFloatReduceAdd'),
               OpDesc(opClass='SimdFloatReduceCmp'),
               OpDesc(opClass='FloatAdd', opLat=5),
               OpDesc(opClass='FloatCmp', opLat=5),
               OpDesc(opClass='FloatCvt', opLat=5),
               OpDesc(opClass='FloatDiv', opLat=9, pipelined=False),
               OpDesc(opClass='FloatSqrt', opLat=33, pipelined=False),
               OpDesc(opClass='FloatMult', opLat=4),
               OpDesc(opClass='FloatMultAcc', opLat=5),
               OpDesc(opClass='FloatMisc', opLat=3) ]
    count = 2

# Load/Store Units
class ARM_Cortex_A76_Load(FUDesc):
    opList = [ OpDesc(opClass='MemRead'),
               OpDesc(opClass='FloatMemRead') ]
    count = 2

class ARM_Cortex_A76_Store(FUDesc):
    opList = [ OpDesc(opClass='MemWrite'),
               OpDesc(opClass='FloatMemWrite') ]
    count = 1

class ARM_Cortex_A76_PredALU(FUDesc):
    opList = [ OpDesc(opClass='SimdPredAlu') ]
    count = 1

# Functional Units for this CPU
class ARM_Cortex_A76_FUP(FUPool):
    FUList = [ARM_Cortex_A76_Simple_Int(),
              ARM_Cortex_A76_Complex_Int(),
              ARM_Cortex_A76_Load(),
              ARM_Cortex_A76_Store(),
              ARM_Cortex_A76_PredALU(),
              ARM_Cortex_A76_FP()]

# Bi-Mode Branch Predictor
class ARM_Cortex_A76_BP(BiModeBP):
    globalPredictorSize = 8192
    globalCtrBits = 2
    choicePredictorSize = 8192
    choiceCtrBits = 2
    BTBEntries = 4096
    BTBTagSize = 16
    RASSize = 16
    instShiftAmt = 2

class ARM_Cortex_A76(DerivO3CPU):
    LSQDepCheckShift = 0
    LFSTSize = 1024
    SSITSize = 1024
    decodeToFetchDelay = 1
    renameToFetchDelay = 1
    iewToFetchDelay = 1
    commitToFetchDelay = 1
    renameToDecodeDelay = 1
    iewToDecodeDelay = 1
    commitToDecodeDelay = 1
    iewToRenameDelay = 1
    commitToRenameDelay = 1
    commitToIEWDelay = 1
    fetchWidth = 4
    fetchBufferSize = 16
    fetchToDecodeDelay = 1
    decodeWidth = 4
    decodeToRenameDelay = 1
    renameWidth = 4
    renameToIEWDelay = 1
    issueToExecuteDelay = 1
    dispatchWidth = 8
    issueWidth = 8
    wbWidth = 8
    fuPool = ARM_Cortex_A76_FUP()
    iewToCommitDelay = 1
    renameToROBDelay = 1
    commitWidth = 8
    squashWidth = 8
    trapLatency = 13
    backComSize = 5
    forwardComSize = 5
    numROBEntries = 192
    # the default value for numPhysVecPredRegs  is 32 in O3 config
    numPhysVecPredRegs = 64
    LQEntries = 68
    SQEntries = 72
    numIQEntries = 120

    switched_out = False
    #branchPred = ARM_Cortex_A76_BP()
    branchPred = Param.BranchPredictor(TournamentBP(
        numThreads = Parent.numThreads), "Branch Predictor")

# The following lines were copied from file devices.py
# provided by BSC (and adjusted by FORTH to match A76)

class A76_L1I(L1_ICache):
    tag_latency = 1
    data_latency = 1
    response_latency = 1
    mshrs = 8
    tgts_per_mshr = 8
    size = '64kB'
    assoc = 4

class A76_L1D(L1_DCache):
    tag_latency = 2
    data_latency = 2
    response_latency = 1
    mshrs = 24
    tgts_per_mshr = 16
    size = '64kB'
    assoc = 4
    write_buffers = 24

class A76_WalkCache(PageTableWalkerCache):
    tag_latency = 4
    data_latency = 4
    response_latency = 4
    mshrs = 6
    tgts_per_mshr = 8
    size = '1kB'
    assoc = 8
    write_buffers = 16

class A76_L2(L2Cache):
    tag_latency = 9
    data_latency = 9
    response_latency = 5
    mshrs = 24
    tgts_per_mshr = 16
    write_buffers = 24
    size = '256kB'
    assoc = 8
    clusivity='mostly_incl' #if not LLC
    # TODO: This was turned off in the BSC version. need to check again!
    # Prefetch also depends on Ruby + CC protocol
    # For now I think it does not work
    prefetch_on_access = True
    # Simple next line prefetcher
    # prefetcher = StridePrefetcher(degree=8, latency = 1)
    # prefetcher = TaggedPrefetcher(degree=8, latency = 1, queue_size = 64)
    {% if PREFETCHER == "NONE" %}
    prefetcher = NULL
    {% elif PREFETCHER == "STREAM" %}
    prefetcher = StreamPrefetcher(degree=8, latency = 1)
    {% elif PREFETCHER == "STRIDE" %}
    prefetcher = StridePrefetcher(degree=8, latency = 1)
    {% else %}
    prefetcher = AMPMPrefetcher()
    {% endif %}

    #tags = LRU() ## ppetrak: this needs extra imports to work.

# class A76_WalkCache(PageTableWalkerCache):
#     tag_latency = 4
#     data_latency = 4
#     response_latency = 4
#     mshrs = 6
#     tgts_per_mshr = 8
#     size = '4kB'
#     assoc = 8
#     write_buffers = 160
#     assoc = 2
#     tag_latency = 2
#     data_latency = 2

# class L2LLC(L2Cache):
#     tag_latency = 9
#     data_latency = 9
#     response_latency = 5
#     mshrs = 32
#     tgts_per_mshr = 8
#     write_buffers = 8
#     size = '8MB'
#     assoc = 16
#     clusivity='mostly_incl' #if LLC
#     # prefetch_on_access = True
#     # Simple next line prefetcher
#     # prefetcher = StridePrefetcher(degree=8, latency = 1)
#     prefetcher = TaggedPrefetcher(degree=8, latency = 1, queue_size = 64)
#     tags = LRU()

# class A76_L3(Cache):
#     size = '16MB'
#     assoc = 16
#     tag_latency = 20
#     data_latency = 20
#     response_latency = 20
#     mshrs = 128
#     tgts_per_mshr = 12
#     write_buffers = 64
#     clusivity='mostly_excl'

# class L3Slice(Cache):
#     size = '1MB'
#     assoc = 16
#     tag_latency = 20
#     data_latency = 20
#     response_latency = 20
#     mshrs = 32
#     tgts_per_mshr = 12
#     write_buffers = 32
#     clusivity='mostly_excl'
