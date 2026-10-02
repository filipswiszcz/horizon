#!/bin/bash

set -e

if [ "$(uname)" != "Linux" ]; then
    echo "OS not supported"
    exit 1
fi

MODE=$1

case "$MODE" in
    dev)
        ;;
    release)
        ;;
    *)
        echo "Available build options: dev, release"
        exit 1
        ;;
esac

CXX="g++"
LIBS=(-lpthread)
FLAGS=(-g -O3 -Wall -Wextra -std=c++17)

rm -rf build
mkdir -p bin build

$CXX "${FLAGS[@]}" \
    -I./lib \
    src/*.cpp \
    "${LIBS[@]}" \
    -o build/device

rm -rf bin/device

if [ "$MODE" == "dev" ]; then
    ln -sf ../build/device bin/device
else
    cp -f build/device bin/
fi

echo "BUILD COMPLETE"