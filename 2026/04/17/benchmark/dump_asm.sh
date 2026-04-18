#!/usr/bin/env bash
set -euo pipefail

INCLUDE="$(dirname "$0")/include"
TMPDIR_LOCAL="$(mktemp -d)"
trap 'rm -rf "$TMPDIR_LOCAL"' EXIT

FLAGS="-std=c++23 -O3 -march=armv8-a+sve2 -msve-vector-bits=128
       -I$INCLUDE -fno-exceptions -fno-rtti"

# ── Tiny translation units that force instantiation ──────────────────────────

cat > "$TMPDIR_LOCAL/asm_neon.cpp" <<'EOF'
#include <cstdint>
#include "neon.h"
// Force the symbol to be emitted with an external, predictable name.
extern "C" __attribute__((noinline, used))
void neon_classify_asm(const uint8_t* p, uint64_t* ws, uint64_t* op) {
    auto [w, o] = neon::classify(p);
    *ws = w; *op = o;
}
EOF

cat > "$TMPDIR_LOCAL/asm_sve.cpp" <<'EOF'
#include <cstdint>
#include "sve.h"
extern "C" __attribute__((noinline, used))
void sve_classify_asm(const uint8_t* p, uint64_t* op, uint64_t* ws) {
    auto [o, w] = sve::classify(p);
    *op = o; *ws = w;
}
EOF

# ── Compile to object files ───────────────────────────────────────────────────
echo "Compiling neon::classify ..."
c++ $FLAGS -c "$TMPDIR_LOCAL/asm_neon.cpp" -o "$TMPDIR_LOCAL/neon.o"

echo "Compiling sve::classify ..."
c++ $FLAGS -c "$TMPDIR_LOCAL/asm_sve.cpp"  -o "$TMPDIR_LOCAL/sve.o"

# ── Disassemble: extract one function from GNU objdump output ────────────────
# Usage: disasm_fn <object_file> <symbol_name>
disasm_fn() {
    local obj="$1" sym="$2"
    local addr
    addr=$(nm --defined-only "$obj" | awk -v s="$sym" '$3 == s { print $1; exit }')
    if [[ -z "$addr" ]]; then
        echo "Symbol '$sym' not found in $obj" >&2
        return 1
    fi
    local body
    body=$(objdump -d --no-show-raw-insn "$obj" \
        | awk -v sym="$sym" '
            /^[[:xdigit:]]+ <'"$sym"'>:/ { found=1 }
            found { print }
            found && /^$/ { exit }
        ')
    echo "$body"
    # Count instruction lines: lines that start with whitespace followed by a hex address
    local count
    count=$(echo "$body" | grep -cE '^\s+[0-9a-f]+:' || true)
    echo "  → ${count} instructions"
}

echo
echo "════════════════════════════════════════════════════════════════"
echo "  neon::classify  (neon_classify_asm)"
echo "════════════════════════════════════════════════════════════════"
disasm_fn "$TMPDIR_LOCAL/neon.o" neon_classify_asm

echo
echo "════════════════════════════════════════════════════════════════"
echo "  sve::classify   (sve_classify_asm)"
echo "════════════════════════════════════════════════════════════════"
disasm_fn "$TMPDIR_LOCAL/sve.o" sve_classify_asm
