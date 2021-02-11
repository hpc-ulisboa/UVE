#!/usr/bin/bash
NEW_UUID=$1
DIR=$2
CONFIG_FILE=$DIR/starter_se_streaming.py
cd $DIR
make -B PLAT=riscv MODE=opt
$(~/thesis_scripts/tools/RVrun.sh "./rv_run_opt" $NEW_UUID $CONFIG_FILE)

