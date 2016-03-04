./bstanzac boot2/core/core.stanza \
           boot2/core/collections.stanza \
           boot2/core/reader.stanza \
           boot2/core/macro-utils.stanza \
           boot2/compiler/stz-algorithms.stanza \
           boot2/compiler/stz-padder.stanza \
           boot2/compiler/stz-utils.stanza \
           boot2/compiler/stz-parser.stanza \
           boot2/compiler/stz-params.stanza \
           boot2/compiler/stz-core-macros.stanza \
           boot2/compiler/stz-ids.stanza \
           boot2/compiler/lang-read.stanza \
           boot2/compiler/lang-check.stanza \
           boot2/compiler/stz-primitives.stanza \
           boot2/compiler/stz-il-ir.stanza \
           boot2/compiler/stz-tl-ir.stanza \
           boot2/compiler/stz-kl-ir.stanza \
           boot2/compiler/stz-tgt-ir.stanza \
           boot2/compiler/stz-bb-ir.stanza \
           boot2/compiler/stz-asm-ir.stanza \
           boot2/compiler/stz-backend.stanza \
           boot2/compiler/stz-input.stanza \
           boot2/compiler/stz-namemap.stanza \
           boot2/compiler/stz-renamer.stanza \
           boot2/compiler/stz-resolver.stanza \
           boot2/compiler/stz-infer.stanza \
           boot2/compiler/stz-type-calculus.stanza \
           boot2/compiler/stz-type.stanza \
           boot2/compiler/stz-kform.stanza \
           boot2/compiler/stz-tgt.stanza \
           boot2/compiler/stz-bb.stanza \
           boot2/compiler/stz-asm-emitter.stanza \
           boot2/compiler/stz-compiler.stanza \
           boot2/compiler/stz-arg-parser.stanza \
           boot2/compiler/stz-langs.stanza  \
           boot2/compiler/lang-renamer.stanza \
           boot2/compiler/lang-resolver.stanza \
           boot2/compiler/stz-main.stanza \
        -s bootprog.s \
        -optimize

gcc bootprog.s boot2/runtime/driver.c -o build/stanza3



#./bstanzac boot2/core/core.stanza \
#           boot2/core/collections.stanza \
#           boot2/tests/tests2.stanza \
#        -s bootprog.s


#./bstanzac boot2/core/core.stanza \
#           boot2/core/collections.stanza \
#           boot2/core/reader.stanza \
#           boot2/core/macro-utils.stanza \
#           boot2/compiler/stz-algorithms.stanza \
#           boot2/compiler/stz-padder.stanza \
#           boot2/compiler/stz-utils.stanza \
#           boot2/compiler/stz-parser.stanza \
#           boot2/compiler/stz-params.stanza \
#           boot2/compiler/stz-core-macros.stanza \
#           boot2/compiler/stz-ids.stanza \
#           boot2/compiler/lang-read.stanza \
#           boot2/compiler/lang-check.stanza \
#           boot2/compiler/stz-primitives.stanza \
#           boot2/compiler/stz-il-ir.stanza \
#           boot2/compiler/stz-tl-ir.stanza \
#           boot2/compiler/stz-kl-ir.stanza \
#           boot2/compiler/stz-tgt-ir.stanza \
#           boot2/compiler/stz-bb-ir.stanza \
#           boot2/compiler/stz-asm-ir.stanza \
#           boot2/compiler/stz-backend.stanza \
#           boot2/compiler/stz-input.stanza \
#           boot2/compiler/stz-namemap.stanza \
#           boot2/compiler/stz-renamer.stanza \
#           boot2/compiler/stz-resolver.stanza \
#           boot2/compiler/stz-infer.stanza \
#           boot2/compiler/stz-type-calculus.stanza \
#           boot2/compiler/stz-type.stanza \
#           boot2/compiler/stz-kform.stanza \
#           boot2/compiler/stz-tgt.stanza \
#           boot2/compiler/stz-bb.stanza \
#           boot2/compiler/stz-asm-emitter.stanza \
#           boot2/compiler/stz-compiler.stanza \
#           boot2/compiler/stz-arg-parser.stanza \
#           boot2/compiler/stz-langs.stanza  \
#           boot2/compiler/lang-renamer.stanza \
#           boot2/compiler/lang-resolver.stanza \
#           boot2/compiler/stz-main.stanza \
#        -s bootprog.s
# gcc bootprog.s boot/runtime/driver.c -o bootprog
