Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing

$root = Split-Path -Parent $PSScriptRoot
$layout = Get-Content -LiteralPath (Join-Path $root 'Source\GeneratedLayout.h') -Raw
$faceplatePath = Join-Path $root 'Resources\ui-v2\runtime-1024\static\static_faceplate_1024x683.png'

function Get-LayoutRect([string] $name) {
    $match = [regex]::Match($layout, ($name + '\(\)\s*\{\s*return\s*\{\s*(\d+),\s*(\d+),\s*(\d+),\s*(\d+)'))
    if (-not $match.Success) { throw "Missing layout bounds: $name" }
    [Drawing.Rectangle]::new([int]$match.Groups[1].Value, [int]$match.Groups[2].Value,
                             [int]$match.Groups[3].Value, [int]$match.Groups[4].Value)
}

$bitmap = [Drawing.Bitmap]::new($faceplatePath)
$graphics = [Drawing.Graphics]::FromImage($bitmap)
$graphics.CompositingMode = [Drawing.Drawing2D.CompositingMode]::SourceCopy
foreach ($name in 'hostSyncLampBounds','bpmValueBounds','timeSigValueBounds','presetValueBounds') {
    $graphics.FillRectangle([Drawing.Brushes]::Transparent, (Get-LayoutRect $name))
}
$graphics.Dispose()
$temporaryPath = "$faceplatePath.tmp.png"
$bitmap.Save($temporaryPath, [Drawing.Imaging.ImageFormat]::Png)
$bitmap.Dispose()
Move-Item -LiteralPath $temporaryPath -Destination $faceplatePath -Force
