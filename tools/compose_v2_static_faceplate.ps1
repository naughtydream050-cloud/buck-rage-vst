Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing

$root = Split-Path -Parent $PSScriptRoot
$source = Join-Path $root 'Resources\ui-v2\reference\master-timeline-reference-1024x683.png'
$destination = Join-Path $root 'Resources\ui-v2\runtime-1024\static\static_faceplate_1024x683.png'

# The source is the one offline normalisation of the user-approved master.
# State rectangles are made transparent: their native V2 state image owns every
# pixel at runtime. No visual is painted twice.
$bitmap = [System.Drawing.Bitmap]::new($source)
$g = [System.Drawing.Graphics]::FromImage($bitmap)
$g.CompositingMode = [System.Drawing.Drawing2D.CompositingMode]::SourceCopy

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
    @(937,409,12,204), @(974,409,12,204), @(923,601,38,21), @(963,601,38,21)
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
