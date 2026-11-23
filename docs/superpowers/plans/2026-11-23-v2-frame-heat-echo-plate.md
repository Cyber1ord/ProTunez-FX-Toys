# ProTunez FX Toys v0.2 — App Frame, Heat, Echo, Plate — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace the 0.1 rack with a one-effect-at-a-time app whose header menu switches between full Heat, Echo and Plate screens (the other 5 listed as "coming"), with presets, Tweak panels and resizing.

**Architecture:** Each effect is a JUCE-free DSP class in `Source/DSP/` implementing `ptfx::Effect`, plus a view class in `Source/UI/`. The processor owns one instance of every built effect and runs only the selected one (choice parameter `effect`); every effect's parameters live in one APVTS with an id prefix so automation stays stable. The editor keeps a shared header and swaps the effect view below it.

**Tech Stack:** C++17, JUCE 8.0.4 (CMake FetchContent), Catch2 3.7.1, GitHub Actions macOS 15, arm64 only.

**Spec:** `docs/feature-spec.md` (mockups: `docs/mockups/`)

## Global Constraints

- Apple Silicon only (`CMAKE_OSX_ARCHITECTURES=arm64`), macOS 11+, AU + VST3 + Standalone.
- Screens use our own names and artwork with "like <original>" beside the title; never the originals' logos, names as titles, or artwork.
- Effect ids and menu order: heat, echo, plate, morph, wider, filter, crystal, space. Squeeze is removed.
- Plug-in codes stay `Rvmd` / `PzFx` (Logic keeps recognising the plug-in).
- DSP stays JUCE-free and unit-tested in `Tests/DSPTests.cpp`; every engine must stay finite and bounded at extreme settings.
- Version bump to 0.2.0 in `CMakeLists.txt` publishes the release automatically.

## Review Focus

1. Sample rates 44.1–192 kHz: delay buffers sized from the real rate; Echo at 2000 ms × 1.5 offset and Plate at 192 kHz must not read out of range.
2. No host tempo (standalone, or host gives none): synced times fall back to 120 BPM, never 0 or NaN.
3. Switching effect while audio plays: no click (20 ms crossfade), the new effect starts from a reset state, latency reported for the active effect.
4. Mono tracks: every effect runs on a mono in/out layout without crashing or silencing.
5. Infinite decay / Freeze / feedback 100% held for 60 s: output stays bounded (< +12 dBFS for a −6 dBFS input).

---

### Task 1: Effect registry and one-at-a-time processor

**Files:**
- Create: `Source/EffectIds.h`, `Source/Params/HeatParams.h`, `Source/Params/EchoParams.h`, `Source/Params/PlateParams.h`
- Modify: `Source/PluginProcessor.h/.cpp`, `Source/DSP/Common.h` (add `virtual float meter() const { return 0; }`)
- Delete: `Source/DSP/Rack.h`, `Source/DSP/Squeeze.h` and their tests
- Test: `Tests/DSPTests.cpp`

**Interfaces:**
- Produces: `enum class EffectId { heat, echo, plate, morph, wider, filter, crystal, space }`, `constexpr int kNumEffects = 8`, `bool isBuilt(EffectId)` (true for first three), `const char* effectName(EffectId)`, `const char* effectLike(EffectId)`.
- Produces: each `XParams.h` has `void addParams(juce::AudioProcessorValueTreeState::ParameterLayout&)` and `X::Params read(const juce::AudioProcessorValueTreeState&)`; ids prefixed `heat_`, `echo_`, `plate_`.
- Produces: processor `EffectId getActiveEffect() const`, parameter `effect` (AudioParameterChoice, built effects only selectable; unbuilt entries shown disabled in UI).

- [ ] Step 1: test `EffectSwitcher crossfades without a jump` — a pure helper `ptfx::Crossfader` (in Common.h, `void start(int samples)`, `float next()`) ramps 1→0 then 0→1; assert max step between consecutive gain values < 0.01 at 48 kHz over 20 ms.
- [ ] Step 2: run, expect fail (no Crossfader).
- [ ] Step 3: implement Crossfader; processor on effect change: fade out old (10 ms), reset new, fade in (10 ms); `setLatencySamples(active.latency())`.
- [ ] Step 4: run tests; build Linux VST3 target; expect pass / compile.
- [ ] Step 5: commit `feat: one-effect-at-a-time processor with effect menu parameter`.

