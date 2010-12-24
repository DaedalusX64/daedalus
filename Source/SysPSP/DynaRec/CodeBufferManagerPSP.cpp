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

#include "Math/MathUtil.h"

#include "Debug/DBGConsole.h"

#include "CodeGeneratorPSP.h"

#include <psputilsforkernel.h>

extern "C" { void _DaedalusICacheInvalidate( const void * address, u32 length ); }

struct SCodeBuffer
{
	u8	*						mpBuffer;
	u32							mBufferPtr;
	u32							mBufferSize;

	SCodeBuffer()
		:	mpBuffer( NULL )
		,	mBufferPtr( 0 )
		,	mBufferSize( 0 )
	{
	}

	void	Initialise( u32 size)
	{
		mBufferPtr = 0;
		mBufferSize = size;
		mpBuffer = new u8[ size ];
	}

	void	Finalise()
	{
		sceKernelIcacheInvalidateRange( mpBuffer, mBufferPtr );
		if (mpBuffer != NULL)
		{
			delete [] mpBuffer;
			mpBuffer = NULL;
		}
		mBufferPtr = 0;
		mBufferSize = 0;
	}

	void	Reset()
	{
		mBufferPtr = 0;
	}

	u8 *	StartNewBlock()
	{
		// Round up to 64 byte boundary - i.e. one cache line
		u8 *	p_rounded( (u8*)AlignPow2( (u32)(mpBuffer + mBufferPtr), 64 ) );
		mBufferPtr = p_rounded - mpBuffer;

		// This is a bit of a hack. We assume that no single entry will generate more than
		// 32k of storage. If there appear to be problems with this assumption, this
		// value can be enlarged
#ifndef DAEDALUS_SILENT
		if (mBufferPtr + 32768 > mBufferSize)
		{
			DAEDALUS_ERROR( "Out of memory for dynamic recompiler" );
		}
#endif
		return mpBuffer + mBufferPtr;
	}

	void	Consume( const u8 * p_base, u32 used_size )
	{
		//	DAEDALUS_ASSERT( AlignPow2( (u32)p_base, 64 ) == (u32)p_base, "Base ptr is not aligned" );
		const u8 * p_lower( RoundPointerDown( p_base, 64 ) );
		const u8 * p_upper( RoundPointerUp( p_base + used_size, 64 ) );
		const u32  size( p_upper - p_lower);

		//sceKernelIcacheInvalidateRange( p_lower, size );
		//sceKernelIcacheClearAll();

		if( size > 0 )
		{
			_DaedalusICacheInvalidate( p_lower, size );
		}

		mBufferPtr += used_size;
	}

};



class CCodeBufferManagerPSP : public CCodeBufferManager
{
public:
	CCodeBufferManagerPSP()
	{
	}

	virtual bool				Initialise();
	virtual void				Reset();
	virtual void				Finalise();

	virtual CCodeGenerator *	StartNewBlock();
	virtual u32					FinaliseCurrentBlock();

private:

	SCodeBuffer					mPrimaryBuffer;
	SCodeBuffer					mSecondaryBuffer;

private:
	CAssemblyBuffer				mAssemblyBufferA;
	CAssemblyBuffer				mAssemblyBufferB;
};

//*****************************************************************************
//
//*****************************************************************************
CCodeBufferManager *	CCodeBufferManager::Create()
{
	return new CCodeBufferManagerPSP;
}

//*****************************************************************************
//
//*****************************************************************************
bool	CCodeBufferManagerPSP::Initialise()
{
	mPrimaryBuffer.Initialise( 3 * 1024 * 1024 );
	mSecondaryBuffer.Initialise( 3 * 1024 * 1024 );

	return true;
}

//*****************************************************************************
//
//*****************************************************************************
void	CCodeBufferManagerPSP::Reset()
{
	mPrimaryBuffer.Reset();
	mSecondaryBuffer.Reset();
}

//*****************************************************************************
//
//*****************************************************************************
void	CCodeBufferManagerPSP::Finalise()
{
	mPrimaryBuffer.Finalise();
	mSecondaryBuffer.Finalise();
}

//*****************************************************************************
//
//*****************************************************************************
CCodeGenerator * CCodeBufferManagerPSP::StartNewBlock()
{
	u8 * primary( mPrimaryBuffer.StartNewBlock() );
	u8 * secondary( mSecondaryBuffer.StartNewBlock() );

	mAssemblyBufferA.SetBuffer( primary );
	mAssemblyBufferB.SetBuffer( secondary );
	return new CCodeGeneratorPSP( &mAssemblyBufferA, &mAssemblyBufferB );
}

//*****************************************************************************
//
//*****************************************************************************
u32 CCodeBufferManagerPSP::FinaliseCurrentBlock()
{
	const u8 *		p_base_a( mAssemblyBufferA.GetStartAddress().GetTargetU8P() );
	u32				main_block_size_a( mAssemblyBufferA.GetSize() );

	mPrimaryBuffer.Consume( p_base_a, main_block_size_a );

	const u8 *		p_base_b( mAssemblyBufferB.GetStartAddress().GetTargetU8P() );
	u32				main_block_size_b( mAssemblyBufferB.GetSize() );

	mSecondaryBuffer.Consume( p_base_b, main_block_size_b );

	return main_block_size_a;
}
