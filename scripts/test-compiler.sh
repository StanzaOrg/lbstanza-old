./stanza boot/core/core.stanza \
         boot/core/collections.stanza \
         boot/core/reader.stanza \
         boot/core/macro-utils.stanza \
         boot/compiler/stz-algorithms.stanza \
         boot/compiler/stz-padder.stanza \
         boot/compiler/stz-utils.stanza \
         boot/compiler/stz-parser.stanza \
         boot/compiler/stz-params.stanza \
         boot/compiler/stz-core-macros.stanza \
         boot/compiler/stz-ids.stanza \
         boot/compiler/lang-read.stanza \
         boot/compiler/lang-check.stanza \
         boot/compiler/stz-primitives.stanza \
         boot/compiler/stz-il-ir.stanza \
         boot/compiler/stz-tl-ir.stanza \
         boot/compiler/stz-kl-ir.stanza \
         boot/compiler/stz-tgt-ir.stanza \
         boot/compiler/stz-bb-ir.stanza \
         boot/compiler/stz-asm-ir.stanza \
         boot/compiler/stz-backend.stanza \
         boot/compiler/stz-input.stanza \
         boot/compiler/stz-namemap.stanza \
         boot/compiler/stz-renamer.stanza \
         boot/compiler/stz-resolver.stanza \
         boot/compiler/stz-infer.stanza \
         boot/compiler/stz-type-calculus.stanza \
         boot/compiler/stz-type.stanza \
         boot/compiler/stz-kform.stanza \
         boot/compiler/stz-tgt.stanza \
         boot/compiler/stz-bb.stanza \
         boot/compiler/stz-asm-emitter.stanza \
         boot/compiler/stz-compiler.stanza \
         boot/compiler/stz-arg-parser.stanza \
         boot/compiler/stz-langs.stanza  \
         boot/compiler/lang-renamer.stanza \
         boot/compiler/lang-resolver.stanza \
         boot/compiler/stz-main.stanza \
      -s test.s \
      -o bin/$1 \
      -no-implicits \
      -optimize
