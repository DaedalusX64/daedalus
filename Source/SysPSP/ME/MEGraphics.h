#pragma once

#include "Base/Types.h"
#include <cstddef>
#include <type_traits>

#ifdef DAEDALUS_PSP_USE_ME
enum class MEJobKind : u32 { Graphics, Audio };
bool MEGraphics_Initialise();
void MEGraphics_Shutdown();
void MEGraphics_Disable(const char *reason);
bool MEGraphics_IsEnabled();
bool MEGraphics_IsFirstTask();
// Submit returns without waiting. Completion is serviced on SC at emulated
// CPU event boundaries; fences drain before renderer/list memory is reused.
bool MEGraphics_Submit(u32 (*process)(u32), u32 argument, void (*complete)(u32), MEJobKind kind = MEJobKind::Graphics);
bool MEGraphics_IsPending();
void MEGraphics_Poll();
void MEGraphics_Wait();
void MEGraphics_RequestDPInterrupt();
bool MEGraphics_TakeDPInterrupt();
void MEGraphics_ReportCommand(u32 address, u32 command);

// me-core uses a stack in the ME's local eDRAM. Testing the stack avoids a
// privileged CP0 read on the main CPU and needs no shared cached flag.
inline bool MEGraphics_IsOnME()
{
    uintptr_t stack;
    asm volatile("move %0, $sp" : "=r"(stack));
    return (stack & 0x1fffffffU) < 0x00400000U;
}

void MEGraphics_CallOnMainCPU(void (*call)(void *), void *data, size_t size);

template<typename T>
void MEGraphics_CallOnMainCPU(void (*call)(void *), T &data)
{
    // Requests are copied out of local eDRAM into shared RAM. In particular,
    // callers must not pass pointers/references to objects on the ME stack.
    static_assert(std::is_trivially_copyable_v<T>);
    static_assert(sizeof(T) <= 256);
    MEGraphics_CallOnMainCPU(call, &data, sizeof(T));
}
#else
inline bool MEGraphics_Initialise() { return false; }
inline void MEGraphics_Shutdown() {}
inline void MEGraphics_Disable(const char *) {}
inline bool MEGraphics_IsEnabled() { return false; }
inline bool MEGraphics_IsFirstTask() { return false; }
inline bool MEGraphics_IsOnME() { return false; }
inline bool MEGraphics_IsPending() { return false; }
inline void MEGraphics_Poll() {}
inline void MEGraphics_Wait() {}
#endif
