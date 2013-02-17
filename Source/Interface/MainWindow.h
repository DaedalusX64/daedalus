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

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef MAINWINDOW_H_
#define MAINWINDOW_H_

#include "Utility/Singleton.h"

#if defined(DAEDALUS_W32) || defined(DAEDALUS_XB)

// Declarations
class CMainWindow : public CSingleton< CMainWindow >
{
	protected:
		HWND	m_hWnd;
		HWND	m_hWndStatus;

	public:
		virtual ~CMainWindow() {}

		virtual HRESULT CreateWnd(INT nWinMode) = 0;

		virtual HWND GetWindow() const { return m_hWnd; }
		virtual HWND GetStatusWindow() const { return m_hWndStatus; }

		virtual void DAEDALUS_VARARG_CALL_TYPE SetStatus(INT nPiece, const char * szFormat, ...) = 0;

		virtual void SelectSaveDir(HWND hWndParent) = 0;

		static void StartEmu() { ::PostMessage(CMainWindow::Get()->GetWindow(), MWM_STARTEMU, 0,0);}
		static void StopEmu() { ::PostMessage(CMainWindow::Get()->GetWindow(), MWM_ENDEMU, 0,0);}

		//
		// Window message handler
		//
		virtual LRESULT WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) = 0;

#ifdef DAEDALUS_XB
		virtual u32		MessageBox( const CHAR * msg, const CHAR * title = "Daedalus", u32 flags = MB_OK ) = 0;
#else
		//
		// Utility functions
		//
		virtual LRESULT SendMessage(UINT msg, WPARAM wParam, LPARAM lParam)
		{
			return ::SendMessage( m_hWnd, msg, wParam, lParam);
		}
		virtual LRESULT PostMessage(UINT msg, WPARAM wParam, LPARAM lParam)
		{
			return ::PostMessage( m_hWnd, msg, wParam, lParam);
		}
		virtual u32 MessageBox( const char * lpText, const char * lpCaption = g_szDaedalusName, UINT uType = MB_OK)
		{
			return ::MessageBox( m_hWnd, lpText, lpCaption, uType );
		}

		enum
		{
			MWM_STARTEMU = WM_USER+1,		// Start emulation	- disable list, grab focus
			MWM_ENDEMU						// Stop emulation - enable list
		};
#endif
};

#endif // DAEDALUS_W32 || DAEDALUS_XB


#endif // MAINWINDOW_H_
