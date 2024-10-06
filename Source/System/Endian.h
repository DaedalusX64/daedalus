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


#ifndef UTILITY_ENDIAN_H_
#define UTILITY_ENDIAN_H_
#include "Base/Types.h"
#include <bit>
#include <cstdint>

constexpr bool is_big_endian = std::endian::native == std::endian::big;

constexpr auto U8_TWIDDLE = is_big_endian ? 0x0 : 0x3;
constexpr auto U16_TWIDDLE = is_big_endian ? 0x0 : 0x2;
constexpr auto U16H_TWIDDLE = is_big_endian ? 0x0 : 0x1;


#define BSWAP32(x) std::byteswap(x)
#endif // UTILITY_ENDIAN_H_
