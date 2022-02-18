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

function fetch_and_build_libbfd {
    version=2.38
    if [ ! -d "$SCRIPTPATH/binutils" ]; then
        pushd "$SCRIPTPATH" || exit 1
        wget https://ftp.gnu.org/gnu/binutils/binutils-$version.tar.gz || exit 1
        tar xzf binutils-$version.tar.gz || exit 1
        mv ./binutils-$version ./binutils || exit 1
        rm binutils-$version.tar.gz || exit 1
        popd
    fi

    pushd "$SCRIPTPATH/binutils/libiberty" || exit 1
    CFLAGS="-fPIC -O3" ./configure || exit 1
    make -j`nproc` || exit 1
    popd

    pushd "$SCRIPTPATH/binutils/zlib" || exit 1
    CFLAGS="-fPIC -O3" ./configure || exit 1
    make -j`nproc` || exit 1
    popd

    pushd "$SCRIPTPATH/binutils/bfd" || exit 1
    LD_LIBRARY_PATH="`pwd`/../libiberty:`pwd`/../zlib" CFLAGS="-fPIC -O3" \
        ./configure --enable-shared || exit 1
    make -j`nproc` || exit 1
    popd

    [ -d "$SCRIPTPATH/binutils/build" ] || \
        mkdir "$SCRIPTPATH/binutils/build"
    pushd "$SCRIPTPATH/binutils/build"
    cp "`pwd`/../bfd/.libs/libbfd.a" . || exit 1
    cp "`pwd`/../libiberty/libiberty.a" . || exit 1
    cp "`pwd`/../zlib/libz.a" . || exit 1
    popd
}

build_sleigh
# build_pugixml
# fetch_and_build_libbfd
