from m5.params import *
from m5.proxy import *
from m5.objects.ClockedObject import ClockedObject
from RiscvTLB import RiscvTLB


class UVEStreamingEngine(ClockedObject):
    type = 'UVEStreamingEngine'
    cxx_header = "uve_simobjs/uve_streaming_engine.hh"

    # Vector port example. Both the instruction and data ports connect to this
    # port which is automatically split out into two ports.
    cpu_side = SlavePort("CPU side port, receives requests")
    mem_side = MasterPort("Memory side port, sends requests")

    latency = Param.Cycles(1, "Cycles taken on a hit or to resolve a miss")

    size = Param.Int(8, "The depth of the queue")

    width = Param.Int(1024, "Width in bits of the queue (Vector Width)")

    start_addr = Param.Addr(0, "First address for Streaming Cache")

    system = Param.System(Parent.any, "The system this cache is part of")

    tlb = Param.RiscvTLB(RiscvTLB(), "TLB/MMU to walk page table")
