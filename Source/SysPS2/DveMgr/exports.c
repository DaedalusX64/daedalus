#include <pspmoduleexport.h>
#define NULL ((void *) 0)

extern int module_start;
extern int module_stop;
extern int module_info;
static const unsigned int __syslib_exports[6] __attribute__((section(".rodata.sceResident"))) = {
	0xD632ACDB,
	0xCEE8593C,
	0xF01D73A7,
	(unsigned int) &module_start,
	(unsigned int) &module_stop,
	(unsigned int) &module_info,
};

extern int pspDveMgrCheckVideoOut;
extern int pspDveMgrSetVideoOut;
static const unsigned int __pspDveManager_exports[4] __attribute__((section(".rodata.sceResident"))) = {
	0x2ACFCB6D,
	0xF9C86C73,
	(unsigned int) &pspDveMgrCheckVideoOut,
	(unsigned int) &pspDveMgrSetVideoOut,
};

extern int pspDveMgrCheckVideoOut;
extern int pspDveMgrSetVideoOut;
static const unsigned int __pspDveManager_driver_exports[4] __attribute__((section(".rodata.sceResident"))) = {
	0x2ACFCB6D,
	0xF9C86C73,
	(unsigned int) &pspDveMgrCheckVideoOut,
	(unsigned int) &pspDveMgrSetVideoOut,
};

const struct _PspLibraryEntry __library_exports[3] __attribute__((section(".lib.ent"), used)) = {
	{ NULL, 0x0000, 0x8000, 4, 1, 2, &__syslib_exports },
	{ "pspDveManager", 0x0000, 0x4001, 4, 0, 2, &__pspDveManager_exports },
	{ "pspDveManager_driver", 0x0000, 0x0001, 4, 0, 2, &__pspDveManager_driver_exports },
};
