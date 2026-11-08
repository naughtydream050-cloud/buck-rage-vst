# BRAVO 1/2 source-specific D manual-test plan

Scope is limited to the existing BRAVO implementation and its measured event
map. BRAVO 1 events are absolute BAR 3, 53, 54, 56, 70, 99. BRAVO 2 events
are absolute BAR 2, 4, 7, 19, 20, 24, 31, 33. Empty BARs are dry. No
four-BAR template may be reintroduced.

The existing 64-slot manual BAR MAP remains manual state. Events after BAR 64
use absolute transport and must never alias to a manual slot. UI geometry,
manual state, XY priority, parameter IDs, existing effects, latency and the
1024-sample alignment remain frozen.

The release gate is: focused native tests, Windows CI, diagnostic/performance
evidence, Jev AC review, Codex review, then VST3 provenance validation and a
project-local `output\\windows\\manual-test` copy. No candidate, final or
Program Files change is permitted. Original-audio similarity and FL listening
remain user validation, not automated claims.
