#!/usr/bin/env bash

# USAGES:
# ./scripts/make.sh ./stanza os-x
# ./scripts/make.sh lstanza windows

HERE="$(dirname "${BASH_SOURCE[0]}")"

set -e
set -o pipefail

if [ $# -lt 2 ]; then
    echo "Not enough arguments"
    exit 2
fi

STANZA="$1"
PLATFORM="$2"
COMMAND="${3:-compile-clean}" # default to `compile-clean` if no command is passed

case "$COMMAND" in
    compile)
        CLEAN=0 COMPILE=1 FINISH=1 ;;
    compile-clean)
        CLEAN=1 COMPILE=1 FINISH=1 ;;
    compile-without-finish)
        CLEAN=0 COMPILE=1 FINISH=0 ;;
    compile-clean-without-finish)
        CLEAN=1 COMPILE=1 FINISH=0 ;;
    clean)
        CLEAN=1 COMPILE=0 FINISH=0 ;;
    *) cat 1>&2 <<EOF
Error: unsupported/unrecognized command: \`$COMMAND\`
Supported commands: compile, compile-clean, compile-without-finish, compile-clean-without-finish, clean
EOF
esac

case "$PLATFORM" in
    windows) PLATFORM_PREFIX="w" ;;
    linux)   PLATFORM_PREFIX="l" ;;
    os-x)    PLATFORM_PREFIX=""  ;;
    *) cat 1>&2 <<EOF
Error: unsupported/unrecognized platform: \`$PLATFORM\`
Supported platforms: windows, linux, os-x
EOF
       exit 2 ;;
esac

case "$PLATFORM" in
    windows) DPLATFORM="Windows" ;;
    linux)   DPLATFORM="Linux"   ;;
    os-x)    DPLATFORM="OS X"    ;;
esac

echo "Building Stanza for $DPLATFORM"

#Pkg packages
PKGFILES="math core/meta-utils arg-parser line-wrap stz/test-driver stz/mocker stz/arg-parser stz/macro-plugin stz/timing-log-reader"
PKGDIR="${PLATFORM_PREFIX}pkgs"
STANZA_S="${PLATFORM_PREFIX}stanza.s"

#Clean
if [[ "$CLEAN" == 1 ]]; then
    echo "Cleaning Stanza files"

    #Delete pkg files
    rm -rf "$PKGDIR"

    "$STANZA" clean
fi

#Make pkg dir if it doesn't exist
mkdir -p "$PKGDIR"

if [[ "$COMPILE" == 1 ]]; then
    echo "Compiling $DPLATFORM Stanza Pkgs"
    "$STANZA" build-stanza.proj stz/driver $PKGFILES -pkg "$PKGDIR" -platform $PLATFORM

    echo "Compiling $DPLATFORM Stanza Optimized Pkgs"
    "$STANZA" build-stanza.proj stz/driver $PKGFILES -pkg "$PKGDIR" -optimize -platform $PLATFORM

    echo "Compiling $DPLATFORM Stanza Executable"
    "$STANZA" build-stanza.proj stz/driver -pkg "$PKGDIR" -s "$STANZA_S" -optimize -platform $PLATFORM
fi

if [[ "$FINISH" == 1 ]]; then
    echo "Finishing $DPLATFORM executable"
    case "$PLATFORM" in
        windows) "$HERE"/wfinish.sh ;;
        linux)   "$HERE"/lfinish.sh ;;
        os-x)    "$HERE"/finish.sh  ;;
    esac
fi
