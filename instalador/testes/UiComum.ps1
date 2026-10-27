# Funcoes comuns dos testes de clique na janela (clique-*.ps1): UI Automation para achar controles e caixas de mensagem,
# cliques POSTADOS no controle (WM_LBUTTONDOWN/UP, BM_CLICK): o mouse do usuario nao se mexe e nada vai para outra janela.
# Uso: . (Join-Path $PSScriptRoot 'UiComum.ps1')
Add-Type -AssemblyName UIAutomationClient, UIAutomationTypes
if (-not ('AmdUi.W' -as [type])) {
    Add-Type @'
using System; using System.Runtime.InteropServices;
namespace AmdUi { public static class W { [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr h, uint m, IntPtr w, IntPtr l); } }
'@
}
$A = [System.Windows.Automation.AutomationElement]
$TS = [System.Windows.Automation.TreeScope]

function Foto([string]$dir) {
    $m = @{}
    foreach ($i in Get-ChildItem -LiteralPath $dir -Recurse -Force) {
        $rel = $i.FullName.Substring($dir.Length).TrimStart('\')
        if ($i.PSIsContainer) { $m["DIR:$rel"] = '' } else { $m[$rel] = (Get-FileHash -LiteralPath $i.FullName -Algorithm SHA256).Hash }
    }
    return $m
}
function Iguais($a, $b) { if ($a.Count -ne $b.Count) { return $false }; foreach ($k in $a.Keys) { if (-not $b.ContainsKey($k) -or $b[$k] -ne $a[$k]) { return $false } }; return $true }
function Esperar([scriptblock]$cond, [int]$seg = 20) { $t = [Diagnostics.Stopwatch]::StartNew(); while ($t.Elapsed.TotalSeconds -lt $seg) { $r = & $cond; if ($r) { return $r }; Start-Sleep -Milliseconds 200 }; throw "tempo esgotado: $cond" }
function Achar($raiz, [string]$nome) {
    $c = New-Object System.Windows.Automation.PropertyCondition($A::NameProperty, $nome)
    return $raiz.FindFirst($TS::Descendants, $c)
}
function Janelas($procId) {
    $c = New-Object System.Windows.Automation.PropertyCondition($A::ProcessIdProperty, $procId)
    return @($A::RootElement.FindAll($TS::Children, $c))
}
# botao da janela (WinForms) = clique postado no controle; botao da caixa de mensagem (Win32) = BM_CLICK
function Clicar($el) { $h = [IntPtr]$el.Current.NativeWindowHandle; $l = [IntPtr]((10 -shl 16) -bor 10); [AmdUi.W]::PostMessage($h, 0x201, [IntPtr]1, $l) | Out-Null; [AmdUi.W]::PostMessage($h, 0x202, [IntPtr]0, $l) | Out-Null }
function Invocar($el) { [AmdUi.W]::PostMessage([IntPtr]$el.Current.NativeWindowHandle, 0xF5, [IntPtr]0, [IntPtr]0) | Out-Null }
# caixa de mensagem com titulo $titulo: aperta o botao $botao (nomes separados por virgula); devolve o texto dela
function Responder($procId, [string]$titulo, [string]$botao) {
    # a caixa de mensagem fica sob a janela dona na arvore da UI Automation
    $c = New-Object System.Windows.Automation.AndCondition((New-Object System.Windows.Automation.PropertyCondition($A::NameProperty, $titulo)), (New-Object System.Windows.Automation.PropertyCondition($A::ClassNameProperty, '#32770')))
    $dlg = Esperar { $A::RootElement.FindFirst($TS::Descendants, $c) } 60
    $txt = ($dlg.FindAll($TS::Descendants, [System.Windows.Automation.Condition]::TrueCondition) | Where-Object { $_.Current.ControlType -ne [System.Windows.Automation.ControlType]::Button -and $_.Current.Name } | ForEach-Object { $_.Current.Name }) -join ' | '
    Invocar (Esperar { $b = $null; foreach ($n in $botao -split ',') { if (-not $b) { $b = Achar $dlg $n } }; $b })
    return $txt
}
# abre a biblioteca na Steam falsa da pasta de teste (lojas\SteamT), sem rede e sem registro, em ingles
function Abrir-BibliotecaTeste([string]$Exe, [string]$Raiz) {
    return Start-Process -FilePath $Exe -ArgumentList @('--steam', ('"' + (Join-Path $Raiz 'lojas\SteamT') + '"'), '--sem-registro', '--sem-rede', '--capas', ('"' + (Join-Path $Raiz 'capas-clique') + '"'), '--payload', ('"' + (Join-Path $Raiz 'payload-biblioteca') + '"'), '--idioma', 'en') -PassThru
}
function Fechar($p) { if (-not $p.HasExited) { $p.CloseMainWindow() | Out-Null; if (-not $p.WaitForExit(5000)) { $p.Kill() } } }
