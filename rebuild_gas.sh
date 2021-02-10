#!/bin/bash
BUILD_DIR="ext_modules/riscv-gnu-toolchain"

if [[ -n $1 ]]; then
    echo "-------------MAKE CLEAN"
    make --directory=${BUILD_DIR} clean
fi

echo "-------------Deleting Backup Files"
./make.py -d

echo "-------------Copying Source Files"
python3 ./files/copy-files.py

echo "-------------Building Files"
./make.py -b

echo "-------------Building GAS"
echo -n "Proceed? [y/n]: "
read ans
if [[ "$ans" == "y" ]]; then
    cd $BUILD_DIR
    make stamps/build-binutils-newlib -B --jobs=$(nproc)
    # make stamps/build-binutils-linux --jobs=5
    make install -B
fi

