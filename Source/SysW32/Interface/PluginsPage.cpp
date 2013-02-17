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

#include "PageDialog.h"
#include "PluginsPage.h"
#include "SysW32/Interface/CheckBox.h"

#include "Core/CPU.h"

#include "SysW32/Plugins/GraphicsPluginW32.h"

#include "Utility/IO.h"
#include "SysW32/Utility/ResourceString.h"
#include "SysW32/Utility/ConfigHandler.h"

#include "ConfigOptions.h"

//*****************************************************************************
//
//*****************************************************************************
LRESULT CPluginsPage::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	// Initialise the
	if (InitPluginList())
		FillPluginCombo();

	if (lstrlen(g_CurrentConfig.szGfxPluginFileName) == 0)
	{
		EnableControls(FALSE);
	}
	else
	{
		EnableControls(TRUE);
	}


	::SetFocus(GetDlgItem(IDC_GRAPHICS_PLUGIN_COMBO));


	// We set the foucus, return false
	return FALSE;
}

//*****************************************************************************
//
//*****************************************************************************
LRESULT CPluginsPage::OnDestroy( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
	// Free any plugins that were found
	FreePluginList();
	return TRUE;
}

//*****************************************************************************
//
//*****************************************************************************
LRESULT CPluginsPage::OnGraphicsAbout( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled )
{
	LONG nSelection = ComboBox_GetCurSel(GetDlgItem(IDC_GRAPHICS_PLUGIN_COMBO));
	LONG nIndex = ComboBox_GetItemData(GetDlgItem(IDC_GRAPHICS_PLUGIN_COMBO), nSelection);
	if (nIndex != CB_ERR && u32( nIndex ) < m_GraphicsPlugins.size())
	{
		if (m_GraphicsPlugins[nIndex].pPlugin != NULL)
		{
			m_GraphicsPlugins[nIndex].pPlugin->DllAbout( *this );
		}
	}
	return TRUE;
}

//*****************************************************************************
//
//*****************************************************************************
LRESULT CPluginsPage::OnGraphicsConfig( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled )
{
	/*
	if ( gGraphicsPlugin == NULL )
	{
		MessageBox(CResourceString(IDS_CONFIGGFX), g_szDaedalusName, MB_OK);
	}
	else
	{
		nSelection = ComboBox_GetCurSel(GetDlgItem(IDC_GRAPHICS_PLUGIN_COMBO));
		nIndex = ComboBox_GetItemData(GetDlgItem(IDC_GRAPHICS_PLUGIN_COMBO), nSelection);
		if (nIndex != CB_ERR && nIndex < m_GraphicsPlugins.size())
		{
			if (m_GraphicsPlugins[nIndex].pPlugin != NULL)
			{
				m_GraphicsPlugins[nIndex].pPlugin->DllConfig( *this );
			}
		}
	}
	*/
	return TRUE;
}

//*****************************************************************************
//
//*****************************************************************************
LRESULT CPluginsPage::OnOk( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled )
{
	LONG nSelection = ComboBox_GetCurSel(GetDlgItem(IDC_GRAPHICS_PLUGIN_COMBO));

	LONG nIndex = ComboBox_GetItemData(GetDlgItem(IDC_GRAPHICS_PLUGIN_COMBO), nSelection);
	if (nIndex != CB_ERR && u32( nIndex ) < m_GraphicsPlugins.size())
	{
		// Update main plugin!
		// Write to config!!!
		// Reinitialise!
		TCHAR szNewFileName[MAX_PATH+1];

		lstrcpyn(szNewFileName, m_GraphicsPlugins[nIndex].szFileName, MAX_PATH);

		{
			// NULL - Write to registry
			ConfigHandler * pConfig = new ConfigHandler("Main");

			if (pConfig != NULL)
			{
				pConfig->WriteString("GraphicsPlugin", szNewFileName);
				delete pConfig;
			}
		}

		lstrcpyn(g_CurrentConfig.szGfxPluginFileName, szNewFileName, MAX_PATH);

		// Set to null so that it is not deleted when we clean up
		//m_GraphicsPlugins[nSelection].pPlugin = NULL;
	}

	return TRUE;
}

