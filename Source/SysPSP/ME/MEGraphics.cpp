#include "SysPSP/ME/MEGraphics.h"
#include "SysPSP/ME/MEAudio.h"
#include <me-core-mapper/me-core.h>
#include <cstring>
#include <cstdio>
#include <pspkernel.h>
#include <pspgu.h>
#include "Graphics/GraphicsContext.h"

// libgu's active context must be GU_CALL while commands are written by ME.
extern int gu_curr_context;
// Prefix of PSPSDK's GuDisplayList (src/gu/guInternal.h). Only this cursor
// needs explicit ownership transfer; the command buffer itself is uncached.
struct GuDisplayList
{
    unsigned int *start;
    unsigned int *current;
    int parent_context;
};
static_assert(sizeof(GuDisplayList) == 12);
extern "C" GuDisplayList *gu_list;

namespace
{
enum State : u32 { Booting, Idle, Processing, Complete, Stop, Stopped, Fault };

struct alignas(64) Mailbox
{
    volatile u32 state;
    volatile u32 service_pending;
    volatile u32 dp_interrupt;
    u32 (*process)(u32);
    u32 argument;
    u32 result;
    uintptr_t gu_current;
    volatile u32 command_address;
    volatile u32 command;
    volatile u32 exception_cause;
    volatile u32 exception_epc;
    volatile u32 exception_address;
    void (*service)(void *);
    MEJobKind kind;
    alignas(64) unsigned char service_data[256];
};

Mailbox sMailbox{};

inline Mailbox *SharedMailbox()
{
    // Compute the address instead of loading a cached pointer during idle.
    return reinterpret_cast<Mailbox *>(reinterpret_cast<uintptr_t>(&sMailbox) | 0x40000000U);
}

// Give SC-only flags a complete cache line; ME graphics must never write back
// a neighbouring RDP global over a newer SC value.
struct alignas(64) MainState
{
    bool core_loaded = false;
    bool enabled = false;
    bool fallback_dp_interrupt = false;
    u32 task_count = 0;
    bool pending = false;
    u32 task_started = 0;
    void (*complete)(u32) = nullptr;
    MEJobKind kind = MEJobKind::Graphics;
#ifdef DAEDALUS_PSP_ME_TIMING
    u32 timed_tasks = 0;
    u32 task_us = 0;
    u32 service_us = 0;
    u32 service_count = 0;
    u32 blocked_us = 0;
    u32 audio_tasks = 0, audio_us = 0;
#endif
};
static_assert(sizeof(MainState) == 64);
MainState sMain;

inline void Barrier()
{
    asm volatile("sync" ::: "memory");
}

inline void RestoreMainGU()
{
    // SC uses libgu's uncached command path. ME flushed and invalidated the
    // complete list before publishing this cursor, including its last line.
    gu_list->start = reinterpret_cast<unsigned int *>(reinterpret_cast<uintptr_t>(gu_list->start) | 0x40000000U);
    gu_list->current = reinterpret_cast<unsigned int *>(SharedMailbox()->gu_current | 0x40000000U);
}

inline void UseCachedMEGU()
{
    // GU_CALL is not consumed by GE until an SC handoff. Cache both commands
    // and inline vertex data while ME owns it; start/current must use the
    // same alias so sceGuCheckList and allocation sizes remain correct.
    gu_list->start = reinterpret_cast<unsigned int *>(reinterpret_cast<uintptr_t>(gu_list->start) & ~0x40000000U);
    gu_list->current = reinterpret_cast<unsigned int *>(reinterpret_cast<uintptr_t>(gu_list->current) & ~0x40000000U);
}

void ServiceMainCPU()
{
#ifdef DAEDALUS_PSP_ME_TIMING
    const u32 service_started = sceKernelGetSystemTimeLow();
#endif
    Barrier();
    // The ME has flushed and relinquished the graphics state. The
    // main CPU may now allocate textures or submit a full GU list.
    sceKernelDcacheWritebackInvalidateAll();
    if (sMain.kind == MEJobKind::Graphics)
        RestoreMainGU();
    SharedMailbox()->service(SharedMailbox()->service_data);
    if (sMain.kind == MEJobKind::Graphics)
        SharedMailbox()->gu_current = reinterpret_cast<uintptr_t>(gu_list->current);
    sceKernelDcacheWritebackInvalidateAll();
    Barrier();
    SharedMailbox()->service_pending = 0;
#ifdef DAEDALUS_PSP_ME_TIMING
    if (sMain.kind == MEJobKind::Graphics)
    {
        sMain.service_us += sceKernelGetSystemTimeLow() - service_started;
        ++sMain.service_count;
    }
#endif
}

bool WaitForState(State wanted, u32 timeout_us = 0)
{
    const u32 started = sceKernelGetSystemTimeLow();
    u32 polls = 0;
    while (SharedMailbox()->state != wanted)
    {
        if (SharedMailbox()->state == Fault)
            return false;
        if (SharedMailbox()->service_pending)
        {
            ServiceMainCPU();
            polls = 0;
        }
        else if (++polls == 256)
        {
            // Briefly poll the uncached mailbox before yielding. Checking the
            // clock and sleeping on every iteration delays short ME services.
            polls = 0;
            sceKernelDelayThread(10);
        }
        else
            continue;
        if (timeout_us && sceKernelGetSystemTimeLow() - started >= timeout_us)
            return false;
    }
    Barrier();
    sceKernelDcacheWritebackInvalidateAll();
    return true;
}

int HoldMEInReset()
{
    // This accesses kernel-only hardware. Always enter through me-core's
    // kernel bridge rather than calling meLibReset from a user thread.
    HW_SYS_RESET_ENABLE = HW_SYS_RESET_ENABLE | SC_HW_RESET;
    Barrier();
    return 0;
}

int RestartME()
{
    meLibReset();
    return 0;
}

void DisableME(const char *reason)
{
    std::printf("ME graphics: %s, state=%lu cmd=%08lx/%08lx cause=%08lx epc=%08lx bad=%08lx\n",
        reason, SharedMailbox()->state, SharedMailbox()->command_address, SharedMailbox()->command,
        SharedMailbox()->exception_cause, SharedMailbox()->exception_epc, SharedMailbox()->exception_address);
    kcall(HoldMEInReset, 0);
    sceKernelDcacheWritebackInvalidateAll();
    if (sMain.enabled && gu_list)
    {
        // A fault can leave libgu's pointers in their ME aliases. Convert
        // the available cursor before SC discards the partial frame; never
        // dirty a cached command line beside subsequent uncached SC writes.
        gu_list->start = reinterpret_cast<unsigned int *>(reinterpret_cast<uintptr_t>(gu_list->start) | 0x40000000U);
        gu_list->current = reinterpret_cast<unsigned int *>(reinterpret_cast<uintptr_t>(gu_list->current) | 0x40000000U);
    }
    sMain.enabled = false;
}
}

