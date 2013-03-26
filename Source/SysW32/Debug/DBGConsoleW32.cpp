/*
Copyright (C) 2001 StrmnNrmn

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

// Lots of additional work by Lkb - thanks!

#include "stdafx.h"

#include "Debug/DBGConsole.h"
#include "DebugPane.h"
#include "DebugPaneOutput.h"
#include "DebugPaneMemory.h"
#include "DebugPaneCP0Regs.h"
#include "DebugPaneCP0Dis.h"

#include "Debug/Dump.h"

#include "Core/Registers.h"	// For RegNames
#include "Core/CPU.h"
#include "Core/Dynamo.h"
#include "Core/Interrupt.h"
#include "Core/RSP.h"
#include "Core/RSP_HLE.h"
#include "Core/R4300.h"
#include "Core/PIF.h"

#ifdef DAEDALUS_ENABLE_OS_HOOKS
#include "OSHLE/Patch.h"		// To get patch names
#include "OSHLE/ultra_r4300.h"
#endif

#include "Interface/MainWindow.h"

#include "Utility/Profiler.h"
#include "Utility/PrintOpCode.h"

#include "SysW32/Plugins/GraphicsPluginW32.h"

#include "ConfigOptions.h"

#include <map>

//*****************************************************************************
// Static variables
//*****************************************************************************

static const COORD sc_outbufsize = { 80, 120 };					// Size of the background text buffer

static const WORD sc_wAttrWhite			= FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE;
static const WORD sc_wAttrBoldWhite		= FOREGROUND_INTENSITY|FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE;
static const WORD sc_wAttrRed			= FOREGROUND_RED;
static const WORD sc_wAttrBoldRed		= FOREGROUND_INTENSITY|FOREGROUND_RED;

static const int NUM_OUTPUT_LINES = 24;
static const int sc_nCommandTextOffset = 59;


//*****************************************************************************
// Implementation of the debug console
//*****************************************************************************
class IDebugConsole : public CDebugConsole
{
	public:
		IDebugConsole();
		virtual ~IDebugConsole()	{ Finalise(); }

		void			EnableConsole( bool bEnable );
		bool			IsVisible() const;

		void			UpdateDisplay();

		void DAEDALUS_VARARG_CALL_TYPE	Msg(u32 type, const char * szFormat, ...);
		void							MsgOverwriteStart();
		void DAEDALUS_VARARG_CALL_TYPE	MsgOverwrite(u32 type, const char * szFormat, ...);
		void							MsgOverwriteEnd();
		void DAEDALUS_VARARG_CALL_TYPE	Stats( StatType stat, const char * szFormat, ...);

	protected:
		enum PaneType
		{
			DBGCON_REG_PANE = 0,		// Register view
			DBGCON_MEM_PANE,
			DBGCON_MID_PANE,			// Disasm/Mem area
			DBGCON_BOT_PANE,			// Output area

			DBGCON_NUM_PANES,
		};

	protected:
		bool			Initialise();
		void			Finalise();

		bool			ResizeConBufAndWindow(HANDLE hConsole, SHORT xSize, SHORT ySize);

		void			SetActivePane(PaneType pane);

		void			DumpContext_NextLine();
		void			DumpContext_NextPage();
		void			DumpContext_PrevLine();
		void			DumpContext_PrevPage();
		void			DumpContext_Home();

		void			RedrawPaneLabels( HANDLE hBuffer );

		static DWORD __stdcall ConsoleFuncStatic(LPVOID pArg);
		DWORD			ConsoleFunc();

		void			ProcessInput();
		void			ProcessKeyEvent( const KEY_EVENT_RECORD & kr );
		void			ProcessMouseEvent( const MOUSE_EVENT_RECORD & kr );


		void			ParseStringHighlights(char * szString, WORD * arrwAttributes, WORD wAttr);
		BOOL			WriteString(HANDLE hBuffer, const char * szString, BOOL bParse, s32 x, s32 y, WORD wAttr, s32 width);

	// Utility functions
		void *			UtilityGetAddressOfRegister(const char * szRegName, DWORD* pdwSize = NULL);
		ULONGLONG		UtilityReadRegister(void* pReg, DWORD regSize = 0);
		void			UtilityWriteRegister(void* pReg, ULONGLONG nValue, DWORD regSize = 0);
		const char *	UtilityGetValueToWrite(const char * szCommand, ULONGLONG nReadBits, ULONGLONG* pnValueToWrite);

		const char *	UtilitySimpleGetValue64(const char * szCommand, ULONGLONG* pValue);

		const char *	UtilityGetValue64(const char * szCommand, ULONGLONG* pValue);
		const char *	UtilityGetValue32(const char * szCommand, DWORD* pValue);

		void			UtilityCommandImplWrite(const char * szCommand, int nBits);

		HANDLE			CreateBuffer( u32 width, u32 height ) const;
		void			CopyBuffer( HANDLE hSource, HANDLE hDest );			// Assumes same-size buffers

	public:
		void			CommandHelp(const char * szCommand);
		void			CommandGo();
		void			CommandStop();

		void			CommandClose();
		void			CommandQuit();

		void			CommandFP(const char * szCommand);
		//void			CommandVec(const char * szCommand);
		void			CommandMem(const char * szCommand);
		void			CommandList(const char * szCommand);
	#ifdef DAEDALUS_ENABLE_OS_HOOKS
		void			CommandListOS(const char * szCommand);
	#endif
		void			CommandBPX(const char * szCommand);
		void			CommandBPD(const char * szCommand);
		void			CommandBPE(const char * szCommand);
		void			CommandShowCPU();

		void			CommandIntPi() { Msg(0, "Pi Interrupt"); Memory_MI_SetRegisterBits(MI_INTR_REG, MI_INTR_PI); R4300_Interrupt_UpdateCause3(); }
		void			CommandIntVi() { Msg(0, "Vi Interrupt"); Memory_MI_SetRegisterBits(MI_INTR_REG, MI_INTR_VI); R4300_Interrupt_UpdateCause3(); }
		void			CommandIntAi() { Msg(0, "Ai Interrupt"); Memory_MI_SetRegisterBits(MI_INTR_REG, MI_INTR_AI); R4300_Interrupt_UpdateCause3(); }
		void			CommandIntDp() { Msg(0, "Dp Interrupt"); Memory_MI_SetRegisterBits(MI_INTR_REG, MI_INTR_DP); R4300_Interrupt_UpdateCause3(); }
		void			CommandIntSp() { Msg(0, "Sp Interrupt"); Memory_MI_SetRegisterBits(MI_INTR_REG, MI_INTR_SP); R4300_Interrupt_UpdateCause3(); }
		void			CommandIntSi() { Msg(0, "Si Interrupt"); Memory_MI_SetRegisterBits(MI_INTR_REG, MI_INTR_SI); R4300_Interrupt_UpdateCause3(); }

		void			CommandDis(const char * szCommand);
		void			CommandRDis(const char * szCommand);
		void			CommandStrings(const char * szCommand);

		void			CommandWrite8(const char * szCommand);
		void			CommandWrite16(const char * szCommand);
		void			CommandWrite32(const char * szCommand);
		void			CommandWrite64(const char * szCommand);
		void			CommandWriteReg(const char * szCommand);

		void			CommandCPUSkip()				{ CPU_Skip(); }
		void			CommandRDPEnableGfx()			{ RSP_HLE_EnableGfx(); }
		void			CommandRDPDisableGfx()			{ RSP_HLE_DisableGfx(); }
#ifdef DAEDALUS_ENABLE_DYNAREC
		void			CommandCPUDynarecEnable()		{ CPU_DynarecEnable(); }
#endif
	#ifdef DUMPOSFUNCTIONS
		void			CommandPatchDumpOsThreadInfo()	{ Patch_DumpOsThreadInfo(); }
		void			CommandPatchDumpOsQueueInfo()	{ Patch_DumpOsQueueInfo(); }
		void			CommandPatchDumpOsEventInfo()	{ Patch_DumpOsEventInfo(); }
	#endif

	#ifdef DAEDALUS_DEBUG_DYNAREC
		void			CommandDumpDyna() { CPU_DumpFragmentCache(); }
	#endif

	protected:

		typedef struct
		{
			const char * szCommand;
			void (IDebugConsole::* pArglessFunc)();
			void (IDebugConsole::* pArgFunc)(const char *);
			const char * szHelpText;
		} DebugCommandInfo;

		typedef struct tagHelpTopicInfo
		{
			const char * szHelpTopic;
			const char * szHelpText;
		} HelpTopicInfo;

		typedef struct
		{
			int				nTopRow;
			int				nNumLines;
			CDebugPane *	CurrentPane;
		} PaneInfo;

		PaneInfo			mPaneInfo[ DBGCON_NUM_PANES ];
		static const DebugCommandInfo			mDebugCommands[];
		static const HelpTopicInfo				mHelpTopics[];

		HANDLE			mhStdOut;
		HANDLE			mhBackBuffer;				// Background buffer for rendering without flashing
		HANDLE			mhConsoleThread;

		bool			mConsoleAllocated;

		PaneType		mActivePane;

		char			mInputBuffer[1024+1];

		CDebugPaneOutput	mPaneOutput;
		CDebugPaneMemory	mPaneMemory;
		CDebugPaneCP0Regs	mPaneCP0Regs;
		CDebugPaneCP0Dis	mPaneCP0Dis;
};

//*****************************************************************************
//
//*****************************************************************************
template<> bool	CSingleton< CDebugConsole >::Create()
{
	DAEDALUS_ASSERT_Q(mpInstance == NULL);

	mpInstance = new IDebugConsole();

	return true;
}

//*****************************************************************************
//
//*****************************************************************************
CDebugConsole::~CDebugConsole()
{

}

//*****************************************************************************
//
//*****************************************************************************
IDebugConsole::IDebugConsole() :
	mhStdOut( NULL ),
	mhBackBuffer( INVALID_HANDLE_VALUE ),
	mhConsoleThread( NULL ),
	mConsoleAllocated( false ),
	mActivePane( DBGCON_MID_PANE )
{
	mInputBuffer[ 0 ] = '\0';

	mPaneInfo[ DBGCON_REG_PANE ].nTopRow = 0;
	mPaneInfo[ DBGCON_REG_PANE ].nNumLines = 11;
	mPaneInfo[ DBGCON_REG_PANE ].CurrentPane = &mPaneCP0Regs;

	mPaneInfo[ DBGCON_MEM_PANE ].nTopRow = 12;
	mPaneInfo[ DBGCON_MEM_PANE ].nNumLines = 7;
	mPaneInfo[ DBGCON_MEM_PANE ].CurrentPane = &mPaneMemory;

	mPaneInfo[ DBGCON_MID_PANE ].nTopRow = 20;
	mPaneInfo[ DBGCON_MID_PANE ].nNumLines = 13;
	mPaneInfo[ DBGCON_MID_PANE ].CurrentPane = &mPaneCP0Dis;

	mPaneInfo[ DBGCON_BOT_PANE ].nTopRow = 34;
	mPaneInfo[ DBGCON_BOT_PANE ].nNumLines = 24;
	mPaneInfo[ DBGCON_BOT_PANE ].CurrentPane = &mPaneOutput;
}

//*****************************************************************************
//
//*****************************************************************************
void IDebugConsole::EnableConsole( bool bEnable )
{
	// Shut down
	Finalise();

	if ( bEnable )
	{
		g_DaedalusConfig.ShowDebug = Initialise();
	}
	else
	{
		g_DaedalusConfig.ShowDebug = false;
	}

}

//*****************************************************************************
//
//*****************************************************************************
bool	IDebugConsole::IsVisible() const
{
	return (g_DaedalusConfig.ShowDebug && mConsoleAllocated);
}

//*****************************************************************************
//
//*****************************************************************************
bool IDebugConsole::Initialise()
{
	BOOL bRetVal;

	// Don't re-init!
	if (mConsoleAllocated)
		return true;

	bRetVal = AllocConsole();
	if (!bRetVal)
		return false;

	mConsoleAllocated = true;

    // Get a handle to the STDOUT screen buffer to copy from and
    // create a new screen buffer to copy to.

    mhStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (mhStdOut == INVALID_HANDLE_VALUE)
        return false;
	ResizeConBufAndWindow(mhStdOut, 80,60);

	mhBackBuffer = CreateBuffer( 80, 60 );
    if (mhBackBuffer == INVALID_HANDLE_VALUE)
        return false;


	if ( !mPaneOutput.Initialise() )
	{
		return false;
	}

	if ( !mPaneMemory.Initialise() )
	{
		return false;
	}
	mPaneMemory.SetAddress( 0xb0000000 );

	if ( !mPaneCP0Regs.Initialise() )
	{
		return false;
	}

	if ( !mPaneCP0Dis.Initialise() )
	{
		return false;
	}
	mPaneCP0Dis.SetAddress( gCPUState.CurrentPC );

	mPaneOutput.SetPos(  0, mPaneInfo[ DBGCON_BOT_PANE ].nTopRow+1, 80, mPaneInfo[ DBGCON_BOT_PANE ].nNumLines );
	mPaneMemory.SetPos(  0, mPaneInfo[ DBGCON_MEM_PANE ].nTopRow+1, 80, mPaneInfo[ DBGCON_MEM_PANE ].nNumLines );
	mPaneCP0Regs.SetPos( 0, mPaneInfo[ DBGCON_REG_PANE ].nTopRow+1, 80, mPaneInfo[ DBGCON_REG_PANE ].nNumLines );
	mPaneCP0Dis.SetPos(  0, mPaneInfo[ DBGCON_MID_PANE ].nTopRow+1, 80, mPaneInfo[ DBGCON_MID_PANE ].nNumLines );


	//
	// Render everything
	//
	mActivePane = DBGCON_BOT_PANE;
	UpdateDisplay();

	DWORD dwThreadID;

	mhConsoleThread = CreateThread(NULL, 0, ConsoleFuncStatic, this, CREATE_SUSPENDED , &dwThreadID);
	if (mhConsoleThread == NULL)
		return false;

	ResumeThread(mhConsoleThread);

	return true;
}

//*****************************************************************************
//
//*****************************************************************************
void IDebugConsole::Finalise()
{
	if (mhConsoleThread != NULL)
	{
		CloseHandle(mhConsoleThread);
		mhConsoleThread = NULL;
	}

	if ( mhBackBuffer != INVALID_HANDLE_VALUE )
	{
		CloseHandle( mhBackBuffer );
		mhBackBuffer = INVALID_HANDLE_VALUE;
	}

	mPaneOutput.Destroy();
	mPaneMemory.Destroy();
	mPaneCP0Regs.Destroy();
	mPaneCP0Dis.Destroy();

	if (mConsoleAllocated)
	{
		FreeConsole();
		mConsoleAllocated = false;
	}
}

//*****************************************************************************
//
//*****************************************************************************
void IDebugConsole::UpdateDisplay()
{
	// Return if we don't have a console!
	if (IsVisible() == false)
		return;

	//
	// Update the PCs
	//
	mPaneCP0Dis.SetAddress( gCPUState.CurrentPC );
	mPaneOutput.SetOffset( 0 );

	for ( u32 i = 0; i < DBGCON_NUM_PANES; i++ )
	{
		mPaneInfo[ i ].CurrentPane->Clear( mhBackBuffer );
		mPaneInfo[ i ].CurrentPane->Display( mhBackBuffer );
	}
	RedrawPaneLabels( mhBackBuffer );
	CopyBuffer( mhBackBuffer, mhStdOut );
}

//*****************************************************************************
//
//*****************************************************************************
DWORD __stdcall IDebugConsole::ConsoleFuncStatic( LPVOID pArg )
{
	IDebugConsole * p_console = static_cast< IDebugConsole * >( pArg );

	return p_console->ConsoleFunc();
}

//*****************************************************************************
//
//*****************************************************************************
DWORD IDebugConsole::ConsoleFunc()
{
    DWORD cNumRead, fdwMode, fdwSaveOldMode, i;
    INPUT_RECORD irInBuf[128];
	COORD curPos;

    HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);
    if (hInput == INVALID_HANDLE_VALUE)
        return 1;

	SetConsoleTitle( "Daedalus Debug Console" );

    // Save the current input mode, to be restored on exit.
    if (!GetConsoleMode(hInput, &fdwSaveOldMode) )
        return 1;

    // Enable the window and mouse input events.
    fdwMode = ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT;
    if ( !SetConsoleMode(hInput, fdwMode) )
        return 1;

	curPos.X = 0;
	curPos.Y = sc_nCommandTextOffset;
	SetConsoleCursorPosition(mhStdOut, curPos);


	// Handle set to null on exit
	while (mhConsoleThread != NULL)
	{

        if ( !ReadConsoleInput(
                hInput,      // input buffer handle
                irInBuf,     // buffer to read into
                128,         // size of read buffer
                &cNumRead) ) // number of records read
		{
            break;
		}

        // Dispatch the events to the appropriate handler.

        for (i = 0; i < cNumRead; i++)
        {
            switch(irInBuf[i].EventType)
            {
                case KEY_EVENT: // keyboard input

                    ProcessKeyEvent( irInBuf[i].Event.KeyEvent );

                    break;

				case MOUSE_EVENT: // mouse input
                    ProcessMouseEvent( irInBuf[i].Event.MouseEvent );
                    break;

                /*case WINDOW_BUFFER_SIZE_EVENT: // scrn buf. resizing
                    ResizeEventProc(
                        irInBuf[i].Event.WindowBufferSizeEvent);
                    break; */

                case FOCUS_EVENT:  // disregard focus events
                    break;

                case MENU_EVENT:   // disregard menu events
                    break;

                default:
                    break;
            }
		}

	}

	return 0;
}



