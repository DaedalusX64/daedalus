	.set noreorder

#include "pspstub.s"

	STUB_START "imposectrl",0x40090000,0x00010005
	STUB_FUNC  0x4D50C461,SetImposeHomeButton
	STUB_END
