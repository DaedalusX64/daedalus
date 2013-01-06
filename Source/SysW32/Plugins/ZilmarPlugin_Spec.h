// Note from StrmnNrmn
// I create this so that the Graphics/Audio plugins share the PLUGIN_INFO
// declaration used here.

#ifndef _ZILMAR_H_INCLUDED__
#define _ZILMAR_H_INCLUDED__

#define EXPORT						__declspec(dllexport)
#define CALL						__cdecl

/***** Structures *****/
typedef struct {
	WORD Version;        /* Set to 0x0102 */
	WORD Type;           /* Set to PLUGIN_TYPE_GFX */
	char Name[100];      /* Name of the DLL */

	/* If DLL supports memory these memory options then set them to TRUE or FALSE
	   if it does not support it */
	BOOL NormalMemory;    /* a normal BYTE array */
	BOOL MemoryBswaped;  /* a normal BYTE array where the memory has been pre
	                          bswap on a dword (32 bits) boundry */
} PLUGIN_INFO;


#endif
