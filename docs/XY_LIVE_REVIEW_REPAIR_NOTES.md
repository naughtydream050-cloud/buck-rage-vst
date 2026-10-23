# XY live review repair, 2026-10-23

The original live Plan and all sixteen requirements are retained. This records
implementation corrections required by those constraints, not new product behavior.

RED source: 22ea548825ff7057a3d17366ffe75b5e2c885d0f.
RED CI: 39886967471, job 120197286512, XY_BUILD_EXIT=0, XY_TEST_EXIT=1.
The executed failures were: immediate_reverse_grains=0; an old grain remained
active after fade; hold phase wrapped to 0 instead of 96 at the seam.
Jev 1.13 supplemental source audit independently classified these three FAIL.

Minimum changes:
- Start a valid pending fold/jump grain immediately, without a ten-ms hop wait.
- Retire an old grain at its three-ms fade end so gain cannot return to one.
- Continue after the crossfaded two-ms head segment when wrapping a hold grain.
- Keep bounded snapshots during the source-ended eight-ms fade; discard them when
  the fade finishes. Do not abruptly zero actual XY audio at the silence threshold.
- Prepare the Processor's live XY path without the unused full-BAR PCM bank or
  background offline renderer, as required by the approved live architecture.

Additional tests compare forward/reverse snapshot PCM to the scalar reference,
check the actual source-ended audio contribution for a hard cut, and assert that
the real Processor prepares no legacy full-BAR bank. Existing non-live engine
unit fixtures keep their legacy prepare mode; the production Processor uses only
the bounded live allocation path. No UI, State, APVTS IDs, absolute timeline,
existing preset algorithm, budget, credential, or model-route change is included.

DeepSeek was invoked once as the provider-failure fallback for the separate AC-10
test contract; it returned INCOMPLETE with no edits. Codex completed the escalated
test and critical review work. git diff --check is preliminary verification only.
The subsequent Windows CI, all actual Jev AC verdicts, and Sol approval remain
mandatory before candidate promotion. FL validation remains NOT TESTED.
