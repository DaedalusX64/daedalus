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
#include "TraceRecorder.h"
#include "Fragment.h"
#include "BranchType.h"

#include "Core/CPU.h"			// For dubious use of PC/NewPC
#include "Core/Registers.h"

#include "Debug/DBGConsole.h"

#include "Utility/Profiler.h"

#include "Utility/PrintOpCode.h"

//#define LOG_ABORTED_TRACES

namespace
{
	const u32 INVALID_IDX = u32( ~0 );
	const u32 INDIRECT_EXIT_ADDRESS = u32( ~0 );

	const u32 MAX_TRACE_LENGTH = 1500;
}
CTraceRecorder				gTraceRecorder;

//*************************************************************************************
//
//*************************************************************************************
CTraceRecorder::CTraceRecorder()
:	mTracing( false )
,	mStartTraceAddress( 0 )
,	mExpectedExitTraceAddress( 0 )
,	mActiveBranchIdx( INVALID_IDX )
,	mStopTraceAfterDelaySlot( false )
,	mNeedIndirectExitMap( false )
{
}

//*************************************************************************************
//
//*************************************************************************************
void	CTraceRecorder::StartTrace( u32 address )
{
	DAEDALUS_PROFILE( "CTraceRecorder::StartTrace" );

	DAEDALUS_ASSERT( !mTracing, "We're already tracing" );

	mTraceBuffer.clear();
	mBranchDetails.clear();
	mNeedIndirectExitMap = false;
	mTracing = true;
	mStartTraceAddress = address;
	mActiveBranchIdx = INVALID_IDX;
	mStopTraceAfterDelaySlot = false;
	mExpectedExitTraceAddress = address + 4;
}

