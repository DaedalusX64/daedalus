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


#ifndef SYSW32_INTERFACE_SETTINGSPAGE_H_
#define SYSW32_INTERFACE_SETTINGSPAGE_H_

#include "Config/ConfigOptions.h"
#include "SysW32/Interface/CheckBox.h"

class CSettingsPage : public CPageDialog, public CDialogImpl< CSettingsPage >
{
	protected:
		friend class CConfigDialog;			// Only CConfigDialog can create!

		CSettingsPage( ) {}

	public:
		virtual ~CSettingsPage() {}


		BEGIN_MSG_MAP( CSettingsPage )
			MESSAGE_HANDLER( WM_INITDIALOG, OnInitDialog )
			MESSAGE_HANDLER( WM_DESTROY, OnDestroy )
		END_MSG_MAP()

		enum { IDD = IDD_PAGE_SETTINGS };

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


		CConfigDialog::PageType GetPageType() const { return CConfigDialog::PAGE_SETTINGS; }

		LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
		{
			CheckBox_SetCheck( GetDlgItem( IDC_RUN_ROMS_AUTO_CHECK ),  g_CurrentConfig.RunAutomatically   ? BST_CHECKED : BST_UNCHECKED );
			CheckBox_SetCheck( GetDlgItem( IDC_RECURSIVE_SCAN_CHECK ), g_CurrentConfig.RecurseRomDirectory ? BST_CHECKED : BST_UNCHECKED );

			return TRUE;
		}

		LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
		{
			// Set parameters (or set on change??)
			g_CurrentConfig.RunAutomatically = CheckBox_GetCheck( GetDlgItem( IDC_RUN_ROMS_AUTO_CHECK ) ) == BST_CHECKED;
			g_CurrentConfig.RecurseRomDirectory = CheckBox_GetCheck( GetDlgItem( IDC_RECURSIVE_SCAN_CHECK ) ) == BST_CHECKED;

			return TRUE;
		}

};

#endif // SYSW32_INTERFACE_SETTINGSPAGE_H_
