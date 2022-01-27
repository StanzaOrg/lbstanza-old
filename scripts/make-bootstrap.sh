# clean up everything
rm -r pkgs/*
unset STANZA_CONFIG

# checkout the bootstrap branch and compile stanzaboot
git checkout merge-new-gc-bootstrap
stanza compiler/stanza.proj stz/driver -o stanzaboot -optimize -verbose

# checkout the merge-new-gc branch
git checkout merge-new-gc

# set this folder as the current installation.
mkdir tempinstall
./stanzaboot install -platform os-x -path tempinstall
export STANZA_CONFIG=$PWD/tempinstall

# Use the bootstrap compiler to compile the new compiler and new pkgs
./stanzaboot core/stanza.proj compiler/stanza.proj -o newstanza -optimize -verbose
./newstanza core/stanza.proj core collections -pkg pkgs

# Display finish
./newstanza version
