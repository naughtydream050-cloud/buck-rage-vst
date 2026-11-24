# XY BAR boundary spill fix

Base: 99dca1df8eb820e8b918c472bd480c50832adf5c. Preserve all unrelated dirty files.

The 1024-sample XY PCM/mask delay can carry the preceding BAR's XY into the raw output of the next BAR. Keep the reported latency and aligned Dry/Preset delay. At output mixing, compare the current BAR to the delayed XY source BAR: suppress only cross-BAR XY, returning to the aligned Dry/Preset path. Fade XY during the final 3 ms before the boundary so suppression does not click. Do not alter COUNT 5, BRAVO, XY trajectory semantics, UI, state or parameter IDs.

RED/GREEN processor render: BAR 1 has XY; BAR 2 has either no Preset or a Preset. Both must be identical to their no-XY reference from the first sample of BAR 2, with a bounded pre-boundary fade and no change away from the boundary. Test 44.1/48/96 kHz, stereo, block boundaries, bypass and 1024-sample latency. Run native regression, Windows CI, same-source 18-condition benchmark, per-AC Jev and Codex review. Only all-PASS may create a new D manual-test folder; preserve old versions, candidate, final and Program Files. FL listening remains NOT TESTED.
