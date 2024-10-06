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

#include <algorithm>
#include <fstream>

#include "Base/Types.h"
#include "Core/Dynamo.h"




#include "Core/CPU.h"
			// For REG_?? defines
#include "Core/Memory.h"
#include "Core/Interrupt.h"
#include "Core/R4300.h"
#include "Debug/Registers.h"		
#include "Interface/ConfigOptions.h"
#include "Debug/DBGConsole.h"
#include "Debug/DebugLog.h"
#include "DynaRec/DynaRecProfile.h"
#include "DynaRec/Fragment.h"
#include "DynaRec/FragmentCache.h"
#include "DynaRec/TraceRecorder.h"
#include "OSHLE/patch.h"				// GetCorrectOp
#include "Ultra/ultra_R4300.h"

#include "Base/Macros.h"
#include "Utility/Profiler.h"
#include "Debug/Synchroniser.h"

#ifdef DAEDALUS_ENABLE_DYNAREC

// These values are very sensitive to change in some games so be carefull!!! //Corn
// War God is sensitive to gHotTraceThreshold
// PD is sensitive to gMaxHotTraceMapSize
//
// Banjo Tooie needs a larger cache size
// BUT leave PSP cache size untouched for now
#ifdef DAEDALUS_PSP
#define TRACE_SIZE 512
#else
#define TRACE_SIZE 1024
#endif

static const u32					gMaxFragmentCacheSize = (8192 + 1024); //Maximum amount of fragments in the cache
static const u32					gMaxHotTraceMapSize = (2048 + TRACE_SIZE);
static const u32					gHotTraceThreshold = 10;	//How many times interpreter has to loop a trace before it becomes hot and sent to dynarec


//std::map< u32, u32, std::less<u32>, MyAllocator >				gHotTraceCountMap;
//std::map< u32, u32, std::less<u32>, boost::pool_allocator<std::pair< const u32, u32 > > >				gHotTraceCountMap;
std::map< u32, u32 >				gHotTraceCountMap {};
CFragmentCache						gFragmentCache {};
static bool							gResetFragmentCache {false};

#ifdef DAEDALUS_DEBUG_DYNAREC
std::map< u32, u32 >				gAbortedTraceReasons;

void								CPU_DumpFragmentCache();
#endif

static void							CPU_HandleDynaRecOnBranch( bool backwards, bool trace_already_enabled );
static void							CPU_UpdateTrace( u32 address, OpCode op_code, bool branch_delay_slot, bool branch_taken );
static void							CPU_CreateAndAddFragment();


#ifdef DAEDALUS_PROFILE_EXECUTION
u32 gFragmentLookupFailure {};
u32 gFragmentLookupSuccess {};
#endif

//*****************************************************************************
//	Indicate that the instruction cache is invalid
//	(we have to dump the dynarec contents and start over, but this is
//	better than crashing :) )
//*****************************************************************************
void  CPU_InvalidateICache()
{
	CPU_ResetFragmentCache();
}

//*****************************************************************************
//
//*****************************************************************************
void CPU_DynarecEnable()
{
	gDynarecEnabled = true;
	gCPUState.AddJob(CPU_CHANGE_CORE);
}

//*****************************************************************************
// If fragments overlap dynarec has to start all over which is very costly
//*****************************************************************************
void  CPU_InvalidateICacheRange( u32 address, u32 length )
{
	if( gFragmentCache.ShouldInvalidateOnWrite( address, length ) )
	{
#ifndef DAEDALUS_SILENT
		printf( "Write to %08x (%d bytes) overlaps fragment cache entries\n", address, length );
#endif
		CPU_ResetFragmentCache();
	}
}


