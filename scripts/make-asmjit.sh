HERE="$(dirname "${BASH_SOURCE[0]}")"

set -e
set -o pipefail

if [ $# -lt 1 ]; then
    echo "Not enough arguments"
    exit 2
fi

OPTION="$1"

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

cmake -S libs/asmjit -B build/asmjit
cmake --build build/asmjit
mv build/asmjit/libasmjit.a bin/$FILENAME
rm -r build/asmjit
