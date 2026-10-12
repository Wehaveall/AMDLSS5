# Teste de ponta a ponta do instalador em jogos FALSOS (nunca toca nos jogos de verdade, nao usa a GPU).
# Monta um pacote pequeno (DLLs x64 do Windows no lugar das do mod, bytes aleatorios no lugar dos .hsaco),
# pastas que imitam o RE (re9.exe + re_chunk_000.pak + _storage_) e o Cyberpunk (bin\x64), e confere:
# deteccao, instalacao, reinstalacao, desinstalacao byte a byte, version.dll/dinput8/INI alheios,
# jogo aberto, sem permissao, arquivos NVIDIA faltando, falha no meio com desfazer automatico,
# pacote real (opcional, -PayloadReal) e download do REFramework (opcional, -Rede). Tira capturas da janela.
#
#   powershell -NoProfile -File instalador\Testar-Instalador.ps1 [-Exe <AMDLSS5-Instalador.exe>] [-PayloadReal <pasta payload>] [-Rede]
param(
    [string]$Exe = (Join-Path $PSScriptRoot 'bin\AMDLSS5-Instalador.exe'),
    [string]$PayloadReal = '',
    [switch]$Rede,
    [string]$Raiz = (Join-Path $env:TEMP ('AMDLSS5-teste-' + (Get-Date -Format 'yyyyMMdd-HHmmss')))
)
$ErrorActionPreference = 'Stop'
$Exe = (Resolve-Path -LiteralPath $Exe).Path
New-Item -ItemType Directory -Force -Path $Raiz | Out-Null
$sys = Join-Path $env:WINDIR 'System32'
$resultados = New-Object System.Collections.Generic.List[object]

function Ok([string]$nome, [bool]$cond, [string]$detalhe = '') {
    $resultados.Add([pscustomobject]@{ Teste = $nome; Ok = $cond; Detalhe = $detalhe })
    $marca = if ($cond) { 'OK  ' } else { 'FALHA' }
    Write-Host "[$marca] $nome $detalhe"
}

