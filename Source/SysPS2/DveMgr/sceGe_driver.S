	.set noreorder

#include "pspimport.s"

// Build List
// sceGe_driver_0000.o sceGe_driver_0001.o sceGe_driver_0002.o sceGe_driver_0003.o sceGe_driver_0004.o sceGe_driver_0005.o sceGe_driver_0006.o sceGe_driver_0007.o sceGe_driver_0008.o sceGe_driver_0009.o sceGe_driver_0010.o sceGe_driver_0011.o sceGe_driver_0012.o sceGe_driver_0013.o sceGe_driver_0014.o sceGe_driver_0015.o sceGe_driver_0016.o sceGe_driver_0017.o sceGe_driver_0018.o sceGe_driver_0019.o sceGe_driver_0020.o sceGe_driver_0021.o sceGe_driver_0022.o sceGe_driver_0023.o sceGe_driver_0024.o sceGe_driver_0025.o sceGe_driver_0026.o sceGe_driver_0027.o sceGe_driver_0028.o sceGe_driver_0029.o sceGe_driver_0030.o sceGe_driver_0031.o sceGe_driver_0032.o sceGe_driver_0033.o sceGe_driver_0034.o 

#ifdef F_sceGe_driver_0000
	IMPORT_START	"sceGe_driver",0x00010011
#endif
#ifdef F_sceGe_driver_0001
	IMPORT_FUNC	"sceGe_driver",0x03444EB4,sceGeListSync
#endif
#ifdef F_sceGe_driver_0002
	IMPORT_FUNC	"sceGe_driver",0x05DB22CE,sceGeUnsetCallback
#endif
#ifdef F_sceGe_driver_0003
	IMPORT_FUNC	"sceGe_driver",0x0BF608FB,sceGeRestoreContext
#endif
#ifdef F_sceGe_driver_0004
	IMPORT_FUNC	"sceGe_driver",0x114E1745,sceGe_driver_114E1745
#endif
#ifdef F_sceGe_driver_0005
	IMPORT_FUNC	"sceGe_driver",0x1C0D95A6,sceGeListEnQueueHead
#endif
#ifdef F_sceGe_driver_0006
	IMPORT_FUNC	"sceGe_driver",0x1F6752AD,sceGeEdramGetSize
#endif
#ifdef F_sceGe_driver_0007
	IMPORT_FUNC	"sceGe_driver",0x438A385A,sceGeSaveContext
#endif
#ifdef F_sceGe_driver_0008
	IMPORT_FUNC	"sceGe_driver",0x4C06E472,sceGeContinue
#endif
#ifdef F_sceGe_driver_0009
	IMPORT_FUNC	"sceGe_driver",0x51C8BB60,sceGeSetReg
#endif
#ifdef F_sceGe_driver_0010
	IMPORT_FUNC	"sceGe_driver",0x51D44C58,sceGeSetCmd
#endif
#ifdef F_sceGe_driver_0011
	IMPORT_FUNC	"sceGe_driver",0x57C8945B,sceGeGetMtx
#endif
#ifdef F_sceGe_driver_0012
	IMPORT_FUNC	"sceGe_driver",0x5A0103E6,sceGe_driver_5A0103E6
#endif
#ifdef F_sceGe_driver_0013
	IMPORT_FUNC	"sceGe_driver",0x5BAA5439,sceGeEdramSetSize
#endif
#ifdef F_sceGe_driver_0014
	IMPORT_FUNC	"sceGe_driver",0x5FB86AB0,sceGeListDeQueue
#endif
#ifdef F_sceGe_driver_0015
	IMPORT_FUNC	"sceGe_driver",0x67B01D8E,sceGeGetList
#endif
#ifdef F_sceGe_driver_0016
	IMPORT_FUNC	"sceGe_driver",0x71FCD1D6,sceGeInit
#endif
#ifdef F_sceGe_driver_0017
	IMPORT_FUNC	"sceGe_driver",0x7B481502,sceGe_driver_7B481502
#endif
#ifdef F_sceGe_driver_0018
	IMPORT_FUNC	"sceGe_driver",0x8F185DF7,sceGeEdramInit
#endif
#ifdef F_sceGe_driver_0019
	IMPORT_FUNC	"sceGe_driver",0x9ACFF59D,sceGeGetListIdList
#endif
#ifdef F_sceGe_driver_0020
	IMPORT_FUNC	"sceGe_driver",0x9DA4A75F,sceGe_driver_9DA4A75F
#endif
#ifdef F_sceGe_driver_0021
	IMPORT_FUNC	"sceGe_driver",0x9F2C2948,sceGeEnd
#endif
#ifdef F_sceGe_driver_0022
	IMPORT_FUNC	"sceGe_driver",0xA4FC06A4,sceGeSetCallback
#endif
#ifdef F_sceGe_driver_0023
	IMPORT_FUNC	"sceGe_driver",0xAB49E76A,sceGeListEnQueue
#endif
#ifdef F_sceGe_driver_0024
	IMPORT_FUNC	"sceGe_driver",0xAEC21518,sceGe_driver_AEC21518
#endif
#ifdef F_sceGe_driver_0025
	IMPORT_FUNC	"sceGe_driver",0xB287BD61,sceGeDrawSync
#endif
#ifdef F_sceGe_driver_0026
	IMPORT_FUNC	"sceGe_driver",0xB415364D,sceGeGetReg
#endif
#ifdef F_sceGe_driver_0027
	IMPORT_FUNC	"sceGe_driver",0xB448EC0D,sceGeBreak
#endif
#ifdef F_sceGe_driver_0028
	IMPORT_FUNC	"sceGe_driver",0xB77905EA,sceGeEdramSetAddrTranslation
#endif
#ifdef F_sceGe_driver_0029
	IMPORT_FUNC	"sceGe_driver",0xBAD6E1CA,sceGe_driver_BAD6E1CA
#endif
#ifdef F_sceGe_driver_0030
	IMPORT_FUNC	"sceGe_driver",0xC576E897,sceGe_driver_C576E897
#endif
#ifdef F_sceGe_driver_0031
	IMPORT_FUNC	"sceGe_driver",0xDC93CFEF,sceGeGetCmd
#endif
#ifdef F_sceGe_driver_0032
	IMPORT_FUNC	"sceGe_driver",0xE0D68148,sceGeListUpdateStallAddr
#endif
#ifdef F_sceGe_driver_0033
	IMPORT_FUNC	"sceGe_driver",0xE47E40E4,sceGeEdramGetAddr
#endif
#ifdef F_sceGe_driver_0034
	IMPORT_FUNC	"sceGe_driver",0xE66CB92E,sceGe_driver_E66CB92E
#endif
