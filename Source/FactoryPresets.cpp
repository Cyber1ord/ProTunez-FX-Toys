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

const std::vector<FactoryPreset> plate = {
    { "Church Vocal", "Bright Vocal Plate", { { "plate_decay", 55 }, { "plate_pre", 30 }, { "plate_mix", 22 }, { "plate_lowcut", 250 } } },
    { "Church Vocal", "Sunday Hall Plate", { { "plate_decay", 75 }, { "plate_pre", 40 }, { "plate_mix", 28 }, { "plate_size", 90 } } },
    { "Podcast Voice", "Tiny Room Plate", { { "plate_decay", 15 }, { "plate_mix", 8 }, { "plate_lowcut", 300 } } },
    { "Podcast Voice", "Intro Sparkle", { { "plate_decay", 45 }, { "plate_mix", 20 }, { "plate_damp", 20 } } },
    { "Choir", "Choir Cathedral", { { "plate_decay", 85 }, { "plate_size", 100 }, { "plate_pre", 50 }, { "plate_mix", 30 } } },
    { "Choir", "Choir Blend", { { "plate_decay", 60 }, { "plate_mix", 20 }, { "plate_damp", 50 } } },
    { "Music", "Snare Plate", { { "plate_decay", 35 }, { "plate_pre", 10 }, { "plate_mix", 100 } } },
    { "Music", "Keys Plate", { { "plate_decay", 65 }, { "plate_mod", 50 }, { "plate_mix", 100 } } },
    { "Creative", "Endless Shimmer", { { "plate_decay", 100 }, { "plate_mod", 90 }, { "plate_mix", 100 } } },
    { "Creative", "Dark Abyss", { { "plate_decay", 95 }, { "plate_damp", 90 }, { "plate_size", 100 } } },
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