//*****************************************************************************
//
//*****************************************************************************
bool IDebugConsole::ResizeConBufAndWindow(HANDLE hConsole, SHORT xSize, SHORT ySize)
{
	CONSOLE_SCREEN_BUFFER_INFO csbi; // hold current console buffer info
	BOOL bSuccess;
	SMALL_RECT srWindowRect; // hold the new console size
	COORD coordScreen;

	bSuccess = GetConsoleScreenBufferInfo(hConsole, &csbi);
	if (!bSuccess)
		return false;

	// get the largest size we can size the console window to

	coordScreen = GetLargestConsoleWindowSize(hConsole);
	if (!bSuccess)
		return false;

	// define the new console window size and scroll position
	srWindowRect.Right = (SHORT) (min(xSize, coordScreen.X) - 1);
	srWindowRect.Bottom = (SHORT) (min(ySize, coordScreen.Y) - 1);

	srWindowRect.Left = srWindowRect.Top = (SHORT) 0;

	// define the new console buffer size
	coordScreen.X = xSize;
	coordScreen.Y = ySize;

	// if the current buffer is larger than what we want, resize the
	// console window first, then the buffer
	if ((DWORD) csbi.dwSize.X * csbi.dwSize.Y > (DWORD) xSize * ySize)
	{
		bSuccess = SetConsoleWindowInfo(hConsole, TRUE, &srWindowRect);
		if (!bSuccess)
			return false;

		bSuccess = SetConsoleScreenBufferSize(hConsole, coordScreen);
		if (!bSuccess)
			return false;
	}

	// if the current buffer is smaller than what we want, resize the
	// buffer first, then the console window
	if ((DWORD) csbi.dwSize.X * csbi.dwSize.Y < (DWORD) xSize * ySize)
	{
		bSuccess = SetConsoleScreenBufferSize(hConsole, coordScreen);
		if (!bSuccess)
			return false;

		bSuccess = SetConsoleWindowInfo(hConsole, TRUE, &srWindowRect);
		if (!bSuccess)
			return false;
	}

	// if the current buffer *is* the size we want, don't do anything!
	return true;
}

