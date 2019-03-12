	.set noreorder

#include "pspstub.s"

	STUB_START "pspDveManager_driver",0x00090000,0x00020005
	STUB_FUNC  0x2ACFCB6D,pspDveMgrCheckVideoOut
	STUB_FUNC  0xF9C86C73,pspDveMgrSetVideoOut
	STUB_END