ME_LIB_SETUP_DEFAULT_SUSPEND_HANDLER()

extern "C" __attribute__((noinline, aligned(4)))
void MEGraphics_HandleFault()
{
    u32 cause, epc, address;
    asm volatile("mfc0 %0, $13\n\tmfc0 %1, $14\n\tmfc0 %2, $8"
        : "=r"(cause), "=r"(epc), "=r"(address) :: "memory");
    SharedMailbox()->exception_cause = cause;
    SharedMailbox()->exception_epc = epc;
    SharedMailbox()->exception_address = address;
    meLibDcacheWritebackInvalidateAll();
    Barrier();
    SharedMailbox()->state = Fault;
    // Returning would retry the same instruction forever. SC will hold ME
    // in reset before touching renderer state or continuing emulation.
    for (;;)
        meLibHalt();
}

extern "C" __attribute__((noinline, aligned(4)))
void meLibOnException()
{
    // me-core's interrupt entry saves only k0/k1/ra/at. Inspect interrupt
    // causes using those scratch registers, without clobbering the interrupted
    // renderer's v0/v1/a0 registers through a normal C++ callback.
    asm volatile(
        ".set push\n\t.set noreorder\n\t"
        "mfc0 $k0, $13\n\tandi $k1, $k0, 0x7c\n\t"
        "beqz $k1, 1f\n\tnop\n\tj %0\n\tnop\n"
        "1:\n\t.set pop"
        :: "i"(MEGraphics_HandleFault) : "k0", "k1", "memory");
}