//*****************************************************************************
//	Execute a single MIPS op. The conditionals for the templated arguments
//	are completely optimised away by the compiler.
//
//	DynaRec:		Run this function with dynarec enabled
//	TranslateOp:	Use this to translate breakpoints/patches to original op
//					before execution.
//*****************************************************************************
template< bool TraceEnabled > inline void CPU_EXECUTE_OP()
{

	u8 * p_Instruction = 0;
	CPU_FETCH_INSTRUCTION( p_Instruction, gCPUState.CurrentPC );
	OpCode op_code = *(OpCode*)p_Instruction;

	// Cache instruction base pointer (used for SpeedHack() @ R4300.0)
	gLastAddress = p_Instruction;

#ifdef DAEDALUS_BREAKPOINTS_ENABLED
	op_code = GetCorrectOp( op_code );
#endif

	#ifdef DAEDALUS_ENABLE_SYNCHRONISATION
	SYNCH_POINT( DAED_SYNC_REG_PC, gCPUState.CurrentPC, "Program Counter doesn't match" );
	SYNCH_POINT( DAED_SYNC_FRAGMENT_PC, gCPUState.CurrentPC + gCPUState.Delay, "Program Counter/Delay doesn't match while interpreting" );
	SYNCH_POINT( DAED_SYNC_REG_PC, gCPUState.CPUControl[C0_COUNT]._u32, "Count doesn't match" );
	#endif
	if( TraceEnabled )
	{
		#ifdef DAEDALUS_ENABLE_ASSERTS
		DAEDALUS_ASSERT( gTraceRecorder.IsTraceActive(), "If TraceEnabled is set, trace should be active" );
		#endif
		u32		pc( gCPUState.CurrentPC ) ;
		bool	branch_delay_slot( gCPUState.Delay == EXEC_DELAY );

		R4300_ExecuteInstruction(op_code);
		gGPR[0]._u64 = 0;	//Ensure r0 is zero

		bool	branch_taken( gCPUState.Delay == DO_DELAY );

		CPU_UpdateTrace( pc, op_code, branch_delay_slot, branch_taken );
	}
	else
	{
		#ifdef DAEDALUS_ENABLE_ASSERTS
		DAEDALUS_ASSERT( !gTraceRecorder.IsTraceActive(), "If TraceEnabled is not set, trace should be inactive" );
		#endif
		R4300_ExecuteInstruction(op_code);
		gGPR[0]._u64 = 0;	//Ensure r0 is zero

#ifdef DAEDALUS_PROFILE_EXECUTION
		gTotalInstructionsEmulated++;
#endif
	}
	#ifdef DAEDALUS_ENABLE_SYNCHRONISATION
	SYNCH_POINT( DAED_SYNC_REGS, CPU_ProduceRegisterHash(), "Registers don't match" );
	#endif
	// Increment count register
	gCPUState.CPUControl[C0_COUNT]._u32 = gCPUState.CPUControl[C0_COUNT]._u32 + COUNTER_INCREMENT_PER_OP;

	if (CPU_ProcessEventCycles( COUNTER_INCREMENT_PER_OP ) )
	{
		CPU_HANDLE_COUNT_INTERRUPT();
	}

	switch (gCPUState.Delay)
	{
	case DO_DELAY:
		// We've got a delayed instruction to execute. Increment
		// PC as normal, so that subsequent instruction is executed
		INCREMENT_PC();
		gCPUState.Delay = EXEC_DELAY;

		break;
	case EXEC_DELAY:
		{
			bool	backwards( gCPUState.TargetPC <= gCPUState.CurrentPC );

			// We've just executed the delayed instr. Now carry out jump as stored in gCPUState.TargetPC;
			CPU_SetPC(gCPUState.TargetPC);
			gCPUState.Delay = NO_DELAY;

			CPU_HandleDynaRecOnBranch( backwards, TraceEnabled );
		}
		break;
	case NO_DELAY:
		// Normal operation - just increment the PC
		INCREMENT_PC();
		break;
	}
}


//*****************************************************************************
//
//*****************************************************************************
void	CPU_ResetFragmentCache()
{
	// Need to make sure this happens at a safe point, so we use a flag
	gResetFragmentCache	= true;
}

//*****************************************************************************
// Keep executing instructions until there are other tasks to do (i.e. gCPUState.GetStuffToDo() is set)
// Process these tasks and loop
//*****************************************************************************
template < bool DynaRec, bool TraceEnabled > void CPU_Go()
{
	DAEDALUS_PROFILE( __FUNCTION__ );

	while (CPU_KeepRunning())
	{
		//
		// Keep executing ops as long as there's nothing to do
		//
		u32	stuff_to_do( gCPUState.GetStuffToDo() );
		while(stuff_to_do == 0)
		{
			CPU_EXECUTE_OP< TraceEnabled >();

			stuff_to_do = gCPUState.GetStuffToDo();
		}

		if( TraceEnabled && (stuff_to_do != CPU_CHANGE_CORE) )
		{
			if(gTraceRecorder.IsTraceActive())
			{
#ifdef DAEDALUS_DEBUG_DYNAREC
				u32 start_address( gTraceRecorder.GetStartTraceAddress() );
				//DBGConsole_Msg( 0, "Aborting tracing of [R%08x] - StuffToDo is %08x", start_address, stuff_to_do );

				gAbortedTraceReasons[ start_address ] = stuff_to_do;
#endif

#ifdef ALLOW_TRACES_WHICH_EXCEPT
				if(stuff_to_do == CPU_CHECK_INTERRUPTS && gCPUState.Delay == NO_DELAY )		// Note checking for exactly equal, not just that it's set
				{
					//DBGConsole_Msg( 0, "Adding chunk at %08x after interrupt\n", gTraceRecorder.GetStartTraceAddress() );
					gTraceRecorder.StopTrace( gCPUState.CurrentPC );
					CPU_CreateAndAddFragment();
				}
#endif

				gTraceRecorder.AbortTrace();		// Abort any traces that were terminated through an interrupt etc
			}
			CPU_SelectCore();
		}

		if (CPU_CheckStuffToDo())
			break;
	}
}

