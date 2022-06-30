#!/bin/bash

ARG1=$1
ARG2=$2

if [ "$ARG1" = "" ]; then
    ARCH="x86"
    if [ "$ARG2" = "seal" ] || [ "$ARG2" = "heaan" ]; then
        LIBREF=$ARG2
    else
        LIBREF="seal"
    fi
elif [ "$ARG1" = "x86" ]; then
    ARCH=$ARG1
    if [ "$ARG2" = "seal" ] || [ "$ARG2" = "heaan" ]; then
        LIBREF=$ARG2
    else
        LIBREF="seal"
    fi
elif [ "$ARG1" = "arm" ] || [ "$ARG1" = "arm-teegris" ]; then
    ARCH=$ARG1
    if [ "$ARG2" = "seal" ] || [ "$ARG2" = "heaan" ]; then
        LIBREF=$ARG2
    else
        LIBREF="seal"
    fi
elif [ "$ARG1" = "seal" ] || [ "$ARG1" = "heaan" ]; then
    ARCH="x86"
    LIBREF=$ARG1
else
    echo "wrong arguments passed in"
fi

CALLER_PATH=$(pwd)
echo "$CALLER_PATH"

SCRIPT_PATH="`dirname \"$0\"`"
SCRIPT_PATH="`( cd \"$SCRIPT_PATH\" && pwd )`"
echo "$SCRIPT_PATH"
cd $SCRIPT_PATH

set -e

./clean.sh
mkdir build || true
cd build
#cmake ../hedge

if [ "$LIBREF" = "seal" ] || [ "$LIBREF" = "heaan" ]; then
    echo "Building $LIBREF-compatible library for $ARCH-architecture"
    if [ "$LIBREF" = "seal" ]; then
        cmake -DCMAKE_BUILD_TYPE=Debug -DARCH=$ARCH ../hedge
    elif [ "$LIBREF" = "heaan" ]; then
        cmake -DCMAKE_BUILD_TYPE=Debug -DARCH=$ARCH ../hedge2
    fi
    make -j1
    make install

    cd "$SCRIPT_PATH"

    if [ "$ARCH" = "arm" ] || [ "$ARCH" = "arm-teegris" ]; then
        echo "Compiled for ARM. Compiling aarch64sim"
        mkdir ./apps/aarch64sim/build || true
        cd ./apps/aarch64sim/build
        cmake -DCMAKE_BUILD_TYPE=Debug -DARCH=$ARCH ..
        make -j16
    else
        if [ "$LIBREF" = "seal" ]; then
            # make test app and run test
            mkdir ./apps/test/build || true
            cd ./apps/test/build
            echo ">> pwd = $(pwd)"
            cmake ..
            make -j16
            ./hedgetest
        elif [ "$LIBREF" = "heaan" ]; then
            mkdir ./apps/test2/build || true
            cd ./apps/test2/build
            echo ">> pwd = $(pwd)"
            cmake ..
            make -j16
            ./hedgetest2
        fi
    fi

    cd "$CALLER_PATH"
fi