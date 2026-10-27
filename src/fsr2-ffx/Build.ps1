# Builds ffx_fsr2_api_x64.dll (MSVC x64 Release).
#   powershell -NoProfile -File Build.ps1 -FfxSdk <Kits\FidelityFX of the FidelityFX SDK 2.3.0> -Fsr2Sdk <src\ffx-fsr2-api of FSR 2.2> [-Build <folder>]
param(
    [Parameter(Mandatory)][string]$FfxSdk,
    [Parameter(Mandatory)][string]$Fsr2Sdk,
    [string]$Build = ''
)
$ErrorActionPreference = 'Stop'
if (-not $Build) { $Build = Join-Path $PSScriptRoot 'build' }
$f = { param($p) ([IO.Path]::GetFullPath($p)).Replace('\', '/') }
& cmake -S $PSScriptRoot -B $Build -G 'Visual Studio 17 2022' -A x64 "-DFFX_SDK_DIR=$(& $f $FfxSdk)" "-DFSR2_SDK_DIR=$(& $f $Fsr2Sdk)"
if ($LASTEXITCODE -ne 0) { throw 'cmake (configure) failed' }
& cmake --build $Build --config Release --parallel
if ($LASTEXITCODE -ne 0) { throw 'cmake (build) failed' }
$dll = Join-Path $Build 'Release\ffx_fsr2_api_x64.dll'
"ffx_fsr2_api_x64.dll sha256 $((Get-FileHash -LiteralPath $dll -Algorithm SHA256).Hash)"
"output: $dll"
