# Troca de idioma PELA JANELA da biblioteca: com o jogo do anticheat escolhido, ingles -> portugues no seletor; o titulo,
# o motivo e a contagem mudam de idioma e o cartao continua escolhido.
#   powershell -NoProfile -File instalador\Testar-Instalador.ps1 -Manter        (a pasta de teste fica; anote o caminho)
#   powershell -NoProfile -File instalador\testes\clique-idioma.ps1 -Raiz <pasta de teste> [-Exe <exe>]
# Codigo de saida 0 = passou.
param(
    [Parameter(Mandatory = $true)][string]$Raiz,
    [string]$Exe = ''   # vazio = ..\bin\AMDLSS5-Instalador.exe
)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'UiComum.ps1')
if (-not $Exe) { $Exe = Join-Path $PSScriptRoot '..\bin\AMDLSS5-Instalador.exe' }   # (no param, $PSScriptRoot fica vazio com [Parameter()] no PowerShell 5.1)
$Exe = (Resolve-Path -LiteralPath $Exe).Path

$p = Abrir-BibliotecaTeste $Exe $Raiz
$ok = $false
try {
    $win = Esperar { Janelas $p.Id | Where-Object { $_.Current.Name -eq 'AMDLSS5 MOD - Game library' } | Select-Object -First 1 }
    $card = Esperar { Achar $win 'Anticheat T' }; Clicar $card
    Esperar { Achar $win 'This game uses anti-cheat: the mod is not installed.' } | Out-Null
    $cmb = Achar $win 'English'
    [AmdUi.W]::PostMessage([IntPtr]$cmb.Current.NativeWindowHandle, 0x100, [IntPtr]0x28, [IntPtr]0x01500001) | Out-Null   # seta para baixo: Portugues
    $win2 = Esperar { Janelas $p.Id | Where-Object { $_.Current.Name -like 'AMDLSS5 MOD - Biblioteca*' } | Select-Object -First 1 }
    "titulo: $($win2.Current.Name)"
    $m = Esperar { $win2.FindAll($TS::Descendants, [System.Windows.Automation.Condition]::TrueCondition) | Where-Object { $_.Current.Name -like '*anticheat: o mod*' } | Select-Object -First 1 }
    "motivo traduzido e cartao mantido: $($m.Current.Name)"
    $s = Esperar { $win2.FindAll($TS::Descendants, [System.Windows.Automation.Condition]::TrueCondition) | Where-Object { $_.Current.Name -like '* jogo*encontrado*' } | Select-Object -First 1 }
    "status: $($s.Current.Name)"
    $ok = $true
} finally { Fechar $p }
"saiu: $($p.HasExited) codigo: $($p.ExitCode)"
if ($ok -and $p.HasExited) { 'RESULTADO: ok' } else { 'RESULTADO: FALHA'; exit 1 }
