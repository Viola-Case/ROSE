$choices = @(
    [System.Management.Automation.Host.ChoiceDescription]::new('&Yes', 'Generate docs')
    [System.Management.Automation.Host.ChoiceDescription]::new('&No', 'Do not')
)

$result = $host.UI.PromptForChoice('Generate Docs', 'Generate API documentation via Doxygen?', $choices, 1)

if ($result -eq 0) {
    $err_result = & doxygen ./api.Doxyfile 2>&1

    if ($LASTEXITCODE -ne 0) {
        Write-Error "API docs generation failed: $err_result"
    } else {
        Write-Output "API docs generated"
    }
}

if (-not $env:VCPKG_ROOT) {
    $vcpkgCmd = Get-Command vcpkg -ErrorAction SilentlyContinue
    if ($vcpkgCmd) {
        $env:VCPKG_ROOT = Split-Path $vcpkgCmd.Source -Parent
    } else {
        $candidates = @(
            "C:\vcpkg",
            "C:\src\vcpkg",
            "$env:USERPROFILE\vcpkg",
            "$env:LOCALAPPDATA\vcpkg"
        )
        foreach ($path in $candidates) {
            if (Test-Path "$path\vcpkg.exe") {
                $env:VCPKG_ROOT = $path
                break
            }
        }
    }
}

if ($env:VCPKG_ROOT) {
    Write-Output "Using vcpkg at: $env:VCPKG_ROOT"
    $vcpkgFlag = "-DUSE_VCPKG=ON"
} else {
    Write-Warning "vcpkg not found; building without it"
    $vcpkgFlag = "-DUSE_VCPKG=OFF"
    if (!$sdl3Root)      { $sdl3Root      = Read-Host "SDL3 path" }
    if (!$imguiRoot)     { $imguiRoot     = Read-Host "Dear ImGui path" }
    if (!$gladRoot)      { $gladRoot      = Read-Host "GLAD path" }
    if (!$glslangRoot)   { $glslangRoot   = Read-Host "glslang path" }
    if (!$sdl3ImageRoot) { $sdl3ImageRoot = Read-Host "SDL3_image path" }
    if (!$sdl3TtfRoot)   { $sdl3TtfRoot   = Read-Host "SDL3_ttf path" }
    if (!$sdl3MixerRoot) { $sdl3MixerRoot = Read-Host "SDL3_mixer path" }
    if (!$spirvCrossRoot){ $spirvCrossRoot = Read-Host "SPIRV-Cross path" }

    $prefixPaths = @($sdl3Root, $imguiRoot, $gladRoot, $glslangRoot,
                     $sdl3ImageRoot, $sdl3TtfRoot, $sdl3MixerRoot, $spirvCrossRoot) |
                   Where-Object { $_ } |
                   ForEach-Object { $_.TrimEnd('\', '/') }
    $prefixFlag = if ($prefixPaths) { "-DCMAKE_PREFIX_PATH=`"$($prefixPaths -join ';')`"" } else { "" }
}

$root = $PSScriptRoot
$cmakeCommon = @($vcpkgFlag, $prefixFlag,
    "-DCMAKE_MAKE_PROGRAM=ninja", "-DCMAKE_C_COMPILER=clang", "-DCMAKE_CXX_COMPILER=clang++") |
    Where-Object { $_ }

if (-not (Test-Path "$root\build")) { mkdir "$root\build" }
Set-Content -Path "$root\build\reconfigure.ps1" -Value "Set-Location ..`r& .\configure.ps1"
Push-Location "$root\build"
cmake -G Ninja @cmakeCommon -DCMAKE_BUILD_TYPE=Release ..
if ($LASTEXITCODE -ne 0) { Write-Error "Release configuration failed."; Pop-Location; exit 1 }
Pop-Location

$editorChoice = $host.UI.PromptForChoice(
    'Editor Config',
    'Also configure an Editor build in build_editor/?',
    @(
        [System.Management.Automation.Host.ChoiceDescription]::new('&Yes', 'Configure Editor build')
        [System.Management.Automation.Host.ChoiceDescription]::new('&No',  'Skip')
    ), 1)

if ($editorChoice -eq 0) {
    if (-not (Test-Path "$root\build_editor")) { mkdir "$root\build_editor" }
    Push-Location "$root\build_editor"
    cmake -G Ninja @cmakeCommon -DCMAKE_BUILD_TYPE=Editor ..
    if ($LASTEXITCODE -ne 0) { Write-Error "Editor configuration failed."; Pop-Location; exit 1 }
    Pop-Location
}