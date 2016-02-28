./bstanzac boot2/core/core.stanza \
           boot2/core/collections.stanza \
           boot2/tests/tests2.stanza \
        -s bootprog.s

gcc bootprog.s boot2/runtime/driver.c -o bootprog

