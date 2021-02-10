riscv-binutils/bfd/reloc.c:
    Add BFD_RELOC_RISCV_UVEB
    to ENUMX of RISC-V relocations, after BFD_RELOC_RISCV_32_PCREL

riscv-gnu-toolchain:
    ./configure --prefix=<Installation Dir> [--with-march=<MICROARCH>] [--with-abi=<ABI>]
    make [-j<THREADS>]
