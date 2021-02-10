#!/bin/sh
RISCV_TOOLCHAIN_SRC=ext_modules/riscv-gnu-toolchain
INSTALL_DIR=$(dirname $(realpath $0))/../build/uve_tc
# EXTRA_ARGS="--with-arch=rv32imf --with-abi=ilp32f --enable-multilib"

cd $RISCV_TOOLCHAIN_SRC
mkdir $INSTALL_DIR/bin -p
export PATH=${PATH}:$INSTALL_DIR/bin

./configure --prefix=$INSTALL_DIR $EXTRA_ARGS
make -j$(nproc)
