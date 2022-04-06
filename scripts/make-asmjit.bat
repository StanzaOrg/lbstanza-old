cmake -G "MinGW Makefiles" -S libs\asmjit -B build\asmjit
cmake --build build\asmjit
move build\asmjit\libasmjit.a bin\libasmjit.a
del /Q /s build\asmjit
rd /Q /s build\asmjit
