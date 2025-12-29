#ifndef MAIN_H
#define MAIN_H

#include <kos.h>
#include "windef.h"

#ifndef SAFE_DELETE
#define SAFE_DELETE(p)  { if(p) { delete (p);     (p)=NULL; } }
#endif

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p)  { if(p) { (p)->Release();     (p)=NULL; } }
#endif

#ifndef SAFE_DELETEARRAY
#define SAFE_DELETEARRAY(p)  { if(p) { delete [](p);     (p)=NULL; } }
#endif

#define ARRAYSIZE(arr)   (sizeof(arr) / sizeof(arr[0]))

#define DAEDLAUS_VERSION_NO		0x007b
#define DAEDALUS_VERSION		"0.07b"
#define DAEDALUS_SITE			"http://daedalus.boob.co.uk/"

// Uncomment this to disable some of the developer functions like
// DL_PF etc for performance improvements
#define DAEDALUS_RELEASE_BUILD
//#undef DAEDALUS_RELEASE_BUILD

// Config stuff - later move to a single struct or somethign
extern BOOL	g_bEepromPresent;

extern BOOL g_bRDPEnableGfx;		// Show graphics
extern BOOL g_bUseDynarec;			// Use dynamic recompilation
extern BOOL g_bApplyPatches;		// Apply os-hooks
extern BOOL g_bCRCCheck;			// Apply a crc-esque check to each texture each frame
extern BOOL g_bSpeedSync;			// Sync vi speed
extern BOOL g_bShowDebug;
extern BOOL g_bIncTexRectEdge;
extern BOOL g_bWarnMemoryErrors;
extern BOOL g_bRunAutomatically;
extern BOOL g_bTrapExceptions;
extern BOOL g_bSkipFirstRSP;

extern DWORD g_dwDisplayWidth;
extern DWORD g_dwDisplayHeight;
extern BOOL  g_bDisplayFullscreen;

// Aufzählung aller Arten von Nachrichten
enum MSG_TYPES
{
    MSG_ERROR,
    MSG_INFO
};

// Gibt eine Nachricht am Bildschirm aus
void outputMessage( MSG_TYPES type, char *format, ... );

// Gibt die Frames per Second aus
void printFPS();

#endif
