cmake -S libs/asmjit -B build/asmjit
cmake --build build/asmjit
mv build/asmjit/libasmjit.a bin/libasmjit.a
rm -r build/asmjit
