#./build/stanzac -i boot/core/core.stanza \
#                   boot/core/collections.stanza \
#                   boot/core/reader.stanza \
#                   boot/core/macro-utils.stanza \
#                   boot/compiler/stz-algorithms.stanza \
#                   boot/compiler/lang-read.stanza \                   
#                   boot/compiler/stz-il-ir.stanza \
#                   boot/compiler/stz-input.stanza \
#                   boot/tests/tests.stanza \
#                -o test.s

./build/stanzac -i boot/core/core.stanza \
                   boot/core/collections.stanza \
                   boot/core/reader.stanza \
                   boot/core/macro-utils.stanza \
                   boot/compiler/stz-algorithms.stanza \
                   boot/tests/tests.stanza \
                -o test.s
gcc test.s boot/runtime/driver.c -o prog
./prog
