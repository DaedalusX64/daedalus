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

#pragma once

#ifndef SYSW32_INTERFACE_CONFIGDIALOG_H_
#define SYSW32_INTERFACE_CONFIGDIALOG_H_

//*****************************************************************************
// Include Files
//*****************************************************************************
#include "Resources/resource.h"

//*****************************************************************************
// Types
//*****************************************************************************
class CPageDialog;

//*****************************************************************************
// Class Definitions
//*****************************************************************************
class CConfigDialog : public CDialogImpl< CConfigDialog >
{
	public:
		CConfigDialog() :
			m_pPageDialog( NULL )
		{
		}

		BEGIN_MSG_MAP( CConfigDialog )
			MESSAGE_HANDLER( WM_INITDIALOG, OnInitDialog )
			COMMAND_HANDLER( IDC_PAGE_LIST, LBN_SELCHANGE, OnPageListSelChange )
			COMMAND_ID_HANDLER( IDOK, OnOk )
			COMMAND_ID_HANDLER( IDCANCEL, OnCancel )
		END_MSG_MAP( )

		enum { IDD = IDD_CONFIG };

		enum PageType
		{
			PAGE_SETTINGS = 0,
			PAGE_DIRECTORIES,
			PAGE_PLUGINS,
			PAGE_DEBUG,

			NUM_PAGE_TYPES
		};

	private:
		LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

		LRESULT OnOk( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );
		LRESULT OnCancel( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );

		LRESULT OnPageListSelChange( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );


		void FillPageList( );

		void ShowPage( PageType page );
		void KillPage( );

		CPageDialog * CreatePage( HWND hWndParent, LONG x, LONG y, PageType page_type );

		static const CHAR * GetPageName( PageType page_type )
		{
			switch ( page_type )
			{
				case PAGE_SETTINGS:		return "Settings";
				case PAGE_DIRECTORIES:	return "Directories";
				case PAGE_PLUGINS:		return "Plugins";
				case PAGE_DEBUG:		return "Debug";
			}
			return "";
		}

	private:
		CPageDialog	*	m_pPageDialog;
};



#endif // SYSW32_INTERFACE_CONFIGDIALOG_H_
