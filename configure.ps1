# Configures ROSE's build trees.
#
# Dependencies come from .vendor/, built by vendor.ps1 from the pinned tags in
# dependencies.toml. Pass -Vcpkg to use vcpkg instead; that path needs
# VCPKG_ROOT and reads vcpkg.json.

[CmdletBinding()]
param(
    # Configurations to configure. Any of Release, Debug, Editor.
    [ValidateSet('Release', 'Debug', 'Editor')]
    [string[]] $Config = @('Release', 'Editor'),

    # Use vcpkg rather than the vendored tree.
    [switch] $Vcpkg,

    # Skip the dependency step and configure against whatever is already built.
    [switch] $SkipVendor,

    # Generate API documentation without being asked.
    [switch] $Docs,

    # Skip documentation without being asked. Without either switch the script
    # asks, unless it is running non-interactively (CI, a piped shell), where
    # there is nobody to answer and the safe default is to skip.
    [switch] $NoDocs
)

$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot

# ---------------------------------------------------------------------- docs
$generateDocs = $false
if ($Docs) {
    $generateDocs = $true
} elseif (-not $NoDocs -and [Environment]::UserInteractive) {
    $choices = @(
        [System.Management.Automation.Host.ChoiceDescription]::new('&Yes', 'Generate docs')
        [System.Management.Automation.Host.ChoiceDescription]::new('&No', 'Do not')
    )
    $generateDocs =
        $host.UI.PromptForChoice('Generate Docs',
            'Generate API documentation via Doxygen?', $choices, 1) -eq 0
}

if ($generateDocs) {
    $err = & doxygen "$root/api.Doxyfile" 2>&1
    if ($LASTEXITCODE -ne 0) {
        Write-Error "API docs generation failed: $err"
    } else {
        Write-Output 'API docs generated'
    }
}

# -------------------------------------------------------------- dependencies
if ($Vcpkg) {
    if (-not $env:VCPKG_ROOT) {
        # The presets reference $env:VCPKG_ROOT directly, so it has to be set
        # before cmake runs rather than passed on the command line.
        $vcpkgCmd = Get-Command vcpkg -ErrorAction SilentlyContinue
        if ($vcpkgCmd) {
            $env:VCPKG_ROOT = Split-Path $vcpkgCmd.Source -Parent
        } else {
            foreach ($candidate in @(
                    'C:\vcpkg',
                    'C:\src\vcpkg',
                    "$env:USERPROFILE\vcpkg",
                    "$env:LOCALAPPDATA\vcpkg")) {
                if (Test-Path "$candidate\vcpkg.exe") {
                    $env:VCPKG_ROOT = $candidate
                    break
                }
            }
        }
    }

    if (-not $env:VCPKG_ROOT) {
        Write-Error 'vcpkg not found. Set VCPKG_ROOT, or drop -Vcpkg to use .vendor.'
        exit 1
    }
    Write-Output "Using vcpkg at: $env:VCPKG_ROOT"
    $suffix = '-vcpkg'
} else {
    $suffix = ''
    if (-not $SkipVendor) {
        Write-Output 'Checking dependencies...'
        & "$root/vendor.ps1"
        if ($LASTEXITCODE -ne 0) {
            Write-Error 'Dependency vendoring failed.'
            exit 1
        }
    }
}

# ------------------------------------------------------------------ configure
foreach ($c in $Config) {
    $preset = "$($c.ToLower())$suffix"
    Write-Output ''
    Write-Output "Configuring preset '$preset'..."
    & cmake --preset $preset
    if ($LASTEXITCODE -ne 0) {
        Write-Error "Configuration of '$preset' failed."
        exit 1
    }
}

Write-Output ''
Write-Output 'Done. Build with, for example:'
Write-Output "    cmake --build --preset $($Config[0].ToLower())$suffix"
