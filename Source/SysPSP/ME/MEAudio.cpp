#include "SysPSP/ME/MEAudio.h"
#include "SysPSP/ME/MEGraphics.h"
#include "SysPSP/ME/AudioMemoryJournal.h"
#include "SysPSP/ME/AudioPacketStream.h"
#include "HLEAudio/AudioMemory.h"
#include "HLEAudio/HLEAudioInternal.h"
#include "Core/CPU.h"
#include "Core/RSP_HLE.h"
#include "Ultra/ultra_rcp.h"
#include <me-core-mapper/me-lib.h>
#include <pspkernel.h>
#include <cstdio>

namespace
{
constexpr u32 OutputFrequency = 44100;
constexpr u32 PacketFrames = 512;
using Stream = AudioPacketStream<4, PacketFrames>;
alignas(64) Stream sStream{};
struct alignas(64) OutputControl
{
    volatile u32 requested = 0, acknowledged = 0;
    volatile u32 underflows = 0, dropped = 0;
};
OutputControl sOutput;
struct alignas(64) Cursor { u32 phase = 0; bool running = false; };
Cursor sCursor; // ME-only; never shares a cache line with SC state.
struct alignas(64) KernelState { bool active = false; u32 gates = 0, clock = 0; };
KernelState sKernel;
alignas(64) AudioMemoryJournal<1024, 128 * 1024> sWrites;

inline Stream *SharedStream()
{
    return reinterpret_cast<Stream *>(reinterpret_cast<uintptr_t>(&sStream) | 0x40000000U);
}
inline OutputControl *Control()
{
    return reinterpret_cast<OutputControl *>(reinterpret_cast<uintptr_t>(&sOutput) | 0x40000000U);
}
inline void Sync() { asm volatile("sync" ::: "memory"); }

// Register setup adapted from mcidclan's MIT-licensed audio-local sample.
// See audio-local-LICENSE.txt for its copyright and permission notice.
int EnableClock()
{
    sKernel.gates = hw(0xBC100058);
    sKernel.clock = hw(0xBC10006C);
    hw(0xBC100058) |= 0x01010000;
    hw(0xBC10006C) = 1;
    Sync();
    return 0;
}
int RestoreClock()
{
    hw(0xBC10006C) = sKernel.clock;
    hw(0xBC100058) = (hw(0xBC100058) & ~0x01010000U) | (sKernel.gates & 0x01010000U);
    Sync();
    return 0;
}
bool WaitForOutput(u32 command)
{
    const u32 start = sceKernelGetSystemTimeLow();
    while (Control()->acknowledged != command)
    {
        MEGraphics_Poll();
        if (!MEGraphics_IsEnabled() || sceKernelGetSystemTimeLow() - start >= 250000)
            return false;
        sceKernelDelayThread(10);
    }
    return true;
}
u32 ProcessAudio(u32)
{
    sWrites.Reset();
    Audio_Ucode();
    return 1;
}
void CompleteAudio(u32 result)
{
    if (result)
        MEAudio_PublishWrites();
    else
        MEAudio_DiscardWrites();
}
void CompleteAudioAsync(u32 result)
{
    CompleteAudio(result);
    RSP_HLE_Finished(SP_STATUS_TASKDONE | SP_STATUS_BROKE | SP_STATUS_HALT);
}
}

extern "C" {
alignas(64) u32 MEAudio_PreviousExceptionVector[16]{};
}

void MEAudio_InstallInterrupt()
{
    u32 previous, status;
    asm volatile("mfc0 %0, $25\n\tmfc0 %1, $12" : "=r"(previous), "=r"(status));
    MEAudio_PreviousExceptionVector[0] = previous;
    const u32 entry = reinterpret_cast<uintptr_t>(MEAudio_ExceptionEntry) | 0x80000000U;
    const u32 period = 16650; // me-core's 166500-tick timer interval / 10.
    status |= 0x8001;
    asm volatile("mtc0 %0, $25\n\tmtc0 $zero, $9\n\tmtc0 %1, $11\n\tmtc0 %2, $12\n\tsync"
        :: "r"(entry), "r"(period), "r"(status) : "memory");
}

