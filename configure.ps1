if (-not (Test-Path build)) { mkdir build }
Set-Content -Path "build/reconfigure.ps1" -Value "cd ..`r.\configure.ps1"
Set-Location build
cmake -G "Visual Studio 18 2026" -A x64 ..