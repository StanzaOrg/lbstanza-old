#!/usr/bin/env bash

# USAGES:
# ./scripts/run-postcompile-tests.sh ./mystanza

STANZA="$1"
export STANZA_COMPILER="$STANZA"

$STANZA run-test tests/stanza.proj stz/stanza-postcompile-tests

$STANZA compile-test build-stanza.proj tests/stanza.proj stz/stanza-postcompile-tests -o build/stanza-postcompile-tests
./build/stanza-postcompile-tests
$STANZA compile-test build-stanza.proj tests/stanza.proj stz/stanza-postcompile-compiler-only-tests -o build/stanza-postcompile-compiler-only-tests
./build/stanza-postcompile-compiler-only-tests
