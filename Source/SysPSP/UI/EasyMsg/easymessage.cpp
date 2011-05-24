/* 
 * File:   easymessage.cpp
 * Author: joris
 * 
 * Created on 18 augustus 2009, 19:39
 */

// Heavy modified by Salvy6735 (2011)

#include "stdafx.h"
#include "easymessage.h"

#include "Graphics/ColourValue.h"
#include "Utility/Preferences.h"

#include <psputility.h>
#include <pspdisplay.h>
#include <pspgu.h>
#include <pspkernel.h>
#include <string.h>

static u32 __attribute__((aligned(16))) list[262144];
//*************************************************************************************
//
//*************************************************************************************
static void ConfigureDialog(pspUtilityMsgDialogParams *dialog, size_t dialog_size)
{
	memset(dialog, 0, dialog_size);

	dialog->base.size			= dialog_size;
	dialog->base.language		= PSP_SYSTEMPARAM_LANGUAGE_ENGLISH;
	dialog->base.buttonSwap		= PSP_UTILITY_ACCEPT_CROSS;

	dialog->base.graphicsThread = 0x11;
	dialog->base.accessThread	= 0x13;
	dialog->base.fontThread		= 0x12;
	dialog->base.soundThread	= 0x10; 
}

//*************************************************************************************
//
//*************************************************************************************
static void RunDialog() 
{
	bool bfinish = false;

	while(!bfinish)
	{
		sceGuStart(GU_DIRECT, list);
		sceGuClearColor(0);
		sceGuClearDepth(0);
		sceGuClear(GU_COLOR_BUFFER_BIT|GU_DEPTH_BUFFER_BIT);
		sceGuFinish();
		sceGuSync(0,0);

		switch(sceUtilityMsgDialogGetStatus())
		{
			case PSP_UTILITY_DIALOG_INIT:
				break;
			
			case PSP_UTILITY_DIALOG_VISIBLE:
				 sceUtilityMsgDialogUpdate(1);
				break;
			
			case PSP_UTILITY_DIALOG_QUIT:
				sceUtilityMsgDialogShutdownStart();
				break;
			
			case PSP_UTILITY_DIALOG_FINISHED:
				bfinish = true;
				break;
				
			case PSP_UTILITY_DIALOG_NONE:
				bfinish = true;
				break;
				
			default :
				break;
		}
		sceDisplayWaitVblankStart();
		sceGuSwapBuffers();
	}

	bfinish = false;
}

//*************************************************************************************
//
//*************************************************************************************
bool ShowMessage(const char* message, bool yesno)
{
	pspUtilityMsgDialogParams params; 

    ConfigureDialog(&params, sizeof(params)); 

    params.mode	   = PSP_UTILITY_MSGDIALOG_MODE_TEXT;
    params.options = PSP_UTILITY_MSGDIALOG_OPTION_TEXT;

    if (yesno)
        params.options |= PSP_UTILITY_MSGDIALOG_OPTION_YESNO_BUTTONS | PSP_UTILITY_MSGDIALOG_OPTION_DEFAULT_NO;

    strcpy(params.message, message);

	sceUtilityMsgDialogInitStart(&params);

    RunDialog();

    if (params.buttonPressed == PSP_UTILITY_MSGDIALOG_RESULT_YES) 
		return true;

    return false;
}