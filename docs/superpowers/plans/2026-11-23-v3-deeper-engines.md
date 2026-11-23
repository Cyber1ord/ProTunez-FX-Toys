# ProTunez FX Toys v0.3 — Deeper Engines — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans. Steps use checkbox (`- [ ]`) syntax.

**Goal:** Replace the simple sound engines in Heat, Echo and Plate with circuit-style models whose behaviour changes with level, frequency and time, keeping every control, parameter ID, preset and screen.

**Architecture:** New JUCE-free building blocks in `Source/DSP/Analog.h` (transformer, tube stage with bias/sag, tape hysteresis, head bump, BBD, compander, oversampler 4x). Each effect's `process()` is rebuilt from these blocks; `Params` structs and parameter IDs do not change.

**Tech Stack:** C++17, JUCE 8.0.4, Catch2 3.7.1.

**Spec:** `docs/feature-spec.md` + the user's decision on 23 Nov 2026: "deeper engine for all" (option 2: circuit-style models without reference recordings).

## Global Constraints

- Parameter IDs, ranges and defaults unchanged (saved sessions and presets keep working).
- Reported plug-in latency stays one constant for all effects (`kLatency`); if Heat's latency grows, `kLatency` follows it.
- Audio thread: no allocation, no locks, no per-sample searches; CPU for any effect < 25 % of one core at 48 kHz in the test build (1 s of audio processed in < 250 ms).
- Every engine stays finite and bounded at extreme settings (existing stability tests keep passing).
- Version 0.3.0.

## Review Focus

1. Denormals in new feedback/state variables at silence (CPU spikes on silent tracks).
2. Sample-rate dependence: every time constant and filter scales with the real rate (44.1–192 kHz).
3. Level matching: switching a style should not jump in loudness by more than ~3 dB with Auto on.
4. Hiss/noise generators must be silent when their amount is 0 and never accumulate.
5. Hysteresis/sag state must not latch (stuck DC or gain) after loud input stops.

---

### Task 1: Analog building blocks — `Source/DSP/Analog.h`

**Produces:** `Oversampler4x { void up(float, float out[4]); float down(const float in[4]); static int latency(); }` (two cascaded 2x stages); `Transformer { prepare(sr, lowHz, satAmount); float process(float) }` (low band saturates more: split at lowHz, saturate low band harder, recombine); `TubeStage { prepare(sr); setDrive(g), setBias(b), setSag(s); float process(float) }` (bias shifts with envelope → level-dependent even harmonics; sag = gain dips with slow envelope); `TapeHysteresis { prepare(sr); setDrive(d); float process(float) }` (simplified Jiles–Atherton style, memory of direction); `HeadBump { prepare(sr); set(freqHz, gainDb) }`; `Compander { prepare(sr); float compress(float); float expand(float) }`; `Bbd { prepare(sr, maxSec); setDelay(samples); setBandwidthFromDelay(); float process(float) }`; `NoiseSource { float next(); }` (deterministic, white→pink option).

- [ ] Tests (DSPTests "Analog: …"): transformer THD at 80 Hz > 1.5× THD at 2 kHz (same level); tube second-harmonic ratio changes > 20 % between −20 dBFS and −3 dBFS input; sag: steady loud tone gain after 300 ms < gain at 5 ms by ≥ 1 dB; hysteresis: rising vs falling half-cycle outputs at same input differ (loop area > 0) and output returns to 0 within 50 ms after input stops; compander compress→expand round-trip within 1 dB for steady tone; BBD: bandwidth (−3 dB) at 500 ms delay < at 50 ms; oversampler 4x passes 18 kHz within 1 dB, latency integer.
- [ ] Implement; all pass; commit `feat: analog building blocks`.

### Task 2: Heat deep engine

Each style = Transformer(in) → TubeStage or tape chain → output transformer/tone, 4x oversampled. A: tape (HeadBump 80 Hz +2 dB, hysteresis, HF loss); E: console (strong input transformer, mild bias); N: preamp (transformer + class-A stage, moderate bias shift); T: triode (big bias shift, sag); P: pentode (hard knee stage, sag, odd-heavy).

- [ ] Tests: existing Heat tests still pass (drive range, punish, steep, thump, alignment, auto gain, styles differ); new: style A low-frequency bump (+1..+3 dB at 80 Hz at low drive); T and P sag (≥ 1 dB slow gain reduction at drive 8); E/N THD at 80 Hz > THD at 2 kHz; harmonic balance shifts with level for T; Auto keeps every style within ±3 dB of input at drive 6; CPU < 250 ms per second at 48 kHz.
- [ ] Implement; latency = Oversampler4x latency; processor kLatency follows; commit `feat: Heat deep analog engine`.

### Task 3: Echo deep engines

`EchoEngine` per style kind: `tape` (pre-emphasis → hysteresis → de-emphasis; gap loss LPF whose cutoff ∝ tape speed (∝ 1/delay); HeadBump ∝ speed; wow (0.5–1.5 Hz random walk) + flutter (6–12 Hz sum of partials); hiss level), `bbd` (Compander around Bbd; bandwidth falls with delay; clock noise), `digital` (clean) / `lofi` (decimation ∝ delay, bit depth 8–12), `radio` (band-limit + AM distortion + noise + slow fading), `spring` (oil-can: short modulated allpass smear). `StyleDef` gains `kind` and engine settings; Style Edit still applies on top.

- [ ] Tests: tape: 1 s echo darker than 100 ms (energy above 5 kHz lower by ≥ 3 dB); bbd: same, and compander breathing (noise floor follows signal by ≥ 6 dB); lofi: aliasing images present (energy above new Nyquist); radio styles band-limited (< −20 dB at 100 Hz) with noise > −70 dBFS when signal present; hiss silent with input silent and feedback 0 after reset? (hiss allowed while engine active only if style hiss > 0; at Clean Digital output exactly 0 for 0 input); all existing Echo tests pass; CPU < 250 ms/s for each kind at 48 kHz.
- [ ] Implement; commit `feat: Echo deep engines (tape, BBD, lo-fi, radio, spring)`.

### Task 4: Plate deep engine

16-line feedback delay network (Householder/Hadamard mixing) with per-line 3-band decay filters set from the model's low/mid/high RT ratios; dispersion = cascade of 8 first-order allpasses on the input (high frequencies arrive first, plate "splash"); transducer (driver) resonance + pickup tilt per model; modulation on 4 lines. Decay control sets mid RT60; models set ratios (e.g. Large Steel: low ×0.7, high ×0.55).

- [ ] Tests: existing Plate tests (RT60 within 20 % at 500 Hz per model, infinite holds, preamps, pre-delay, width, duck, freeze, bounded 192 kHz); new: RT at 4 kHz shorter than at 500 Hz per the model ratio (±25 %); dispersion: 6 kHz energy peak in the IR arrives earlier than 300 Hz peak; density: after 30 ms the IR envelope has no single sample > 6× local RMS; CPU < 250 ms/s at 48 kHz.
- [ ] Implement; commit `feat: Plate deep engine (dense FDN, dispersion, per-band decay)`.

### Task 5: Release 0.3.0 + review

- [ ] Version 0.3.0, README "What's new"; full suites green; push branch; CI green (auval); fresh reviewer on the branch; fix Critical/Important with tests; fast-forward main → release; screenshots unchanged (no UI change) — send release link.
