#!/bin/bash
set -Eeuxo pipefail
PS4='>>> '
TOP="${PWD}"

# This script is designed to be run from a Concourse Task with the following env vars

USAGE="STANZA_CONFIG=/path $0"

# Required env var inputs
echo "     STANZA_CONFIG:" "${STANZA_CONFIG:?Usage: ${USAGE}}"          # directory where .stanza config file will be stored, as in normal stanza behavior

# Defaulted env var inputs - can override if necessary
echo "              REPODIR:" "${REPODIR:=lbstanza}"
echo "      CONAN_USER_HOME:" "${CONAN_USER_HOME:=${REPODIR}}"
echo "       CREATE_ARCHIVE:" "${CREATE_ARCHIVE:=false}"
echo "       CREATE_PACKAGE:" "${CREATE_PACKAGE:=false}"
echo "STANZA_BUILD_PLATFORM:" "${STANZA_BUILD_PLATFORM:=$(uname -s)}"  # linux|macos|windows
echo "                  VER:" "${VER:=$(git -C ${REPODIR} describe --tags --abbrev=0)}"

# special case - if STANZA_CONFIG starts with "./", then replace it with the full path
[[ ${STANZA_CONFIG::2} == "./" ]] && STANZA_CONFIG=${PWD}/${STANZA_CONFIG:2}

# Calculated env vars
STANZADIR=$(grep ^install-dir $STANZA_CONFIG/.stanza | cut -f2 -d\")

case "$STANZA_BUILD_PLATFORM" in
    Linux* | linux* | ubuntu*)
        STANZA_BUILD_PLATFORM=linux
        STANZA_PLATFORMCHAR="l"
    ;;
    Darwin | mac* | os-x)
        STANZA_BUILD_PLATFORM=os-x
        STANZA_PLATFORMCHAR=""
    ;;
    MINGW* | win*)
        STANZA_BUILD_PLATFORM=windows
        STANZA_PLATFORMCHAR="w"
    ;;
    *)
        printf "\n\n*** ERROR: unknown build platform \"${STANZA_BUILD_PLATFORM}\"\n\n\n" && exit -2
    ;;
esac


cd "${REPODIR}"
echo "Building lbstanza version ${VER} in ${PWD}"

mkdir -p build
mkdir -p bin
# copy asmjit.a from stanza install into repository bin directory
# so it can be linked into the bootstrap compiler
cp -a ${STANZADIR}/bin/libasmjit.a bin/libasmjit.a

${STANZADIR}/stanza build-stanza.proj stz/driver -o stanzatemp -flags BOOTSTRAP -optimize -verbose
# verify that expected output file exists
ls stanzatemp

scripts/make.sh ./stanzatemp ${STANZA_BUILD_PLATFORM} compile-clean-without-finish

scripts/${STANZA_PLATFORMCHAR}finish.sh


if [ "$CREATE_PACKAGE" == "true" ] ; then
  ci/zipstanza.sh ${VER//./_}  # convert dots to underscores
fi

if [ "$CREATE_ARCHIVE" == "true" ] ; then
  #zip -r -9 -q stanza-build-${PLATFORM}-${VER}.zip \
  #    .conan \
  #    .stanza \
  #    CMakeUserPresets.json \
  #    build
fi
