gcc -std=gnu99 -c core/sha256.c -O3 -o build/sha256.o -fPIC -I include
gcc -std=gnu99 -c compiler/cvm.c -O3 -o build/cvm.o -fPIC -I include
gcc -std=gnu99 runtime/driver.c runtime/linenoise.c build/cvm.o build/sha256.o lstanza.s -o lstanza -DPLATFORM_LINUX -lm -ldl -fPIC -I include -g1