//*****************************************************************************
//
//*****************************************************************************
void IDebugConsole::SetActivePane(PaneType pane)
{
	mActivePane = pane;

	UpdateDisplay();
}

//*****************************************************************************
//
//*****************************************************************************
void IDebugConsole::RedrawPaneLabels( HANDLE hBuffer )
{
	int i;
	CHAR szLine[80+1];
	CHAR szPaneName[80+1];
	CHAR * s, * d;
	static const CHAR cLine = '-';
	static const CHAR cDownArrow = '+';
	static const CHAR cLeftrightArrow = '|';

	// Top pane
	for ( DWORD p = 0; p < DBGCON_NUM_PANES; p++ )
	{
		lstrcpyn(szPaneName, mPaneInfo[ p ].CurrentPane->GetName(), 80);

		for (i = 0; i < 80; i++)
			szLine[i] = cLine;

		if ( p == mActivePane )
		{
			szLine[1] = szLine[2] = szLine[3] = szLine[4] = cDownArrow;
		}
		szLine[5] = 'F'; szLine[6] = CHAR( '1' + p );
		for (s = szPaneName, d = &szLine[8]; s[0] != 0; s++,d++)
			d[0] = s[0];


		szLine[78] = cLeftrightArrow;

		szLine[80] = 0;
		WriteString( hBuffer, szLine, FALSE, 0, mPaneInfo[ p ].nTopRow, BACKGROUND_GREEN, 80 );
	}
}

//*****************************************************************************
//
//*****************************************************************************
void IDebugConsole::DumpContext_PrevLine()
{
	mPaneInfo[ mActivePane ].CurrentPane->ScrollUp();
	mPaneInfo[ mActivePane ].CurrentPane->Display( mhStdOut );
}

//*****************************************************************************
//
//*****************************************************************************
void IDebugConsole::DumpContext_PrevPage()
{
	mPaneInfo[ mActivePane ].CurrentPane->PageUp();
	mPaneInfo[ mActivePane ].CurrentPane->Display( mhStdOut );
}

//*****************************************************************************
//
//*****************************************************************************
void IDebugConsole::DumpContext_NextLine()
{
	mPaneInfo[ mActivePane ].CurrentPane->ScrollDown();
	mPaneInfo[ mActivePane ].CurrentPane->Display( mhStdOut );
}

//*****************************************************************************
//
//*****************************************************************************
void IDebugConsole::DumpContext_NextPage()
{
	mPaneInfo[ mActivePane ].CurrentPane->PageDown();
	mPaneInfo[ mActivePane ].CurrentPane->Display( mhStdOut );
}

//*****************************************************************************
//
//*****************************************************************************
void IDebugConsole::DumpContext_Home()
{
	mPaneInfo[ mActivePane ].CurrentPane->Home();
	mPaneInfo[ mActivePane ].CurrentPane->Display( mhStdOut );
}

//*****************************************************************************
// Write a string of characters to a screen buffer.
//*****************************************************************************
BOOL IDebugConsole::WriteString( HANDLE hBuffer, const char * szString, BOOL bParse, s32 x, s32 y, WORD wAttr, s32 width )
{
    DWORD cWritten;
    BOOL bSuccess;
    COORD coord;
	WORD wNumToWrite;
	char szBuffer[2048+1];
	WORD arrwAttributes[2048];

    coord.X = s16( x );            // start at first cell
    coord.Y = s16( y );            //   of first row

	lstrcpyn(szBuffer, szString, 2048);

	if (bParse)
	{
		ParseStringHighlights(szBuffer, arrwAttributes, wAttr);
	}

	wNumToWrite = lstrlen(szBuffer);
	if (wNumToWrite > width)
		wNumToWrite = s16( width );

    bSuccess = WriteConsoleOutputCharacter(
        hBuffer,              // screen buffer handle
        szBuffer,				// pointer to source string
        wNumToWrite,			// length of string
        coord,					// first cell to write to
        &cWritten);				// actual number written
    if (!bSuccess)
        return FALSE;

	// Copy the visible portion to the display
	if (bParse)
	{
		/*bSuccess = */WriteConsoleOutputAttribute(
			hBuffer,
			arrwAttributes,
			wNumToWrite,
			coord,
			&cWritten);
	}
	else
	{
		// Just clear
		/*bSuccess = */FillConsoleOutputAttribute(
			hBuffer,
			wAttr,
			wNumToWrite,		// Clear to end
			coord,
			&cWritten);
	}

	// Clear the end of the buffer
	if (coord.X + wNumToWrite < width)
	{
		coord.X += wNumToWrite;
		/*bSuccess = */FillConsoleOutputAttribute(
			hBuffer,
			wAttr,
			width - wNumToWrite,		// Clear to end
			coord,
			&cWritten);

		/*bSuccess = */FillConsoleOutputCharacter(hBuffer,
			' ', width - coord.X, coord, &cWritten);
	}

	return TRUE;
}

