Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing

$root = Split-Path -Parent $PSScriptRoot
$source = Join-Path $root 'Resources\ui-v2\source\approved-standalone\neutral-default-ui-1280x853.png'
$neutralReference = Join-Path $root 'Resources\ui-v2\reference\neutral-default-reference-1024x683.png'
$destination = Join-Path $root 'Resources\ui-v2\runtime-1024\static\static_faceplate_1024x683.png'

# The source is the one offline normalisation of the user-approved master.
# State rectangles are made transparent: their native V2 state image owns every
# pixel at runtime. No visual is painted twice.
$bitmap = [System.Drawing.Bitmap]::new(1024,683,[System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
$g = [System.Drawing.Graphics]::FromImage($bitmap)
$g.CompositingMode = [System.Drawing.Drawing2D.CompositingMode]::SourceCopy
$master = [System.Drawing.Image]::FromFile($source)
$g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
$g.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
$g.DrawImage($master, [System.Drawing.Rectangle]::new(0, 0, 1024, 683))
$master.Dispose()
$bitmap.Save($neutralReference,[System.Drawing.Imaging.ImageFormat]::Png)

# The supplied neutral UI is the sole normal-state source. These are direct
# native-pixel crops; only normal PNGs are replaced. Selected/playing assets
# remain their approved state assets.
function Export-Crop([System.Drawing.Bitmap]$image, [int[]]$bounds, [string]$path) {
    $crop = $image.Clone([System.Drawing.Rectangle]::new($bounds[0], $bounds[1], $bounds[2], $bounds[3]), [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    try { $crop.Save($path, [System.Drawing.Imaging.ImageFormat]::Png) } finally { $crop.Dispose() }
}

$tabs = @(
    @('tab_1_16_normal.png', @(251,74,105,27)), @('tab_17_32_normal.png', @(360,74,105,27)),
    @('tab_33_48_normal.png', @(470,74,106,27)), @('tab_49_64_normal.png', @(580,74,105,27))
)
foreach ($tab in $tabs) { Export-Crop $bitmap $tab[1] (Join-Path $root ('Resources\\ui-v2\\runtime-1024\\tabs\\' + $tab[0])) }

$presetNames = @('off','forward_cut','backspin','chirp','baby','transform','drag','zigzag','tape_brake','custom')
$presetBounds = @(@(750,100,84,64),@(836,100,84,64),@(924,100,84,64),@(750,166,84,64),@(836,166,84,64),@(924,166,84,64),@(750,232,84,64),@(836,232,84,64),@(924,232,84,64),@(750,296,84,64))
for ($i = 0; $i -lt $presetNames.Count; ++$i) { Export-Crop $bitmap $presetBounds[$i] (Join-Path $root ('Resources\\ui-v2\\runtime-1024\\presets\\preset_' + $presetNames[$i] + '_normal.png')) }

$lengthNames = @('1_16','1_8','1_4','1_2','1_bar')
$lengthBounds = @(@(742,425,32,26),@(773,425,32,26),@(803,425,32,26),@(834,425,32,26),@(864,425,32,26))
for ($i = 0; $i -lt $lengthNames.Count; ++$i) { Export-Crop $bitmap $lengthBounds[$i] (Join-Path $root ('Resources\\ui-v2\\runtime-1024\\length\\length_' + $lengthNames[$i] + '_normal.png')) }

$barBounds = @(@(259,137,56,80),@(317,137,56,80),@(378,137,56,80),@(437,137,56,80),@(494,137,56,80),@(553,137,56,80),@(611,137,56,80),@(670,137,56,80),@(259,221,56,80),@(317,221,56,80),@(378,221,56,80),@(437,221,56,80),@(494,221,56,80),@(553,221,56,80),@(611,221,56,80),@(670,221,56,80))
for ($i = 0; $i -lt $barBounds.Count; ++$i) { Export-Crop $bitmap $barBounds[$i] (Join-Path $root ('Resources\\ui-v2\\runtime-1024\\bars\\bar_{0:D2}_normal.png' -f ($i + 1))) }

# 27/43/59 inherited BAR 11's selected rim in the old generated set. They have
# no distinct state: retain their label/mini pixels, replacing only the two-pixel
# perimeter with a neutral neighbour shell from the same row.
function Repair-Normal-Bar-Rim([string]$targetPath, [string]$sourcePath) {
    $target = [System.Drawing.Bitmap]::new($targetPath)
    $source = [System.Drawing.Bitmap]::new($sourcePath)
    $temporaryPath = $targetPath + '.repair.png'
    try {
        for ($x = 0; $x -lt 56; ++$x) { for ($y = 0; $y -lt 2; ++$y) { $target.SetPixel($x, $y, $source.GetPixel($x, $y)); $target.SetPixel($x, 79 - $y, $source.GetPixel($x, 79 - $y)) } }
        for ($y = 0; $y -lt 80; ++$y) { for ($x = 0; $x -lt 2; ++$x) { $target.SetPixel($x, $y, $source.GetPixel($x, $y)); $target.SetPixel(55 - $x, $y, $source.GetPixel(55 - $x, $y)) } }
        $target.Save($temporaryPath, [System.Drawing.Imaging.ImageFormat]::Png)
    } finally { $source.Dispose(); $target.Dispose() }
    Move-Item -LiteralPath $temporaryPath -Destination $targetPath -Force
}
$barRoot = Join-Path $root 'Resources\\ui-v2\\runtime-1024\\bars'
Repair-Normal-Bar-Rim (Join-Path $barRoot 'bar_27_normal.png') (Join-Path $barRoot 'bar_26_normal.png')
Repair-Normal-Bar-Rim (Join-Path $barRoot 'bar_43_normal.png') (Join-Path $barRoot 'bar_42_normal.png')
Repair-Normal-Bar-Rim (Join-Path $barRoot 'bar_59_normal.png') (Join-Path $barRoot 'bar_58_normal.png')

$clear = [System.Drawing.Brushes]::Transparent
$stateRects = @(
    @(251,74,105,27), @(360,74,105,27), @(470,74,106,27), @(580,74,105,27),
    @(259,137,56,80), @(317,137,56,80), @(378,137,56,80), @(437,137,56,80), @(494,137,56,80), @(553,137,56,80), @(611,137,56,80), @(670,137,56,80),
    @(259,221,56,80), @(317,221,56,80), @(378,221,56,80), @(437,221,56,80), @(494,221,56,80), @(553,221,56,80), @(611,221,56,80), @(670,221,56,80),
    @(750,100,84,64), @(836,100,84,64), @(924,100,84,64), @(750,166,84,64), @(836,166,84,64), @(924,166,84,64), @(750,232,84,64), @(836,232,84,64), @(924,232,84,64), @(750,296,84,64),
    @(742,425,32,26), @(773,425,32,26), @(803,425,32,26), @(834,425,32,26), @(864,425,32,26),
    @(744,513,48,48), @(793,513,48,48), @(848,513,48,48),
    @(742,563,48,16), @(793,563,48,16), @(848,563,48,16),
    @(931,14,80,31),
    @(938,420,18,174), @(971,420,18,174), @(923,601,38,21), @(963,601,38,21)
)
foreach($r in $stateRects) { $g.FillRectangle($clear,[System.Drawing.Rectangle]::new($r[0],$r[1],$r[2],$r[3])) }

$g.Dispose()

# Remove only the master’s example XY trace/point inside the pad.  This is a
# deterministic local-background repair; the panel, labels and buttons remain
# the single static visual owner.  No second XY panel is composited.
function Is-DynamicGold([System.Drawing.Color]$c) {
    return $c.R -gt 150 -and $c.G -gt 80 -and $c.B -lt 145 -and ($c.R - $c.G) -gt 25
}
for ($y = 450; $y -lt 570; ++$y) {
    for ($x = 56; $x -lt 213; ++$x) {
        $pixel = $bitmap.GetPixel($x,$y)
        if (Is-DynamicGold $pixel) {
            $samples = @()
            foreach ($d in @(@(-2,0),@(2,0),@(0,-2),@(0,2))) {
                $sample = $bitmap.GetPixel($x + $d[0],$y + $d[1])
                if (-not (Is-DynamicGold $sample)) { $samples += $sample }
            }
            if ($samples.Count -gt 0) {
                $r = [int](($samples | Measure-Object -Property R -Average).Average)
                $green = [int](($samples | Measure-Object -Property G -Average).Average)
                $b = [int](($samples | Measure-Object -Property B -Average).Average)
                $bitmap.SetPixel($x,$y,[System.Drawing.Color]::FromArgb(255,$r,$green,$b))
            }
        }
    }
}

$bitmap.Save($destination,[System.Drawing.Imaging.ImageFormat]::Png)
$bitmap.Dispose()
