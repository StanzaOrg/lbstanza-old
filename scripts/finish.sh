gcc -std=gnu99 -c compiler/cvm.c -O3 -o cvm.o
gcc -std=gnu99 runtime/driver.c runtime/linenoise.c cvm.o stanza.s -o stanza -DPLATFORM_OS_X -lm
