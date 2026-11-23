// Factory presets: five folders per effect. Values are in each control's real units.
#include "PresetManager.h"

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

const std::vector<FactoryPreset> echo = {
    { "Church Vocal", "Eighth Note Vocal", { { "echo_sync", 1 }, { "echo_div", 5 }, { "echo_fb", 25 }, { "echo_mix", 15 }, { "echo_highcut", 4500 } } },
    { "Church Vocal", "Worship Throw", { { "echo_sync", 1 }, { "echo_div", 8 }, { "echo_fb", 45 }, { "echo_mix", 22 }, { "echo_mode", 1 } } },
    { "Podcast Voice", "Subtle Room Slap", { { "echo_time", 90 }, { "echo_fb", 5 }, { "echo_mix", 8 }, { "echo_style", 0 } } },
    { "Podcast Voice", "Intro Stinger", { { "echo_sync", 1 }, { "echo_div", 9 }, { "echo_fb", 40 }, { "echo_mix", 30 } } },
    { "Choir", "Choir Halo", { { "echo_sync", 1 }, { "echo_div", 8 }, { "echo_fb", 35 }, { "echo_mix", 18 }, { "echo_mode", 2 } } },
    { "Choir", "Wide Choir Double", { { "echo_time", 28 }, { "echo_fb", 0 }, { "echo_mix", 30 }, { "echo_mode", 2 } } },
    { "Music", "Rock Slapback", { { "echo_time", 110 }, { "echo_fb", 10 }, { "echo_mix", 30 } } },
    { "Music", "Dotted Guitar", { { "echo_sync", 1 }, { "echo_div", 6 }, { "echo_fb", 40 }, { "echo_mix", 30 }, { "echo_mode", 1 } } },
    { "Creative", "Lo-Fi Tape Wash", { { "echo_style", 3 }, { "echo_fb", 60 }, { "echo_wobble", 70 }, { "echo_mix", 40 } } },
    { "Creative", "Runaway Dub", { { "echo_style", 2 }, { "echo_fb", 95 }, { "echo_sat", 70 }, { "echo_mix", 40 } } },
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
