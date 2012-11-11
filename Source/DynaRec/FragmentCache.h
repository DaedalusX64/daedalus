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

#ifndef __DAEDALUS_FRAGMENTCACHE_H__
#define __DAEDALUS_FRAGMENTCACHE_H__

class	CFragment;
class	CJumpLocation;
class	CCodeBufferManager;

#include <map>
#include <vector>

struct FHashT
{
	u32	addr;
	u32 ptr;
};

//*************************************************************************************
//
//*************************************************************************************
class CFragmentCacheCoverage
{
public:
	CFragmentCacheCoverage() { Reset(); }

	void			ExtendCoverage( u32 address, u32 len );
	bool			IsCovered( u32 address, u32 len ) const;

	void			Reset();

private:
	static u32		AddressToIndex( u32 address );

private:
	static const u32 BASE_ADDRESS = 0x80000000;

	static const u32 MEMORY_8_MEG = 8*1024*1024;
	static const u32 MEM_USAGE_SHIFT = 12;		// 4k
	static const u32 NUM_MEM_USAGE_ENTRIES = MEMORY_8_MEG >> MEM_USAGE_SHIFT;

	bool			mCacheCoverage[ NUM_MEM_USAGE_ENTRIES ];
};

//*************************************************************************************
//
//*************************************************************************************
class CFragmentCache
{
public:
	CFragmentCache();
	~CFragmentCache();

#ifdef DAEDALUS_DEBUG_DYNAREC
	CFragment *				LookupFragment( u32 address ) const;
#endif
	CFragment *				LookupFragmentQ( u32 address ) const;
	void					InsertFragment( CFragment * p_fragment );

	u32						GetCacheSize() const					{ return mFragments.size(); }
	void					Clear();

#ifdef DAEDALUS_DEBUG_DYNAREC
	void					DumpStats( const char * outputdir ) const;
#endif

	u32						GetMemoryUsage() const					{ return mMemoryUsage; }

	CCodeBufferManager *	GetCodeBufferManager() const			{ return mpCodeBufferManager; }

	bool					ShouldInvalidateOnWrite( u32 address, u32 length ) const;

private:
	struct SFragmentEntry
	{
		SFragmentEntry( u32 address, CFragment * fragment )
			:	Address( address )
			,	Fragment( fragment )
		{
		}

		bool operator<( const SFragmentEntry & rhs ) const
		{
			return Address < rhs.Address;
		}

		u32			Address;
		CFragment *	Fragment;
	};

	typedef std::vector< SFragmentEntry >	FragmentVec;
	FragmentVec				mFragments;			// Sorted on Address

	u32						mMemoryUsage;
	u32						mInputLength;
	u32						mOutputLength;

	typedef std::vector< CJumpLocation >	JumpList;
	typedef std::map< u32, JumpList >		JumpMap;
	JumpMap					mJumpMap;

	mutable u32				mCachedFragmentAddress;
	mutable CFragment *		mpCachedFragment;

	static const u32 HASH_TABLE_BITS = 15;
	static const u32 HASH_TABLE_SIZE = 1<<HASH_TABLE_BITS;

	//Low 2 bits will always be 0, remove this redundancy (the amount of folding depends on the hash table size)
	//#define MakeHashIdx( addr ) (((addr >> (2 * HASH_TABLE_BITS + 2)) ^ (addr >> (HASH_TABLE_BITS + 2)) ^ addr >> 2 ) & (HASH_TABLE_SIZE-1))
	#define MakeHashIdx( addr ) (((addr >> (HASH_TABLE_BITS + 2)) ^ addr >> 2 ) & (HASH_TABLE_SIZE-1))

	mutable FHashT			mpCacheHashTable[HASH_TABLE_SIZE];

	CCodeBufferManager *	mpCodeBufferManager;

	CFragmentCacheCoverage	mCacheCoverage;
};

extern CFragmentCache				gFragmentCache;
#endif
