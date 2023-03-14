# Assumes Stanza 0.17.56

stanza compile-macros \
  compiler/fastio-serializer-macros.stanza \
  compiler/renamer-lang.stanza \
  compiler/check-lang.stanza \
  compiler/ast-lang2.stanza \
  compiler/ast-lang.stanza \
  compiler/resolver-lang.stanza \
  compiler/message-lang.stanza \
  compiler/ast-printer-lang.stanza \
  compiler/reader-lang.stanza \
  -o bootstrap-macros 

stanza build-stanza.proj stz/driver -o stanzatemp -macros bootstrap-macros -flags BOOTSTRAP -optimize
