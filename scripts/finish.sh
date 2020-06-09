gcc -std=gnu99 -c core/sha256.c -O3 -o sha256.o
gcc -std=gnu99 -c compiler/cvm.c -O3 -o cvm.o
gcc -std=gnu99 runtime/driver.c runtime/linenoise.c cvm.o sha256.o stanza.s -o stanza -DPLATFORM_OS_X -lm -mmacosx-version-min=10.13
