param(
    [string] $ProjectRoot = (Split-Path -Parent $PSScriptRoot),
    [Parameter(Mandatory = $true)] [string] $DefaultUiPath
)

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing

$bars = Join-Path $ProjectRoot 'Resources\ui-v2\runtime-1024\bars'

function Save-PngAtomically([System.Drawing.Bitmap] $bitmap, [string] $path) {
    $temporary = "$path.tmp.png"
    $bitmap.Save($temporary, [System.Drawing.Imaging.ImageFormat]::Png)
    Copy-Item -LiteralPath $temporary -Destination $path -Force
    Remove-Item -LiteralPath $temporary -Force
}

function Read-PngCopy([string] $path) {
    $bytes = [System.IO.File]::ReadAllBytes($path)
    $stream = [System.IO.MemoryStream]::new([byte[]] $bytes, $false)
    $image = [System.Drawing.Image]::FromStream($stream)
    $copy = [System.Drawing.Bitmap]::new($image)
    $image.Dispose(); $stream.Dispose()
    return $copy
}

# The user-provided neutral default UI is the source for the complete normal
# BAR frame. It is cropped once at the master cell location (BAR 11) and
# reduced offline to the native 56 x 80 canvas. Only each target's label and
# mini preview are retained from its existing normal cell.
$master = Read-PngCopy $DefaultUiPath
if ($master.Width -ne 1280 -or $master.Height -ne 853) { throw 'Expected 1280x853 default UI source' }
$neutralCell = [System.Drawing.Bitmap]::new(56, 80, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
$masterGraphics = [System.Drawing.Graphics]::FromImage($neutralCell)
$masterGraphics.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
$masterGraphics.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::Half
$masterGraphics.DrawImage($master, [System.Drawing.Rectangle]::new(0, 0, 56, 80), 472, 276, 70, 100, [System.Drawing.GraphicsUnit]::Pixel)
$masterGraphics.Dispose()
$master.Dispose()
foreach ($targetId in 11, 27, 43, 59) {
    $targetPath = Join-Path $bars ("bar_{0:D2}_normal.png" -f $targetId)
    $target = Read-PngCopy $targetPath
    if ($target.Width -ne 56 -or $target.Height -ne 80) {
        throw "Unexpected BAR cell dimensions for $targetId"
    }
    $result = [System.Drawing.Bitmap]::new($neutralCell)
    $graphics = [System.Drawing.Graphics]::FromImage($result)
    $graphics.CompositingMode = [System.Drawing.Drawing2D.CompositingMode]::SourceCopy
    # Preserve glyph pixels only; the full label band carries the old gold rim.
    $graphics.DrawImage($target, [System.Drawing.Rectangle]::new(4, 5, 48, 14), 4, 5, 48, 14, [System.Drawing.GraphicsUnit]::Pixel)
    $graphics.DrawImage($target, [System.Drawing.Rectangle]::new(8, 36, 40, 20), 8, 36, 40, 20, [System.Drawing.GraphicsUnit]::Pixel)
    $graphics.Dispose()
    Save-PngAtomically $result $targetPath
    $result.Dispose(); $target.Dispose()
}
$neutralCell.Dispose()

Get-FileHash -LiteralPath (11, 27, 43, 59 | ForEach-Object { Join-Path $bars ("bar_{0:D2}_normal.png" -f $_) }) -Algorithm SHA256 |
    Select-Object Path, Hash
