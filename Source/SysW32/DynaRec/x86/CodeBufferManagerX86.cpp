/*
Copyright (C) 2005 StrmnNrmn

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

#include "stdafx.h"
#include "DynaRec/CodeBufferManager.h"

#include "Debug/DBGConsole.h"

#include "CodeGeneratorX86.h"

/* Added by Lkb (24/8/2001)
The second buffer is used to hold conditionally executed code pieces that will usually not be executed

Example:

globalbuffer:
...
TEST a, a
JZ globalsecondbuffer_1234 # usually a is nonzero
MOV EAX, a
return_from_globalsecondbuffer_1234:

globalsecondbuffer:
...
globalsecondbuffer_1234
CALL xxx
JMP return_from_globalsecondbuffer_1234

The code that uses this is responsible to mantain the pointer 16-byte-aligned.
The second buffer must come AFTER the first in the memory layout, otherwise branch prediction will be screwed up.

This is the same algorithm used in the Linux schedule() function (/usr/src/linux/kernel/sched.c)

The only problem of this system is that it uses 32-bit relative addresses (and thus 6-byte long instructions) are used.
However managing 8-bit +127/-128 relative displacements would be challenging since they would interfere with the normal code
Otherwise a 16-bit override prefix could be used (but is it advantageous?)
*/

class CCodeBufferManagerX86 : public CCodeBufferManager
{
public:
	CCodeBufferManagerX86()
		:	mpBuffer( NULL )
		,	mBufferPtr( 0 )
		,	mBufferSize( 0 )
		,	mpSecondBuffer( NULL )
		,	mSecondBufferPtr( 0 )
		,	mSecondBufferSize( 0 )
	{
	}

	virtual bool			Initialise();
	virtual void			Reset();
	virtual void			Finalise();

	virtual CCodeGenerator *StartNewBlock();
	virtual u32				FinaliseCurrentBlock();

private:

	u8	*					mpBuffer;
	u32						mBufferPtr;
	u32						mBufferSize;

	u8 *					mpSecondBuffer;
	u32						mSecondBufferPtr;
	u32						mSecondBufferSize;

private:
	CAssemblyBuffer			mPrimaryBuffer;
	CAssemblyBuffer			mSecondaryBuffer;
};

//*****************************************************************************
//
//*****************************************************************************
CCodeBufferManager *	CCodeBufferManager::Create()
{
	return new CCodeBufferManagerX86;
}

//*****************************************************************************
//
//*****************************************************************************
bool	CCodeBufferManagerX86::Initialise()
{
	// Reserve a huge range of memory. We do this because we can't simply
	// allocate a new buffer and copy the existing code across (this would
	// mess up all the existing function pointers and jumps etc).
	// Note that this call does not actually allocate any storage - we're not
	// actually asking Windows to allocate 256Mb!
	mpBuffer = (u8*)VirtualAlloc(NULL, 256 * 1024 * 1024, MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	if (mpBuffer == NULL)
		return false;

	mBufferPtr = 0;
	mBufferSize = 0;

	mpSecondBuffer = mpBuffer + 192 * 1024 * 1024;
	mSecondBufferPtr = 0;
	mSecondBufferSize = 0;

	return true;
}

//*****************************************************************************
//
//*****************************************************************************
void	CCodeBufferManagerX86::Reset()
{
	mBufferPtr = 0;
	mSecondBufferPtr = 0;
}

//*****************************************************************************
//
//*****************************************************************************
void	CCodeBufferManagerX86::Finalise()
{
	if (mpBuffer != NULL)
	{
		// Decommit all the pages first
		VirtualFree(mpBuffer, 256 * 1024 * 1024, MEM_DECOMMIT);
		// Now release
		VirtualFree(mpBuffer, 0, MEM_RELEASE);
		mpBuffer = NULL;
	}

	mpSecondBuffer = NULL;
}

//*****************************************************************************
//
//*****************************************************************************
CCodeGenerator * CCodeBufferManagerX86::StartNewBlock()
{
	// Round up to 16 byte boundry
	u32 aligned_ptr( (mBufferPtr + 15) & (~15) );

	u32	padding( aligned_ptr - mBufferPtr );
	if( padding > 0 )
	{
		memset( mpBuffer + mBufferPtr, 0xcc, padding );		// 0xcc is 'int 3'
	}

	mBufferPtr = aligned_ptr;

	// This is a bit of a hack. We assume that no single entry will generate more than
	// 32k of storage. If there appear to be problems with this assumption, this
	// value can be enlarged
	if (mBufferPtr + 32768 > mBufferSize)
	{
		// Increase by 1MB
		LPVOID pNewAddress;

		mBufferSize += 1024 * 1024;
		pNewAddress = VirtualAlloc(mpBuffer, mBufferSize, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
		if (pNewAddress == 0)
		{
			DBGConsole_Msg(0, "SR Buffer allocation failed"); // maybe this should be an abort?
		}
		else
		{
			DBGConsole_Msg(0, "Allocated %dMB of storage for dynarec buffer", mBufferSize / (1024*1024));
		}

	}

	if (mSecondBufferPtr + 32768 > mSecondBufferSize)
	{
		// Increase by 1MB
		LPVOID pNewAddress;

		mSecondBufferSize += 1024 * 1024;
		pNewAddress = VirtualAlloc(mpSecondBuffer, mSecondBufferSize, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
		if (pNewAddress == 0)
		{
			DBGConsole_Msg(0, "SR Second Buffer allocation failed"); // maybe this should be an abort?
		}
		else
		{
			DBGConsole_Msg(0, "Allocated %dMB of storage for dynarec second buffer",
				mSecondBufferSize / (1024*1024));
		}
	}

	mPrimaryBuffer.SetBuffer( mpBuffer + mBufferPtr );
	mSecondaryBuffer.SetBuffer( mpSecondBuffer + mSecondBufferPtr );

	return new CCodeGeneratorX86( &mPrimaryBuffer, &mSecondaryBuffer );
}

//*****************************************************************************
//
//*****************************************************************************
u32 CCodeBufferManagerX86::FinaliseCurrentBlock()
{
	u32		main_block_size( mPrimaryBuffer.GetSize() );

	mBufferPtr += main_block_size;

	mSecondBufferPtr += mSecondaryBuffer.GetSize();
	mSecondBufferPtr = ((mSecondBufferPtr - 1) & 0xfffffff0) + 0x10; // align to 16-byte boundary

	return main_block_size;
}
