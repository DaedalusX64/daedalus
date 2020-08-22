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
#include "FileNameHandler.h"

#include "System/IO.h"
#include "Base/Macros.h"

// MODIFIED BY Lkb - 21/jul/2001 - savestate support
FileNameHandler::FileNameHandler(LPCTSTR szSectionName,
								 LPCTSTR szFilter, int nFilterIndex, LPCTSTR szDefExt, LPCTSTR pszDefaultDir)
{
	// Try and get default dir from registry entry...
	lstrcpy(m_szFile, "");
	lstrcpy(m_szFileTitle, "");
	lstrcpy(m_szSectionName, szSectionName);

	if(pszDefaultDir)
		strcpy(m_szCurrentDirectory, pszDefaultDir);
	else
		GetDefaultDirectory(m_szCurrentDirectory);

	ZeroMemory(&m_OFN, sizeof(OPENFILENAME));
	m_OFN.lStructSize = sizeof(OPENFILENAME);

	m_OFN.nFilterIndex = nFilterIndex;
	m_OFN.lpstrFile = m_szFile;
	m_OFN.nMaxFile = 300;
	m_OFN.lpstrFileTitle = m_szFileTitle;
	m_OFN.nMaxFileTitle = 300;
	m_OFN.lpstrInitialDir = m_szCurrentDirectory;
	m_OFN.lpstrFilter = szFilter;
	m_OFN.lpstrDefExt = szDefExt;
	m_OFN.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
	m_OFN.hInstance = g_hInstance;
}

FileNameHandler::~FileNameHandler()
{
	SetDefaultDirectory(m_szCurrentDirectory);
}

// Get the default directory from the system
// registry (if specified, otherwise it creates)
void FileNameHandler::GetDefaultDirectory(LPTSTR szOutBuffer)
{
	HKEY hRegKey;
	DWORD datatype, nResult;

	BOOL bNeedToSetKey = FALSE;

	// Initialize the directory name
	GetModuleDirectory(szOutBuffer);

	IO::Path::Append(szOutBuffer, m_szSectionName);

	// Don't save default path for unknown sections...
	if (lstrlen(m_szSectionName) == 0)
		return;

	// Try and get default dir from registry entry...
	if (RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\Daedalus\\Default Directory\\",
				 0, m_szSectionName,
				 REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS,
				 NULL, &hRegKey, &nResult) != ERROR_SUCCESS)
	{
		// Couldn't open key - return
		return;
	}

	if (nResult == REG_CREATED_NEW_KEY)
	{
		// The key has just been created - no information is currently stored
		//  so we need to set the value
		bNeedToSetKey = TRUE;
	}
	else if (nResult == REG_OPENED_EXISTING_KEY)
	{
		// Key was already in registry - try to read value
		DWORD datasize = ARRAYSIZE(m_szSectionName);
		if (RegQueryValueEx(hRegKey, m_szSectionName, NULL,
			&datatype, (LPBYTE) szOutBuffer, &datasize) != ERROR_SUCCESS)
		{
			bNeedToSetKey = TRUE;
		}
	}
	else
	{
		// We don't know what happened. Just try to set
		bNeedToSetKey = TRUE;
	}

	if (bNeedToSetKey)
	{
		// Set to initial value...
		GetModuleDirectory(szOutBuffer);
		lstrcat(szOutBuffer, m_szSectionName);

		RegSetValueEx(hRegKey, m_szSectionName, 0, REG_SZ,
			(LPBYTE) szOutBuffer, lstrlen(szOutBuffer)+1);
	}

	RegCloseKey(hRegKey);
}

// Set the default directory to the system
// registry (if specified, otherwise it creates)
void FileNameHandler::SetDefaultDirectory(LPCTSTR szDir)
{
	HKEY hRegKey;
	DWORD nResult;

	// Don't save default path for unknown sections...
	if (lstrlen(m_szSectionName) == 0)
		return;

	// Try and get default dir from registry entry...
	if (RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\Daedalus\\Default Directory\\",
				 0, m_szSectionName,
				 REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS,
				 NULL, &hRegKey, &nResult) != ERROR_SUCCESS)
	{
		// Couldn't open key - return
		return;
	}

	RegSetValueEx(hRegKey, m_szSectionName, 0, REG_SZ,
		(LPBYTE)szDir, lstrlen(szDir)+1);

	RegCloseKey(hRegKey);
}

// szOutBuffer must be at least IO::Path::kMaxPathLen in length
// FIXME(strmnnrmn): Pass a PathBuf.
void FileNameHandler::GetModuleDirectory(LPTSTR szOutBuffer)
{
	// Gets the directory that this DLL was loaded from
	GetModuleFileName(g_hInstance, szOutBuffer, IO::Path::kMaxPathLen);

	// Remove trailing slash
	IO::Path::RemoveFileSpec(szOutBuffer);
}

// Retrieves the path from the currently selected file
// szOutBuffer must be at least IO::Path::kMaxPathLen in length
// FIXME(strmnnrmn): Pass a PathBuf.
void FileNameHandler::GetCurrentDirectory(LPTSTR szOutBuffer)
{
	lstrcpyn(szOutBuffer, m_szFile, IO::Path::kMaxPathLen);

	// Remove trailing slash
	IO::Path::RemoveFileSpec(szOutBuffer);
}

// Retrieves the path/name of the currently selected file
// szOutBuffer must be at least IO::Path::kMaxPathLen in length
// FIXME(strmnnrmn): Pass a PathBuf.
void FileNameHandler::GetCurrentFileName(LPTSTR szOutBuffer)
{
	lstrcpyn(szOutBuffer, m_szFile, IO::Path::kMaxPathLen);
}

// Set the current filename
// szOutBuffer must be at least IO::Path::kMaxPathLen in length
// FIXME(strmnnrmn): Pass a PathBuf.
void FileNameHandler::SetFileName(LPCTSTR szNewName)
{
	lstrcpyn(m_szFile, szNewName, IO::Path::kMaxPathLen);
}

// Opens a common file requester and gets filename.
// Returns FALSE is user clicked cancel, TRUE if
// user clicked ok
BOOL FileNameHandler::GetOpenName(HWND hwnd, LPTSTR szOutBuffer)
{
	m_OFN.hwndOwner = hwnd;

	// If the file title has been set, use it as the filename
	// (this gets rid of the ugly directory info)
	if (lstrlen(m_szFileTitle) > 0)
		lstrcpyn(m_szFile, m_szFileTitle, IO::Path::kMaxPathLen);

	if(GetOpenFileName(&m_OFN))
	{
		lstrcpyn(szOutBuffer, m_szFile, IO::Path::kMaxPathLen);

		// Copy the current directory
		GetCurrentDirectory(m_szCurrentDirectory);
		return TRUE;
	} else {
		return FALSE;
	}
}

// Opens a common file requester and gets filename.
// Returns FALSE is user clicked cancel, TRUE if
// user clicked ok
BOOL FileNameHandler::GetSaveName(HWND hwnd, LPTSTR szOutBuffer)
{
	m_OFN.hwndOwner = hwnd;

	if (lstrlen(m_szFileTitle) > 0)
		lstrcpyn(m_szFile, m_szFileTitle, IO::Path::kMaxPathLen);

	if(GetSaveFileName(&m_OFN))
	{
		lstrcpyn(szOutBuffer, m_szFile, IO::Path::kMaxPathLen);

		// Copy the current directory
		GetCurrentDirectory(m_szCurrentDirectory);
		return TRUE;
	} else {
		return FALSE;
	}
}

