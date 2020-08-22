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

#ifndef SYSW32_INTERFACE_FILENAMEHANDLER_H_
#define SYSW32_INTERFACE_FILENAMEHANDLER_H_

#include <commdlg.h>
#include "System/IO.h"

class FileNameHandler {
public:
	// MODIFIED BY Lkb - 21/jul/2001 - savestate support
	FileNameHandler( LPCTSTR szSectionName,
					 LPCTSTR szFilter, int nFilterIndex, LPCTSTR szDefExt, LPCTSTR pszDefaultDir = NULL );
	~FileNameHandler();

	void GetDefaultDirectory( LPTSTR );
	void SetDefaultDirectory( LPCTSTR szDir );

	void GetModuleDirectory( LPTSTR szOutBuffer );
	void GetCurrentDirectory( LPTSTR szOutBuffer );

	void GetCurrentFileName( LPTSTR szOutBuffer );
	void SetFileName( LPCTSTR szNewName );
	BOOL GetOpenName( HWND, LPTSTR );
	BOOL GetSaveName( HWND hwnd, LPTSTR );
private:
	OPENFILENAME		m_OFN;

	char				m_szSectionName[128];

	char				m_szFile[300];
	char				m_szFileTitle[300];

	IO::Filename		m_szCurrentDirectory;
};

#endif // SYSW32_INTERFACE_FILENAMEHANDLER_H_