//*****************************************************************************
//
//*****************************************************************************
void IDebugConsole::ProcessKeyEvent( const KEY_EVENT_RECORD & kr )
{
	char szInputChar[1+1];
	COORD curPos;
	DWORD dwInputLen;

	dwInputLen = lstrlen(mInputBuffer);

	if (kr.bKeyDown)
	{
		switch (kr.wVirtualKeyCode)
		{
		case VK_RETURN:
			// Process the current input
			if (lstrlen(mInputBuffer) > 0)
			{
				ProcessInput();

				// Reset the input buffer
				lstrcpy(mInputBuffer, "");

				WriteString(mhStdOut, mInputBuffer, FALSE, 0, sc_nCommandTextOffset, BACKGROUND_GREEN | BACKGROUND_BLUE, 80);
			}
			break;
		case VK_BACK:
			if (dwInputLen > 0)
			{
				mInputBuffer[dwInputLen-1] = '\0';
			}
			WriteString(mhStdOut, mInputBuffer, FALSE, 0, sc_nCommandTextOffset, BACKGROUND_GREEN | BACKGROUND_BLUE, 80);
			break;
		case VK_PRIOR:
			DumpContext_PrevPage();
			break;
		case VK_UP:
			DumpContext_PrevLine();
			break;
		case VK_HOME:
			DumpContext_Home();
			break;
		case VK_DOWN:
			DumpContext_NextLine();
			break;
		case VK_NEXT:
			DumpContext_NextPage();
			break;

		case VK_F1:
			SetActivePane(DBGCON_REG_PANE);
			break;
		case VK_F2:
			SetActivePane(DBGCON_MEM_PANE);
			break;
		case VK_F3:
			SetActivePane(DBGCON_MID_PANE);
			break;
		case VK_F4:
			SetActivePane(DBGCON_BOT_PANE);
			break;

		// Step into
		case VK_F10:
			CPU_Step();
			break;


		default:
			if (kr.dwControlKeyState & LEFT_CTRL_PRESSED ||
				kr.dwControlKeyState & RIGHT_CTRL_PRESSED)
			{
				if (kr.wVirtualKeyCode == 'D' ||
					kr.wVirtualKeyCode == 'd')		// Not sure if lowercase is necessary
				{
					EnableConsole(!g_DaedalusConfig.ShowDebug);
				}
			}

			if(kr.uChar.AsciiChar >= 32)
			{

				wsprintf(szInputChar, "%c", kr.uChar.AsciiChar);
				lstrcat(mInputBuffer, szInputChar);

				WriteString(mhStdOut, mInputBuffer, FALSE, 0, sc_nCommandTextOffset, BACKGROUND_GREEN | BACKGROUND_BLUE, 80);
			}

			break;
		}
	}

	curPos.X = lstrlen(mInputBuffer);
	curPos.Y = sc_nCommandTextOffset;

	SetConsoleCursorPosition(mhStdOut, curPos);

}


//*****************************************************************************
//
//*****************************************************************************
void IDebugConsole::ProcessMouseEvent( const MOUSE_EVENT_RECORD & mr )
{
	static bool left_mouse_held = false;
	static u32 left_mouse_click_y = 0;
	static PaneType dragging_pane = DBGCON_NUM_PANES;

	if ( mr.dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED )
	{
		if ( mr.dwEventFlags == 0 )
		{
			left_mouse_held = true;
			left_mouse_click_y = mr.dwMousePosition.Y;
			dragging_pane = DBGCON_NUM_PANES;

			// Single click
			for ( u32 i = 0; i < DBGCON_NUM_PANES; i++ )
			{
				int top = mPaneInfo[ i ].nTopRow;
				int lines = mPaneInfo[ i ].nNumLines;

				if ( mr.dwMousePosition.Y == top )
				{
					dragging_pane = static_cast< PaneType >( i );

					if ( mr.dwMousePosition.X == 78 )
					{
						// Flip pane
						if ( i == DBGCON_MID_PANE )
						{
							CommandShowCPU();
						}
					}
				}
				else if ( mr.dwMousePosition.Y >= top &&
						  mr.dwMousePosition.Y <= top + lines )
				{
					SetActivePane( PaneType( i ) );
				}
			}
		}
		else if ( mr.dwEventFlags == DOUBLE_CLICK )
		{
		}
		else if ( mr.dwEventFlags == MOUSE_MOVED )
		{
			if ( left_mouse_held && dragging_pane != DBGCON_NUM_PANES )
			{
				s32 current_y = mr.dwMousePosition.Y;
				s32 last_line = 59;

				if ( dragging_pane+1 < DBGCON_NUM_PANES )
					last_line = mPaneInfo[ dragging_pane+1 ].nTopRow;


				if ( dragging_pane > 0 )			// Can't drag top pane
				{
					if ( current_y < mPaneInfo[ dragging_pane - 1 ].nTopRow+1 )
						current_y = mPaneInfo[ dragging_pane - 1 ].nTopRow+1;
					if ( current_y >= last_line )
						current_y = last_line - 1;

					mPaneInfo[ dragging_pane - 1 ].nNumLines = current_y - (mPaneInfo[ dragging_pane - 1 ].nTopRow+1);
					mPaneInfo[ dragging_pane ].nTopRow = current_y;
					mPaneInfo[ dragging_pane ].nNumLines = last_line - (mPaneInfo[ dragging_pane ].nTopRow+1);

					mPaneInfo[ dragging_pane-1 ].CurrentPane->SetPos(  0, mPaneInfo[ dragging_pane-1 ].nTopRow+1, 80, mPaneInfo[ dragging_pane-1 ].nNumLines );
					mPaneInfo[ dragging_pane   ].CurrentPane->SetPos(  0, mPaneInfo[ dragging_pane   ].nTopRow+1, 80, mPaneInfo[ dragging_pane   ].nNumLines );

					UpdateDisplay();
				}
			}
		}
	}
	else
	{
		if ( left_mouse_held )
		{
			left_mouse_held = false;
		}
	}
}


//*****************************************************************************
//
//*****************************************************************************
void IDebugConsole::ProcessInput()
{
	DWORD i;
	BOOL bProcessed;

	bProcessed = FALSE;

	for (i = 0; ; i++)
	{
		if (mDebugCommands[i].szCommand == NULL)
			break;

		DWORD dwCmdLen = lstrlen(mDebugCommands[i].szCommand);
		DWORD dwInputLen = lstrlen(mInputBuffer);

		if (_strnicmp(mDebugCommands[i].szCommand, mInputBuffer, dwCmdLen) == 0)
		{
			if ( mDebugCommands[i].pArglessFunc )
			{
				(this->*mDebugCommands[i].pArglessFunc)( );
			}
			else if ( mDebugCommands[i].pArgFunc )
			{
				// Trim spaces...
				char * szArgs = mInputBuffer + dwCmdLen;
				while (*szArgs == ' ')
					szArgs++;

				(this->*mDebugCommands[i].pArgFunc)(szArgs);
			}
			bProcessed = TRUE;
			break;
		}
	}

	if ( !bProcessed )
	{
		DBGConsole_Msg(0, "[YUnknown command: %s]", mInputBuffer);
	}
}

//*****************************************************************************
//
//*****************************************************************************
typedef struct tagMiscRegInfo
{
	const char * szName;
	void* pReg;
	DWORD nBits;
} MiscRegInfo;

#define BEGIN_MISCREG_MAP(name) \
static const MiscRegInfo name[] =	\
{
#define MISCREG(name, reg, bits) \
{ name, &(reg), bits },

#define END_MISCREG_MAP()		\
	{ NULL, NULL }					\
};

BEGIN_MISCREG_MAP(g_MiscRegNames)
	MISCREG("pc", gCPUState.CurrentPC, 32)
	MISCREG("hi", gCPUState.MultHi._u64, 64)
	MISCREG("lo", gCPUState.MultLo._u64, 64)
END_MISCREG_MAP()

//*****************************************************************************
// returns size of the register in bits. 0 = register not found
//*****************************************************************************
void * IDebugConsole::UtilityGetAddressOfRegister( const char * szRegName, DWORD* pdwSize )
{
	void* pReg = NULL;
	DWORD dwSize = 0;

	int i;

	for (i = 0; ; i++)
	{
		if (g_MiscRegNames[i].szName == NULL)
			break;

		if (_strcmpi(g_MiscRegNames[i].szName, szRegName) == 0)
		{
			dwSize = g_MiscRegNames[i].nBits;
			pReg = g_MiscRegNames[i].pReg;
		}
	}
	if(pReg == NULL)
	{
		for (i = 0; i < 32; i++)
		{
			if (_strcmpi(szRegName, RegNames[i]) == 0)
			{
				pReg = &gGPR[i];
				dwSize = 64;
				break;
			}
			else if (_strcmpi(szRegName, Cop0RegNames[i]) == 0)
			{
				pReg = &gCPUState.CPUControl[i];
				dwSize = 64;
				break;
			}
			else if (_strcmpi(szRegName, ShortCop0RegNames[i]) == 0)
			{
				pReg = &gCPUState.CPUControl[i];
				dwSize = 64;
				break;
			}
		}
	}
	if(pReg == NULL)
	{
		DBGConsole_Msg(0, "[YInvalid register name in numeric argument! Type \"help numbers\" for more info]");
		//DBGConsole_Msg(0, "[Yreg %s not found]", szRegName);
		return NULL;
	}
	else
	{
		if(pdwSize != NULL)
		{
			*pdwSize = dwSize;
		}
		return pReg;
	}
}

