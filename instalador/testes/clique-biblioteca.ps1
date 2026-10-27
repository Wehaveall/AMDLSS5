# Instalar e remover um jogo falso PELA JANELA da biblioteca: clique no cartao, Install, OK, Remove, Yes, OK; no fim a
# pasta do jogo tem de estar byte a byte como antes (Foto/Iguais).
#   powershell -NoProfile -File instalador\Testar-Instalador.ps1 -Manter        (a pasta de teste fica; anote o caminho)
#   powershell -NoProfile -File instalador\testes\clique-biblioteca.ps1 -Raiz <pasta de teste> [-Jogo 'RE T'] [-Exe <exe>]
# Codigo de saida 0 = passou.
param(
    [Parameter(Mandatory = $true)][string]$Raiz,
    [string]$Exe = '',   # vazio = ..\bin\AMDLSS5-Instalador.exe
    [string]$Jogo = 'RE T',
    [string]$Pasta = ''
)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'UiComum.ps1')
if (-not $Exe) { $Exe = Join-Path $PSScriptRoot '..\bin\AMDLSS5-Instalador.exe' }   # (no param, $PSScriptRoot fica vazio com [Parameter()] no PowerShell 5.1)
$Exe = (Resolve-Path -LiteralPath $Exe).Path
if (-not $Pasta) { $Pasta = Join-Path $Raiz "lojas\SteamT\steamapps\common\$Jogo" }

$antes = Foto $Pasta
$p = Abrir-BibliotecaTeste $Exe $Raiz
$ok = $false
try {
    $win = Esperar { Janelas $p.Id | Where-Object { $_.Current.Name -eq 'AMDLSS5 MOD - Game library' } | Select-Object -First 1 }
    $card = Esperar { Achar $win $Jogo }
    $h = [IntPtr]$card.Current.NativeWindowHandle
    $l = [IntPtr]((20 -shl 16) -bor 20)
    [AmdUi.W]::PostMessage($h, 0x201, [IntPtr]1, $l) | Out-Null; [AmdUi.W]::PostMessage($h, 0x202, [IntPtr]0, $l) | Out-Null   # clique no cartao
    $inst = Esperar { $b = Achar $win 'Install'; if ($b -and $b.Current.IsEnabled -and -not $b.Current.IsOffscreen) { $b } }
    "cartao clicado; Install habilitado"
    Clicar $inst
    $t = Responder $p.Id 'Installed' 'OK'
    "instalado: $t"
    $instalado = Foto $Pasta
    "arquivos depois de instalar: $($instalado.Count) (antes $($antes.Count))"
    $rem = Esperar { $b = Achar $win 'Remove'; if ($b -and $b.Current.IsEnabled) { $b } }
    $estado = (Esperar { $win.FindAll($TS::Descendants, [System.Windows.Automation.Condition]::TrueCondition) | Where-Object { $_.Current.Name -like 'Mod: Installed v*' } | Select-Object -First 1 }).Current.Name
    "painel: $estado"
    Clicar $rem
    $t = Responder $p.Id 'Remove' 'Yes,Sim,&Yes,&Sim'
    $t = Responder $p.Id 'Uninstalled' 'OK'
    "removido: $t"
    $estado = (Esperar { Achar $win 'Mod: Not installed' }).Current.Name
    "painel: $estado"
    $ok = $instalado.Count -gt $antes.Count -and (Iguais $antes (Foto $Pasta))
    "Iguais (Foto): $(Iguais $antes (Foto $Pasta))"
} finally { Fechar $p }
if ($ok) { 'RESULTADO: ok' } else { 'RESULTADO: FALHA'; exit 1 }
