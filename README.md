# ProTunez FX Toys

An effects rack plug-in for Apple Silicon Macs (AU + VST3 + standalone app), by The Revivalists Media.
Made for church service vocals, podcast voices and music production. For personal use.

## Effects (stage 1)

| Effect | What it does |
|---|---|
| **Heat** | Saturation and distortion: Warm Tube, Tape, Transistor, Diode, Fuzz |
| **Echo** | Delay: Digital, Tape, Analog, Lo-Fi styles; Single, Ping-Pong, Wide modes; tempo sync |
| **Plate** | Plate reverb with pre-delay, size, damping and movement |
| **Squeeze** | Compressor: Clean, Vintage, Smash characters, with parallel mix |

Effects run left to right; use the `<` `>` buttons to change their order. Each has an ON switch.
The header has the preset menu and Input, Output and Rack Mix knobs.

## Install

1. Download the latest `.pkg` from **Releases** (or from the newest run under **Actions**).
2. Double-click it. If macOS blocks it: right-click → **Open**.
3. In Logic Pro: **Audio FX → Audio Units → The Revivalists Media → ProTunez FX Toys**.

## Project layout

- `Source/DSP/` — the sound engines in plain C++ (no JUCE), one file per effect, plus `Rack.h`.
- `Source/PluginProcessor.*` — parameters, presets, saving/loading.
- `Source/PluginEditor.*` — the screen.
- `Tests/` — automated checks for the sound engines.
- `.github/workflows/build.yml` — the cloud build that makes the installer.

## Build it yourself

```bash
cmake -B build -G Xcode
cmake --build build --config Release
```
