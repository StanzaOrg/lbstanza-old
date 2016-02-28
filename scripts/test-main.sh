./build/stanzac -i boot/core/core.stanza \
                   boot/core/collections.stanza \
                   boot/tests/tests2.stanza \
                -o test2.s \
                -optimize


gcc test2.s boot/runtime/driver.c -o prog2