//*****************************************************************************
//
//*****************************************************************************
LRESULT CPluginsPage::OnCancel( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled )
{
	return TRUE;
}


//*****************************************************************************
// Free all plugins in list
//*****************************************************************************
void CPluginsPage::FreePluginList()
{
	LONG i;
	LONG nNumPlugins;

	nNumPlugins = m_GraphicsPlugins.size();
	for (i = 0; i < nNumPlugins; i++)
	{
		if (m_GraphicsPlugins[i].pPlugin != NULL &&
			!m_GraphicsPlugins[i].bCurrentlyInUse)
		{
			delete m_GraphicsPlugins[i].pPlugin;
		}
	}
	m_GraphicsPlugins.clear();
}

//*****************************************************************************
// Scan
//*****************************************************************************
BOOL CPluginsPage::InitPluginList()
{
	HANDLE hFind;
	WIN32_FIND_DATA fd;
	TCHAR szFileSpec[MAX_PATH+1];
	GraphicsPluginInfo api;

	// Delete existing plugins
	FreePluginList();

	lstrcpyn(szFileSpec, gDaedalusExePath, MAX_PATH);
	IO::Path::Append(szFileSpec, TEXT("Plugins"));
	IO::Path::Append(szFileSpec, TEXT("*.dll"));


	hFind = FindFirstFile(szFileSpec, &fd);
	if (hFind == INVALID_HANDLE_VALUE)
		return FALSE;

	do
	{
		lstrcpyn(api.szFileName, szFileSpec, MAX_PATH);
		IO::Path::RemoveFileSpec(api.szFileName);
		IO::Path::Append(api.szFileName, fd.cFileName);

		/*if (_strcmpi(api.szFileName, g_CurrentConfig.szGfxPluginFileName) == 0)
		{
			api.pPlugin = g_pAiPlugin;
			api.bCurrentlyInUse = TRUE;
		}
		else*/
		{
			api.pPlugin = CGraphicsPluginDll::Create( api.szFileName );
			api.bCurrentlyInUse = FALSE;
		}

		if (api.pPlugin == NULL)
			continue;

		m_GraphicsPlugins.push_back(api);
	}
	while (FindNextFile(hFind, &fd));

	FindClose(hFind);

	return TRUE;
}


//*****************************************************************************
//
//*****************************************************************************
void CPluginsPage::FillPluginCombo()
{
	LONG i;
	LONG nNumPlugins;
	HWND hWndCombo;
	CHAR szName[100];
	LONG nIndex;
	LONG nSelection;

	hWndCombo = GetDlgItem(IDC_GRAPHICS_PLUGIN_COMBO);
	nNumPlugins = m_GraphicsPlugins.size();

	ComboBox_ResetContent(hWndCombo);

	nSelection = -1;

	for (i = 0; i < nNumPlugins; i++)
	{
		if (m_GraphicsPlugins[i].pPlugin == NULL)
			continue;

		m_GraphicsPlugins[i].pPlugin->GetPluginName(szName);

		nIndex = ComboBox_InsertString(hWndCombo, -1, szName);

		if (nIndex != CB_ERR && nIndex != CB_ERRSPACE)
		{
			// Record the index if this is the current selection
			if (_strcmpi(m_GraphicsPlugins[i].szFileName,
				g_CurrentConfig.szGfxPluginFileName) == 0)
			{
				nSelection = nIndex;
			}

			// Set item data
			ComboBox_SetItemData(hWndCombo, nIndex, i);
		}
	}

	if (nSelection == -1)
		nSelection = 0;

	ComboBox_SetCurSel(hWndCombo, nSelection);

}


//*****************************************************************************
//
//*****************************************************************************
void CPluginsPage::EnableControls(BOOL bEnabled)
{
//	::EnableWindow(GetDlgItem(IDC_GRAPHICS_PLUGIN_COMBO), bEnabled);
	::EnableWindow(GetDlgItem(IDC_ABOUT_GRAPHICS_BUTTON), bEnabled);
	//::EnableWindow(GetDlgItem(IDC_CONFIG_BUTTON), bEnabled);
}

