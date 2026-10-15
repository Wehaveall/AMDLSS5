# Builds the AMDLSS5 helper (nr_native_adapter.dll + colorcs/motioncs/depthcs.cso).
#   powershell -NoProfile -File Build-Helper.ps1 -Dxc <dxc.exe> [-HipInclude <folder>] [-DxcReprojecao <dxc.exe>] [-Saida <folder>]
# -Dxc           dxc.exe for the scaling shaders (the project uses the one from Radeon GPU Analyzer 2.14.2)
# -DxcReprojecao dxc.exe for the reprojection shaders (default: the one from Windows SDK 10.0.26100)
# -HipInclude    folder with hip/hip_runtime_api.h (default: $env:HIP_PATH\include)
param(
    [Parameter(Mandatory)][string]$Dxc,
    [string]$HipInclude = '',
    [string]$DxcReprojecao = 'C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64\dxc.exe',
    [string]$Saida = '',
    [string]$Build = ''
)
$ErrorActionPreference = 'Stop'
$raiz = $PSScriptRoot
if (-not $HipInclude) { $HipInclude = Join-Path $env:HIP_PATH 'include' }
if (-not (Test-Path -LiteralPath (Join-Path $HipInclude 'hip\hip_runtime_api.h'))) { throw "HIP headers not found in $HipInclude" }
foreach ($d in @($Dxc, $DxcReprojecao)) { if (-not (Test-Path -LiteralPath $d)) { throw "dxc not found: $d" } }
if (-not $Build) { $Build = Join-Path $raiz 'build' }
if (-not $Saida) { $Saida = Join-Path $raiz 'build\saida' }
$f = { param($p) ([IO.Path]::GetFullPath($p)).Replace('\', '/') }
& cmake -S $raiz -B $Build -G 'Visual Studio 17 2022' -A x64 "-DNR_SCALE_DXC=$(& $f $Dxc)" "-DNR_REPROJECT_DXC=$(& $f $DxcReprojecao)" "-DNR_HIP_INCLUDE_DIR=$(& $f $HipInclude)"
if ($LASTEXITCODE -ne 0) { throw 'cmake (configure) failed' }
& cmake --build $Build --config Release --target nr_live_adapter --parallel
if ($LASTEXITCODE -ne 0) { throw 'cmake (build) failed' }
New-Item -ItemType Directory -Force $Saida | Out-Null
foreach ($n in 'nr_native_adapter.dll', 'colorcs.cso', 'motioncs.cso', 'depthcs.cso') {
    Copy-Item -LiteralPath (Join-Path $Build "helper\Release\$n") -Destination (Join-Path $Saida $n) -Force
}
"nr_native_adapter.dll sha256 $((Get-FileHash -LiteralPath (Join-Path $Saida 'nr_native_adapter.dll') -Algorithm SHA256).Hash)"
"output: $Saida"
