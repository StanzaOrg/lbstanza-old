#include <stanza.h>
//Simple sanity-check function to test successful connection to macro plugin.
STANZA_API_FUNC uint32_t handshake () {
  return 0xcafebabe;
}
