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

#ifndef __CONFIGHANDLER_H__
#define __CONFIGHANDLER_H__


class ConfigHandler {
public:

	ConfigHandler(TCHAR *szSectionName);
	~ConfigHandler();

	void SetSection(TCHAR *szNewSection) { strcpy(m_szSectionName, szNewSection); };

	void WriteValue(TCHAR *szKeyName, u32 nValue) {WriteValue(szKeyName, nValue, -1);};
	void WriteValue(TCHAR *szKeyName, u32 nValue, s32 nNumber);

	void WriteString(TCHAR *szKeyName, TCHAR *szValue) {WriteString(szKeyName, szValue, -1);};
	void WriteString(TCHAR *szKeyName, TCHAR *szValue, s32 nNumber);

	void ReadValue(TCHAR *szKeyName, u32 *nValue, u32 nDefault);
	void ReadValue(TCHAR *szKeyName, u32 *nValue, u32 nDefault, u32 nMin, u32 nMax);
	void ReadValue(TCHAR *szKeyName, u32 *nValue, u32 nDefault, u32 nMin, u32 nMax, s32 nNumber);

	void ReadString(TCHAR *szKeyName, TCHAR *szOutBuffer, u32 nOutSize, TCHAR *szDefaultBuffer);
	void ReadString(TCHAR *szKeyName, TCHAR *szOutBuffer, u32 nOutSize, TCHAR *szDefaultBuffer, s32 nNumber);

private:
	void WriteRegValue(TCHAR *szKeyName, u32 nValue);
	void WriteRegString(TCHAR *szKeyName, TCHAR *szValueBuffer);

	u32 ReadRegValue(TCHAR *szKeyName, u32 nDefault);
	u32 ReadRegString(TCHAR *szKeyName, TCHAR *szOutBuffer, u32 nOutSize);
private:
	TCHAR m_szSectionName[300];

};

#endif //__CONFIGHANDLER_H__
