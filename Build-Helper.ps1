# Compila o helper do AMDLSS5 (nr_native_adapter.dll + colorcs/motioncs/depthcs.cso).
#   powershell -NoProfile -File Build-Helper.ps1 -Dxc <dxc.exe> [-HipInclude <pasta>] [-DxcReprojecao <dxc.exe>] [-Saida <pasta>]
# -Dxc           dxc.exe dos shaders de escala (o projeto usa o do Radeon GPU Analyzer 2.14.2)
# -DxcReprojecao dxc.exe dos shaders da reprojecao (padrao: o do Windows SDK 10.0.26100)
# -HipInclude    pasta com hip/hip_runtime_api.h (padrao: $env:HIP_PATH\include)
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
if (-not (Test-Path -LiteralPath (Join-Path $HipInclude 'hip\hip_runtime_api.h'))) { throw "headers do HIP nao encontrados em $HipInclude" }
foreach ($d in @($Dxc, $DxcReprojecao)) { if (-not (Test-Path -LiteralPath $d)) { throw "dxc nao encontrado: $d" } }
if (-not $Build) { $Build = Join-Path $raiz 'build' }
if (-not $Saida) { $Saida = Join-Path $raiz 'build\saida' }
$f = { param($p) ([IO.Path]::GetFullPath($p)).Replace('\', '/') }
& cmake -S $raiz -B $Build -G 'Visual Studio 17 2022' -A x64 "-DNR_SCALE_DXC=$(& $f $Dxc)" "-DNR_REPROJECT_DXC=$(& $f $DxcReprojecao)" "-DNR_HIP_INCLUDE_DIR=$(& $f $HipInclude)"
if ($LASTEXITCODE -ne 0) { throw 'cmake (configuracao) falhou' }
& cmake --build $Build --config Release --target nr_live_adapter --parallel
if ($LASTEXITCODE -ne 0) { throw 'cmake (build) falhou' }
New-Item -ItemType Directory -Force $Saida | Out-Null
foreach ($n in 'nr_native_adapter.dll', 'colorcs.cso', 'motioncs.cso', 'depthcs.cso') {
    Copy-Item -LiteralPath (Join-Path $Build "helper\Release\$n") -Destination (Join-Path $Saida $n) -Force
}
"nr_native_adapter.dll sha256 $((Get-FileHash -LiteralPath (Join-Path $Saida 'nr_native_adapter.dll') -Algorithm SHA256).Hash)"
"saida: $Saida"
