# "Adicionar jogo manualmente" (a janela --pasta) PELA JANELA: o jogo falso com BEClient ao lado do exe (3 pastas abaixo)
# nao instala: Install mostra o erro do anticheat e a pasta do jogo fica como estava.
#   powershell -NoProfile -File instalador\Testar-Instalador.ps1 -Manter        (a pasta de teste fica; anote o caminho)
#   powershell -NoProfile -File instalador\testes\clique-manual-anticheat.ps1 -Raiz <pasta de teste> [-Exe <exe>]
# Codigo de saida 0 = passou.
param(
    [Parameter(Mandatory = $true)][string]$Raiz,
    [string]$Exe = ''   # vazio = ..\bin\AMDLSS5-Instalador.exe
)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'UiComum.ps1')
if (-not $Exe) { $Exe = Join-Path $PSScriptRoot '..\bin\AMDLSS5-Instalador.exe' }   # (no param, $PSScriptRoot fica vazio com [Parameter()] no PowerShell 5.1)
$Exe = (Resolve-Path -LiteralPath $Exe).Path
$pasta = Join-Path $Raiz 'lojas\SteamAC\steamapps\common\AC Fundo T'

$antes = Foto $pasta
$p = Start-Process -FilePath $Exe -ArgumentList @('--pasta', ('"' + $pasta + '"'), '--payload', ('"' + (Join-Path $Raiz 'payload-biblioteca') + '"'), '--idioma', 'en') -PassThru
$ok = $false
try {
    $win = Esperar { Janelas $p.Id | Where-Object { $_.Current.Name -eq 'AMDLSS5 MOD - Installer' } | Select-Object -First 1 }
    $inst = Esperar { $b = Achar $win 'Install'; if ($b -and $b.Current.IsEnabled) { $b } }
    Clicar $inst
    $t = Responder $p.Id 'Error' 'OK'
    "erro: $t"
    Start-Sleep 1
    $intacto = Iguais $antes (Foto $pasta)
    "pasta intacta: $intacto"
    $ok = $t -match 'anti-cheat' -and $intacto
} finally { Fechar $p }
if ($ok) { 'RESULTADO: ok' } else { 'RESULTADO: FALHA'; exit 1 }
