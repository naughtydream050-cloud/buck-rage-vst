param(
    [string] $Root = (Resolve-Path "$PSScriptRoot\..\..\..").Path
)

$ErrorActionPreference = "Stop"

$project = Join-Path $Root "projects\VINTAGE_RAWNESS"
$reports = Join-Path $Root "reports\latest\vintage-rawness"
$deliverables = Join-Path $Root "deliverables"
$windowsOut = Join-Path $deliverables "VINTAGE_RAWNESS_WINDOWS_FL_STUDIO_TEST"
$macOut = Join-Path $deliverables "VINTAGE_RAWNESS_MAC_TEST"

New-Item -ItemType Directory -Force -Path $windowsOut, $macOut | Out-Null

function Copy-LatestBundle {
    param(
        [string] $Filter,
        [string] $Destination,
        [scriptblock] $PathPredicate = { param($item) $true }
    )

    $bundle = Get-ChildItem -Path $reports, (Join-Path $Root "reports\latest") -Recurse -Directory -Filter $Filter -ErrorAction SilentlyContinue |
        Where-Object { & $PathPredicate $_ } |
        Sort-Object LastWriteTime -Descending |
        Select-Object -First 1

    if (-not $bundle) {
        Write-Warning "No $Filter bundle found under reports/latest yet."
        return $false
    }

    $target = Join-Path $Destination $bundle.Name
    if (Test-Path -LiteralPath $target) {
        Remove-Item -LiteralPath $target -Recurse -Force
    }

    Copy-Item -LiteralPath $bundle.FullName -Destination $target -Recurse -Force
    Write-Host "Copied $($bundle.FullName) -> $target"
    return $true
}

$windowsCopied = Copy-LatestBundle -Filter "VINTAGE RAWNESS.vst3" -Destination $windowsOut -PathPredicate {
    param($item)
    $item.FullName -notmatch "mac|Mac|AU|component"
}
$macVst3Copied = Copy-LatestBundle -Filter "VINTAGE RAWNESS.vst3" -Destination $macOut -PathPredicate {
    param($item)
    $item.FullName -match "mac|Mac"
}
$macAuCopied = Copy-LatestBundle -Filter "VINTAGE RAWNESS.component" -Destination $macOut -PathPredicate {
    param($item)
    $item.FullName -match "mac|Mac|AU|component"
}

Copy-Item -LiteralPath (Join-Path $project "docs\mac-install-test.md") -Destination (Join-Path $macOut "mac-install-test.md") -Force

foreach ($package in @(
    @{ Path = $windowsOut; Zip = Join-Path $deliverables "VINTAGE_RAWNESS_WINDOWS_FL_STUDIO_TEST.zip"; Ready = $windowsCopied },
    @{ Path = $macOut; Zip = Join-Path $deliverables "VINTAGE_RAWNESS_MAC_TEST.zip"; Ready = ($macVst3Copied -or $macAuCopied) }
)) {
    if ($package.Ready) {
        if (Test-Path -LiteralPath $package.Zip) {
            Remove-Item -LiteralPath $package.Zip -Force
        }
        Compress-Archive -LiteralPath (Join-Path $package.Path "*") -DestinationPath $package.Zip -Force
        Write-Host "Created $($package.Zip)"
    }
}

$summary = [ordered]@{
    schemaVersion = 1
    generatedAt = (Get-Date).ToUniversalTime().ToString("o")
    windowsCopied = $windowsCopied
    macVst3Copied = $macVst3Copied
    macAuCopied = $macAuCopied
    windowsDeliverable = $windowsOut
    macDeliverable = $macOut
}

$summaryPath = Join-Path $Root "reports\latest\vintage-rawness-package-report.json"
$summary | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath $summaryPath -Encoding UTF8
Write-Host "Wrote $summaryPath"
