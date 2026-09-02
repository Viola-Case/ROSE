# One-time setup for a fresh working copy: the things the build does not do for
# you and git cannot carry, because they live in gitignored directories.
# Run `./init.ps1 --help` for the tasks and flags.
#
# All the logic lives in cmake/init.cmake so that this and init.sh cannot drift
# apart -- the same arrangement, and for the same reason, as vendor.ps1.

$ErrorActionPreference = 'Stop'

if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
    Write-Error "cmake was not found on PATH. Install CMake 3.21 or newer."
    exit 1
}

& cmake -P (Join-Path $PSScriptRoot 'cmake/init.cmake') -- @args
exit $LASTEXITCODE
