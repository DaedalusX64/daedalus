	.set noreorder

#include "pspstub.s"

	STUB_START "MediaEngine",0x40090000,0x00020005
	STUB_FUNC  0x284FAFFC,InitME
	STUB_FUNC  0xE6B12941,KillME
	STUB_END
