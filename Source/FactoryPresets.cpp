// Factory presets: five folders per effect. Values are in each control's real units.
#include "PresetManager.h"
#include "DSP/Echo.h"

namespace ptfx
{
namespace
{
const std::vector<FactoryPreset> heat = {
    { "Church Vocal", "Warm Lead Vocal", { { "heat_style", 2 }, { "heat_drive", 3.0f }, { "heat_tone", 10 }, { "heat_mix", 60 }, { "heat_lowcut", 80 } } },
    { "Church Vocal", "Choir Leader Edge", { { "heat_style", 1 }, { "heat_drive", 4.0f }, { "heat_tone", 20 }, { "heat_mix", 45 } } },
    { "Podcast Voice", "Radio Warmth", { { "heat_style", 0 }, { "heat_drive", 3.5f }, { "heat_lowcut", 90 }, { "heat_mix", 50 } } },
    { "Podcast Voice", "Broadcast Presence", { { "heat_style", 2 }, { "heat_drive", 4.0f }, { "heat_tone", 25 }, { "heat_mix", 40 }, { "heat_highcut", 15000 } } },
    { "Choir", "Choir Glue", { { "heat_style", 0 }, { "heat_drive", 2.5f }, { "heat_mix", 35 } } },
    { "Choir", "Choir Air", { { "heat_style", 1 }, { "heat_drive", 2.0f }, { "heat_tone", 30 }, { "heat_mix", 30 } } },
    { "Music", "Drum Bus Crunch", { { "heat_style", 3 }, { "heat_drive", 6.0f }, { "heat_thump", 1 }, { "heat_lowcut", 60 }, { "heat_mix", 50 } } },
    { "Music", "Massive Bass", { { "heat_style", 3 }, { "heat_drive", 5.0f }, { "heat_thump", 1 }, { "heat_lowcut", 45 }, { "heat_highcut", 6000 }, { "heat_steep", 1 } } },
    { "Creative", "Telephone Fuzz", { { "heat_style", 4 }, { "heat_drive", 8.0f }, { "heat_lowcut", 500 }, { "heat_highcut", 3500 }, { "heat_steep", 1 } } },
    { "Creative", "Punished", { { "heat_style", 4 }, { "heat_drive", 7.0f }, { "heat_punish", 1 }, { "heat_tone", -20 } } },
};

/** An Echo preset that also carries the chosen style's Style Edit settings. */
FactoryPreset echoPreset (const char* cat, const char* name, const char* style, std::vector<std::pair<const char*, float>> v)
{
    const int idx = Echo::styleIndex (style);
    jassert (idx >= 0);
    const auto& st = Echo::styles()[(size_t) idx].edit;
    v.insert (v.begin(), {
        { "echo_style", (float) idx }, { "echo_s_bandlo", st.bandLowHz }, { "echo_s_bandhi", st.bandHighHz },
        { "echo_s_lowf", st.lowF }, { "echo_s_lowg", st.lowG }, { "echo_s_lowdec", st.lowDecay },
        { "echo_s_midf", st.midF }, { "echo_s_midg", st.midG }, { "echo_s_middec", st.midDecay }, { "echo_s_midres", st.midRes * 100.0f },
        { "echo_s_highf", st.highF }, { "echo_s_highg", st.highG }, { "echo_s_highdec", st.highDecay },
        { "echo_s_sat", (float) st.satType }, { "echo_s_diff", st.diffAmount * 100.0f }, { "echo_s_diffsize", st.diffSize * 100.0f },
        { "echo_s_diffloop", st.diffLoop ? 1.0f : 0.0f }, { "echo_s_wobdepth", st.wobDepth * 100.0f }, { "echo_s_wobrate", st.wobRate },
        { "echo_s_wobshape", (float) st.wobShape }, { "echo_s_wobsync", st.wobSync ? 1.0f : 0.0f } });
    return { cat, name, std::move (v) };
}

// echo_mode: 0 Single, 1 Dual, 2 Ping-Pong, 3 Rhythm · echo_timemode: 0 Time, 1 Note, 2 Dot, 3 Trip
// echo_note: 0 1/64 .. 3 1/8, 4 1/4, 5 1/2, 6 bar, 7 2 bars
const std::vector<FactoryPreset> echo = {
    echoPreset ("Church Vocal", "Eighth Note Vocal", "Studio Tape", { { "echo_timemode", 1 }, { "echo_note", 3 }, { "echo_fb", 25 }, { "echo_mix", 15 }, { "echo_highcut", 6000 }, { "echo_duck", 40 } }),
    echoPreset ("Church Vocal", "Worship Throw", "Master Tape", { { "echo_mode", 2 }, { "echo_timemode", 2 }, { "echo_note", 3 }, { "echo_fb", 45 }, { "echo_mix", 22 }, { "echo_duck", 60 } }),
    echoPreset ("Podcast Voice", "Subtle Room Slap", "Clean Digital", { { "echo_timemode", 0 }, { "echo_time", 90 }, { "echo_fb", 5 }, { "echo_mix", 8 }, { "echo_lowcut", 200 } }),
    echoPreset ("Podcast Voice", "Intro Stinger", "Tape Loop", { { "echo_timemode", 1 }, { "echo_note", 4 }, { "echo_fb", 40 }, { "echo_mix", 30 }, { "echo_duck", 50 } }),
    echoPreset ("Choir", "Choir Halo", "Ambient", { { "echo_mode", 1 }, { "echo_timemode", 1 }, { "echo_note", 4 }, { "echo_note2", 5 }, { "echo_fb", 35 }, { "echo_mix", 18 } }),
    echoPreset ("Choir", "Wide Choir Double", "Analog Chorus", { { "echo_timemode", 0 }, { "echo_time", 28 }, { "echo_fb", 0 }, { "echo_mix", 30 }, { "echo_lroffset", 14 } }),
    echoPreset ("Music", "Rock Slapback", "Studio Tape", { { "echo_timemode", 0 }, { "echo_time", 110 }, { "echo_fb", 10 }, { "echo_mix", 30 } }),
    echoPreset ("Music", "Swing Guitar", "Bucket Analog", { { "echo_mode", 2 }, { "echo_timemode", 1 }, { "echo_note", 3 }, { "echo_fb", 40 }, { "echo_groove", 60 }, { "echo_mix", 30 } }),
    echoPreset ("Creative", "Rhythm Cascade", "Digital Chorus", { { "echo_mode", 3 }, { "echo_rbeats", 2 }, { "echo_grid", 3 }, { "echo_repeats", 7 }, { "echo_shape", 2 }, { "echo_pan", 2 }, { "echo_fb", 30 }, { "echo_mix", 35 } }),
    echoPreset ("Creative", "Runaway Dub", "Space Tape", { { "echo_timemode", 2 }, { "echo_note", 3 }, { "echo_fb", 105 }, { "echo_sat", 60 }, { "echo_mix", 40 } }),
};

// plate_model: 0 Large Steel, 1 Compact Gold Foil, 2 Studio A, 3 Studio B, 4 Studio C · plate_preamp: 0 Tube, 1 Solid, 2 Clean
const std::vector<FactoryPreset> plate = {
    { "Church Vocal", "Bright Vocal Plate", { { "plate_model", 1 }, { "plate_decay", 2.2f }, { "plate_pre", 35 }, { "plate_mix", 22 }, { "plate_low", 250 }, { "plate_duck", 30 } } },
    { "Church Vocal", "Sunday Hall Plate", { { "plate_model", 3 }, { "plate_decay", 3.8f }, { "plate_pre", 45 }, { "plate_mix", 26 }, { "plate_low", 180 } } },
    { "Podcast Voice", "Tiny Room Plate", { { "plate_model", 1 }, { "plate_decay", 0.6f }, { "plate_mix", 8 }, { "plate_low", 300 }, { "plate_preamp", 2 } } },
    { "Podcast Voice", "Intro Sparkle", { { "plate_model", 1 }, { "plate_decay", 1.8f }, { "plate_mix", 20 }, { "plate_eq2f", 6000 }, { "plate_eq2g", 3 } } },
    { "Choir", "Choir Cathedral", { { "plate_model", 3 }, { "plate_decay", 6.0f }, { "plate_pre", 60 }, { "plate_mix", 30 }, { "plate_width", 140 } } },
    { "Choir", "Choir Blend", { { "plate_model", 0 }, { "plate_decay", 2.8f }, { "plate_mix", 20 }, { "plate_low", 200 } } },
    { "Music", "Classic Snare Plate", { { "plate_model", 0 }, { "plate_decay", 1.6f }, { "plate_pre", 10 }, { "plate_low", 150 } } },
    { "Music", "Dark Keys Plate", { { "plate_model", 4 }, { "plate_decay", 3.0f }, { "plate_mod", 50 } } },
    { "Creative", "Endless Plate", { { "plate_model", 3 }, { "plate_decay", 60.0f }, { "plate_mod", 70 }, { "plate_modrate", 0.4f } } },
    { "Creative", "Ducked Shimmer", { { "plate_model", 1 }, { "plate_decay", 8.0f }, { "plate_duck", 80 }, { "plate_eq2f", 8000 }, { "plate_eq2g", 6 } } },
};

const std::vector<FactoryPreset> none;
} // namespace

const std::vector<FactoryPreset>& factoryPresets (EffectId e)
{
    switch (e)
    {
        case EffectId::heat:  return heat;
        case EffectId::echo:  return echo;
        case EffectId::plate: return plate;
        case EffectId::morph: case EffectId::wider: case EffectId::filter: case EffectId::crystal: case EffectId::space:
        default:              return none;
    }
}
} // namespace ptfx
