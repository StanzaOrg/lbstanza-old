# USAGES:
# ./scripts/make.sh ./stanza

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
       compiler/stz-algorithms.stanza \
       compiler/stz-padder.stanza \
       compiler/stz-utils.stanza \
       compiler/stz-serializer.stanza \
       compiler/stz-params.stanza \
       compiler/stz-core-macros.stanza \
       compiler/stz-ids.stanza \
       compiler/lang-read.stanza \
       compiler/lang-check.stanza \
       compiler/stz-primitives.stanza \
       compiler/stz-il-ir.stanza \
       compiler/stz-tl-ir.stanza \
       compiler/stz-kl-ir.stanza \
       compiler/stz-pl-ir.stanza \
       compiler/stz-tgt-ir.stanza \
       compiler/stz-tgt-utils.stanza \
       compiler/stz-asm-ir.stanza \
       compiler/stz-input.stanza \
       compiler/stz-namemap.stanza \
       compiler/stz-renamer.stanza \
       compiler/stz-resolver.stanza \
       compiler/stz-infer.stanza \
       compiler/stz-type-calculus.stanza \
       compiler/stz-type.stanza \
       compiler/stz-type-to-kform.stanza \
       compiler/stz-kform.stanza \
       compiler/stz-dec-table.stanza \
       compiler/stz-kform-to-tgt.stanza \
       compiler/stz-khier.stanza \
       compiler/stz-trie.stanza \
       compiler/stz-backend.stanza \
       compiler/stz-reg-alloc.stanza \
       compiler/stz-codegen.stanza \
       compiler/stz-stitcher.stanza \
       compiler/stz-asm-emitter.stanza \
       compiler/stz-fuse.stanza \
       compiler/stz-pkg.stanza \
       compiler/stz-compiler.stanza \
       compiler/stz-arg-parser.stanza \
       compiler/stz-langs.stanza \
       compiler/lang-renamer.stanza \
       compiler/lang-resolver.stanza \
       compiler/lang-serializer.stanza \
       compiler/stz-build.stanza \
       compiler/stz-config.stanza \
       compiler/stz-main.stanza \
       compiler/stz-driver.stanza"

#Delete pkg files
rm -rf pkgs
rm -rf fast-pkgs
rm -rf lpkgs
rm -rf lfast-pkgs
rm -rf wpkgs
rm -rf wfast-pkgs

#Create folders
mkdir -p pkgs
mkdir -p fast-pkgs
mkdir -p lpkgs
mkdir -p lfast-pkgs
mkdir -p wpkgs
mkdir -p wfast-pkgs

#Compile OSX Pkgs and Executable
echo "Compiling OSX Pkgs"
$STANZA $FILES -pkg pkgs
echo "Compiling OSX Executable"
$STANZA $FILES -pkg fast-pkgs -optimize -s stanza.s

#Compile Linux Pkgs and Executable
echo "Compiling Linux Pkgs"
$STANZA $FILES -pkg lpkgs -platform linux
echo "Compiling Linux Executable"
$STANZA $FILES -pkg lfast-pkgs -optimize -s lstanza.s -platform linux

#Compile Windows Pkgs and Executable
echo "Compiling Windows Pkgs"
$STANZA $FILES -pkg wpkgs -platform windows
echo "Compiling Windows Executable"
$STANZA $FILES -pkg wfast-pkgs -optimize -s wstanza.s -platform windows

#Finish on osx
#gcc -std=gnu99 runtime/driver.c stanza.s -o stanza -DPLATFORM_OS_X -lm
#Finish on linux
#gcc -std=gnu99 runtime/driver.c lstanza.s -o lstanza -DPLATFORM_LINUX -lm -ldl -fPIC
#Finish on windows
#gcc -std=gnu99 runtime/driver.c wstanza.s -o wstanza -DPLATFORM_WINDOWS -lm