// me-core.h is included only in this translation unit; it installs this entry
// point at boot. GU start/finish, GE submission and OS services stay on the SC.
__attribute__((noinline, aligned(4)))
void meLibOnProcess()
{
    // Start scalar rendering with round-to-nearest and FPU traps disabled.
    asm volatile("ctc1 $zero, $31\n\tnop\n\tnop" ::: "memory");
    meLibExceptionHandlerInit(0);
#ifdef DAEDALUS_PSP_ME_AUDIO
    MEAudio_InstallInterrupt();
#endif
    meLibDcacheWritebackInvalidateAll();
    Barrier();
    SharedMailbox()->state = Idle;
    for (;;)
    {
        while (SharedMailbox()->state != Processing && SharedMailbox()->state != Stop)
            meLibDelayPipeline();

        Barrier();
        if (SharedMailbox()->state == Stop)
        {
            meLibDcacheWritebackInvalidateAll();
            Barrier();
#ifdef DAEDALUS_PSP_ME_AUDIO
            u32 status;
            asm volatile("mfc0 %0, $12" : "=r"(status));
            status &= ~0x8000U;
            asm volatile("mtc0 %0, $12\n\tsync" :: "r"(status) : "memory");
#endif
            SharedMailbox()->state = Stopped;
            for (;;)
                meLibHalt();
        }

        // Polling or interrupt handling can cache shared globals after the
        // previous flush. SC has changed them since then: invalidate again at
        // the handoff, before graphics writes can dirty a stale cache line.
        meLibDcacheWritebackInvalidateAll();
        Barrier();
        if (SharedMailbox()->kind == MEJobKind::Graphics)
            UseCachedMEGU();
        SharedMailbox()->result = SharedMailbox()->process(SharedMailbox()->argument);
        // Publish the SDK cursor separately. SC must append RET after the
        // final ME command/vertex allocation, never at a stale cached cursor.
        if (SharedMailbox()->kind == MEJobKind::Graphics)
            SharedMailbox()->gu_current = reinterpret_cast<uintptr_t>(gu_list->current);
        meLibDcacheWritebackInvalidateAll();
        Barrier();
        SharedMailbox()->state = Complete;
    }
}

bool MEGraphics_Initialise()
{
    if (sMain.enabled)
        return true;

    // These paths perform logging, debugger locking or profiling OS calls
    // inside individual commands. Keep them on the SC for interactive use.
#if defined(DAEDALUS_DEBUG_DISPLAYLIST) || defined(DAEDALUS_DEBUG_CONSOLE) || defined(DAEDALUS_ENABLE_ASSERTS) || defined(DAEDALUS_ENABLE_PROFILING) || defined(DAEDALUS_GPROF) || defined(DAEDALUS_LOG) || defined(DAEDALUS_BATCH_TEST_ENABLED) || defined(DAEDALUS_DEBUG_AUDIO) || defined(DEBUG_AUDIO)
    return false;
#endif

    sceKernelDcacheWritebackInvalidateAll();
    SharedMailbox()->state = Booting;
    SharedMailbox()->service_pending = 0;
    SharedMailbox()->dp_interrupt = 0;
    SharedMailbox()->exception_cause = 0;
    SharedMailbox()->exception_epc = 0;
    SharedMailbox()->exception_address = 0;
    sMain.task_count = 0;
    sMain.pending = false;
    sMain.complete = nullptr;
#ifdef DAEDALUS_PSP_ME_TIMING
    sMain.timed_tasks = sMain.task_us = sMain.service_us = sMain.service_count = sMain.blocked_us = 0;
    sMain.audio_tasks = sMain.audio_us = 0;
#endif
    std::printf("ME graphics: starting me-core\n");
    Barrier();
    if (!sMain.core_loaded)
    {
        const int result = meLibDefaultInit();
        if (result < 0)
        {
            std::printf("ME graphics: me-core error %d; using main CPU.\n", result);
            return false;
        }
        sMain.core_loaded = true;
        std::printf("ME graphics: core image %d loaded\n", result);
    }
    else
    {
        kcall(RestartME, 0);
    }
    if (!WaitForState(Idle, 1000000))
    {
        DisableME("startup failed; using main CPU");
        return false;
    }
    sMain.enabled = true;
    std::printf("ME graphics: ready\n");
    return true;
}

