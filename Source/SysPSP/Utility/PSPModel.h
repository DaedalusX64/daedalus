#ifndef HEADER_FILE
#define HEADER_FILE


#include <pspiofilemgr.h>
#include <pspge.h>


//int PSVitaDetect = sceIoOpen("flash0:/kd/kermit_idstorage.prx", PSP_O_RDONLY | PSP_O_WRONLY, 0777);
int PSPDetect(int);

#endif
