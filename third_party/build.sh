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

function build_pugixml {
    if [ ! -f "$SCRIPTPATH/pugixml/LICENSE.md" ]; then
        git submodule update --init
    fi

    [ -d "$SCRIPTPATH/pugixml/build" ] \
        || mkdir "$SCRIPTPATH/pugixml/build"

    pushd "$SCRIPTPATH/pugixml/build" || exit 1
    cmake .. || exit 1
    make -j`nproc` || exit 1
    popd
}

build_sleigh
build_pugixml
