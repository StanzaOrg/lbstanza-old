../stanza ../core/stanza.proj ../compiler/stanza.proj dummy.stanza -s dummy.s
STANZADIR="/Users/patricksli/Docs/Programming/stanzadev"
gcc dummy.s $STANZADIR/runtime/driver.c $STANZADIR/runtime/debug.c $STANZADIR/core/sighandler.c $STANZADIR/runtime/linenoise.c -DBUILD_DEBUG -std=gnu99 -ldl -DPLATFORM_OS_X -I$STANZADIR/include -o dummy -D_XOPEN_SOURCE
cp dummy $STANZADIR/runtime/debug
