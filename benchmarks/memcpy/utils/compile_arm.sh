#!/usr/bin/bash
MACHINE=juawei-a28
LDCC="module load Generic-AArch64/RHEL/7/arm-hpc-compiler/19.2;"
CHDIR="cd ~/joaomario_temp/memcpy;"
TARGET=MEMCPY
OBJS="main.c mysecond.c memcpy.c dataset.h"

echo "Sync Files"
rsync -avzh -e ssh Makefile $OBJS $MACHINE:~/joaomario_temp/${TARGET,,}
# scp Makefile hello_risc_rect.c dataset.h $MACHINE:~/joaomario_temp

echo "Compiling"
ssh $MACHINE  "$CHDIR mkdir -p ./build"
ssh $MACHINE  "$CHDIR $LDCC make -B PLAT=arm MODE=sve"
ssh $MACHINE  "$CHDIR $LDCC make -B PLAT=arm"
# make arm_run

echo "Getting Binaries"
rsync -avzh -e ssh $MACHINE:~/joaomario_temp/${TARGET,,}/*_run .

NEW_UUID=$(cat /dev/urandom | tr -dc 'a-zA-Z0-9' | fold -w 10 | head -n 1)

echo "-----Gem5 Simulation ARM $NEW_UUID"

$(./../../tools/ARMrun.sh "./${TARGET}_arm_run" $NEW_UUID)&
pids[0]=$!
$(./../../tools/ARMrun.sh "./${TARGET}_sve_run" $NEW_UUID)&
pids[1]=$!

for pid in ${pids[*]}; do
    wait $pid
done

SVE_FILE="./m5out_arm_$NEW_UUID/logs/${TARGET}_sve_run.stdout"
ARM_FILE="./m5out_arm_$NEW_UUID/logs/${TARGET}_arm_run.stdout"

vim -O  $ARM_FILE $SVE_FILE
