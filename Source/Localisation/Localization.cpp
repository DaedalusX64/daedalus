/*
Copyright (C) 2001 Lkb

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
#include "LanguageTable.h"

#include "Utility/IO.h"

using namespace std;

extern OSVERSIONINFO g_OSVersionInfo;

// currently we only have at most two localization dlls loaded, so the vector is not very useful

struct ResourceModuleEntry
{
	HINSTANCE hInstance;
	BOOL bCurrentVersion; // if true, the DLL has the same version number of Daedalus
	TCHAR szLanguage[6]; // en-us\0
};

static vector<ResourceModuleEntry> g_vhResourceModules;

// internal
BOOL Localization_AddLanguage(LPCTSTR pLanguage)
{
	if(strlen(pLanguage) > 5)
	{
		return FALSE;
	}

	vector<ResourceModuleEntry>::const_iterator iter;

	for(iter = g_vhResourceModules.begin(); iter < g_vhResourceModules.end(); iter++)
	{
		if(strcmp((*iter).szLanguage, pLanguage) == 0)
		{
			// already loaded
			return FALSE;
		}
	}

	// based on code in AudioDialog.cpp
	// Windows is not case sensitive so don't care about it
	TCHAR szFileSpec[MAX_PATH+1];
	lstrcpyn(szFileSpec, gDaedalusExePath, MAX_PATH);
	IO::Path::Append(szFileSpec, TEXT("Localization"));

	IO::Path::Append(szFileSpec, TEXT("Daedalus_"));

	// Daedalus.en-us.dll
	strcat(szFileSpec, pLanguage);
	strcat(szFileSpec, ".dll");

	ResourceModuleEntry entry;
	if(g_OSVersionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT)
	{
		// this doesn't execute anything, but it's only available in WinNT
		entry.hInstance = LoadLibraryEx(szFileSpec, NULL, DONT_RESOLVE_DLL_REFERENCES);
		//entry.hInstance = LoadLibrary(szFileSpec);
	}
	else
	{
		// SECURITY PROBLEM: this executes DllMain
		// TODO: if possible, find DllMain and fail if it's present to avoid executing code that may be malicious
		entry.hInstance = LoadLibrary(szFileSpec);
	}
	if(entry.hInstance == NULL)
		return FALSE;

	TCHAR szVersion[256];
	LoadString(entry.hInstance, IDS_LOCALIZATION_DLL_FOR_VERSION, szVersion, 256);

	entry.bCurrentVersion = (_strcmpi(szVersion, DAEDALUS_VERSION) == 0);
	strcpy(entry.szLanguage, pLanguage);

	g_vhResourceModules.push_back(entry);


	if ( entry.bCurrentVersion )
	{
		_Module.m_hInstResource = entry.hInstance;
	}

	return TRUE;
}

// internal
BOOL Localization_ClearLanguages()
{
	vector<ResourceModuleEntry>::const_iterator iter;
	for(iter = g_vhResourceModules.begin(); iter < g_vhResourceModules.end(); iter++)
	{
		FreeLibrary((*iter).hInstance);
	}
	g_vhResourceModules.clear();
	return TRUE;
}

// supports either "en-us" or "en" language names
BOOL Localization_SetLanguage(LPCTSTR pLanguage) // NULL sets default language
{
	TCHAR szLanguageName[256];
	Localization_ClearLanguages();
	szLanguageName[0] = 0;
	if(pLanguage == NULL)
	{
		LCID locale = GetUserDefaultLCID();
		// Win9x doesn't support LOCALE_SISO639LANGNAME and LOCALE_SISO3166CTRYNAME, but we try anyways
		GetLocaleInfo(locale, LOCALE_SISO639LANGNAME  , szLanguageName, 256);
		if(szLanguageName[0] != 0)
		{
			strcat(szLanguageName, "-");
			GetLocaleInfo(locale, LOCALE_SISO3166CTRYNAME , szLanguageName + strlen(szLanguageName), 256);
			pLanguage = szLanguageName;
		}
		else
		{
			// TODO: Is there a better way to do this in Win9x?
			WORD langID = GetUserDefaultLangID();
			langID = MAKELANGID(PRIMARYLANGID(langID), (SUBLANGID(langID) == 0) ? 1 : SUBLANGID(langID));
			for(int n = 0; g_LanguageTable[n].pszLanguageName != NULL; n++)
			{
				if(g_LanguageTable[n].nLangID == langID)
				{
					pLanguage = g_LanguageTable[n].pszLanguageName;
				}
			}
			if(pLanguage == NULL)
				pLanguage = "en-us";
		}
	}

	if(pLanguage != szLanguageName)
	{
		strcpy(szLanguageName, pLanguage);
		pLanguage = szLanguageName;
	}
	LPTSTR pSeparator = strchr(szLanguageName, '-');
	if(pSeparator != NULL)
	{
		Localization_AddLanguage(szLanguageName);
		*pSeparator = 0;
		Localization_AddLanguage(szLanguageName);
	}
	else
	{
		Localization_AddLanguage(pLanguage);
	}

	return TRUE;
}