#ifdef DAEDALUS_DEBUG_DYNAREC

struct SAddressHitCount
{
	u32		Address = 0;
	u32		HitCount = 0;

	SAddressHitCount( u32 address, u32 hitcount ) : Address( address ), HitCount( hitcount ) {}

	u32		GetAbortReason() const
	{
		std::map<u32, u32>::const_iterator		it( gAbortedTraceReasons.find( Address ) );
		if( it != gAbortedTraceReasons.end() )
		{
			return it->second;
		}

		return 0;
	}
};

bool SortByHitCount( const SAddressHitCount & a, const SAddressHitCount & b )
{
	return a.HitCount > b.HitCount;
}

//*****************************************************************************
//
//*****************************************************************************
void	CPU_DumpFragmentCache()
{

	std::filesystem::path dynarecDump = setBasePath("DynarecDump");
	std::filesystem::create_directory(dynarecDump);

	dynarecDump /= "hot_trace_map.html";

	
	std::ofstream fh(dynarecDump, std::ios::in);

	if (fh.is_open())
	{
		std::vector< SAddressHitCount >	hit_counts;

		hit_counts.reserve( gHotTraceCountMap.size() );

		for(std::map<u32,u32>::const_iterator it = gHotTraceCountMap.begin(); it != gHotTraceCountMap.end(); ++it )
		{
			hit_counts.push_back( SAddressHitCount( it->first, it->second ) );
		}

		std::sort( hit_counts.begin(), hit_counts.end(), SortByHitCount );

		fh <<  "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">";
		fh <<  "<html xmlns=\"http://www.w3.org/1999/xhtml\">\n";
		fh <<  "<head><title>Hot Trace Map</title>\n";
		fh <<  "<link rel=\"stylesheet\" href=\"default.css\" type=\"text/css\" media=\"all\" />\n";
		fh <<  "</head><body>\n";
		fh <<  "<h1>Hot Trace Map</h1>\n";
		fh <<  "<div align=\"center\"><table>\n";
		fh <<  "<tr><th>Address</th><th>Hit Count</th><th>Abort Reason</th></tr>\n";

		for( u32 i = 0; i < hit_counts.size(); ++i )
		{
			const SAddressHitCount & info( hit_counts[ i ] );

			u32		abort_reason( info.GetAbortReason() );

			fprintf( fh, "<tr><td>%08x</td><td>%d</td>\n", info.Address, info.HitCount );

			fh << "<td>";
			if(abort_reason & CPU_CHECK_EXCEPTIONS)		{ fh <<  " Exception"; }
			if(abort_reason & CPU_CHECK_INTERRUPTS)		{ fh <<  " Interrupt"; }
			if(abort_reason & CPU_STOP_RUNNING)			{ fh <<  " StopRunning"; }
			if(abort_reason & CPU_CHANGE_CORE)			{ fh << " ChangeCore"; }
			fh << "/td></tr>\n";

			//if( info.HitCount >= gHotTraceThreshold )
		}
		fh << "</table></div>\n";
		fh << "</body></html>\n";

	}	

	gFragmentCache.DumpStats( "DynarecDump/" );
}
#endif

