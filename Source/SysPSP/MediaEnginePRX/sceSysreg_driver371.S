#	.set noreorder
#
# include "pspimport.s"
#
#	IMPORT_START   "sceSysreg_driver",0x00030000
#	IMPORT_FUNC    "sceSysreg_driver",0xA9997109,sceSysregMeResetEnable371
#	IMPORT_FUNC    "sceSysreg_driver",0x3199CF1C,sceSysregMeBusClockEnable371
#	IMPORT_FUNC    "sceSysreg_driver",0x76220E94,sceSysregMeResetDisable371
#	IMPORT_FUNC    "sceSysreg_driver",0x17A22D51,sceSysregVmeResetEnable371
#	IMPORT_FUNC    "sceSysreg_driver",0xE5B3D348,sceSysregAvcResetEnable371
#	IMPORT_FUNC    "sceSysreg_driver",0x07881A0B,sceSysregMeBusClockDisable371
	.set noreorder

#include "pspstub.s"

	STUB_START	 "sceSysreg_driver",0x00010011,0x00060005
	STUB_FUNC      0xA9997109,sceSysregMeResetEnable371
	STUB_FUNC      0x3199CF1C,sceSysregMeBusClockEnable371
	STUB_FUNC      0x76220E94,sceSysregMeResetDisable371
	STUB_FUNC      0x17A22D51,sceSysregVmeResetEnable371
	STUB_FUNC      0xE5B3D348,sceSysregAvcResetEnable371
	STUB_FUNC      0x07881A0B,sceSysregMeBusClockDisable371
	STUB_END