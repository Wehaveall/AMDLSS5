# Gera o logo do mod (instalador/logo.png) e o icone do instalador (instalador/AMDLSS5.ico)
# no estilo do menu do jogo: painel escuro, faixa da marca no topo (vermelho AMD a esquerda,
# verde NVIDIA a direita), "AMD" vermelho + "LSS5" verde + "MOD" branco em fonte mono negrito e
# o selo verde "DLSS 5 ON". Cores tiradas das capturas do menu (menu_100.png / selo.png):
#   vermelho 237,28,36   verde 118,185,0   painel 13,13,14   texto 219,224,232   selo 28,132,28
# Reprodutivel: so System.Drawing, sem arquivo externo.
#
#   powershell -NoProfile -File instalador\Gerar-Logo.ps1 [-Saida <pasta>]
param([string]$Saida = $PSScriptRoot)
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing

$vermelho = [Drawing.Color]::FromArgb(255, 237, 28, 36)
$verde = [Drawing.Color]::FromArgb(255, 118, 185, 0)
$painel = [Drawing.Color]::FromArgb(255, 13, 13, 14)
$borda = [Drawing.Color]::FromArgb(255, 48, 48, 52)
$texto = [Drawing.Color]::FromArgb(255, 219, 224, 232)
$cinza = [Drawing.Color]::FromArgb(255, 151, 161, 176)
$seloVerde = [Drawing.Color]::FromArgb(255, 28, 132, 28)

function Nova-Fonte([float]$px) {
    foreach ($nome in 'Consolas', 'Cascadia Mono', 'Courier New') {
        $f = New-Object Drawing.Font($nome, $px, [Drawing.FontStyle]::Bold, [Drawing.GraphicsUnit]::Pixel)
        if ($f.Name -eq $nome) { return $f }
        $f.Dispose()
    }
    return New-Object Drawing.Font([Drawing.FontFamily]::GenericMonospace, $px, [Drawing.FontStyle]::Bold, [Drawing.GraphicsUnit]::Pixel)
}

