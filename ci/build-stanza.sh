#!/bin/bash
set -Eeuxo pipefail
PS4='>>> '
TOP="${PWD}"

# This script is designed to be run from a Concourse Task with the following env vars

USAGE="STANZA_CONFIG=/path $0"

# Required env var inputs
echo "     STANZA_CONFIG:" "${STANZA_CONFIG:?Usage: ${USAGE}}"          # directory where .stanza config file will be stored, as in normal stanza behavior

# Defaulted env var inputs - can override if necessary
echo "           REPODIR:" "${REPODIR:=lbstanza}"
echo "    CREATE_ARCHIVE:" "${CREATE_ARCHIVE:=false}"
echo "    CREATE_PACKAGE:" "${CREATE_PACKAGE:=false}"
echo "               VER:" "${VER:=$(git -C ${REPODIR} describe --tags --abbrev=0)}"

# Calculated env vars
STANZADIR=$(grep ^install-dir $STANZA_CONFIG/.stanza | cut -f2 -d\")

cd "${REPODIR}"
echo "Building lbstanza version ${VER} in ${PWD}"

mkdir -p build
${STANZADIR}/stanza build-stanza.proj stz/driver -o stanzatemp -flags BOOTSTRAP -optimize -verbose

if [ "$CREATE_PACKAGE" == "true" ] ; then
  # TODO FIXME
  make package
fi

if [ "$CREATE_ARCHIVE" == "true" ] ; then
  zip -r -9 -q stanza-build-${PLATFORM}-${VER}.zip \
      .conan \
      .stanza \
      CMakeUserPresets.json \
      build
fi
