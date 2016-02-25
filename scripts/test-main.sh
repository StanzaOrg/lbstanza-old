./build/stanzac -i boot/core/core.stanza \
                   boot/core/collections.stanza \
                   boot/core/reader.stanza \
                   boot/core/macro-utils.stanza \
                   boot/compiler/stz-algorithms.stanza \
                   boot/compiler/stz-parser.stanza \
                   boot/tests/tests2.stanza \
                -o test2.s

gcc test2.s boot/runtime/driver.c -o prog2
