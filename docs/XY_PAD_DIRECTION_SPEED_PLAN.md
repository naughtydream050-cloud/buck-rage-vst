# XY Pad: horizontal direction, vertical speed

Approved by the user on 2026-11-18. This replaces only the short-lived
`Y axis Speed` opt-in and `Load BRAVO XY Accelerator` experiment.

## Fixed behaviour

- Every saved XY path uses this rule; there is no mode, parameter, hidden
  switch, or special BAR.
- X chooses source position and signed direction. Left-to-right is forward;
  right-to-left is reverse. A change of X direction keeps the existing
  crossfade/re-entry protection.
- Y controls the signed read-rate magnitude continuously for every output
  sample, not just when a grain is created. With `y` normalized from bottom
  `0` to top `1`, `rate(y) = 0.5 + 0.5*y + y*y`; it is 0.5x, 1x and 2x at
  bottom, centre and top respectively. Natural pitch follows the read rate;
  no FFT or independent pitch shifter is introduced.
- A near-vertical path retains the preceding X direction and loops the current
  short slice. A first vertical path defaults forward. Its loop phase advances
  at the same continuous Y rate, including reverse direction.
- Existing trajectories remain serialized and visually unchanged. Legacy
  `yAxisSpeedEnabled` data is accepted but ignored and is not emitted again.

## Scope

Change only XY engine, the obsolete state/menu plumbing, and focused tests.
Keep UI artwork/layout, BRAVO 1/2, COUNT 5, XY priority, all public parameter
IDs, manual BAR mapping, and the 1024-sample latency. COUNT-tail repair is
explicitly outside this change. No audio-thread allocation, lock, ValueTree,
I/O or logging is permitted.

## Implementation constraints

- Preserve X-derived base read magnitude and source selection. Apply the Y
  multiplier while each active grain is read, with a persistent fractional read
  position so a changing Y value does not jump the source position.
- Give the hold loop a fractional phase and its retained direction. Use the
  existing seam/crossfade path on forward or reverse wrap.
- Remove the `Y axis Speed` and `Load BRAVO XY Accelerator` right-click items,
  their state methods, state property emission, processor callback plumbing,
  and obsolete tests. Do not alter saved XY point data.

## Delivery gate

The exact source must pass the numbered criteria, focused native tests,
Windows CI, Jev per-criterion review and Codex review before a new, SHA-matched
bundle may be copied to `output/windows/manual-test/<new-folder>`. Existing
manual-test bundles, candidate, final and Program Files are not overwritten.
FL Studio listening remains `NOT TESTED` until the user performs it.
