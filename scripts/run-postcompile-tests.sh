#!/usr/bin/env bash

# USAGES:
# ./scripts/run-postcompile-tests.sh ./mystanza

# Check arguments
if [ $# -lt 1 ]; then
    echo "Not enough arguments"
    exit 2
fi

STANZA="$1"
export STANZA_COMPILER="$STANZA"

# Run all the tests in stanza-postcompile-tests in the VM
$STANZA run-test tests/stanza.proj stz/stanza-postcompile-tests

# Compile and run all the tests in stanza-postcompile-tests 
$STANZA compile-test build-stanza.proj tests/stanza.proj stz/stanza-postcompile-tests -o build/stanza-postcompile-tests
build/stanza-postcompile-tests

# Run all the tests in stz/stanza-postcompile-compiler-only-tests in the VM
$STANZA compile-test build-stanza.proj tests/stanza.proj stz/stanza-postcompile-compiler-only-tests -o build/stanza-postcompile-compiler-only-tests -ccfiles tests/extern_c_callbacks.c
build/stanza-postcompile-compiler-only-tests

# Compile and run the tests in stz/stanza-postcompile-compiler-only-tests using a specially bootstrapped compiler
$STANZA extend tests/stanza.proj -supported-vm-packages stz/test-externs -o build/stanzatest -ccfiles tests/extern_c_callbacks.c
build/stanzatest run-test build-stanza.proj tests/stanza.proj stz/stanza-postcompile-compiler-only-tests
