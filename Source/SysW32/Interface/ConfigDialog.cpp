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


#include "stdafx.h"
#include "Resources/resource.h"
#include "ConfigDialog.h"
#include "PageDialog.h"

#include "Core/CPU.h"
#include "Debug/DBGConsole.h"
#include "Debug/Dump.h"
#include "Plugins/GraphicsPlugin.h"
#include "SysW32/Utility/ResourceString.h"

#include "SettingsPage.h"
#include "DirectoriesPage.h"
#include "PluginsPage.h"
#include "DebugPage.h"




DaedalusConfig g_CurrentConfig;

//*****************************************************************************
//
//*****************************************************************************
LRESULT	CConfigDialog::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	g_CurrentConfig = g_DaedalusConfig;

	// Initialise the list of pages
	FillPageList();

	ShowPage( PAGE_SETTINGS );

	::SetFocus( GetDlgItem(IDC_PAGE_LIST) );

	// We set the foucus, return false
	return FALSE;
}

//*****************************************************************************
//
//*****************************************************************************
void CConfigDialog::FillPageList()
{
	LONG i;
	HWND hWndList;
	LONG nSelection;
	LONG nIndex;

	hWndList = GetDlgItem(IDC_PAGE_LIST);

	ListBox_ResetContent(hWndList);

	nSelection = 0;
	PageType nCurrentPage = PAGE_SETTINGS;
	if ( m_pPageDialog )
	{
		nCurrentPage = m_pPageDialog->GetPageType();
	}

	for (i = 0; i < NUM_PAGE_TYPES; i++)
	{
		PageType type = static_cast< PageType >( i );

		nIndex = ListBox_InsertString(hWndList, -1, GetPageName( type ) );

		if (nIndex != LB_ERR && nIndex != LB_ERRSPACE)
		{
			// Record the index if this is the current selection
			if ( nCurrentPage == i )
			{
				nSelection = nIndex;
			}

			// Set item data
			ListBox_SetItemData(hWndList, nIndex, i);
		}
	}

	ListBox_SetCurSel(hWndList, nSelection);

}

//*****************************************************************************
//
//*****************************************************************************
void	CConfigDialog::KillPage( )
{
	if ( m_pPageDialog != NULL )
	{
		m_pPageDialog->DestroyPage();
		m_pPageDialog = NULL;
	}
}

//*****************************************************************************
//
//*****************************************************************************
void	CConfigDialog::ShowPage( PageType page )
{
	KillPage();

	m_pPageDialog = CreatePage( *this, 130, 0, page );
}


//*****************************************************************************
//
//*****************************************************************************
CPageDialog * CConfigDialog::CreatePage( HWND hWndParent, LONG x, LONG y, PageType page_type )
{
	RECT rect = { x, y, 100, 100 };

	CPageDialog * p_new = NULL;

	switch ( page_type )
	{
		case PAGE_SETTINGS:		p_new = new CSettingsPage(); break;
		case PAGE_DIRECTORIES:	p_new = new CDirectoriesPage(); break;
		case PAGE_PLUGINS:		p_new = new CPluginsPage(); break;
		case PAGE_DEBUG:		p_new = new CDebugPage(); break;
	}

	if ( p_new )
	{
		p_new->CreatePage( hWndParent, rect );
	}

	return p_new;
}



//*****************************************************************************
// OnOk handling
//*****************************************************************************
LRESULT	CConfigDialog::OnOk( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled )
{
	// We have to do this so that the current settings are commited!
	KillPage();

	if (_strcmpi(g_DaedalusConfig.szGfxPluginFileName, g_CurrentConfig.szGfxPluginFileName) != 0)
	{
		DBGConsole_Msg(0, "Graphics plugin [C%s] selected", g_CurrentConfig.szGfxPluginFileName);

		if (CPU_IsRunning())
		{
			MessageBox( CResourceString(IDS_STOPSTARTCPU), g_szDaedalusName, MB_OK );
		}
	}

	bool bCurrDebugShow = g_DaedalusConfig.ShowDebug;


	// Copy across config to active config
	g_DaedalusConfig = g_CurrentConfig;

	if ( bCurrDebugShow != g_DaedalusConfig.ShowDebug )
	{
		CDebugConsole::Get()->EnableConsole( g_DaedalusConfig.ShowDebug );
	}

	EndDialog( IDOK );
	return TRUE;
}

//*****************************************************************************
// OnCancel handling
//*****************************************************************************
LRESULT	CConfigDialog::OnCancel( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled )
{
	// Although there isn't a cancel button, this will be called when the close button is hit
	EndDialog( IDCANCEL );
	return TRUE;
}

//*****************************************************************************
// OnPageListSelChange
//*****************************************************************************
LRESULT	CConfigDialog::OnPageListSelChange( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled )
{
	LONG nIndex;
	LONG nSelection;

	nSelection = ListBox_GetCurSel(GetDlgItem(IDC_PAGE_LIST));
	nIndex = ListBox_GetItemData(GetDlgItem(IDC_PAGE_LIST), nSelection);
	if (nIndex != LB_ERR && nIndex < NUM_PAGE_TYPES )
	{
		ShowPage( (PageType)nIndex );
	}

	return TRUE;
}
