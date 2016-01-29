./build/stanzac boot/core/core.stanza test.s
gcc test.s boot/runtime/driver.c -o prog
./prog
