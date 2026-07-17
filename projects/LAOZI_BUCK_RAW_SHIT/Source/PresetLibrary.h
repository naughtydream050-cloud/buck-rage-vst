#pragma once

#include <array>

struct LaoziPreset
{
    const char* name;
    float pressure;
    float kick;
    float aura;
    float glue;
    float outputDb;
};

namespace LaoziPresetLibrary
{
inline constexpr std::array<LaoziPreset, 12> presets {{
    { "IRON PULSE",      0.72f, 0.96f, 0.12f, 0.68f, -2.0f },
    { "GRIME ENGINE",    0.95f, 0.85f, 0.08f, 0.56f, -2.0f },
    { "CRUSHED KICK",    0.62f, 1.00f, 0.10f, 0.63f, -1.5f },
    { "WIDE RITUAL",     0.42f, 0.45f, 0.98f, 0.38f, -1.0f },
    { "BATTLE SMOKE",    0.58f, 0.70f, 0.10f, 0.42f, -1.0f },
    { "CONCRETE FANG",   0.66f, 0.62f, 0.08f, 0.50f, -1.5f },
    { "ASH PARADE",      0.54f, 0.58f, 0.90f, 0.45f, -1.5f },
    { "VILLAIN HALO",    0.66f, 0.55f, 0.86f, 0.40f, -1.5f },
    { "COLD SPIRAL",     0.45f, 0.88f, 0.66f, 0.28f, -1.0f },
    { "FINAL ROUND",     0.70f, 0.68f, 0.16f, 0.52f, -1.5f },
    { "NIGHT PRESS",     0.55f, 0.78f, 0.72f, 0.34f, -1.5f },
    { "RAW CEREMONY",    0.68f, 0.86f, 0.58f, 0.42f, -1.5f }
}};
}
