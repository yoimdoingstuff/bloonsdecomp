#!/usr/bin/env bash
set -euo pipefail

if [[ $# != 3 ]]; then
    echo 'Usage: bash ci_build.sh ROCKBOX_SOURCE BUILD_DIRECTORY ipodvideo|ipod6g' >&2
    exit 2
fi
source_dir=$(realpath "$1")
build_dir=$(realpath -m "$2")
target=$3
case "$target" in ipodvideo|ipod6g) ;; *) echo 'Unsupported iPod target' >&2; exit 2 ;; esac
test -f "$source_dir/apps/plugins/bloons/bloons.make"
mkdir -p "$build_dir/toolchain-shim"

# The pinned configure script expects arm-elf-eabi-* names. Ubuntu's ARM
# bare-metal compiler uses arm-none-eabi-*; both use EABI. We retain every
# target CPU/ABI flag selected by Rockbox and use that compiler for C and C++.
for tool in gcc g++ ar as ld nm objcopy objdump ranlib strip size; do
    executable=$(command -v "arm-none-eabi-$tool")
    ln -sf "$executable" "$build_dir/toolchain-shim/arm-elf-eabi-$tool"
done
export PATH="$build_dir/toolchain-shim:$PATH"
cd "$build_dir"
bash "$source_dir/tools/configure" --target="$target" --type=n --no-ccache </dev/null

# Build normal generated headers/libraries, but only our plugin. The compiler's
# own CPU/mode-selected libm is picked by bloons.make rather than a guessed path.
make -j2 SELECTED_PLUGINS_SRC= SELECTED_PLUGINS_SUBDIRS="$source_dir/apps/plugins/bloons" \
    BLOONS_CXX=arm-none-eabi-g++
test -s apps/plugins/bloons/bloons.rock
