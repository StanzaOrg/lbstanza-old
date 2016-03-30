./bootprog boot2/core/core.stanza \
           boot2/core/collections.stanza \
           boot2/tests/tests2.stanza \
        -s boottest.s

gcc boottest.s boot2/runtime/driver.c -o boottest
