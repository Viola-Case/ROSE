# Fetches and builds ROSE's dependencies into .vendor/, as pinned by
# dependencies.toml. Run `./vendor.ps1 --help` for the flags.
#
# All the logic lives in cmake/vendor.cmake so that this and vendor.sh cannot
# drift apart. A single file that is both valid sh and valid PowerShell is not
# achievable in general -- PowerShell parses a script in full before running it,
# so the sh half has to sit inside a <# ... #> block, and there is no way to open
# one on a line sh will also ignore -- so the shared part is CMake instead, which
# this project already requires.

$ErrorActionPreference = 'Stop'

if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
    Write-Error "cmake was not found on PATH. Install CMake 3.21 or newer."
    exit 1
}

& cmake -P (Join-Path $PSScriptRoot 'cmake/vendor.cmake') -- @args
exit $LASTEXITCODE
