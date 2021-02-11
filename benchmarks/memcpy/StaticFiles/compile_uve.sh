#!/usr/bin/bash
NEW_UUID=$1
DIR=$2
CONFIG_FILE=$DIR/starter_se_streaming.py
cd $DIR
make -B PLAT=riscv MODE=uve
$(./RVrun.sh "./uve_run" $NEW_UUID $CONFIG_FILE)
