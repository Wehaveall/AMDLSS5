# Baixa os headers publicos do XeSS 1.3.1 (intel/xess, tag v1.3.1) para third_party\xess-1.3.1.
# Ficam fora do git: a licenca e a da Intel (licenses/LICENSE.pdf), nao MIT. O 1.3.1 e um superconjunto do
# 1.0-1.2: os structs do D3D12 sao iguais e so entram funcoes e presets novos.
$ErrorActionPreference = 'Stop'
$destino = Join-Path $PSScriptRoot 'third_party\xess-1.3.1'
New-Item -ItemType Directory -Force (Join-Path $destino 'xess') | Out-Null
$base = 'https://raw.githubusercontent.com/intel/xess/v1.3.1'
foreach ($h in 'xess.h', 'xess_d3d12.h', 'xess_debug.h', 'xess_d3d12_debug.h') {
    Invoke-WebRequest -UseBasicParsing "$base/inc/xess/$h" -OutFile (Join-Path $destino "xess\$h")
}
Invoke-WebRequest -UseBasicParsing "$base/licenses/LICENSE.pdf" -OutFile (Join-Path $destino 'LICENSE.pdf')
Write-Host "headers do XeSS 1.3.1 em $destino"
