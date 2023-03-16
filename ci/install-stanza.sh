#!/bin/bash
set -Eeuxo pipefail
PS4='>>> '
TOP="${PWD}"

# This script is designed to be run from a Concourse Task with the following env vars

# Following the gcc style of compile system names,
# There are three system names that the build knows about:
#   the machine you are building on (build)
#   the machine that you are building for (host)
#   and the machine that GCC will produce code for (target).

USAGE="STANZA_BUILD_PLATFORM={linux|macos|windows} STANZA_BUILD_VER=0.17.56 STANZA_CONFIG=/path STANZA_INSTALL_DIR=/path $0"
# TODO FIXME downloading from internet addresses without https is dangerous
STANZA_DOWNLOAD_BASEURL="http://lbstanza.org/resources/stanza"


# Required env var inputs
echo "   STANZA_INSTALL_DIR:" "${STANZA_INSTALL_DIR:?Usage: ${USAGE}}"     # where the stanza binaries will be installed
echo "        STANZA_CONFIG:" "${STANZA_CONFIG:?Usage: ${USAGE}}"          # directory where .stanza config file will be stored, as in normal stanza behavior

# Defaulted env var inputs - can override if necessary
# find and read build-stanza-version.txt
# find the directory where this script is stored
THISDIR=$(dirname $(readlink -f $0))
BSVTXT="${THISDIR}/build-stanza-version.txt"
# extract the version from first non-comment line of the file
BSTZVER=$(grep -v ^\# "${BSVTXT}" | head -1 | awk '{ print $1}')
echo "Using existing stanza version $BSTZVER"
echo "     STANZA_BUILD_VER:" "${STANZA_BUILD_VER:=${BSTZVER}}"        # 0.17.56
echo "STANZA_BUILD_PLATFORM:" "${STANZA_BUILD_PLATFORM:=$(uname -s)}"  # linux|macos|Darwin|windows|MINGW64

# var input validation
case "${STANZA_BUILD_PLATFORM}" in
    Linux* | linux* | ubuntu*)
        STANZA_BUILD_PLATFORM=linux
        STANZA_DOWNLOAD_PLATFORMCHAR="l"
    ;;
    Darwin | mac* | os-x)
        STANZA_BUILD_PLATFORM=os-x
        STANZA_DOWNLOAD_PLATFORMCHAR=""
    ;;
    MINGW* | win*)
        STANZA_BUILD_PLATFORM=windows
        STANZA_DOWNLOAD_PLATFORMCHAR="w"
    ;;
    *)
        printf "\n\n*** ERROR: STANZA_BUILD_PLATFORM input \"$STANZA_BUILD_PLATFORM\" not one of linux, macos, windows\n\n\n" && exit -2
    ;;
esac
# special case - if STANZA_INSTALL_DIR or STANZA_CONFIG start with "./", then replace it with the full path
[[ ${STANZA_INSTALL_DIR::2} == "./" ]] && STANZA_INSTALL_DIR=${PWD}/${STANZA_INSTALL_DIR:2}
[[ ${STANZA_CONFIG::2} == "./" ]] && STANZA_CONFIG=${PWD}/${STANZA_CONFIG:2}
# verify full paths
[[ ${STANZA_INSTALL_DIR::1} != "/" ]] && printf "\n\n*** ERROR: STANZA_INSTALL_DIR must be a full path starting with \"/\"\n\n\n" && exit -2
[[ ${STANZA_CONFIG::1} != "/" ]] && printf "\n\n*** ERROR: STANZA_CONFIG must be a full path starting with \"/\"\n\n\n" && exit -2
mkdir -p "${STANZA_CONFIG}"

# Calculated env vars
STANZA_DOWNLOAD_VER=${STANZA_BUILD_VER//./_}  # convert dots to underscores
STANZA_DOWNLOAD_FILE="${STANZA_DOWNLOAD_PLATFORMCHAR}stanza_${STANZA_DOWNLOAD_VER}.zip"
STANZA_DOWNLOAD_URL="${STANZA_DOWNLOAD_BASEURL}/${STANZA_DOWNLOAD_FILE}"
TOP="${PWD}"

# download zip to a tmp dir, unzip, install, and clean up tmp dir
TMP="$(mktemp -d ./stanza.XXXXXXXXXX)"
(
    set -Eeuxo pipefail
    trap "rm -rf \"${TMP}\"" ERR EXIT
    cd "${TMP}"
    echo "Downloading ${STANZA_DOWNLOAD_URL} to ${TMP}/${STANZA_DOWNLOAD_FILE}"
    curl -L "${STANZA_DOWNLOAD_URL}" -o "${STANZA_DOWNLOAD_FILE}"
    echo "Installing existing stanza version ${STANZA_BUILD_VER} for ${STANZA_BUILD_PLATFORM} in \"${STANZA_INSTALL_DIR}\""
    unzip -q "${STANZA_DOWNLOAD_FILE}" -d "${STANZA_INSTALL_DIR}"
    cd "${STANZA_INSTALL_DIR}"
    ./stanza install -platform ${STANZA_BUILD_PLATFORM} -path "${STANZA_CONFIG}"
)

