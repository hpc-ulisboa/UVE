#!/usr/bin/bash

NEW_UUID=$1
BUILD_DIR=$2
CHECKPOINT=$3
BUILD_PATH=$BUILD_DIR/$NEW_UUID

mkdir -p ./$BUILD_PATH/m5out

for filename in StaticFiles/*; do
   ./utils/tcopy.py $filename ./$BUILD_PATH/$(basename $filename)
done

for filename in Templates/*; do
   ./utils/trender.py $filename ./$BUILD_PATH/$(basename $filename) "${@:4}"
done

for filename in SpecificFiles/*; do
   ./utils/trender.py $filename ./$BUILD_PATH/$(basename $filename) "${@:4}"
done

cd ./$BUILD_PATH/m5out
