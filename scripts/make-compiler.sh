# USAGES:
# ./make-compiler
# ./make-compiler bin/stanzadev
# ./make-compiler ./stanzac bin/stanzadev

if [ $# -eq 0 ]
then
    STANZA=./stanza
    OUT=./bin/stanzadev
elif [ $# -eq 1 ]
then
    STANZA=./stanza
    OUT=$1
elif [ $# -eq 2 ]
then
    STANZA=$1
    OUT=$2
fi

mkdir -p bin

$STANZA  core/core.stanza \
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
         compiler/stz-imms.stanza \
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
         compiler/stz-backend.stanza \
         compiler/stz-reg-alloc.stanza \
         compiler/stz-codegen.stanza \
         compiler/stz-stitcher.stanza \
         compiler/stz-asm-emitter.stanza \
         compiler/stz-fuse.stanza \
         compiler/stz-pkg.stanza \
         compiler/stz-compiler.stanza \
         compiler/stz-arg-parser.stanza \
         compiler/stz-langs.stanza  \
         compiler/lang-renamer.stanza \
         compiler/lang-resolver.stanza \
         compiler/lang-serializer.stanza \
         compiler/stz-main.stanza \
      -o $OUT

#$STANZA  core/core.stanza \
#         core/collections.stanza \
#         core/reader.stanza \
#         core/parser.stanza \
#         core/macro-utils.stanza \
#         compiler/stz-algorithms.stanza \
#         compiler/stz-padder.stanza \
#         compiler/stz-utils.stanza \
#         compiler/stz-serializer.stanza \
#         compiler/stz-params.stanza \
#         compiler/stz-core-macros.stanza \
#         compiler/stz-ids.stanza \
#         compiler/lang-read.stanza \
#         compiler/lang-check.stanza \
#         compiler/stz-primitives.stanza \
#         compiler/stz-imms.stanza \
#         compiler/stz-il-ir.stanza \
#         compiler/stz-tl-ir.stanza \
#         compiler/stz-pkg-ir.stanza \
#         compiler/stz-kl-ir.stanza \
#         compiler/stz-tgt-ir.stanza \
#         compiler/stz-bb-ir.stanza \
#         compiler/stz-asm-ir.stanza \
#         compiler/stz-backend.stanza \
#         compiler/stz-pkg.stanza \
#         compiler/stz-input.stanza \
#         compiler/stz-namemap.stanza \
#         compiler/stz-renamer.stanza \
#         compiler/stz-resolver.stanza \
#         compiler/stz-infer.stanza \
#         compiler/stz-type-calculus.stanza \
#         compiler/stz-type.stanza \
#         compiler/stz-kform.stanza \
#         compiler/stz-tgt.stanza \
#         compiler/stz-tgt-writer.stanza \
#         compiler/stz-bb.stanza \
#         compiler/stz-asm-emitter.stanza \
#         compiler/stz-compiler.stanza \
#         compiler/stz-arg-parser.stanza \
#         compiler/stz-langs.stanza  \
#         compiler/lang-renamer.stanza \
#         compiler/lang-resolver.stanza \
#         compiler/lang-serializer.stanza \
#         compiler/stz-main.stanza \
#      -o $OUT
