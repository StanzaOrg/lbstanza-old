#!/usr/bin/env bash

set -e

if [[ -z "$CC" ]]; then
    if [[ "$(uname -s)" == MINGW* ]]; then
        # Not cross-compiling, so just use the stock GCC
        CC=gcc
    else
        echo 'Need to set $CC to a valid compiler or cross-compiler' >/dev/stderr
        exit 2
    fi
fi

CCFLAGS="-I include -DPLATFORM_WINDOWS -std=gnu99 -O3 -fPIC -Wall $CCFLAGS"

[[ ! -d "build" ]] && mkdir "build"

"$CC" $CCFLAGS -c core/sha256.c       -o build/sha256.o
"$CC" $CCFLAGS -c compiler/cvm.c      -o build/cvm.o
"$CC" $CCFLAGS -c runtime/driver.c    -o build/driver.o

"$CC" \
    build/sha256.o    \
    build/cvm.o       \
    build/driver.o    \
    wstanza.s         \
    -o wstanza -Wl,-Bstatic -lm -lpthread -fPIC
