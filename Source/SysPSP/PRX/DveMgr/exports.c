#include <pspmoduleexport.h>
#define NULL ((void *) 0)

void extern module_start;
void extern module_stop;
void extern module_info;
static const unsigned int __syslib_exports[6] __attribute__((section(".rodata.sceResident"))) = {
	0xD632ACDB,
	0xCEE8593C,
	0xF01D73A7,
	(unsigned int) &module_start,
	(unsigned int) &module_stop,
	(unsigned int) &module_info,
};

void extern pspDveMgrCheckVideoOut;
void extern pspDveMgrSetVideoOut;
static const unsigned int __pspDveManager_exports[4] __attribute__((section(".rodata.sceResident"))) = {
	0x2ACFCB6D,
	0xF9C86C73,
	(unsigned int) &pspDveMgrCheckVideoOut,
	(unsigned int) &pspDveMgrSetVideoOut,
};

void extern pspDveMgrCheckVideoOut;
void extern pspDveMgrSetVideoOut;
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
