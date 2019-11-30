# USAGES:
# ./scripts/make.sh ./stanza

set -e
set -o pipefail

if [ $# -lt 1 ]; then
    echo "Not enough arguments"
    exit 2
fi

STANZA=$1

#Pkg packages
PKGFILES="stz/test-driver"

#Delete pkg files
rm -rf pkgs
rm -rf lpkgs
#rm -rf wpkgs

#Create folders
mkdir -p pkgs
mkdir -p lpkgs
#mkdir -p wpkgs

#Compile OSX Pkgs and Executable
echo "Compiling OSX Pkgs"
$STANZA build-stanza.proj stz/driver $PKGFILES -pkg pkgs
$STANZA build-stanza.proj stz/driver $PKGFILES -pkg pkgs -optimize
echo "Compiling OSX Executable"
$STANZA build-stanza.proj stz/driver -pkg pkgs -s stanza.s -optimize

#Compile Linux Pkgs and Executable
echo "Compiling Linux Pkgs"
$STANZA build-stanza.proj stz/driver $PKGFILES -pkg lpkgs -platform linux
$STANZA build-stanza.proj stz/driver $PKGFILES -pkg lpkgs -optimize -platform linux
echo "Compiling Linux Executable"
$STANZA build-stanza.proj stz/driver -pkg lpkgs -s lstanza.s -optimize -platform linux

#Finish on osx
#gcc -std=gnu99 -c compiler/cvm.c -O3 -o cvm.o
#gcc -std=gnu99 runtime/driver.c runtime/linenoise.c cvm.o stanza.s -o stanza -DPLATFORM_OS_X -lm
#Finish on linux
#gcc -std=gnu99 -c compiler/cvm.c -O3 -o cvm.o
#gcc -std=gnu99 runtime/driver.c runtime/linenoise.c cvm.o lstanza.s -o lstanza -DPLATFORM_LINUX -lm -ldl -fPIC
#Finish on windows
#gcc -std=gnu99 runtime/driver.c wstanza.s -o wstanza -DPLATFORM_WINDOWS -lm
