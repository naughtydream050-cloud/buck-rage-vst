param(
    [string] $ProjectRoot = (Resolve-Path "$PSScriptRoot\..").Path,
    [string] $LabelPreviewPath = '',
    [string] $VerifiedLabelDirectory = 'D:\Development\RAZOR_FACE_COMPANY\01_PLUGINS\archive\legacy-cleanup-20260809\project-output-backups\ToyotomiHideyoshi\legacy-pre-01-ssot-20260809-090700\Resources\ui-bar-map-preview\bar-labels'
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

function Get-BrightColumnGroups([System.Drawing.Bitmap] $source, [System.Drawing.Rectangle] $searchArea) {
    $groups = @()
    $start = -1
    for ($x = $searchArea.Left; $x -lt $searchArea.Right; ++$x) {
        $active = $false
        for ($y = $searchArea.Top; $y -lt $searchArea.Bottom; ++$y) {
            $pixel = $source.GetPixel($x, $y)
            $luminance = (0.2126 * $pixel.R) + (0.7152 * $pixel.G) + (0.0722 * $pixel.B)
            if ($luminance -ge 72.0) { $active = $true; break }
        }
        if ($active -and $start -lt 0) { $start = $x }
        if (-not $active -and $start -ge 0) {
            $groups += [System.Drawing.Rectangle]::new($start, $searchArea.Top, $x - $start, $searchArea.Height)
            $start = -1
        }
    }
    if ($start -ge 0) {
        $groups += [System.Drawing.Rectangle]::new($start, $searchArea.Top, $searchArea.Right - $start, $searchArea.Height)
    }
    if ($groups.Count -eq 0) { throw "No bright glyph group in $searchArea." }
    return ,$groups
}

function Copy-DigitGroup([System.Drawing.Bitmap] $target, [System.Drawing.Bitmap] $source, [bool] $useLastGroup, [int] $targetX) {
    $groups = Get-BrightColumnGroups $source ([System.Drawing.Rectangle]::new(41, 0, 31, 22))
    $glyph = if ($useLastGroup) { $groups[$groups.Count - 1] } else { $groups[0] }
    Copy-GlyphPixels $target $source $glyph $targetX
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

    # The archived preview set contains the reviewed full-size BAR 01..64
    # pixel labels. Adopt them verbatim rather than re-composing tiny glyphs:
    # no font, rasterisation, or runtime text drawing is used.
    if (!(Test-Path -LiteralPath $VerifiedLabelDirectory)) { throw "Verified BAR label source is missing: $VerifiedLabelDirectory" }
    for ($number = 1; $number -le 64; ++$number) {
        $name = 'bar_label_{0:D2}.png' -f $number
        $source = Join-Path $VerifiedLabelDirectory $name
        if (!(Test-Path -LiteralPath $source)) { throw "Verified BAR label is missing: $source" }
        $image = [System.Drawing.Bitmap]::new($source)
        if ($image.Width -ne 66 -or $image.Height -ne 24) { $image.Dispose(); throw "Unexpected verified label size: $name" }
        Save-Png $image (Join-Path $labelDir $name)
        $image.Dispose()
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