//*****************************************************************************
//
//*****************************************************************************
void CPU_CreateAndAddFragment()
{
	// std::shared_ptr<CFragment> p_fragment( gTraceRecorder.CreateFragment( gFragmentCache.GetCodeBufferManager() ) );
	CFragment * p_fragment( gTraceRecorder.CreateFragment( gFragmentCache.GetCodeBufferManager() ) );

	if( p_fragment != nullptr )
	{
		gHotTraceCountMap.erase( p_fragment->GetEntryAddress() );
		gFragmentCache.InsertFragment( p_fragment );

		//DBGConsole_Msg( 0, "Inserted hot trace at [R%08x]! (size is %d. %dKB)", p_fragment->GetEntryAddress(), gFragmentCache.GetCacheSize(), gFragmentCache.GetMemoryUsage() / 1024 );
	}
}

//*****************************************************************************
//
//*****************************************************************************
void CPU_UpdateTrace( u32 address, OpCode op_code, bool branch_delay_slot, bool branch_taken )
{
	#ifdef DAEDALUS_PROFILE
	DAEDALUS_PROFILE( "CPU_UpdateTrace" );
		#endif
	#ifdef DAEDALUS_ENABLE_ASSERTS
	DAEDALUS_ASSERT_Q( (gCPUState.Delay == EXEC_DELAY) == branch_delay_slot );
	#endif


#ifdef DAEDALUS_DEBUG_DYNAREC
	CFragment * p_address_fragment( gFragmentCache.LookupFragment( address ) );
#else
	CFragment * p_address_fragment( gFragmentCache.LookupFragmentQ( address ) );
#endif
	if( gTraceRecorder.UpdateTrace( address, branch_delay_slot, branch_taken, op_code, p_address_fragment ) == CTraceRecorder::UTS_CREATE_FRAGMENT )
	{
		CPU_CreateAndAddFragment();
		#ifdef DAEDALUS_ENABLE_ASSERTS
		DAEDALUS_ASSERT( !gTraceRecorder.IsTraceActive(), "Why is a trace still active?" );
		#endif
		CPU_SelectCore();
	}
	#ifdef DAEDALUS_ENABLE_ASSERTS
	else
	{
		DAEDALUS_ASSERT( gTraceRecorder.IsTraceActive(), "The trace should still be enabled" );
	}
	#endif
}

