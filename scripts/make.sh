if [ $# -lt 1 ]; then
    echo "Not enough arguments"
    exit 2
fi

STANZA=$1

#All input source files
FILES="core/core.stanza \
       core/collections.stanza \
       core/reader.stanza \
       core/macro-utils.stanza \
       compiler/stz-algorithms.stanza \
       compiler/stz-padder.stanza \
       compiler/stz-utils.stanza \
       compiler/stz-parser.stanza \
       compiler/stz-serializer.stanza \
       compiler/stz-params.stanza \
       compiler/stz-core-macros.stanza \
       compiler/stz-ids.stanza \
       compiler/lang-read.stanza \
       compiler/lang-check.stanza \
       compiler/stz-primitives.stanza \
       compiler/stz-imms.stanza \
       compiler/stz-il-ir.stanza \
       compiler/stz-tl-ir.stanza \
       compiler/stz-pkg-ir.stanza \
       compiler/stz-kl-ir.stanza \
       compiler/stz-tgt-ir.stanza \
       compiler/stz-bb-ir.stanza \
       compiler/stz-asm-ir.stanza \
       compiler/stz-backend.stanza \
       compiler/stz-pkg.stanza \
       compiler/stz-input.stanza \
       compiler/stz-namemap.stanza \
       compiler/stz-renamer.stanza \
       compiler/stz-resolver.stanza \
       compiler/stz-infer.stanza \
       compiler/stz-type-calculus.stanza \
       compiler/stz-type.stanza \
       compiler/stz-kform.stanza \
       compiler/stz-tgt.stanza \
       compiler/stz-tgt-writer.stanza \
       compiler/stz-bb.stanza \
       compiler/stz-asm-emitter.stanza \
       compiler/stz-compiler.stanza \
       compiler/stz-arg-parser.stanza \
       compiler/stz-langs.stanza  \
       compiler/lang-renamer.stanza \
       compiler/lang-resolver.stanza \
       compiler/lang-serializer.stanza \
       compiler/stz-main.stanza"

#Delete pkg files
rm pkgs/*.pkg
rm fast-pkgs/*.pkg

#Compile OSX Executable
$STANZA $FILES -pkg fast-pkgs -optimize -o stanza

#Compile Linux Executable
$STANZA $FILES -optimize -s lstanza.s -platform linux

#Compile Pkg Files
$STANZA $FILES -pkg pkgs

#Compile Fast Pkg Files
$STANZA $FILES -pkg fast-pkgs -optimize
