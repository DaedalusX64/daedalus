#ifndef PSPMODEL_H
#define PSPMODEL_H
#include "PSPModel.h"
#include <pspiofilemgr.h>

// This is a simple function to replace the kubridge and bring back earlier firmware support.

int PSPVramSize =  sceGeEdramGetSize() / 1024;


int PSPDetect (int PSPModel)
{

		if (PSPVramSize > 4096)
		{
			PSPModel=1; // Slim+
		}
		//else if (PSPVramSize > 4096 && PSVitaDetect == 0)
		//{
		//	PSPModel=3; // Vita
		//}
		else if (PSPVramSize < 4000)
		{
			PSPModel=0; // Fat
		}

	}

	#endif