function Novo-Grafico($bmp) {
    $g = [Drawing.Graphics]::FromImage($bmp)
    $g.SmoothingMode = [Drawing.Drawing2D.SmoothingMode]::AntiAlias
    $g.TextRenderingHint = [Drawing.Text.TextRenderingHint]::AntiAliasGridFit
    $g.InterpolationMode = [Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
    $g.PixelOffsetMode = [Drawing.Drawing2D.PixelOffsetMode]::HighQuality
    return $g
}

function Retangulo-Arredondado([float]$x, [float]$y, [float]$w, [float]$h, [float]$r) {
    $p = New-Object Drawing.Drawing2D.GraphicsPath
    $d = 2 * $r
    $p.AddArc($x, $y, $d, $d, 180, 90)
    $p.AddArc($x + $w - $d, $y, $d, $d, 270, 90)
    $p.AddArc($x + $w - $d, $y + $h - $d, $d, $d, 0, 90)
    $p.AddArc($x, $y + $h - $d, $d, $d, 90, 90)
    $p.CloseFigure()
    return $p
}

# Desenha pedacos de texto lado a lado (cada um com a sua cor), medidos sem a folga do GDI+.
function Texto-Colorido($g, $partes, $fonte, [float]$x, [float]$y) {
    $fmt = [Drawing.StringFormat]::GenericTypographic
    foreach ($p in $partes) {
        $pincel = New-Object Drawing.SolidBrush($p[1])
        $g.DrawString($p[0], $fonte, $pincel, $x, $y, $fmt)
        $x += $g.MeasureString($p[0], $fonte, 10000, $fmt).Width
        $pincel.Dispose()
    }
    return $x
}

function Faixa($g, [float]$x, [float]$y, [float]$w, [float]$h) {
    $meio = [Math]::Round($w * 0.52)
    $pv = New-Object Drawing.SolidBrush($vermelho); $g.FillRectangle($pv, $x, $y, $meio, $h); $pv.Dispose()
    $pg = New-Object Drawing.SolidBrush($verde); $g.FillRectangle($pg, $x + $meio, $y, $w - $meio, $h); $pg.Dispose()
}

# ---------------------------------------------------------------------------------- logo.png
$W = 512; $H = 160
$bmp = New-Object Drawing.Bitmap($W, $H, [Drawing.Imaging.PixelFormat]::Format32bppArgb)
$g = Novo-Grafico $bmp
$g.Clear([Drawing.Color]::Transparent)
$caixa = Retangulo-Arredondado 0.5 0.5 ($W - 1) ($H - 1) 10
$pp = New-Object Drawing.SolidBrush($painel); $g.FillPath($pp, $caixa); $pp.Dispose()
$g.SetClip($caixa)
Faixa $g 0 0 $W 7
$g.ResetClip()
$pb = New-Object Drawing.Pen($borda, 1); $g.DrawPath($pb, $caixa); $pb.Dispose()

$fTitulo = Nova-Fonte 58
$fim = Texto-Colorido $g @(@('AMD', $vermelho), @('LSS5', $verde), @(' MOD', $texto)) $fTitulo 26 26
# selo "DLSS 5 ON" como no jogo (branco sobre verde)
$fSelo = Nova-Fonte 22
$fmt = [Drawing.StringFormat]::GenericTypographic
$tSelo = 'DLSS 5 ON'
$tam = $g.MeasureString($tSelo, $fSelo, 1000, $fmt)
$sx = 28; $sy = 106; $sw = [Math]::Ceiling($tam.Width) + 24; $sh = 34
$ps = New-Object Drawing.SolidBrush($seloVerde); $g.FillRectangle($ps, $sx, $sy, $sw, $sh); $ps.Dispose()
$pw = New-Object Drawing.SolidBrush([Drawing.Color]::White)
$g.DrawString($tSelo, $fSelo, $pw, $sx + 12, $sy + ($sh - $tam.Height) / 2, $fmt)
$pw.Dispose()
$fSub = Nova-Fonte 15
$pc = New-Object Drawing.SolidBrush($cinza)
$g.DrawString('neural rendering  .  Radeon', $fSub, $pc, $sx + $sw + 18, $sy + 9, $fmt)
$pc.Dispose()
$g.Dispose()
$logo = Join-Path $Saida 'logo.png'
$bmp.Save($logo, [Drawing.Imaging.ImageFormat]::Png)
$bmp.Dispose()

# ---------------------------------------------------------------------------------- AMDLSS5.ico
function Icone([int]$n) {
    $b = New-Object Drawing.Bitmap($n, $n, [Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $g = Novo-Grafico $b
    $g.Clear([Drawing.Color]::Transparent)
    $r = [Math]::Max(2, $n / 8)
    $caixa = Retangulo-Arredondado 0 0 $n $n $r
    $pp = New-Object Drawing.SolidBrush($painel); $g.FillPath($pp, $caixa); $pp.Dispose()
    $g.SetClip($caixa)
    Faixa $g 0 0 $n ([Math]::Max(2, [Math]::Round($n * 0.11)))
    $g.ResetClip()
    $fmt = New-Object Drawing.StringFormat
    $fmt.Alignment = [Drawing.StringAlignment]::Center
    $fmt.LineAlignment = [Drawing.StringAlignment]::Center
    if ($n -ge 48) {
        # duas linhas: AMD (vermelho) / LSS5 (verde)
        $f = Nova-Fonte ($n * 0.30)
        $pv = New-Object Drawing.SolidBrush($vermelho); $pg = New-Object Drawing.SolidBrush($verde)
        $g.DrawString('AMD', $f, $pv, (New-Object Drawing.RectangleF(0, ($n * 0.14), $n, ($n * 0.42))), $fmt)
        $g.DrawString('LSS5', $f, $pg, (New-Object Drawing.RectangleF(0, ($n * 0.52), $n, ($n * 0.42))), $fmt)
        $pv.Dispose(); $pg.Dispose(); $f.Dispose()
    } else {
        # tamanho pequeno: so o "5" verde, legivel
        $f = Nova-Fonte ($n * 0.78)
        $pg = New-Object Drawing.SolidBrush($verde)
        $g.DrawString('5', $f, $pg, (New-Object Drawing.RectangleF(0, ($n * 0.10), $n, ($n * 0.95))), $fmt)
        $pg.Dispose(); $f.Dispose()
    }
    $g.Dispose()
    return $b
}

# Entrada DIB (32 bpp BGRA de baixo para cima + mascara AND de 1 bpp), aceita por qualquer
# leitor de icone; 256 vai em PNG (formato do Vista em diante).
function Bytes-Dib($b) {
    $n = $b.Width
    $ms = New-Object IO.MemoryStream
    $w = New-Object IO.BinaryWriter($ms)
    $w.Write([int]40); $w.Write([int]$n); $w.Write([int]($n * 2)); $w.Write([int16]1); $w.Write([int16]32)
    $w.Write([int]0); $w.Write([int]0); $w.Write([int]0); $w.Write([int]0); $w.Write([int]0); $w.Write([int]0)
    for ($y = $n - 1; $y -ge 0; $y--) {
        for ($x = 0; $x -lt $n; $x++) { $c = $b.GetPixel($x, $y); $w.Write([byte]$c.B); $w.Write([byte]$c.G); $w.Write([byte]$c.R); $w.Write([byte]$c.A) }
    }
    $linha = [int]([Math]::Ceiling($n / 32) * 4)
    for ($y = $n - 1; $y -ge 0; $y--) {
        $mask = New-Object byte[] $linha
        for ($x = 0; $x -lt $n; $x++) { if ($b.GetPixel($x, $y).A -lt 128) { $mask[[int][Math]::Floor($x / 8)] = $mask[[int][Math]::Floor($x / 8)] -bor (0x80 -shr ($x % 8)) } }
        $w.Write($mask)
    }
    $w.Flush()
    return $ms.ToArray()
}

$tamanhos = 16, 32, 48, 256
$imagens = @()
foreach ($n in $tamanhos) {
    $b = Icone $n
    if ($n -ge 256) { $ms = New-Object IO.MemoryStream; $b.Save($ms, [Drawing.Imaging.ImageFormat]::Png); $dados = $ms.ToArray() }
    else { $dados = Bytes-Dib $b }
    $b.Dispose()
    $imagens += , @($n, $dados)
}
$ico = Join-Path $Saida 'AMDLSS5.ico'
$fs = [IO.File]::Create($ico)
$w = New-Object IO.BinaryWriter($fs)
$w.Write([int16]0); $w.Write([int16]1); $w.Write([int16]$imagens.Count)
$offset = 6 + 16 * $imagens.Count
foreach ($im in $imagens) {
    $n = $im[0]; $d = $im[1]
    $lado = if ($n -ge 256) { 0 } else { $n }
    $w.Write([byte]$lado); $w.Write([byte]$lado); $w.Write([byte]0); $w.Write([byte]0)
    $w.Write([int16]1); $w.Write([int16]32); $w.Write([int]$d.Length); $w.Write([int]$offset)
    $offset += $d.Length
}
foreach ($im in $imagens) { $w.Write([byte[]]$im[1]) }
$w.Close()

"logo : $logo"
"icone: $ico"
