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

# Each affected cell retains its own approved label/mini content and receives
# only the 4px rim from an adjacent approved normal cell on the same page.
foreach ($pair in @(@{ Target = 11; Donor = 10 }, @{ Target = 27; Donor = 26 }, @{ Target = 43; Donor = 42 }, @{ Target = 59; Donor = 58 })) {
    $targetPath = Join-Path $bars ("bar_{0:D2}_normal.png" -f $pair.Target)
    $donorPath = Join-Path $bars ("bar_{0:D2}_normal.png" -f $pair.Donor)
    $target = Read-PngCopy $targetPath
    $donor = Read-PngCopy $donorPath
    if ($target.Width -ne 56 -or $target.Height -ne 80 -or $donor.Width -ne 56 -or $donor.Height -ne 80) {
        throw "Unexpected BAR cell dimensions for $($pair.Target) / $($pair.Donor)"
    }
    for ($y = 0; $y -lt 80; ++$y) {
        for ($x = 0; $x -lt 56; ++$x) {
            if ($x -lt 4 -or $x -ge 52 -or $y -lt 4 -or $y -ge 76) {
                $target.SetPixel($x, $y, $donor.GetPixel($x, $y))
            }
        }
    }
    Save-PngAtomically $target $targetPath
    $target.Dispose(); $donor.Dispose()
}

# Keep the runtime manifest's provenance hashes authoritative for the four
# replaced normal assets.
$manifestPath = Join-Path $ProjectRoot 'ui\v2\runtime-manifest.json'
$manifest = Get-Content -Raw -LiteralPath $manifestPath | ConvertFrom-Json -Depth 100
foreach ($bar in $manifest.barMap.bars) {
    if ($bar.bar_id -in 11, 27, 43, 59) {
        $path = Join-Path $ProjectRoot $bar.normal_asset.file
        $bar.normal_asset.sha256 = (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash
        $bar.normal_asset.provenance = 'neutral-default-or-approved-normal-rim-replacement'
    }
}
$manifest | ConvertTo-Json -Depth 100 | Set-Content -LiteralPath $manifestPath -Encoding utf8

Get-FileHash -LiteralPath (11, 27, 43, 59 | ForEach-Object { Join-Path $bars ("bar_{0:D2}_normal.png" -f $_) }) -Algorithm SHA256 |
    Select-Object Path, Hash
