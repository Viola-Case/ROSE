#!/bin/sh
#
# Fetches and builds ROSE's dependencies into .vendor/, as pinned by
# dependencies.toml. Run `./vendor.sh --help` for the flags.
#
# All the logic lives in cmake/vendor.cmake so that this and vendor.ps1 cannot
# drift apart. See the comment in vendor.ps1 for why this is two files.

set -eu

if ! command -v cmake >/dev/null 2>&1; then
    echo "vendor: cmake was not found on PATH. Install CMake 3.21 or newer." >&2
    exit 1
fi

root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)

exec cmake -P "$root/cmake/vendor.cmake" -- "$@"
