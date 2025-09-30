#!/bin/bash
set -e
SRC_FILE="braindead.c"
GCC_DIR="gcc"
mkdir -p "$GCC_DIR"
CLANG_DIR="clang"
mkdir -p "$CLANG_DIR"
gcc -O2 "$SRC_FILE" -o "$GCC_DIR/braindead"
gcc -g "$SRC_FILE" -o "$GCC_DIR/braindead_debug"
gcc -fsanitize=address -g "$SRC_FILE" -o "$GCC_DIR/braindead_asan"
gcc -Os "$SRC_FILE" -o "$GCC_DIR/braindead_small"
if gcc -m32 -v &>/dev/null; then
    gcc -m32 "$SRC_FILE" -o "$GCC_DIR/braindead_x86"
fi
if command -v aarch64-linux-gnu-gcc &>/dev/null; then
    aarch64-linux-gnu-gcc -O2 "$SRC_FILE" -o "$GCC_DIR/braindead_arm64"
fi
clang -O2 "$SRC_FILE" -o "$CLANG_DIR/braindead"
clang -g "$SRC_FILE" -o "$CLANG_DIR/braindead_debug"
clang -fsanitize=address -g "$SRC_FILE" -o "$CLANG_DIR/braindead_asan"
clang -Os "$SRC_FILE" -o "$CLANG_DIR/braindead_small"
if clang -m32 -v &>/dev/null; then
    clang -m32 "$SRC_FILE" -o "$CLANG_DIR/braindead_x86"
fi