//*****************************************************************************
//
//*****************************************************************************
ULONGLONG IDebugConsole::UtilityReadRegister( void* pReg, DWORD regSize )
{
	if(pReg == NULL)
		return 0;

	if(regSize == 0)
	{
		regSize = (pReg == &gCPUState.CurrentPC) ? 32 : 64;
	}
	//ULONGLONG regValue = *(ULONGLONG*)pReg;
	/*switch(regSize)
	{
		case 8:
			return regValue & (u64)~((u64)(1 << 8));
		case 16:
			return regValue & (u64)~((u64)(1 << 16));
		case 32:
			return regValue & (u64)~((u64)(1 << 32));
		default:
			return regValue;
	}*/

	// STRMNRMN - bugfix - these were & (u64)~((u64)1<<n), which masked with ..111101111111 etc
	// It's easier just to read the correct size to start with?
	switch(regSize)
	{
		case 8:
			return *(BYTE*)pReg;
		case 16:
			return *(WORD*)pReg;
		case 32:
			return *(DWORD*)pReg;
		default:
			return *(ULONGLONG*)pReg;
	}

}

//*****************************************************************************
//
//*****************************************************************************
void IDebugConsole::UtilityWriteRegister( void* pReg, ULONGLONG nValue, DWORD regSize )
{
	if(pReg == NULL)
		return;

	if(regSize == 0)
	{
		regSize = (pReg == &gCPUState.CurrentPC) ? 32 : 64;
	}
	ULONGLONG pcTemp = 0;
	if(pReg == &gCPUState.CurrentPC)
	{
		pReg = &pcTemp;
	}
	ULONGLONG regValue = *(ULONGLONG*)pReg;
	switch(regSize)
	{
		case 8:
			*(u8*)pReg = (u8)nValue;
			break;
		case 16:
			*(u16*)pReg = (u16)nValue;
			break;
		case 32:
			*(u32*)pReg = (u32)nValue;
			break;
		default:
			*(u64*)pReg = nValue;
			break;
	}
	if(pReg == &pcTemp)
	{
		CPU_SetPC((DWORD)pcTemp);
	}
}

//*****************************************************************************
// if bHex is TRUE non-0x values are interpreted as hex, else they are interpreted as decimal
// 0xaaaaaaa -> Hex
// 0t3465767 -> Dec (t -> ten :) )
//
// The fact the some commands use decimal and others hex as the default may lead to confusion
// However I'm not sure on what the best solution is
//
//
//*****************************************************************************
const char * IDebugConsole::UtilitySimpleGetValue64(const char * szCommand, ULONGLONG* pValue)
{
	// PRB: What happens if %i is specified and the number is > 2^31? I hope it's read as an unsigned integer

	int n = -1;
	n = sscanf(szCommand, "0x%I64x", pValue);
	if (n != 1)
	{
		n = sscanf(szCommand, "0t%I64i", pValue);
	}
	if (n != 1)
	{
		// Check for absence of 0x or 0t prefixes - default is hex!
		//if(bHex)
		//{
			n = sscanf(szCommand, "%I64x", pValue);
		//}
		//else
		//{
		//	n = sscanf(szCommand, "%I64i", pValue);
		//}
	}
	if (n != 1)
	{
		// TODO: implement "help numbers"
		DBGConsole_Msg(0, "[YInvalid numeric argument! Type \"help numbers\" for more info]");
		return NULL;
	}

	const char * pszWhiteSpaceAfterScannedValue = strchr(szCommand, ' ');
	if(pszWhiteSpaceAfterScannedValue != NULL)
	{
		return pszWhiteSpaceAfterScannedValue;
	}
	else
	{
		return szCommand + strlen(szCommand);
	}
}

//*****************************************************************************
// Converts the first value in the string into a number, returns the
// character just after the string
//*****************************************************************************
const char * IDebugConsole::UtilityGetValue64(const char * szCommand, ULONGLONG* pValue)
{
	if(szCommand == NULL)
		return NULL;

	while(*szCommand == ' ') szCommand++;

	const char * pszCharAfterValue = szCommand + strlen(szCommand);

	ULONGLONG qwAddress = 0;

	// Check if this is a register, e.g. "mem [sp+30]"
	if (szCommand[0] == '%')
	{
		char szRegName[100];

		const char * temp_pszRegisterNameEnd = strpbrk(szCommand + 1, "+- ");
		if(temp_pszRegisterNameEnd != NULL)
			pszCharAfterValue = temp_pszRegisterNameEnd;

		DWORD dwRegNameLength = pszCharAfterValue - szCommand - 1;
		if(dwRegNameLength == 0)
		{
			DBGConsole_Msg(0, "[YInvalid register name in numeric argument! Type \"help numbers\" for more info]");
			return NULL;
		}

		memcpy(szRegName, szCommand + 1, dwRegNameLength);
		szRegName[dwRegNameLength] = 0;

		DWORD regSize;
		void* pReg = UtilityGetAddressOfRegister(szRegName, &regSize);
		if(pReg != NULL)
		{
			qwAddress = UtilityReadRegister(pReg, regSize);
		}

		if((*pszCharAfterValue != 0) && (strpbrk(pszCharAfterValue, "+-") != NULL))
		{
			ULONGLONG qwOffset;
			if((pszCharAfterValue = UtilitySimpleGetValue64(pszCharAfterValue, &qwOffset)) == NULL)
				return NULL;
			qwAddress += qwOffset;
		}
	}
	else
	{
		if((pszCharAfterValue = UtilitySimpleGetValue64(szCommand, &qwAddress)) == NULL)
			return NULL;
	}

	*pValue = qwAddress;

	return pszCharAfterValue;
}

//*****************************************************************************
//
//*****************************************************************************
const char * IDebugConsole::UtilityGetValue32(const char * szCommand, DWORD* pValue)
{
	ULONGLONG temp64;
	const char * retVal = UtilityGetValue64(szCommand, &temp64);
	*pValue = (DWORD)temp64;
	return retVal;
}

//*****************************************************************************
//
//*****************************************************************************
void IDebugConsole::CommandFP(const char * szCommand)
{
	LONG n;
	DWORD dwReg;

	// We need the decimal value
	n = sscanf(szCommand, "%d", &dwReg);
	if (n == 1)
	{
		// TODO show long and double values
		DBGConsole_Msg(0, "FP%02d", dwReg);
		DBGConsole_Msg(0, "w: 0x%08x = %d", gCPUState.FPU[dwReg]._u32, gCPUState.FPU[dwReg]._u32);
		DBGConsole_Msg(0, "f: %f", gCPUState.FPU[dwReg]._u32);
	}

}

//*****************************************************************************
//
//*****************************************************************************
//void IDebugConsole::CommandVec(const char * szCommand)
//{
//	LONG n;
//	DWORD dwReg1, dwReg2, dwReg3;
//
//	// We need the decimal value
//	n = sscanf(szCommand, "%d %d %d", &dwReg1, &dwReg2, &dwReg3);
//	if (n == 1)
//	{
//		RSP_DumpVector(dwReg1);
//	}
//	else if (n == 3)
//	{
//		RSP_DumpVectors(dwReg1, dwReg2, dwReg3);
//	}
//}

//*****************************************************************************
//
//*****************************************************************************
void IDebugConsole::CommandMem(const char * szCommand)
{
	DWORD dwAddress;
	if(UtilityGetValue32(szCommand, &dwAddress) != NULL)
	{
		// TODO: Check if this pane is visible!
		mPaneMemory.SetAddress( dwAddress );
		mPaneMemory.Display( mhStdOut );
	}
}

//*****************************************************************************
//
//*****************************************************************************
void WriteBits(DWORD dwAddress, ULONGLONG nValue, int nBits)
{
	switch(nBits)
	{
		case 8:
			Write8Bits(dwAddress, (u8)nValue);
			break;
		case 16:
			Write16Bits(dwAddress, (u16)nValue);
			break;
		case 32:
			Write32Bits(dwAddress, (u32)nValue);
			break;
		case 64:
			Write64Bits(dwAddress, (u64)nValue);
			break;
	}
}

