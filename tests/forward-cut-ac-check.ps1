param(
  [Parameter(Mandatory=$true)][string]$Ac,
  [string]$TestLog = 'toyotomi-scratch-engine-tests.log'
)
$ErrorActionPreference = 'Stop'
if (!(Test-Path -LiteralPath $TestLog)) { throw "Missing deterministic test log: $TestLog" }
$text = Get-Content -LiteralPath $TestLog -Raw
if ($text -match '(?m)^FAIL ') { throw 'Scratch engine test log contains FAIL.' }
$required = @{
  'AC-01' = @('forward-cut-read-rate-0-25','forward-cut-read-rate-1-0','forward-cut-read-rate-4-0')
  'AC-02' = @('forward-cut-read-rate-0-25','forward-cut-read-rate-1-0','forward-cut-read-rate-4-0')
  'AC-03' = @('forward-cut-cycle-0-25','forward-cut-cycle-1-0','forward-cut-cycle-4-0')
  'AC-04' = @('forward-cut-speed-increases-retrigger-density')
  'AC-05' = @('forward-cut-fixed-forward-capture')
  'AC-06' = @('forward-cut-no-click-or-nonfinite-output')
  'AC-07' = @('forward-cut-transport-stop-returns-dry')
  'AC-08' = @('forward-cut-wet-throughout-one-bar')
  'AC-09' = @('depth-zero-is-bit-exact-dry','forward-cut-wet-throughout-one-bar')
  'AC-10' = @('baby-read-motion-alternates-forward-and-reverse','backspin-window-stays-bounded-through-one-bar','tape-brake-speed-shapes-curve-and-always-stops-at-length-end','off-steady-state-is-bit-exact-after-transition')
  'AC-11' = @('forward-cut-short-gap-does-not-end-source','forward-cut-30ms-source-silence-latches-current-bar','forward-cut-source-ended-wet-to-dry','forward-cut-source-latch-ignores-input-restart')
  'AC-12' = @('v2-host-sync-on-variable-blocks-keep-backspin-wet-and-history','v2-host-sync-off-keeps-internal-timeline-origin')
  'AC-13' = @('forward-cut-all-lengths')
  'AC-14' = @('continuous-absolute-bars-1-through-64-do-not-reset')
  'AC-15' = @('forward-cut-no-click-or-nonfinite-output')
}
if (!$required.ContainsKey($Ac)) { throw "Unknown AC: $Ac" }
foreach ($label in $required[$Ac]) {
  if ($text -notmatch [regex]::Escape("PASS $label")) { throw "Missing PASS evidence: $label" }
}
Write-Output "PASS $Ac"
