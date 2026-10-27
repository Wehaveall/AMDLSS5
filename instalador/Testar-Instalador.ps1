# Teste de ponta a ponta do instalador em jogos FALSOS (nunca toca nos jogos de verdade, nao usa a GPU).
# Monta um pacote pequeno (DLLs x64 do Windows no lugar das do mod, bytes aleatorios no lugar dos .hsaco),
# pastas que imitam o RE (re9.exe + re_chunk_000.pak + _storage_) e o Cyberpunk (bin\x64), e confere:
# deteccao, instalacao, reinstalacao, desinstalacao byte a byte, version.dll/dinput8/INI alheios,
# jogo aberto, sem permissao, arquivos NVIDIA faltando, falha no meio com desfazer automatico,
# pacote real (opcional, -PayloadReal) e download do REFramework (opcional, -Rede). Tira capturas da janela.
#
#   powershell -NoProfile -File instalador\Testar-Instalador.ps1 [-Exe <AMDLSS5-Instalador.exe>] [-PayloadReal <pasta payload>] [-Rede] [-Manter]
# Tudo certo = a pasta de teste e apagada no fim (-Manter a deixa); com falha ela fica para inspecao.
param(
    [string]$Exe = (Join-Path $PSScriptRoot 'bin\AMDLSS5-Instalador.exe'),
    [string]$PayloadReal = '',
    [switch]$Rede,
    [switch]$Manter,   # nao apaga a pasta de teste no fim (com falha ela sempre fica)
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
function Novo-Payload([string]$dir, [switch]$SemNvngx, [switch]$SemPesos, [switch]$SemReframework, [switch]$ComIni, [string]$IniTexto = '', [hashtable]$PorJogo = @{}, [string]$Testados = '', [hashtable]$PorJogoArq = @{}, [hashtable]$IniJogo = @{}) {
    # rdr2 (26/10): -PorJogoArq @{ exe = @{ relativo = <arquivo de origem copiado> } } (extras com recurso de versao, ex. DLLs do Windows);
    # -IniJogo @{ 'RDR2.ini' = <texto> } -> payload\ini\<nome> + "inis" no pacote.json (precisa de -ComIni/-IniTexto: o INI do jogo
    # so entra quando o pacote leva o INI padrao)
    if ($IniTexto) { $ComIni = [switch]::Present }
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
    if ($ComIni) {
        if (-not $IniTexto) { $IniTexto = "[DlssNrOnAmd]`r`nEnabled=1`r`nScale=0.03125`r`nTemporal=1`r`nNeuralPostPercent=100`r`n" }
        [IO.File]::WriteAllBytes((Join-Path $mod 'dlssnr_on_amd.ini'), [Text.Encoding]::GetEncoding(28591).GetBytes($IniTexto))
    }
    $arq = [ordered]@{}
    foreach ($k in @($dlls.Keys) + @('nr_reg_fast.hsaco', 'nr_mh_layout.hsaco', 'nr_resize.hsaco', 'colorcs.cso', 'depthcs.cso', 'motioncs.cso', 'dlssnr_on_amd_weights.bin') + $(if ($ComIni) { @('dlssnr_on_amd.ini') } else { @() })) {
        $p = Join-Path $mod $k
        $arq[$k] = if (Test-Path -LiteralPath $p) { (Get-FileHash -LiteralPath $p -Algorithm SHA256).Hash } else { 'E16BCF15E16E13F527491CDF7845B2FE6521A738D8F7C9C721866A8496E1FC8E' }
    }
    $doc = [ordered]@{ produto = 'AMDLSS5 MOD'; versao = 'teste'; arquivos = $arq }
    # lista de jogos testados (JSON do testados.json) + "testados" (SHA-256) no pacote.json
    if ($Testados) {
        [IO.File]::WriteAllText((Join-Path $dir 'testados.json'), $Testados)
        $doc.testados = (Get-FileHash -LiteralPath (Join-Path $dir 'testados.json') -Algorithm SHA256).Hash
    }
    if (-not $SemReframework) {
        New-Item -ItemType Directory -Force -Path (Join-Path $dir 'reframework') | Out-Null
        Copy-Item -LiteralPath (Join-Path $sys 'dinput8.dll') -Destination (Join-Path $dir 'reframework\dinput8.dll')
        $doc.reframework = [ordered]@{ arquivo = 'reframework/dinput8.dll'; sha256 = (Get-FileHash -LiteralPath (Join-Path $dir 'reframework\dinput8.dll') -Algorithm SHA256).Hash; revisao = 'teste' }
    }
    # extras por jogo: @{ 're9' = @{ 'reframework/plugins/x.dll' = <semente de bytes> } } -> payload\porjogo\re9\... + "porjogo" no pacote.json
    if ($PorJogo.Count -or $PorJogoArq.Count) {
        $pj = [ordered]@{}
        foreach ($fonte in @($PorJogo, $PorJogoArq)) {
            foreach ($exe in $fonte.Keys) {
                if (-not $pj.Contains($exe)) { $pj[$exe] = [ordered]@{} }
                foreach ($rel in $fonte[$exe].Keys) {
                    $dest = Join-Path $dir (Join-Path 'porjogo' (Join-Path $exe ($rel -replace '/', '\')))
                    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $dest) | Out-Null
                    if ($fonte[$exe][$rel] -is [string]) { Copy-Item -LiteralPath $fonte[$exe][$rel] -Destination $dest } else { Bytes $dest 3000 $fonte[$exe][$rel] }
                    $pj[$exe][$rel] = (Get-FileHash -LiteralPath $dest -Algorithm SHA256).Hash
                }
            }
        }
        $doc.porjogo = $pj
    }
    if ($IniJogo.Count) {
        New-Item -ItemType Directory -Force -Path (Join-Path $dir 'ini') | Out-Null
        $inis = [ordered]@{}
        foreach ($nomeIni in $IniJogo.Keys) {
            $c = Join-Path $dir (Join-Path 'ini' $nomeIni)
            [IO.File]::WriteAllBytes($c, [Text.Encoding]::GetEncoding(28591).GetBytes($IniJogo[$nomeIni]))
            $inis[$nomeIni] = (Get-FileHash -LiteralPath $c -Algorithm SHA256).Hash
        }
        $doc.inis = $inis
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

# versao instalada: antes de instalar nada
$r = Rodar @('--detectar', $re, '--payload', $pay)
Ok 'versao: antes de instalar' ($r.Texto -match 'versao_instalada: -') "texto=$($r.Texto)"

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
# 1.3.0: o mod cria 3 arquivos (INI, amdlss5.dat e nr_orcamento.ini) na raiz e em _storage_ = 6 marcas
Ok 'log e acoes.json ao lado do backup (INI, amdlss5.dat e nr_orcamento.ini marcados como do mod)' ($sess.Count -eq 1 -and (Test-Path (Join-Path $sess[0].FullName 'instalacao.log')) -and ([regex]::Matches($acoes, 'criado_pelo_mod')).Count -eq 6)

# versao instalada: depois de instalar; sessao antiga (sem versao.txt) = '?'
$r = Rodar @('--detectar', $re, '--payload', $pay)
Ok 'versao: depois de instalar' ($r.Texto -match '(?m)versao_instalada: teste\s*$') "texto=$($r.Texto)"
Remove-Item -LiteralPath (Join-Path $sess[0].FullName 'versao.txt')
$r = Rodar @('--detectar', $re, '--payload', $pay)
Ok 'versao: sessao antiga sem versao.txt' ($r.Texto -match 'versao_instalada: \?') "texto=$($r.Texto)"
[IO.File]::WriteAllText((Join-Path $sess[0].FullName 'versao.txt'), 'teste', (New-Object Text.UTF8Encoding($false)))

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

# T4b 1.5.2: versao nova por cima da velha = uma pasta de backup so (sem o helper velho), desinstalar volta ao original
$payB = Join-Path $Raiz 'payload-v2'
Copy-Item -LiteralPath $pay -Destination $payB -Recurse
Copy-Item -LiteralPath (Join-Path $sys 'dcomp.dll') -Destination (Join-Path $payB 'mod\nr_native_adapter.dll') -Force
Bytes (Join-Path $payB 'mod\nr_extra.hsaco') 3000 31
$docB = Get-Content -Raw (Join-Path $payB 'pacote.json') | ConvertFrom-Json
$docB.arquivos.'nr_native_adapter.dll' = (Get-FileHash (Join-Path $payB 'mod\nr_native_adapter.dll')).Hash
$docB.arquivos | Add-Member -NotePropertyName 'nr_extra.hsaco' -NotePropertyValue (Get-FileHash (Join-Path $payB 'mod\nr_extra.hsaco')).Hash
$docB.versao = 'teste-v2'
[IO.File]::WriteAllText((Join-Path $payB 'pacote.json'), ($docB | ConvertTo-Json -Depth 4))
$reU = Novo-RE (Join-Path $Raiz 'RE Atualizar')
$fotoU = Foto $reU
$r1 = Rodar @('--instalar', $reU, '--payload', $pay)
Simula-Jogo $reU
$r2 = Rodar @('--instalar', $reU, '--payload', $payB)
$sessU = @(Get-ChildItem -LiteralPath (Join-Path $reU 'AMDLSS5_backup') -Directory)
$helperVelho = (Get-FileHash (Join-Path $pay 'mod\nr_native_adapter.dll')).Hash
$copiasVelho = @(Get-ChildItem -LiteralPath (Join-Path $reU 'AMDLSS5_backup') -Recurse -File | Where-Object { (Get-FileHash $_.FullName).Hash -eq $helperVelho })
$novoOk = (Get-FileHash (Join-Path $reU 'nr_native_adapter.dll')).Hash -eq $docB.arquivos.'nr_native_adapter.dll' -and (Test-Path (Join-Path $reU '_storage_\nr_extra.hsaco'))
Ok 'atualizar por cima: 1 pasta de backup, sem o helper da versao velha' ($r1.Codigo -eq 0 -and $r2.Codigo -eq 0 -and $novoOk -and $sessU.Count -eq 1 -and $copiasVelho.Count -eq 0) "codigo=$($r1.Codigo)/$($r2.Codigo) sessoes=$($sessU.Count) copias_velho=$($copiasVelho.Count)"
$rv = Rodar @('--detectar', $reU, '--payload', $payB)
Ok 'atualizar por cima: 1 sessao e --detectar mostra a versao nova' ($rv.Codigo -eq 0 -and $sessU.Count -eq 1 -and $rv.Texto -match '(?m)versao_instalada: teste-v2\s*$') "sessoes=$($sessU.Count) texto=$($rv.Texto)"
$r = Rodar @('--desinstalar', $reU, '--payload', $payB)
Ok 'desinstalar depois de atualizar: byte a byte' ($r.Codigo -eq 0 -and (Iguais $fotoU (Foto $reU))) "codigo=$($r.Codigo) $(Dif $fotoU (Foto $reU))"

# T4c P0-4 (19/10): o jogador troca o nosso version.dll pelo de OUTRO mod depois de instalar e atualiza o AMDLSS5 por cima.
# A consolidacao das pastas de backup nao pode perder a copia do arquivo dele: desinstalar devolve o version.dll dele.
$reX = Novo-RE (Join-Path $Raiz 'RE Terceiro')
$fotoX = Foto $reX
$rx1 = Rodar @('--instalar', $reX, '--payload', $pay)
Simula-Jogo $reX
Copy-Item -LiteralPath (Join-Path $sys 'd3d11.dll') -Destination (Join-Path $reX 'version.dll') -Force
$rx2 = Rodar @('--instalar', $reX, '--payload', $payB)
$sessX = @(Get-ChildItem -LiteralPath (Join-Path $reX 'AMDLSS5_backup') -Directory)
$rx3 = Rodar @('--desinstalar', $reX, '--payload', $payB)
$esperadoX = $fotoX.Clone()
$esperadoX['version.dll'] = (Get-FileHash -LiteralPath (Join-Path $sys 'd3d11.dll')).Hash
Ok 'P0-4: version.dll de outro mod posto depois da instalacao volta ao desinstalar (com atualizacao no meio)' ($rx1.Codigo -eq 0 -and $rx2.Codigo -eq 0 -and $rx3.Codigo -eq 0 -and $sessX.Count -eq 1 -and (Iguais $esperadoX (Foto $reX))) "codigo=$($rx1.Codigo)/$($rx2.Codigo)/$($rx3.Codigo) sessoes=$($sessX.Count) $(Dif $esperadoX (Foto $reX))"

# T4d P1-15 (19/10): atualizar troca o INI que o MOD criou pelo do pacote (valores novos chegam) e mantem so as chaves do
# jogador (idioma, contador compacto, limite de FPS, filtros, F10); desinstalar volta ao original (sem INI).
$iniV1 = "[DlssNrOnAmd]`r`nEnabled=0`r`nScale=0.03125`r`nTemporal=1`r`nNeuralPostPercent=100`r`n"
$iniV2 = "[DlssNrOnAmd]`r`nEnabled=0`r`nScale=0.025`r`nTemporal=0`r`nNeuralPostPercent=67`r`nNeuralChaveNova=7`r`nNeuralMenuCompacto=2`r`n"
$payI1 = Novo-Payload (Join-Path $Raiz 'payload-ini-v1') -IniTexto $iniV1
$payI2 = Novo-Payload (Join-Path $Raiz 'payload-ini-v2') -IniTexto $iniV2
$reI = Novo-RE (Join-Path $Raiz 'RE INI Atualizar')
$fotoI = Foto $reI
$ri1 = Rodar @('--instalar', $reI, '--payload', $payI1, '--idioma', 'en')
foreach ($d in $reI, (Join-Path $reI '_storage_')) {
    # o jogador no menu: idioma, contador compacto escondido, limite de FPS, filtro, F10 ligado; e mexeu na forca (regulagem)
    [IO.File]::WriteAllText((Join-Path $d 'dlssnr_on_amd.ini'), "[DlssNrOnAmd]`r`nEnabled=1`r`nScale=0.05`r`nTemporal=1`r`nNeuralPostPercent=100`r`nMenuLanguage=es`r`nNeuralMenuCompacto=0`r`nNeuralFrameCap=90`r`nNeuralFiltroCas=1`r`nLocalStructure=1.300`r`nNeuralStyle=2`r`nNeuralPasses=1.5`r`nInline=0`r`n")
}
$ri2 = Rodar @('--instalar', $reI, '--payload', $payI2)
$okIni = $true; $faltou = @()
foreach ($d in $reI, (Join-Path $reI '_storage_')) {
    $t = [IO.File]::ReadAllText((Join-Path $d 'dlssnr_on_amd.ini'))
    foreach ($par in 'Scale=0.025', 'Temporal=0', 'NeuralPostPercent=67', 'NeuralChaveNova=7', 'MenuLanguage=es', 'NeuralMenuCompacto=0', 'NeuralFrameCap=90', 'NeuralFiltroCas=1', 'Enabled=1', 'LocalStructure=1.300', 'NeuralStyle=2', 'NeuralPasses=1.5') {
        if ($t -notmatch ('(?m)^' + [regex]::Escape($par) + '\r?$')) { $okIni = $false; $faltou += $par }
    }
    if ([regex]::Matches($t, '(?m)^NeuralMenuCompacto=').Count -ne 1 -or $t -match 'Scale=0\.05' -or $t -match '(?m)^Inline=') { $okIni = $false; $faltou += 'duplicada/velha' }
}
$sessI = @(Get-ChildItem -LiteralPath (Join-Path $reI 'AMDLSS5_backup') -Directory)
Ok 'P1-15: atualizar troca o INI do mod pelo do pacote e mantem as chaves do jogador' ($ri1.Codigo -eq 0 -and $ri2.Codigo -eq 0 -and $okIni -and $sessI.Count -eq 1) "codigo=$($ri1.Codigo)/$($ri2.Codigo) sessoes=$($sessI.Count) $($faltou -join ',')"
$ri3 = Rodar @('--desinstalar', $reI, '--payload', $payI2)
Ok 'P1-15: desinstalar depois da troca do INI: byte a byte' ($ri3.Codigo -eq 0 -and (Iguais $fotoI (Foto $reI))) "codigo=$($ri3.Codigo) $(Dif $fotoI (Foto $reI))"

# T4e pasta de backup sem acoes.json (sobra de uma limpeza que falhou, com arquivo somente leitura): nao desliga a
# consolidacao nem deixa a desinstalacao "parcial"; e apagada.
$reV = Novo-RE (Join-Path $Raiz 'RE Sobra')
$fotoV = Foto $reV
$rv1 = Rodar @('--instalar', $reV, '--payload', $pay)
$sobra = Join-Path $reV 'AMDLSS5_backup\20000101-000000'   # a mais velha: antes virava a "base" e desligava a consolidacao
New-Item -ItemType Directory -Force -Path $sobra | Out-Null
Bytes (Join-Path $sobra 'arquivo-sobra.bin') 100 3
Set-ItemProperty -LiteralPath (Join-Path $sobra 'arquivo-sobra.bin') -Name IsReadOnly -Value $true
Simula-Jogo $reV
$rv2 = Rodar @('--instalar', $reV, '--payload', $payB)
$sessV = @(Get-ChildItem -LiteralPath (Join-Path $reV 'AMDLSS5_backup') -Directory)
Ok 'pasta de backup sem acoes.json (somente leitura) nao trava a consolidacao e e apagada' ($rv1.Codigo -eq 0 -and $rv2.Codigo -eq 0 -and $sessV.Count -eq 1 -and -not (Test-Path -LiteralPath $sobra)) "codigo=$($rv1.Codigo)/$($rv2.Codigo) sessoes=$($sessV.Count)"
$sobra2 = Join-Path $reV 'AMDLSS5_backup\20000101-000001'
New-Item -ItemType Directory -Force -Path $sobra2 | Out-Null
Bytes (Join-Path $sobra2 'arquivo-sobra.bin') 100 4
Set-ItemProperty -LiteralPath (Join-Path $sobra2 'arquivo-sobra.bin') -Name IsReadOnly -Value $true
$rv3 = Rodar @('--desinstalar', $reV, '--payload', $payB)
Ok 'desinstalar com sobra somente leitura: codigo 0 (nao parcial), byte a byte' ($rv3.Codigo -eq 0 -and (Iguais $fotoV (Foto $reV))) "codigo=$($rv3.Codigo) $(Dif $fotoV (Foto $reV))"

# T4f acoes.json adulterado: backup apontando para fora da pasta da sessao e recusado (nada de fora e copiado no jogo)
$reT = Novo-RE (Join-Path $Raiz 'RE Adulterado')
$rt1 = Rodar @('--instalar', $reT, '--payload', $pay)
$segredo = Join-Path $Raiz 'segredo.bin'
Bytes $segredo 500 61
$pakAntes = (Get-FileHash -LiteralPath (Join-Path $reT 're_chunk_000.pak')).Hash
$sessT = @(Get-ChildItem -LiteralPath (Join-Path $reT 'AMDLSS5_backup') -Directory)[0].FullName
$jsonT = Join-Path $sessT 'acoes.json'
$listaT = New-Object System.Collections.ArrayList
foreach ($x in ([IO.File]::ReadAllText($jsonT) | ConvertFrom-Json)) { [void]$listaT.Add($x) }
[void]$listaT.Add([pscustomobject]@{ acao = 'trocado'; caminho = 're_chunk_000.pak'; backup = '..\..\..\segredo.bin'; anterior = '00'; novo = '11' })
[IO.File]::WriteAllText($jsonT, (ConvertTo-Json -InputObject @($listaT.ToArray()) -Depth 4))
$rt2 = Rodar @('--desinstalar', $reT, '--payload', $pay)
Ok 'acoes.json com backup fora da sessao: recusado (parcial, nada de fora copiado)' ($rt1.Codigo -eq 0 -and $rt2.Codigo -eq 1 -and (Get-FileHash -LiteralPath (Join-Path $reT 're_chunk_000.pak')).Hash -eq $pakAntes -and $rt2.Texto -match 'recusad|refused|rechazad') "codigo=$($rt1.Codigo)/$($rt2.Codigo)"

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

# T6c INI do jogador em UTF-16 com duas secoes: as chaves que faltam entram NA SUA secao, na mesma codificacao; uma chave
# que so existe em outra secao nao conta como presente
$payS = Novo-Payload (Join-Path $Raiz 'payload-secoes') -IniTexto "[DlssNrOnAmd]`r`nEnabled=1`r`nScale=0.03125`r`nTemporal=1`r`n[Outra]`r`nCor=2`r`n"
$reS16 = Novo-RE (Join-Path $Raiz 'RE INI Unicode')
$iniS16 = Join-Path $reS16 'dlssnr_on_amd.ini'
[IO.File]::WriteAllText($iniS16, "[DlssNrOnAmd]`r`nScale=0.05`r`n; comentario: a" + [char]0xE7 + [char]0xE3 + "o`r`n[Outra]`r`nTemporal=9`r`n", [Text.Encoding]::Unicode)
$fotoS16 = Foto $reS16
$r = Rodar @('--instalar', $reS16, '--payload', $payS)
$bS = [IO.File]::ReadAllBytes($iniS16)
$esperadoS = "[DlssNrOnAmd]`r`nScale=0.05`r`nEnabled=1`r`nTemporal=1`r`n; comentario: a" + [char]0xE7 + [char]0xE3 + "o`r`n[Outra]`r`nTemporal=9`r`nCor=2`r`n"
$okS = $bS.Length -gt 2 -and $bS[0] -eq 0xFF -and $bS[1] -eq 0xFE -and [Text.Encoding]::Unicode.GetString($bS, 2, $bS.Length - 2) -eq $esperadoS
$r2 = Rodar @('--desinstalar', $reS16, '--payload', $payS)
Ok 'INI do jogador (UTF-16, 2 secoes): chaves novas na secao certa, mesma codificacao; desinstala byte a byte' ($r.Codigo -eq 0 -and $okS -and $r2.Codigo -eq 0 -and (Iguais $fotoS16 (Foto $reS16))) "codigo=$($r.Codigo)/$($r2.Codigo) ini_ok=$okS"

# T6d INI de um jogo (payload\ini\<exe>.ini) conferido pelo "inis" do pacote.json: certo instala, adulterado recusa
$payJg = Novo-Payload (Join-Path $Raiz 'payload-ini-jogo') -ComIni
New-Item -ItemType Directory -Force -Path (Join-Path $payJg 'ini') | Out-Null
$iniJg = Join-Path $payJg 'ini\re9.ini'
[IO.File]::WriteAllText($iniJg, "[DlssNrOnAmd]`r`nEnabled=0`r`nScale=0.02`r`n")
$docJg = Get-Content -Raw (Join-Path $payJg 'pacote.json') | ConvertFrom-Json
$docJg | Add-Member -NotePropertyName 'inis' -NotePropertyValue ([pscustomobject]@{ 're9.ini' = (Get-FileHash -LiteralPath $iniJg).Hash })
[IO.File]::WriteAllText((Join-Path $payJg 'pacote.json'), ($docJg | ConvertTo-Json -Depth 4))
$reJg = Novo-RE (Join-Path $Raiz 'RE INI Jogo')
$fotoJg = Foto $reJg
$r = Rodar @('--instalar', $reJg, '--payload', $payJg)
$okJg = (Test-Path (Join-Path $reJg 'dlssnr_on_amd.ini')) -and (Get-FileHash (Join-Path $reJg 'dlssnr_on_amd.ini')).Hash -eq (Get-FileHash $iniJg).Hash
$r2 = Rodar @('--desinstalar', $reJg, '--payload', $payJg)
Ok 'INI do jogo com hash no pacote: instalado e desinstalado byte a byte' ($r.Codigo -eq 0 -and $okJg -and $r2.Codigo -eq 0 -and (Iguais $fotoJg (Foto $reJg))) "codigo=$($r.Codigo)/$($r2.Codigo)"
[IO.File]::AppendAllText($iniJg, "NeuralGravarF9=1`r`n")
$reJg2 = Novo-RE (Join-Path $Raiz 'RE INI Jogo Adulterado')
$fotoJg2 = Foto $reJg2
$r = Rodar @('--instalar', $reJg2, '--payload', $payJg)
Ok 'INI do jogo adulterado: recusa (codigo 1, nada escrito)' ($r.Codigo -eq 1 -and (Iguais $fotoJg2 (Foto $reJg2)) -and $r.Texto -match 'ini/re9.ini') "codigo=$($r.Codigo)"

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
# rev-diag (22/10, review M-17): desinstalar com o jogo aberto tambem recusa sem tocar em nada (GuardaJogoAberto do desinstalar)
$reAb = Novo-RE (Join-Path $Raiz 'RE Aberto')
$fotoAbAntes = Foto $reAb
$r = Rodar @('--instalar', $reAb, '--payload', $pay)
Ok 'jogo aberto (desinstalar): instalado antes' ($r.Codigo -eq 0) "codigo=$($r.Codigo)"
$fotoAb = Foto $reAb
$proc = Start-Process -FilePath (Join-Path $reAb 're9.exe') -ArgumentList '/T', '30', '/D', 'S', '/C', 'SN' -PassThru -WindowStyle Hidden
Start-Sleep -Milliseconds 700
$r = Rodar @('--desinstalar', $reAb, '--payload', $pay)
try { $proc.Kill() } catch { }
$proc.WaitForExit(5000) | Out-Null
Ok 'jogo aberto: desinstalar recusa (codigo 3, nada mexido)' ($r.Codigo -eq 3 -and (Iguais $fotoAb (Foto $reAb))) "codigo=$($r.Codigo)"
$r = Rodar @('--desinstalar', $reAb, '--payload', $pay)
Ok 'jogo fechado: desinstalar volta byte a byte' ($r.Codigo -eq 0 -and (Iguais $fotoAbAntes (Foto $reAb))) "codigo=$($r.Codigo)"

# T9 sem permissao de escrita
$prot = Novo-RE (Join-Path $Raiz 'RE Protegido')
$fotoProt = Foto $prot
& icacls $prot /deny "${env:USERNAME}:(WD,AD)" | Out-Null
try { $r = Rodar @('--instalar', $prot, '--payload', $pay) }
finally { & icacls $prot /remove:d $env:USERNAME | Out-Null }   # sempre: senao a pasta de teste nao sai
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

# T12b review P2 (26/10): somente leitura. (1) version.dll instalado e depois marcado somente leitura (antivirus, sincronizador,
# jogador): desinstalar tira o atributo e volta byte a byte. (2) arquivo somente leitura no PACOTE: o File.Copy levava o atributo
# para o jogo; agora o instalado sai sem ele.
$reRo = Novo-RE (Join-Path $Raiz 'RE Somente Leitura')
$fotoRo = Foto $reRo
$r = Rodar @('--instalar', $reRo, '--payload', $pay)
foreach ($c in (Join-Path $reRo 'version.dll'), (Join-Path $reRo '_storage_\version.dll')) { Set-ItemProperty -LiteralPath $c -Name IsReadOnly -Value $true }
$r2 = Rodar @('--desinstalar', $reRo, '--payload', $pay)
Ok 'version.dll somente leitura no jogo: desinstalar volta byte a byte (codigo 0)' ($r.Codigo -eq 0 -and $r2.Codigo -eq 0 -and (Iguais $fotoRo (Foto $reRo))) "codigo=$($r.Codigo)/$($r2.Codigo) $(Dif $fotoRo (Foto $reRo))"
$payRo = Join-Path $Raiz 'payload-somente-leitura'
Copy-Item -LiteralPath $pay -Destination $payRo -Recurse
Set-ItemProperty -LiteralPath (Join-Path $payRo 'mod\version.dll') -Name IsReadOnly -Value $true
$reRo2 = Novo-RE (Join-Path $Raiz 'RE Pacote Somente Leitura')
$fotoRo2 = Foto $reRo2
$r = Rodar @('--instalar', $reRo2, '--payload', $payRo)
$roInst = @((Join-Path $reRo2 'version.dll'), (Join-Path $reRo2 '_storage_\version.dll') | Where-Object { -not (Test-Path -LiteralPath $_) -or (Get-Item -LiteralPath $_).IsReadOnly })
$r2 = Rodar @('--desinstalar', $reRo2, '--payload', $payRo)
Ok 'version.dll somente leitura no pacote: instalado sem o atributo; desinstalar byte a byte' ($r.Codigo -eq 0 -and $roInst.Count -eq 0 -and $r2.Codigo -eq 0 -and (Iguais $fotoRo2 (Foto $reRo2))) "codigo=$($r.Codigo)/$($r2.Codigo) somente_leitura=$($roInst -join ',') $(Dif $fotoRo2 (Foto $reRo2))"

# T12c review P2 (26/10): um arquivo do mod travado (aberto sem compartilhar) na desinstalacao nao derruba o resto: tudo o mais
# volta, o resultado e parcial (codigo 1, o acoes.json fica) e uma nova desinstalacao, com o arquivo livre, termina o que faltou
$reLk = Novo-RE (Join-Path $Raiz 'RE Travado Desinstalar')
$fotoLk = Foto $reLk
$r = Rodar @('--instalar', $reLk, '--payload', $pay)
$trava = [IO.File]::Open((Join-Path $reLk 'nr_resize.hsaco'), 'Open', 'Read', 'None')
try { $r2 = Rodar @('--desinstalar', $reLk, '--payload', $pay) } finally { $trava.Close() }
$depoisLk = Foto $reLk
$sobraLk = @($depoisLk.Keys | Where-Object { -not $fotoLk.ContainsKey($_) -and $_ -ne 'nr_resize.hsaco' -and $_ -notmatch '^(DIR:)?AMDLSS5_backup' })
$mudouLk = @($fotoLk.Keys | Where-Object { -not $depoisLk.ContainsKey($_) -or $depoisLk[$_] -ne $fotoLk[$_] })
$livros = @(Get-ChildItem -LiteralPath (Join-Path $reLk 'AMDLSS5_backup') -Recurse -File -Filter 'acoes.json' -ErrorAction SilentlyContinue)
Ok 'arquivo travado na desinstalacao: o resto volta, parcial (codigo 1), acoes.json fica' ($r.Codigo -eq 0 -and $r2.Codigo -eq 1 -and $sobraLk.Count -eq 0 -and $mudouLk.Count -eq 0 -and $depoisLk.ContainsKey('nr_resize.hsaco') -and $livros.Count -ge 1 -and $r2.Texto -match '(desfazer|undo|deshacer) nr_resize\.hsaco') "codigo=$($r.Codigo)/$($r2.Codigo) sobra=$($sobraLk -join ',') mudou=$($mudouLk -join ',') livros=$($livros.Count)"
$r3 = Rodar @('--desinstalar', $reLk, '--payload', $pay)
Ok 'arquivo travado na desinstalacao: a 2a desinstalacao termina (codigo 0, byte a byte)' ($r3.Codigo -eq 0 -and (Iguais $fotoLk (Foto $reLk))) "codigo=$($r3.Codigo) $(Dif $fotoLk (Foto $reLk))"

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
    $inisJogo = @(Get-ChildItem -LiteralPath $reR -Recurse -File -Filter '*.ini' | Where-Object { $_.FullName -notmatch 'AMDLSS5_backup' })
    $iniMan = $man.arquivos.PSObject.Properties['dlssnr_on_amd.ini']
    if ($iniMan) {
        # 1.3.0: pacote com INI. No RE (sem payload\ini\re9.ini) vai o INI padrao do pacote, na raiz e em _storage_, e nada mais.
        $iniRe = Join-Path $PayloadReal 'ini\re9.ini'
        $esperado = if (Test-Path -LiteralPath $iniRe) { (Get-FileHash -LiteralPath $iniRe).Hash } else { $iniMan.Value }
        $iniOk = $inisJogo.Count -eq 2
        foreach ($alvo in $reR, (Join-Path $reR '_storage_')) {
            $c = Join-Path $alvo 'dlssnr_on_amd.ini'
            if (-not (Test-Path -LiteralPath $c) -or (Get-FileHash -LiteralPath $c).Hash -ne $esperado) { $iniOk = $false }
        }
        Ok 'pacote real: INI do pacote no RE (raiz e _storage_), nenhum outro .ini' $iniOk "jogo=$($inisJogo.Count)"
    } else {
        $inisPacote = @(Get-ChildItem -LiteralPath (Split-Path -Parent $PayloadReal) -Recurse -File -Filter '*.ini')
        Ok 'pacote real sem nenhum .ini (pacote e jogo depois de instalar)' ($inisPacote.Count -eq 0 -and $inisJogo.Count -eq 0) "pacote=$($inisPacote.Count) jogo=$($inisJogo.Count)"
    }
    # 1.6.0 (biblioteca): plugin publico do RE (payload\porjogo\re9\...) sem log, so no RE Requiem; INI do RE com as chaves de RT.
    $plugReal = Join-Path $PayloadReal 'porjogo\re9\reframework\plugins\nr_jogador_refw.dll'
    if (Test-Path -LiteralPath $plugReal) {
        $bytesP = [IO.File]::ReadAllBytes($plugReal)
        $achou = @()
        foreach ($txt in '.log', 'nr_render_sonda') {
            if ([Text.Encoding]::ASCII.GetString($bytesP).Contains($txt) -or [Text.Encoding]::Unicode.GetString($bytesP).Contains($txt)) { $achou += $txt }
        }
        Ok 'pacote real: plugin publico sem as strings .log e nr_render_sonda' ($achou.Count -eq 0) ("achou: " + ($achou -join ','))
        $plugInst = Join-Path $reR 'reframework\plugins\nr_jogador_refw.dll'
        Ok 'pacote real: plugin so na pasta do RE (nada em _storage_)' ((Test-Path -LiteralPath $plugInst) -and (Get-FileHash -LiteralPath $plugInst).Hash -eq (Get-FileHash -LiteralPath $plugReal).Hash -and -not (Test-Path -LiteralPath (Join-Path $reR '_storage_\reframework'))) 'plugin ausente ou diferente'
        $cpP = Novo-CP (Join-Path $Raiz 'CP Pacote Real')
        $rc = Rodar @('--instalar', $cpP, '--payload', $PayloadReal)
        Ok 'pacote real: Cyberpunk nao recebe o plugin do RE' ($rc.Codigo -eq 0 -and -not (Test-Path -LiteralPath (Join-Path $cpP 'bin\x64\reframework')) -and -not (Test-Path -LiteralPath (Join-Path $cpP 'reframework'))) "codigo=$($rc.Codigo)"
        $rd = Rodar @('--desinstalar', $cpP, '--payload', $PayloadReal)
        $iniRe9 = Join-Path $PayloadReal 'ini\re9.ini'
        if (Test-Path -LiteralPath $iniRe9) {
            $t = [IO.File]::ReadAllText($iniRe9, [Text.Encoding]::GetEncoding(28591))
            $faltam = @(@{GiHistorico=10; GiResolucao=1; Denoise=1; PathGuiding=1; Shading=1; Qualidade=2; Pele=1}.GetEnumerator() | Where-Object { $t -notmatch ("(?m)^NeuralReRt$($_.Key)=$($_.Value)\s*$") } | ForEach-Object { $_.Key })
            Ok 'pacote real: re9.ini com as 7 chaves NeuralReRt*' ($faltam.Count -eq 0) ("faltam: " + ($faltam -join ','))
        }
    }
    # 21/10 (revisao final): os INIs publicos (re9.ini e os outros) sem log/gravacao ligados, sem comentarios internos e sem
    # as chaves inertes do YuNet; listas tiradas do proprio ferramentas\MontarPacote.cs (Desligar e Permitidas)
    $fonteMp = [IO.File]::ReadAllText((Join-Path $PSScriptRoot 'ferramentas\MontarPacote.cs'))
    $listaMp = { param($nome) @([regex]::Matches([regex]::Match($fonteMp, "string\[\] $nome = \{([^}]*)\}").Groups[1].Value, '"([A-Za-z0-9_]+)"') | ForEach-Object { $_.Groups[1].Value }) }
    $desligarMp = & $listaMp 'Desligar'; $permitidasMp = & $listaMp 'Permitidas'
    $inisReais = @(Get-ChildItem -LiteralPath (Join-Path $PayloadReal 'ini') -File -Filter '*.ini' -ErrorAction SilentlyContinue) + @(Get-ChildItem -LiteralPath (Join-Path $PayloadReal 'mod') -File -Filter '*.ini' -ErrorAction SilentlyContinue)
    foreach ($iniR in $inisReais) {
        $linhasR = [IO.File]::ReadAllText($iniR.FullName, [Text.Encoding]::GetEncoding(28591)) -split "`r`n|`r|`n"
        $ruins = @()
        foreach ($l in $linhasR) {
            $t = $l.Trim()
            if ($t.StartsWith(';')) { $ruins += "comentario[$t]"; continue }
            if ($t -notmatch '^([^=]+)=(.*)$') { continue }
            $k = $Matches[1].Trim(); $v = $Matches[2].Trim()
            if ($k -like 'NeuralRostoJogadorYunet*') { $ruins += $k; continue }
            $zero = $v -match '^[0-9.+-]+$' -and ($v -as [double]) -eq 0
            if ($desligarMp -contains $k -and $v -ne '0') { $ruins += "$k=$v" }
            elseif ($permitidasMp -notcontains $k -and $k -cmatch 'Log|Grav|Dump|Trace|Diag|Captura|Sonda' -and -not $zero) { $ruins += "$k=$v" }
        }
        Ok "pacote real: $($iniR.Directory.Name)\$($iniR.Name) sem log/gravacao ligados, sem ';' e sem NeuralRostoJogadorYunet* ($($desligarMp.Count) chaves da lista)" ($desligarMp.Count -ge 24 -and $ruins.Count -eq 0) ("achou: " + ($ruins -join ', '))
    }

    Simula-Jogo $reR
    $r = Rodar @('--desinstalar', $reR, '--payload', $PayloadReal)
    Ok 'pacote real: desinstalar byte a byte' ($r.Codigo -eq 0 -and (Iguais $fotoR (Foto $reR))) "codigo=$($r.Codigo) $(Dif $fotoR (Foto $reR))"

    # 1.5.2: INI proprio de um jogo (payload\ini\<exe>.ini), ex. witcher3.ini, vai so para esse jogo.
    foreach ($iniJogo in @(Get-ChildItem -LiteralPath (Join-Path $PayloadReal 'ini') -File -Filter '*.ini' -ErrorAction SilentlyContinue)) {
        $exeNome = $iniJogo.BaseName + '.exe'
        $dirJ = Join-Path $Raiz ("Jogo Real " + $iniJogo.BaseName)
        New-Item -ItemType Directory -Force -Path $dirJ | Out-Null
        Copy-Item -LiteralPath $exeStub -Destination (Join-Path $dirJ $exeNome)
        Copy-Item -LiteralPath (Join-Path $sys 'dcomp.dll') -Destination (Join-Path $dirJ 'amd_fidelityfx_dx12.dll')
        $fotoJ = Foto $dirJ
        $r = Rodar @('--instalar', (Join-Path $dirJ $exeNome), '--payload', $PayloadReal)
        $c = Join-Path $dirJ 'dlssnr_on_amd.ini'
        $okJ = $r.Codigo -eq 0 -and (Test-Path -LiteralPath $c) -and (Get-FileHash -LiteralPath $c).Hash -eq (Get-FileHash -LiteralPath $iniJogo.FullName).Hash
        Ok "pacote real: $($iniJogo.Name) instalado em $exeNome" $okJ "codigo=$($r.Codigo)"
        Simula-Jogo $dirJ
        $r = Rodar @('--desinstalar', $dirJ, '--payload', $PayloadReal)
        Ok "pacote real: desinstalar $exeNome byte a byte" ($r.Codigo -eq 0 -and (Iguais $fotoJ (Foto $dirJ))) "codigo=$($r.Codigo) $(Dif $fotoJ (Foto $dirJ))"
    }
    # rdr2 (26/10): RDR2 limpo (so o FSR 2 do jogo, sem DLL da FidelityFX API) com o pacote real: helper, proxy, RDR2.ini com
    # NeuralFsr2ParaFfx=1 e os 2 extras (loader 2.3.0, upscaler 4.1.1); desinstalar = byte a byte
    if (Test-Path -LiteralPath (Join-Path $PayloadReal 'ini\RDR2.ini')) {
        $rdR = Join-Path $Raiz 'RDR2 Pacote Real'
        New-Item -ItemType Directory -Force -Path $rdR | Out-Null
        Copy-Item -LiteralPath $exeStub -Destination (Join-Path $rdR 'RDR2.exe')
        Bytes (Join-Path $rdR 'ffx_fsr2_api_x64.dll') 5000 91
        $fotoRR = Foto $rdR
        $rdet = Rodar @('--detectar', $rdR, '--payload', $PayloadReal)
        $r = Rodar @('--instalar', (Join-Path $rdR 'RDR2.exe'), '--payload', $PayloadReal)
        $okRR = $r.Codigo -eq 0 -and $rdet.Texto -match 'dx12: True\s+fsr: split 4\.1\.1'
        foreach ($nArq in 'version.dll', 'nr_native_adapter.dll') { if (-not (Test-Path -LiteralPath (Join-Path $rdR $nArq)) -or (Get-FileHash -LiteralPath (Join-Path $rdR $nArq)).Hash -ne $man.arquivos.$nArq) { $okRR = $false } }
        $versoesRR = @()
        foreach ($nArq in 'amd_fidelityfx_loader_dx12.dll', 'amd_fidelityfx_upscaler_dx12.dll') {
            $c = Join-Path $rdR $nArq
            if (-not (Test-Path -LiteralPath $c) -or (Get-FileHash -LiteralPath $c).Hash -ne $man.porjogo.rdr2.$nArq) { $okRR = $false } else { $versoesRR += (Get-Item -LiteralPath $c).VersionInfo.FileVersion }
        }
        if ($versoesRR.Count -ne 2 -or $versoesRR[0] -notlike '2.3.0*' -or $versoesRR[1] -notlike '4.1.1*') { $okRR = $false }
        $iniRR = Join-Path $rdR 'dlssnr_on_amd.ini'
        $okIniRR = (Test-Path -LiteralPath $iniRR) -and (Get-FileHash -LiteralPath $iniRR).Hash -eq (Get-FileHash -LiteralPath (Join-Path $PayloadReal 'ini\RDR2.ini')).Hash -and ([IO.File]::ReadAllText($iniRR) -match '(?m)^NeuralFsr2ParaFfx=1\s*$')
        Ok 'pacote real: RDR2 limpo (so FSR 2) recebe helper, proxy, RDR2.ini (NeuralFsr2ParaFfx=1), loader 2.3.0 e upscaler 4.1.1' ($okRR -and $okIniRR) "codigo=$($r.Codigo) versoes=$($versoesRR -join ',') ini=$okIniRR"
        # pacote-160 (27/10): extra repetido entre jogos (DLL de FG do SotTR e do RDR2) vai uma vez so em payload\porjogo\_comum\<SHA>\
        $nFg = 'amd_fidelityfx_framegeneration_dx12.dll'
        if ($man.porjogo.rdr2.$nFg) {
            $shaFg = $man.porjogo.rdr2.$nFg
            $comumFg = Join-Path $PayloadReal "porjogo\_comum\$shaFg\$nFg"
            $semCopias = -not (Test-Path -LiteralPath (Join-Path $PayloadReal "porjogo\rdr2\$nFg")) -and -not (Test-Path -LiteralPath (Join-Path $PayloadReal "porjogo\sottr\$nFg"))
            $fgJogo = Join-Path $rdR $nFg
            $okFg = (Test-Path -LiteralPath $comumFg) -and $semCopias -and (Test-Path -LiteralPath $fgJogo) -and (Get-FileHash -LiteralPath $fgJogo).Hash -eq $shaFg -and $man.porjogo.sottr.$nFg -eq $shaFg
            Ok 'pacote real: DLL de FG uma vez so (porjogo\_comum) e instalada no RDR2 pelo hash' $okFg "comum=$(Test-Path -LiteralPath $comumFg) sem_copias=$semCopias jogo=$(Test-Path -LiteralPath $fgJogo)"
        }
        Simula-Jogo $rdR
        $r = Rodar @('--desinstalar', $rdR, '--payload', $PayloadReal)
        Ok 'pacote real: desinstalar RDR2 byte a byte' ($r.Codigo -eq 0 -and (Iguais $fotoRR (Foto $rdR))) "codigo=$($r.Codigo) $(Dif $fotoRR (Foto $rdR))"
    }
}

# T12a extras por jogo (payload\porjogo\<exe>\<relativo>): so na pasta do exe, so para o exe certo, com backup e desfazer
$relPlug = 'reframework/plugins/nr_jogador_refw.dll'
$payX = Novo-Payload (Join-Path $Raiz 'payload-extras') -PorJogo @{ 're9' = @{ $relPlug = 55 } }
$hashPlug = (Get-FileHash -LiteralPath (Join-Path $payX 'porjogo\re9\reframework\plugins\nr_jogador_refw.dll') -Algorithm SHA256).Hash
$reE = Novo-RE (Join-Path $Raiz 'RE Extras')
$fotoE = Foto $reE
$r = Rodar @('--instalar', $reE, '--payload', $payX)
$plugE = Join-Path $reE 'reframework\plugins\nr_jogador_refw.dll'
Ok 'extras: re9.exe recebe o plugin' ($r.Codigo -eq 0 -and (Test-Path -LiteralPath $plugE) -and (Get-FileHash -LiteralPath $plugE -Algorithm SHA256).Hash -eq $hashPlug -and -not (Test-Path -LiteralPath (Join-Path $reE '_storage_\reframework'))) "codigo=$($r.Codigo)"
Simula-Jogo $reE
$r = Rodar @('--desinstalar', $reE, '--payload', $payX)
Ok 'extras: desinstalar volta byte a byte' ($r.Codigo -eq 0 -and (Iguais $fotoE (Foto $reE))) "codigo=$($r.Codigo) $(Dif $fotoE (Foto $reE))"

$re4 = Novo-RE (Join-Path $Raiz 'RE4 Extras') 're4.exe'
$fotoE4 = Foto $re4
$r = Rodar @('--instalar', $re4, '--payload', $payX)
Ok 'extras: re4.exe nao recebe' ($r.Codigo -eq 0 -and -not (Test-Path -LiteralPath (Join-Path $re4 'reframework')) -and (Test-Path -LiteralPath (Join-Path $re4 'version.dll'))) "codigo=$($r.Codigo)"
Simula-Jogo $re4
$r = Rodar @('--desinstalar', $re4, '--payload', $payX)
Ok 'extras: re4 desinstala byte a byte' ($r.Codigo -eq 0 -and (Iguais $fotoE4 (Foto $re4))) "codigo=$($r.Codigo) $(Dif $fotoE4 (Foto $re4))"

$reP = Novo-RE (Join-Path $Raiz 'RE Plugin Antigo')
New-Item -ItemType Directory -Force -Path (Join-Path $reP 'reframework\plugins') | Out-Null
$plugP = Join-Path $reP 'reframework\plugins\nr_jogador_refw.dll'
Bytes $plugP 2500 77
$hashAntigo = (Get-FileHash -LiteralPath $plugP -Algorithm SHA256).Hash
$fotoP = Foto $reP
$r1 = Rodar @('--instalar', $reP, '--payload', $payX)
$trocou = (Get-FileHash -LiteralPath $plugP -Algorithm SHA256).Hash -eq $hashPlug
Simula-Jogo $reP
$r2 = Rodar @('--desinstalar', $reP, '--payload', $payX)
Ok 'extras: arquivo que ja existia volta' ($r1.Codigo -eq 0 -and $trocou -and $r2.Codigo -eq 0 -and (Get-FileHash -LiteralPath $plugP -Algorithm SHA256).Hash -eq $hashAntigo -and (Iguais $fotoP (Foto $reP))) "codigo=$($r1.Codigo)/$($r2.Codigo) $(Dif $fotoP (Foto $reP))"

$payRuim = Join-Path $Raiz 'payload-extras-ruim'
Copy-Item -LiteralPath $payX -Destination $payRuim -Recurse
[IO.File]::AppendAllText((Join-Path $payRuim 'porjogo\re9\reframework\plugins\nr_jogador_refw.dll'), 'adulterado')
$reR = Novo-RE (Join-Path $Raiz 'RE Extras Ruim')
$fotoR = Foto $reR
$r = Rodar @('--instalar', $reR, '--payload', $payRuim)
Ok 'extras: hash errado recusa sem tocar' ($r.Codigo -eq 1 -and (Iguais $fotoR (Foto $reR))) "codigo=$($r.Codigo) $(Dif $fotoR (Foto $reR))"

# T12d (26/10) Red Dead Redemption 2: FSR 2 na pasta (ffx_fsr2_api_x64.dll; o exe nao importa d3d12.dll) + loader/upscaler da
# FidelityFX API nos extras do pacote = FSR dividido, DX12, instalavel; INI do jogo; DLL da FidelityFX API mais nova do jogador fica.
# Extras com recurso de versao: DLLs do Windows (10.0.x) no lugar do loader/upscaler; "mais nova" = msvcp140.dll (14.x) renomeada.
$extRdr = @{ 'rdr2' = @{ 'amd_fidelityfx_loader_dx12.dll' = (Join-Path $sys 'dcomp.dll'); 'amd_fidelityfx_upscaler_dx12.dll' = (Join-Path $sys 'uxtheme.dll') } }
$iniRdrTexto = "[DlssNrOnAmd]`r`nEnabled=1`r`nScale=0.03125`r`nNeuralFsr2ParaFfx=1`r`n"
$payRdr = Novo-Payload (Join-Path $Raiz 'payload-rdr2') -ComIni -PorJogoArq $extRdr -IniJogo @{ 'RDR2.ini' = $iniRdrTexto }
$manRdr = Get-Content -Raw (Join-Path $payRdr 'pacote.json') | ConvertFrom-Json
function Novo-RDR2([string]$dir) {
    New-Item -ItemType Directory -Force -Path (Join-Path $dir 'x64') | Out-Null
    Copy-Item -LiteralPath $exeStub -Destination (Join-Path $dir 'RDR2.exe')
    Bytes (Join-Path $dir 'ffx_fsr2_api_x64.dll') 5000 81
    Bytes (Join-Path $dir 'ffx_fsr2_api_dx12_x64.dll') 5000 82
    Bytes (Join-Path $dir 'x64\dados.rpf') 4096 83
    return $dir
}
$rd = Novo-RDR2 (Join-Path $Raiz 'RDR2 Falso')
$fotoRd = Foto $rd
$r = Rodar @('--detectar', $rd, '--payload', $payRdr)
Ok 'rdr2: FSR 2 na pasta + loader/upscaler nos extras = FSR dividido e DX12 (instalavel)' ($r.Texto -match 'dx12: True\s+fsr: split' -and $r.Texto -notmatch 'FSR 2/3\.0') $r.Texto
$r0 = Rodar @('--detectar', $rd, '--payload', $pay)
Ok 'rdr2: sem os extras no pacote continua FSR "outro" e sem DX12' ($r0.Texto -match 'dx12: False\s+fsr: outro') $r0.Texto
$r = Rodar @('--instalar', (Join-Path $rd 'RDR2.exe'), '--payload', $payRdr)
$okRd = $r.Codigo -eq 0
foreach ($nArq in 'version.dll', 'nr_native_adapter.dll') { if (-not (Test-Path -LiteralPath (Join-Path $rd $nArq)) -or (Get-FileHash -LiteralPath (Join-Path $rd $nArq)).Hash -ne $manRdr.arquivos.$nArq) { $okRd = $false } }
foreach ($nArq in 'amd_fidelityfx_loader_dx12.dll', 'amd_fidelityfx_upscaler_dx12.dll') { if (-not (Test-Path -LiteralPath (Join-Path $rd $nArq)) -or (Get-FileHash -LiteralPath (Join-Path $rd $nArq)).Hash -ne $manRdr.porjogo.rdr2.$nArq) { $okRd = $false } }
$iniRd = Join-Path $rd 'dlssnr_on_amd.ini'
$okIniRd = (Test-Path -LiteralPath $iniRd) -and (Get-FileHash -LiteralPath $iniRd).Hash -eq $manRdr.inis.'RDR2.ini' -and ([IO.File]::ReadAllText($iniRd) -match '(?m)^NeuralFsr2ParaFfx=1\s*$')
Ok 'rdr2: instalar poe helper, proxy, os 2 extras e o RDR2.ini (NeuralFsr2ParaFfx=1); FSR 2 do jogo intocado' ($okRd -and $okIniRd -and (Get-FileHash -LiteralPath (Join-Path $rd 'ffx_fsr2_api_x64.dll')).Hash -eq $fotoRd['ffx_fsr2_api_x64.dll']) "codigo=$($r.Codigo) ini=$okIniRd"
Simula-Jogo $rd
$r = Rodar @('--desinstalar', $rd, '--payload', $payRdr)
Ok 'rdr2: desinstalar volta a pasta byte a byte' ($r.Codigo -eq 0 -and (Iguais $fotoRd (Foto $rd))) "codigo=$($r.Codigo) $(Dif $fotoRd (Foto $rd))"

# upscaler do jogador MAIS NOVO que o do pacote: fica (nada registrado); o loader que falta entra; desinstalar = byte a byte
$rdN = Novo-RDR2 (Join-Path $Raiz 'RDR2 Upscaler Novo')
Copy-Item -LiteralPath (Join-Path $sys 'msvcp140.dll') -Destination (Join-Path $rdN 'amd_fidelityfx_upscaler_dx12.dll')
$fotoN = Foto $rdN
$r = Rodar @('--instalar', $rdN, '--payload', $payRdr)
$okN = $r.Codigo -eq 0 -and (Get-FileHash -LiteralPath (Join-Path $rdN 'amd_fidelityfx_upscaler_dx12.dll')).Hash -eq $fotoN['amd_fidelityfx_upscaler_dx12.dll'] -and
    (Get-FileHash -LiteralPath (Join-Path $rdN 'amd_fidelityfx_loader_dx12.dll')).Hash -eq $manRdr.porjogo.rdr2.'amd_fidelityfx_loader_dx12.dll' -and $r.Texto -match 'amd_fidelityfx_upscaler_dx12\.dll \([^)]*14\.'
Ok 'rdr2: upscaler mais novo do jogador nao e trocado (sem rebaixar)' $okN "codigo=$($r.Codigo)"
Simula-Jogo $rdN
$r = Rodar @('--desinstalar', $rdN, '--payload', $payRdr)
Ok 'rdr2: desinstalar com o upscaler mantido volta byte a byte' ($r.Codigo -eq 0 -and (Iguais $fotoN (Foto $rdN))) "codigo=$($r.Codigo) $(Dif $fotoN (Foto $rdN))"
# upscaler do jogador sem versao (ou mais velho): trocado com backup e devolvido na desinstalacao
$rdV = Novo-RDR2 (Join-Path $Raiz 'RDR2 Upscaler Velho')
Bytes (Join-Path $rdV 'amd_fidelityfx_upscaler_dx12.dll') 6000 84
$fotoV = Foto $rdV
$r1 = Rodar @('--instalar', $rdV, '--payload', $payRdr)
$trocouV = (Get-FileHash -LiteralPath (Join-Path $rdV 'amd_fidelityfx_upscaler_dx12.dll')).Hash -eq $manRdr.porjogo.rdr2.'amd_fidelityfx_upscaler_dx12.dll'
Simula-Jogo $rdV
$r2 = Rodar @('--desinstalar', $rdV, '--payload', $payRdr)
Ok 'rdr2: upscaler sem versao e trocado e volta na desinstalacao' ($r1.Codigo -eq 0 -and $trocouV -and $r2.Codigo -eq 0 -and (Iguais $fotoV (Foto $rdV))) "codigo=$($r1.Codigo)/$($r2.Codigo) $(Dif $fotoV (Foto $rdV))"

# fsr2-ffx (26/10): a ffx_fsr2_api_x64.dll NOSSA (FSR 2 -> FidelityFX API + geracao de quadros, analysis/fsr2-ffx) e o nr_fsr2_ffx.ini dela
# como extras do RDR2: a DLL do FSR 2 do jogo e trocada com backup (nao e amd_fidelityfx_*: sem a regra da versao mais nova), o INI
# entra, e desinstalar volta a pasta byte a byte (a original do jogo de volta). DLL do Windows no papel da nossa (recurso de versao).
$iniFsr2 = Join-Path $Raiz 'nr_fsr2_ffx-teste.ini'
[IO.File]::WriteAllText($iniFsr2, "[Fsr2Ffx]`r`nGeracaoQuadros=1`r`n")
$extFsr2 = @{ 'rdr2' = @{ 'ffx_fsr2_api_x64.dll' = (Join-Path $sys 'dwmapi.dll'); 'nr_fsr2_ffx.ini' = $iniFsr2;
                          'amd_fidelityfx_loader_dx12.dll' = (Join-Path $sys 'dcomp.dll'); 'amd_fidelityfx_upscaler_dx12.dll' = (Join-Path $sys 'uxtheme.dll') } }
$payF2 = Novo-Payload (Join-Path $Raiz 'payload-rdr2-fsr2') -ComIni -PorJogoArq $extFsr2 -IniJogo @{ 'RDR2.ini' = $iniRdrTexto }
$manF2 = Get-Content -Raw (Join-Path $payF2 'pacote.json') | ConvertFrom-Json
$rdF = Novo-RDR2 (Join-Path $Raiz 'RDR2 FSR2 Nossa')
$fotoF = Foto $rdF
$r = Rodar @('--instalar', (Join-Path $rdF 'RDR2.exe'), '--payload', $payF2)
$okF = $r.Codigo -eq 0
foreach ($nArq in 'ffx_fsr2_api_x64.dll', 'nr_fsr2_ffx.ini', 'amd_fidelityfx_loader_dx12.dll', 'amd_fidelityfx_upscaler_dx12.dll') {
    if (-not (Test-Path -LiteralPath (Join-Path $rdF $nArq)) -or (Get-FileHash -LiteralPath (Join-Path $rdF $nArq)).Hash -ne $manF2.porjogo.rdr2.$nArq) { $okF = $false } }
$okF = $okF -and (Get-FileHash -LiteralPath (Join-Path $rdF 'ffx_fsr2_api_x64.dll')).Hash -ne $fotoF['ffx_fsr2_api_x64.dll'] -and
    (Get-FileHash -LiteralPath (Join-Path $rdF 'ffx_fsr2_api_dx12_x64.dll')).Hash -eq $fotoF['ffx_fsr2_api_dx12_x64.dll']
Ok 'rdr2 fsr2-ffx: a ffx_fsr2_api_x64.dll nossa troca a do jogo (backup), o nr_fsr2_ffx.ini entra, a dx12 do jogo fica' $okF "codigo=$($r.Codigo)"
Simula-Jogo $rdF
$r = Rodar @('--desinstalar', $rdF, '--payload', $payF2)
Ok 'rdr2 fsr2-ffx: desinstalar devolve a ffx_fsr2_api_x64.dll original byte a byte (pasta inteira igual)' ($r.Codigo -eq 0 -and (Iguais $fotoF (Foto $rdF))) "codigo=$($r.Codigo) $(Dif $fotoF (Foto $rdF))"

# T12b caminho invalido no modo silencioso: mensagem de erro e codigo 1 (antes: excecao fora do try, exe derrubado)
$r = Rodar @('--instalar', $re, '--payload', 'C:\inv<a>lido|pacote')
Ok 'silencioso com --payload invalido: ERRO e codigo 1 (sem travar)' ($r.Codigo -eq 1 -and $r.Texto -match 'ERRO') "codigo=$($r.Codigo)"

# T12c argumentos da reabertura como administrador: "E:\" (raiz de disco) e caminhos com espaco e aspas chegam inteiros
Add-Type -Namespace AmdTeste -Name Argv -MemberDefinition @'
[DllImport("shell32.dll", SetLastError = true, CharSet = CharSet.Unicode)] static extern System.IntPtr CommandLineToArgvW(string linha, out int n);
[DllImport("kernel32.dll")] static extern System.IntPtr LocalFree(System.IntPtr p);
public static string[] Partir(string linha) {
    int n; System.IntPtr p = CommandLineToArgvW(linha, out n);
    string[] r = new string[n];
    for (int i = 0; i < n; i++) r[i] = Marshal.PtrToStringUni(Marshal.ReadIntPtr(p, i * System.IntPtr.Size));
    LocalFree(p); return r;
}
'@
$asm = [Reflection.Assembly]::LoadFile($Exe)
$util = $asm.GetType('Amdlss5.Instalador.Util')
$okArg = $true
foreach ($cam in 'E:\', 'C:\Program Files (x86)\Steam\steamapps\common\Jogo\', 'D:\a b\c', 'D:\x"y\', '') {
    $q = $util.GetMethod('Argumento').Invoke($null, @($cam))
    $partes = [AmdTeste.Argv]::Partir('exe.exe --pasta ' + $q + ' --payload ' + $util.GetMethod('Argumento').Invoke($null, @('P:\payload\')))
    if ($partes.Count -ne 5 -or $partes[2] -ne $cam -or $partes[4] -ne 'P:\payload\') { $okArg = $false }
}
Ok 'argumentos da reabertura (barra final, espacos, aspas) chegam inteiros' $okArg

# 21/10 (revisao final): "Atualizar" so quando o pacote e MAIS NOVO (nunca volta para uma versao velha); "?" = atualizar
$cartaoT = $asm.GetType('Amdlss5.Instalador.Cartao')
$tT = $asm.GetType('Amdlss5.Instalador.T')
$casosV = @(@('1.5.2', '1.6.0', $true), @('1.6.0', '1.5.2', $false), @('1.6.0', '1.6.0', $false), @('1.6.0-teste', '1.6.0', $true),
    @('1.6.0', '1.6.0-teste', $false), @('1.6.0-teste', '1.5.2', $false), @('?', '1.6.0', $true), @('1.10.0', '1.9.0', $false),
    @('1.9.0', '1.10.0', $true), @('1.6', '1.6.0', $false), @('0.9', 'teste', $true))
$falhasV = @()
foreach ($c in $casosV) { $res = $cartaoT.GetMethod('PacoteMaisNovo').Invoke($null, @($c[0], $c[1])); if ($res -ne $c[2]) { $falhasV += "$($c[0])->$($c[1])=$res" } }
Ok 'versao: Atualizar so com pacote mais novo (instalado mais novo, igual, sufixo -teste, ?, 1.10 > 1.9)' ($falhasV.Count -eq 0) ($falhasV -join ' ')
$jogoV = [Activator]::CreateInstance($asm.GetType('Amdlss5.Instalador.Jogo'), $true); $jogoV.VersaoInstalada = '?'
$pacoteV = [Activator]::CreateInstance($asm.GetType('Amdlss5.Instalador.Payload'), $true); $pacoteV.Versao = '1.6.0'
$esperadoV = @{ pt = 'vers.o desconhecida'; en = 'unknown version'; es = 'versi.n desconocida' }
$okV = "$($cartaoT.GetMethod('Estado').Invoke($null, @($jogoV, $pacoteV)))" -eq 'Atualizar'
$textosV = @()
foreach ($i in 'pt', 'en', 'es') {
    [void]$tT.GetMethod('Definir').Invoke($null, @($i))
    $tx = $cartaoT.GetMethod('TextoAtualizar').Invoke($null, @($jogoV, $pacoteV)); $textosV += $tx
    if ($tx -notmatch $esperadoV[$i] -or $tx.Contains('v?') -or -not $tx.Contains('v1.6.0')) { $okV = $false }
}
$jogoV.VersaoInstalada = '1.7.0'
$okV = $okV -and "$($cartaoT.GetMethod('Estado').Invoke($null, @($jogoV, $pacoteV)))" -eq 'Instalado'
Ok 'versao: "?" = Atualizar com "versao desconhecida" (nunca "v?") nos 3 idiomas; 1.7.0 instalada sobre pacote 1.6.0 = Instalado' $okV ($textosV -join ' / ')

# 21/10 (revisao final): as linhas de maquina das lojas (LOJA_ERRO/JOGO_ERRO) aparecem traduzidas no log da biblioteca
$lpj = $asm.GetType('Amdlss5.Instalador.Biblioteca').GetMethod('LinhaParaJanela', [Reflection.BindingFlags]'NonPublic,Static')
[void]$tT.GetMethod('Definir').Invoke($null, @('en'))
$lj1 = $lpj.Invoke($null, @('LOJA_ERRO: steam: appmanifest_99.acf: manifesto invalido'))
$lj2 = $lpj.Invoke($null, @('JOGO_ERRO: D:\Jogos\Foo Bar: Access to the path is denied.'))
$lj3 = $lpj.Invoke($null, @('LOJA_ERRO: battlenet: x'))
$lj4 = $lpj.Invoke($null, @('linha comum'))
[void]$tT.GetMethod('Definir').Invoke($null, @('pt')); $lj5 = $lpj.Invoke($null, @('LOJA_ERRO: epic: x'))
[void]$tT.GetMethod('Definir').Invoke($null, @('es')); $lj6 = $lpj.Invoke($null, @('JOGO_ERRO: E:\Foo: y'))
[void]$tT.GetMethod('Definir').Invoke($null, @('en'))
Ok 'biblioteca: LOJA_ERRO/JOGO_ERRO viram texto traduzido no log da janela' ($lj1 -eq 'Could not read the Steam library.' -and $lj2 -eq 'Could not check Foo Bar.' -and $lj3 -eq 'Could not read the Battle.net library.' -and $lj4 -eq 'linha comum' -and $lj5 -match '^N.o foi poss.vel ler a biblioteca da Epic Games\.$' -and $lj6 -eq 'No se pudo comprobar Foo.') "$lj1 | $lj2 | $lj3 | $lj5 | $lj6"

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

# T14b lojas: --listar em Steam e Epic falsas (so leitura)
function Nova-Steam([string]$raiz, [object[]]$jogos, [string]$biblioteca2 = '', [object[]]$jogos2 = @(), [switch]$TruncarLib2) {
    $bib1 = Join-Path $raiz 'steamapps'
    New-Item -ItemType Directory -Force -Path (Join-Path $bib1 'common') | Out-Null
    $bibs = @($raiz)
    if ($biblioteca2) { New-Item -ItemType Directory -Force -Path (Join-Path $biblioteca2 'steamapps\common') | Out-Null; $bibs += $biblioteca2 }
    $vdf = "`"libraryfolders`"`r`n{`r`n"
    for ($i = 0; $i -lt $bibs.Count; $i++) {
        $p = $bibs[$i].Replace('\', '\\')
        $vdf += "`t`"$i`"`r`n`t{`r`n`t`t`"path`"`t`t`"$p`"`r`n`t`t`"label`"`t`t`"`"`r`n`t`t`"apps`"`r`n`t`t{`r`n`t`t}`r`n`t}`r`n"
    }
    $vdf += "}`r`n"
    [IO.File]::WriteAllText((Join-Path $bib1 'libraryfolders.vdf'), $vdf)
    $todos = @(); foreach ($j in $jogos) { $todos += , @($raiz, $j) }; foreach ($j in $jogos2) { $todos += , @($biblioteca2, $j) }
    foreach ($t in $todos) {
        $j = $t[1]; $sa = Join-Path $t[0] 'steamapps'
        if ($j.criar -ne $false) { New-Item -ItemType Directory -Force -Path (Join-Path $sa "common\$($j.dir)") | Out-Null; Copy-Item -LiteralPath $exeStub -Destination (Join-Path $sa "common\$($j.dir)\jogo.exe") }
        [IO.File]::WriteAllText((Join-Path $sa "appmanifest_$($j.appid).acf"), "`"AppState`"`r`n{`r`n`t`"appid`"`t`t`"$($j.appid)`"`r`n`t`"name`"`t`t`"$($j.name)`"`r`n`t`"installdir`"`t`t`"$($j.dir)`"`r`n}`r`n")
    }
}
function Nova-Epic([string]$dir, [object[]]$jogos) {
    New-Item -ItemType Directory -Force -Path $dir | Out-Null
    foreach ($j in $jogos) {
        $loc = $j.dir.Replace('\', '\\')
        [IO.File]::WriteAllText((Join-Path $dir "$($j.id).item"), "{`"FormatVersion`":0,`"DisplayName`":`"$($j.name)`",`"InstallLocation`":`"$loc`",`"CatalogItemId`":`"$($j.id)`",`"LaunchExecutable`":`"jogo.exe`"}")
    }
}
$lj = Join-Path $Raiz 'lojas'
$stA = Join-Path $lj 'SteamA'; $stB = Join-Path $lj 'SteamB'; $epM = Join-Path $lj 'EpicManifests'
Nova-Steam $stA @(@{ appid = 3764200; name = 'RE Falso'; dir = 'RE Falso' }, @{ appid = 228980; name = 'Steamworks Common Redistributables'; dir = 'Steamworks Shared' }, @{ appid = 777; name = 'Fantasma'; dir = 'Fantasma'; criar = $false }) $stB @(@{ appid = 1245620; name = 'Outro Jogo'; dir = 'Outro Jogo' })
$epDir = Join-Path $lj 'EpicJogo'
New-Item -ItemType Directory -Force -Path $epDir | Out-Null; Copy-Item -LiteralPath $exeStub -Destination (Join-Path $epDir 'jogo.exe')
Nova-Epic $epM @(@{ id = 'abc123'; name = 'Jogo da Epic'; dir = $epDir }, @{ id = 'dup999'; name = 'RE na Epic'; dir = (Join-Path $stA 'steamapps\common\RE Falso').ToUpper() })
function Cinco([string]$l) { return (($l -split '\|')[0..4]) -join '|' }   # --listar: JOGO|loja|id|nome|pasta|grupo|exe|versao
$r = Rodar @('--listar', '--steam', $stA, '--epic', $epM, '--sem-registro')
$linhas = @($r.Texto -split "`r?`n" | Where-Object { $_ -like 'JOGO|*' } | ForEach-Object { Cinco $_ })
Ok 'listar: steam e epic' ($r.Codigo -eq 0 -and $linhas.Count -eq 3 -and ($linhas -contains "JOGO|steam|3764200|RE Falso|$stA\steamapps\common\RE Falso") -and ($linhas -contains "JOGO|steam|1245620|Outro Jogo|$stB\steamapps\common\Outro Jogo") -and ($linhas -contains "JOGO|epic|abc123|Jogo da Epic|$epDir")) "codigo=$($r.Codigo) linhas=$($linhas.Count)"
Ok 'listar: duplicata some (fica a Steam)' (@($linhas | Where-Object { $_ -like '*RE na Epic*' }).Count -eq 0 -and @($linhas | Where-Object { $_ -like '*|RE Falso|*' }).Count -eq 1)
Ok 'listar: ferramenta da steam fora' (@($linhas | Where-Object { $_ -like '*228980*' }).Count -eq 0)
Ok 'listar: pasta apagada fora' (@($linhas | Where-Object { $_ -like '*Fantasma*' }).Count -eq 0)

# steam quebrado: .acf com lixo + libraryfolders.vdf truncado na 2a biblioteca
$stC = Join-Path $lj 'SteamC'; $stD = Join-Path $lj 'SteamD'
Nova-Steam $stC @(@{ appid = 11; name = 'Bom C'; dir = 'Bom C' }) $stD @(@{ appid = 22; name = 'Bom D'; dir = 'Bom D' })
[IO.File]::WriteAllText((Join-Path $stC 'steamapps\appmanifest_99.acf'), "`"AppState`"`r`n{ lixo `"appid`" `"`r`n \x00\x01 {{{")
$vdfC = Join-Path $stC 'steamapps\libraryfolders.vdf'
$txt = [IO.File]::ReadAllText($vdfC); [IO.File]::WriteAllText($vdfC, $txt.Substring(0, $txt.Length - 40))
$r = Rodar @('--listar', '--steam', $stC, '--sem-registro')
$linhas = @($r.Texto -split "`r?`n" | Where-Object { $_ -like 'JOGO|*' })
Ok 'listar: steam quebrado nao derruba' ($r.Codigo -eq 0 -and ($linhas -match 'Bom C').Count -eq 1) "codigo=$($r.Codigo) linhas=$($linhas.Count)"
Ok 'listar: a linha de maquina LOJA_ERRO continua crua no --listar' ($r.Texto -match '(?m)^LOJA_ERRO: steam: appmanifest_99\.acf: ') $r.Texto

# rev-diag (22/10, review M-14): installdir com .. / \ / : no appmanifest = recusado (antes o cartao apontava para qualquer pasta do disco)
$stE = Join-Path $lj 'SteamE'
Nova-Steam $stE @(@{ appid = 31; name = 'Bom E'; dir = 'Bom E' }, @{ appid = 32; name = 'Fora Da Steam'; dir = '../../Fora'; criar = $false })
New-Item -ItemType Directory -Force -Path (Join-Path $stE 'Fora') | Out-Null; Copy-Item -LiteralPath $exeStub -Destination (Join-Path $stE 'Fora\jogo.exe')
$r = Rodar @('--listar', '--steam', $stE, '--sem-registro')
$linhas = @($r.Texto -split "`r?`n" | Where-Object { $_ -like 'JOGO|*' })
Ok 'listar: installdir com .. fora da steamapps\common e recusado' ($r.Codigo -eq 0 -and ($linhas -match 'Bom E').Count -eq 1 -and ($linhas -match 'Fora Da Steam').Count -eq 0 -and $r.Texto -match '(?m)^LOJA_ERRO: steam: appmanifest_32\.acf: installdir invalido') $r.Texto

# 300 jogos falsos
$st3 = Join-Path $lj 'Steam300'
$jg = 1..300 | ForEach-Object { @{ appid = 100000 + $_; name = "Jogo $_"; dir = "Jogo $_" } }
Nova-Steam $st3 $jg
$cron = [Diagnostics.Stopwatch]::StartNew()
$r = Rodar @('--listar', '--steam', $st3, '--sem-registro')
$cron.Stop()
$n3 = @($r.Texto -split "`r?`n" | Where-Object { $_ -like 'JOGO|*' }).Count
Ok 'listar: 300 jogos falsos em menos de 10 s' ($r.Codigo -eq 0 -and $n3 -eq 300 -and $cron.Elapsed.TotalSeconds -lt 10) ("{0:N1} s, {1} jogos" -f $cron.Elapsed.TotalSeconds, $n3)

# xbox: disco com .GamingRoot mas sem XboxGames nao vira lista de "jogos" (Windows, Users...)
$dsk = Join-Path $lj 'DiscoXbox'
New-Item -ItemType Directory -Force -Path (Join-Path $dsk 'Windows'), (Join-Path $dsk 'Users') | Out-Null
[IO.File]::WriteAllText((Join-Path $dsk '.GamingRoot'), 'x')
$r = Rodar @('--listar', '--xbox-disco', $dsk, '--sem-registro', '--steam', (Join-Path $lj 'nada'), '--epic', (Join-Path $lj 'nada'))
Ok 'listar: disco com .GamingRoot sem XboxGames = 0 jogos' ($r.Codigo -eq 0 -and @($r.Texto -split "`r?`n" | Where-Object { $_ -like 'JOGO|*' }).Count -eq 0) $r.Texto
New-Item -ItemType Directory -Force -Path (Join-Path $dsk 'XboxGames\Jogo X\Content'), (Join-Path $dsk 'XboxGames\GameSave') | Out-Null
Copy-Item -LiteralPath $exeStub -Destination (Join-Path $dsk 'XboxGames\Jogo X\Content\jogo.exe')
$r = Rodar @('--listar', '--xbox-disco', $dsk, '--sem-registro', '--steam', (Join-Path $lj 'nada'), '--epic', (Join-Path $lj 'nada'))
Ok 'listar: disco com XboxGames lista so os jogos dele' (@($r.Texto -split "`r?`n" | Where-Object { $_ -like 'JOGO|xbox|Jogo X|Jogo X|*\Jogo X\Content|*|jogo.exe|-' }).Count -eq 1 -and $r.Texto -notmatch 'Windows')
Ok 'listar: pasta da loja sem exe de jogo (GameSave) nao aparece' ($r.Texto -notmatch 'GameSave' -and @($r.Texto -split "`r?`n" | Where-Object { $_ -like 'JOGO|*' }).Count -eq 1) $r.Texto

# T14c classificacao: Steam falsa com 7 pastas; a lista de testados do pacote casa por (loja, id)
$stT = Join-Path $lj 'SteamT'
$ids = @{ re = 3764200; qualquer = 555001; ac = 555002; semfsr = 555003; x86 = 555004; semexe = 555005; demo = 3764201 }
Nova-Steam $stT @(
    @{ appid = $ids.re; name = 'RE T'; dir = 'RE T'; criar = $false }, @{ appid = $ids.qualquer; name = 'Qualquer T'; dir = 'Qualquer T'; criar = $false },
    @{ appid = $ids.ac; name = 'Anticheat T'; dir = 'Anticheat T'; criar = $false }, @{ appid = $ids.semfsr; name = 'Sem FSR T'; dir = 'Sem FSR T'; criar = $false },
    @{ appid = $ids.x86; name = 'X86 T'; dir = 'X86 T'; criar = $false }, @{ appid = $ids.semexe; name = 'Sem Exe T'; dir = 'Sem Exe T'; criar = $false },
    @{ appid = $ids.demo; name = 'Demo T'; dir = 'Demo T'; criar = $false })
$cT = Join-Path $stT 'steamapps\common'
Novo-RE (Join-Path $cT 'RE T') | Out-Null
Novo-RE (Join-Path $cT 'Qualquer T') 'qualquer.exe' | Out-Null
Novo-RE (Join-Path $cT 'Anticheat T') 'ac.exe' | Out-Null
New-Item -ItemType Directory -Force -Path (Join-Path $cT 'Anticheat T\bin\EasyAntiCheat') | Out-Null
New-Item -ItemType Directory -Force -Path (Join-Path $cT 'Sem FSR T') | Out-Null; Copy-Item -LiteralPath $exeStub -Destination (Join-Path $cT 'Sem FSR T\jogo.exe')
New-Item -ItemType Directory -Force -Path (Join-Path $cT 'X86 T') | Out-Null
Copy-Item -LiteralPath (Join-Path $env:WINDIR 'SysWOW64\choice.exe') -Destination (Join-Path $cT 'X86 T\jogo.exe')
Copy-Item -LiteralPath (Join-Path $sys 'dcomp.dll') -Destination (Join-Path $cT 'X86 T\amd_fidelityfx_dx12.dll')
New-Item -ItemType Directory -Force -Path (Join-Path $cT 'Sem Exe T') | Out-Null
Novo-RE (Join-Path $cT 'Demo T') | Out-Null   # mesmo re9.exe do jogo completo, outro appid
$testadosJson = '{"jogos":[{"loja":"steam","id":"3764200","exe":"re9.exe"},{"loja":"epic","exe":"epicsemid.exe"}]}'
$payT = Novo-Payload (Join-Path $Raiz 'payload-testados') -Testados $testadosJson
$r = Rodar @('--listar', '--steam', $stT, '--sem-registro', '--payload', $payT)
$lt = @{}
foreach ($l in @($r.Texto -split "`r?`n" | Where-Object { $_ -like 'JOGO|*' })) { $c = $l -split '\|'; $lt[$c[3]] = $c }
Ok 'classificar: 8 campos por linha, 6 jogos (a pasta sem exe fica fora)' ($r.Codigo -eq 0 -and $lt.Count -eq 6 -and -not $lt.ContainsKey('Sem Exe T') -and @($lt.Values | Where-Object { $_.Count -ne 8 }).Count -eq 0) $r.Texto
Ok 'classificar: RE Falso (re9.exe, id da lista) = tested' ($lt['RE T'][5] -eq 'tested' -and $lt['RE T'][6] -eq 're9.exe' -and $lt['RE T'][7] -eq '-')
Ok 'classificar: com FSR e exe qualquer = not_tested' ($lt['Qualquer T'][5] -eq 'not_tested' -and $lt['Qualquer T'][6] -eq 'qualquer.exe')
Ok 'classificar: pasta EasyAntiCheat = not_supported' ($lt['Anticheat T'][5] -eq 'not_supported')
Ok 'classificar: sem DLL de FSR = not_supported' ($lt['Sem FSR T'][5] -eq 'not_supported')
Ok 'classificar: exe de 32 bits = not_supported' ($lt['X86 T'][5] -eq 'not_supported')
Ok 'classificar: id da demo nao marca o jogo completo' ($lt['Demo T'][5] -eq 'not_tested' -and $lt['Demo T'][6] -eq 're9.exe')
# sem pacote (payload inexistente) nada e testado; o resto da classificacao continua
$r = Rodar @('--listar', '--steam', $stT, '--sem-registro', '--payload', (Join-Path $Raiz 'nao-existe'))
Ok 'classificar: sem pacote, RE Falso = not_tested' ($r.Codigo -eq 0 -and $r.Texto -match 'JOGO\|steam\|3764200\|RE T\|[^|]*\|not_tested\|re9.exe\|-') $r.Texto
# lista adulterada = pacote recusado (hash) e o --listar segue sem lista
Add-Content -LiteralPath (Join-Path $payT 'testados.json') ' '
$r = Rodar @('--listar', '--steam', $stT, '--sem-registro', '--payload', $payT)
Ok 'classificar: testados.json adulterado = ninguem testado' ($r.Codigo -eq 0 -and $r.Texto -match '\|RE T\|[^|]*\|not_tested\|')

# 21/10 (revisao final): anticheat ao lado de um exe 3 pastas abaixo da pasta da loja e as marcas novas (BEClient*.dll,
# EasyAntiCheat*.dll/.sys); a mesma regra (Classificacao.TemAnticheat) bloqueia o Instalar da janela "adicionar manualmente"
$stAC = Join-Path $lj 'SteamAC'
Nova-Steam $stAC @(@{ appid = 555101; name = 'AC Fundo T'; dir = 'AC Fundo T'; criar = $false }, @{ appid = 555102; name = 'EAC Dll T'; dir = 'EAC Dll T'; criar = $false },
    @{ appid = 555103; name = 'Limpo Fundo T'; dir = 'Limpo Fundo T'; criar = $false },
    @{ appid = 555104; name = 'EA AC Pasta T'; dir = 'EA AC Pasta T'; criar = $false }, @{ appid = 555105; name = 'EA AC Exe T'; dir = 'EA AC Exe T'; criar = $false })
$cAC = Join-Path $stAC 'steamapps\common'
foreach ($par in @(@('AC Fundo T', 'acfundo.exe'), @('Limpo Fundo T', 'limpo.exe'))) {
    $w64 = Join-Path $cAC "$($par[0])\Jogo\Binarios\Win64"
    New-Item -ItemType Directory -Force -Path $w64 | Out-Null
    Copy-Item -LiteralPath $exeStub -Destination (Join-Path $w64 $par[1])
    Copy-Item -LiteralPath (Join-Path $sys 'dcomp.dll') -Destination (Join-Path $w64 'amd_fidelityfx_dx12.dll')
}
Bytes (Join-Path $cAC 'AC Fundo T\Jogo\Binarios\Win64\BEClient_x64.dll') 1024 41
Novo-RE (Join-Path $cAC 'EAC Dll T') 'eacdll.exe' | Out-Null
Bytes (Join-Path $cAC 'EAC Dll T\EasyAntiCheat_EOS.sys') 1024 42
# 21/10: Battlefield 6 (EA AntiCheat / Javelin) aparecia "ainda nao testado": a pasta EAAntiCheat e o EAAntiCheat.GameServiceLauncher.exe
Novo-RE (Join-Path $cAC 'EA AC Pasta T') 'bf.exe' | Out-Null
New-Item -ItemType Directory -Force -Path (Join-Path $cAC 'EA AC Pasta T\EAAntiCheat') | Out-Null
Novo-RE (Join-Path $cAC 'EA AC Exe T') 'bf2.exe' | Out-Null
Bytes (Join-Path $cAC 'EA AC Exe T\EAAntiCheat.GameServiceLauncher.exe') 1024 43
$r = Rodar @('--listar', '--steam', $stAC, '--sem-registro', '--payload', (Join-Path $Raiz 'nao-existe'))
$la = @{}
foreach ($l in @($r.Texto -split "`r?`n" | Where-Object { $_ -like 'JOGO|*' })) { $c = $l -split '\|'; $la[$c[3]] = $c }
Ok 'anticheat: BEClient ao lado de um exe 3 pastas abaixo = not_supported (o mesmo jogo sem ele = not_tested)' ($la['AC Fundo T'][5] -eq 'not_supported' -and $la['AC Fundo T'][6] -eq 'acfundo.exe' -and $la['Limpo Fundo T'][5] -eq 'not_tested' -and $la['Limpo Fundo T'][6] -eq 'limpo.exe') $r.Texto
Ok 'anticheat: EasyAntiCheat*.sys na pasta do jogo = not_supported' ($la['EAC Dll T'][5] -eq 'not_supported') $r.Texto
Ok 'anticheat: pasta EAAntiCheat (EA/Battlefield 6) = not_supported' ($la['EA AC Pasta T'][5] -eq 'not_supported') $r.Texto
Ok 'anticheat: EAAntiCheat.GameServiceLauncher.exe = not_supported' ($la['EA AC Exe T'][5] -eq 'not_supported') $r.Texto
$tac = $asm.GetType('Amdlss5.Instalador.Classificacao').GetMethod('TemAnticheat', [Reflection.BindingFlags]'NonPublic,Static')
$w64AC = [string](Join-Path $cAC 'AC Fundo T\Jogo\Binarios\Win64'); $w64L = [string](Join-Path $cAC 'Limpo Fundo T\Jogo\Binarios\Win64')
$raizAC = [string](Join-Path $cAC 'AC Fundo T'); $raizL = [string](Join-Path $cAC 'Limpo Fundo T')
Ok 'anticheat: regra da janela manual (pasta escolhida = raiz do jogo ou a do exe)' ($tac.Invoke($null, [object[]]@($w64AC, $raizAC)) -and $tac.Invoke($null, [object[]]@($w64AC, $w64AC)) -and -not $tac.Invoke($null, [object[]]@($w64L, $raizL)) -and -not $tac.Invoke($null, [object[]]@($w64L, $w64L)))

# testados.json do pacote (modelos): entrada sem id casa pelo exe em qualquer loja (Witcher 3 na Epic = tested);
# o re9.exe so tem entrada com id da Steam (o RE na Epic fica not_tested)
$epTE = Join-Path $lj 'EpicTestados'
$w3E = Join-Path $lj 'W3 Epic'; New-Item -ItemType Directory -Force -Path $w3E | Out-Null
Copy-Item -LiteralPath $exeStub -Destination (Join-Path $w3E 'witcher3.exe'); Copy-Item -LiteralPath (Join-Path $sys 'dcomp.dll') -Destination (Join-Path $w3E 'amd_fidelityfx_dx12.dll')
$reE = Novo-RE (Join-Path $lj 'RE Epic')
Nova-Epic $epTE @(@{ id = 'w3epic'; name = 'W3 Epic'; dir = $w3E }, @{ id = 'reepic'; name = 'RE Epic'; dir = $reE })
$payM = Novo-Payload (Join-Path $Raiz 'payload-testados-modelo') -Testados ([IO.File]::ReadAllText((Join-Path $PSScriptRoot 'modelos\testados.json')))
$r = Rodar @('--listar', '--epic', $epTE, '--steam', (Join-Path $lj 'nada'), '--sem-registro', '--payload', $payM)
Ok 'testados: entrada sem id (witcher3.exe) = tested na Epic; re9.exe so por id da Steam = not_tested na Epic' ($r.Texto -match '(?m)^JOGO\|epic\|w3epic\|W3 Epic\|[^|]*\|tested\|witcher3\.exe\|' -and $r.Texto -match '(?m)^JOGO\|epic\|reepic\|RE Epic\|[^|]*\|not_tested\|re9\.exe\|') $r.Texto

# rdr2 (26/10): Red Dead Redemption 2 (Steam 1174180, so FSR 2 na pasta) com o testados.json do pacote e os extras do RDR2 = tested;
# o mesmo jogo num pacote sem os extras = not_supported
$stR = Join-Path $lj 'SteamRDR2'
Nova-Steam $stR @(@{ appid = 1174180; name = 'Red Dead Redemption 2'; dir = 'Red Dead Redemption 2'; criar = $false })
Novo-RDR2 (Join-Path $stR 'steamapps\common\Red Dead Redemption 2') | Out-Null
$payRT = Novo-Payload (Join-Path $Raiz 'payload-rdr2-testados') -Testados ([IO.File]::ReadAllText((Join-Path $PSScriptRoot 'modelos\testados.json'))) -PorJogoArq $extRdr
$r = Rodar @('--listar', '--steam', $stR, '--sem-registro', '--payload', $payRT)
$r2 = Rodar @('--listar', '--steam', $stR, '--sem-registro', '--payload', $payM)
Ok 'rdr2: biblioteca mostra o RDR2 (Steam 1174180) como tested com os extras; sem eles not_supported' ($r.Texto -match '(?m)^JOGO\|steam\|1174180\|Red Dead Redemption 2\|[^|]*\|tested\|RDR2\.exe\|' -and $r2.Texto -match '(?m)^JOGO\|steam\|1174180\|[^|]*\|[^|]*\|not_supported\|RDR2\.exe\|') "$($r.Texto) / $($r2.Texto)"

# T14d capas (offline: Steam falsa com o jpg local; a rede so com -Rede)
Add-Type -AssemblyName System.Drawing
function Novo-Jpg([string]$caminho, [switch]$Pequeno) {
    # capa de verdade (300x450 com ruido: > 4 KB) ou, com -Pequeno, o marcador da Steam (60x90 liso, < 4 KB: o instalador recusa)
    $bmp = if ($Pequeno) { New-Object System.Drawing.Bitmap 60, 90 } else { New-Object System.Drawing.Bitmap 300, 450 }
    $g = [System.Drawing.Graphics]::FromImage($bmp); $g.Clear([System.Drawing.Color]::SteelBlue)
    if (-not $Pequeno) {
        $rnd = New-Object Random 5
        for ($i = 0; $i -lt 400; $i++) {
            $br = New-Object System.Drawing.SolidBrush ([System.Drawing.Color]::FromArgb($rnd.Next(256), $rnd.Next(256), $rnd.Next(256)))
            $g.FillRectangle($br, $rnd.Next(300), $rnd.Next(450), $rnd.Next(4, 40), $rnd.Next(4, 40)); $br.Dispose()
        }
    }
    $g.Dispose()
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $caminho) | Out-Null
    $bmp.Save($caminho, [System.Drawing.Imaging.ImageFormat]::Jpeg); $bmp.Dispose()
}
function Valida-Jpg([string]$caminho) {
    try { $im = [System.Drawing.Image]::FromFile($caminho); $ok = $im.Width -gt 0; $im.Dispose(); return $ok } catch { return $false }
}
function Capa-Caminho([string]$texto) { $m = [regex]::Match($texto, 'CAPA: ([^\r\n]*)'); if ($m.Success) { return $m.Groups[1].Value.Trim() } return '' }
$cp = Join-Path $Raiz 'capas'
$stCap = Join-Path $cp 'Steam'
Novo-Jpg (Join-Path $stCap 'appcache\librarycache\3764200_library_600x900.jpg')
$cache1 = Join-Path $cp 'cache1'
$r = Rodar @('--capa', 'steam|3764200|RE Falso', '--capas', $cache1, '--steam', $stCap, '--sem-rede')
$c = Capa-Caminho $r.Texto
Ok 'capa: cache local do steam' ($r.Codigo -eq 0 -and $c -eq (Join-Path $cache1 'steam_3764200.jpg') -and (Test-Path -LiteralPath $c) -and (Valida-Jpg $c)) $r.Texto
Novo-Jpg (Join-Path $stCap 'appcache\librarycache\555\library_600x900.jpg')
$r = Rodar @('--capa', 'steam|555|Pasta Por Appid', '--capas', $cache1, '--steam', $stCap, '--sem-rede')
Ok 'capa: forma <appid>\library_600x900.jpg do steam' ((Capa-Caminho $r.Texto) -eq (Join-Path $cache1 'steam_555.jpg')) $r.Texto
$r = Rodar @('--capa', 'steam|3764200|RE Falso', '--capas', $cache1, '--steam', (Join-Path $cp 'SteamVazia'), '--sem-rede')
Ok 'capa: ja no cache nao baixa' ((Capa-Caminho $r.Texto) -eq (Join-Path $cache1 'steam_3764200.jpg')) $r.Texto
# 21/10 (PC do usuario: RE Requiem, BF6 e MH Wilds sem capa): o Steam novo guarda a vertical em <appid>\<hash>\library_capsule.jpg,
# a web devolve um marcador de 1,6 KB para jogo novo e uma sobra pequena na cache prendia a capa errada
Novo-Jpg (Join-Path $stCap 'appcache\librarycache\666\5efba891\library_capsule.jpg')
$r = Rodar @('--capa', 'steam|666|Steam Novo', '--capas', $cache1, '--steam', $stCap, '--sem-rede')
$c = Capa-Caminho $r.Texto
Ok 'capa: Steam novo <appid>\<hash>\library_capsule.jpg' ($c -eq (Join-Path $cache1 'steam_666.jpg') -and (Valida-Jpg $c)) $r.Texto
Novo-Jpg (Join-Path $stCap 'appcache\librarycache\777\aa11\library_600x900.jpg') -Pequeno
Novo-Jpg (Join-Path $stCap 'appcache\librarycache\777\bb22\library_capsule.jpg')
Novo-Jpg (Join-Path $cache1 'steam_777.jpg') -Pequeno
$r = Rodar @('--capa', 'steam|777|Marcador', '--capas', $cache1, '--steam', $stCap, '--sem-rede')
$c = Capa-Caminho $r.Texto
Ok 'capa: marcador pequeno (na cache e na Steam) e recusado e trocado pela capa boa' ($c -eq (Join-Path $cache1 'steam_777.jpg') -and (Get-Item -LiteralPath $c).Length -ge 4096) $r.Texto
Novo-Jpg (Join-Path $stCap 'appcache\librarycache\888\cc33\library_600x900.jpg') -Pequeno
$r = Rodar @('--capa', 'steam|888|So Marcador', '--capas', (Join-Path $cp 'cache-marcador'), '--steam', $stCap, '--sem-rede')
Ok 'capa: so marcador e sem rede = capa simples' ((Capa-Caminho $r.Texto) -eq '-') $r.Texto
$cache2 = Join-Path $cp 'cache2'
$r = Rodar @('--capa', 'steam|123|Sem Capa', '--capas', $cache2, '--steam', (Join-Path $cp 'SteamVazia'), '--sem-rede')
Ok 'capa: sem rede e sem cache' ($r.Codigo -eq 0 -and (Capa-Caminho $r.Texto) -eq '-') $r.Texto
# nome de arquivo: id limpo vira <loja>_<id>.jpg; id com caracteres invalidos vira id_limpo-<md5 6 bytes>; ambos so [A-Za-z0-9_.-]
Novo-Jpg (Join-Path $cache2 'epic_abc.jpg')
$r = Rodar @('--capa', 'epic|abc|Tom Clancy''s: "X" & Y', '--capas', $cache2, '--sem-rede')
$c = Capa-Caminho $r.Texto
Ok 'capa: nome com caracteres especiais vira arquivo valido' ($r.Codigo -eq 0 -and $c -eq (Join-Path $cache2 'epic_abc.jpg') -and (Split-Path -Leaf $c) -match '^[A-Za-z0-9_.-]+$') $r.Texto
$idRuim = 'a/b:c*d'
$md5 = [Security.Cryptography.MD5]::Create().ComputeHash([Text.Encoding]::UTF8.GetBytes($idRuim))
$esp = 'epic_a_b_c_d-' + (($md5[0..5] | ForEach-Object { $_.ToString('x2') }) -join '') + '.jpg'
Novo-Jpg (Join-Path $cache2 $esp)
$r = Rodar @('--capa', ('epic|' + $idRuim + '|Nome'), '--capas', $cache2, '--sem-rede')
$c = Capa-Caminho $r.Texto
Ok 'capa: id com caracteres invalidos vira nome valido' ($c -eq (Join-Path $cache2 $esp) -and (Split-Path -Leaf $c) -match '^[A-Za-z0-9_.-]+$') $r.Texto
if ($Rede) {
    $cache3 = Join-Path $cp 'cache3'
    $r = Rodar @('--capa', 'steam|1091500|Cyberpunk 2077', '--capas', $cache3, '--steam', (Join-Path $cp 'SteamVazia'))
    $c = Capa-Caminho $r.Texto
    Ok 'capa: steam pela web' ($c -ne '-' -and $c -match '\.jpg$' -and (Test-Path -LiteralPath $c) -and (Valida-Jpg $c)) $r.Texto
    # rev-diag (22/10, review M-1): a busca pelo nome (manda o nome do jogo para a Steam) so com --capas-por-nome
    $r = Rodar @('--capa', 'epic|zzz|Cyberpunk 2077', '--capas', (Join-Path $cp 'cache4'), '--steam', (Join-Path $cp 'SteamVazia'))
    Ok 'capa: outra loja sem --capas-por-nome = capa simples (nenhuma busca)' ((Capa-Caminho $r.Texto) -eq '-') $r.Texto
    $r = Rodar @('--capa', 'epic|zzz|Cyberpunk 2077', '--capas', $cache3, '--steam', (Join-Path $cp 'SteamVazia'), '--capas-por-nome')
    $c = Capa-Caminho $r.Texto
    Ok 'capa: outra loja pela busca do nome (--capas-por-nome)' ($c -ne '-' -and (Test-Path -LiteralPath $c) -and (Valida-Jpg $c)) $r.Texto
    # 21/10: RE Requiem (3764200) nao tem library_600x900/header no endereco antigo (404): vem o header_image do appdetails
    $r = Rodar @('--capa', 'steam|3764200|Resident Evil Requiem', '--capas', $cache3, '--steam', (Join-Path $cp 'SteamVazia'))
    $c = Capa-Caminho $r.Texto
    Ok 'capa: jogo novo sem a imagem antiga = header da loja (appdetails)' ($c -ne '-' -and (Test-Path -LiteralPath $c) -and (Get-Item -LiteralPath $c).Length -ge 4096 -and (Valida-Jpg $c)) $r.Texto
}

# T14e ferramentas\MontarPacote.cs (o que o Montar-Pacote.ps1 compila) com um pacote falso: o INI publico sai com a lista
# Desligar em 0, sem comentarios e sem NeuralRostoJogadorYunet*; uma chave de log que sobrou ligada derruba a montagem
$mpExe = Join-Path $Raiz 'MontarPacote-teste.exe'
& (Join-Path $env:WINDIR 'Microsoft.NET\Framework64\v4.0.30319\csc.exe') /nologo /target:exe /optimize+ "/out:$mpExe" /reference:System.Web.Extensions.dll (Join-Path $PSScriptRoot 'ferramentas\MontarPacote.cs') | Out-Null
$mpPac = Join-Path $Raiz 'mp-pacote'
New-Item -ItemType Directory -Force -Path $mpPac | Out-Null
Copy-Item -LiteralPath (Join-Path $sys 'version.dll') -Destination (Join-Path $mpPac 'version.dll')
$shaProxy = (Get-FileHash -LiteralPath (Join-Path $mpPac 'version.dll')).Hash
$bHelper = New-Object byte[] 4128   # helper falso: bytes quaisquer + o SHA-256 do proxy (a montagem confere que ele esta dentro)
(New-Object Random 43).NextBytes($bHelper)
for ($i = 0; $i -lt 32; $i++) { $bHelper[4096 + $i] = [Convert]::ToByte($shaProxy.Substring($i * 2, 2), 16) }
[IO.File]::WriteAllBytes((Join-Path $mpPac 'nr_native_adapter.dll'), $bHelper)
[IO.File]::WriteAllText("$mpPac.json", (@{ 'version.dll' = $shaProxy; 'nr_native_adapter.dll' = (Get-FileHash -LiteralPath (Join-Path $mpPac 'nr_native_adapter.dll')).Hash } | ConvertTo-Json))
function Montar-Teste([string]$nome, [string]$iniTexto, [string]$manter = '0', [string[]]$extras = @()) {
    $ini = Join-Path $Raiz "mp-$nome.ini"
    [IO.File]::WriteAllBytes($ini, [Text.Encoding]::GetEncoding(28591).GetBytes($iniTexto))
    $dest = Join-Path $Raiz "mp-saida-$nome"
    $so = Join-Path $Raiz "mp-$nome-out.txt"; $se = Join-Path $Raiz "mp-$nome-err.txt"
    $argsMp = @($mpPac, "$mpPac.json", (Join-Path $mpPac 'nr_native_adapter.dll'), '-', '-', $dest, '9.9.9', $manter, (Join-Path $PSScriptRoot 'modelos'), "re9=$ini") + $extras | ForEach-Object { '"' + $_ + '"' }
    $p = Start-Process -FilePath $mpExe -ArgumentList $argsMp -Wait -PassThru -WindowStyle Hidden -RedirectStandardOutput $so -RedirectStandardError $se
    $saida = Join-Path $dest 'payload\ini\re9.ini'
    return [pscustomobject]@{ Codigo = $p.ExitCode; Texto = [IO.File]::ReadAllText($so) + [IO.File]::ReadAllText($se); Ini = $(if (Test-Path -LiteralPath $saida) { [IO.File]::ReadAllText($saida, [Text.Encoding]::GetEncoding(28591)) } else { '' }) }
}
$iniPesquisa = "[DlssNrOnAmd]`nEnabled=1`nNeuralGravarAviso=1`r`nNeuralRostoJogadorLogJobs=1`r`n; nota interna de pesquisa`r`n;NeuralRostoJogadorRedeFixa=2`r`n" +
    "NeuralRostoJogadorYunetAvx=1`r`nNeuralRostoJogadorYunetFaixa=1280`rNeuralAgeLogFrames=600`r`nNeuralCapturaCarimbo=1`r`n  NeuralEsperaDiag = 2`r`n" +
    "NeuralLogTetoMB=8`r`nNeuralGravarF9Quadros=200`r`nNeuralKernelTraceJobs=3`r`nNeuralSemDiagMov=1`r`nNeuralRostoCor=1`r`nNeuralTelaCapturaEm=0`r`n"
$m1 = Montar-Teste 'ok' $iniPesquisa
$okIni = $m1.Codigo -eq 0 -and $m1.Ini -notmatch '(?m)^\s*;' -and $m1.Ini -notmatch 'Yunet'
foreach ($k in 'NeuralGravarAviso', 'NeuralRostoJogadorLogJobs', 'NeuralAgeLogFrames', 'NeuralCapturaCarimbo', 'NeuralEsperaDiag') { if ($m1.Ini -notmatch "(?m)^\s*$k\s*=0\s*$") { $okIni = $false } }
foreach ($kv in 'NeuralLogTetoMB=8', 'NeuralGravarF9Quadros=200', 'NeuralKernelTraceJobs=3', 'NeuralSemDiagMov=1', 'NeuralRostoCor=1', 'Enabled=1', '[DlssNrOnAmd]') { if (-not $m1.Ini.Contains($kv)) { $okIni = $false } }
Ok 'montar pacote: INI publico com log/gravacao em 0, sem comentarios e sem NeuralRostoJogadorYunet*' $okIni "codigo=$($m1.Codigo) $($m1.Texto -replace '\s+', ' ')"
# ao (26/10): os avisos de licenca de terceiros (MIT) vao na raiz do pacote, iguais ao modelo
$avisos = Join-Path $Raiz 'mp-saida-ok\THIRD_PARTY_NOTICES.txt'
Ok 'montar pacote: THIRD_PARTY_NOTICES.txt na raiz do pacote, igual ao modelo' ((Test-Path -LiteralPath $avisos) -and ((Get-FileHash -LiteralPath $avisos).Hash -eq (Get-FileHash -LiteralPath (Join-Path $PSScriptRoot 'modelos\THIRD_PARTY_NOTICES.txt')).Hash)) $avisos
$m2 = Montar-Teste 'sobra' ($iniPesquisa + "NeuralNovoLogQualquer=1`r`n")
Ok 'montar pacote: chave de log fora da lista ainda ligada = a montagem falha e diz qual' ($m2.Codigo -eq 1 -and $m2.Texto -match 'NeuralNovoLogQualquer=1') ($m2.Texto -replace '\s+', ' ')
$m3 = Montar-Teste 'manter' ($iniPesquisa + "NeuralNovoLogQualquer=1`r`n") '1'
Ok 'montar pacote: -ManterDiagnostico leva o INI como esta (comentarios e chaves ligadas)' ($m3.Codigo -eq 0 -and $m3.Ini -match '(?m)^; nota interna' -and $m3.Ini -match 'NeuralNovoLogQualquer=1' -and $m3.Ini -match 'NeuralGravarAviso=1') "codigo=$($m3.Codigo)"
# review P1-13 (26/10): todo .dll/.asi do payload e dos extras por jogo passa pela varredura sem log (a regra do
# analysis\v040-port\Verificar-SemLog.ps1): um nome de log embutido (ASCII ou UTF-16) derruba a montagem e diz o arquivo e o nome;
# -ManterDiagnostico deixa passar. INI extra de um jogo sai como os INIs publicos (sem ';', log/gravacao e NeuralFiltrosComFg em 0).
Ok 'montar pacote: helper e proxy do pacote varridos sem log (semlog=2)' ($m1.Texto -match 'semlog=2 binario') ($m1.Texto -replace '\s+', ' ')
$mpExtra = Join-Path $Raiz 'mp-extra'
New-Item -ItemType Directory -Force -Path $mpExtra | Out-Null
$dllLog = Join-Path $mpExtra 'nr_plugin_teste.dll'
$bLog = New-Object byte[] 8192
(New-Object Random 44).NextBytes($bLog)
$marcaA = [Text.Encoding]::ASCII.GetBytes("`0nr_teste.log`0"); [Array]::Copy($marcaA, 0, $bLog, 2000, $marcaA.Length)
$marcaU = [Text.Encoding]::Unicode.GetBytes("`0nr_utf16_teste.log`0"); [Array]::Copy($marcaU, 0, $bLog, 5001, $marcaU.Length)   # paridade impar
[IO.File]::WriteAllBytes($dllLog, $bLog)
$m4 = Montar-Teste 'dll-log' $iniPesquisa '0' @("@sottr:nr_plugin_teste.dll=$dllLog")
Ok 'montar pacote: DLL extra com nr_teste.log (ASCII) e nr_utf16_teste.log (UTF-16) = falha e diz o arquivo e os nomes' ($m4.Codigo -eq 1 -and $m4.Texto -match 'porjogo/sottr/nr_plugin_teste\.dll' -and $m4.Texto -match 'nr_teste\.log' -and $m4.Texto -match 'nr_utf16_teste\.log') ($m4.Texto -replace '\s+', ' ')
$m5 = Montar-Teste 'dll-log-manter' $iniPesquisa '1' @("@sottr:nr_plugin_teste.dll=$dllLog")
Ok 'montar pacote: -ManterDiagnostico nao varre os binarios' ($m5.Codigo -eq 0) "codigo=$($m5.Codigo) $($m5.Texto -replace '\s+', ' ')"
$iniExtra = Join-Path $mpExtra 'nr_xess_fsr.ini'
[IO.File]::WriteAllBytes($iniExtra, [Text.Encoding]::GetEncoding(28591).GetBytes("; nota interna em portugues: geracao de quadros`r`n[XessFsr]`r`n  ; outra nota`r`nPublicarEstencil=1`r`nNeuralGravarF9=1`r`nNeuralFiltrosComFg=1`r`n"))
$m6 = Montar-Teste 'ini-extra' $iniPesquisa '0' @("@sottr:nr_xess_fsr.ini=$iniExtra")
$iniExtraSai = Join-Path $Raiz 'mp-saida-ini-extra\payload\porjogo\sottr\nr_xess_fsr.ini'
$tx = if (Test-Path -LiteralPath $iniExtraSai) { [IO.File]::ReadAllText($iniExtraSai, [Text.Encoding]::GetEncoding(28591)) } else { '' }
$shaX = ''
try { $shaX = (Get-Content -Raw (Join-Path $Raiz 'mp-saida-ini-extra\payload\pacote.json') | ConvertFrom-Json).porjogo.sottr.'nr_xess_fsr.ini' } catch { }
$okX = $m6.Codigo -eq 0 -and $tx -notmatch '(?m)^\s*;' -and $tx -match '(?m)^\[XessFsr\]' -and $tx -match '(?m)^PublicarEstencil=1\s*$' -and $tx -match '(?m)^NeuralGravarF9=0\s*$' -and $tx -match '(?m)^NeuralFiltrosComFg=0\s*$' -and (Test-Path -LiteralPath $iniExtraSai) -and $shaX -eq (Get-FileHash -LiteralPath $iniExtraSai).Hash
Ok 'montar pacote: INI extra de um jogo sem comentarios, log/gravacao e NeuralFiltrosComFg em 0, SHA-256 do pacote.json = o limpo' $okX "codigo=$($m6.Codigo) ini=[$($tx -replace '\s+', ' ')]"

# T15 capturas da janela (pt, en, es) com o RE falso escolhido
foreach ($idioma in 'pt', 'en', 'es') {
    $png = Join-Path $Raiz "janela-$idioma.png"
    $p = Start-Process -FilePath $Exe -ArgumentList '--pasta', ('"' + $re + '"'), '--payload', ('"' + $pay + '"'), '--idioma', $idioma, '--captura', ('"' + $png + '"') -PassThru
    if (-not $p.WaitForExit(30000)) { try { $p.Kill() } catch { } }
    Ok "captura da janela ($idioma)" (Test-Path $png) $png
}

# T16 biblioteca (janela padrao, sem --pasta): Steam falsa da classificacao (6 jogos), capa local de um, um jogo instalado
# com a versao do pacote e outro com uma versao velha (selo "atualizar"); capturas nos 3 idiomas, sem rede e sem registro
$payB = Novo-Payload (Join-Path $Raiz 'payload-biblioteca') -Testados $testadosJson
$payV = Novo-Payload (Join-Path $Raiz 'payload-velho')
$pjV = Join-Path $payV 'pacote.json'; [IO.File]::WriteAllText($pjV, ([IO.File]::ReadAllText($pjV) -replace '"versao":\s*"teste"', '"versao": "0.9"'))
$r1 = Rodar @('--instalar', (Join-Path $cT 'Qualquer T'), '--payload', $payB)
$r2 = Rodar @('--instalar', (Join-Path $cT 'Demo T'), '--payload', $payV)
Ok 'biblioteca: preparo (2 jogos falsos instalados)' ($r1.Codigo -eq 0 -and $r2.Codigo -eq 0) "codigo=$($r1.Codigo)/$($r2.Codigo)"
Novo-Jpg (Join-Path $stT 'appcache\librarycache\3764200_library_600x900.jpg')
function Abrir-Biblioteca([string]$png, [string[]]$extra) {
    $lista = @('--steam', ('"' + $stT + '"'), '--sem-registro', '--sem-rede', '--capas', ('"' + (Join-Path $Raiz 'capas-biblioteca') + '"'), '--captura', ('"' + $png + '"')) + $extra
    $p = Start-Process -FilePath $Exe -ArgumentList $lista -PassThru
    $saiu = $p.WaitForExit(30000)
    if (-not $saiu) { try { $p.Kill() } catch { } }
    $largura = 0
    if (Test-Path -LiteralPath $png) { try { $im = [System.Drawing.Image]::FromFile($png); $largura = $im.Width; $im.Dispose() } catch { } }
    $kb = if (Test-Path -LiteralPath $png) { [int]((Get-Item -LiteralPath $png).Length / 1KB) } else { 0 }
    return [pscustomobject]@{ Saiu = $saiu; Codigo = $(if ($saiu) { $p.ExitCode } else { -1 }); Kb = $kb; Largura = $largura }
}
foreach ($idioma in 'en', 'pt', 'es') {
    $png = Join-Path $Raiz "biblioteca-$idioma.png"
    $b = Abrir-Biblioteca $png @('--payload', ('"' + $payB + '"'), '--idioma', $idioma)
    # largura >= 1000: e a biblioteca (a janela antiga tem 780)
    Ok "biblioteca: captura $idioma" ($b.Saiu -and $b.Codigo -eq 0 -and $b.Kb -gt 50 -and $b.Largura -ge 1000) "saiu=$($b.Saiu) codigo=$($b.Codigo) kb=$($b.Kb) largura=$($b.Largura) $png"
}
$png = Join-Path $Raiz 'biblioteca-sem-argumentos.png'
$b = Abrir-Biblioteca $png @()
Ok 'biblioteca: sem argumentos nao exige pasta' ($b.Saiu -and $b.Codigo -eq 0 -and $b.Kb -gt 50 -and $b.Largura -ge 1000) "saiu=$($b.Saiu) codigo=$($b.Codigo) kb=$($b.Kb) largura=$($b.Largura)"

$falhas = @($resultados | Where-Object { -not $_.Ok })
''
if ($falhas.Count -or $Manter) {
    "RESULTADO: $($resultados.Count - $falhas.Count)/$($resultados.Count) ok  (pasta de teste mantida: $Raiz)"
    if ($falhas.Count) { exit 1 }
} else {
    # tudo certo: a pasta de teste (jogos falsos, pacotes, capturas) sai; -Manter a deixa para inspecao
    Remove-Item -LiteralPath $Raiz -Recurse -Force -ErrorAction SilentlyContinue
    "RESULTADO: $($resultados.Count - $falhas.Count)/$($resultados.Count) ok  (pasta de teste apagada: $Raiz)"
}
