#!/usr/bin/env bash

# USAGES:
# ./scripts/run-postcompile-tests.sh ./mystanza

STANZA="$1"
STANZA_COMPILER="$STANZA" $STANZA run-test tests/stanza.proj stz/stanza-postcompile-tests
STANZA_COMPILER="$STANZA" $STANZA compile-test build-stanza.proj tests/stanza.proj stz/stanza-postcompile-tests -o build/stanza-postcompile-tests
STANZA_COMPILER="$STANZA" ./build/stanza-postcompile-tests
