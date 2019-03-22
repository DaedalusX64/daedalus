#include <pspmoduleexport.h>
#define NULL ((void *) 0)

extern int module_start;
extern int module_info;
static const unsigned int __syslib_exports[4] __attribute__((section(".rodata.sceResident"))) = {
	0xD632ACDB,
	0xF01D73A7,
	(unsigned int) &module_start,
	(unsigned int) &module_info,
};

extern int InitME;
extern int KillME;
static const unsigned int __MediaEngine_exports[4] __attribute__((section(".rodata.sceResident"))) = {
	0x284FAFFC,
	0xE6B12941,
	(unsigned int) &InitME,
	(unsigned int) &KillME,
};

const struct _PspLibraryEntry __library_exports[2] __attribute__((section(".lib.ent"), used)) = {
	{ NULL, 0x0000, 0x8000, 4, 1, 1, &__syslib_exports },
	{ "MediaEngine", 0x0000, 0x4001, 4, 0, 2, &__MediaEngine_exports },
};