//*****************************************************************************
//
//*****************************************************************************
ULONGLONG ReadBits(DWORD dwAddress, int nBits)
{
	switch(nBits)
	{
		case 8:
			return (ULONGLONG)Read8Bits(dwAddress);
		case 16:
			return (ULONGLONG)Read16Bits(dwAddress);
		case 32:
			return (ULONGLONG)Read32Bits(dwAddress);
		case 64:
			return (ULONGLONG)Read64Bits(dwAddress);
	}
	return 0;
}

//*****************************************************************************
//
//*****************************************************************************
const char * IDebugConsole::UtilityGetValueToWrite(const char * szCommand, ULONGLONG nReadBits, ULONGLONG* pnValueToWrite)
{
	ULONGLONG nRValue;
	BOOL bBitMode = FALSE;
	BOOL bNotMode = FALSE;
	char cOperator = '=';
	const char * psz = szCommand;

	if(psz == NULL)
		return NULL;

	while(*psz == ' ') psz++;

	while(TRUE)
	{
		switch(*psz)
		{
			case '$':
				bBitMode = TRUE;
				break;
			case '^':
			case '|':
			case '&':
			case '=':
			case '+':
			case '-':
				cOperator = *psz;
				break;
			case '~':
				bNotMode = TRUE;
				break;
			default:
				goto endoperators;
		}
		psz++;
	}
endoperators:

	if(UtilityGetValue64(psz, &nRValue) == NULL)
		return NULL;

	if(bBitMode)
	{
		nRValue = 1LL << (BYTE)nRValue;
	}
	if(bNotMode)
	{
		nRValue = ~nRValue;
	}
	switch(cOperator)
	{
		case '^':
			nRValue = nRValue ^ nReadBits;
			break;
		case '|':
			nRValue |= nReadBits;
			break;
		case '&':
			nRValue &= nReadBits;
			break;
		case '+':
			nRValue += nReadBits;
			break;
		case '-':
			nRValue = nReadBits - nRValue;
			break;
	}
	*pnValueToWrite = nRValue;
	return psz;
}

//*****************************************************************************
//
//*****************************************************************************
void IDebugConsole::UtilityCommandImplWrite(const char * szCommand, int nBits)
{
	ULONGLONG nRValue;
	DWORD dwAddress;
	const char * psz = UtilityGetValue32(szCommand, &dwAddress);
	if(UtilityGetValueToWrite(psz, ReadBits(dwAddress, nBits), &nRValue) != NULL)
	{
		WriteBits(dwAddress, nRValue, nBits);

		UpdateDisplay();
	}
}

//*****************************************************************************
//
//*****************************************************************************
void IDebugConsole::CommandDis(const char * szCommand)
{
	LONG n;
	char szStart[300];
	char szEnd[300];
	char szFileName[MAX_PATH+1] = "";		// Initialise to empty string
	DWORD dwStart;
	DWORD dwEnd;

	n = sscanf(szCommand, "%s %s %s", szStart, szEnd, szFileName);
	// Note - if the filename is left blank, Dump_Disassemble defaults
	if (n < 2)
	{
		DBGConsole_Msg(0, "[YInvalid argument! Type \"help dis\" for more info]");
		return;
	}

	UtilityGetValue32(szStart, &dwStart);
	UtilityGetValue32(szEnd, &dwEnd);

	Dump_Disassemble(dwStart, dwEnd, szFileName);

}

//*****************************************************************************
//
//*****************************************************************************
void IDebugConsole::CommandRDis(const char * szCommand)
{
	Dump_RSPDisassemble(szCommand);
}

//*****************************************************************************
//
//*****************************************************************************
void IDebugConsole::CommandStrings(const char * szCommand)
{
	Dump_Strings(szCommand);
}


//*****************************************************************************
//
//*****************************************************************************
#define IMPLEMENT_DBGCommand_Write(bits) void IDebugConsole::CommandWrite##bits (const char * szCommand) {UtilityCommandImplWrite(szCommand, bits );}

IMPLEMENT_DBGCommand_Write(8)
IMPLEMENT_DBGCommand_Write(16)
IMPLEMENT_DBGCommand_Write(32)
IMPLEMENT_DBGCommand_Write(64)

#undef IMPLEMENT_DBGCommand_Write



//*****************************************************************************
//
//*****************************************************************************
void IDebugConsole::CommandWriteReg(const char * szCommand)
{
	while(*szCommand == '%') szCommand++;
	const char * pszCharAfterValue = strchr(szCommand, ' ');
	if(pszCharAfterValue == NULL)
	{
		DBGConsole_Msg(0, "[YInvalid numeric argument! Type \"help numbers\" for more info]");
		return;
	}

	DWORD dwRegNameLength = (DWORD)pszCharAfterValue - (DWORD)szCommand;
	if(dwRegNameLength == 0)
	{
		DBGConsole_Msg(0, "[YInvalid register name in numeric argument! Type \"help numbers\" for more info]");
		return;
	}

	char szRegName[100];
	memcpy(szRegName, szCommand, dwRegNameLength);
	szRegName[dwRegNameLength] = 0;

	DWORD nBits;
	void* pReg = UtilityGetAddressOfRegister(szRegName, &nBits);
	if(pReg == NULL)
		return;

	ULONGLONG nRValue;
	/*
	if(UtilityGetValue64(pszCharAfterValue, &nRValue) == NULL)
		return;
	*/
	if(UtilityGetValueToWrite(pszCharAfterValue, UtilityReadRegister(pReg, nBits), &nRValue) != NULL)
	{
		UtilityWriteRegister(pReg, nRValue, nBits);

		UpdateDisplay();
	}
}

//*****************************************************************************
//
//*****************************************************************************
void IDebugConsole::CommandList(const char * szCommand)
{
	DWORD dwAddress;
	if(UtilityGetValue32(szCommand, &dwAddress) != NULL)
	{
		mPaneInfo[ DBGCON_REG_PANE ].CurrentPane = &mPaneCP0Regs;
		mPaneInfo[ DBGCON_MID_PANE ].CurrentPane = &mPaneCP0Dis;

		mPaneCP0Dis.SetAddress( dwAddress );
		UpdateDisplay();
	}

}


#ifdef DAEDALUS_ENABLE_OS_HOOKS
//*****************************************************************************
//
//*****************************************************************************
void IDebugConsole::CommandListOS(const char * szCommand)
{
	if (lstrlen(szCommand) == 0)
	{
		DBGConsole_Msg(0, "Invalid argument: %s", szCommand);
	}
	else
	{
		mPaneInfo[ DBGCON_REG_PANE ].CurrentPane = &mPaneCP0Regs;
		mPaneInfo[ DBGCON_MID_PANE ].CurrentPane = &mPaneCP0Dis;

		DWORD dwAddress = Patch_GetSymbolAddress(szCommand);
		if (dwAddress != ~0)
		{
			mPaneCP0Dis.SetAddress( dwAddress );
		}
		else
		{
			DBGConsole_Msg(0, "Symbol %s not found", szCommand);
		}

		UpdateDisplay();
	}
}
#endif

//*****************************************************************************
//
//*****************************************************************************
void IDebugConsole::CommandBPX(const char * szCommand)
{
#ifdef DAEDALUS_BREAKPOINTS_ENABLED
	DWORD dwAddress;
	if(UtilityGetValue32(szCommand, &dwAddress) != NULL)
	{
		CPU_AddBreakPoint(dwAddress);
		UpdateDisplay();
	}
#endif
}

//*****************************************************************************
//
//*****************************************************************************
void IDebugConsole::CommandBPD(const char * szCommand)
{
#ifdef DAEDALUS_BREAKPOINTS_ENABLED
	DWORD dwAddress;
	if(UtilityGetValue32(szCommand, &dwAddress) != NULL)
	{
		CPU_EnableBreakPoint(dwAddress, FALSE);
		UpdateDisplay();
	}
#endif
}

//*****************************************************************************
//
//*****************************************************************************
void IDebugConsole::CommandBPE(const char * szCommand)
{
#ifdef DAEDALUS_BREAKPOINTS_ENABLED
	DWORD dwAddress;
	if(UtilityGetValue32(szCommand, &dwAddress) != NULL)
	{
		CPU_EnableBreakPoint(dwAddress, TRUE);
		UpdateDisplay();
	}
#endif
}

//*****************************************************************************
//
//*****************************************************************************
void IDebugConsole::CommandShowCPU()
{
	mPaneInfo[ DBGCON_REG_PANE ].CurrentPane = &mPaneCP0Regs;
	mPaneInfo[ DBGCON_MID_PANE ].CurrentPane = &mPaneCP0Dis;
	UpdateDisplay();
}

