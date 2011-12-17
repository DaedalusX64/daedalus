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

#include "stdafx.h"
#include "FragmentCache.h"

#include "Fragment.h"
#include "CodeBufferManager.h"
#include "DynaRecProfile.h"

#include "Debug/DBGConsole.h"

#include "Utility/Profiler.h"
#include "Utility/IO.h"

#include "AssemblyUtils.h"


#include <algorithm>

using namespace AssemblyUtils;

//*************************************************************************************
//
//*************************************************************************************
CFragmentCache::CFragmentCache()
:	mMemoryUsage( 0 )
,	mInputLength( 0 )
,	mOutputLength( 0 )
,	mCachedFragmentAddress( 0 )
,	mpCachedFragment( NULL )
{
	memset( mpCacheHashTable, 0, sizeof(mpCacheHashTable) );

	mFragments.reserve( 2000 );

	mpCodeBufferManager = CCodeBufferManager::Create();
	if(mpCodeBufferManager != NULL)
	{
		mpCodeBufferManager->Initialise();
	}
}

//*************************************************************************************
//
//*************************************************************************************
CFragmentCache::~CFragmentCache()
{
	Clear();

	mpCodeBufferManager->Finalise();
	delete mpCodeBufferManager;
}

//*************************************************************************************
//
//*************************************************************************************
#if DAEDALUS_DEBUG_DYNAREC
CFragment * CFragmentCache::LookupFragment( u32 address ) const
{
	DAEDALUS_PROFILE( "CFragmentCache::LookupFragment" );

	if( address != mCachedFragmentAddress )
	{
		mCachedFragmentAddress = address;

		// check if in hash table
		u32 ix = MakeHashIdx( address );

		if ( address != mpCacheHashTable[ix].addr )
		{
			SFragmentEntry				entry( address, NULL );
			FragmentVec::const_iterator	it( std::lower_bound( mFragments.begin(), mFragments.end(), entry ) );
			if( it != mFragments.end() && it->Address == address )
			{
				mpCachedFragment = it->Fragment;
			}
			else
			{
				mpCachedFragment = NULL;
			}

			// put in hash table
			mpCacheHashTable[ix].addr = address;
			mpCacheHashTable[ix].ptr = reinterpret_cast< u32 >( mpCachedFragment );
		}
		else
		{
			mpCachedFragment = reinterpret_cast< CFragment * >( mpCacheHashTable[ix].ptr );
		}
	}

	CFragment * p = mpCachedFragment;

	DYNAREC_PROFILE_LOGLOOKUP( address, p );

	return p;
}
#endif
//*************************************************************************************
//
//*************************************************************************************
CFragment * CFragmentCache::LookupFragmentQ( u32 address ) const
{
	DAEDALUS_PROFILE( "CFragmentCache::LookupFragmentQ" );

	if( address != mCachedFragmentAddress )
	{
		mCachedFragmentAddress = address;

		// check if in hash table
		u32 ix = MakeHashIdx( address );

		if ( address != mpCacheHashTable[ix].addr )
		{
			SFragmentEntry				entry( address, NULL );
			FragmentVec::const_iterator	it( std::lower_bound( mFragments.begin(), mFragments.end(), entry ) );
			if( it != mFragments.end() && it->Address == address )
			{
				mpCachedFragment = it->Fragment;
			}
			else
			{
				mpCachedFragment = NULL;
			}

			// put in hash table
			mpCacheHashTable[ix].addr = address;
			mpCacheHashTable[ix].ptr = reinterpret_cast< u32 >( mpCachedFragment );
		}
		else
		{
			mpCachedFragment = reinterpret_cast< CFragment * >( mpCacheHashTable[ix].ptr );
		}
	}

	return mpCachedFragment;
}

