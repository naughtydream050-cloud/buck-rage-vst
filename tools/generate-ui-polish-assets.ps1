[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing

$root = Split-Path -Parent $PSScriptRoot
$runtime = Join-Path $root 'Resources\ui-v2\runtime-1024'
$out = Join-Path $runtime 'polish'
New-Item -ItemType Directory -Force -Path $out | Out-Null

function Save-Scaled([System.Drawing.Bitmap]$source, [string]$path, [int]$width, [int]$height) {
    $target = [System.Drawing.Bitmap]::new($width, $height, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $g = [System.Drawing.Graphics]::FromImage($target)
    try {
        $g.CompositingQuality = [System.Drawing.Drawing2D.CompositingQuality]::HighQuality
        $g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
        $g.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
        $g.DrawImage($source, 0, 0, $width, $height)
        $target.Save($path, [System.Drawing.Imaging.ImageFormat]::Png)
    } finally { $g.Dispose(); $target.Dispose() }
}

function New-GlowPen([System.Drawing.Color]$color, [float]$width) {
    $pen = [System.Drawing.Pen]::new($color, $width)
    $pen.LineJoin = [System.Drawing.Drawing2D.LineJoin]::Round
    return $pen
}

# Preserve the original glyph geometry and alpha, while making every visible
# glyph pixel warm ivory. No value, layout or display backing is regenerated.
$ivory = [System.Drawing.Color]::FromArgb(255, 226, 216, 194)

# Rebuild the digit source sprites from the approved master reference. The old
# imported atlas is not present in this checkout and left opaque rectangles.
$reference = [System.Drawing.Bitmap]::FromFile((Join-Path $root 'Resources\ui-v2\reference\master-timeline-reference-1024x683.png'))
$digitSources = [ordered]@{
    0 = @(356,232,7,10); 1 = @(296,146,7,11); 2 = @(353,146,7,11); 3 = @(412,146,7,11); 4 = @(470,146,7,11);
    5 = @(529,146,7,11); 6 = @(587,146,8,11); 7 = @(647,146,7,11); 8 = @(706,146,7,11); 9 = @(296,232,8,10)
}
foreach ($digit in $digitSources.Keys) {
    $x, $y, $width, $height = $digitSources[$digit]
    $glyph = [System.Drawing.Bitmap]::new($width, $height, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    try {
        for ($gy = 0; $gy -lt $height; ++$gy) {
            for ($gx = 0; $gx -lt $width; ++$gx) {
                $pixel = $reference.GetPixel($x + $gx, $y + $gy)
                if ([Math]::Max($pixel.R, [Math]::Max($pixel.G, $pixel.B)) -ge 70) {
                    $glyph.SetPixel($gx, $gy, $pixel)
                }
            }
        }
        $glyph.Save((Join-Path $runtime "native-display\toy_display_digit_$digit.png"), [System.Drawing.Imaging.ImageFormat]::Png)
    } finally { $glyph.Dispose() }
}
$reference.Dispose()

$glyphs = @('dot','minus','slash','s','t','percent','init','minus_inf') + (0..9 | ForEach-Object { "digit_$_" })
foreach ($glyph in $glyphs) {
    $sourcePath = Join-Path $runtime "native-display\toy_display_$glyph.png"
    if (-not (Test-Path -LiteralPath $sourcePath)) { throw "MISSING_GLYPH:$glyph" }
    $source = [System.Drawing.Bitmap]::FromFile($sourcePath)
    try {
        $target = [System.Drawing.Bitmap]::new($source.Width, $source.Height, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
        try {
            for ($y = 0; $y -lt $source.Height; ++$y) {
                for ($x = 0; $x -lt $source.Width; ++$x) {
                    $a = $source.GetPixel($x, $y).A
                    if ($a -ne 0) { $target.SetPixel($x, $y, [System.Drawing.Color]::FromArgb($a, $ivory.R, $ivory.G, $ivory.B)) }
                }
            }
            $target.Save((Join-Path $out "ui_polish_display_$glyph.png"), [System.Drawing.Imaging.ImageFormat]::Png)
        } finally { $target.Dispose() }
    } finally { $source.Dispose() }
}

# 28 native-pixel LED segments, pre-rendered with restrained local bloom.
foreach ($channel in @(@('left',16), @('right',15))) {
    $name, $width = $channel
    $height = 174
    $scale = 4
    $hi = [System.Drawing.Bitmap]::new($width * $scale, $height * $scale, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $g = [System.Drawing.Graphics]::FromImage($hi)
    try {
        $g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
        for ($segment = 0; $segment -lt 28; ++$segment) {
            $fromBottom = 27 - $segment
            $color = if ($fromBottom -ge 25) { [System.Drawing.Color]::FromArgb(255, 235, 52, 43) }
                     elseif ($fromBottom -ge 21) { [System.Drawing.Color]::FromArgb(255, 238, 190, 48) }
                     else { [System.Drawing.Color]::FromArgb(255, 81, 191, 75) }
            $top = 3 + $segment * 6
            $g.FillRectangle([System.Drawing.SolidBrush]::new([System.Drawing.Color]::FromArgb(34, $color.R, $color.G, $color.B)),
                             0, ($top - 1) * $scale, $width * $scale, 5 * $scale)
            $g.FillRectangle([System.Drawing.SolidBrush]::new([System.Drawing.Color]::FromArgb(105, $color.R, $color.G, $color.B)),
                             1 * $scale, $top * $scale, ($width - 2) * $scale, 3 * $scale)
            $g.FillRectangle([System.Drawing.SolidBrush]::new($color),
                             2 * $scale, ($top + 1) * $scale, ($width - 4) * $scale, 1 * $scale)
        }
        Save-Scaled $hi (Join-Path $out "ui_polish_output_meter_$name.png") $width $height
    } finally { $g.Dispose(); $hi.Dispose() }
}

# Recessed 11x12 status lamp variants are supersampled before being reduced.
foreach ($state in @('off','on')) {
    $hi = [System.Drawing.Bitmap]::new(44, 48, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $g = [System.Drawing.Graphics]::FromImage($hi)
    try {
        $g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
        $isOn = $state -eq 'on'
        if ($isOn) {
            $g.FillEllipse([System.Drawing.SolidBrush]::new([System.Drawing.Color]::FromArgb(32,255,28,42)), 0, 2, 44, 44)
            $g.FillEllipse([System.Drawing.SolidBrush]::new([System.Drawing.Color]::FromArgb(90,255,30,46)), 5, 7, 34, 34)
        }
        $g.FillEllipse([System.Drawing.SolidBrush]::new([System.Drawing.Color]::FromArgb(255, 23, 18, 17)), 7, 8, 30, 30)
        $g.DrawEllipse((New-GlowPen ([System.Drawing.Color]::FromArgb(220,177,134,67)) 3), 8, 9, 28, 28)
        $core = if ($isOn) { [System.Drawing.Color]::FromArgb(255,229,45,39) } else { [System.Drawing.Color]::FromArgb(255,66,28,27) }
        $g.FillEllipse([System.Drawing.SolidBrush]::new($core), 13, 14, 18, 18)
        if ($isOn) { $g.FillEllipse([System.Drawing.SolidBrush]::new([System.Drawing.Color]::FromArgb(220,255,160,138)), 17, 17, 7, 7) }
        Save-Scaled $hi (Join-Path $out "ui_polish_host_sync_$state.png") 11 12
    } finally { $g.Dispose(); $hi.Dispose() }
}

# Generate complete BAR state sprites. The original cell includes a fixed
# waveform, so replace only the dynamic mini rectangle with neutral texture.
# A playing BAR always owns the full red border, including selected+playing.
$neutralShell = [System.Drawing.Bitmap]::FromFile((Join-Path $runtime 'bar-map\shells\bar_cell_shell_normal_56x80.png'))
for ($bar = 1; $bar -le 64; ++$bar) {
    $id = $bar.ToString('00')
    $source = [System.Drawing.Bitmap]::FromFile((Join-Path $runtime "bars\bar_${id}_normal.png"))
    try {
        foreach ($state in @('normal','selected','playing','selected_playing')) {
            $target = [System.Drawing.Bitmap]::new($source)
            $g = [System.Drawing.Graphics]::FromImage($target)
            try {
                $g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
                $g.DrawImage($neutralShell, [System.Drawing.Rectangle]::new(8, 36, 40, 20), [System.Drawing.Rectangle]::new(8, 36, 40, 20), [System.Drawing.GraphicsUnit]::Pixel)
                $selected = $state -eq 'selected'
                $playing = $state -in @('playing','selected_playing')
                if ($selected) {
                    $g.DrawRectangle((New-GlowPen ([System.Drawing.Color]::FromArgb(52,214,166,77)) 4), 1, 1, 53, 77)
                    $g.DrawRectangle((New-GlowPen ([System.Drawing.Color]::FromArgb(235,214,166,77)) 1), 1, 1, 53, 77)
                }
                if ($playing) {
                    $g.DrawRectangle((New-GlowPen ([System.Drawing.Color]::FromArgb(54,220,50,40)) 4), 1, 1, 53, 77)
                    $g.DrawRectangle((New-GlowPen ([System.Drawing.Color]::FromArgb(235,220,50,40)) 1), 1, 1, 53, 77)
                }
                $target.Save((Join-Path $out "ui_polish_bar_${id}_$state.png"), [System.Drawing.Imaging.ImageFormat]::Png)
            } finally { $g.Dispose(); $target.Dispose() }
        }
    } finally { $source.Dispose() }
}
$neutralShell.Dispose()

$manifest = [ordered]@{
    schema = 1
    intent = 'approved black-gold digital neon polish'
    generated_at_utc = [DateTime]::UtcNow.ToString('o')
    assets = @(Get-ChildItem -File $out | Sort-Object Name | ForEach-Object {
        [ordered]@{ file = $_.Name; bytes = $_.Length; sha256 = (Get-FileHash $_.FullName -Algorithm SHA256).Hash }
    })
}
$manifest | ConvertTo-Json -Depth 4 | Set-Content -Encoding utf8 (Join-Path $out 'ui-polish-manifest.json')
