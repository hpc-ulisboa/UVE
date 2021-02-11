#!/bin/bash

UUID=$1
ARCH=$2
UUIDc=`expr match "$UUID" '\([a-zA-Z0-9+]*\)'`
BUILD_PATH=builds/$UUIDc
ID=${UUID##${UUIDc}_}

PREP_BUILD=./utils/prepare_build.sh
CLEAN_BUILD=./utils/clean_build.sh
EXECUTE_BUILD=$BUILD_PATH/$ID/compile.sh
EXTRACT_DATA=$BUILD_PATH/$ID/extract_data.py

RESULTS_DIR=$BUILD_PATH/$ID/m5out/$ID

$PREP_BUILD $ID $BUILD_PATH ${@:3}
cp ~/thesis_scripts/tools/*run.sh $BUILD_PATH/$ID

$EXECUTE_BUILD $ID $ARCH

$EXTRACT_DATA $ID $BUILD_PATH/$ID 

# $CLEAN_BUILD $ID