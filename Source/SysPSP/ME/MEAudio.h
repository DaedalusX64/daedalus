#pragma once

#include "Base/Types.h"

#ifdef DAEDALUS_PSP_ME_AUDIO
bool MEAudio_Process(bool asynchronous);
bool MEAudio_StartOutput();
void MEAudio_StopOutput();
bool MEAudio_IsOutputActive();
void MEAudio_QueueSamples(const void *samples, u32 count, u32 frequency);
void MEAudio_PublishWrites();
void MEAudio_DiscardWrites();
// ME interrupt entry calls this with all GPR/FPU/HI/LO state preserved.
extern "C" void MEAudio_Interrupt();
extern "C" void MEAudio_ExceptionEntry();
void MEAudio_InstallInterrupt();
#else
inline bool MEAudio_Process(bool) { return false; }
inline bool MEAudio_StartOutput() { return false; }
inline void MEAudio_StopOutput() {}
inline bool MEAudio_IsOutputActive() { return false; }
inline void MEAudio_QueueSamples(const void *, u32, u32) {}
#endif
