./build/stanza3 boot2/core/core.stanza \
                boot2/core/collections.stanza \
                boot2/core/reader.stanza \
                boot2/core/macro-utils.stanza \
                boot2/compiler/stz-params.stanza \
                boot2/compiler/stz-algorithms.stanza \
                boot2/compiler/stz-parser.stanza \
                boot2/compiler/stz-arg-parser.stanza \
                boot2/compiler/stz-main.stanza \
             -s test.s

gcc test.s boot2/runtime/driver.c -o test
