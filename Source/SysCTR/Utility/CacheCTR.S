.arm

.global _InvalidateAndFlushCaches

_ClearCacheKernel:
	cpsid aif
	mov r0, #0
	mcr p15, 0, r0, c7, c10, 0	@ Clean entire data cache
	mcr p15, 0, r0, c7, c10, 5	@ Data Memory Barrier
	mcr p15, 0, r0, c7, c5, 0	@ Invalidate entire instruction cache / Flush BTB
	mcr p15, 0, r0, c7, c10, 4	@ Data Sync Barrier
	bx lr

_InvalidateAndFlushCaches:
	ldr r0, =_ClearCacheKernel
	svc 0x80					@ svcCustomBackdoor
	bx lr