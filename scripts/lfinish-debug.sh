#scripts/make-asmjit.sh linux
#gcc -std=gnu99 -c core/sha256.c -O3 -o build/sha256.o -fPIC -I include
#gcc -std=gnu99 -c compiler/cvm.c -O3 -o build/cvm.o -fPIC -I include
#gcc -std=gnu99 core/threadedreader.c runtime/driver.c runtime/linenoise.c build/cvm.o build/sha256.o lstanza.s -o lstanza -DPLATFORM_LINUX -lm -ldl -fPIC -I include -Lbin -lasmjit-linux -lstdc++ -lrt -lpthread

../stanza ../core/stanza.proj ../compiler/stanza.proj dummy.stanza -s dummy.s
STANZADIR="/home/opliss/Work/lbstanza"
cc dummy.s $STANZADIR/runtime/driver.c $STANZADIR/runtime/debug.c $STANZADIR/core/sighandler.c $STANZADIR/runtime/linenoise.c -DBUILD_DEBUG -std=gnu99 -lm -lpthread -ldl -fPIC -DPLATFORM_LINUX -I/home/opliss/Work/lbstanza/include -o dummy
cp dummy /home/opliss/Work/lbstanza/runtime/debug