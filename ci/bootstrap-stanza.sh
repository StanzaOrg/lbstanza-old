#!/bin/bash
set -Eeuxo pipefail
PS4='>>> '

# Stanza bootstrap compiler commands
# - This script will compile an output file called "stanzatemp" in the current directory
#   using the existing stanza compiler defined by $STANZA_CONFIG/.stanza
# - The expected existing compiler version will be verified against the version number
#   stored in "build-stanza-version.txt" in the same directory as this script

# This script is designed to be run with the following env vars

USAGE="STANZA_CONFIG=/path $0"

# Required env var inputs
echo "     STANZA_CONFIG:" "${STANZA_CONFIG:?Usage: ${USAGE}}"          # directory where .stanza config file will be stored, as in normal stanza behavior

# special case - if STANZA_CONFIG starts with "./", then replace it with the full path
[[ ${STANZA_CONFIG::2} == "./" ]] && STANZA_CONFIG=${PWD}/${STANZA_CONFIG:2}

# Calculated env vars
STANZADIR=$(grep ^install-dir $STANZA_CONFIG/.stanza | cut -f2 -d\")
STANZA="${STANZADIR}/stanza"

# find and read build-stanza-version.txt
# find the directory where this script is stored
THISDIR=$(dirname $(readlink -f $0))
BSVTXT="${THISDIR}/build-stanza-version.txt"
# extract the version from first non-comment line of the file
BSTZVER=$(grep -v ^\# "${BSVTXT}" | head -1 | awk '{ print $1}')
echo "Using existing stanza version $BSTZVER"

# verify existing stanza version
CURVER="$(${STANZA} version -terse)"
if [[ "${BSTZVER}" != "${CURVER}" ]] ; then
    printf "\n\n*** ERROR: existing stanza version \"${CURVER}\" of stanza at path\n      \"${STANZA}\"\n    does not match the desired version of \"${BSTZVER}\" from \n      \"${THISDIR}/build-stanza-version.txt\"\n\n\n"
    exit -2
fi

# stanza build commands go here
# format commands like:
#    ${STANZA} compile timon pumbaa -o stanzatemp
# assume the current directory is the root of a stanza repository

${STANZA} compile-macros \
  compiler/fastio-serializer-macros.stanza \
  compiler/renamer-lang.stanza \
  compiler/check-lang.stanza \
  compiler/ast-lang2.stanza \
  compiler/ast-lang.stanza \
  compiler/resolver-lang.stanza \
  compiler/message-lang.stanza \
  compiler/ast-printer-lang.stanza \
  compiler/reader-lang.stanza \
  -o bootstrap.macros 

${STANZA} build-stanza.proj stz/driver -o stanzatemp -macros bootstrap.macros -flags BOOTSTRAP -optimize

