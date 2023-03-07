#!/bin/bash
set -Eeuxo pipefail
PS4='>>> '

#USAGE: ./scripts/zipstanza.sh 0_11_1
​
if [ $# -lt 1 ]; then
    echo "Not enough arguments"
    exit 2
fi
​
FILES="docs \
       bin \
       include \
       pkgs \
       stanza \
       lpkgs \
       lstanza \
       wpkgs \
       wstanza.exe \
       core \
       compiler \
       examples \
       runtime \
       stanza.proj \
       License.txt \
       ChangeLog.txt"
​
mkdir workdir
mkdir workdir/build
cd ../stanzadev
cp -r $FILES ../stanzasite/workdir
cd ../stanzasite/workdir
mv bin/libasmjit-os-x.a bin/libasmjit.a
zip -r stanza.zip build bin/libasmjit.a include docs pkgs compiler core runtime examples License.txt ChangeLog.txt stanza.proj stanza
​
rm -rf pkgs
mv lpkgs pkgs
mv lstanza stanza
mv bin/libasmjit-linux.a bin/libasmjit.a
zip -r lstanza.zip build bin/libasmjit.a include docs pkgs compiler core runtime examples License.txt ChangeLog.txt stanza.proj stanza
​
rm -rf pkgs
mv wpkgs pkgs
mv wstanza.exe stanza.exe
mv bin/libasmjit-windows.a bin/libasmjit.a
zip -r wstanza.zip build bin/libasmjit.a include docs pkgs compiler core runtime examples License.txt ChangeLog.txt stanza.proj stanza.exe
​
mv -f stanza.zip ../resources/stanza/stanza_$1.zip
mv -f lstanza.zip ../resources/stanza/lstanza_$1.zip
mv -f wstanza.zip ../resources/stanza/wstanza_$1.zip
cd ..
rm -rf workdir
