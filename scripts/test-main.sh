./build/stanzac -i boot/core/core.stanza boot/core/collections.stanza boot/core/reader.stanza -o test.s
gcc test.s boot/runtime/driver.c -o prog
./prog
