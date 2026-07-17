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
    { "IRON PULSE",      0.68f, 0.82f, 0.42f, 0.64f, -1.0f },
    { "GRIME ENGINE",    0.86f, 0.74f, 0.30f, 0.48f, -1.5f },
    { "CRUSHED KICK",    0.57f, 1.00f, 0.24f, 0.54f, -1.0f },
    { "WIDE RITUAL",     0.46f, 0.58f, 0.88f, 0.68f, -1.5f },
    { "BATTLE SMOKE",    0.79f, 0.90f, 0.36f, 0.60f, -1.0f },
    { "CONCRETE FANG",   0.72f, 0.94f, 0.18f, 0.72f, -2.0f },
    { "ASH PARADE",      0.63f, 0.63f, 0.78f, 0.55f, -1.5f },
    { "VILLAIN HALO",    0.81f, 0.67f, 0.72f, 0.43f, -1.5f },
    { "COLD SPIRAL",     0.52f, 0.76f, 0.92f, 0.70f, -2.0f },
    { "FINAL ROUND",     0.88f, 0.88f, 0.48f, 0.77f, -2.0f },
    { "NIGHT PRESS",     0.74f, 0.54f, 0.66f, 0.84f, -2.5f },
    { "RAW CEREMONY",    0.93f, 0.80f, 0.55f, 0.68f, -2.0f }
}};
}
