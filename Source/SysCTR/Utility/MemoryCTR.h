#ifndef __MEMORYCTR_H__
#define __MEMORYCTR_H__

#ifdef __cplusplus
extern "C" {
#endif

int hack3dsTestDynamicRecompilation(void);

int  _SetMemoryPermission(void *buffer, int size, int permission);
void _InvalidateInstructionCache(void);
void _FlushDataCache(void);
void _InvalidateAndFlushCaches(void);
int  _InitializeSvcHack(void);

#ifdef __cplusplus
}
#endif

#endif //__MEMORYCTR_H__