//*************************************************************************************
//	
//*************************************************************************************
CTraceRecorder::EUpdateTraceStatus	CTraceRecorder::UpdateTrace( u32 address,
																 bool branch_delay_slot,
																 bool branch_taken,
																 OpCode op_code,
																 CFragment * p_fragment )
{
	DAEDALUS_ASSERT( mTracing, "We're not tracing" );

	bool				want_to_stop( p_fragment != NULL );
	
	if( mTraceBuffer.size() > MAX_TRACE_LENGTH )
	{
		DBGConsole_Msg(0, "Hit max trace size!");
		want_to_stop = true;
	}

	// Terminate if the current instruction is in the fragment cache or the trace reaches a specified size
	if( want_to_stop && (mActiveBranchIdx == INVALID_IDX) )
	{
		DAEDALUS_ASSERT( mActiveBranchIdx == INVALID_IDX, "Exiting trace while in the middle of handling branch!" );
		// Stop immediately so we can be sure of linking up with fragment
		mTracing = false;
		mExpectedExitTraceAddress = address;
		return UTS_CREATE_FRAGMENT;
	}

	//
	//	Figure out whether to terminate this trace after adding this instruction
	//
	bool	stop_trace_on_exit( false );

	//
	//	We want to record the delay slot op for the active branch
	//
	if( mActiveBranchIdx != INVALID_IDX )
	{
		DAEDALUS_ASSERT( mActiveBranchIdx < mBranchDetails.size(), "Branch index is out of bounds" );
		mBranchDetails[ mActiveBranchIdx ].DelaySlotTraceIndex = mTraceBuffer.size();

		if (mBranchDetails[ mActiveBranchIdx ].SpeedHack == SHACK_POSSIBLE)
		{
			if (op_code._u32 == 0)
			{
				mBranchDetails[ mActiveBranchIdx ].SpeedHack = SHACK_SKIPTOEVENT;
			}else if (op_code.op == OP_ADDIU || op_code.op == OP_DADDI
				|| op_code.op == OP_ADDI || op_code.op == OP_DADDIU)
			{
				mBranchDetails[ mActiveBranchIdx ].SpeedHack = SHACK_COPYREG;
			}
		}
		mActiveBranchIdx = INVALID_IDX;
	}

	// If we had to terminate on the last branch (e.g. for an indirect jump)
	// or if we're jumping backwards, terminate the run when we exit
	if( mStopTraceAfterDelaySlot && branch_delay_slot )
	{
		mStopTraceAfterDelaySlot = false;
		stop_trace_on_exit = true;
	}

	//
	//	Update the expected trace exit address
	//	We assume that if we'll exit on the next instruction (assuming this isn't a branch)
	//
	if( !branch_delay_slot )
	{
		mExpectedExitTraceAddress = address + 4;
	}

	//
	//	If this is a branch, we need to determine if it was taken or not.
	//	We store a information about the 'off-trace' target.
	//	If the branch was taken, we need to flip the condition of the
	//	branch so that anything failing the test is directed off our trace
	//
	u32		branch_idx( INVALID_IDX );

	ER4300BranchType	branch_type( GetBranchType( op_code ) );
	if( branch_type != BT_NOT_BRANCH )
	{
		SBranchDetails	details;

		details.Likely = IsBranchTypeLikely( branch_type );

		if( branch_type == BT_ERET )
		{
			stop_trace_on_exit = true;
			details.Eret = true;
			details.Direct = false;
			details.TargetAddress = INDIRECT_EXIT_ADDRESS;

			mExpectedExitTraceAddress = details.TargetAddress;
		}
		else if( !IsConditionalBranch( branch_type ) )
		{
			details.Direct = IsBranchTypeDirect( branch_type );
			details.TargetAddress = gCPUState.TargetPC;
			details.ConditionalBranchTaken = true;

			mExpectedExitTraceAddress = details.TargetAddress;

			if (!details.Direct || gCPUState.TargetPC <= gCPUState.CurrentPC)
			{
				// all indirect call will stop the trace
				mStopTraceAfterDelaySlot = true;
			}

			if (details.Direct && gCPUState.TargetPC == gCPUState.CurrentPC)
			{
				details.SpeedHack = SHACK_POSSIBLE;
			}
		}
		else
		{
			// Must be conditional, direct
			DAEDALUS_ASSERT( IsBranchTypeDirect( branch_type ), "Not expecting an indirect branch here" );

			if( branch_taken )
			{
				// XXXXXX should be able to get this some other way?
				bool	backwards( gCPUState.TargetPC <= gCPUState.CurrentPC );

				if( backwards )
				{
					mStopTraceAfterDelaySlot = true;
				}

				if (gCPUState.TargetPC == gCPUState.CurrentPC)
				{
					details.SpeedHack = SHACK_POSSIBLE;
				}
			}

			u32		branch_target_address( GetBranchTarget( address, op_code, branch_type ) );
			u32		fallthrough_address( address + 8 );

			u32		target_address;
			if( branch_taken )
			{
				// We're following the branch.
				target_address = fallthrough_address;
				mExpectedExitTraceAddress = branch_target_address;
			}
			else
			{
				// We're not following the branch, and falling through.
				target_address = branch_target_address;
				mExpectedExitTraceAddress = fallthrough_address;
			}

			details.ConditionalBranchTaken = branch_taken;
			details.Direct = true;
			details.TargetAddress = target_address;
		}

		if( !details.Direct )
		{
			mNeedIndirectExitMap = true;
		}
			
		mActiveBranchIdx = mBranchDetails.size();
		branch_idx = mBranchDetails.size();
		mBranchDetails.push_back( details );
	}

	StaticAnalysis::RegisterUsage usage;

	StaticAnalysis::Analyse(op_code,usage);

	// Add this op to the trace buffer.
	STraceEntry		entry = { address, op_code, usage, branch_idx, branch_delay_slot };

	mTraceBuffer.push_back( entry );

	if( stop_trace_on_exit )
	{
		DAEDALUS_ASSERT( branch_type == BT_ERET || mActiveBranchIdx == INVALID_IDX, "Exiting trace while in the middle of handling branch!" );

		mTracing = false;
		return UTS_CREATE_FRAGMENT;
	}

	return UTS_CONTINUE_TRACE;
}

//*************************************************************************************
//
//*************************************************************************************
void	CTraceRecorder::StopTrace( u32 exit_address )
{
	DAEDALUS_ASSERT( mTracing, "We're not tracing" );
	DAEDALUS_ASSERT( mActiveBranchIdx == INVALID_IDX, "Stopping trace when a branch is active" );

	mTracing = false;
	mExpectedExitTraceAddress = exit_address;
}

