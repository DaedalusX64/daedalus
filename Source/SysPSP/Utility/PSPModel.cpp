#ifndef PSPMODEL_H
#define PSPMODEL_H
#include "PSPModel.h"
#include <pspiofilemgr.h>
#include <stdio.h>
// This is a simple function to replace the kubridge and bring back earlier firmware support.

int PSPVramSize =  sceGeEdramGetSize() / 1024;
int PSVitaDetect = sceIoOpen("flash0:/kd/kermit_idstorage.prx", PSP_O_RDONLY | PSP_O_WRONLY, 0777);
extern bool PSP_IS_SLIM;

int PSPDetect (int PSPModel)
{

		if (PSPVramSize >= 4096)
		{
			// This is a PSP Slim or above including Vita
			// We need to exclude the Vita from this

			PSP_IS_SLIM = true;
            PSPModel = 1; // Slim

		}
		else if (PSPVramSize >= 4096 && PSVitaDetect == 0)
		{
			PSPModel=3; // Vita
			PSP_IS_SLIM = true;
    #undef DAEDALUS_PSP_USE_ME // Vita cannot use the MediaEngine. 
		}
		else
		{
PSP_IS_SLIM = false;
PSPModel= 0; // Fat
		}

	}

	#endif
