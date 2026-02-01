#pragma once

#include <windows.h>
#include <cstdint>

static constexpr wchar_t kMapName[] = L"UnlockerIsland.SharedMem";
static constexpr uint32_t kMagic = 0x4B4C4E55; // "UNLK"

struct SharedData
{
    uint32_t magic;
    LONG seq;
    LONG alive;
    LONG unlock_enabled;
    int32_t target_fps;
    LONG hookfov_enabled;
    float target_fov;
    LONG hookDither_enabled;
};
