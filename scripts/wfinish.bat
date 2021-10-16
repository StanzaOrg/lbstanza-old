gcc -std=gnu99 -c core/sha256.c -O3 -o build/sha256.o -I include
gcc -std=gnu99 -c compiler/cvm.c -O3 -o build/cvm.o -I include
gcc -std=gnu99 runtime/driver.c build/cvm.o build/sha256.o wstanza.s -o wstanza -DPLATFORM_WINDOWS -Wl,-Bstatic -lm -lpthread -I include
