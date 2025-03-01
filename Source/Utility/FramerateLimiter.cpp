#include "Base/Types.h"
#include "FramerateLimiter.h"

#include "System/Timing.h"


#include "Core/Memory.h"
#include "Core/ROM.h"

#include <chrono>
#include <thread>

static u32 gTicksBetweenVbls = 0;
static u32 gTicksPerSecond = 0;
static u64 gLastVITime = 0;
static u32 gLastOrigin = 0;
static u32 gVblsSinceFlip = 0;
static u32 gCurrentAverageTicksPerVbl = 0;
static FramerateSyncFn gAuxSyncFn = nullptr;
static void* gAuxSyncArg = nullptr;

constexpr std::array<u32, 3> gTvFrequencies = { 50, 60, 50 }; // PAL, NTSC, MPAL

void FramerateLimiter_SetAuxillarySyncFunction(FramerateSyncFn fn, void* arg)
{
    gAuxSyncFn = fn;
    gAuxSyncArg = arg;
}

bool FramerateLimiter_Reset()
{
    u64 frequency = 0;
    gLastVITime = 0;
    gLastOrigin = 0;
    gVblsSinceFlip = 0;

    if (NTiming::GetPreciseFrequency(&frequency))
    {
        gTicksBetweenVbls = static_cast<u32>(frequency / gTvFrequencies[g_ROM.TvType]);
        gTicksPerSecond = static_cast<u32>(frequency);
    }
    else
    {
        gTicksBetweenVbls = 0;
        gTicksPerSecond = 0;
    }
    return true;
}

static u32 FramerateLimiter_UpdateAverageTicksPerVbl(u32 elapsed_ticks)
{
    static std::array<u32, 4> s{};
    static u32 ptr = 0;

    s[ptr++ % s.size()] = elapsed_ticks;

    return (s[0] + s[1] + s[2] + s[3] + 2) >> 2; // Averaging 4 frames
}

void FramerateLimiter_Limit()
{
    gVblsSinceFlip++;

    u32 current_origin = Memory_VI_GetRegister(VI_ORIGIN_REG);
    if (gAuxSyncFn)
    {
        gAuxSyncFn(gAuxSyncArg);
    }

    if (current_origin == gLastOrigin)
        return;

    u64 now = 0;
    NTiming::GetPreciseTime(&now);

    u32 elapsed_ticks = static_cast<u32>(now - gLastVITime);
    gCurrentAverageTicksPerVbl = FramerateLimiter_UpdateAverageTicksPerVbl(elapsed_ticks / gVblsSinceFlip);

    if (gSpeedSyncEnabled && !gAuxSyncFn)
    {
        u32 required_ticks = gTicksBetweenVbls * gVblsSinceFlip;
        if (gSpeedSyncEnabled == 2)
            required_ticks *= 2; // Slow down to half speed

        s32 delay_ticks = required_ticks - elapsed_ticks - 50;
        if (delay_ticks > 0)
        {
            std::this_thread::sleep_for(std::chrono::nanoseconds(delay_ticks * (1'000'000'000 / gTicksPerSecond)));
            NTiming::GetPreciseTime(&now);
        }
    }

    gLastOrigin = current_origin;
    gLastVITime = now;
    gVblsSinceFlip = 0;
}

f32 FramerateLimiter_GetSync()
{
    return gCurrentAverageTicksPerVbl == 0 ? 0.0f : static_cast<f32>(gTicksBetweenVbls) / gCurrentAverageTicksPerVbl;
}

u32 FramerateLimiter_GetTvFrequencyHz()
{
    return gTvFrequencies[g_ROM.TvType];
}
