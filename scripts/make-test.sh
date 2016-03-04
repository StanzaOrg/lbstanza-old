./build/stanza3 boot2/core/core.stanza \
                boot2/core/collections.stanza \
                boot2/tests/tests2.stanza \
             -s test.s

gcc test.s boot2/runtime/driver.c -o test
