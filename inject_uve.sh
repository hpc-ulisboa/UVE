#!/bin/bash
BUILD_DIR="ext_modules/riscv-gnu-toolchain"

echo "-------------Deleting Backup Files"
./utils/make.py -d

echo "-------------Copying Source Files"
python3 ./files/copy-files.py

echo "-------------Building Files"
./utils/make.py -b
