# ProTunez FX Toys

One effects plug-in for Apple Silicon Macs (AU + VST3 + standalone app), by The Revivalists Media.
Pick an effect from the **Effect** menu at the top and the whole window becomes that effect.
Made for church service vocals, podcast voices and music production. For personal use.

## Effects

| Effect | Like | Status |
|---|---|---|
| **Heat** | Decapitator | ✅ 0.3.0: new analog engine. Each style (A tape, E console, N preamp, T triode, P pentode) has its own circuit model (tape hysteresis, transformers, tube stages), run at 4x oversampling. Drive 0–10, Punish, Attitude meter, Low Cut + Thump, Tone, High Cut + Steep, Auto, Mix |
| **Echo** | EchoBoy | ✅ 0.3.0: 30 styles, each now with its own machine engine (tape with wow and flutter, BBD, lo-fi digital, radio, spring), Single/Dual/Ping-Pong/Rhythm, Tap tempo, Groove, Feel, Prime Numbers, Style Edit, Rhythm Editor, Duck, Freeze |
| **Plate** | SuperPlate | ✅ 0.3.0: new reverb tank (16-line feedback delay network with per-band decay) and plate dispersion; 5 plates, Tube/Solid/Clean preamps, decay to ∞, Pre-Delay, Modulation, filters, 2-band EQ, Width, Duck, Freeze |
| Voice Morph | Little AlterBoy | coming soon |
| Wider | MicroShift | coming soon |
| Filter | FilterFreak | coming soon |
| Crystal Echo | Crystallizer | coming soon |
| Space | SpaceBlender | coming soon |

Voice Morph, Wider, Filter, Crystal Echo and Space are still coming soon; they are not in 0.3.0.

Every effect has a **Tweak** button for its hidden controls. Shift-drag a knob for fine control, double-click to reset,
right-click for the exact value. The header has presets (Church Vocal, Podcast Voice, Choir, Music, Creative + your own),
Save, Revert, Undo, A/B compare, window Size and Bypass.

Your presets are saved in **Music → ProTunez FX Toys → Presets**, rhythm patterns in **Music → ProTunez FX Toys → Patterns**.

## Install

1. Download the latest `.pkg` from **Releases**.
2. Double-click it. If macOS blocks it: right-click → **Open**.
3. In Logic Pro: **Audio FX → Audio Units → The Revivalists Media → ProTunez FX Toys**.

## Project layout

- `Source/DSP/` — the sound engines in plain C++ (no JUCE), one file per effect.
- `Source/Params/` — each effect's parameter list.
- `Source/UI/` — the header, shared controls, and one screen per effect.
- `Source/PresetManager.*`, `Source/FactoryPresets.cpp` — presets.
- `Tests/` — automated checks (sound engines and presets).
- `tools/Screenshots.cpp` — renders every screen to PNG (Linux build only).
- `docs/feature-spec.md` — the full feature list; `docs/mockups/` — the screen designs.

## Build it yourself

```bash
cmake -B build -G Xcode
cmake --build build --config Release
```
