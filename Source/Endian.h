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

#ifndef ENDIAN_H_
#define ENDIAN_H_

#if (DAEDALUS_ENDIAN_MODE == DAEDALUS_ENDIAN_BIG)
#define U8_SWAP(x) x
#define  U8_TWIDDLE 0x0
#define  U16_TWIDDLE 0x0
#elif (DAEDALUS_ENDIAN_MODE == DAEDALUS_ENDIAN_LITTLE)
#define U8_SWAP(x) ((x >> 24) | ((x >> 8) & 0xFF00) | ((x & 0xFF00) << 8) | (x << 24))
#define U8_TWIDDLE 0x3
#define U16_TWIDDLE 0x2
#define U16H_TWIDDLE 0x1
#else
#error No DAEDALUS_ENDIAN_MODE specified
#endif

#endif // ENDIAN_H_
