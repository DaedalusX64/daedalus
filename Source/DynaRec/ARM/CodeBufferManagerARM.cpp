/*
Copyright (C) 2020 MasterFeizz

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


#include <stdio.h>
#include <malloc.h>

#ifdef DAEDALUS_POSIX
#include <sys/mman.h>
#elif DAEDALUS_CTR
#include "SysCTR/Utility/MemoryCTR.h"
#endif
#include <iostream>

#include "DynaRec/CodeBufferManager.h"
#include "Debug/DBGConsole.h"
#include "CodeGeneratorARM.h"

#define CODE_BUFFER_SIZE (8 * 1024 * 1024)

class CCodeBufferManagerARM : public CCodeBufferManager
{
public:
	CCodeBufferManagerARM()
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

	virtual std::shared_ptr<CCodeGenerator> StartNewBlock();
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


std::shared_ptr<CCodeBufferManager>	CCodeBufferManager::Create()
{
	return std::make_shared<CCodeBufferManagerARM>();
}


bool	CCodeBufferManagerARM::Initialise()
{
	// mpSecondBuffer is currently unused

#ifdef DAEDALUS_CTR
	mpBuffer = (u8*)memalign(4096, CODE_BUFFER_SIZE*2);
#elif DAEDALUS_POSIX

	mpBuffer = (u8*)mmap(nullptr, CODE_BUFFER_SIZE*2, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
#endif	
	if (mpBuffer == MAP_FAILED)
		return false;

	mpSecondBuffer = mpBuffer + CODE_BUFFER_SIZE;
	
#ifdef __linux__
		// Make the memory executable
	if (mprotect(mpBuffer, CODE_BUFFER_SIZE * 2, PROT_READ | PROT_WRITE | PROT_EXEC) != 0)
	{
		perror("mprotect");
		return false;
	}
#elif DAEDALUS_CTR
	_SetMemoryPermission((unsigned int*)mpBuffer, CODE_BUFFER_SIZE*2, 7);
	//_SetMemoryPermission((unsigned int*)mpSecondBuffer, CODE_BUFFER_SIZE, 7);
	#endif

	mBufferPtr = 0;
	mBufferSize = 0;

	mSecondBufferPtr = 0;
	mSecondBufferSize = 0;

	return true;
}


void	CCodeBufferManagerARM::Reset()
{
	mBufferPtr = 0;
	mSecondBufferPtr = 0;
}


void	CCodeBufferManagerARM::Finalise()
{
	if (mpBuffer != NULL && mpSecondBuffer != NULL)
	{
		#ifdef DAEDALUS_CTR
		_SetMemoryPermission((unsigned int*)mpBuffer, CODE_BUFFER_SIZE*2 / 4096, 3);
		//_SetMemoryPermission((unsigned int*)mpSecondBuffer, CODE_BUFFER_SIZE / 4096, 3);
		#endif
		
		free(mpBuffer);
		//free(mpSecondBuffer);

		mpBuffer = NULL;
		mpSecondBuffer = NULL;
	}
}


std::shared_ptr<CCodeGenerator> CCodeBufferManagerARM::StartNewBlock()
{
	// Round up to 16 byte boundry
	u32 aligned_ptr = (mBufferPtr + 15) & (~15);

	mBufferPtr = aligned_ptr;

	u32 aligned_ptr2 = (mSecondBufferPtr + 15) & (~15);
	mSecondBufferPtr = aligned_ptr2;

	mPrimaryBuffer.SetBuffer( mpBuffer + mBufferPtr );
	mSecondaryBuffer.SetBuffer( mpSecondBuffer + mSecondBufferPtr );

	return std::make_shared<CCodeGeneratorARM>( &mPrimaryBuffer, &mSecondaryBuffer );
}


u32 CCodeBufferManagerARM::FinaliseCurrentBlock()
{
	u32		main_block_size( mPrimaryBuffer.GetSize() );

	 char* start = reinterpret_cast<char*>(mpBuffer + mBufferPtr);
	 char* end   = start + main_block_size;


	mBufferPtr += main_block_size;
	mSecondBufferPtr += mSecondaryBuffer.GetSize();

	#if 0 //Second buffer is currently unused
	mSecondBufferPtr += mSecondaryBuffer.GetSize();
	mSecondBufferPtr = ((mSecondBufferPtr - 1) & 0xfffffff0) + 0x10; // align to 16-byte boundary
	#endif


	#ifdef DAEDALUS_CTR
	_InvalidateAndFlushCaches();
	#elif DAEDALUS_POSIXls
	 __builtin___clear_cache(start,end);
	#endif
	
	return main_block_size;
}
