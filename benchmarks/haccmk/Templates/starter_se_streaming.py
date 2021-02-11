# Copyright (c) 2016-2017 ARM Limited
# All rights reserved.
#
# The license below extends only to copyright in the software and shall
# not be construed as granting a license to any other intellectual
# property including but not limited to intellectual property relating
# to a hardware implementation of the functionality of the software
# licensed hereunder.  You may use the software subject to the license
# terms below provided that you ensure that this notice is replicated
# unmodified and in its entirety in all distributions of the software,
# modified or unmodified, in source code or in binary form.
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
#  Authors:  Andreas Sandberg
#            Chuan Zhu
#            Gabor Dozsa
#

"""This script is the syscall emulation example script from the ARM
Research Starter Kit on System Modeling. More information can be found
at: http://www.arm.com/ResearchEnablement/SystemModeling
"""

from __future__ import print_function
from __future__ import absolute_import

import sys, os
import m5
from m5.util import addToPath
from m5.objects import *
import argparse
import shlex

m5.util.addToPath(os.environ['GEM5_RV_ROOT']+'/configs')
from common import MemConfig

m5.util.addToPath(os.path.dirname(sys.argv[0]))
from O3_ARM_v7a import *
from ARM_Cortex_A76 import *

UVE_VECTOR_LENGTH = 512
UVE_FIFO_DEPTH = {{ UVE_DEPTH or 8 }}
UVE_MAX_REQUEST_SIZE = UVE_VECTOR_LENGTH * UVE_FIFO_DEPTH
UVE_STREAM_RENAME = True
UVE_AGUTHROUGHPUT = {{ UVE_THROUGHPUT or 8 }}

# Pre-defined CPU configurations. Each tuple must be ordered as : (cpu_class,
# l1_icache_class, l1_dcache_class, walk_cache_class, l2_Cache_class). Any of
# the cache class may be 'None' if the particular cache is not present.
cpu_types = {
    "atomic" : ( AtomicSimpleCPU, None, None, None, None),
    "o3"  : ( O3_ARM_v7a_3,
              O3_ARM_v7a_ICache,
              O3_ARM_v7a_DCache,
              O3_ARM_v7aWalkCache,
              O3_ARM_v7aL2),
    "a76" : ( ARM_Cortex_A76,
             A76_L1I,
             A76_L1D,
             A76_WalkCache,
             A76_L2)
}

# CrossBar modeled with only one master and 2 slaves.
class L1SExBar(CoherentXBar):
    # 1024-bit crossbar by default
    width = 128

    # Assume that most of this is covered by the cache latencies, with
    # no more than a single pipeline stage for any packet.
    frontend_latency = 1
    forward_latency = 0
    response_latency = 1
    snoop_response_latency = 1
    max_routing_table_size=4096



class SimpleSeSystem(System):
    '''
    Example system class for syscall emulation mode
    '''

    # Use a fixed cache line size of 64 bytes
    cache_line_size = 64

    def __init__(self, args, **kwargs):
        super(SimpleSeSystem, self).__init__(**kwargs)
        print("CPU Model " + args.cpu)

        self._num_cpus = 0

        # Create a voltage and clock domain for system components
        self.voltage_domain = VoltageDomain(voltage="3.3V")
        self.clk_domain = SrcClockDomain(clock=args.cpu_freq,
                                         voltage_domain=self.voltage_domain)

        # Create the off-chip memory bus.
        self.membus = SystemXBar()

        # Wire up the system port that gem5 uses to load the kernel
        # and to perform debug accesses.
        self.system_port = self.membus.slave


        # Add CPUs to the system. A cluster of CPUs typically have
        # private L1 caches and a shared L2 cache.
        self.cpu = cpu_types[args.cpu][0](cpu_id=0,clk_domain=self.clk_domain)
        self.cpu.createThreads()
        self.cpu.createInterruptController()


        # Create a cache hierarchy (unless we are simulating a
        # functional CPU in atomic memory mode) for the CPU cluster
        # and connect it to the shared memory bus.
        self.cpu.icache = cpu_types[args.cpu][1](addr_ranges=["64GB"])
        self.cpu.dcache = cpu_types[args.cpu][2](addr_ranges=["64GB"])
        # self.cpu.dwalkercache = O3_ARM_v7aWalkCache()
        # self.cpu.iwalkercache = O3_ARM_v7aWalkCache()
        self.cpu.toL1SE = L1SExBar(clk_domain = self.clk_domain, )

        self.cpu.icache_port = self.cpu.icache.cpu_side
        self.cpu.dcache_port = self.cpu.toL1SE.slave
        self.cpu.toL1SE.master = self.cpu.dcache.cpu_side
        # self.cpu.dcache.cpu_side = self.cpu.toL1DBus.master
        #JMTODO: Check if page walking table is being set up properly

        self.toL2Bus = L2XBar(width=64, clk_domain = self.clk_domain, max_routing_table_size=4096)
        self.l2 = cpu_types[args.cpu][4]()
        self.cpu.icache.mem_side = self.toL2Bus.slave
        self.cpu.dcache.mem_side = self.toL2Bus.slave
        # self.cpu.itb.walker.port = self.toL2Bus.slave
        # self.cpu.dtb.walker.port = self.toL2Bus.slave
        # if self.checker != NULL:
        #     self.cpu.checker.itb.walker.port = self.toL2Bus.slave
        #     self.cpu.checker.dtb.walker.port = self.toL2Bus.slave
        self.toL2Bus.master = self.l2.cpu_side

        self.l2.mem_side = self.membus.slave

        #JMNOTE: CPU model changes
        print("UVE Model:\n\twidht:" + str(UVE_VECTOR_LENGTH) + "\n\tfifo_depth:"+
               str(UVE_FIFO_DEPTH)+"\n\trequest_size:"+str(UVE_MAX_REQUEST_SIZE)+
               "\n\tRename:"+str(UVE_STREAM_RENAME)) 
        self.cpu.streamEngine = UVEStreamingEngine(start_addr="64GB",
                                                   width=UVE_VECTOR_LENGTH,
                                                   fifo_depth=UVE_FIFO_DEPTH,
                                                   max_request_size=UVE_MAX_REQUEST_SIZE,
                                                   do_rename=UVE_STREAM_RENAME,
                                                   streams_throughput=UVE_AGUTHROUGHPUT)
        self.cpu.streamEngine[0].mem_side_store = self.cpu.toL1SE.slave
        self.cpu.streamEngine[0].mem_side_load_lv1 = self.cpu.toL1SE.slave
        self.cpu.streamEngine[0].mem_side_load_lv2 = self.toL2Bus.slave
        self.cpu.streamEngine[0].mem_side_load_lv3 = self.membus.slave

        # Tell gem5 about the memory mode used by the CPUs we are
        # simulating.
        self.mem_mode = O3_ARM_v7a_3.memory_mode()


    def numCpuClusters(self):
        return 1

    def addCpuCluster(self, cpu_cluster, num_cpus):
        pass

    def numCpus(self):
        return 1

