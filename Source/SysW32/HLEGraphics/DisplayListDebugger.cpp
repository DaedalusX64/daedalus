#include "stdafx.h"
#include "HLEGraphics/DisplayListDebugger.h"

#include "Graphics/GraphicsContext.h"
#include "Graphics/PngUtil.h"

#include "HLEGraphics/DLDebug.h"
#include "HLEGraphics/DLParser.h"


#ifdef DAEDALUS_DEBUG_DISPLAYLIST

static bool gDebugging = false;


bool DLDebugger_IsDebugging()
{
	return gDebugging;
}

void DLDebugger_RequestDebug()
{
	gDebugging = true;
}

bool DLDebugger_Process()
{
	//Fix ME W32
	return false;
}

#endif	//DAEDALUS_DEBUG_DISPLAYLIST
