#ifndef PSPMODEL_H
#define PSPMODEL_H
#include "PSPModel.h"
#include <pspiofilemgr.h>
#include <stdio.h>
// This is a simple function to replace the kubridge and bring back earlier firmware support.

int PSPVramSize =  sceGeEdramGetSize() / 1024;
extern bool PSP_IS_SLIM;

int PSPDetect (int PSPModel)
{

		if (PSPVramSize >= 4096)
		{
			// This is a PSP Slim or above including Vita
			// We need to exclude the Vita from this

			PSP_IS_SLIM = true;
            PSPModel = 1; // Slim
	//		printf("PSPModel is %d",PSPModel);
		}
		//else if (PSPVramSize > 4096 && PSVitaDetect == 0)
		//{
		//	PSPModel=3; // Vita
		//}
		else
		{
PSP_IS_SLIM = false;
PSPModel= 0; // Fat
	//				printf("PSPModel is %d",PSPModel);
		}

	}

	#endif
