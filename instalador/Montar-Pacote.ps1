# Monta o pacote para jogadores: <Saida>\AMDLSS5-<versao>\ (AMDLSS5-Instalador.exe + payload\ + LEIA-ME.txt)
# e <Saida>\AMDLSS5-<versao>.zip, FORA do repositorio (binarios nunca entram nele; o repositorio
# publico leva so o fonte do instalador). Nunca apaga nada: pasta ou zip existentes = erro.
#
#   powershell -NoProfile -File instalador\Montar-Pacote.ps1 [-Versao 1.0.0] [-Helper <nr_native_adapter.dll publico>]
#        [-Pacote <pasta>] [-Ini <ini>] [-ManterDiagnostico] [-REFramework <pasta>|''] [-IniJogo @{Exe='ini'}] [-Saida <pasta>] [-SemZip]
#
# Padroes:
#   -Pacote      mod\candidatos\menu-final-0310\pacote-re do checkout principal (instalado no RE em 03/10); o manifesto
#                <Pacote>.json (nome -> SHA-256) confere cada arquivo
#   -Helper      o helper do -Pacote (de pesquisa). Para o release publico passe o helper PUBLICO: ele precisa ter sido
#                feito para o mesmo version.dll do pacote (conferido: o SHA-256 do proxy tem de estar dentro do helper)
#   -Ini         1.1.0: padrao = SEM INI (o helper publico cria o INI da proxy no primeiro jogo; o pacote so leva
#                padroes embutidos). Um caminho de INI = pacote antigo, com o diagnostico desligado (F9, captura, despejos)
#   -REFramework mod\terceiros\reframework (dinput8.dll embutido); '' = nao embute (o instalador baixa do GitHub)
#   -Saida       <pasta-mae do repositorio>\AMDLSS5-release
# O trabalho de copiar/conferir e feito por ferramentas\MontarPacote.cs (compilado aqui com o csc do .NET Framework).
param(
    [string]$Versao = '1.3.0',
    [string]$Pacote = '',
    [string]$Helper = '',
    [string]$Ini = '',
    [hashtable]$IniJogo = @{},
    [switch]$ManterDiagnostico,
    [string]$REFramework = '',
    [string]$Saida = '',
    [switch]$SemZip
)
$ErrorActionPreference = 'Stop'