//*****************************************************************************
//
//*****************************************************************************
void IDebugConsole::CommandHelp(const char * szCommand)
{
	if(*szCommand == 0)
	{
		int i;

		for(i = 0;; i++)
		{
			if(mHelpTopics[i].szHelpTopic == NULL)
				break;
		}

		DBGConsole_Msg(0, "[YSupported commands (help is available for light blue commands)]");

		for (i = 0; ; i++)
		{
			if (mDebugCommands[i].szCommand == NULL)
				break;

			DBGConsole_Msg(0, "\t[%c%s%s]", (mDebugCommands[i].szHelpText != NULL) ? 'C' : 'W', mDebugCommands[i].szCommand, (mDebugCommands[i].pArgFunc != NULL) ? " <parameters>" : "");
		}

		DBGConsole_Msg(0, "\n[YAvailable help topics]");

		for (i = 0; ; i++)
		{
			if (mHelpTopics[i].szHelpTopic == NULL)
				break;

			DBGConsole_Msg(0, "\t[%c%s]", (mHelpTopics[i].szHelpText != NULL) ? 'C' : 'W', mHelpTopics[i].szHelpTopic);
		}

		DBGConsole_Msg(0, "\n[YUse \"help <topic>\" to get help on a specific topic or command\n]");
	}
	else
	{
		BOOL bHelpDisplayed = FALSE;
		for (int i = 0; ; i++)
		{
			if (mHelpTopics[i].szHelpTopic == NULL)
			{
				break;
			}

			if((mHelpTopics[i].szHelpTopic == NULL) || (mHelpTopics[i].szHelpText == NULL))
				continue;

			if (_strcmpi(mHelpTopics[i].szHelpTopic, szCommand) == 0)
			{
				DBGConsole_Msg(0, "[YHelp text for \"%s\":]", mHelpTopics[i].szHelpTopic);

				DBGConsole_Msg(0, "[C%s\n]", mHelpTopics[i].szHelpText);

				bHelpDisplayed = TRUE;
				break;
			}
		}
		if(!bHelpDisplayed)
		{
			for (int i = 0; ; i++)
			{
				if (mDebugCommands[i].szCommand == NULL)
				{
					break;
				}

				if(mDebugCommands[i].szHelpText == NULL)
					continue;

				if (_strcmpi(mDebugCommands[i].szCommand, szCommand) == 0)
				{
					DBGConsole_Msg(0, "[YHelp text for \"%s\":]", mDebugCommands[i].szCommand);

					DBGConsole_Msg(0, "[C%s\n]", mDebugCommands[i].szHelpText);

					bHelpDisplayed = TRUE;
					break;
				}
			}
		}
		if(!bHelpDisplayed)
		{
			DBGConsole_Msg(0, "[YNo help available on \"%s\"]", szCommand);
		}
	}
}


//*****************************************************************************
//
//*****************************************************************************
void IDebugConsole::CommandGo()
{
	CHAR szReason[300+1];

	if (!CPU_StartThread(szReason, 300))
	{
		DBGConsole_Msg(0, "[YGo: %s]", szReason);
	}
}

//*****************************************************************************
//
//*****************************************************************************
void IDebugConsole::CommandStop()
{
	CPU_StopThread();
}




//*****************************************************************************
//
//*****************************************************************************
void IDebugConsole::CommandClose()
{
	EnableConsole( false );
}

//*****************************************************************************
//
//*****************************************************************************
void IDebugConsole::CommandQuit()
{
	CMainWindow::Get()->SendMessage(WM_CLOSE, 0, 0);
}

//*****************************************************************************
//
//*****************************************************************************
int DBGConsole_GetStringHighlight(char c)
{
	switch(c)
	{
		case 'r': return FOREGROUND_RED;
		case 'g': return FOREGROUND_GREEN;
		case 'b': return FOREGROUND_BLUE;
		case 'c': return FOREGROUND_GREEN|FOREGROUND_BLUE;
		case 'm': return FOREGROUND_RED|FOREGROUND_BLUE;
		case 'y': return FOREGROUND_RED|FOREGROUND_GREEN;
		case 'w': return sc_wAttrWhite;
		case 'R': return FOREGROUND_RED|FOREGROUND_INTENSITY;
		case 'G': return FOREGROUND_GREEN|FOREGROUND_INTENSITY;
		case 'B': return FOREGROUND_BLUE|FOREGROUND_INTENSITY;
		case 'C': return FOREGROUND_GREEN|FOREGROUND_BLUE|FOREGROUND_INTENSITY;
		case 'M': return FOREGROUND_RED|FOREGROUND_BLUE|FOREGROUND_INTENSITY;
		case 'Y': return FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_INTENSITY;
		case 'W': return sc_wAttrBoldWhite;
		default: return -1;
	}
}

//*****************************************************************************
//
//*****************************************************************************
void IDebugConsole::ParseStringHighlights(char * szString, WORD * arrwAttributes, WORD wAttr)
{
	WORD wCurrAttribute = wAttr;
	int iIn, iOut;
	int nMax = lstrlen(szString);

	for (iIn = 0, iOut = 0; iIn < nMax; iIn++)
	{
		if (szString[iIn] == '[')
		{
			int highlight = DBGConsole_GetStringHighlight(szString[iIn+1]);
			if(highlight != -1)
			{
				wCurrAttribute = (WORD)highlight;
			}
			else
			{
				switch (szString[iIn+1])
				{
				case '[':
				case ']':
					szString[iOut] = szString[iIn+1];
					arrwAttributes[iOut] = wCurrAttribute;

					iOut++;
					break;
				}
			}

			// Skip colour character
			iIn++;
		}
		else if (szString[iIn] == ']')
		{
			wCurrAttribute = wAttr;
		}
		else
		{
			szString[iOut] = szString[iIn];
			arrwAttributes[iOut] = wCurrAttribute;

			iOut++;
		}
	}
	szString[iOut] = '\0';

}

//*****************************************************************************
//
//*****************************************************************************
void DAEDALUS_VARARG_CALL_TYPE IDebugConsole::Stats( StatType stat, const char * szFormat, ...)
{

    DWORD cWritten;
    BOOL bSuccess;
    COORD coord;
	WORD wNumToWrite;
    va_list va;
	char szBuffer[2048+1];

	// Return if we don't have a console!
	if (IsVisible() == false)
		return;


    coord.X = 55;
    coord.Y = (SHORT)(mPaneInfo[ DBGCON_MID_PANE ].nTopRow + 1 + (u32)stat );

	WORD wMaxWidth = 80 - coord.X;

	// Format the output
	va_start(va, szFormat);
	// Don't use wvsprintf as it doesn't handle floats!
	vsprintf(szBuffer, szFormat, va);
	va_end(va);


	wNumToWrite = lstrlen(szBuffer);
	if (wNumToWrite > wMaxWidth)
		wNumToWrite = wMaxWidth;

    bSuccess = WriteConsoleOutputCharacter(
        mhStdOut,              // screen buffer handle
        szBuffer,								// pointer to source string
        wNumToWrite,							// length of string
        coord,									// first cell to write to
        &cWritten);								// actual number written
    if (!bSuccess)
        return;

	// Write a string of colors to a screen buffer.
	bSuccess = FillConsoleOutputAttribute(
		mhStdOut,
		sc_wAttrWhite,
		wMaxWidth,		// Clear to end
		coord,
		&cWritten);
    if (!bSuccess)
        return;

	// Clear the end of the buffer
	if (wNumToWrite < wMaxWidth)
	{
		coord.X += wNumToWrite;
		FillConsoleOutputCharacter(mhStdOut,
			' ', wMaxWidth - wNumToWrite, coord, &cWritten);
	}

	return;


}

//*****************************************************************************
//
//*****************************************************************************
DWORD g_nTabSpaces = 4;

