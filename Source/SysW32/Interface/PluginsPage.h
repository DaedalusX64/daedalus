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


#ifndef __PLUGINSPAGEDIALOG_H__
#define __PLUGINSPAGEDIALOG_H__

class CGraphicsPluginDll;
class CAudioPluginDll;

class CPluginsPage : public CPageDialog, public CDialogImpl< CPluginsPage >
{
	protected:
		friend class CConfigDialog;			// Only CConfigDialog can create!

		CPluginsPage( ) {}

	public:
		virtual ~CPluginsPage() {}

		BEGIN_MSG_MAP( CPluginsPage )
			MESSAGE_HANDLER( WM_INITDIALOG, OnInitDialog )
			MESSAGE_HANDLER( WM_DESTROY, OnDestroy )
			COMMAND_ID_HANDLER( IDC_ABOUT_GRAPHICS_BUTTON, OnGraphicsAbout )
			//COMMAND_ID_HANDLER( IDC_GRAPHICS_CONFIG_BUTTON, OnGraphicsConfig )
			//COMMAND_ID_HANDLER( IDC_AUDIO_CONFIG_BUTTON, OnAudioConfig )
			COMMAND_ID_HANDLER( IDOK, OnOk )
			COMMAND_ID_HANDLER( IDCANCEL, OnCancel )
		END_MSG_MAP()

		enum { IDD = IDD_PAGE_PLUGINS };

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

		CConfigDialog::PageType GetPageType() const { return CConfigDialog::PAGE_PLUGINS; }

	protected:
		LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
		LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

		LRESULT OnOk( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );
		LRESULT OnCancel( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );
		LRESULT OnGraphicsAbout( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );
		LRESULT OnGraphicsConfig( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );

		typedef struct
		{
			TCHAR	szFileName[MAX_PATH+1];
			CGraphicsPluginDll * pPlugin;
			BOOL bCurrentlyInUse;

		} GraphicsPluginInfo;

		typedef std::vector< GraphicsPluginInfo > GraphicsPluginVector;

		GraphicsPluginVector m_GraphicsPlugins;


		void FreePluginList();
		BOOL InitPluginList();
		void FillPluginCombo();
		void EnableControls(BOOL bEnabled);
};

#endif // __PLUGINSPAGEDIALOG_H__