//*************************************************************************************
//
//*************************************************************************************
CFragment *		CTraceRecorder::CreateFragment( CCodeBufferManager * p_manager )
{
	DAEDALUS_PROFILE( "CTraceRecorder::CreateFragment" );

	DAEDALUS_ASSERT( !mTraceBuffer.empty(), "No trace ready for creation?" );

	SRegisterUsageInfo	register_usage;
	Analyse( register_usage );

	CFragment *	p_frament( new CFragment( p_manager, mStartTraceAddress, mExpectedExitTraceAddress, 
		mTraceBuffer, register_usage, mBranchDetails, mNeedIndirectExitMap ) );

	//DBGConsole_Msg( 0, "Inserting hot trace for [R%08x]!", mStartTraceAddress );

	mTracing = false;
	mStartTraceAddress = 0;
	mTraceBuffer.clear();
	mBranchDetails.clear();
	mExpectedExitTraceAddress = 0;
	mActiveBranchIdx = INVALID_IDX;
	mStopTraceAfterDelaySlot = false;
	mNeedIndirectExitMap = false;

	return p_frament;
}

//*************************************************************************************
//
//*************************************************************************************
void	CTraceRecorder::AbortTrace()
{
	DAEDALUS_PROFILE( "CTraceRecorder::AbortTrace" );

	if( mTracing )
	{
#ifdef LOG_ABORTED_TRACES
		FILE * fh( fopen( "aborted_traces.txt", "a" ) );
		if(fh)
		{
			fprintf( fh, "\n\nTrace: (%d ops)\n", mTraceBuffer.size() );

			u32		last_address( mTraceBuffer.size() > 0 ? mTraceBuffer[ 0 ].Address-4 : 0 );
			for(std::vector< STraceEntry >::const_iterator it = mTraceBuffer.begin(); it != mTraceBuffer.end(); ++it)
			{
				u32		address( it->Address );
				OpCode	op_code( it->OpCode );
				u32		branch_index( it->BranchIdx );

				if( branch_index != INVALID_IDX )
				{
					DAEDALUS_ASSERT( branch_index < mBranchDetails.size(), "The branch index is out of range" );

					const SBranchDetails &	details( mBranchDetails[ branch_index ] );

					fprintf( fh, " BRANCH %d -> %08x\n", branch_index, details.TargetAddress );
				}

				char		buf[100];
				SprintOpCodeInfo( buf, address, op_code );

				bool		is_jump( address != last_address + 4 );

				fprintf( fh, "%08x: %c%s\n", address, is_jump ? '*' : ' ', buf );

				last_address = address;
			}

			fclose(fh);
		}
#endif


		//DBGConsole_Msg( 0, "Aborting tracing of     [R%08x]", mStartTraceAddress );
		mTracing = false;
		mStartTraceAddress = 0;
		mTraceBuffer.clear();
		mBranchDetails.clear();
		mExpectedExitTraceAddress = 0;
		mActiveBranchIdx = INVALID_IDX;
		mStopTraceAfterDelaySlot = false;
		mNeedIndirectExitMap = false;
	}

}

//*************************************************************************************
//
//*************************************************************************************
void CTraceRecorder::Analyse( SRegisterUsageInfo & register_usage )
{
	DAEDALUS_PROFILE( "CTraceRecorder::Analyse" );

	std::pair< s32, s32 >		reg_spans[ NUM_N64_REGS ];
	std::pair< s32, s32 >		invalid_span( std::pair< s32, s32 >( mTraceBuffer.size(), -1 ) );

	std::fill( reg_spans, reg_spans + NUM_N64_REGS, invalid_span );		// Set the interval to an invalid range

	for( u32 i = 0; i < mTraceBuffer.size(); ++i )
	{
		const STraceEntry & ti( mTraceBuffer[ i ] );
		const StaticAnalysis::RegisterUsage&	usage = ti.Usage;

		register_usage.RegistersRead |= usage.RegReads;
		register_usage.RegistersWritten |= usage.RegWrites;
		register_usage.RegistersAsBases |= usage.RegBase;

		u32		all_uses( usage.RegReads | usage.RegWrites | usage.RegBase );

		// Update the live span
		for( s32 reg = 1; reg < NUM_N64_REGS; ++reg )
		{
			if( all_uses & (1<<reg) )
			{
				if( s32( i ) < reg_spans[ reg ].first )		reg_spans[ reg ].first = i;
				if( s32( i ) > reg_spans[ reg ].second )	reg_spans[ reg ].second = i;
			}
		}
	}

	register_usage.SpanList.clear();
	register_usage.SpanList.reserve( NUM_N64_REGS );

	// Iterate through registers, inserting all that are used into span list
	for( u32 i = 0; i < NUM_N64_REGS; ++i )
	{
		s32		start( reg_spans[ i ].first );
		s32		end( reg_spans[ i ].second );

		if( start <= end )
		{
			SRegisterSpan	span( EN64Reg( i ), start, end );

			register_usage.SpanList.push_back( span );
		}
	}
}