//*************************************************************************************
//
//*************************************************************************************
void CFragmentCache::InsertFragment( CFragment * p_fragment )
{
	u32		fragment_address( p_fragment->GetEntryAddress() );

	mCacheCoverage.ExtendCoverage( fragment_address, p_fragment->GetInputLength() );

	SFragmentEntry				entry( fragment_address, NULL );
	FragmentVec::iterator		it( std::lower_bound( mFragments.begin(), mFragments.end(), entry ) );
	DAEDALUS_ASSERT( it == mFragments.end() || it->Address != fragment_address, "A fragment with this address already exists" );
	entry.Fragment = p_fragment;
	mFragments.insert( it, entry );

	// Update the hash table (it stores failed lookups now, so we need to be sure to purge any stale entries in there
	u32 ix = MakeHashIdx( fragment_address );
	mpCacheHashTable[ix].addr = fragment_address;
	mpCacheHashTable[ix].ptr = reinterpret_cast< u32 >( p_fragment );

	// Process any jumps for this before inserting new ones
	JumpMap::iterator	jump_it( mJumpMap.find( fragment_address ) );
	if( jump_it != mJumpMap.end() )
	{
		const JumpList &		jumps( jump_it->second );
		for( JumpList::const_iterator it = jumps.begin(); it != jumps.end(); ++it )
		{
			//DBGConsole_Msg( 0, "Inserting [R%08x], patching jump at %08x ", address, (*it) );
			PatchJumpLongAndFlush( (*it), p_fragment->GetEntryTarget() );
		}

		// All patched - clear
		mJumpMap.erase( jump_it );
	}

	// Finally register any links that this fragment may have
	const FragmentPatchList &	patch_list( p_fragment->GetPatchList() );
	for( FragmentPatchList::const_iterator it = patch_list.begin(); it != patch_list.end(); ++it )
	{
		u32				target_address( it->Address );
		CJumpLocation	jump( it->Jump );

		DAEDALUS_ASSERT( jump.IsSet(), "No exit jump?" );

#if DAEDALUS_DEBUG_DYNAREC
		CFragment * p_fragment( LookupFragment( target_address ) );
#else
		CFragment * p_fragment( LookupFragmentQ( target_address ) );
#endif
		if( p_fragment != NULL )
		{
			PatchJumpLongAndFlush( jump, p_fragment->GetEntryTarget() );

			DAEDALUS_ASSERT( mJumpMap.find( target_address ) == mJumpMap.end(), "Jump map still contains an entry for this" );
		}
		else if( target_address != u32(~0) )
		{
			// Store the address for later processing
			mJumpMap[ target_address ].push_back( jump );
		}
	}

	// Free memoire
	p_fragment->DiscardPatchList();

	// For simulation only
	p_fragment->SetCache( this );

	// Update memory usage etc after discarding patch list
	mMemoryUsage += p_fragment->GetMemoryUsage();
	mInputLength += p_fragment->GetInputLength();
	mOutputLength += p_fragment->GetOutputLength();

#ifndef DAEDALUS_SILENT
	if((mFragments.size() % 100) == 0)
	{
		u32		expansion = 1;
		if(mInputLength > 0)
		{
			expansion = (100 * mOutputLength) / mInputLength;
		}
		DBGConsole_Msg( 0, "Dynarec: %d fragments, %dKB (i:o %d:%d = %d%%)",
			mFragments.size(), mMemoryUsage / 1024, mInputLength / 1024, mOutputLength / 1024, expansion );
	}
#endif
}

//*************************************************************************************
//
//*************************************************************************************
void CFragmentCache::Clear()
{
#ifdef DAEDALUS_DEBUG_CONSOLE
	if(CDebugConsole::IsAvailable())
	{
		DBGConsole_Msg( 0, "Clearing fragment cache of %d fragments", mFragments.size() );
	}
#endif
	// Clear out all the framents
	for(FragmentVec::iterator it = mFragments.begin(); it != mFragments.end(); ++it)
	{
		delete it->Fragment;
	}

	mFragments.erase( mFragments.begin(), mFragments.end() );
	mMemoryUsage = 0;
	mInputLength = 0;
	mOutputLength = 0;
	mCachedFragmentAddress = 0;
	mpCachedFragment = NULL;
	memset( mpCacheHashTable, 0, sizeof(mpCacheHashTable) );
	mJumpMap.clear();

	mCacheCoverage.Reset();

	mpCodeBufferManager->Reset();
}

//*************************************************************************************
//
//*************************************************************************************
bool CFragmentCache::ShouldInvalidateOnWrite( u32 address, u32 length ) const
{
	return mCacheCoverage.IsCovered( address, length );
}

#ifdef DAEDALUS_DEBUG_DYNAREC
//*************************************************************************************
//
//*************************************************************************************
struct SDescendingCyclesSort
{
     bool operator()(CFragment* const & a, CFragment* const & b)
     {
		return b->GetCyclesExecuted() < a->GetCyclesExecuted();
     }
};

