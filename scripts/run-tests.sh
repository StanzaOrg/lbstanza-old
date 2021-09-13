stanza run-test tests/stanza.proj stz/stanza-tests
stanza compile-test build-stanza.proj tests/stanza.proj stz/stanza-tests -o build/stanza-tests
./build/stanza-tests