void MEGraphics_Shutdown()
{
    MEGraphics_Wait();
    MEAudio_StopOutput();
    if (!sMain.enabled)
        return;
    Barrier();
    SharedMailbox()->state = Stop;
    if (!WaitForState(Stopped, 1000000))
        DisableME("shutdown failed");
    sceGuSync(GU_SYNC_FINISH, GU_SYNC_WHAT_DONE);
    sMain.enabled = false;
}

bool MEGraphics_IsEnabled() { return sMain.enabled; }
bool MEGraphics_IsFirstTask()
{
#ifdef DAEDALUS_PSP_ME_DIAGNOSTICS
    return sMain.enabled && sMain.kind == MEJobKind::Graphics && sMain.task_count == 1;
#else
    return false;
#endif
}

void MEGraphics_Disable(const char *reason)
{
    DisableME(reason);
    // Also release a task wait if GPU submission failed inside an SC service.
    SharedMailbox()->state = Fault;
}

bool MEGraphics_IsPending() { return sMain.pending; }

__attribute__((noinline))
bool MEGraphics_Submit(u32 (*process)(u32), u32 argument, void (*complete)(u32), MEJobKind kind)
{
    MEGraphics_Wait();
    if (!sMain.enabled)
        return false;

    if (kind == MEJobKind::Graphics && gu_curr_context != GU_CALL)
    {
        DisableME("GU context is not GU_CALL; using main CPU");
        return false;
    }

    SharedMailbox()->kind = kind;
    sMain.kind = kind;
    SharedMailbox()->process = process;
    SharedMailbox()->argument = argument;
    SharedMailbox()->dp_interrupt = 0;
    SharedMailbox()->command_address = 0;
    SharedMailbox()->command = 0;
    if (kind == MEJobKind::Graphics)
        ++sMain.task_count;
    const bool trace = MEGraphics_IsFirstTask();
    if (trace)
        std::printf("ME graphics: first display list submitted\n");
    sMain.task_started = sceKernelGetSystemTimeLow();
    sMain.complete = complete;
    sMain.pending = true;
    sceKernelDcacheWritebackInvalidateAll();
    Barrier();
    SharedMailbox()->state = Processing;
    return true;
}

