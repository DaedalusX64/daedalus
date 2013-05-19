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

#pragma once

#ifndef SYSW32_INTERFACE_CHECKBOX_H_
#define SYSW32_INTERFACE_CHECKBOX_H_

// Windowsx.h type defines - hope these don't conflict with anything!
#define CheckBox_GetCheck(hwnd)			SendMessage(hwnd, BM_GETCHECK, 0,0)
#define CheckBox_SetCheck(hwnd, state)	SendMessage(hwnd, BM_SETCHECK, (WPARAM)state,0)

#define CheckBox_IsChecked( hwnd )		(CheckBox_GetCheck( (hwnd) ) == BST_CHECKED)

#endif // SYSW32_INTERFACE_CHECKBOX_H_
