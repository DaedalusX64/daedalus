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

#ifndef __ROMSETTINGSDIALOG_H__
#define __ROMSETTINGSDIALOG_H__

#include "Resources/resource.h"
#include "Core/ROM.h"

class	CRomSettings : public CDialogImpl< CRomSettings >
{
	public:
		CRomSettings( const RomID & id );

		BEGIN_MSG_MAP( CRomSettings )
			MESSAGE_HANDLER( WM_INITDIALOG, OnInitDialog )
			COMMAND_ID_HANDLER( IDOK, OnOk )
			COMMAND_ID_HANDLER( IDCANCEL, OnCancel )
		END_MSG_MAP( )

		enum { IDD = IDD_ROMSETTINGS };

	private:
		LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

		LRESULT OnOk( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );
		LRESULT OnCancel( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );
		LRESULT OnEepromCheck( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );

		enum
		{
			COMMENT_BLANK = 0,
			COMMENT_PLAYABLE = 1,
			COMMENT_ALMOSTPLAYABLE = 2,
			COMMENT_NOTPLAYABLE = 3,
			COMMENT_DOESNTWORK = 4
		};

		void UpdateEntry( );
		LONG GetCommentType(LPCTSTR szComment);
		void FillCommentCombo( const CHAR * comment );

	private:
		RomID			mRomID;
		RomSettings		mRomSettings;

};

#endif
