# USAGES:
# ./scripts/make.sh ./stanza

set -e
set -o pipefail

if [ $# -lt 1 ]; then
    echo "Not enough arguments"
    exit 2
fi

STANZA=$1

#All input source files
FILES="core/core.stanza \
       core/collections.stanza \
       core/reader.stanza \
       core/parser.stanza \
       core/macro-utils.stanza \
       compiler/line-noise-prompter.stanza \
       compiler/stz-algorithms.stanza \
       compiler/stz-utils.stanza \
       compiler/stz-stable-arrays.stanza \
       compiler/stz-serializer.stanza \
       compiler/stz-params.stanza \
       compiler/stz-core-macros.stanza \
       compiler/stz-ids.stanza \
       compiler/lang-read.stanza \
       compiler/lang-check.stanza \
       compiler/stz-primitives.stanza \
       compiler/stz-il-ir.stanza \
       compiler/stz-tl-ir.stanza \
       compiler/stz-input.stanza \
       compiler/stz-visibility.stanza \
       compiler/stz-basic-ops.stanza \
       compiler/stz-namemap.stanza \
       compiler/stz-renamer.stanza \
       compiler/stz-resolver.stanza \
       compiler/stz-infer.stanza \
       compiler/stz-type-calculus.stanza \
       compiler/stz-type.stanza \
       compiler/stz-tl-to-el.stanza \
       compiler/stz-tl-to-dl.stanza \
       compiler/stz-el-ir.stanza \
       compiler/stz-dl-ir.stanza \
       compiler/stz-vm-ir.stanza \
       compiler/stz-typeset.stanza \
       compiler/stz-dl.stanza \
       compiler/stz-el.stanza \
       compiler/stz-el-infer.stanza \
       compiler/stz-basic-blocks.stanza \
       compiler/stz-ehier.stanza \
       compiler/stz-vm.stanza \
       compiler/stz-bindings.stanza \
       compiler/stz-bindings-to-vm.stanza \
       compiler/stz-vm-encoder.stanza \
       compiler/stz-trie.stanza \
       compiler/stz-dyn-tree.stanza \
       compiler/stz-dyn-bi-table.stanza \
       compiler/stz-vm-table.stanza \
       compiler/stz-branch-table.stanza \
       compiler/stz-vm-analyze.stanza \
       compiler/stz-vm-bindings.stanza \
       compiler/stz-loaded-ids.stanza \
       compiler/stz-el-to-vm.stanza \
       compiler/stz-vm-normalize.stanza \
       compiler/stz-compiler.stanza \
       compiler/stz-arg-parser.stanza \
       compiler/stz-langs.stanza \
       compiler/lang-renamer.stanza \
       compiler/lang-resolver.stanza \
       compiler/lang-serializer.stanza \
       compiler/stz-codegen2.stanza \
       compiler/stz-reg-alloc2.stanza \
       compiler/stz-stitcher2.stanza \
       compiler/stz-const-pool.stanza \
       compiler/stz-padder.stanza \
       compiler/stz-build.stanza \
       compiler/stz-repl.stanza \
       compiler/stz-config.stanza \
       compiler/stz-asm-ir2.stanza \
       compiler/stz-asm-emitter.stanza \
       compiler/stz-pkg.stanza \
       compiler/stz-backend.stanza \
       compiler/stz-gen-bindings.stanza \
       compiler/stz-dependencies.stanza \
       compiler/call-records.stanza \
       compiler/stz-main.stanza \
       compiler/stz-driver.stanza"

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
$STANZA $FILES -pkg pkgs
echo "Compiling OSX Executable"
$STANZA $FILES -pkg pkgs -optimize -s stanza.s

#Compile Linux Pkgs and Executable
echo "Compiling Linux Pkgs"
$STANZA $FILES -pkg lpkgs -platform linux
echo "Compiling Linux Executable"
$STANZA $FILES -pkg lpkgs -optimize -s lstanza.s -platform linux

#Compile Windows Pkgs and Executable
#echo "Compiling Windows Pkgs"
#$STANZA $FILES -pkg wpkgs -platform windows
#echo "Compiling Windows Executable"
#$STANZA $FILES -pkg wfast-pkgs -optimize -s wstanza.s -platform windows

#Finish on osx
#gcc -std=gnu99 -c compiler/cvm.c -O3 -o cvm.o
#gcc -std=gnu99 runtime/driver.c runtime/linenoise.c cvm.o stanza.s -o stanza -DPLATFORM_OS_X -lm
#Finish on linux
#gcc -std=gnu99 -c compiler/cvm.c -O3 -o cvm.o
#gcc -std=gnu99 runtime/driver.c runtime/linenoise.c cvm.o lstanza.s -o lstanza -DPLATFORM_LINUX -lm -ldl -fPIC
#Finish on windows
#gcc -std=gnu99 runtime/driver.c wstanza.s -o wstanza -DPLATFORM_WINDOWS -lm
