# Compila o instalador (AMDLSS5-Instalador.exe) com o csc.exe do .NET Framework 4 que vem no
# Windows: nenhuma ferramenta extra. Fonte em instalador\src\*.cs; logo e icone gerados por
# Gerar-Logo.ps1 (refeitos se faltarem).
#
#   powershell -NoProfile -File instalador\Build-Instalador.ps1 [-Saida <pasta>] [-Versao 1.0.0]
#
# Saida padrao: instalador\bin\AMDLSS5-Instalador.exe (ignorado pelo git: *.exe).
param(
    [string]$Saida = (Join-Path $PSScriptRoot 'bin'),
    [string]$Versao = '1.5.2'
)
$ErrorActionPreference = 'Stop'
$csc = Join-Path $env:WINDIR 'Microsoft.NET\Framework64\v4.0.30319\csc.exe'
if (-not (Test-Path -LiteralPath $csc)) { $csc = Join-Path $env:WINDIR 'Microsoft.NET\Framework\v4.0.30319\csc.exe' }
if (-not (Test-Path -LiteralPath $csc)) { throw 'csc.exe do .NET Framework 4 nao encontrado' }

$logo = Join-Path $PSScriptRoot 'logo.png'
$ico = Join-Path $PSScriptRoot 'AMDLSS5.ico'
if (-not (Test-Path -LiteralPath $logo) -or -not (Test-Path -LiteralPath $ico)) {
    & powershell -NoProfile -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot 'Gerar-Logo.ps1') | Out-Null
}
New-Item -ItemType Directory -Force -Path $Saida | Out-Null
$exe = Join-Path $Saida 'AMDLSS5-Instalador.exe'

# VERSIONINFO do exe (Propriedades > Detalhes): o csc monta o recurso com estes atributos (CompanyName, ProductName,
# FileDescription, FileVersion, ProductVersion, LegalCopyright; OriginalFilename/InternalName = nome do exe).
# Sem empacotador, sem compressao, sem ofuscacao: exe .NET simples, o payload vai como arquivos soltos ao lado.
$obj = Join-Path $env:TEMP ('amdlss5-versao-' + [guid]::NewGuid().ToString('N') + '.cs')
$partes = @($Versao -split '[^0-9]+' | Where-Object { $_ }) + @('0', '0', '0', '0')
$v4 = $partes[0..3] -join '.'
Set-Content -LiteralPath $obj -Encoding utf8 -Value @"
[assembly: System.Reflection.AssemblyTitle("AMDLSS5 MOD - Instalador")]
[assembly: System.Reflection.AssemblyCompany("AMDLSS5 MOD")]
[assembly: System.Reflection.AssemblyCopyright("Copyright (c) 2026 AMDLSS5 MOD")]
[assembly: System.Reflection.AssemblyTrademark("")]
[assembly: System.Reflection.AssemblyProduct("AMDLSS5 MOD")]
[assembly: System.Reflection.AssemblyDescription("Instalador do AMDLSS5 MOD")]
[assembly: System.Reflection.AssemblyVersion("$v4")]
[assembly: System.Reflection.AssemblyFileVersion("$v4")]
[assembly: System.Reflection.AssemblyInformationalVersion("$Versao")]
"@
try {
    $fontes = @(Get-ChildItem -LiteralPath (Join-Path $PSScriptRoot 'src') -Filter '*.cs' | ForEach-Object { $_.FullName }) + $obj
    $argsCsc = @(
        '/nologo', '/target:winexe', '/platform:anycpu', '/optimize+', '/codepage:65001', '/langversion:5',
        "/out:$exe",
        "/win32icon:$ico",
        "/win32manifest:$(Join-Path $PSScriptRoot 'src\app.manifest')",
        "/resource:$logo,Amdlss5.logo.png",
        '/reference:System.dll', '/reference:System.Core.dll', '/reference:System.Drawing.dll', '/reference:System.Windows.Forms.dll', '/reference:System.IO.Compression.dll', '/reference:System.IO.Compression.FileSystem.dll',
        '/reference:System.Web.Extensions.dll'
    ) + $fontes
    $saidaCsc = & $csc @argsCsc 2>&1
    if ($LASTEXITCODE -ne 0) { $saidaCsc | ForEach-Object { "$_" }; throw "csc falhou ($LASTEXITCODE)" }
    $saidaCsc | Where-Object { "$_" -match 'warning' } | ForEach-Object { "$_" }
} finally { Remove-Item -LiteralPath $obj -Force -ErrorAction SilentlyContinue }

$h = (Get-FileHash -LiteralPath $exe -Algorithm SHA256).Hash
"exe   : $exe"
"bytes : $((Get-Item -LiteralPath $exe).Length)"
"sha256: $h"
