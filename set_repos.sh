#!/bin/bash

cd ext_modules

cd riscv-gnu-toolchain
git clean -fd
git clean -fdX
git checkout c3ad555
git reset --hard HEAD
git restore .
git apply ../../patches/riscv-gnu-toolchain.patch

cd riscv-binutils
git clean -fd
git clean -fdX
git checkout 0d2fb1b
git reset --hard HEAD
git restore .
git apply ../../../patches/riscv-gnu-toolchain.riscv-binutils.patch
cd ..

cd riscv-gcc
git checkout 489c9e7
git restore .
cd ..

cd riscv-gdb
git checkout 635c14e
git restore .
cd ..

cd riscv-newlib
git checkout 320b28e
git restore .
cd ..

cd ../riscv-opcodes
git clean -fd
git restore .
git checkout a1a194b
