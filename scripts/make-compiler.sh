# USAGES:
# ./make-compiler
# ./make-compiler bin/stanzadev
# ./make-compiler ./stanzac bin/stanzadev

if [ $# -eq 0 ]
then
    STANZA=stanza
    OUT=./bin/stanzadev
elif [ $# -eq 1 ]
then
    STANZA=stanza
    OUT=$1
elif [ $# -eq 2 ]
then
    STANZA=$1
    OUT=$2
fi

mkdir -p bin

$STANZA \
  core/core.stanza \
  core/collections.stanza \
  core/reader.stanza \
  core/parser.stanza \
  core/macro-utils.stanza \
  compiler/line-noise-prompter.stanza \
  compiler/stz-algorithms.stanza \
  compiler/stz-utils.stanza \
  compiler/stz-stable-arrays.stanza \
  compiler/stz-serializer.stanza \
  compiler/stz-params.stanza \
  compiler/stz-core-macros.stanza \
  compiler/stz-ids.stanza \
  compiler/lang-read.stanza \
  compiler/lang-check.stanza \
  compiler/stz-primitives.stanza \
  compiler/stz-il-ir.stanza \
  compiler/stz-tl-ir.stanza \
  compiler/stz-input.stanza \
  compiler/stz-visibility.stanza \
  compiler/stz-basic-ops.stanza \
  compiler/stz-namemap.stanza \
  compiler/stz-renamer.stanza \
  compiler/stz-resolver.stanza \
  compiler/stz-infer.stanza \
  compiler/stz-type-calculus.stanza \
  compiler/stz-type.stanza \
  compiler/stz-tl-to-el.stanza \
  compiler/stz-tl-to-dl.stanza \
  compiler/stz-el-ir.stanza \
  compiler/stz-dl-ir.stanza \
  compiler/stz-vm-ir.stanza \
  compiler/stz-typeset.stanza \
  compiler/stz-dl.stanza \
  compiler/stz-el.stanza \
  compiler/stz-el-infer.stanza \
  compiler/stz-basic-blocks.stanza \
  compiler/stz-ehier.stanza \
  compiler/stz-vm.stanza \
  compiler/stz-bindings.stanza \
  compiler/stz-bindings-to-vm.stanza \
  compiler/stz-vm-encoder.stanza \
  compiler/stz-trie.stanza \
  compiler/stz-trie-table.stanza \
  compiler/stz-hash.stanza \
  compiler/stz-keyed-set.stanza \
  compiler/stz-dyn-tree.stanza \
  compiler/stz-dyn-graph.stanza \
  compiler/stz-dyn-bi-table.stanza \
  compiler/stz-vm-table.stanza \
  compiler/stz-branch-table.stanza \
  compiler/stz-vm-analyze.stanza \
  compiler/stz-vm-bindings.stanza \
  compiler/stz-vm-ids.stanza \
  compiler/stz-el-to-vm.stanza \
  compiler/stz-vm-normalize.stanza \
  compiler/stz-compiler.stanza \
  compiler/stz-arg-parser.stanza \
  compiler/stz-langs.stanza \
  compiler/lang-renamer.stanza \
  compiler/lang-resolver.stanza \
  compiler/lang-serializer.stanza \
  compiler/stz-codegen2.stanza \
  compiler/stz-reg-alloc2.stanza \
  compiler/stz-stitcher2.stanza \
  compiler/stz-const-pool.stanza \
  compiler/stz-data-pool.stanza \
  compiler/stz-padder.stanza \
  compiler/stz-build.stanza \
  compiler/stz-repl.stanza \
  compiler/stz-config.stanza \
  compiler/stz-asm-ir2.stanza \
  compiler/stz-asm-emitter.stanza \
  compiler/stz-pkg.stanza \
  compiler/stz-backend.stanza \
  compiler/stz-gen-bindings.stanza \
  compiler/stz-dependencies.stanza \
  compiler/call-records.stanza \
  compiler/stz-main.stanza \
  compiler/stz-driver.stanza \
  -ccfiles \
  runtime/linenoise.c \
  compiler/cvm.c \
  -s $OUT.s \
  -o $OUT \

