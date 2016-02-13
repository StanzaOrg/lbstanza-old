./build/stanzac -i boot/core/core.stanza \
                   boot/core/collections.stanza \
                   boot/core/reader.stanza \
                   boot/core/macro-utils.stanza \
                   boot/compiler/stz-algorithms.stanza \
                   boot/compiler/stz-utils.stanza \
                   boot/compiler/stz-parser.stanza \
                   boot/compiler/stz-core-macros.stanza \
                   boot/compiler/stz-ids.stanza \
                   boot/compiler/lang-read.stanza \
                   boot/compiler/lang-check.stanza \
                   boot/compiler/stz-primitives.stanza \
                   boot/compiler/stz-il-ir.stanza \
                   boot/compiler/stz-tl-ir.stanza \
                   boot/compiler/stz-kl-ir.stanza \
                   boot/compiler/stz-input.stanza \
                   boot/compiler/stz-namemap.stanza \
                   boot/compiler/stz-renamer.stanza \
                   boot/compiler/stz-resolver.stanza \
                   boot/compiler/stz-infer.stanza \
                   boot/compiler/stz-type-calculus.stanza \
                   boot/compiler/stz-type.stanza \
                   boot/tests/tests.stanza \
                -o test.s

gcc test.s boot/runtime/driver.c -o prog

