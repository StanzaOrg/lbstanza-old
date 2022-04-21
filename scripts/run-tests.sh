stanza run-test build-stanza.proj tests/stanza.proj stz/stanza-tests -not-tagged long
stanza compile-test build-stanza.proj tests/stanza.proj stz/stanza-tests -o build/stanza-tests
./build/stanza-tests
