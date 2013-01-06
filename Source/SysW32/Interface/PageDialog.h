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

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#ifndef __PAGEDIALOG_H__
#define __PAGEDIALOG_H__

#include "ConfigDialog.h"

struct DaedalusConfig;
extern DaedalusConfig g_CurrentConfig;

class CPageDialog
{
	protected:
		CPageDialog() {}

	public:
		virtual ~CPageDialog() {}

		virtual CConfigDialog::PageType GetPageType() const = 0;

		virtual void CreatePage( HWND hWndParent, RECT & rect ) = 0;
		virtual void DestroyPage( ) = 0;
};


#endif