# raiz do repositorio e do checkout principal (num worktree, ".git" e um arquivo "gitdir: <principal>/.git/worktrees/<nome>")
$repoAtual = $PSScriptRoot
while ($repoAtual -and -not (Test-Path -LiteralPath (Join-Path $repoAtual '.git'))) { $repoAtual = Split-Path -Parent $repoAtual }
if (-not $repoAtual) { throw 'nao achei a raiz do repositorio acima de instalador\' }
$repoPrincipal = $repoAtual
$marca = Join-Path $repoAtual '.git'
if (Test-Path -LiteralPath $marca -PathType Leaf) {
    $dirComum = ((Get-Content -LiteralPath $marca -TotalCount 1) -replace '^gitdir:\s*', '').Trim() -replace '/', '\'
    $repoPrincipal = Split-Path -Parent (Split-Path -Parent (Split-Path -Parent $dirComum))
}
if (-not $Pacote) { $Pacote = Join-Path $repoPrincipal 'mod\candidatos\menu-final-0310\pacote-re' }
$Pacote = (Resolve-Path -LiteralPath $Pacote).Path
if (-not $Helper) { $Helper = Join-Path $Pacote 'nr_native_adapter.dll' }
$iniArg = if ($Ini) { (Resolve-Path -LiteralPath $Ini).Path } else { '-' }   # 1.1.0: sem INI por padrao
if (-not $PSBoundParameters.ContainsKey('REFramework')) { $REFramework = Join-Path $repoPrincipal 'mod\terceiros\reframework' }
if (-not $Saida) { $Saida = Join-Path (Split-Path -Parent $repoPrincipal) 'AMDLSS5-release' }
$Saida = [IO.Path]::GetFullPath($Saida)
foreach ($r in @($repoPrincipal, $repoAtual)) {
    $rr = [IO.Path]::GetFullPath($r).TrimEnd('\') + '\'
    if (($Saida.TrimEnd('\') + '\').StartsWith($rr, [StringComparison]::OrdinalIgnoreCase)) { throw "-Saida dentro do repositorio ($r)" }
}
$manifesto = $Pacote.TrimEnd('\') + '.json'
if (-not (Test-Path -LiteralPath $manifesto)) { $manifesto = Join-Path $Pacote 'pacote.json' }
if (-not (Test-Path -LiteralPath $manifesto)) { throw "manifesto do pacote nao encontrado ($Pacote.json)" }
$dest = Join-Path $Saida "AMDLSS5-$Versao"
$zip = Join-Path $Saida "AMDLSS5-$Versao.zip"
if (Test-Path -LiteralPath $dest) { throw "$dest ja existe: use outra -Versao" }
if (-not $SemZip -and (Test-Path -LiteralPath $zip)) { throw "$zip ja existe: use outra -Versao" }

$csc = Join-Path $env:WINDIR 'Microsoft.NET\Framework64\v4.0.30319\csc.exe'
$ferramenta = Join-Path $env:TEMP 'AMDLSS5-MontarPacote.exe'
& $csc /nologo /target:exe /optimize+ "/out:$ferramenta" /reference:System.Web.Extensions.dll (Join-Path $PSScriptRoot 'ferramentas\MontarPacote.cs')
if ($LASTEXITCODE -ne 0) { throw 'csc falhou (ferramentas\MontarPacote.cs)' }
$refArg = if ($REFramework) { (Resolve-Path -LiteralPath $REFramework).Path } else { '-' }
$extras = @($IniJogo.Keys | ForEach-Object { "$_=$((Resolve-Path -LiteralPath $IniJogo[$_]).Path)" })
$saidaFerramenta = & $ferramenta $Pacote $manifesto (Resolve-Path -LiteralPath $Helper).Path $iniArg $refArg $dest $Versao $(if ($ManterDiagnostico) { '1' } else { '0' }) (Join-Path $PSScriptRoot 'modelos') @extras
if ($LASTEXITCODE -ne 0) { throw "montagem falhou: $saidaFerramenta" }

$build = & powershell -NoProfile -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot 'Build-Instalador.ps1') -Saida $dest -Versao $Versao
if ($LASTEXITCODE -ne 0) { $build; throw 'build do instalador falhou' }
if (-not $SemZip) {
    # 11/10: entrada por entrada com '/' (o CreateFromDirectory do .NET Framework no PowerShell 5.1 grava '\', fora do
    # padrao ZIP: alguns extratores criam arquivos com '\' no nome em vez de pastas).
    Add-Type -AssemblyName System.IO.Compression, System.IO.Compression.FileSystem
    $raiz = Split-Path -Parent $dest; $fz = [IO.File]::Open($zip, [IO.FileMode]::CreateNew)
    try {
        $za = New-Object IO.Compression.ZipArchive($fz, [IO.Compression.ZipArchiveMode]::Create)
        try {
            foreach ($f in Get-ChildItem -LiteralPath $dest -Recurse -File | Sort-Object FullName) {
                $nome = $f.FullName.Substring($raiz.Length).TrimStart('\').Replace('\', '/')
                [void][IO.Compression.ZipFileExtensions]::CreateEntryFromFile($za, $f.FullName, $nome, [IO.Compression.CompressionLevel]::Optimal)
            }
        } finally { $za.Dispose() }
    } finally { $fz.Dispose() }
}

"pacote : $dest ($([Math]::Round((Get-ChildItem -LiteralPath $dest -Recurse -File | Measure-Object Length -Sum).Sum / 1MB)) MB)"
$saidaFerramenta | ForEach-Object { "  $_" }
$build | Where-Object { $_ -match '^sha256' } | ForEach-Object { "  exe $_" }
if (-not $SemZip) { "zip    : $zip ($([Math]::Round((Get-Item -LiteralPath $zip).Length / 1MB)) MB) sha256 $((Get-FileHash -LiteralPath $zip -Algorithm SHA256).Hash)" }
