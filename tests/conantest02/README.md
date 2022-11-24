
# Overview
This repo will do the following:
- Download and install stanza in the current directory
- Use conan to download a pre-compiled static library of asmjit
- Use stanza to compile the stanza-asmjit interface code written in C++
- Use stanza to compile the asmjit-app test program written in stanza
- Use stanza to link the results into an executable

# Prerequisites
Already have curl, conan, cmake, gcc installed
No need to have stanza installed

# Build

```
make stanza conan asmjit-app
```

# Run output

```
build/asmjit-app
```

# Expected output

Calls a simple function in the library and outputs the result

```
RES = 67
```