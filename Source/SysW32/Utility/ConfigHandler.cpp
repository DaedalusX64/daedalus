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
#include "ConfigHandler.h"

///////////////////////////////////////////////
//// Constructors / Deconstructors
///////////////////////////////////////////////

ConfigHandler::ConfigHandler(TCHAR *szSectionName)
{
	lstrcpy(m_szSectionName, szSectionName);
}

ConfigHandler::~ConfigHandler()
{
}

///////////////////////////////////////////////
// Write an integer to the file/section
void ConfigHandler::WriteValue(TCHAR *szKeyName, u32 nValue, s32 nNumber)
{
	TCHAR szKeyBuffer[255];

	// Append number to Key Name if not 0
	if (nNumber < 0)	strcpy(szKeyBuffer, szKeyName);
	else				wsprintf(szKeyBuffer, "%s_%d", szKeyName, nNumber);

	WriteRegValue(szKeyBuffer, nValue);

}

void ConfigHandler::WriteRegValue(TCHAR *szKeyName, u32 nValue)
{
	HKEY hRegKey;
	TCHAR szSubKey[256] = "Software\\Daedalus\\";
	u32 nResult;

	// Try and get default dir from registry entry...
	strcat(szSubKey, m_szSectionName);

	if (RegCreateKeyEx(HKEY_CURRENT_USER, szSubKey,
				 0, m_szSectionName, 0, KEY_ALL_ACCESS,
				 NULL, &hRegKey, &nResult) == ERROR_SUCCESS)
	{

		RegSetValueEx(hRegKey, szKeyName, 0, REG_DWORD, (LPBYTE)&nValue, sizeof(u32));
		RegCloseKey(hRegKey);
	}
}

/////////////////////////////////////////////
// Write a string to the file/section
void ConfigHandler::WriteString(TCHAR *szKeyName, TCHAR *szValueBuffer, s32 nNumber)
{
	TCHAR szKeyBuffer[255];

	// Append number to Key Name if not 0
	if (nNumber < 0)	strcpy(szKeyBuffer, szKeyName);
	else				wsprintf(szKeyBuffer, "%s_%d", szKeyName, nNumber);

	WriteRegString(szKeyBuffer, szValueBuffer);

}


void ConfigHandler::WriteRegString(TCHAR *szKeyName, TCHAR *szValueBuffer)
{
	HKEY hRegKey;
	TCHAR szSubKey[256] = "Software\\Daedalus\\";
	u32 nResult;

	// Try and get default dir from registry entry...
	strcat(szSubKey, m_szSectionName);

	if (RegCreateKeyEx(HKEY_CURRENT_USER, szSubKey,
				 0, m_szSectionName, 0, KEY_ALL_ACCESS,
				 NULL, &hRegKey, &nResult) == ERROR_SUCCESS)
	{

		RegSetValueEx(hRegKey, szKeyName, 0, REG_SZ, (LPBYTE)szValueBuffer, strlen(szValueBuffer)+1);
		RegCloseKey(hRegKey);
	}
}

//////////////////////////////////////////////
// Read a value from .ini file/registry
void ConfigHandler::ReadValue(TCHAR *szKeyName,
							  u32 *nValue, u32 nDefault)
{
	ReadValue(szKeyName, nValue, nDefault, 0, 0, -1);
}
void ConfigHandler::ReadValue(TCHAR *szKeyName,
							  u32 *nValue, u32 nDefault,
							  u32 nMin, u32 nMax)
{
	ReadValue(szKeyName, nValue, nDefault, nMin, nMax, -1);
}

void ConfigHandler::ReadValue(TCHAR *szKeyName,
							  u32 *nValue, u32 nDefault,
							  u32 nMin, u32 nMax,
							  s32 nNumber)
{

	TCHAR szKeyBuffer[255];
	u32 nReturnVal;

	// Append number to Key Name if not 0
	if (nNumber < 0)	strcpy(szKeyBuffer, szKeyName);
	else				wsprintf(szKeyBuffer, "%s_%d", szKeyName, nNumber);

	nReturnVal = ReadRegValue(szKeyBuffer, nDefault);

	if (nMin < nMax)
	{
		if (nReturnVal < nMin) nReturnVal = nMin;
		if (nReturnVal > nMax) nReturnVal = nMax;
	}
	*nValue = nReturnVal;
}

u32 ConfigHandler::ReadRegValue(TCHAR *szKeyName, u32 nDefault)
{
	HKEY hRegKey;
	TCHAR szSubKey[256] = "Software\\Daedalus\\";
	u32 datatype, datasize, nResult;
	u32 nDWValue;

	// Try and get default dir from registry entry...
	strcat(szSubKey, m_szSectionName);

	RegCreateKeyEx(HKEY_CURRENT_USER, szSubKey,
				 0, m_szSectionName, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS,
				 NULL, &hRegKey, &nResult);

	datasize = sizeof(u32);
	nResult = RegQueryValueEx(hRegKey, szKeyName, NULL,
		&datatype, (LPBYTE)&nDWValue, &datasize);

	RegCloseKey(hRegKey);

	if (nResult == ERROR_SUCCESS)	return nDWValue;
	else							return nDefault;
}

////////////////////////////////////////////
// Read a string from .ini file/registry
void ConfigHandler::ReadString(TCHAR *szKeyName,
							   TCHAR *szOutBuffer, u32 nOutSize,
							   TCHAR *szDefaultBuffer)
{
	ReadString(szKeyName, szOutBuffer, nOutSize, szDefaultBuffer, -1);
}

void ConfigHandler::ReadString(TCHAR *szKeyName,
							   TCHAR *szOutBuffer, u32 nOutSize,
							   TCHAR *szDefaultBuffer,
							   s32 nNumber)
{
	TCHAR szKeyBuffer[255];
	u32 nResult;

	// Append number to Key Name if not 0
	if (nNumber < 0)	strcpy(szKeyBuffer, szKeyName);
	else				wsprintf(szKeyBuffer, "%s_%d", szKeyName, nNumber);

	nResult = ReadRegString(szKeyBuffer, szOutBuffer, nOutSize);

	// If read failed, copy default to the output buffer
	if (nResult == 0) strcpy(szOutBuffer, szDefaultBuffer);
}

u32 ConfigHandler::ReadRegString(TCHAR *szKeyName, TCHAR *szOutBuffer, u32 nOutSize)
{
	HKEY hRegKey;
	TCHAR szSubKey[256] = "Software\\Daedalus\\";
	u32 datatype, datasize, nResult;

	// Try and get default dir from registry entry...
	strcat(szSubKey, m_szSectionName);

	RegCreateKeyEx(HKEY_CURRENT_USER, szSubKey,
				 0, m_szSectionName, 0, KEY_ALL_ACCESS,
				 NULL, &hRegKey, &nResult);

	datasize = nOutSize;
	nResult = RegQueryValueEx(hRegKey, szKeyName, NULL,
		&datatype, (LPBYTE)szOutBuffer, &datasize);

	RegCloseKey(hRegKey);

	if (nResult == ERROR_SUCCESS)	return 1;
	else							return 0;
}
