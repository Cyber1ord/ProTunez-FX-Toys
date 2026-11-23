# ProTunez FX Toys

One effects plug-in for Apple Silicon Macs (AU + VST3 + standalone app), by The Revivalists Media.
Pick an effect from the **Effect** menu at the top and the whole window becomes that effect.
Made for church service vocals, podcast voices and music production. For personal use.

## Effects

| Effect | Like | Status |
|---|---|---|
| **Heat** | Decapitator | ✅ 0.2.0: styles A/E/N/T/P, Drive 0–10, Punish, Attitude meter, Low Cut + Thump, Tone, High Cut + Steep, Auto, Mix |
| **Echo** | EchoBoy | ✅ 0.2.0: 30 styles, Single/Dual/Ping-Pong/Rhythm, Tap tempo, Groove, Feel, Prime Numbers, Style Edit, Rhythm Editor, Duck, Freeze |
| **Plate** | SuperPlate | ✅ 0.2.0: 5 plates, Tube/Solid/Clean preamps, decay to ∞, Pre-Delay, Modulation, filters, 2-band EQ, Width, Duck, Freeze |
| Voice Morph | Little AlterBoy | coming next |
| Wider | MicroShift | coming next |
| Filter | FilterFreak | coming next |
| Crystal Echo | Crystallizer | later |
| Space | SpaceBlender | later |

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