extern "C" void MEAudio_Interrupt()
{
    const u32 command = Control()->requested;
    const bool wanted = (command & 1) != 0;
    if (wanted != sCursor.running)
    {
        if (wanted)
        {
            hw(0xBC100050) |= 0x300;
            hw(0xBE000000) = 1;
            Sync();
            hw(0xBE000050) = 0x100;
            hw(0xBE000004) = 2;
            hw(0xBE000010) = hw(0xBE000004);
        }
        else
        {
            hw(0xBE000004) = 0;
            hw(0xBE000010) = 0;
        }
        Sync();
        sCursor.running = wanted;
        sCursor.phase = 0;
    }
    // Acknowledge each request generation, including off->off cancellations.
    // SC cannot mistake an old 'off' reply for completion of a late enable.
    Control()->acknowledged = command;
    if (!wanted)
        return;
    // Bound interrupt work even if a bad hardware status never clears.
    for (u32 i = 0; i < 256 && (hw(0xBE000028) & 0x20); ++i)
    {
        u32 sample;
        if (!SharedStream()->Next(sCursor.phase, sample))
            Control()->underflows = Control()->underflows + 1;
        hw(0xBE000070) = sample;
        Sync();
    }
}

bool MEAudio_StartOutput()
{
    if (sKernel.active)
        return MEGraphics_IsEnabled();
    if (!MEGraphics_IsEnabled())
        return false;
    SharedStream()->Reset();
    Control()->underflows = 0;
    Control()->dropped = 0;
    kcall(EnableClock, 0);
    Sync();
    const u32 command = (Control()->requested + 2) | 1U;
    Control()->requested = command;
    if (!WaitForOutput(command))
    {
        const u32 cancelled = (command + 2) & ~1U;
        Control()->requested = cancelled;
        // Cancel an enable that arrived just as its timeout expired.
        WaitForOutput(cancelled);
        kcall(RestoreClock, 0);
        std::printf("ME audio: FIFO startup failed; using CPU playback\n");
        return false;
    }
    sKernel.active = true;
    std::printf("ME audio: direct FIFO output ready\n");
    return true;
}

void MEAudio_StopOutput()
{
    if (!sKernel.active)
        return;
    const u32 command = (Control()->requested + 2) & ~1U;
    Control()->requested = command;
    Sync();
    WaitForOutput(command);
    kcall(RestoreClock, 0);
    sKernel.active = false;
}
bool MEAudio_IsOutputActive() { return sKernel.active; }

void MEAudio_QueueSamples(const void *samples, u32 count, u32 frequency)
{
    if (!frequency || frequency > 192000)
        return;
    const auto *source = static_cast<const u8 *>(samples);
    while (count)
    {
        const u32 chunk = std::min(count, PacketFrames);
        const u32 start = sceKernelGetSystemTimeLow();
        while (!SharedStream()->Push(source, chunk, frequency, OutputFrequency))
        {
            MEGraphics_Poll();
            if (!MEGraphics_IsEnabled() || sceKernelGetSystemTimeLow() - start >= 100000)
            {
                Control()->dropped += count;
                return;
            }
            sceKernelDelayThread(100);
        }
        source += chunk * sizeof(u32);
        count -= chunk;
    }
}

bool MEAudio_Process(bool asynchronous)
{
    if (!MEGraphics_Submit(ProcessAudio, 0, asynchronous ? CompleteAudioAsync : CompleteAudio, MEJobKind::Audio))
        return false;
    if (asynchronous)
        CPU_RequestMEGraphicsPoll();
    else
        MEGraphics_Wait();
    return true;
}
void MEAudio_PublishWrites()
{
    sWrites.Commit(g_pu8RamBase);
    sWrites.Reset();
}
void MEAudio_DiscardWrites() { sWrites.Reset(); }

void Audio_ReadMemory(u32 address, void *dest, u32 size)
{
    if (address > gRamSize || size > gRamSize - address)
    {
        std::memset(dest, 0, size);
        return;
    }
    if (MEGraphics_IsOnME())
        sWrites.Read(g_pu8RamBase, address, dest, size);
    else
        std::memmove(dest, g_pu8RamBase + address, size);
}
void Audio_WriteMemory(u32 address, const void *source, u32 size)
{
    if (address > gRamSize || size > gRamSize - address)
        return;
    if (!MEGraphics_IsOnME())
    {
        std::memmove(g_pu8RamBase + address, source, size);
        return;
    }
    // Overflow is rare; publish a full transaction before reusing its space.
    // Audio DSP normally reaches SP completion without an SC service call.
    const auto *bytes = static_cast<const u8 *>(source);
    while (size)
    {
        const u32 chunk = std::min<u32>(size, 128 * 1024);
        if (!sWrites.Write(address, bytes, chunk))
        {
            u32 request = 0;
            MEGraphics_CallOnMainCPU([](void *) { MEAudio_PublishWrites(); }, request);
            continue;
        }
        size -= chunk;
        address += chunk;
        bytes += chunk;
    }
}
