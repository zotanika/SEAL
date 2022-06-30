#!/bin/bash

ARCH="x86"
ALGO="heaan"
TRUSTENV="qsee"
TEST=0

usage()
{
    echo "Usage: build.sh [OPTION]... 
Example: build.sh --arch arm --algo heaan -t

  -c, --arch        select architecture (x86 or arm)
  -a ,--algo        select algorithm (heaan or seal)
  -e, --trustenv    select trusted execution environment (qsee or teegris)
  -t, --test        build and run tests
  -d, --debug       build with debug options
  -r, --contrand    Use constant random across runs
  "
    exit 2
}

test()
{
    if [ "$ARCH" = "arm" ]; then
        echo "Compiled for ARM. Compiling aarch64sim"
        mkdir ./apps/aarch64sim/build || true
        cd ./apps/aarch64sim/build
        cmake -DCMAKE_BUILD_TYPE=Debug -DARCH=$ARCH -DTRUSTENV=$TENV ..
        make -j16
    else
        if [ "$ALGO" = "seal" ]; then
            # make test app and run test
            mkdir ./apps/test/build || true
            cd ./apps/test/build
            echo ">> pwd = $(pwd)"
            cmake -DDEBUG_WITH_CPP4C_PRNG=$CONSTRAND ..
            make -j16
            ./hedgetest
        elif [ "$ALGO" = "heaan" ]; then
            mkdir ./apps/test2/build || true
            cd ./apps/test2/build
            echo ">> pwd = $(pwd)"
            cmake -DDEBUG_WITH_CPP4C_PRNG=$CONSTRAND ..
            make -j16
            ./hedgetest2
        fi
    fi
}

PARSED_ARGUMENTS=$(getopt -a -n build.sh -o c:a:te:drh --long arch:,algo:,test,trustenv:,debug,constrand,help -- "$@")
VALID_ARGUMENTS=$?
if [ "$VALID_ARGUMENTS" != "0" ]; then
    usage
fi

#echo "ARGUMENTS : $PARSED_ARGUMENTS"
eval set -- "$PARSED_ARGUMENTS"

while :
do
    case "$1" in
        -c | --arch)      ARCH=$2     ; shift 2 ;;
        -a | --algo)      ALGO=$2     ; shift 2 ;;
        -t | --test)      TEST=1      ; shift   ;;
        -e | --trustenv)  TRUSTENV=1  ; shift   ;;
        -d | --debug)     DEBUG=1     ; shift   ;;
        -r | --constrand) CONSTRAND=1 ; shift   ;;
        -h | --help)      usage       ; shift   ;;
        --) shift; break ;;
        *) echo "Unknown option : $1 "
            usage ;;
    esac
done

CALLER_PATH=$(pwd)

echo "ARCH     : $ARCH"
echo "ALGO     : $ALGO"
echo "TRUSTENV : $TRUSTENV"
echo "TEST     : $TEST"
echo "DEBUG    : $DEBUG"
echo "CONSTRAND: $CONSTRAND"
echo "PWD      : $CALLER_PATH"

if [ "$ALGO" != "seal" ] && [ "$ALGO" != "heaan" ]; then
    echo "Unsupported algorithm $ALGO"
    exit 2
fi

SCRIPT_PATH="`dirname \"$0\"`"
SCRIPT_PATH="`( cd \"$SCRIPT_PATH\" && pwd )`"
echo "$SCRIPT_PATH"
cd $SCRIPT_PATH

set -e

./clean.sh
mkdir build || true
cd build

echo "Building $ALGO-compatible library for $ARCH-architecture"
if [ "$ALGO" = "seal" ]; then
    cmake -DCMAKE_BUILD_TYPE=Debug -DARCH=$ARCH -DTRUSTENV=$TENV ../hedge
elif [ "$ALGO" = "heaan" ]; then
    cmake -DCMAKE_BUILD_TYPE=Debug -DARCH=$ARCH -DTRUSTENV=$TENV -DDEBUG_WITH_CPP4C_PRNG=$CONSTRAND ../hedge2
fi
make -j8
make install

cd "$SCRIPT_PATH"

if [ $TEST == 1 ]; then
    test
fi

cd "$CALLER_PATH"