def get_processes(cmd):
    """Interprets commands to run and returns a list of processes"""

    cwd = os.getcwd()
    multiprocesses = []
    for idx, c in enumerate(cmd):
        argv = shlex.split(c)

        process = Process(pid=100 + idx, cwd=cwd, cmd=argv, executable=argv[0])

        print("info: %d. command and arguments: %s" % (idx + 1, process.cmd))
        multiprocesses.append(process)

    return multiprocesses


def create(args):
    ''' Create and configure the system object. '''

    system = SimpleSeSystem(args)

    # Tell components about the expected physical memory ranges. This
    # is, for example, used by the MemConfig helper to determine where
    # to map DRAMs in the physical address space.
    system.mem_ranges = [ AddrRange(start=0, size="8192MB") ]

    # Configure the off-chip memory system.
    MemConfig.config_mem(args, system)

    # Parse the command line and get a list of Processes instances
    # that we can pass to gem5.
    processes = get_processes(args.commands_to_run)
    if len(processes) != args.num_cores:
        print("Error: Cannot map %d command(s) onto %d CPU(s)" %
              (len(processes), args.num_cores))
        sys.exit(1)

    # Assign one workload to each CPU
    #JMNOTE: Set UVE vector length bits
    #JMNOTE: Tests on rv_ext are designed for 1024 bits
    system.cpu.isa[0].uve_vl_se = UVE_VECTOR_LENGTH
    system.cpu.workload = processes[0]

    return system


def main():
    parser = argparse.ArgumentParser(epilog=__doc__)

    parser.add_argument("commands_to_run", metavar="command(s)", nargs='*',
                        help="Command(s) to run")
    parser.add_argument("--cpu", type=str, choices=cpu_types.keys(),
                        default="atomic",
                        help="CPU model to use")
    parser.add_argument("--cpu-freq", type=str, default="1GHz")
    parser.add_argument("--num-cores", type=int, default=1,
                        help="Number of CPU cores")
    # parser.add_argument("--mem-type", default="DDR4_2400_8x8",
                        # choices=MemConfig.mem_names(),
    parser.add_argument("--mem-type", default="DDR3_1600_8x8",
                        choices=MemConfig.mem_names(),
                        help = "type of memory to use")
    parser.add_argument("--mem-channels", type=int, default=2,
                        help = "number of memory channels")
    parser.add_argument("--mem-ranks", type=int, default=None,
                        help = "number of memory ranks per channel")
    parser.add_argument("--mem-size", action="store", type=str,
                        default="2GB",
                        help="Specify the physical memory size")

    args = parser.parse_args()

    # Create a single root node for gem5's object hierarchy. There can
    # only exist one root node in the simulator at any given
    # time. Tell gem5 that we want to use syscall emulation mode
    # instead of full system mode.
    root = Root(full_system=False)

    # Populate the root node with a system. A system corresponds to a
    # single node with shared memory.
    root.system = create(args)

    # Instantiate the C++ object hierarchy. After this point,
    # SimObjects can't be instantiated anymore.
    m5.instantiate()

    # Start the simulator. This gives control to the C++ world and
    # starts the simulator. The returned event tells the simulation
    # script why the simulator exited.
    event = m5.simulate()

    # Print the reason for the simulation exit. Some exit codes are
    # requests for service (e.g., checkpoints) from the simulation
    # script. We'll just ignore them here and exit.
    print(event.getCause(), " @ ", m5.curTick())
    sys.exit(event.getCode())


if __name__ == "__m5_main__":
    main()
