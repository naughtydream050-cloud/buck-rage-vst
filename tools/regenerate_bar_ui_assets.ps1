param(
    [string] $ProjectRoot = (Resolve-Path "$PSScriptRoot\..").Path,
    [string] $LabelPreviewPath = ''
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

function Copy-GlyphPixels([System.Drawing.Bitmap] $target, [System.Drawing.Bitmap] $source, [System.Drawing.Rectangle] $sourceRect, [int] $targetX) {
    # Label crops come from an opaque cell image. Extract only the existing
    # bright ivory glyph pixels; cell texture is deliberately not copied.
    # This is a deterministic pixel transfer, never a font substitution.
    for ($y = $sourceRect.Top; $y -lt $sourceRect.Bottom; ++$y) {
        for ($x = $sourceRect.Left; $x -lt $sourceRect.Right; ++$x) {
            $pixel = $source.GetPixel($x, $y)
            $luminance = (0.2126 * $pixel.R) + (0.7152 * $pixel.G) + (0.0722 * $pixel.B)
            if ($luminance -ge 72.0) {
                $destinationX = $targetX + ($x - $sourceRect.Left)
                if ($destinationX -ge 0 -and $destinationX -lt $target.Width) {
                    $target.SetPixel($destinationX, $y, $pixel)
                }
            }
        }
    }
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

    # Build 17-64 entirely from MASTER DEFAULT label pixels. Direct source
    # crops are opaque, so only their real ivory glyph pixels are transferred.
    # No font, rasterisation, or runtime text drawing is involved.
    $prefixRect = [System.Drawing.Rectangle]::new(0, 0, 42, 22)
    $firstDigitArea = [System.Drawing.Rectangle]::new(42, 0, 7, 22)
    $secondDigitArea = [System.Drawing.Rectangle]::new(48, 0, 12, 22)
    $singleDigitArea = [System.Drawing.Rectangle]::new(42, 0, 18, 22)
    for ($number = 17; $number -le 64; ++$number) {
        $tens = [Math]::Floor($number / 10)
        $ones = $number % 10
        $target = New-ArgbBitmap 72 22
        Copy-GlyphPixels $target $referenceLabels[9] $prefixRect 0

        # The 10..16 labels provide the approved two-digit baseline for 0..6.
        if ($tens -eq 1) {
            Copy-GlyphPixels $target $referenceLabels[9] $firstDigitArea 42
        } else {
            # 2..6 live in the second position of BAR 12..16. 7..9 are
            # available as single-digit labels. All are positioned at the
            # approved first-digit baseline.
            if ($tens -le 6) {
                Copy-GlyphPixels $target $referenceLabels[9 + $tens] $secondDigitArea 42
            } else {
                Copy-GlyphPixels $target $referenceLabels[$tens - 1] $singleDigitArea 37
            }
        }
        if ($ones -le 6) {
            Copy-GlyphPixels $target $referenceLabels[9 + $ones] $secondDigitArea 48
        } else {
            Copy-GlyphPixels $target $referenceLabels[$ones - 1] $singleDigitArea 43
        }
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

    if ($LabelPreviewPath) {
        $preview = New-ArgbBitmap 720 220
        $g = [System.Drawing.Graphics]::FromImage($preview)
        $g.Clear([System.Drawing.Color]::Black)
        $g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::NearestNeighbor
        for ($number = 40; $number -le 64; ++$number) {
            $label = [System.Drawing.Bitmap]::new((Join-Path $labelDir ('bar_label_{0:D2}.png' -f $number)))
            $slot = $number - 40
            $destination = [System.Drawing.Rectangle]::new(($slot % 5) * 144, [Math]::Floor($slot / 5) * 44, 144, 44)
            $g.DrawImage($label, $destination, [System.Drawing.Rectangle]::new(0, 0, 72, 22), [System.Drawing.GraphicsUnit]::Pixel)
            $label.Dispose()
        }
        $g.Dispose()
        Save-Png $preview $LabelPreviewPath
        $preview.Dispose()
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
