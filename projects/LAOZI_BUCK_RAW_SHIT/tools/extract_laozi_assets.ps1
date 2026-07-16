param(
    [Parameter(Mandatory = $true)]
    [string]$ReferenceImage
)

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent (Split-Path -Parent $PSCommandPath)
$referenceDir = Join-Path $projectRoot 'ui/reference'
$resourcesDir = Join-Path $projectRoot 'Resources'
New-Item -ItemType Directory -Force -Path $referenceDir, $resourcesDir | Out-Null

Add-Type -AssemblyName System.Drawing

function Save-CircularCrop {
    param(
        [System.Drawing.Bitmap]$Source,
        [int]$CenterX,
        [int]$CenterY,
        [int]$Size,
        [string]$Destination
    )

    $crop = New-Object System.Drawing.Bitmap $Size, $Size, ([System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $graphics = [System.Drawing.Graphics]::FromImage($crop)
    $graphics.Clear([System.Drawing.Color]::Transparent)
    $left = $CenterX - [int]($Size / 2)
    $top = $CenterY - [int]($Size / 2)
    $graphics.DrawImage($Source,
        (New-Object System.Drawing.Rectangle 0, 0, $Size, $Size),
        $left, $top, $Size, $Size,
        [System.Drawing.GraphicsUnit]::Pixel)
    $graphics.Dispose()

    $radius = $Size / 2.0
    for ($y = 0; $y -lt $Size; $y++) {
        for ($x = 0; $x -lt $Size; $x++) {
            $dx = ($x + 0.5) - $radius
            $dy = ($y + 0.5) - $radius
            if (($dx * $dx + $dy * $dy) -gt ($radius * $radius)) {
                $crop.SetPixel($x, $y, [System.Drawing.Color]::FromArgb(0, 0, 0, 0))
            }
        }
    }

    $crop.Save($Destination, [System.Drawing.Imaging.ImageFormat]::Png)
    $crop.Dispose()
}

if (-not (Test-Path -LiteralPath $ReferenceImage)) {
    throw "Reference image not found: $ReferenceImage"
}

$source = [System.Drawing.Bitmap]::FromFile((Resolve-Path -LiteralPath $ReferenceImage))
try {
    if ($source.Width -ne 1280 -or $source.Height -ne 905) {
        throw "Unexpected reference size: $($source.Width)x$($source.Height). Expected 1280x905."
    }

    $source.Save((Join-Path $referenceDir 'reference.png'), [System.Drawing.Imaging.ImageFormat]::Png)
    $faceplatePath = Join-Path $resourcesDir 'faceplate_laozi_buck_raw_shit.png'
    $source.Save($faceplatePath, [System.Drawing.Imaging.ImageFormat]::Png)

    # Full tick-ring crops cover the reference knobs exactly, preventing double-knob rendering.
    Save-CircularCrop $source 210 526 196 (Join-Path $resourcesDir 'knob_pressure.png')
    Save-CircularCrop $source 458 526 196 (Join-Path $resourcesDir 'knob_kick.png')
    Save-CircularCrop $source 691 526 196 (Join-Path $resourcesDir 'knob_aura.png')
    Save-CircularCrop $source 918 526 196 (Join-Path $resourcesDir 'knob_glue.png')
    Save-CircularCrop $source 1155 687 100 (Join-Path $resourcesDir 'knob_output.png')
}
finally {
    $source.Dispose()
}