function Foto([string]$dir) {
    $m = @{}
    foreach ($i in Get-ChildItem -LiteralPath $dir -Recurse -Force) {
        $rel = $i.FullName.Substring($dir.Length).TrimStart('\')
        if ($i.PSIsContainer) { $m["DIR:$rel"] = '' } else { $m[$rel] = (Get-FileHash -LiteralPath $i.FullName -Algorithm SHA256).Hash }
    }
    return $m
}

function Iguais($a, $b) {
    if ($a.Count -ne $b.Count) { return $false }
    foreach ($k in $a.Keys) { if (-not $b.ContainsKey($k) -or $b[$k] -ne $a[$k]) { return $false } }
    return $true
}

function Dif($a, $b) {
    $l = @()
    foreach ($k in $a.Keys) { if (-not $b.ContainsKey($k)) { $l += "-$k" } elseif ($b[$k] -ne $a[$k]) { $l += "~$k" } }
    foreach ($k in $b.Keys) { if (-not $a.ContainsKey($k)) { $l += "+$k" } }
    return ($l | Select-Object -First 8) -join ' '
}

$script:n = 0
function Rodar([string[]]$argumentos) {
    $script:n++
    $saida = Join-Path $Raiz ("saida-{0:D2}.txt" -f $script:n)
    $lista = @('--silencioso') + $argumentos + @('--saida', $saida) | ForEach-Object { if ($_ -match '\s') { '"' + $_ + '"' } else { $_ } }
    $p = Start-Process -FilePath $Exe -ArgumentList $lista -Wait -PassThru -WindowStyle Hidden
    $texto = if (Test-Path -LiteralPath $saida) { [IO.File]::ReadAllText($saida) } else { '' }
    return [pscustomobject]@{ Codigo = $p.ExitCode; Texto = $texto }
}

function Bytes([string]$p, [int]$tam, [int]$semente) {
    $r = New-Object Random $semente
    $b = New-Object byte[] $tam
    $r.NextBytes($b)
    [IO.File]::WriteAllBytes($p, $b)
}

# ------------------------------------------------------------------------------------------ pacote falso
function Novo-Payload([string]$dir, [switch]$SemNvngx, [switch]$SemPesos, [switch]$SemReframework, [switch]$ComIni) {
    $mod = Join-Path $dir 'mod'
    New-Item -ItemType Directory -Force -Path $mod | Out-Null
    $dlls = @{ 'version.dll' = 'version.dll'; 'nr_native_adapter.dll' = 'winmm.dll'; 'amdhip64_7.dll' = 'dwmapi.dll'; 'amd_comgr.dll' = 'uxtheme.dll'; 'rocm_kpack.dll' = 'msimg32.dll'; 'nvngx_dlssnr.dll' = 'dxgi.dll' }
    foreach ($k in $dlls.Keys) {
        if ($k -eq 'nvngx_dlssnr.dll' -and $SemNvngx) { continue }
        Copy-Item -LiteralPath (Join-Path $sys $dlls[$k]) -Destination (Join-Path $mod $k)
    }
    $s = 1
    foreach ($k in 'nr_reg_fast.hsaco', 'nr_mh_layout.hsaco', 'nr_resize.hsaco', 'colorcs.cso', 'depthcs.cso', 'motioncs.cso') { Bytes (Join-Path $mod $k) (4096 * $s) $s; $s++ }
    if (-not $SemPesos) { Bytes (Join-Path $mod 'dlssnr_on_amd_weights.bin') (1MB) 99 }
    # 1.1.0: o pacote nao leva INI (o helper publico cria o da proxy ao jogar); -ComIni = pacote antigo (1.0.0)
    if ($ComIni) { [IO.File]::WriteAllText((Join-Path $mod 'dlssnr_on_amd.ini'), "[DlssNrOnAmd]`r`nEnabled=1`r`nScale=0.03125`r`nTemporal=1`r`nNeuralPostPercent=100`r`n") }
    $arq = [ordered]@{}
    foreach ($k in @($dlls.Keys) + @('nr_reg_fast.hsaco', 'nr_mh_layout.hsaco', 'nr_resize.hsaco', 'colorcs.cso', 'depthcs.cso', 'motioncs.cso', 'dlssnr_on_amd_weights.bin') + $(if ($ComIni) { @('dlssnr_on_amd.ini') } else { @() })) {
        $p = Join-Path $mod $k
        $arq[$k] = if (Test-Path -LiteralPath $p) { (Get-FileHash -LiteralPath $p -Algorithm SHA256).Hash } else { 'E16BCF15E16E13F527491CDF7845B2FE6521A738D8F7C9C721866A8496E1FC8E' }
    }
    $doc = [ordered]@{ produto = 'AMDLSS5 MOD'; versao = 'teste'; arquivos = $arq }
    if (-not $SemReframework) {
        New-Item -ItemType Directory -Force -Path (Join-Path $dir 'reframework') | Out-Null
        Copy-Item -LiteralPath (Join-Path $sys 'dinput8.dll') -Destination (Join-Path $dir 'reframework\dinput8.dll')
        $doc.reframework = [ordered]@{ arquivo = 'reframework/dinput8.dll'; sha256 = (Get-FileHash -LiteralPath (Join-Path $dir 'reframework\dinput8.dll') -Algorithm SHA256).Hash; revisao = 'teste' }
    }
    [IO.File]::WriteAllText((Join-Path $dir 'pacote.json'), ($doc | ConvertTo-Json -Depth 4))
    return $dir
}

# 1.1.0: o que o MOD grava ao jogar (o instalador nao escreve nada disso): o INI da proxy completado pelo helper e o
# amdlss5.dat das escolhas do menu, na pasta do exe e em _storage_ se existir.
function Simula-Jogo([string]$pasta) {
    foreach ($d in @($pasta) + $(if (Test-Path (Join-Path $pasta '_storage_')) { @(Join-Path $pasta '_storage_') } else { @() })) {
        $ini = Join-Path $d 'dlssnr_on_amd.ini'
        if (Test-Path -LiteralPath $ini) { [IO.File]::AppendAllText($ini, "`r`nInline=0`r`nPredWait=0`r`n") }
        else { [IO.File]::WriteAllText($ini, "[DlssNrOnAmd]`r`nEnabled=0`r`nLocalStructure=1.450`r`nInline=0`r`nPredWait=0`r`n") }
        [IO.File]::WriteAllText((Join-Path $d 'amdlss5.dat'), 'bm9zc28gZGF0IGRlIHRlc3Rl')
    }
}

# ------------------------------------------------------------------------------------------ jogos falsos
$exeStub = Join-Path $sys 'choice.exe'     # x64, importa VERSION.dll (como os jogos) e fica aberto com /T
function Novo-RE([string]$dir, [string]$nomeExe = 're9.exe') {
    New-Item -ItemType Directory -Force -Path (Join-Path $dir '_storage_') | Out-Null
    Copy-Item -LiteralPath $exeStub -Destination (Join-Path $dir $nomeExe)
    Bytes (Join-Path $dir 're_chunk_000.pak') 65536 7
    Copy-Item -LiteralPath (Join-Path $sys 'dcomp.dll') -Destination (Join-Path $dir 'amd_fidelityfx_dx12.dll')
    Bytes (Join-Path $dir '_storage_\dados_do_jogo.bin') 2048 8
    return $dir
}
function Novo-CP([string]$dir) {
    $x64 = Join-Path $dir 'bin\x64'
    New-Item -ItemType Directory -Force -Path $x64, (Join-Path $dir 'archive\pc\content') | Out-Null
    Copy-Item -LiteralPath $exeStub -Destination (Join-Path $x64 'Cyberpunk2077.exe')
    Copy-Item -LiteralPath $exeStub -Destination (Join-Path $dir 'REDprelauncher.exe')
    Copy-Item -LiteralPath (Join-Path $sys 'dcomp.dll') -Destination (Join-Path $x64 'amd_fidelityfx_loader_dx12.dll')
    Copy-Item -LiteralPath (Join-Path $sys 'dcomp.dll') -Destination (Join-Path $x64 'amd_fidelityfx_upscaler_dx12.dll')
    Bytes (Join-Path $dir 'archive\pc\content\basegame_1.archive') 8192 9
    return $dir
}

$pay = Novo-Payload (Join-Path $Raiz 'payload')
$re = Novo-RE (Join-Path $Raiz 'RE Falso')
$fotoRe = Foto $re

# T1 deteccao
$r = Rodar @('--detectar', $re, '--payload', $pay)
Ok 'deteccao RE' ($r.Codigo -eq 0 -and $r.Texto -match 're_engine: True' -and $r.Texto -match 'storage: True' -and $r.Texto -match 'fsr: monolitico') "codigo=$($r.Codigo)"

# T2 instalar (REFramework automatico, idioma en)
$r = Rodar @('--instalar', $re, '--payload', $pay, '--idioma', 'en')
$okArq = (Test-Path (Join-Path $re 'version.dll')) -and (Test-Path (Join-Path $re '_storage_\version.dll')) -and (Test-Path (Join-Path $re 'dinput8.dll')) -and -not (Test-Path (Join-Path $re '_storage_\dinput8.dll'))
$semIni = -not (Test-Path (Join-Path $re 'dlssnr_on_amd.ini')) -and -not (Test-Path (Join-Path $re '_storage_\dlssnr_on_amd.ini'))
$hashOk = (Get-FileHash (Join-Path $re 'nr_native_adapter.dll')).Hash -eq (Get-FileHash (Join-Path $pay 'mod\nr_native_adapter.dll')).Hash
Ok 'instalar RE (raiz + _storage_ + REFramework, sem INI)' ($r.Codigo -eq 0 -and $okArq -and $hashOk -and $semIni) "codigo=$($r.Codigo)"
$sess = @(Get-ChildItem -LiteralPath (Join-Path $re 'AMDLSS5_backup') -Directory)
$acoes = if ($sess.Count -eq 1) { [IO.File]::ReadAllText((Join-Path $sess[0].FullName 'acoes.json')) } else { '' }
Ok 'log e acoes.json ao lado do backup (INI e amdlss5.dat marcados como do mod)' ($sess.Count -eq 1 -and (Test-Path (Join-Path $sess[0].FullName 'instalacao.log')) -and ([regex]::Matches($acoes, 'criado_pelo_mod')).Count -eq 4)

# o mod roda: cria o INI da proxy e o amdlss5.dat (raiz e _storage_)
Simula-Jogo $re

# T3 reinstalar (tudo igual; o INI e o .dat que o mod criou ficam e sao guardados)
$iniAntes = (Get-FileHash (Join-Path $re 'dlssnr_on_amd.ini')).Hash
$r = Rodar @('--instalar', $re, '--payload', $pay)
Ok 'reinstalar RE (INI do mod intacto)' ($r.Codigo -eq 0 -and $r.Texto -match '\(j. igual\)|already identical|ya igual' -and (Get-FileHash (Join-Path $re 'dlssnr_on_amd.ini')).Hash -eq $iniAntes) "codigo=$($r.Codigo)"

# T4 desinstalar -> identico (o INI e o amdlss5.dat criados pelo mod saem)
$r = Rodar @('--desinstalar', $re, '--payload', $pay)
$depois = Foto $re
Ok 'desinstalar RE byte a byte (INI e amdlss5.dat do mod removidos)' ($r.Codigo -eq 0 -and (Iguais $fotoRe $depois)) "codigo=$($r.Codigo) $(Dif $fotoRe $depois)"

# T5 Cyberpunk: escolher a raiz, o mod vai para bin\x64
$cp = Novo-CP (Join-Path $Raiz 'Cyberpunk Falso')
$fotoCp = Foto $cp
$r = Rodar @('--instalar', $cp, '--payload', $pay)
$x64 = Join-Path $cp 'bin\x64'
Ok 'instalar CP em bin\x64 (sem REFramework, sem INI)' ($r.Codigo -eq 0 -and (Test-Path (Join-Path $x64 'version.dll')) -and -not (Test-Path (Join-Path $x64 'dinput8.dll')) -and -not (Test-Path (Join-Path $cp 'version.dll')) -and -not (Test-Path (Join-Path $x64 'dlssnr_on_amd.ini')) -and $r.Texto -match 'fsr: split') "codigo=$($r.Codigo)"
Simula-Jogo $x64
$r = Rodar @('--desinstalar', $cp, '--payload', $pay)
$depois = Foto $cp
Ok 'desinstalar CP byte a byte' ($r.Codigo -eq 0 -and (Iguais $fotoCp $depois)) "codigo=$($r.Codigo) $(Dif $fotoCp $depois)"

# T6 jogo com arquivos alheios: version.dll de outro mod, dinput8 do usuario, INI do usuario, .hsaco diferente
$re2 = Novo-RE (Join-Path $Raiz 'RE Alheio') 're4.exe'
Copy-Item -LiteralPath (Join-Path $sys 'd3d11.dll') -Destination (Join-Path $re2 'version.dll')
Copy-Item -LiteralPath (Join-Path $sys 'dinput8.dll') -Destination (Join-Path $re2 'dinput8.dll')
[IO.File]::WriteAllBytes((Join-Path $re2 'dlssnr_on_amd.ini'), [Text.Encoding]::GetEncoding(28591).GetBytes("[DlssNrOnAmd]`r`nScale=0.05`r`n; comentario do usuario: a`xE7`xE3o`r`nEnabled=0"))
Bytes (Join-Path $re2 'nr_reg_fast.hsaco') 777 55
$fotoRe2 = Foto $re2
$r = Rodar @('--instalar', $re2, '--payload', $pay)
Ok 'alheio sem --forcar recusa (codigo 2)' ($r.Codigo -eq 2 -and (Iguais $fotoRe2 (Foto $re2))) "codigo=$($r.Codigo)"
$r = Rodar @('--instalar', $re2, '--payload', $pay, '--forcar')
$iniUsuario = $fotoRe2['dlssnr_on_amd.ini']
$dinOk = (Get-FileHash (Join-Path $re2 'dinput8.dll')).Hash -eq (Get-FileHash (Join-Path $sys 'dinput8.dll')).Hash
$verOk = (Get-FileHash (Join-Path $re2 'version.dll')).Hash -eq (Get-FileHash (Join-Path $pay 'mod\version.dll')).Hash
Ok 'alheio com --forcar: troca version.dll, mantem dinput8 e o INI do usuario' ($r.Codigo -eq 0 -and $verOk -and $dinOk -and (Get-FileHash (Join-Path $re2 'dlssnr_on_amd.ini')).Hash -eq $iniUsuario) "codigo=$($r.Codigo)"
Simula-Jogo $re2   # o helper completa o INI do usuario com as chaves da proxy que faltam
$r = Rodar @('--desinstalar', $re2, '--payload', $pay)
$depois = Foto $re2
Ok 'desinstalar alheio byte a byte (version.dll, INI e .hsaco originais)' ($r.Codigo -eq 0 -and (Iguais $fotoRe2 $depois)) "codigo=$($r.Codigo) $(Dif $fotoRe2 $depois)"

# T6b pacote antigo (1.0.0, com INI): o instalador ainda mescla o INI e desinstala limpo
$payIni = Novo-Payload (Join-Path $Raiz 'payload-com-ini') -ComIni
$re6 = Novo-RE (Join-Path $Raiz 'RE Pacote Antigo')
$foto6 = Foto $re6
$r = Rodar @('--instalar', $re6, '--payload', $payIni, '--idioma', 'en')
$ini6 = if (Test-Path (Join-Path $re6 'dlssnr_on_amd.ini')) { [IO.File]::ReadAllText((Join-Path $re6 'dlssnr_on_amd.ini')) } else { '' }
$r2 = Rodar @('--desinstalar', $re6, '--payload', $payIni)
Ok 'pacote antigo com INI: instala o INI (MenuLanguage) e desinstala byte a byte' ($r.Codigo -eq 0 -and $ini6 -match 'MenuLanguage=en' -and $r2.Codigo -eq 0 -and (Iguais $foto6 (Foto $re6))) "codigo=$($r.Codigo)/$($r2.Codigo)"

# T7 incompativel: exe que nao carrega version.dll
$inc = Join-Path $Raiz 'Incompativel'
New-Item -ItemType Directory -Force -Path $inc | Out-Null
$semVersion = Get-ChildItem $sys -Filter '*.exe' | Where-Object { $_.Length -lt 200KB -and $_.Name -notmatch 'setup|install|crash|report|launcher' } | Select-Object -First 60 | Where-Object {
    $t = [IO.File]::ReadAllBytes($_.FullName); -not ([Text.Encoding]::ASCII.GetString($t) -match '(?i)version\.dll') } | Select-Object -First 1
Copy-Item -LiteralPath $semVersion.FullName -Destination (Join-Path $inc 'jogo.exe')
$fotoInc = Foto $inc
$r = Rodar @('--instalar', $inc, '--payload', $pay, '--forcar')
Ok 'incompativel recusa (codigo 2, nada escrito)' ($r.Codigo -eq 2 -and (Iguais $fotoInc (Foto $inc)) -and $r.Texto -match 'BLOQUEIO') "codigo=$($r.Codigo) ($($semVersion.Name))"

# T8 jogo aberto
$proc = Start-Process -FilePath (Join-Path $re 're9.exe') -ArgumentList '/T', '30', '/D', 'S', '/C', 'SN' -PassThru -WindowStyle Hidden
Start-Sleep -Milliseconds 700
$r = Rodar @('--instalar', $re, '--payload', $pay)
try { $proc.Kill() } catch { }
$proc.WaitForExit(5000) | Out-Null
Ok 'jogo aberto recusa (codigo 3, nada escrito)' ($r.Codigo -eq 3 -and (Iguais $fotoRe (Foto $re))) "codigo=$($r.Codigo)"

# T9 sem permissao de escrita
$prot = Novo-RE (Join-Path $Raiz 'RE Protegido')
$fotoProt = Foto $prot
& icacls $prot /deny "${env:USERNAME}:(WD,AD)" | Out-Null
$r = Rodar @('--instalar', $prot, '--payload', $pay)
& icacls $prot /remove:d $env:USERNAME | Out-Null
Ok 'sem permissao (codigo 4, nada escrito)' ($r.Codigo -eq 4 -and (Iguais $fotoProt (Foto $prot)) -and $r.Texto -match 'administrador|administrator') "codigo=$($r.Codigo)"

# T10 arquivos NVIDIA fora do pacote
$paySemNv = Novo-Payload (Join-Path $Raiz 'payload-sem-nvidia') -SemNvngx -SemPesos
$re3 = Novo-RE (Join-Path $Raiz 'RE NVIDIA')
$fotoRe3 = Foto $re3
$r = Rodar @('--instalar', $re3, '--payload', $paySemNv)
Ok 'sem nvngx no pacote e sem --nvngx (codigo 5, nada escrito)' ($r.Codigo -eq 5 -and (Iguais $fotoRe3 (Foto $re3))) "codigo=$($r.Codigo)"
$meuNv = Join-Path $Raiz 'meu_nvngx_dlssnr.dll'
Copy-Item -LiteralPath (Join-Path $sys 'dxgi.dll') -Destination $meuNv
$r = Rodar @('--instalar', $re3, '--payload', $paySemNv, '--nvngx', $meuNv)
Ok 'sem pesos no pacote: mensagem clara (codigo 5)' ($r.Codigo -eq 5 -and $r.Texto -match 'dlssnr_on_amd_weights.bin' -and (Iguais $fotoRe3 (Foto $re3))) "codigo=$($r.Codigo)"
$meusPesos = Join-Path $Raiz 'pesos.bin'
Bytes $meusPesos 4096 3
$r = Rodar @('--instalar', $re3, '--payload', $paySemNv, '--nvngx', $meuNv, '--pesos', $meusPesos)
$nvOk = (Test-Path (Join-Path $re3 'nvngx_dlssnr.dll')) -and ((Get-FileHash (Join-Path $re3 'nvngx_dlssnr.dll')).Hash -eq (Get-FileHash $meuNv).Hash)
Ok 'nvngx e pesos do usuario instalados' ($r.Codigo -eq 0 -and $nvOk -and $r.Texto -match 'nvngx_dlssnr.dll') "codigo=$($r.Codigo)"
$r = Rodar @('--desinstalar', $re3, '--payload', $paySemNv)
Ok 'desinstalar (arquivos do usuario) byte a byte' ($r.Codigo -eq 0 -and (Iguais $fotoRe3 (Foto $re3))) "codigo=$($r.Codigo) $(Dif $fotoRe3 (Foto $re3))"

# T11 falha no meio: um arquivo do jogo travado -> desfaz sozinho
$re4 = Novo-RE (Join-Path $Raiz 'RE Travado')
Bytes (Join-Path $re4 'nr_resize.hsaco') 333 77
$fotoRe4 = Foto $re4
$trava = [IO.File]::Open((Join-Path $re4 'nr_resize.hsaco'), 'Open', 'Read', 'None')
try { $r = Rodar @('--instalar', $re4, '--payload', $pay) } finally { $trava.Close() }
$depois = Foto $re4
Ok 'falha no meio desfaz tudo (codigo 1, pasta identica)' ($r.Codigo -eq 1 -and (Iguais $fotoRe4 $depois) -and $r.Texto -match 'desfeito|undone|deshizo') "codigo=$($r.Codigo) $(Dif $fotoRe4 $depois)"

# T12 arquivo do mod trocado pelo usuario depois da instalacao: desinstalar nao apaga o dele
$re5 = Novo-RE (Join-Path $Raiz 'RE Usuario')
$r = Rodar @('--instalar', $re5, '--payload', $pay)
Bytes (Join-Path $re5 'nr_mh_layout.hsaco') 100 5
$r2 = Rodar @('--desinstalar', $re5, '--payload', $pay)
Ok 'arquivo trocado pelo usuario fica' ($r.Codigo -eq 0 -and $r2.Codigo -eq 0 -and (Test-Path (Join-Path $re5 'nr_mh_layout.hsaco')) -and -not (Test-Path (Join-Path $re5 'version.dll'))) "codigo=$($r2.Codigo)"

# T13 pacote real (opcional)
if ($PayloadReal) {
    $reR = Novo-RE (Join-Path $Raiz 'RE Pacote Real')
    $fotoR = Foto $reR
    $r = Rodar @('--instalar', $reR, '--payload', $PayloadReal)
    $man = Get-Content -Raw (Join-Path $PayloadReal 'pacote.json') | ConvertFrom-Json
    $todos = $true
    foreach ($p in $man.arquivos.PSObject.Properties) {
        if ($p.Name -eq 'dlssnr_on_amd.ini') { continue }
        foreach ($alvo in $reR, (Join-Path $reR '_storage_')) {
            $c = Join-Path $alvo $p.Name
            if (-not (Test-Path $c) -or (Get-FileHash $c).Hash -ne $p.Value) { $todos = $false }
        }
    }
    $refOk = (Get-FileHash (Join-Path $reR 'dinput8.dll')).Hash -eq $man.reframework.sha256
    Ok "pacote real: $(@($man.arquivos.PSObject.Properties).Count) arquivos + REFramework conferidos (raiz e _storage_)" ($r.Codigo -eq 0 -and $todos -and $refOk) "codigo=$($r.Codigo)"
    $inisPacote = @(Get-ChildItem -LiteralPath (Split-Path -Parent $PayloadReal) -Recurse -File -Filter '*.ini')
    $inisJogo = @(Get-ChildItem -LiteralPath $reR -Recurse -File -Filter '*.ini' | Where-Object { $_.FullName -notmatch 'AMDLSS5_backup' })
    Ok 'pacote real sem nenhum .ini (pacote e jogo depois de instalar)' ($inisPacote.Count -eq 0 -and $inisJogo.Count -eq 0 -and -not $man.arquivos.PSObject.Properties['dlssnr_on_amd.ini']) "pacote=$($inisPacote.Count) jogo=$($inisJogo.Count)"
    Simula-Jogo $reR
    $r = Rodar @('--desinstalar', $reR, '--payload', $PayloadReal)
    Ok 'pacote real: desinstalar byte a byte' ($r.Codigo -eq 0 -and (Iguais $fotoR (Foto $reR))) "codigo=$($r.Codigo) $(Dif $fotoR (Foto $reR))"
}

# T13b pacote sem REFramework e download nao pedido: recusa sem tocar no jogo (o download e opcional e desligado)
$paySemRef0 = Novo-Payload (Join-Path $Raiz 'payload-sem-ref-0') -SemReframework
$reS = Novo-RE (Join-Path $Raiz 'RE Sem Download')
$fotoS = Foto $reS
$r = Rodar @('--instalar', $reS, '--payload', $paySemRef0)
Ok 'sem REFramework no pacote e sem --permitir-download: recusa (codigo 5, nada escrito, nada baixado)' ($r.Codigo -eq 5 -and (Iguais $fotoS (Foto $reS)) -and $r.Texto -notmatch 'api.github.com') "codigo=$($r.Codigo)"

# T14 download do REFramework (opcional, internet)
if ($Rede) {
    $paySemRef = Novo-Payload (Join-Path $Raiz 'payload-sem-ref') -SemReframework
    $reN = Novo-RE (Join-Path $Raiz 'RE Download')
    $fotoN = Foto $reN
    $r = Rodar @('--instalar', $reN, '--payload', $paySemRef, '--permitir-download')
    Ok 'REFramework baixado do GitHub e conferido' ($r.Codigo -eq 0 -and (Test-Path (Join-Path $reN 'dinput8.dll')) -and $r.Texto -match 'github.com/praydog/REFramework-nightly') "codigo=$($r.Codigo)"
    ($r.Texto -split "`n" | Where-Object { $_ -match 'REFramework' }) | ForEach-Object { "    $_" }
    $r = Rodar @('--desinstalar', $reN, '--payload', $paySemRef)
    Ok 'download: desinstalar byte a byte' ($r.Codigo -eq 0 -and (Iguais $fotoN (Foto $reN))) "codigo=$($r.Codigo)"
}

# T15 capturas da janela (pt, en, es) com o RE falso escolhido
foreach ($idioma in 'pt', 'en', 'es') {
    $png = Join-Path $Raiz "janela-$idioma.png"
    $p = Start-Process -FilePath $Exe -ArgumentList '--pasta', ('"' + $re + '"'), '--payload', ('"' + $pay + '"'), '--idioma', $idioma, '--captura', ('"' + $png + '"') -PassThru
    if (-not $p.WaitForExit(30000)) { try { $p.Kill() } catch { } }
    Ok "captura da janela ($idioma)" (Test-Path $png) $png
}

$falhas = @($resultados | Where-Object { -not $_.Ok })
''
"RESULTADO: $($resultados.Count - $falhas.Count)/$($resultados.Count) ok  (pasta de teste: $Raiz)"
if ($falhas.Count) { exit 1 }
