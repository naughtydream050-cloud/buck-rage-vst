param(
    [string] $ProjectRoot = (Split-Path -Parent $PSScriptRoot)
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

# Each affected cell retains its exact existing label/mini pixels while every
# other pixel is replaced by the approved 56x80 neutral shell. This removes
# the full baked selected shell, not merely its outermost four pixels.
$shellPath = Join-Path $ProjectRoot 'Resources\ui-v2\runtime-1024\bar-map\shells\bar_cell_shell_normal_56x80.png'
foreach ($targetId in 11, 27, 43, 59) {
    $targetPath = Join-Path $bars ("bar_{0:D2}_normal.png" -f $targetId)
    $target = Read-PngCopy $targetPath
    $shell = Read-PngCopy $shellPath
    if ($target.Width -ne 56 -or $target.Height -ne 80 -or $shell.Width -ne 56 -or $shell.Height -ne 80) {
        throw "Unexpected BAR cell dimensions for $targetId"
    }
    $result = [System.Drawing.Bitmap]::new($shell)
    $graphics = [System.Drawing.Graphics]::FromImage($result)
    $graphics.CompositingMode = [System.Drawing.Drawing2D.CompositingMode]::SourceCopy
    # Preserve glyph pixels only; the full label band carries the old gold rim.
    $graphics.DrawImage($target, [System.Drawing.Rectangle]::new(4, 5, 48, 14), 4, 5, 48, 14, [System.Drawing.GraphicsUnit]::Pixel)
    $graphics.DrawImage($target, [System.Drawing.Rectangle]::new(8, 36, 40, 20), 8, 36, 40, 20, [System.Drawing.GraphicsUnit]::Pixel)
    $graphics.Dispose()
    Save-PngAtomically $result $targetPath
    $result.Dispose(); $target.Dispose(); $shell.Dispose()
}

Get-FileHash -LiteralPath (11, 27, 43, 59 | ForEach-Object { Join-Path $bars ("bar_{0:D2}_normal.png" -f $_) }) -Algorithm SHA256 |
    Select-Object Path, Hash
