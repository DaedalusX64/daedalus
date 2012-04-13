/*
Copyright (C) 2008 StrmnNrmn

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

#ifndef PATHSPSP_H_
#define PATHSPSP_H_

#ifdef DAEDALUS_SILENT
#define DAEDALUS_PSP_PATH(p)				p
#else
#ifdef DAEDALUS_PSP_ALT
#define DAEDALUS_PSP_PATH(p)				"host0:/" p
#else
#define DAEDALUS_PSP_PATH(p)				p
#endif
#endif

#endif // PATHSPSP_H_
