#!/usr/bin/bash
CMD=$1
TARGET=MEMCPY

if [ "$#" -lt 1 ] ; then
CMD=""
fi

echo "Compiling"
if [ "$CMD" != "rv" ] ; then
    make -B PLAT=riscv MODE=uve
fi
if [ "$CMD" != "uve" ] ; then
    make -B PLAT=riscv MODE=opt
fi

NEW_UUID=$(cat /dev/urandom | tr -dc 'a-zA-Z0-9' | fold -w 10 | head -n 1)

echo "-----Gem5 Simulation $NEW_UUID"

if [ "$CMD" != "uve" ] ; then
    $(./../../tools/RVrun.sh "./${TARGET}_rv_run_opt" $NEW_UUID)&
    pids[0]=$!
fi
if [ "$CMD" != "rv" ] ; then
    $(./../../tools/RVrun.sh "./${TARGET}_uve_run" $NEW_UUID)&
    pids[1]=$!
fi

for pid in ${pids[*]}; do
    wait $pid
done

UVE_FILE=""
RV_FILE=""

if [ "$CMD" != "rv" ] ; then
UVE_FILE="./m5out_$NEW_UUID/logs/${TARGET}_uve_run.stdout"
fi
if [ "$CMD" != "uve" ] ; then
RV_FILE="./m5out_$NEW_UUID/logs/${TARGET}_rv_run_opt.stdout"
fi

vim -O $RV_FILE $UVE_FILE
