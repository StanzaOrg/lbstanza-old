#!/usr/bin/env bash

# USAGES:
# ./scripts/run-postcompile-tests.sh ./mystanza

STANZA="$1"
STANZA_COMPILER="$STANZA" $STANZA run-test tests/stanza.proj stz/stanza-postcompile-tests
