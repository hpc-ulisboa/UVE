#!/usr/bin/bash

NEW_UUID=$1
BUILD_DIR=$2
BUILD_PATH=$BUILD_DIR/$NEW_UUID

mkdir -p ./$BUILD_PATH/build

for filename in StaticFiles/*; do
   ./utils/tcopy.py $filename ./$BUILD_PATH/$(basename $filename)
done

for filename in Templates/*; do
   ./utils/trender.py $filename ./$BUILD_PATH/$(basename $filename) "${@:3}"
done

