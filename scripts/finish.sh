scripts/make-asmjit.sh os-x
gcc -std=gnu99 -c core/sha256.c -O3 -o build/sha256.o -I include
gcc -std=gnu99 -c compiler/cvm.c -O3 -o build/cvm.o -I include
gcc -std=gnu99 core/threadedreader.c runtime/driver.c compiler/exec-alloc.c runtime/linenoise.c build/cvm.o build/sha256.o stanza.s -o stanza -DPLATFORM_OS_X -lm -I include -Lbin -lasmjit-os-x -lc++
