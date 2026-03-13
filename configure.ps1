$vsPath = & "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" `
    -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
    -property installationPath

$vcvars = Join-Path $vsPath 'VC\Auxiliary\Build\vcvars64.bat'
cmd /c "`"$vcvars`" && set" | ForEach-Object {
    if ($_ -match '^(.*?)=(.*)$') {
        if (-not [System.Environment]::GetEnvironmentVariable($matches[1])) {
            [System.Environment]::SetEnvironmentVariable($matches[1], $matches[2])
        }
    }
}

if (-not (Test-Path build)) { mkdir build }
Set-Content -Path "build/reconfigure.ps1" -Value "cd ..`r.\configure.ps1"
Set-Location build
cmake -G "Visual Studio 18 2026" -A x64 ..

if ($LASTEXITCODE -ne 0) {
    Write-Error "CMake configuration failed."
    exit 1
}

$choices = @(
    [System.Management.Automation.Host.ChoiceDescription]::new('&Yes', 'Open solution in Visual Studio')
    [System.Management.Automation.Host.ChoiceDescription]::new('&No', 'Terminate script')
)

$result = $host.UI.PromptForChoice('Open Solution', 'Open solution in Visual Studio?', $choices, 1)

if ($result -eq 0) {
    devenv .\ROSE.slnx
}
