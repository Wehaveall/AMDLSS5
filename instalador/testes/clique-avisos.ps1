# Avisos da biblioteca PELA JANELA: o jogo com anticheat nao tem Install e mostra o motivo; o jogo "nao testado" com
# versao velha mostra "Update", o clique pede confirmacao e o "Nao" nao toca na pasta.
#   powershell -NoProfile -File instalador\Testar-Instalador.ps1 -Manter        (a pasta de teste fica; anote o caminho)
#   powershell -NoProfile -File instalador\testes\clique-avisos.ps1 -Raiz <pasta de teste> [-Exe <exe>]
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
function ClicarCartao($win, $nome) { $card = Esperar { Achar $win $nome }; Clicar $card; Start-Sleep -Milliseconds 500 }
$ok = $false
try {
    $win = Esperar { Janelas $p.Id | Where-Object { $_.Current.Name -eq 'AMDLSS5 MOD - Game library' } | Select-Object -First 1 }
    ClicarCartao $win 'Anticheat T'
    $b = Achar $win 'Install'
    $semInstall = -not ($b -and -not $b.Current.IsOffscreen)
    "Anticheat: Install visivel=$(-not $semInstall); motivo: $((Esperar { Achar $win 'This game uses anti-cheat: the mod is not installed.' }).Current.Name)"
    $pastaDemo = Join-Path $Raiz 'lojas\SteamT\steamapps\common\Demo T'
    $fotoDemo = Foto $pastaDemo
    ClicarCartao $win 'Demo T'
    "Demo: $((Esperar { Achar $win 'Mod: Installed: v0.9. The package has vteste: click Update.' }).Current.Name)"
    $up = Esperar { $b = Achar $win 'Update'; if ($b -and $b.Current.IsEnabled) { $b } }
    Clicar $up
    $c = New-Object System.Windows.Automation.AndCondition((New-Object System.Windows.Automation.PropertyCondition($A::NameProperty, 'Warning')), (New-Object System.Windows.Automation.PropertyCondition($A::ClassNameProperty, '#32770')))
    $dlg = Esperar { $A::RootElement.FindFirst($TS::Descendants, $c) }
    $txt = ($dlg.FindAll($TS::Descendants, [System.Windows.Automation.Condition]::TrueCondition) | ForEach-Object { $_.Current.Name } | Where-Object { $_ -like '*tested*' }) -join ' '
    "aviso nao testado: $txt"
    $no = $dlg.FindFirst($TS::Descendants, (New-Object System.Windows.Automation.PropertyCondition($A::AutomationIdProperty, '7')))   # IDNO
    Invocar $no
    Start-Sleep 1
    $intacto = Iguais $fotoDemo (Foto $pastaDemo)
    "Demo T intacto depois do Nao: $intacto"
    $ok = $semInstall -and $txt.Length -gt 0 -and $intacto
} finally { Fechar $p }
if ($ok) { 'RESULTADO: ok' } else { 'RESULTADO: FALHA'; exit 1 }
