#./stanza core/reader.stanza \
#         core/macro-utils.stanza \
#         compiler/stz-algorithms.stanza \
#         compiler/stz-padder.stanza \
#         compiler/stz-utils.stanza \
#         compiler/stz-parser.stanza \
#         compiler/stz-params.stanza \
#         compiler/stz-core-macros.stanza \
#         compiler/stz-ids.stanza \
#         compiler/lang-read.stanza \
#         compiler/lang-check.stanza \
#         compiler/stz-primitives.stanza \
#         compiler/stz-il-ir.stanza \
#         compiler/stz-tl-ir.stanza \
#         compiler/stz-pkg-ir.stanza \
#         compiler/stz-kl-ir.stanza \
#         compiler/stz-tgt-ir.stanza \
#         compiler/stz-bb-ir.stanza \
#         compiler/stz-asm-ir.stanza \
#         compiler/stz-input.stanza \
#         compiler/stz-namemap.stanza \
#         compiler/stz-renamer.stanza \
#         compiler/stz-resolver.stanza \
#         compiler/stz-infer.stanza \
#         compiler/stz-type-calculus.stanza \
#         compiler/stz-type.stanza \
#         compiler/stz-kform.stanza \
#         compiler/stz-compiler.stanza \
#         compiler/stz-arg-parser.stanza \
#         compiler/stz-langs.stanza  \
#         compiler/lang-renamer.stanza \
#         compiler/lang-resolver.stanza \
#         compiler/stz-main.stanza \
#      -o bin/$1

./stanza core/reader.stanza \
         core/macro-utils.stanza \
         compiler/stz-algorithms.stanza \
         compiler/stz-padder.stanza \
         compiler/stz-utils.stanza \
         compiler/stz-parser.stanza \
         compiler/stz-params.stanza \
         compiler/stz-core-macros.stanza \
         compiler/stz-ids.stanza \
         compiler/lang-read.stanza \
         compiler/lang-check.stanza \
         compiler/stz-primitives.stanza \
         compiler/stz-il-ir.stanza \
         compiler/stz-tl-ir.stanza \
         compiler/stz-pkg-ir.stanza \
         compiler/stz-kl-ir.stanza \
         compiler/stz-tgt-ir.stanza \
         compiler/stz-bb-ir.stanza \
         compiler/stz-asm-ir.stanza \
         compiler/stz-backend.stanza \
         compiler/stz-input.stanza \
         compiler/stz-namemap.stanza \
         compiler/stz-renamer.stanza \
         compiler/stz-resolver.stanza \
         compiler/stz-infer.stanza \
         compiler/stz-type-calculus.stanza \
         compiler/stz-type.stanza \
         compiler/stz-kform.stanza \
         compiler/stz-tgt.stanza \
         compiler/stz-bb.stanza \
         compiler/stz-asm-emitter.stanza \
         compiler/stz-compiler.stanza \
         compiler/stz-arg-parser.stanza \
         compiler/stz-langs.stanza  \
         compiler/lang-renamer.stanza \
         compiler/lang-resolver.stanza \
         compiler/stz-main.stanza \
      -o bin/$1
