from m5.params import *
from m5.proxy import *
from m5.objects.ClockedObject import ClockedObject
from RiscvTLB import RiscvTLB


class UVEStreamingEngine(ClockedObject):
    type = 'UVEStreamingEngine'
    cxx_header = "uve_simobjs/uve_streaming_engine.hh"

    # Vector port example. Both the instruction and data ports connect to this
    # port which is automatically split out into two ports.
    mem_side_store = MasterPort("Memory side port, sends requests")
    mem_side_load = MasterPort("Memory side port, sends requests")

    latency = Param.Cycles(1, "Cycles taken on a hit or to resolve a miss")

    fifo_depth = Param.Int(8, "The depth of the data fifo")

    max_request_size = Param.Int(512, "The max size per memory request,"
    + "per iteration")

    width = Param.Int(1024, "Width in bits of the queue (Vector Width)")

    start_addr = Param.Addr(0, "First address for Streaming Cache")

    do_rename = Param.Bool(True, "Activate or deactivate stream id renaming")

    system = Param.System(Parent.any, "The system this cache is part of")

    tlb = Param.RiscvTLB(RiscvTLB(), "TLB/MMU to walk page table")
