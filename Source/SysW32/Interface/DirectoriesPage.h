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


#ifndef __DIRECTORIESPAGEDIALOG_H__
#define __DIRECTORIESPAGEDIALOG_H__

#include <shlobj.h>
#include "Debug/Dump.h"
#include "SysW32/Utility/ResourceString.h"

class CDirectoriesPage : public CPageDialog, public CDialogImpl< CDirectoriesPage >
{
	protected:
		friend class CConfigDialog;			// Only CConfigDialog can create!

		CDirectoriesPage( )
		{
		}

	public:
		virtual ~CDirectoriesPage()
		{
		}

		BEGIN_MSG_MAP( CDirectoriesPage )
			MESSAGE_HANDLER( WM_INITDIALOG, OnInitDialog )
			MESSAGE_HANDLER( WM_DESTROY, OnDestroy )
			COMMAND_ID_HANDLER( IDC_ADD_ROMSDIR_BUTTON, OnAddRomsDir )
			COMMAND_ID_HANDLER( IDC_REMOVE_ROMSDIR_BUTTON, OnRemoveRomsDir )
			COMMAND_ID_HANDLER( IDC_SELECT_SAVE_BUTTON, OnSelectSaveDir )

		END_MSG_MAP()

		enum { IDD = IDD_PAGE_DIRECTORIES };

		void CreatePage( HWND hWndParent, RECT & rect )
		{
			Create( hWndParent/*, rect*/ );
			SetWindowPos( NULL, rect.left, rect.top, 0, 0, SWP_NOSIZE|SWP_NOZORDER|SWP_SHOWWINDOW);
			//ShowWindow( SW_SHOWNORMAL );
			UpdateWindow(  );
			InvalidateRect( NULL, TRUE );
		}

		void DestroyPage()
		{
			if ( IsWindow() )
			{
				// Ok this window before destroying?
				SendMessage( WM_COMMAND, MAKELONG(IDOK,0), 0);
				DestroyWindow();
			}
			delete this;
		}

		CConfigDialog::PageType GetPageType() const { return CConfigDialog::PAGE_DIRECTORIES; }

		LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
		{
			// Init parameters
			RefreshRomDirList();

			::Edit_SetText( GetDlgItem( IDC_SAVE_DIR_EDIT ), g_CurrentConfig.szSaveDir );

			CHAR szDumpDir[ MAX_PATH + 1 ];
			Dump_GetDumpDirectory( szDumpDir, "" );
			::Edit_SetText( GetDlgItem( IDC_DUMPS_DIR_EDIT ), szDumpDir );
			return TRUE;
		}

		LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
		{
			// Set parameters (or set on change??)
			return TRUE;
		}

		LRESULT OnAddRomsDir( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled )
		{
			if ( g_CurrentConfig.nNumRomsDirs < DaedalusConfig::MAX_ROMS_DIRS )
			{
				if ( SelectDir( g_CurrentConfig.szRomsDirs[ g_CurrentConfig.nNumRomsDirs ], CResourceString(IDS_SELECTFOLDER) ) )
				{
					g_CurrentConfig.nNumRomsDirs++;

					RefreshRomDirList();

				}
			}
			return TRUE;
		}

		LRESULT OnRemoveRomsDir( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled )
		{
			HWND hWndRomsList = GetDlgItem( IDC_ROM_DIRECTORY_LIST );

			s32 index = ListBox_GetCurSel( hWndRomsList );

			if ( index != LB_ERR && g_CurrentConfig.nNumRomsDirs > 0 )
			{
				u32 data = ListBox_GetItemData( hWndRomsList, index );

				for ( u32 i = data; i+1 < g_CurrentConfig.nNumRomsDirs; i++ )
				{
					strcpy( g_CurrentConfig.szRomsDirs[ i ], g_CurrentConfig.szRomsDirs[ i + 1 ] );
				}

				g_CurrentConfig.nNumRomsDirs--;

				RefreshRomDirList();
			}


			return TRUE;
		}

		LRESULT OnSelectSaveDir( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled )
		{
			SelectDir( g_CurrentConfig.szSaveDir, CResourceString(IDS_SELECTSAVEFOLDER) );
			return TRUE;
		}

	protected:
		void	RefreshRomDirList()
		{
			HWND hWndRomsList = GetDlgItem( IDC_ROM_DIRECTORY_LIST );
			ListBox_ResetContent( hWndRomsList );

			for ( u32 rd = 0; rd < g_CurrentConfig.nNumRomsDirs; rd++ )
			{
				u32 index = ListBox_InsertString( hWndRomsList, -1, g_CurrentConfig.szRomsDirs[ rd ] );
				if ( index != LB_ERR )
				{
					ListBox_SetItemData( hWndRomsList, index, rd );
				}
			}
		}

		BOOL SelectDir( CHAR * pszDirectory, const CHAR * pszPrompt )
		{
			BROWSEINFO bi;
			LPITEMIDLIST lpidl;
			TCHAR szDisplayName[MAX_PATH+1];

			bi.hwndOwner = *this;
			bi.pidlRoot = NULL;
			bi.pszDisplayName = szDisplayName;
			bi.lpszTitle = pszPrompt;
			bi.ulFlags = BIF_RETURNONLYFSDIRS;
			bi.lpfn = NULL;
			bi.lParam = NULL;
			bi.iImage = 0;

			lpidl = SHBrowseForFolder(&bi);
			if (lpidl)
			{
				return SHGetPathFromIDList(lpidl, pszDirectory);
			}

			// None selected
			return FALSE;
		}

};

#endif // __DIRECTORIESPAGEDIALOG_H__
