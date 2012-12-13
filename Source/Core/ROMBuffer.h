/*
Copyright (C) 2006 StrmnNrmn

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

#ifndef ROMBUFFER_H_
#define ROMBUFFER_H_


//
//	This class is responsible for maintaining the image of the rom.
//	For the PC, this just loads the rom into a single chunk of memory.
//	For the PSP/Xbox the whole rom won't fit into memory at once,
//	so it must stream chunks in on demand.
//

class RomBuffer
{
	public:
		static bool		Create();
		static void		Destroy();

		static void		Open();
		static void		Close();

		static bool		IsRomLoaded();

		static u32		GetRomSize();

		/// Copy bytes of memory from the cart, with no swizzling etc
		/// rom_start is 0-based (i.e. not a full rom address)
		static void		GetRomBytesRaw( void * p_dst, u32 rom_start, u32 length );
		static void		PutRomBytesRaw( u32 rom_start, const void * p_src, u32 length );

		template< typename T > T static ReadValueRaw( u32 rom_start )
		{
			T	result;

			GetRomBytesRaw( &result, rom_start, sizeof( T ) );

			return result;
		}

		template< typename T > static void WriteValueRaw( u32 rom_start, T value)
		{
			PutRomBytesRaw( rom_start, &value, sizeof( T ) );
		}

		static void * GetAddressRaw( u32 rom_start );

		static void CopyToRam( u8 * p_dst, u32 dst_offset, u32 dst_size, u32 src_offset, u32 length );
		//static void CopyFromRam( u32 dst_offset, const u8 * p_src, u32 src_offset, u32 src_size, u32 length );

		static bool IsRomAddressFixed();
		static const void * GetFixedRomBaseAddress();
};

#endif // ROMBUFFER_H_

