Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing

$root = Split-Path -Parent $PSScriptRoot
$referencePath = Join-Path $root 'Resources\ui-v2\reference\master-timeline-reference-1024x683.png'
$faceplatePath = Join-Path $root 'Resources\ui-v2\runtime-1024\static\static_faceplate_1024x683.png'
$manifestPath = Join-Path $root 'ui\v2\visual_acceptance_manifest.json'
$reference = [System.Drawing.Bitmap]::new($referencePath)
$faceplate = [System.Drawing.Bitmap]::new($faceplatePath)

try {
    if ($reference.Width -ne 1024 -or $reference.Height -ne 683 -or $faceplate.Width -ne 1024 -or $faceplate.Height -ne 683) {
        throw 'STATIC_FACEPLATE_PIXEL_IDENTITY_GATE: expected two 1024x683 PNGs.'
    }

    $manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
    $results = @()
    $dynamicRegions = @($manifest.regions | Where-Object { $_.dynamic_region })
    foreach ($region in $manifest.regions | Where-Object { -not $_.dynamic_region }) {
        $b = $region.bounds
        $different = 0
        for ($y = $b[1]; $y -lt ($b[1] + $b[3]); ++$y) {
            for ($x = $b[0]; $x -lt ($b[0] + $b[2]); ++$x) {
                $coveredByDynamicRegion = $false
                foreach ($dynamic in $dynamicRegions) {
                    $d = $dynamic.bounds
                    if ($x -ge $d[0] -and $x -lt ($d[0] + $d[2]) -and $y -ge $d[1] -and $y -lt ($d[1] + $d[3])) {
                        $coveredByDynamicRegion = $true
                        break
                    }
                }
                if (-not $coveredByDynamicRegion -and $reference.GetPixel($x, $y).ToArgb() -ne $faceplate.GetPixel($x, $y).ToArgb()) { ++$different }
            }
        }
        $results += [pscustomobject]@{ region = $region.name; bounds = $b; differing_pixels = $different; pass = ($different -eq 0) }
    }

    $report = [pscustomobject]@{
        gate = 'STATIC_FACEPLATE_PIXEL_IDENTITY_GATE'
        reference = $referencePath
        faceplate = $faceplatePath
        regions = $results
        pass = (@($results | Where-Object { -not $_.pass }).Count -eq 0)
    }
    $report | ConvertTo-Json -Depth 5
    if (-not $report.pass) { exit 1 }
} finally {
    $reference.Dispose()
    $faceplate.Dispose()
}
