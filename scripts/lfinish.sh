gcc -std=gnu99 -c core/sha256.c -O3 -o sha256.o -fPIC
gcc -std=gnu99 -c compiler/cvm.c -O3 -o cvm.o -fPIC
gcc -std=gnu99 runtime/driver.c runtime/linenoise.c cvm.o sha256.o lstanza.s -o lstanza -DPLATFORM_LINUX -lm -ldl -fPIC