//*************************************************************************************
//
//*************************************************************************************
void CFragmentCache::DumpStats( const char * outputdir ) const
{
	typedef std::vector< CFragment * >		FragmentList;
	FragmentList		all_fragments;

	all_fragments.reserve( mFragments.size() );

	u32		total_cycles( 0 );

	// Sort in order of expended cycles
	for(FragmentVec::const_iterator it = mFragments.begin(); it != mFragments.end(); ++it)
	{
		all_fragments.push_back( it->Fragment );
		total_cycles += it->Fragment->GetCyclesExecuted();
	}

	std::sort( all_fragments.begin(), all_fragments.end(), SDescendingCyclesSort() );


	char	filename[ IO::Path::MAX_PATH_LEN + 1 ];
	char	fragments_dir[ IO::Path::MAX_PATH_LEN + 1 ];

	strcpy( fragments_dir, outputdir );
	IO::Path::Append( fragments_dir, "fragments" );
	IO::Directory::EnsureExists( fragments_dir );

	IO::Path::Combine( filename, outputdir, "fragments.html" );

	FILE * fh( fopen( filename, "w" ) );
	if(fh)
	{
		fputs( "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">", fh );
		fputs( "<html xmlns=\"http://www.w3.org/1999/xhtml\">\n", fh );
		fputs( "<head><title>Fragments</title>\n", fh );
		fputs( "<link rel=\"stylesheet\" href=\"default.css\" type=\"text/css\" media=\"all\" />\n", fh );
		fputs( "</head><body>\n", fh );


		fputs( "<h1>Fragments</h1>\n", fh );
		fputs( "<div align=\"center\"><table>\n", fh );
		fputs( "<tr><th>Address</th><th>Loops</th><th>Cycle Count</th><th>Cycle %</th><th>Hit Count</th><th>Input Bytes</th><th>Output Bytes</th><th>Expansion Ratio</th></tr>\n", fh );

		for(FragmentList::const_iterator it = all_fragments.begin(); it != all_fragments.end(); ++it)
		{
			const CFragment * fragment( *it );
			
			if (fragment->GetCyclesExecuted() == 0)
				continue;

			fputs( "<tr>", fh );
			fprintf( fh, "<td><a href=\"fragments//%08x.html\">0x%08x</a></td>", fragment->GetEntryAddress(), fragment->GetEntryAddress() );
			fprintf( fh, "<td>%s</td>", fragment->GetEntryAddress() == fragment->GetExitAddress() ? "*" : "&nbsp;" );
			fprintf( fh, "<td>%d</td>", fragment->GetCyclesExecuted() );
			fprintf( fh, "<td>%#.2f%%</td>", f32( fragment->GetCyclesExecuted() * 100.0f ) / f32( total_cycles ) );
			fprintf( fh, "<td>%d</td>", fragment->GetHitCount() );
			fprintf( fh, "<td>%d</td>", fragment->GetInputLength() );
			fprintf( fh, "<td>%d</td>", fragment->GetOutputLength() );
			fprintf( fh, "<td>%#.2f</td>", f32( fragment->GetOutputLength() ) / f32( fragment->GetInputLength() ) );
			fputs( "</tr>\n", fh );

			char	fragment_path[ IO::Path::MAX_PATH_LEN + 1 ];
			char	fragment_name[ 32+1 ];
			sprintf( fragment_name, "%08x.html", fragment->GetEntryAddress() );
			IO::Path::Combine( fragment_path, fragments_dir, fragment_name );

			FILE * fragment_fh( fopen( fragment_path, "w" ) );
			if( fragment_fh != NULL )
			{
				fragment->DumpFragmentInfoHtml( fragment_fh, total_cycles );
				fclose( fragment_fh );
			}
		}

		fputs( "</table></div>\n", fh );
		fputs( "</body></html>\n", fh );

		fclose( fh );
	}
}
#endif // DAEDALUS_DEBUG_DYNAREC


//*************************************************************************************
//
//*************************************************************************************
#define AddressToIndex( addr ) ((addr - BASE_ADDRESS) >> MEM_USAGE_SHIFT)

//*************************************************************************************
//
//*************************************************************************************
void CFragmentCacheCoverage::ExtendCoverage( u32 address, u32 len )
{
	u32 first_entry( AddressToIndex( address ) );
	u32 last_entry( AddressToIndex( address + len ) );

	// Mark all entries as true
	for( u32 i = first_entry; i <= last_entry && i < NUM_MEM_USAGE_ENTRIES; ++i )
	{
		mCacheCoverage[ i ] = true;
	}
}

//*************************************************************************************
//
//*************************************************************************************
bool CFragmentCacheCoverage::IsCovered( u32 address, u32 len ) const
{
	u32 first_entry( AddressToIndex( address ) );
	u32 last_entry( AddressToIndex( address + len ) );

	// Mark all entries as true
	for( u32 i = first_entry; i <= last_entry && i < NUM_MEM_USAGE_ENTRIES; ++i )
	{
		if( mCacheCoverage[ i ] )
			return true;
	}

	return false;
}

//*************************************************************************************
//
//*************************************************************************************
void CFragmentCacheCoverage::Reset( )
{
	memset( mCacheCoverage, 0, sizeof( mCacheCoverage ) );
}
