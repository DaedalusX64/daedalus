/* 
 * File:   easymessage.cpp
 * Author: joris
 * 
 * Created on 18 augustus 2009, 19:39
 */
#include "stdafx.h"
#include "easymessage.h"

#include "Graphics/ColourValue.h"
#include "Utility/Preferences.h"

#include <pspdisplay.h>
#include <pspgu.h>
#include <pspkernel.h>
#include <string.h>

static u32 __attribute__((aligned(16))) list[262144];


EasyMessage::EasyMessage() 
{
    memset(&params, 0, sizeof (params));
    params.base.size = sizeof (params);
    sceUtilityGetSystemParamInt(PSP_SYSTEMPARAM_ID_INT_LANGUAGE, &params.base.language); // Prompt language
    sceUtilityGetSystemParamInt(PSP_SYSTEMPARAM_ID_INT_UNKNOWN, &params.base.buttonSwap); // X/O button swap
    params.base.graphicsThread = 0x11;
    params.base.accessThread = 0x13;
    params.base.fontThread = 0x12;
    params.base.soundThread = 0x10;
}

EasyMessage::~EasyMessage() {
}

u32 EasyMessage::ShowMessage(const char* message, bool yesno)
{
    params.mode = PSP_UTILITY_MSGDIALOG_MODE_TEXT;
    params.options = PSP_UTILITY_MSGDIALOG_OPTION_TEXT;

    if (yesno)
        params.options |= PSP_UTILITY_MSGDIALOG_OPTION_YESNO_BUTTONS | PSP_UTILITY_MSGDIALOG_OPTION_DEFAULT_NO;

    strcpy(params.message, message);

    _RunDialog();

    if (params.buttonPressed == PSP_UTILITY_MSGDIALOG_RESULT_YES) return 1;
    else return 0;
}

void EasyMessage::_RunDialog() 
{
	c32 colour = c32::Black; // default black

	// set wherever color the user has set for gui
	//
	switch( gGlobalPreferences.GuiColor )
	{
	case BLACK:		colour = c32::Black;		break;
	case RED:		colour = c32::Red;		break;
	case GREEN:		colour = c32::Green;		break;
	case MAGENTA:	colour = c32::Magenta;	break;
	case BLUE:		colour = c32::Blue;		break;
	case TURQUOISE:	colour = c32::Turquoise;		break;
	case ORANGE:	colour = c32::Orange;		break;
	case PURPLE:	colour = c32::Purple;		break;
	case GREY:		colour = c32::Grey;		break;
	}

	
    sceUtilityMsgDialogInitStart(&params);
    for (;;) 
	{
        sceGuStart(GU_DIRECT, list);
        sceGuClearColor(colour.GetColour());
        sceGuClearDepth(0);
        sceGuClear(GU_COLOR_BUFFER_BIT | GU_DEPTH_BUFFER_BIT | GU_FAST_CLEAR_BIT);
        sceGuFinish();
        sceGuSync(0, 0);
        switch (sceUtilityMsgDialogGetStatus())
		{

            case PSP_UTILITY_DIALOG_VISIBLE:
                sceUtilityMsgDialogUpdate(1);
                break;

            case PSP_UTILITY_DIALOG_QUIT:
                sceUtilityMsgDialogShutdownStart();
                break;

            case PSP_UTILITY_DIALOG_NONE:
                return;
        }
        sceDisplayWaitVblankStart();
        sceGuSwapBuffers();
    }
}

