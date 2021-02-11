#!/usr/bin/bash
NEW_UUID=$1
DIR=$2
CONFIG_FILE=$DIR/starter_se_sve.py
cd $DIR

BUILD_DIR="~/joaomario_temp/builds/$NEW_UUID"

MACHINE=juawei-a28
LDCC="module load Generic-AArch64/RHEL/7/arm-hpc-compiler/19.2;"
CHDIR="cd $BUILD_DIR;"
OBJS=$(find . -name '*.c' -o -name "*.h")

ssh $MACHINE "mkdir -p $BUILD_DIR/build"
rsync -avzh -e ssh Makefile $OBJS $MACHINE:$BUILD_DIR

ssh $MACHINE  "$CHDIR $LDCC make -B PLAT=arm"

rsync -avzh -e ssh $MACHINE:$BUILD_DIR/arm_run $DIR

$(./ARMrun.sh "./arm_run" $NEW_UUID $CONFIG_FILE)

