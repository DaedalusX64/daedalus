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

#pragma once

#ifndef SYSPSP_UTILITY_PATHSPSP_H_
#define SYSPSP_UTILITY_PATHSPSP_H_

#ifdef DAEDALUS_PSP_ALT
#define DAEDALUS_PSP_PATH(p)				"host0:/" p
#else
#define DAEDALUS_PSP_PATH(p)				p
#endif

#endif // SYSPSP_UTILITY_PATHSPSP_H_
