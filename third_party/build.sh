#!/bin/bash

SCRIPT=`realpath $0`
SCRIPTPATH=`dirname $SCRIPT`

function build_sleigh {
    if [ ! -f "$SCRIPTPATH/sleigh/LICENSE" ]; then
        git submodule update --init
    fi

    [ -d "$SCRIPTPATH/sleigh/build" ] \
        || mkdir "$SCRIPTPATH/sleigh/build"

    pushd "$SCRIPTPATH/sleigh" || exit 1
    ./build.sh                 || exit 1
    popd
}

build_sleigh
