#!/bin/sh
#
# One-time setup for a fresh working copy: the things the build does not do for
# you and git cannot carry, because they live in gitignored directories.
# Run `./init.sh --help` for the tasks and flags.
#
# All the logic lives in cmake/init.cmake so that this and init.ps1 cannot drift
# apart. See the comment in vendor.ps1 for why this is two files.

set -eu

if ! command -v cmake >/dev/null 2>&1; then
    echo "init: cmake was not found on PATH. Install CMake 3.21 or newer." >&2
    exit 1
fi

root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)

exec cmake -P "$root/cmake/init.cmake" -- "$@"
