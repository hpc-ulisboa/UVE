#!/bin/bash

cd ext_modules

cd riscv-gnu-toolchain
git checkout c3ad555
git clean -fd
git clean -fdX
git reset --hard HEAD
git restore .
echo PATCHES
git apply ../../patches/riscv-gnu-toolchain.patch
echo END_PATCHES

cd riscv-binutils
git checkout 0d2fb1b
git clean -fd
git clean -fdX
git reset --hard HEAD
echo PATCHES
git apply ../../../patches/riscv-gnu-toolchain.riscv-binutils.patch
echo END_PATCHES
cd ..

cd riscv-gcc
git checkout 489c9e7
cd ..

cd riscv-gdb
git checkout 635c14e
cd ..

cd riscv-newlib
git checkout 320b28e
cd ..

cd ../riscv-opcodes
git clean -fd
git checkout a1a194b