namespace
{
void CompleteTask(bool success)
{
    if (!success)
    {
        DisableME("display list failed; using main CPU");
        // A partially executed command cannot be replayed safely.
        if (sMain.kind == MEJobKind::Graphics)
        {
            GraphicsContextPSP_DiscardMEFrame();
            sMain.fallback_dp_interrupt = true;
        }
    }
    else if (sMain.kind == MEJobKind::Graphics)
        RestoreMainGU();
    const bool trace = MEGraphics_IsFirstTask();
    if (trace)
        std::printf("ME graphics: first display list completed, GU=%p cursor=%08lx\n",
            gu_list, static_cast<unsigned long>(SharedMailbox()->gu_current));
#ifdef DAEDALUS_PSP_ME_TIMING
    const u32 elapsed = sceKernelGetSystemTimeLow() - sMain.task_started;
    if (sMain.kind == MEJobKind::Audio)
    {
        sMain.audio_us += elapsed;
        if (++sMain.audio_tasks == 300)
        {
            std::printf("ME audio timing: avg task=%lu us/300 tasks\n",
                static_cast<unsigned long>(sMain.audio_us / sMain.audio_tasks));
            sMain.audio_us = sMain.audio_tasks = 0;
        }
    }
    else
    {
        sMain.task_us += elapsed;
        if (++sMain.timed_tasks == 300)
        {
            // Task time includes rendering, service work and mailbox scheduling.
            // Service time includes both SC cache handoffs and the callback.
            std::printf("ME graphics timing: avg task=%lu us, SC service=%lu us, calls=%lu/300 tasks, fence=%lu us\n",
                static_cast<unsigned long>(sMain.task_us / sMain.timed_tasks),
                static_cast<unsigned long>(sMain.service_us / sMain.timed_tasks),
                static_cast<unsigned long>(sMain.service_count),
                static_cast<unsigned long>(sMain.blocked_us / sMain.timed_tasks));
            sMain.timed_tasks = sMain.task_us = sMain.service_us = sMain.service_count = sMain.blocked_us = 0;
        }
    }
#endif
    if (trace)
        std::printf("ME graphics: GU cursor restored\n");
    const u32 result = success ? SharedMailbox()->result : 0;
    auto complete = sMain.complete;
    sMain.pending = false;
    sMain.complete = nullptr;
    // EndScene and emulated interrupts can now use renderer state on SC.
    if (complete)
        complete(result);
}
}

void MEGraphics_Poll()
{
    if (!sMain.pending)
        return;
    if (SharedMailbox()->service_pending)
        ServiceMainCPU();
    const u32 state = SharedMailbox()->state;
    if (state == Complete)
    {
        Barrier();
        sceKernelDcacheWritebackInvalidateAll();
        CompleteTask(true);
    }
    else if (state == Fault || sceKernelGetSystemTimeLow() - sMain.task_started >= 5000000)
        CompleteTask(false);
}

void MEGraphics_Wait()
{
    if (!sMain.pending)
        return;
#ifdef DAEDALUS_PSP_ME_TIMING
    const u32 started = sceKernelGetSystemTimeLow();
#endif
    const bool success = WaitForState(Complete, 5000000);
#ifdef DAEDALUS_PSP_ME_TIMING
    if (sMain.kind == MEJobKind::Graphics)
        sMain.blocked_us += sceKernelGetSystemTimeLow() - started;
#endif
    CompleteTask(success);
}

void MEGraphics_CallOnMainCPU(void (*call)(void *), void *data, size_t size)
{
    std::memcpy(SharedMailbox()->service_data, data, size);
    SharedMailbox()->service = call;
    if (SharedMailbox()->kind == MEJobKind::Graphics)
        SharedMailbox()->gu_current = reinterpret_cast<uintptr_t>(gu_list->current);
    meLibDcacheWritebackInvalidateAll();
    Barrier();
    SharedMailbox()->service_pending = 1;
    while (SharedMailbox()->service_pending)
        meLibDelayPipeline();
    // The SC service may have updated a line fetched while ME was waiting.
    meLibDcacheWritebackInvalidateAll();
    Barrier();
    if (SharedMailbox()->kind == MEJobKind::Graphics)
    {
        gu_list->current = reinterpret_cast<unsigned int *>(SharedMailbox()->gu_current);
        UseCachedMEGU();
    }
    std::memcpy(data, SharedMailbox()->service_data, size);
}

void MEGraphics_RequestDPInterrupt() { SharedMailbox()->dp_interrupt = 1; }

bool MEGraphics_TakeDPInterrupt()
{
    const bool fallback = sMain.fallback_dp_interrupt;
    sMain.fallback_dp_interrupt = false;
    if (!sMain.enabled)
        return fallback;
    bool pending = fallback || SharedMailbox()->dp_interrupt != 0;
    SharedMailbox()->dp_interrupt = 0;
    return pending;
}

void MEGraphics_ReportCommand(u32 address, u32 command)
{
    SharedMailbox()->command_address = address;
    SharedMailbox()->command = command;
}
