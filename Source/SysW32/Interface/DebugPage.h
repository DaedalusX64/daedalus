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


#ifndef __DEBUGPAGEDIALOG_H__
#define __DEBUGPAGEDIALOG_H__

#include "Debug/DBGConsole.h"
#include "SysW32/Interface/CheckBox.h"

class CDebugPage : public CPageDialog, public CDialogImpl< CDebugPage >
{
	protected:
		friend class CConfigDialog;			// Only CConfigDialog can create!

		CDebugPage( ) {}

	public:
		virtual ~CDebugPage() {}

		BEGIN_MSG_MAP( CDebugPage )
			MESSAGE_HANDLER( WM_INITDIALOG, OnInitDialog )
			MESSAGE_HANDLER( WM_DESTROY, OnDestroy )
		END_MSG_MAP()

		enum { IDD = IDD_PAGE_DEBUG };

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


		CConfigDialog::PageType GetPageType() const { return CConfigDialog::PAGE_DEBUG; }

		LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
		{
			CheckBox_SetCheck( GetDlgItem( IDC_DEBUG_CONSOLE_CHECK ),   g_CurrentConfig.ShowDebug        ? BST_CHECKED : BST_UNCHECKED );
			CheckBox_SetCheck( GetDlgItem( IDC_TRAP_EXCEPTIONS_CHECK ), g_CurrentConfig.TrapExceptions   ? BST_CHECKED : BST_UNCHECKED );
			CheckBox_SetCheck( GetDlgItem( IDC_WARN_MEM_ERRORS_CHECK ), g_CurrentConfig.WarnMemoryErrors ? BST_CHECKED : BST_UNCHECKED );
			return TRUE;
		}

		LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
		{
			// Set parameters (or set on change??)
			g_CurrentConfig.ShowDebug        = CheckBox_GetCheck( GetDlgItem( IDC_DEBUG_CONSOLE_CHECK ) )   == BST_CHECKED;
			g_CurrentConfig.TrapExceptions   = CheckBox_GetCheck( GetDlgItem( IDC_TRAP_EXCEPTIONS_CHECK ) ) == BST_CHECKED;
			g_CurrentConfig.WarnMemoryErrors = CheckBox_GetCheck( GetDlgItem( IDC_WARN_MEM_ERRORS_CHECK ) ) == BST_CHECKED;
			return TRUE;
		}
};

#endif // __DEBUGPAGEDIALOG_H__