### Task 2: Presets, A/B, revert, undo

**Files:**
- Create: `Source/PresetManager.h/.cpp`, `Source/FactoryPresets.cpp`, `Tests/PluginTests.cpp` (JUCE console test target `PluginTests`, added in `Tests/CMakeLists.txt` only when `PTFX_BUILD_PLUGIN`)

**Interfaces:**
- Produces: `class PresetManager { PresetManager(APVTS&, std::function<EffectId()>); juce::StringArray categories(EffectId); juce::StringArray presetsIn(EffectId, const juce::String& category); void load(EffectId, category, name); bool saveUser(EffectId, const juce::String& name); void next(); void previous(); void revert(); void storeAB(int slot /*0=A,1=B*/); void recallAB(int slot); juce::String currentName() const; }`
- User presets folder: `~/Music/ProTunez FX Toys/Presets/<Effect>/User/<name>.ptfx` (XML of that effect's params only).
- Factory categories per effect: `Church Vocal`, `Podcast Voice`, `Choir`, `Music`, `Creative` (≥2 presets each for heat/echo/plate).
- Undo: APVTS constructed with a `juce::UndoManager`; header Undo calls `undo()`.

- [ ] Step 1: tests in `PluginTests.cpp`: `load factory preset sets only that effect's params`; `saveUser then load round-trips values to 1e-4`; `revert restores values captured at last load`; `A/B recall swaps values`; `next() wraps from last to first`.
- [ ] Step 2: run, expect fail.
- [ ] Step 3: implement; loading touches only params with the active prefix.
- [ ] Step 4: run, expect pass.
- [ ] Step 5: commit `feat: preset manager with factory folders, user presets, A/B, revert`.

### Task 3: Editor frame

**Files:**
- Create: `Source/UI/LookAndFeel.h/.cpp` (from current editor), `Source/UI/Header.h/.cpp`, `Source/UI/EffectView.h` (base: `virtual void setTweakOpen(bool)`, `virtual juce::Colour accent()`), `Source/UI/ComingSoonView.h/.cpp`, `Source/UI/Widgets.h/.cpp` (Knob with label+value, LedReadout, SegmentButtons, VuMeter, LevelMeters)
- Modify: `Source/PluginEditor.h/.cpp` (header + current view; Tweak panel slides over bottom 40%)

**Interfaces:**
- Consumes: Task 1 ids, Task 2 PresetManager.
- Produces: `std::unique_ptr<EffectView> makeView(EffectId, ProTunezFXToysProcessor&)`.
- Window: base 1000×600, resizable 0.7×–1.6× keeping aspect (`setResizeLimits` + `getConstrainer()->setFixedAspectRatio`), size menu in settings (75/100/125/150 %) stored in state.
- Knobs: Shift-drag fine (`setVelocityModeParameters` / `juce::Slider::setMouseDragSensitivity` ×4 when shift), double-click default, right-click shows exact value popup.

- [ ] Step 1: build Standalone on Linux, run under Xvfb, screenshot each of the 8 menu entries.
- [ ] Step 2: compare against `docs/mockups/v3-*.png` header; unbuilt effects show the ComingSoon view with their name and "like …".
- [ ] Step 3: commit `feat: app frame with effect menu, header, resizing, tweak panel`.

### Task 4: Heat (like Decapitator)

**Files:** Modify `Source/DSP/Heat.h`, `Source/Params/HeatParams.h`; Create `Source/UI/HeatView.h/.cpp`; Test `Tests/DSPTests.cpp`

**Interfaces:** `Heat::Params { int style /*0..4 = A,E,N,T,P*/; float drive /*0..10*/; bool punish; bool autoGain; float outputDb /*-24..24*/; float mix; float lowCutHz /*20..1000*/; bool thump; float tone /*-1..1*/; float highCutHz /*1000..20000, >=19999 = off*/; bool steep; }`, `float meter() const` (attitude 0..1).

- [ ] Step 1: tests — `drive 0..10 maps to 0..+36 dB pre-gain`; `punish adds 20 dB (measure small-signal gain at drive 0, auto off)`; `steep: at 2 octaves above High Cut, attenuation >= 50 dB with steep, 9–15 dB without`; `thump: +2..+5 dB at low-cut frequency vs thump off`; `high cut off passes 15 kHz within 1 dB`; `each style differs (THD at drive 6 differs pairwise by > 5 %)`; keep existing latency/alignment/finite tests.
- [ ] Step 2: run, expect fail.
- [ ] Step 3: implement: 5 shapers (A tape: soft symmetric + slight HF loss; E console: asymmetric transformer-ish; N preamp: warm asymmetric with even harmonics; T triode: strong even; P pentode: hard odd); steep = 4th-order Linkwitz–Riley + one-pole (30 dB/oct); thump = +4 dB peak (Q 1) at low cut before shaper; attitude = smoothed peak of shaper input / 4.
- [ ] Step 4: HeatView per mockup v3-1 (Attitude VU, Punish, Drive 0–10, Output+Auto, Low Cut+Thump, Tone, High Cut+Steep, Mix, A–P).
- [ ] Step 5: run tests, screenshot, commit `feat: Heat full feature set and screen`.

### Task 5: Echo (like EchoBoy)

**Files:** Rewrite `Source/DSP/Echo.h` → `Source/DSP/Echo.h` + `Source/DSP/EchoStyles.h` (style table) + `Source/DSP/RhythmPattern.h`; `Source/Params/EchoParams.h`; `Source/UI/EchoView.h/.cpp`, `Source/UI/RhythmEditor.h/.cpp`, `Source/UI/StyleEditor.h/.cpp`

**Interfaces:**
- `Echo::Params { int mode /*single,dual,pingPong,rhythm*/; int timeMode /*ms,note,dot,trip,pattern*/; float timeMs /*1..2500*/; int noteIndex /*1/64..2 bars*/; float time2Ms; int note2Index /*dual*/; bool prime; float feedback /*0..1.1*/; float saturation; float lowCutHz; float highCutHz; float mix; float inDb; float outDb; float groove /*-1 shuffle..+1 swing*/; float feel /*-1 drag..+1 rush, ±25 ms*/; float accent, accent2, width, lrOffsetMs /*0..25*/, fbMix, balance, fbBalance; int rhythmLenBeats /*1..8*/; int gridIndex; float warp; int panShape /*0..7*/; int repeats /*1..16*/; int shape /*decay,reverse,swell,fade,nonlin*/; StyleEdit style; float duck /*0..1*/; bool freeze; }`
- `struct StyleEdit { float lowF, lowG, lowDecay, midF, midG, midDecay, midRes, highF, highG, highDecay; int satType /*0..8*/; float diffAmount, diffSize; bool diffLoop; float wobDepth, wobRate; int wobShape /*0..4*/; bool wobSync; }`
- `EchoStyles.h`: `struct StyleDef { const char* name; StyleEdit edit; float highCut; float lowCut; float noise; }` and `extern const StyleDef kEchoStyles[30]` — our own names (e.g. "Master Tape", "Studio Tape", "Tube Tape", "Cheap Tape", "Tape Loop", "Space Tape", "Bucket Analog", "Analog Bright", "Oil Can", "Transistor Box", "Telephone", "AM Radio", "FM Radio", "Shortwave", "Transmitter", "Clean Digital", "Cheap Digital", "Digital Chorus", "Analog Chorus", "Vintage Chorus", "Vibrato", "Saturated", "Fat", "Distressed", "Limited", "Distorted", "Squeaky", "Ambient", "Diffused", "Splattered"); choosing a style copies its StyleEdit into the editable params.
- `RhythmPattern { struct Tap { float pos /*beats*/; float level; float len; }; std::array<Tap,16> taps; int count; }` with `apply(shape, repeats)`.

- [ ] Step 1: tests — `prime nudges times to nearest prime sample count`; `note 1/4 at 120 BPM = 500 ms, dot = 750, trip = 333.3`; `no tempo → 120 BPM fallback`; `groove +1 shortens every second repeat by 1/3 of interval, -1 lengthens`; `feel +1 moves first echo 25 ms earlier`; `ping-pong alternates L/R`; `dual: two independent times measured on impulse`; `rhythm mode: 4 taps at 0.25/0.5/0.75/1 beat produce 4 impulses at those times`; `shape decay: tap levels strictly decreasing`; `style switch changes spectrum (Telephone has < -20 dB at 100 Hz vs Clean Digital)`; `duck 1.0: echo level −12 dB or more while input above −30 dBFS`; `freeze holds level within 3 dB for 10 s with input silent`; `feedback 1.1 + all styles bounded for 60 s`; `192 kHz 2500 ms ×1.5 offset finite`.
- [ ] Step 2: run, expect fail.
- [ ] Step 3: implement on top of current DelayLine/Hermite reader; diffusion = 4 allpasses (in loop or post); wobble shapes incl. random walk / S&H; sat types as 9 shapers; 3-band EQ per repeat inside the loop with decay scaling gain per pass.
- [ ] Step 4: EchoView per mockup v3-2 plus Tweak panel (per-mode controls), Style Edit panel, Rhythm Editor (click add, Alt-click delete, drag move/resize, Cmd off-grid, save/load patterns).
- [ ] Step 5: run tests, screenshots, commit `feat: Echo full feature set, style editor, rhythm editor`.

### Task 6: Plate (like SuperPlate)

**Files:** Modify `Source/DSP/Plate.h` (+ `Source/DSP/PlateModels.h`); `Source/Params/PlateParams.h`; `Source/UI/PlateView.h/.cpp`, `Source/UI/EqGraph.h/.cpp`

**Interfaces:** `Plate::Params { int model /*0..4*/; int preamp /*tube,solid,clean*/; float decaySec /*0.5..60, >=60 = infinite*/; float preDelayMs /*0..250*/; float modDepth, modRateHz /*0.1..5*/; float lowHz, highHz; int lowSlope /*6,12,24*/; float eq1F, eq1G, eq1Q, eq2F, eq2G, eq2Q; float width /*0..2*/; float balance /*-1..1*/; float inDb, outDb, mix; float duck; bool freeze; }`. `PlateModels.h`: 5 `ModelDef { float sizeScale; float dampHz; float bandwidth; float diffusion[4]; float tilt; }` named "Large Steel", "Compact Gold Foil", "Studio Plate A", "Studio Plate B", "Studio Plate C".

- [ ] Step 1: tests — `decay 2 s gives measured RT60 within ±20 %` (energy decay of impulse at 500 Hz band); `decay infinite holds energy within 3 dB over 10 s`; `switching model keeps decay setting`; `preamp clean is linear (THD < 0.1 % at −6 dBFS)`, `tube adds THD > 0.5 % at −6 dBFS`; `pre-delay 100 ms: no output before 100 ms`; `width 0 = mono (L==R)`; `duck: tail −10 dB while input loud`; existing stability tests at 192 kHz.
- [ ] Step 2: run, expect fail.
- [ ] Step 3: implement: decay → tank gain `g = 10^(-3·loopSec / decaySec)` with loopSec from current delay lengths; infinite = g 1.0 with damping off and soft limiter in loop.
- [ ] Step 4: PlateView per mockup v3-3, Tweak panel with EqGraph (drag points, Alt-scroll Q).
- [ ] Step 5: run tests, screenshots, commit `feat: Plate full feature set and screen`.

### Task 7: Release 0.2.0

- [ ] Step 1: `project(... VERSION 0.2.0)`; update README (effects table, "coming next").
- [ ] Step 2: all tests pass locally; push; CI green (auval passes, arm64 only).
- [ ] Step 3: confirm `v0.2.0` release has `ProTunez-FX-Toys-0.2.0.pkg`; send screenshots and link.
