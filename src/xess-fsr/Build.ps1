# Builds libxess.dll (MSVC x64 Release). Downloads the XeSS 1.3.1 headers first if they are missing (Baixar-Headers.ps1).
#   powershell -NoProfile -File Build.ps1 -FfxSdk <Kits\FidelityFX of the FidelityFX SDK 2.3.0> [-Build <folder>]
param(
    [Parameter(Mandatory)][string]$FfxSdk,
    [string]$Build = ''
)
$ErrorActionPreference = 'Stop'
if (-not (Test-Path (Join-Path $PSScriptRoot 'third_party\xess-1.3.1\xess\xess_d3d12.h'))) {
    & (Join-Path $PSScriptRoot 'Baixar-Headers.ps1')
}
if (-not $Build) { $Build = Join-Path $PSScriptRoot 'build' }
$f = { param($p) ([IO.Path]::GetFullPath($p)).Replace('\', '/') }
& cmake -S $PSScriptRoot -B $Build -G 'Visual Studio 17 2022' -A x64 "-DFFX_SDK_DIR=$(& $f $FfxSdk)"
if ($LASTEXITCODE -ne 0) { throw 'cmake (configure) failed' }
& cmake --build $Build --config Release --parallel
if ($LASTEXITCODE -ne 0) { throw 'cmake (build) failed' }
$dll = Join-Path $Build 'Release\libxess.dll'
"libxess.dll sha256 $((Get-FileHash -LiteralPath $dll -Algorithm SHA256).Hash)"
"output: $dll"
