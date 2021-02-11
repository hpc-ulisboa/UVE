#!/usr/bin/bash
remote_compilation() {
    if [[ $COMPILE == true ]]; then
        BUILD_DIR="~/joaomario_temp/builds/$NEW_UUID"
        MACHINE=juawei-a28
        LDCC="module load Generic-AArch64/RHEL/7/arm-hpc-compiler/19.2;"
        CHDIR="cd $BUILD_DIR;"
        OBJS=$(find . -name '*.c' -o -name "*.h")
        ssh $MACHINE "mkdir -p $BUILD_DIR/build"
        rsync -avzhL -e ssh Makefile $OBJS $MACHINE:$BUILD_DIR
        ssh $MACHINE  "$CHDIR $LDCC make -B $1"
        rsync -avzh -e ssh $MACHINE:$BUILD_DIR/${2}_run $DIR
    fi
}

local_compilation() {
    if [[ $COMPILE == true ]]; then
        make -B $1
    fi
}

COMPILE=true #Set to false to not re-compile
if [[ -n $3 ]]; then
    echo -e "Skipping compilation steps\n"
    COMPILE=false
    DEBUG_FLAGS="--debug-flags=UVEMem,UVEArith,UVEFifo,VecPredRegs,UVEUtils,UVESEI,JMDEVEL,UVESE,UVEBranch,UVERename"
    echo -e "Setting debugging flags: $DEBUG_FLAGS\n"
fi


NEW_UUID=$1
ARCH=uve
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

cd $DIR

case $ARCH in

    rv)
        CONFIG_FILE="$DIR/starter_se_streaming.py"
        BINARY="rv_run_opt"
        iGEM5=$GEM5_RV
        local_compilation "PLAT=riscv MODE=opt"
    ;;

    uve)
        CONFIG_FILE=$DIR/starter_se_streaming.py
        BINARY=uve_run
        iGEM5=$GEM5_RV
        local_compilation "PLAT=riscv MODE=uve"
    ;;

    arm)
        CONFIG_FILE=$DIR/starter_se_sve.py
        BINARY=arm_run
        iGEM5=$GEM5_ARM
        remote_compilation "PLAT=arm" "arm"
        # local_compilation "PLAT=arm" "arm"
    ;;

    sve)
        CONFIG_FILE=$DIR/starter_se_sve.py
        BINARY=sve_run
        iGEM5=$GEM5_ARM
        remote_compilation "PLAT=arm MODE=sve" "sve"
        # local_compilation "PLAT=arm MODE=sve" "sve"
    ;;

    x86)
       BINARY=x86_run
       local_compilation "PLAT=x86" "x86"
       ./x86_run
       exit 0
    ;;

    *)
    echo "Error: ${2} is not a valid architecture"
    ;;

esac


DIR=m5out
mkdir -p $DIR/logs
OUTPUT_DIR=--outdir=$DIR
LOGS_DIR="-r -e --stdout-file=stdout --stderr-file=stderr"
DEBUG_CONF=--debug-file=trace.out
STATS_CONF=--stats-file=stats.out
JSON_CONF=--json-config=jsonconfig.out
ADD_CONF="--cpu=o3" 

time $iGEM5 $OUTPUT_DIR $LOGS_DIR $DEBUG_FLAGS $DEBUG_CONF $STATS_CONF $JSON_CONF $CONFIG_FILE $ADD_CONF "$BINARY $NEW_UUID"