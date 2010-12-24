/*
Copyright (C) 2009 StrmnNrmn

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
#include "BatchTest.h"

#ifdef DAEDALUS_BATCH_TEST_ENABLED

#include "System.h"
#include "Core/ROM.h"
#include "Core/CPU.h"

#include "Debug/Dump.h"

#include "HLEGraphics/DLParser.h"

#include "Utility/IO.h"
#include "Utility/Timer.h"
#include "Utility/Timing.h"
#include "Utility/Hash.h"
#include "Utility/ROMFile.h"

#include <vector>
#include <string>
#include <algorithm>

#include <stdarg.h>

void MakeRomList( const char * romdir, std::vector< std::string > & roms )
{
	IO::FindHandleT		find_handle;
	IO::FindDataT		find_data;
	if(IO::FindFileOpen( romdir, &find_handle, find_data ))
	{
		do
		{
			const char * filename( find_data.Name );
			if( IsRomfilename( filename ) )
			{
				char rompath[MAX_PATH+1];
				
				IO::Path::Combine( rompath, romdir, filename );

				roms.push_back( rompath );
			}
		}
		while(IO::FindFileNext( find_handle, find_data ));

		IO::FindFileClose( find_handle );
	}

	//_finddata_t		data;
	//_findfirst("foo", &data )
}

FILE * gBatchFH = NULL;
FILE * gRomLogFH = NULL;
CBatchTestEventHandler * gBatchTestEventHandler = NULL;

static EAssertResult BatchAssertHook( const char * expression, const char * file, unsigned int line, const char * msg, ... )
{
	char buffer[ 1024 ];
	va_list va;
	va_start(va, msg);
	vsnprintf( buffer, 1024, msg, va );
	buffer[1023] = 0;
	va_end(va);

	if( gBatchTestEventHandler )
		return gBatchTestEventHandler->OnAssert( expression, file, line, buffer );

	return AR_IGNORE;
}

CBatchTestEventHandler * BatchTest_GetHandler()
{
	return gBatchTestEventHandler;
}

static void MakeNewLogFilename( char (&filepath)[MAX_PATH+1], const char * rundir )
{
	u32 count = 0;
	do 
	{
		char filename[MAX_PATH+1];

		sprintf( filename, "log%04d.txt", count );
		++count;

		IO::Path::Combine(filepath, rundir, filename);
	}
	while( IO::File::Exists( filepath ) );
}

static void SprintRunDirectory( char (&rundir)[MAX_PATH+1], const char * batchdir, u32 run_id )
{
	char filename[MAX_PATH+1];
	sprintf( filename, "run%04d", run_id );
	IO::Path::Combine(rundir, batchdir, filename);
}

static bool MakeRunDirectory( char (&rundir)[MAX_PATH+1], const char * batchdir )
{
	// Find an unused directory
	for( u32 run_id = 0; run_id < 100; ++run_id )
	{
		SprintRunDirectory( rundir, batchdir, run_id );

		// Skip if it already exists as a file or directory
		if( IO::Directory::Create( rundir ) )
			return true;
	}

	return false;
}

void BatchTestMain( int argc, char* argv[] )
{
	//ToDo: Allow other directories and configuration
	const char * romdir( "host1:/" );

	bool	random_order( false );		// Whether to randomise the order of processing, to help avoid hangs
	bool	update_results( false );	// Whether to update existing results
	s32		run_id( -1 );				// New run by default

	for(int i = 1; i < argc; ++i )
	{
		const char * arg( argv[i] );
		if( *arg == '-' )
		{
			++arg;
			if( strcmp( arg, "rand" ) == 0 || strcmp( arg, "random" ) == 0 )
			{
				random_order = true;
			}
			else if( strcmp( arg, "u" ) == 0 || strcmp( arg, "update" ) == 0 )
			{
				update_results = true;
			}
			else if( strcmp( arg, "r" ) == 0 || strcmp( arg, "run" ) == 0 )
			{
				if( i+1 < argc )
				{
					++i;	// Consume next arg
					run_id = atoi( argv[i] );
				}
			}
		}
	}

	char batchdir[MAX_PATH+1];
	Dump_GetDumpDirectory( batchdir, "batch" );

	char rundir[MAX_PATH+1];
	if( run_id < 0 )
	{
		if( !MakeRunDirectory( rundir, batchdir ) )
		{
			printf( "Couldn't start a new run\n" );
			return;
		}
	}
	else
	{
		SprintRunDirectory( rundir, batchdir, run_id );
		if( !IO::Directory::IsDirectory( rundir ) )
		{
			printf( "Couldn't resume run %d\n", run_id );
			return;
		}
	}

	gBatchTestEventHandler = new CBatchTestEventHandler();

	char logpath[MAX_PATH+1];
	MakeNewLogFilename( logpath, rundir );
	gBatchFH = fopen(logpath, "w");
	if( !gBatchFH )
	{
		printf( "Unable to open '%s' for writing", logpath );
		return;
	}

	std::vector< std::string > roms;
	MakeRomList( romdir, roms );

	u64 time;
	if( NTiming::GetPreciseTime( &time ) )
		srand( (int)time );

	CTimer	timer;

	//
	//	Set up an assert hook to capture all asserts
	//
	SetAssertHook( BatchAssertHook );

	char tmpfilepath[MAX_PATH+1];
	IO::Path::Combine( tmpfilepath, rundir, "tmp.tmp" );

	while( !roms.empty() )
	{
		gBatchTestEventHandler->Reset();

		u32 idx( 0 );

		// Picking roms in a random order means we can work around roms which crash the emulator a little more easily
		if( random_order )
			idx = rand() % roms.size();

		std::string	r;
		r.swap( roms[idx] );
		roms.erase( roms.begin() + idx );

		// Make a filename of the form: '<rundir>/<romfilename>.txt'
		char rom_logpath[MAX_PATH+1];
		IO::Path::Combine( rom_logpath, rundir, IO::Path::FindFileName( r.c_str() ) );
		IO::Path::AddExtension( rom_logpath, ".txt" );

		bool	result_exists( IO::File::Exists( rom_logpath ) );

		if( !update_results && result_exists )
		{
			// Already exists, skip
			fprintf( gBatchFH, "\n\n%#.3f: Skipping %s - log already exists\n", timer.GetElapsedSecondsSinceReset(), r.c_str() );
		}
		else
		{
			fprintf( gBatchFH, "\n\n%#.3f: Processing: %s\n", timer.GetElapsedSecondsSinceReset(), r.c_str() );

			gRomLogFH = fopen( tmpfilepath, "w" );
			if( !gRomLogFH )
			{
				fprintf( gBatchFH, "#%.3f: Unable to open temp file\n", timer.GetElapsedSecondsSinceReset() );
			}
			else
			{
				fflush( gBatchFH );

				// TODO: use ROM_GetRomDetailsByFilename and the alternative form of ROM_LoadFile with overridden preferences (allows us to test if roms break by changing prefs)
				System_Open( r.c_str() );

				CPU_Run();

				System_Close();

				const char * reason( CBatchTestEventHandler::GetTerminationReasonString( gBatchTestEventHandler->GetTerminationReason() ) );

				fprintf( gBatchFH, "%#.3f: Finished running: %s - %s\n", timer.GetElapsedSecondsSinceReset(), r.c_str(), reason );

				// Copy temp file over rom_logpath
				gBatchTestEventHandler->PrintSummary( gRomLogFH );
				fclose( gRomLogFH );
				gRomLogFH = NULL;
				if( result_exists )
				{
					IO::File::Delete( rom_logpath );
				}
				if( !IO::File::Move( tmpfilepath, rom_logpath ) )
				{
					fprintf( gBatchFH, "%#.3f: Coping %s -> %s failed\n", timer.GetElapsedSecondsSinceReset(), tmpfilepath, rom_logpath );
				}
			}

		}

	}

	SetAssertHook( NULL );

	fclose( gBatchFH );
	gBatchFH = NULL;

	delete gBatchTestEventHandler;
}

// Should make these configurable
const u32 MAX_DLS = 10;
const u32 MAX_VBLS_WITHOUT_DL = 1000;
const f32 BATCH_TIME_LIMIT = 60.0f;

CBatchTestEventHandler::CBatchTestEventHandler()
:	mNumDisplayListsCompleted( 0 )
,	mNumVerticalBlanksSinceDisplayList( 0 )
,	mTerminationReason( TR_UNKNOWN )
{

}

void CBatchTestEventHandler::Reset()
{
	mNumDisplayListsCompleted = 0;
	mNumVerticalBlanksSinceDisplayList = 0;
	mTimer.Reset();
	mTerminationReason = TR_UNKNOWN;
	mAsserts.clear();
}

void CBatchTestEventHandler::Terminate( ETerminationReason reason )
{
	mTerminationReason = reason;
	CPU_Halt( "End of batch run" );
}

void CBatchTestEventHandler::OnDisplayListComplete()
{
	++mNumDisplayListsCompleted;
	mNumVerticalBlanksSinceDisplayList = 0;
	if( MAX_DLS != 0 && mNumDisplayListsCompleted >= MAX_DLS )
	{
		Terminate( TR_REACHED_DL_COUNT );
	}
}

void CBatchTestEventHandler::OnVerticalBlank()
{
	++mNumVerticalBlanksSinceDisplayList;
	if( mNumVerticalBlanksSinceDisplayList > MAX_VBLS_WITHOUT_DL )
	{
		Terminate( TR_TOO_MANY_VBLS_WITH_NO_DL );
	}

	if( mTimer.GetElapsedSecondsSinceReset() > BATCH_TIME_LIMIT )
	{
		Terminate( TR_TIME_LIMIT_REACHED );
	}
}

EAssertResult CBatchTestEventHandler::OnAssert( const char * expression, const char * file, unsigned int line, const char * formatted_msg )
{
	u32		assert_hash( murmur2_hash( (const u8 *)file, strlen( file ), line ) );

	std::vector<u32>::iterator	it( std::lower_bound( mAsserts.begin(), mAsserts.end(), assert_hash ) );
	if( it == mAsserts.end() || *it != assert_hash )
	{
		if( gRomLogFH )
		{
			fprintf( gRomLogFH, "! Assert Failed: Location: %s(%d), [%s] %s\n", file, line, expression, formatted_msg );
		}

		mAsserts.insert( it, assert_hash );
	}

	// Don't return AR_IGNORE as this prevents asserts firing for subsequent roms
	return AR_IGNORE_ONCE;
}

void CBatchTestEventHandler::OnDebugMessage( const char * msg )
{
	if( gRomLogFH )
	{
		fputs( msg, gRomLogFH );
	}
}

const char * CBatchTestEventHandler::GetTerminationReasonString( ETerminationReason reason )
{
	switch( reason )
	{
	case TR_UNKNOWN:						return "Unknown";
	case TR_REACHED_DL_COUNT:				return "Reached display list count";
	case TR_TIME_LIMIT_REACHED:				return "Time limit reached";
	case TR_TOO_MANY_VBLS_WITH_NO_DL:		return "Too many vertical blanks without a display list";
	}

	DAEDALUS_ERROR( "Unhandled reason" );
	return "Unknown";
}

void CBatchTestEventHandler::PrintSummary( FILE * fh )
{
	bool			success( mTerminationReason >= 0 );
	const char *	reason( GetTerminationReasonString( mTerminationReason ) );

	fprintf( fh, "\n\nSummary:\n--------\n\n" );
	fprintf( fh, "Termination Reason: [%s] - %s\n", success ? " OK " : "FAIL", reason );
	fprintf( fh, "Display Lists Completed: %d / %d\n", mNumDisplayListsCompleted, MAX_DLS );
}


#endif // DAEDALUS_BATCH_TEST_ENABLED
