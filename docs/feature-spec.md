# ProTunez FX Toys — Feature Spec (v2 plan)

Approved 23 Nov 2026. Live copy: https://claude.ai/artifact/6RvLnoEChVjWfgWSQmUjYE
Screen mockups: [`docs/mockups/`](mockups/) (open `index.html?fx=heat|echo|plate|morph|wider|filter|crystal|space`).

## How the app works

One plug-in (Apple Silicon, AU + VST3 + standalone). An **Effect** menu in the header switches the whole
window to one of 8 effects. Each screen has the same controls and functions as the original, with our own
name and artwork, and a "like …" reference beside the title.

**Header (every screen):** Effect menu · previous/next preset · preset name and browser (category folders) ·
Save · Revert · settings · Bypass.

**Shared on every screen:** Tweak button for hidden controls · resizable window (drag corner or size menu) ·
Shift-drag fine adjust, double-click reset, right-click exact value · Input/Output with LED meters (yellow =
6 dB below clip, red = clip) where the original has them · tempo sync + Tap Tempo where the original has it ·
A/B compare, undo, full automation · shared Shape Editor and Rhythm Editor.

| Our name | Like | Main controls | Hidden (Tweak) |
|---|---|---|---|
| Heat | Decapitator | Styles A/E/N/T/P (tape, console, preamp, triode, pentode characters), Drive 0–10, Punish (+20 dB), Attitude meter, Auto gain, Output, Mix, Low Cut + Thump, Tone, High Cut + Steep (6 or 30 dB/oct) | — |
| Echo | EchoBoy | ~30 styles; Single/Dual/Ping-Pong/Rhythm; Echo Time as ms/Note/Dot/Trip/Pattern; Tap, BPM 30–300, host sync; Prime Numbers; Feedback; Saturation; Low/High Cut; Mix; In/Out; Groove (shuffle↔swing); Feel (drag↔rush) | Per mode: Accent, Width, L/R Offset 0–25 ms; Dual: Accent 1/2, Feedback Mix, Balance, Feedback Balance; Rhythm: Length 1–8 beats, Grid 1/2–1/64, Warp, Pan shape (Double, Center, Alt 1–3, Sweep L/R, Pan). Rhythm editor: 16 taps, Repeats, Shape (Decay, Reverse, Swell, Fade, Nonlin), "The One" tap, save patterns. Style Edit: 3-band EQ with per-repeat Decay, Mid Res; saturation types Clean, Tape, Warm, Pump, Dirt, Hard/Soft/Warm/Bright Limit; Diffusion Amount/Size/Loop-Post; Wobble Depth/Rate/Shape/Sync |
| Plate | SuperPlate | 5 plate models; preamp Tube/Solid/Clean; Decay 0.5 s–∞ (kept across models); Pre-Delay 0–250 ms; Modulation; Low/High filter; In/Out; Mix (default 100% wet) | Mod Rate, Width, L/R Balance, Low Cut slope, 2-band parametric EQ on a graph (Alt-scroll = Q), Auto-Decay ducking |
| Voice Morph | Little AlterBoy | Pitch ±12 st, Formant, Link, Transpose/Quantize/Robot (Robot 0 = C5), Drive (tube), Mix | MIDI note → pitch (intervals; vocoder-style in Robot) |
| Wider | MicroShift | Styles I/II/III, Detune 50–200%, Delay 50–200%, Focus 20 Hz–10 kHz, Mix | — |
| Filter | FilterFreak 1 & 2 | Per filter: Freq, Res (self-oscillates), Gain, Shape LP/BP/HP/BR, Poles 2/4/6/8, Mod; response display; Mix, In/Out. Modes: LFO (0–100 Hz), Rhythm (tap/host, note, shape, groove), Envelope (Threshold, Gain, Attack, Release), Random S&H, Step (trigger), ADSR. Dual: Parallel/Serial, Link | Shape Editor, Smoothing + mode (Lin/Sin/Exp/Sym/Rev), Freq Mod ±10 oct, Res Mod, Level Mod, L/R Offset, L/R Mode Normal/Flipped, Analog Style (Clean, Fat, Squash, Dirt, Crunch, Shred, Pump), Rhythm Editor (bars, beats/bar, grid, presets) |
| Crystal Echo | Crystallizer | Mix, Pitch (cents), Splice, Delay, Recycle with LED readouts; Threshold; Gate↔Duck; Reverse/Forward; MIDI Sync; In/Out | FB Mode, Pitch/Splice/Delay L/R offsets, Gate/Duck Attack & Release, Low/High Cut |
| Space | SpaceBlender | Visualizer with draggable cursor; Time 100 ms–60 s (Sync 1–32 beats); Warp; Color; Texture (grains↔lush); Mod; Mix; Freeze | Right-click % readout, Shift fine adjust |

## Extras (all approved)

- Echo: ducking (echoes dip while singing)
- Voice Morph: key + scale for Quantize
- Plate and Echo: Freeze button
- Wider: mono-check button
- Preset folders for Church Vocal, Podcast Voice and Choir on every effect

## Build order

1. App frame: Effect menu, shared header, presets, resizing, Tweak panels
2. Heat, Echo, Plate (upgrade existing engines)
3. Wider, Filter, Voice Morph
4. Crystal Echo, Space

Squeeze (compressor from 0.1.0) leaves the menu.
