/*
Copyright (C) 2001,2005 StrmnNrmn

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
#endif

#ifndef ASSEMBLYBUFFER_H_
#define ASSEMBLYBUFFER_H_

#include "AssemblyUtils.h"

class CAssemblyBuffer
{
	public:
		CAssemblyBuffer()
		:	mpCodeBuffer(NULL)
		,	mpWritePointer(NULL)
		,	mCurrentPos( 0 )
		{
		}

		inline void	PadTo16Bytes()
		{
			mCurrentPos = (--mCurrentPos & 0xfffffff0) + 0x10; // align to 16-byte boundary
		}

		inline void EmitBYTE(u8 byte)
		{
			mpWritePointer[mCurrentPos] = byte;
			mCurrentPos++;
		}

		inline void EmitWORD(u16 word)
		{
			*(u16 *)(&mpWritePointer[mCurrentPos]) = word;
			mCurrentPos += 2;
		}

		inline void EmitDWORD(u32 dword)
		{
			*(u32 *)(&mpWritePointer[mCurrentPos]) = dword;
			mCurrentPos += 4;
		}

		void EmitData( const void * pdata, u32 count )
		{
			memcpy( &mpWritePointer[ mCurrentPos ], pdata, count );
			mCurrentPos += count;
		}

		CCodeLabel			GetStartAddress() const										{ return CCodeLabel(&mpCodeBuffer[0]); }
		CCodeLabel			GetLabel() const											{ return CCodeLabel(&mpCodeBuffer[mCurrentPos]); }
		CJumpLocation		GetJumpLocation() const										{ return CJumpLocation(&mpCodeBuffer[mCurrentPos]); }
		u32					GetSize() const												{ return mCurrentPos; }

		void				SetBuffer( u8 * pbuffer )
		{
			mpCodeBuffer = pbuffer;

			// For the PSP we don't want to cache our writes, ToDo:why?
			mpWritePointer = (u8*)MAKE_UNCACHED_PTR(mpCodeBuffer);
			//ToDo: Test this
			//mpWritePointer = mpCodeBuffer;
			mCurrentPos = 0;
		}

protected:
	u8 *					mpCodeBuffer;
	u8 *					mpWritePointer;
	u32						mCurrentPos;		// Current writing position in buffer
};

#endif // ASSEMBLYBUFFER_H_
