# RUDE HYPE DSP Preview Notes

## Target

RUDE HYPE is not a simple clipper. The target is a two-knob melody processor that creates upper-mid density, melody forwardness, dirty width, and burned saturation through several lightweight destructive layers.

References checked as titles only to keep token use low:

- `Soju - Bxg Bxng [Krump Music]`
- `Soju - Daywalker [Krump Music]`

## SHOUT Response

Role: push melody forward.

Signal idea:

1. soft asymmetric saturation
2. upper-mid isolation around the 2 kHz to 6.8 kHz presence band
3. parallel exciter from the isolated band
4. micro stereo motion from a no-delay previous-sample blend
5. soft clip
6. mid/side widening

Behavior:

- `0.0`: mostly clean, no artificial forwardness.
- `0.5`: melody gets closer, wider, and more present without heavy fuzz.
- `1.0`: aggressive upper-mid density with controlled ceiling.

Preservation rule: sub/low body is not directly excited by SHOUT.

## BURN Response

Role: burn the melody texture without turning it into unusable noise.

Signal idea:

1. low-safe split around 160 Hz
2. transistor-style biased saturation on the non-low band
3. wavefold layer
4. mild downsample/quantized flavor
5. tape-style envelope compression
6. high fizz exciter above roughly 6.2 kHz
7. soft final limiting

Behavior:

- `0.0`: clean warmth.
- `0.5`: dirty saturation with musical density.
- `1.0`: near-broken burned texture, still level-contained.

Preservation rule: low-safe split is blended back to avoid sub collapse.

## Parameter Mapping

SHOUT controls:

- drive: `1.0 + shout * 4.5`
- exciter mix: `0.18 + shout * 0.42`
- stereo width: `1.0 + shout * 0.36`
- chorus depth: previous-sample micro blend, `0.012 + shout * 0.035`
- transient flatten: light forward-dry relaxation
- soft clip amount: `1.0 + shout * 0.85`

BURN controls:

- wavefold amount: `burn * 0.62`
- transistor drive: `1.0 + burn * 6.0`
- downsample amount: dynamic quantization from 96 to 24 steps
- tape compression: envelope-driven gain reduction
- fizz: high-band parallel fizz, `burn * 0.24`
- density: parallel burn blend, `burn * (0.30 + burn * 0.42)`

## Realtime Safety

- No heap allocation in `processBlock`.
- No FFT.
- No delay buffer allocation.
- `juce::ScopedNoDenormals` is active.
- SHOUT and BURN are APVTS parameters.
- SHOUT and BURN use `juce::SmoothedValue`.
- Output is bounded by `kOutputCeiling`.
- DC blocker state is maintained per stereo channel.

## Validation

Required local reports:

- `reports/latest/rude-hype/dsp-macro-report.json`
- `reports/latest/rude-hype/dsp-policy-check.json`
- `reports/latest/rude-hype/score.json`

Live audio review remains human-guided, but failure conditions are machine-readable:

- clipping explosion
- DC buildup
- missing smoothing
- missing output ceiling
- processBlock realtime policy violation
