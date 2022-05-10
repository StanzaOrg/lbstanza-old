# Set early failure flags
set -e
set -o pipefail

# Sanity check: Ensure we are calling
# this script with the right number of arguments.
if [ $# -lt 1 ]; then
    echo "Not enough arguments"
    exit 2
fi

# Set OPTION as the first argument.
OPTION="$1"

# Compute FILENAME based upon setting of OPTION.
case "$OPTION" in
    os-x)
        FILENAME="libasmjit-os-x.a" ;;
    linux)
        # gcc needs explicit flag for position-independent code.
        export CXXFLAGS="-fPIC"  
        FILENAME="libasmjit-linux.a" ;;
    current)
        FILENAME="libasmjit.a" ;;
    *) cat 1>&2 <<EOF
Error: unsupported option.
EOF
esac

# Call CMake to build AsmJIT
cmake -S libs/asmjit -B build/asmjit
cmake --build build/asmjit

# Move the built artifact to the right place with
# the right filename.
mv build/asmjit/libasmjit.a bin/$FILENAME

# Remove the old build folder
rm -r build/asmjit