//*****************************************************************************
//
//*****************************************************************************
void CPU_HandleDynaRecOnBranch( bool backwards, bool trace_already_enabled )
{
	#ifdef DAEDALUS_PROFILE
	DAEDALUS_PROFILE( "CPU_HandleDynaRecOnBranch" );
	#endif
	bool	start_of_trace( false );

	if( backwards )
	{
		start_of_trace = true;
	}

	bool	change_core( false );
	#ifdef DAEDALUS_LOG
	DAED_LOG( DEBUG_DYNAREC_CACHE, "CPU_HandleDynaRecOnBranch" );
	#endif

	while( gCPUState.GetStuffToDo() == 0 && gCPUState.Delay == NO_DELAY )
	{
		#ifdef DAEDALUS_ENABLE_DYNAREC_PROFILE
		DAEDALUS_ASSERT( gCPUState.Delay == NO_DELAY, "Why are we entering with a delay slot active?" );
		u32			entry_count( gCPUState.CPUControl[C0_COUNT]._u32 ); // Just used DYNAREC_PROFILE_ENTEREXIT
#endif
		u32			entry_address( gCPUState.CurrentPC );
#ifdef DAEDALUS_DEBUG_DYNAREC
		CFragment * p_fragment( gFragmentCache.LookupFragment( entry_address ) );
#else
		CFragment * p_fragment( gFragmentCache.LookupFragmentQ( entry_address ) );
#endif
		if( p_fragment != nullptr )
		{
		#ifdef DAEDALUS_PROFILE_EXECUTION
			gFragmentLookupSuccess++;
		#endif

		// Check if another trace is active and we're about to enter
			if( gTraceRecorder.IsTraceActive() )
			{
				gTraceRecorder.StopTrace( gCPUState.CurrentPC );
				CPU_CreateAndAddFragment();

				// We need to change the core when exiting
				change_core = true;
			}

			p_fragment->Execute();

			DYNAREC_PROFILE_ENTEREXIT( entry_address, gCPUState.CurrentPC, gCPUState.CPUControl[C0_COUNT]._u32 - entry_count );

			start_of_trace = true;
		}
		else
		{
		#ifdef DAEDALUS_PROFILE_EXECUTION
			gFragmentLookupFailure++;
		#endif
			if( start_of_trace )
			{
				start_of_trace = false;

				if( !gTraceRecorder.IsTraceActive() )
				{
					if (gResetFragmentCache)
					{
#ifdef DAEDALUS_ENABLE_OS_HOOKS
						//Don't reset the cache if there is no fragment except OSHLE function stubs
						if (gFragmentCache.GetCacheSize() >= gNumOfOSFunctions)
#else
						if(true)
#endif
						{
							gFragmentCache.Clear();
							gHotTraceCountMap.clear();		// Makes sense to clear this now, to get accurate usage stats
#ifdef DAEDALUS_ENABLE_OS_HOOKS
							Patch_PatchAll();
#endif
						}
#ifdef DAEDALUS_DEBUG_CONSOLE
						else
						{
							DBGConsole_Msg(0, "Safely skipped one flush");
						}
#endif
						gResetFragmentCache = false;
					}

					if( gFragmentCache.GetCacheSize() > gMaxFragmentCacheSize)
					{
						gFragmentCache.Clear();
						gHotTraceCountMap.clear();		// Makes sense to clear this now, to get accurate usage stats
#ifdef DAEDALUS_ENABLE_OS_HOOKS
						Patch_PatchAll();
#endif
					}

					// If there is no fragment for this target, start tracing
					u32 trace_count( ++gHotTraceCountMap[ gCPUState.CurrentPC ] );
					if( gHotTraceCountMap.size() >= gMaxHotTraceMapSize )
					{
						#ifdef DAEDALUS_DEBUG_CONSOLE
						DBGConsole_Msg( 0, "Hot trace cache hit %d, dumping", gHotTraceCountMap.size() );
						#endif
						gHotTraceCountMap.clear();
						gFragmentCache.Clear();
#ifdef DAEDALUS_ENABLE_OS_HOOKS
						Patch_PatchAll();
#endif
					}
					else if( trace_count == gHotTraceThreshold )
					{
						//DBGConsole_Msg( 0, "Identified hot trace at [R%08x]! (size is %d)", gCPUState.CurrentPC, gHotTraceCountMap.size() );
						gTraceRecorder.StartTrace( gCPUState.CurrentPC );

						if(!trace_already_enabled)
						{
							change_core = true;
						}
						DAED_LOG( DEBUG_DYNAREC_CACHE, "StartTrace( %08x )", gCPUState.CurrentPC );
					}
#ifdef DAEDALUS_DEBUG_DYNAREC
					else if( trace_count > gHotTraceThreshold )
					{
						if(gAbortedTraceReasons.find( gCPUState.CurrentPC ) != gAbortedTraceReasons.end() )
						{
							u32 reason [[maybe_unused]] = gAbortedTraceReasons[ gCPUState.CurrentPC ];
							//DBGConsole_Msg( 0, "Hot trace at [R%08x] has count of %d! (reason is %x) size %d", gCPUState.CurrentPC, trace_count, reason, gHotTraceCountMap.size( ) );
							DAED_LOG( DEBUG_DYNAREC_CACHE, "Hot trace at %08x has count of %d! (reason is %x) size %d", gCPUState.CurrentPC, trace_count, reason, gHotTraceCountMap.size( ) );
						}
						else
						{
							DAED_LOG( DEBUG_DYNAREC_CACHE, "Hot trace at %08x has count of %d! (reason is UNKNOWN!)", gCPUState.CurrentPC, trace_count );
						}
					}
#endif //DAEDALUS_DEBUG_DYNAREC
				}
			}
			#ifdef DAEDALUS_LOG
			else
			{
				DAED_LOG( DEBUG_DYNAREC_CACHE, "Not start of trace" );
			}
			#endif
			break;
		}
	}

	if(change_core)
	{
		CPU_SelectCore();
	}
}

void Dynamo_Reset()
{
	gHotTraceCountMap.clear();
	gFragmentCache.Clear();
	gResetFragmentCache = false;
	gTraceRecorder.AbortTrace();
#ifdef DAEDALUS_DEBUG_DYNAREC
	gAbortedTraceReasons.clear();
#endif
}

void Dynamo_SelectCore()
{
	bool trace_enabled = gTraceRecorder.IsTraceActive();

	if (trace_enabled)
	{
		g_pCPUCore = CPU_Go< true, true >;
	}
	else
	{
		g_pCPUCore = CPU_Go< true, false >;
	}
}

#else

void CPU_ResetFragmentCache() {}
void Dynamo_Reset() {}
void  CPU_InvalidateICacheRange( u32 address [[maybe_unused]], u32 length [[maybe_unused]] ) {}

#endif //DAEDALUS_ENABLE_DYNAREC
