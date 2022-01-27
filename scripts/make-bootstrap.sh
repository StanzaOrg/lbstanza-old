rm -r pkgs/*
git checkout merge-new-bootstrap
stanza compiler/stanza.proj stz/driver -o stanzaboot -optimize -verbose
git checkout merge-new-gc
mkdir tempinstall
./stanzaboot install -platform os-x -path tempinstall
./stanzaboot core/stanza.proj compiler/stanza.proj -o newstanza -optimize -verbose
./newstanza core/stanza.proj core collections -pkg pkgs
