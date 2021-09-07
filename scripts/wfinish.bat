gcc -std=gnu99 -c core/sha256.c -O3 -o build/sha256.o
gcc -std=gnu99 -c compiler/cvm.c -O3 -o build/cvm.o
gcc -std=gnu99 runtime/driver.c build/cvm.o build/sha256.o wstanza.s -o wstanza -DPLATFORM_WINDOWS -lm
