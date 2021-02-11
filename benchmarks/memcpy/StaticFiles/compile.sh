#!/usr/bin/bash
NEW_UUID=$1
ARCH=$2
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

case $ARCH in

    rv)
    $DIR/compile_rv.sh $NEW_UUID $DIR
    ;;

    uve)
    $DIR/compile_uve.sh $NEW_UUID $DIR
    ;;

    arm)
    $DIR/compile_arm.sh $NEW_UUID $DIR
    ;;

    sve)
    $DIR/compile_sve.sh $NEW_UUID $DIR
    ;;

    *)
    echo "Error: ${2} is not a valid architecture"
    ;;

esac