const char * DBGConsole_ParseTabs(const char * psz, LPWORD pwAttributes)
{
	// I made this a static buffer as the calls to new/delete are quite expensive when tons of messages are flying past
	static char pszOutput[ 2048 ];

	int len = strlen(psz);
	int tabCount = 0;

	for(int i = 0; i < len; i++)
	{
		if(psz[i] == '\t')
			tabCount++;
	}

	int tabbedLen = len + (tabCount * (g_nTabSpaces - 1));

	if ( tabbedLen > ARRAYSIZE( pszOutput ) )
	{
		DAEDALUS_ERROR( "Tabbed output is too long" );
		strncpy( pszOutput, psz, 2048 );
		pszOutput[ 2048-1 ] = 0;
		return pszOutput;
	}

	if(tabCount > 0)
	{
		LPWORD pwOutAttributes = new WORD[tabbedLen];

		int iOut = 0;

		for(int i = 0; i < len; i++)
		{
			if(psz[i] == '\t')
			{
				int tabsToAdd = g_nTabSpaces - (iOut % g_nTabSpaces);
				memset(pszOutput + iOut, ' ', tabsToAdd);
				for(int iTab = 0; iTab < tabsToAdd; iTab++)
				{
					pwOutAttributes[iOut + iTab] = pwAttributes[iTab];
				}
				iOut += tabsToAdd;
			}
			else
			{
				pszOutput[iOut] = psz[i];
				pwOutAttributes[iOut] = pwAttributes[i];
				iOut++;
			}
		}

		pszOutput[iOut] = 0;

		memcpy(pwAttributes, pwOutAttributes, tabbedLen * sizeof(WORD));

		delete [] pwOutAttributes;
	}
	else
	{
		strcpy(pszOutput, psz);
	}

	return pszOutput;
}

//*****************************************************************************
//
//*****************************************************************************
void DAEDALUS_VARARG_CALL_TYPE IDebugConsole::Msg(u32 type, const char * szFormat, ...)
{
    va_list va;

	// Return if we don't have a console!
	if (IsVisible() == false)
		return;

	if (szFormat == NULL)
		return;

	char szBuffer[65536+1];
	char * pszBuffer = szBuffer;

	// Parse the buffer:
	try {
		// Format the output
		va_start(va, szFormat);
		// Don't use wvsprintf as it doesn't handle floats!
		vsprintf(pszBuffer, szFormat, va);
		va_end(va);
	}
	catch (...)
	{
		// Ignore g_DaedalusConfig.TrapExceptions
		return;
	}

	int nOutputLength = lstrlen(pszBuffer);

	WORD arrwAttributes[2048];
	WORD wCurrAttribute = sc_wAttrWhite;

	char cLastNewLine = '\0';
	int iOut = 0;

	for(int i = 0; i <= nOutputLength; i++)
	{
		char c = pszBuffer[i];

		//
		// Wrap at the end of a line
		//
		if ( iOut >= 80 )
		{
			i--;
			c = '\n';
		}

		if(
			((c == '\n') && (cLastNewLine == '\r')) ||
			((c == '\r') && (cLastNewLine == '\n'))
			)
		{
			cLastNewLine = 0;
			//pszLineBeginning++;
		}
		else if((c == '\n') || (c == '\r') || (c == '\0'))
		{
			pszBuffer[iOut] = '\0';
			// ParseTabs return a newed string. It never returns the parameter even if there are no tabs.
			char * pszTabParsed = (char *)DBGConsole_ParseTabs(pszBuffer, arrwAttributes);
			mPaneOutput.InsertLine( pszTabParsed, arrwAttributes );		// Insert the line into the buffer
			iOut = 0;

			cLastNewLine = c;
			//pszLineBeginning = pszBuffer + i + 1;
		}
		else if (c == '[')
		{
			int highlight = DBGConsole_GetStringHighlight(pszBuffer[i+1]);
			if(highlight != -1)
			{
				wCurrAttribute = (WORD)highlight;
			}
			else
			{
				switch (pszBuffer[i+1])
				{
				case '[':
				case ']':
					pszBuffer[iOut] = pszBuffer[i+1];
					arrwAttributes[iOut] = wCurrAttribute;

					iOut++;
					break;
				}
			}

			// Skip colour character
			i++;
		}
		else if (c == ']')
		{
			wCurrAttribute = sc_wAttrWhite;
		}
		else
		{
			pszBuffer[iOut] = c;
			arrwAttributes[iOut] = wCurrAttribute;

			iOut++;
		}
	}

	mPaneOutput.SetOffset( 0 );
	mPaneOutput.Display( mhStdOut );
}


//*****************************************************************************
//
//*****************************************************************************
void	IDebugConsole::MsgOverwriteStart()
{
	// Return if we don't have a console!
	if (!mPaneOutput.IsValid())
		return;

	mPaneOutput.OverwriteLineStart();
}

//*****************************************************************************
//
//*****************************************************************************
void	IDebugConsole::MsgOverwriteEnd()
{
	mPaneOutput.OverwriteLineEnd();
}

//*****************************************************************************
// Used to overwrite previous lines - like for doing % complete indicators
//*****************************************************************************
void DAEDALUS_VARARG_CALL_TYPE IDebugConsole::MsgOverwrite(u32 type, const char * szFormat, ...)
{
    va_list va;
	char szBuffer[2048+1];

	WORD arrwAttributes[2048];

	// Return if we don't have a console!
	if (IsVisible() == false)
		return;

	//
	// Parse the buffer, format the output
	//
	va_start(va, szFormat);
	vsprintf(szBuffer, szFormat, va);			// Don't use wvsprintf as it doesn't handle floats!
	va_end(va);

	// Scan through szBuffer and set up attributes
	ParseStringHighlights(szBuffer, arrwAttributes, sc_wAttrWhite);

	// Write the output to the buffer
	mPaneOutput.OverwriteLine( szBuffer, arrwAttributes );

	mPaneOutput.SetOffset( 0 );
	mPaneOutput.Display( mhStdOut );
}


//*****************************************************************************
// Create a console buffer of the specified width and height
//*****************************************************************************
HANDLE	IDebugConsole::CreateBuffer( u32 width, u32 height ) const
{
	HANDLE hBuffer;
	COORD size;

	size.X = s16( width );
	size.Y = s16( height );

	// Create a new screen buffer to write to.
	hBuffer = CreateConsoleScreenBuffer(
									   GENERIC_READ |           // read-write access
									   GENERIC_WRITE,
									   0,                       // not shared
									   NULL,                    // no security attributes
									   CONSOLE_TEXTMODE_BUFFER, // must be TEXTMODE
									   NULL);                   // reserved; must be NULL
	if (hBuffer == INVALID_HANDLE_VALUE)
	{
		DAEDALUS_ERROR( "Couldn't create the output buffer" );
		return hBuffer;
	}

	// Resize to some reasonable size
	if ( !SetConsoleScreenBufferSize( hBuffer, size ) )
	{
		DAEDALUS_ERROR( "Couldn't size the output buffer" );
		// Still return the handle
		return hBuffer;
	}

	return hBuffer;
}

//*****************************************************************************
// Assumes same-size buffers
//*****************************************************************************
void	IDebugConsole::CopyBuffer( HANDLE hSource, HANDLE hDest )
{
	CONSOLE_SCREEN_BUFFER_INFO csbiInfo;
	SMALL_RECT srctReadRect;
	SMALL_RECT srctWriteRect;
	CHAR_INFO chiBuffer[ 80 * 60 ];
	COORD coordBufSize;
	COORD coordBufCoord;
	BOOL bSuccess;

	s16 width;
	s16 height;

	bSuccess = GetConsoleScreenBufferInfo( hSource, &csbiInfo );
	if (!bSuccess)
		return;

	width = csbiInfo.dwSize.X;
	height = csbiInfo.dwSize.Y;

	// The temporary buffer size is 60 rows x 80 columns - limit the copy to this
	if ( width > 80 )		width = 80;
	if ( height > 60 )		height = 60;

	// Set the source rectangle
	srctReadRect.Left = 0;
	srctReadRect.Top  = 0;
	srctReadRect.Right = 0 + width - 1;
	srctReadRect.Bottom = 0 + height - 1;

	// Set the destination rectangle.
	srctWriteRect.Left = 0;
	srctWriteRect.Top = 0;
	srctWriteRect.Right = 0 + width - 1;
	srctWriteRect.Bottom = 0 + height - 1;

	coordBufCoord.X = 0;
	coordBufCoord.Y = 0;
	coordBufSize.X = width;
	coordBufSize.Y = height;

	// Copy the block from the screen buffer to the temp. buffer.
	bSuccess = ReadConsoleOutput( hSource, chiBuffer, coordBufSize, coordBufCoord, &srctReadRect);
	if (!bSuccess)
		return;

	// Copy from the temporary buffer to the new screen buffer.
	bSuccess = WriteConsoleOutput( hDest, chiBuffer, coordBufSize, coordBufCoord, &srctWriteRect);
	if (!bSuccess)
		return;

	// Done
}


#include "DBGConsoleCmd.inl"
