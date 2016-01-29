./build/stanzac -i boot/core/core.stanza -o test.s
gcc test.s boot/runtime/driver.c -o prog
./prog
