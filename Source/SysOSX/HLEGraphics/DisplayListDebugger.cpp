#include "stdafx.h"
#include "HLEGraphics/DisplayListDebugger.h"


#ifdef DAEDALUS_DEBUG_DISPLAYLIST
bool DLDebugger_IsDebugging()
{
	return false;
}

void DLDebugger_RequestDebug()
{
}

bool DLDebugger_Process()
{
	return false;
}
#endif
