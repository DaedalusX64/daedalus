#pragma once

#include "Base/Types.h"
#include "Core/Memory.h"
#include <cstring>

// Audio DSP work may run concurrently with the emulated CPU. Its writes are
// staged on ME and published on SC at SP completion, rather than writing back
// entire RDRAM cache lines over unrelated CPU changes.
#ifdef DAEDALUS_PSP_ME_AUDIO
void Audio_ReadMemory(u32 address, void *dest, u32 size);
void Audio_WriteMemory(u32 address, const void *source, u32 size);
#else
inline void Audio_ReadMemory(u32 address, void *dest, u32 size)
{
    std::memmove(dest, g_pu8RamBase + address, size);
}
inline void Audio_WriteMemory(u32 address, const void *source, u32 size)
{
    std::memmove(g_pu8RamBase + address, source, size);
}
#endif

inline u16 Audio_ReadHalf(u32 address)
{
    u16 value;
    Audio_ReadMemory(address, &value, sizeof(value));
    return value;
}
inline void Audio_WriteHalf(u32 address, u16 value)
{
    Audio_WriteMemory(address, &value, sizeof(value));
}
