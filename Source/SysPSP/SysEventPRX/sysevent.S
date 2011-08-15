	.set noreorder

#include "pspstub.s"

	STUB_START "sysevent",0x40090000,0x00020005
	STUB_FUNC  0xA1110D68,kernel_sceKernelRegisterSysEventHandler
	STUB_FUNC  0xABE629DE,kernel_sceKernelUnregisterSysEventHandler
	STUB_END
