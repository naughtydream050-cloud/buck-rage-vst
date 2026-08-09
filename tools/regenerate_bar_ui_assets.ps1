param(
    [string] $ProjectRoot = (Resolve-Path "$PSScriptRoot\..").Path
)

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing

$assetRoot = Join-Path $ProjectRoot 'Resources\ui-master-default-preview-20260808'
$masterPath = Join-Path $assetRoot 'previews\A_default_1280x853.png'
$labelDir = Join-Path $assetRoot 'bar-labels'
$waveDir = Join-Path $assetRoot 'bar-waveforms'
$tabDir = Join-Path $assetRoot 'tabs'

if (!(Test-Path -LiteralPath $masterPath)) { throw "MASTER DEFAULT is missing: $masterPath" }
New-Item -ItemType Directory -Force -Path $waveDir | Out-Null

function New-ArgbBitmap([int] $width, [int] $height) {
    return [System.Drawing.Bitmap]::new($width, $height, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
}

function Save-Png([System.Drawing.Bitmap] $image, [string] $path) {
    $image.Save($path, [System.Drawing.Imaging.ImageFormat]::Png)
}

function Crop-Image([System.Drawing.Bitmap] $source, [System.Drawing.Rectangle] $rect) {
    $image = New-ArgbBitmap $rect.Width $rect.Height
    $g = [System.Drawing.Graphics]::FromImage($image)
    $g.CompositingMode = [System.Drawing.Drawing2D.CompositingMode]::SourceCopy
    $g.DrawImage($source, [System.Drawing.Rectangle]::new(0, 0, $rect.Width, $rect.Height), $rect, [System.Drawing.GraphicsUnit]::Pixel)
    $g.Dispose()
    return $image
}

function Copy-Region([System.Drawing.Graphics] $graphics, [System.Drawing.Bitmap] $source, [System.Drawing.Rectangle] $sourceRect, [int] $x, [int] $y) {
    $graphics.DrawImage($source, [System.Drawing.Rectangle]::new($x, $y, $sourceRect.Width, $sourceRect.Height), $sourceRect, [System.Drawing.GraphicsUnit]::Pixel)
}

$master = [System.Drawing.Bitmap]::new($masterPath)
try {
    if ($master.Width -ne 1280 -or $master.Height -ne 853) { throw 'MASTER DEFAULT must be 1280x853.' }

    # These are the actual 72x22 label pixels from the approved 1-16 master grid.
    $referenceLabels = @()
    for ($index = 0; $index -lt 16; ++$index) {
        $x = 322 + (($index % 8) * 75)
        $y = 175 + ([Math]::Floor($index / 8) * 97)
        $label = Crop-Image $master ([System.Drawing.Rectangle]::new($x, $y, 72, 22))
        $referenceLabels += $label
        Save-Png $label (Join-Path $labelDir ('bar_label_{0:D2}.png' -f ($index + 1)))
    }

    # Build 17-64 entirely from MASTER DEFAULT label pixels: prefix and digit
    # glyphs are copied at their final native positions; no fonts are used.
    $prefixRect = [System.Drawing.Rectangle]::new(0, 0, 41, 22)
    $firstDigitRect = [System.Drawing.Rectangle]::new(42, 0, 12, 22)
    $secondDigitRect = [System.Drawing.Rectangle]::new(54, 0, 18, 22)
    for ($number = 17; $number -le 64; ++$number) {
        $tens = [Math]::Floor($number / 10)
        $ones = $number % 10
        $target = New-ArgbBitmap 72 22
        $g = [System.Drawing.Graphics]::FromImage($target)
        $g.CompositingMode = [System.Drawing.Drawing2D.CompositingMode]::SourceCopy
        Copy-Region $g $referenceLabels[9] $prefixRect 0 0

        # 10..16 provide the approved two-digit baseline for 0..6.
        if ($tens -eq 1) {
            Copy-Region $g $referenceLabels[9] $firstDigitRect 42 0
        } elseif ($tens -le 6) {
            # 12..16 provide the real 2..6 glyph in their second-digit slot.
            Copy-Region $g $referenceLabels[9 + $tens] $secondDigitRect 46 0
        } else {
            # 7..9 only occur as single digits in the master; position their
            # real pixels in the established first-digit slot.
            Copy-Region $g $referenceLabels[$tens - 1] ([System.Drawing.Rectangle]::new(45, 0, 27, 22)) 42 0
        }
        if ($ones -le 6) {
            Copy-Region $g $referenceLabels[9 + $ones] $secondDigitRect 54 0
        } else {
            # 7..9 are taken from their real single-digit MASTER DEFAULT labels
            # and shifted only into the established second-digit slot.
            $singleDigit = $referenceLabels[$ones - 1]
            Copy-Region $g $singleDigit ([System.Drawing.Rectangle]::new(45, 0, 27, 22)) 50 0
        }
        $g.Dispose()
        Save-Png $target (Join-Path $labelDir ('bar_label_{0:D2}.png' -f $number))
        $target.Dispose()
    }

    # Produce one opaque real-pixel motion tile for every BAR. The 16 source
    # motifs come from MASTER DEFAULT. Page-specific source offsets retain the
    # same authored pixels while ensuring 17-64 are not identical copies.
    $pageOffsets = @(0, 1, -1, 2)
    for ($index = 0; $index -lt 64; ++$index) {
        $sourceCell = $index % 16
        $page = [Math]::Floor($index / 16)
        $x = 322 + (($sourceCell % 8) * 75) + 5 + $pageOffsets[$page]
        $y = 175 + ([Math]::Floor($sourceCell / 8) * 97) + 33
        $wave = Crop-Image $master ([System.Drawing.Rectangle]::new($x, $y, 62, 30))
        Save-Png $wave (Join-Path $waveDir ('bar_wave_{0:D2}.png' -f ($index + 1)))
        $wave.Dispose()
    }

    # Make the strip overlays fully opaque by compositing their real pixels
    # over the MASTER DEFAULT strip rectangle. This preserves the approved
    # texture at anti-aliased edges while preventing black bleed-through.
    Get-ChildItem -LiteralPath $tabDir -Filter 'tab_strip_selected_*.png' -File | ForEach-Object {
        $strip = [System.Drawing.Bitmap]::new($_.FullName)
        if ($strip.Width -ne 542 -or $strip.Height -ne 34) { $strip.Dispose(); throw "Unexpected tab strip size: $($_.Name)" }
        $opaque = Crop-Image $master ([System.Drawing.Rectangle]::new(317, 99, 542, 34))
        $g = [System.Drawing.Graphics]::FromImage($opaque)
        $g.CompositingMode = [System.Drawing.Drawing2D.CompositingMode]::SourceOver
        $g.DrawImageUnscaled($strip, 0, 0)
        $g.Dispose()
        $strip.Dispose()
        Save-Png $opaque $_.FullName
        $opaque.Dispose()
    }
}
finally {
    foreach ($label in $referenceLabels) { $label.Dispose() }
    $master.Dispose()
}

Write-Host 'Regenerated BAR labels, BAR waveform tiles, and opaque tab strips from MASTER DEFAULT pixels.'
