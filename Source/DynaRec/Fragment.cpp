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
#include "Fragment.h"

#include "FragmentCache.h"
#include "BranchType.h"
#include "StaticAnalysis.h"
#include "IndirectExitMap.h"

#include "Core/Registers.h"
#include "Core/CPU.h"			// Try to remove this cyclic dependency
#include "Core/R4300.h"
#include "Core/Interrupt.h"

#include "Debug/DBGConsole.h"

#include "DynaRec/CodeBufferManager.h"
#include "DynaRec/CodeGenerator.h"

#include "Utility/PrintOpCode.h"
#include "Utility/Synchroniser.h"
#include "Utility/Profiler.h"

#include "OSHLE/ultra_R4300.h"
#include "OSHLE/patch.h"

#include <algorithm>

//#define IMMEDIATE_COUNTER_UPDATE
//#define UPDATE_COUNTER_ON_EXCEPTION

//*************************************************************************************
//
//*************************************************************************************
namespace
{

	typedef std::vector<STraceEntry> TraceBuffer;

	const u32	INVALID_IDX( u32(~0) );

	//
	//	We stuff some extra instructions at the start of each fragment, for instance
	//	to record the hitcount. This allows us to offset that from the stats.
	//
#ifdef FRAGMENT_RETAIN_ADDITIONAL_INFO
	const u32	ADDITIONAL_OUTPUT_BYTES = 5 * 4;
#else
	const u32	ADDITIONAL_OUTPUT_BYTES = 0;
#endif

}

//*************************************************************************************
//
//*************************************************************************************
CFragment::CFragment( CCodeBufferManager * p_manager,
					  u32 entry_address,
					  u32 exit_address,
					  const TraceBuffer & trace,
					  SRegisterUsageInfo &	register_usage,
					  const BranchBuffer & branch_details,
					  bool need_indirect_exit_map )
:	mEntryAddress( entry_address )
,	mEntryPoint( NULL )
,	mInputLength( trace.size() * sizeof( OpCode ) )
,	mOutputLength( 0 )
,	mFragmentFunctionLength( 0 )
,	mpIndirectExitMap( need_indirect_exit_map ? new CIndirectExitMap : NULL )
#ifdef FRAGMENT_RETAIN_ADDITIONAL_INFO
,	mHitCount( 0 )
,	mTraceBuffer( trace )
,	mBranchBuffer( branch_details )
,	mExitAddress( exit_address )
#endif
#ifdef FRAGMENT_SIMULATE_EXECUTION
,	mpCache( NULL )
#endif
{
#ifdef FRAGMENT_RETAIN_ADDITIONAL_INFO
	mRegisterUsage = register_usage;
#endif

	Assemble( p_manager, exit_address, trace, branch_details, register_usage );
}

#ifdef DAEDALUS_ENABLE_OS_HOOKS
//*************************************************************************************
// Create a Fragement for Patch Function
//*************************************************************************************
CFragment::CFragment(CCodeBufferManager * p_manager, u32 entry_address, 
						u32 function_length, void* function_Ptr)
	:	mEntryAddress( entry_address )
	,	mInputLength(function_length  * sizeof( OpCode ) )
	,	mOutputLength( 0 )
	,	mFragmentFunctionLength( 0 )
	,	mpIndirectExitMap( new CIndirectExitMap )
#ifdef FRAGMENT_RETAIN_ADDITIONAL_INFO
	,	mHitCount( 0 )
	,	mTraceBuffer( NULL )
	,	mBranchBuffer( NULL )
	,	mExitAddress( 0 )
#endif
#ifdef FRAGMENT_SIMULATE_EXECUTION
	,	mpCache( NULL )
#endif
{
	Assemble(p_manager, CCodeLabel(function_Ptr));
}
#endif
//*************************************************************************************
//
//*************************************************************************************
CFragment::~CFragment()
{
	delete mpIndirectExitMap;
}

//*************************************************************************************
//
//*************************************************************************************
void	CFragment::SetCache( const CFragmentCache * p_cache )
{
#ifdef FRAGMENT_SIMULATE_EXECUTION
	mpCache = p_cache;
#endif

	if( mpIndirectExitMap != NULL )
	{
		mpIndirectExitMap->SetCache( p_cache );
	}
}

//*************************************************************************************
//
//*************************************************************************************
void CFragment::Execute()
{
	DAEDALUS_PROFILE( "CFragment::Execute" );

	SYNCH_POINT( DAED_SYNC_FRAGMENT_PC, gCPUState.CurrentPC + gCPUState.Delay, "Program Counter/Delay doesn't match on entry to fragment" );
	DAEDALUS_ASSERT( gCPUState.Delay == NO_DELAY, "Why are we entering with a delay slot active?" );

#ifdef FRAGMENT_SIMULATE_EXECUTION

	CFragment * p_fragment( this );

	while( p_fragment != NULL )
	{
		CFragment * next = p_fragment->Simulate();

		DAEDALUS_ASSERT( next == NULL || gCPUState.Delay == NO_DELAY, "Why are we entering with a delay slot active?" );

		p_fragment = next;
	}

#else
	const void *		p( g_pu8RamBase - 0x80000000 );
	u32					upper( 0x80000000 + gRamSize );
	_EnterDynaRec( mEntryPoint.GetTarget(), &gCPUState, p, upper );
#endif

	SYNCH_POINT( DAED_SYNC_FRAGMENT_PC, gCPUState.CurrentPC + gCPUState.Delay, "Program Counter/Delay doesn't match on exit from fragment" );
	//if(gCPUState.Delay != NO_DELAY)
	//{
	//	SYNCH_POINT( DAED_SYNC_FRAGMENT_PC, gCPUState.TargetPC, "New Program Counter doesn't match on exit from fragment" );
	//}

	// We have to do this when we exit to make sure the cached read pointer is updated correctly
	CPU_SetPC( gCPUState.CurrentPC );
}

#ifdef FRAGMENT_SIMULATE_EXECUTION
//*************************************************************************************
//
//*************************************************************************************
namespace
{
	void HandleException()
	{
		switch (gCPUState.Delay)
		{
			case DO_DELAY:
				gCPUState.CurrentPC += 4;
				gCPUState.Delay = EXEC_DELAY;
				break;
			case EXEC_DELAY:
				gCPUState.CurrentPC = gCPUState.TargetPC;
				gCPUState.Delay = NO_DELAY;
				break;
			case NO_DELAY:
				gCPUState.CurrentPC += 4;
				break;
			default:	// MSVC extension - the default will never be reached
				NODEFAULT;
		}
	}
#ifdef UPDATE_COUNTER_ON_EXCEPTION
	void UpdateCountAndHandleException_Counter( u32 instructions_executed )
	{
		// If we're updating the counter on every instruction, there's no need to do this...
	#ifndef IMMEDIATE_COUNTER_UPDATE
		CPU_UpdateCounterNoInterrupt( instructions_executed );
	#endif
		HandleException();
	}
#else
	void UpdateCountAndHandleException()
	{
		HandleException();
	}
#endif
	void CheckCop1Usable()
	{
		if( (gCPUState.CPUControl[C0_SR]._u32_0 & SR_CU1) == 0 )
		{
			DAEDALUS_ERROR( "Benign: Cop1 unusable fired - check logic" );
			R4300_Exception_CopUnusuable();
		}

	}
}

//*************************************************************************************
//
//*************************************************************************************
CFragment * CFragment::Simulate()
{
	DAEDALUS_ASSERT( gCPUState.GetStuffToDo() == 0, "Entering when there is stuff to do?" );
	DAEDALUS_ASSERT( gCPUState.CurrentPC == mEntryAddress, "Why are we entering at the wrong address?" );
	DAEDALUS_ASSERT( gCPUState.Delay == NO_DELAY, "Why are we entering with a delay slot active?" );

#ifdef FRAGMENT_RETAIN_ADDITIONAL_INFO
	mHitCount++;
#endif

	//
	//	Keep executing ops until we take a branch
	//
	u32			instructions_executed( 0 );

	u32			branch_idx_taken( INVALID_IDX );		// Index into mBranchBuffer of the taken branch
	u32			branch_taken_address( 0 );
	bool		checked_cop1_usable( false );

	u32			count_entry( gCPUState.CPUControl[C0_COUNT]._u32_0 );

	OpCode		last_executed_op;

	for( u32 i = 0; i < mTraceBuffer.size(); ++i)
	{
		const STraceEntry & ti( mTraceBuffer[ i ] );
		OpCode				op_code( ti.OpCode );
		u32					branch_idx( ti.BranchIdx );

		DAEDALUS_ASSERT( op_code._u32 == *(u32*)ReadAddress( ti.Address ), "Self modifying code detected but not handled" );

		bool				branch_taken;

		if( ti.BranchDelaySlot )
		{
			gCPUState.Delay = EXEC_DELAY;
		}

		DAEDALUS_ASSERT( gCPUState.Delay == (ti.BranchDelaySlot ? EXEC_DELAY : NO_DELAY), "Delay doesn't match expectations" );

		// Check the cop1 usable flag. Do this only once (theoretically it could be toggled mid-fragment but this is unlikely)
		if( op_code.op == OP_COPRO1 && !checked_cop1_usable )
		{
			checked_cop1_usable = true;
			CheckCop1Usable();
			if(gCPUState.GetStuffToDo() != 0)
			{
#ifdef UPDATE_COUNTER_ON_EXCEPTION
				UpdateCountAndHandleException_Counter( instructions_executed );
#else
				UpdateCountAndHandleException();
#endif
				return NULL;
			}
		}

		last_executed_op = op_code;

		CPU_ExecuteOpRaw( count_entry+instructions_executed, ti.Address, op_code, R4300_GetInstructionHandler( op_code ), &branch_taken );

#ifdef IMMEDIATE_COUNTER_UPDATE
		CPU_UpdateCounter( 1 );
#endif
		instructions_executed++;

		if(gCPUState.GetStuffToDo() != 0)
		{
#ifdef UPDATE_COUNTER_ON_EXCEPTION
			UpdateCountAndHandleException_Counter( instructions_executed );
#else
			UpdateCountAndHandleException();
#endif
			return NULL;
		}

		// Break out of the loop if this is a branch instruction and it was taken
		if( branch_idx != INVALID_IDX )
		{
			DAEDALUS_ASSERT( branch_idx < mBranchBuffer.size(), "Branch index is out of bounds" );
			const SBranchDetails &	details( mBranchBuffer[ branch_idx ] );

			// Check whether we want to invert the status of this branch
			bool	exit_trace;

			if( details.Eret )
			{
				exit_trace = true;
			}
			else if( !details.Direct )
			{
				exit_trace = (gCPUState.TargetPC != details.TargetAddress);
			}
			else
			{
				exit_trace = details.ConditionalBranchTaken ? !branch_taken : branch_taken;
			}

			if( exit_trace )
			{
				branch_taken_address = ti.Address;
				branch_idx_taken = branch_idx;
				break;
			}
		}
		else
		{
			DAEDALUS_ASSERT( !branch_taken, "Why are we branching with no branch details?" );
			if(ti.BranchDelaySlot)
			{
				gCPUState.Delay = NO_DELAY;
			}
		}
	}

	DAEDALUS_ASSERT( (GetBranchType(last_executed_op) == BT_NOT_BRANCH) || branch_idx_taken != INVALID_IDX, "The last instruction was a branch, but no branch index on exit" );

	//
	//	Now we're leaving the fragment, handle the exit stubs
	//
	CFragment * p_target_fragment( NULL );
	u32			exit_address;
	u32			exit_delay;
	if( branch_idx_taken != INVALID_IDX )
	{
		//
		//	A branch was taken - this means we have to execute it's delay op
		//
		DAEDALUS_ASSERT( branch_idx_taken < mBranchBuffer.size(), "The branch index is invalid?" );

		SBranchDetails &	details( mBranchBuffer[ branch_idx_taken ] );
		bool				executed_delay_op( true );

		if( details.Likely )
		{
			if( details.ConditionalBranchTaken )
			{
				// The branch was taken in our trace, so we flipped the logic around
				// This means the likely branch WASN'T taken just now.
				// Bail out with an address of PC+8
				// TODO: Could cache this target fragment?
				exit_delay = NO_DELAY;
				exit_address = branch_taken_address + 8;

				p_target_fragment = mpCache->LookupFragmentQ( exit_address );
			}
			else
			{
				// The branch wasn't taken in our trace, so the original logic is used
				// We never saw the branch delay slot, so bail out to interpreter
				exit_delay = EXEC_DELAY;
				gCPUState.TargetPC = details.TargetAddress;
				exit_address = branch_taken_address + 4;		// i.e. execute the branch

				// XXXX This is potentially unsafe - we exit with the flag set. The target
				// fragment may not behave properly if an exception is thrown on the first instruction
				//p_target_fragment = mpCache->LookupFragmentQ( exit_address );
			}
			executed_delay_op = false;
			//return NULL;
		}
		else if( details.Direct )
		{
			exit_address = details.TargetAddress;
			exit_delay = NO_DELAY;
			p_target_fragment = mpCache->LookupFragmentQ( details.TargetAddress );
		}
		else
		{
			if( details.Eret )
			{
				DAEDALUS_ASSERT( instructions_executed == mTraceBuffer.size(), "Why wasn't ERET the last instruction?" );
				DAEDALUS_ASSERT( details.DelaySlotTraceIndex == -1, "Why does this ERET have a return instruction?" );

				exit_address = gCPUState.CurrentPC + 4;
				p_target_fragment = mpIndirectExitMap->LookupIndirectExit( exit_address );
			}
			else
			{
				exit_address = gCPUState.TargetPC;

				p_target_fragment = mpIndirectExitMap->LookupIndirectExit( gCPUState.TargetPC );
			}
			exit_delay = NO_DELAY;
		}

		//
		//	Not all branches have delay instructions
		//

		if( executed_delay_op && details.DelaySlotTraceIndex != -1 )
		{
			OpCode		delay_op_code( mTraceBuffer[details.DelaySlotTraceIndex].OpCode );
			u32 		delay_address( mTraceBuffer[details.DelaySlotTraceIndex].Address );

			bool		dummy_branch_taken;
			gCPUState.Delay = EXEC_DELAY;

			if( delay_op_code.op == OP_COPRO1 && !checked_cop1_usable )
			{
				checked_cop1_usable = true;
				CheckCop1Usable();
				if(gCPUState.GetStuffToDo() != 0)
				{
#ifdef UPDATE_COUNTER_ON_EXCEPTION
					UpdateCountAndHandleException_Counter( instructions_executed );
#else
					UpdateCountAndHandleException();
#endif
					return NULL;
				}
			}

			CPU_ExecuteOpRaw( count_entry+instructions_executed, delay_address, delay_op_code, R4300Instruction[ delay_op_code.op ], &dummy_branch_taken );

#ifdef IMMEDIATE_COUNTER_UPDATE
			CPU_UpdateCounter( 1 );
#endif
			instructions_executed++;

			if(gCPUState.GetStuffToDo() != 0)
			{
#ifdef UPDATE_COUNTER_ON_EXCEPTION
				UpdateCountAndHandleException_Counter( instructions_executed );
#else
				UpdateCountAndHandleException();
#endif
				return NULL;
			}

			gCPUState.Delay = NO_DELAY;
		}
	}
	else
	{
		DAEDALUS_ASSERT( instructions_executed == mTraceBuffer.size(), "Didn't handle the expected number of instructions" );

		p_target_fragment = mpCache->LookupFragmentQ( mExitAddress );
		exit_address = mExitAddress;
		exit_delay = NO_DELAY;
	}

#ifndef IMMEDIATE_COUNTER_UPDATE
	CPU_UpdateCounter( instructions_executed );
#endif

	//
	//	Finally set up all the registers required for transferring control to the next branch
	//

	DAEDALUS_ASSERT( exit_address != u32( ~0 ), "Invalid exit address" );

	gCPUState.CurrentPC = exit_address;
	gCPUState.Delay = exit_delay;

	if( exit_address == mEntryAddress )
	{
		DAEDALUS_ASSERT( gCPUState.Delay == NO_DELAY, "Branching to self with delay slot active?" );
		p_target_fragment = this;
	}

	if( gCPUState.GetStuffToDo() != 0 )
	{
		// Quit to the interpreter if there are CPU jobs to do
		p_target_fragment = NULL;
	}

	return p_target_fragment;
}
#endif	// FRAGMENT_SIMULATE_EXECUTION

//*************************************************************************************
//
//*************************************************************************************
u32	CFragment::GetMemoryUsage() const
{
	// Ignore the 'additional info' when computing this

	return sizeof( CFragment ) +
		   mPatchList.size() * sizeof( SFragmentPatchDetails );
}

//*************************************************************************************
//
//*************************************************************************************
namespace
{
	struct SBranchHandlerInfo
	{
		SBranchHandlerInfo()
			:	Index( u32( ~0 ) )
			,	Jump()
			,	RegisterSnapshot( u32( ~0 ) )
		{
		}

		u32						Index;
		CJumpLocation			Jump;
		RegisterSnapshotHandle	RegisterSnapshot;
	};
}

//*************************************************************************************
//
//*************************************************************************************
void	CFragment::AddPatch( u32 address, CJumpLocation jump_location )
{
	if( jump_location.IsSet() )
	{
		SFragmentPatchDetails	patch_details;

		patch_details.Address = address;
		patch_details.Jump = jump_location;

		mPatchList.push_back( patch_details );
	}
}


//*************************************************************************************
//
//*************************************************************************************
void CFragment::Assemble( CCodeBufferManager * p_manager,
						  u32 exit_address,
						  const std::vector< STraceEntry > & trace,
						  const std::vector< SBranchDetails > & branch_details,
						  const SRegisterUsageInfo & register_usage )
{
	DAEDALUS_PROFILE( "CFragment::Assemble" );

	const u32				NO_JUMP_ADDRESS( 0 );

	CCodeGenerator *		p_generator( p_manager->StartNewBlock() );

	mEntryPoint = p_generator->GetEntryPoint();

#ifdef FRAGMENT_RETAIN_ADDITIONAL_INFO
	p_generator->Initialise( mEntryAddress, exit_address, &mHitCount, &gCPUState, register_usage );
#else
	p_generator->Initialise( mEntryAddress, exit_address, NULL, &gCPUState, register_usage );
#endif

	//Trace: (3 ops, 13 hits)
	//80317934:  SLT       at = (t7<a0)
	//BRANCH 0 -> 80317940
	//80317938:  BNEL      at != r0 --> 0x80317934
	//8031793c:  LW        t7 <- 0x0000(v0)

	if(trace.size() == 3)
	{
		if( trace[0].OpCode._u32 == 0x01E4082A &&
			trace[1].OpCode._u32 == 0x5420FFFE &&
			trace[2].OpCode._u32 == 0x8C4F0000)
		{
#ifndef DAEDALUS_SILENT
			printf("Speedhack complex %08x\n", trace[0].Address );
#endif
			p_generator->ExecuteNativeFunction( CCodeLabel( reinterpret_cast< const void * >( CPU_SkipToNextEvent ) ) );
		}
	}

	//
	//	Keep executing ops until we take a branch
	//
//	std::vector< CJumpLocation >		exception_handler_jumps;
	std::vector< SBranchHandlerInfo >	branch_handler_info( branch_details.size() );
//	bool								checked_cop1_usable( false );

	for( u32 i = 0; i < trace.size(); ++i )
	{
		const STraceEntry & ti( trace[ i ] );
		u32	branch_idx( ti.BranchIdx );

#ifdef FRAGMENT_RETAIN_ADDITIONAL_INFO
		mInstructionStartLocations.push_back( p_generator->GetCurrentLocation().GetTargetU8P() );
#endif

		p_generator->UpdateRegisterCaching( i );

		// Check the cop1 usable flag. Do this only once (theoretically it could be toggled mid-fragment but this is unlikely)
		/*
		if( op_code.op == OP_COPRO1 && !checked_cop1_usable )
		{
			checked_cop1_usable = true;

			// TODO
			//CJumpLocation	handler( p_generator->GenerateCheckCop1Usable( CheckCop1Usable(), ti.BranchDelaySlot ) );
			//exception_handler_jumps.push_back( handler );
		}
		*/

		const SBranchDetails * p_branch( NULL );
		if( branch_idx != INVALID_IDX )
		{
			DAEDALUS_ASSERT( branch_idx < branch_details.size(), "Branch index is out of bounds" );
			p_branch = &branch_details[ branch_idx ];

#ifndef DAEDALUS_SILENT
			switch(p_branch->SpeedHack)
			{
				case SHACK_SKIPTOEVENT:
					{
					printf("Speedhack event (skip busy loop)\n");
					char opinfo[128];
					SprintOpCodeInfo( opinfo, trace[i].Address, trace[i].OpCode );
					printf("\t%p: <0x%08x> %s\n", (u32*)trace[i].Address, trace[i].OpCode._u32, opinfo);
					
					SprintOpCodeInfo( opinfo, trace[i+1].Address, trace[i+1].OpCode );
					printf("\t%p: <0x%08x> %s\n", (u32*)trace[i+1].Address, trace[i+1].OpCode._u32, opinfo);

					p_generator->ExecuteNativeFunction( CCodeLabel( reinterpret_cast< const void * >( CPU_SkipToNextEvent ) ) );
					}
					break;

				case SHACK_COPYREG:
					{
					printf("Speedhack copyreg (not handled)\n");
					char opinfo[128];
					SprintOpCodeInfo( opinfo, trace[i].Address, trace[i].OpCode );
					printf("\t%p: <0x%08x> %s\n", (u32*)trace[i].Address, trace[i].OpCode._u32, opinfo);
					
					SprintOpCodeInfo( opinfo, trace[i+1].Address, trace[i+1].OpCode );
					printf("\t%p: <0x%08x> %s\n", (u32*)trace[i+1].Address, trace[i+1].OpCode._u32, opinfo);
					}
					break;

				case SHACK_POSSIBLE:
					{
					printf("Speedhack unknown (not handled)\n");
					char opinfo[128];
					SprintOpCodeInfo( opinfo, trace[i].Address, trace[i].OpCode );
					printf("\t%p: <0x%08x> %s\n", (u32*)trace[i].Address, trace[i].OpCode._u32, opinfo);
					
					SprintOpCodeInfo( opinfo, trace[i+1].Address, trace[i+1].OpCode );
					printf("\t%p: <0x%08x> %s\n", (u32*)trace[i+1].Address, trace[i+1].OpCode._u32, opinfo);
					}
					break;

				default:
					break;
			}
#else
			if(p_branch->SpeedHack == SHACK_SKIPTOEVENT)
			{
				p_generator->ExecuteNativeFunction( CCodeLabel( reinterpret_cast< const void * >( CPU_SkipToNextEvent ) ) );
			}
#endif
		}

		CJumpLocation	branch_jump( NULL );
		p_generator->GenerateOpCode( ti, ti.BranchDelaySlot, p_branch, &branch_jump);
		/*
		CJumpLocation	exception_handler_jump( p_generator->GenerateOpCode( ti, ti.BranchDelaySlot, p_branch, &branch_jump) );

		if( exception_handler_jump.IsSet() )
		{
			exception_handler_jumps.push_back( exception_handler_jump );
		}
		*/

		// Check whether we want to invert the status of this branch
		if( p_branch != NULL )
		{
			branch_handler_info[ branch_idx ].Index = i;
			branch_handler_info[ branch_idx ].Jump = branch_jump;
			branch_handler_info[ branch_idx ].RegisterSnapshot = p_generator->GetRegisterSnapshot();
		}
	}
#ifdef FRAGMENT_RETAIN_ADDITIONAL_INFO
		mInstructionStartLocations.push_back( p_generator->GetCurrentLocation().GetTargetU8P() );
#endif

	CCodeLabel		no_next_fragment( NULL );
	CJumpLocation	exit_jump( p_generator->GenerateExitCode( exit_address, NO_JUMP_ADDRESS, trace.size(), no_next_fragment ) );

	AddPatch( exit_address, exit_jump );

	//
	//	Generate handlers for each exit branch
	//
	for( u32 i = 0; i < branch_details.size(); ++i )
	{
		const SBranchDetails &	details( branch_details[ i ] );
		u32						instruction_idx( branch_handler_info[ i ].Index );

		// If we didn't generate a branch instruction, then it didn't need handling
		if(!branch_handler_info[ i ].Jump.IsSet())
		{
			continue;
		}

		DAEDALUS_ASSERT( instruction_idx < trace.size(), "The instruction index is invalid" );

		u32					branch_instruction_address( trace[ instruction_idx ].Address );
		u32					num_instructions_executed( instruction_idx + 1 );

		p_generator->GenerateBranchHandler( branch_handler_info[ i ].Jump, branch_handler_info[ i ].RegisterSnapshot );

		//
		//	Not all branches have delay instructions
		//
		if( !details.Likely && details.DelaySlotTraceIndex != -1 )
		{
			const STraceEntry & ti( trace[ details.DelaySlotTraceIndex ] );
#ifdef DAEDALUS_DEBUG_CONSOLE			
			OpCode		delay_op_code( ti.OpCode );
#endif
#ifdef FRAGMENT_SIMULATE_EXECUTION
			u32			delay_address( ti.Address );
#endif
			/*
			if( delay_op_code.op == OP_COPRO1 && !checked_cop1_usable )
			{
				checked_cop1_usable = true;

				//CJumpLocation	handler( p_generator->GenerateCheckCop1Usable( CheckCop1Usable(), true ) );
				//exception_handler_jumps.push_back( handler );
			}
			*/

			p_generator->GenerateOpCode( ti, true, NULL, NULL);
			/*
			CJumpLocation	exception_handler_jump( p_generator->GenerateOpCode( ti, true, NULL, NULL) );

			if( exception_handler_jump.IsSet() )
			{
				exception_handler_jumps.push_back( exception_handler_jump );
			}
			*/
			num_instructions_executed++;
		}


		if( details.Likely )
		{
			u32				exit_address;
			CJumpLocation	jump_location;

			if( details.ConditionalBranchTaken )
			{
				exit_address = branch_instruction_address + 8;
				jump_location = p_generator->GenerateExitCode( exit_address, NO_JUMP_ADDRESS, num_instructions_executed, no_next_fragment );
			}
			else
			{
				exit_address = branch_instruction_address + 4;
				jump_location = p_generator->GenerateExitCode( exit_address, details.TargetAddress, num_instructions_executed, no_next_fragment );
			}

			AddPatch( exit_address, jump_location );
		}
		else if( details.Direct )
		{
			u32				exit_address( details.TargetAddress );
			CJumpLocation	jump_location( p_generator->GenerateExitCode( details.TargetAddress, NO_JUMP_ADDRESS, num_instructions_executed, no_next_fragment ) );

			AddPatch( exit_address, jump_location );
		}
		else
		{
			DAEDALUS_ASSERT( mpIndirectExitMap != NULL, "There is no indirect exit map!" );

			if( details.Eret )
			{
				DAEDALUS_ASSERT( details.DelaySlotTraceIndex == -1, "Why does this ERET have a return instruction?" );
				p_generator->GenerateEretExitCode( num_instructions_executed, mpIndirectExitMap );
			}
			else
			{
				p_generator->GenerateIndirectExitCode( num_instructions_executed, mpIndirectExitMap );
			}
		}
	}
	// We handle exceptions directly with _ReturnFromDynaRecIfStuffToDo - we should never get here on the psp
//	DAEDALUS_ASSERT( exception_handler_jumps.empty(), "Not expecting to have any exception handler jumps to process" );

	p_generator->Finalise();

	mFragmentFunctionLength = p_manager->FinaliseCurrentBlock();
	mOutputLength = mFragmentFunctionLength - ADDITIONAL_OUTPUT_BYTES;

	delete p_generator;
}

#ifdef DAEDALUS_ENABLE_OS_HOOKS
//*************************************************************************************
//
//*************************************************************************************
void CFragment::Assemble( CCodeBufferManager * p_manager, CCodeLabel function_ptr)
{
//	std::vector< CJumpLocation >		exception_handler_jumps;
	SRegisterUsageInfo register_usage;

	CCodeGenerator *p_generator = p_manager->StartNewBlock();
	mEntryPoint = p_generator->GetEntryPoint();

	
#ifdef FRAGMENT_RETAIN_ADDITIONAL_INFO
		p_generator->Initialise( mEntryAddress, 0, &mHitCount, &gCPUState,  register_usage);
#else
		p_generator->Initialise( mEntryAddress, 0, NULL, &gCPUState, register_usage );
#endif

	CJumpLocation jump = p_generator->ExecuteNativeFunction(function_ptr, true);
	p_generator->GenerateIndirectExitCode(100, mpIndirectExitMap);
	AssemblyUtils::PatchJumpLong(jump, p_generator->GetCurrentLocation());
	p_generator->GenerateEretExitCode(100, mpIndirectExitMap);

	// We handle exceptions directly with _ReturnFromDynaRecIfStuffToDo - we should never get here on the psp
//	DAEDALUS_ASSERT( exception_handler_jumps.empty(), "Not expecting to have any exception handler jumps to process" );

	p_generator->Finalise();
	mFragmentFunctionLength = p_manager->FinaliseCurrentBlock();
	mOutputLength = mFragmentFunctionLength - ADDITIONAL_OUTPUT_BYTES;
	
	delete p_generator;
}
#endif
//*************************************************************************************
//
//*************************************************************************************
#ifdef DAEDALUS_DEBUG_DYNAREC

#include <string>

const char gIllegalChars[] = "&<>";

const char * Sanitise( const char * str )
{
	//
	//	Quickly check to see if there are any illegal characters in the string
	//
	const char * b( str );
	const char * e( str + strlen(str) );		//  Point to null char
	const char * s = std::find_first_of( b, e, gIllegalChars, gIllegalChars+ARRAYSIZE(gIllegalChars));
	if( s == e )
	{
		return str;
	}

	static std::string		out;
	out.resize( 0 );
	out.reserve( strlen( str ) );

	//
	//	Pretty lame. Could avoid doing a char-by-char copy
	//
	while( *str )
	{
		switch( *str )
		{
		case '&':	out += "&amp;";	break;
		case '<':	out += "&lt;";	break;
		case '>':	out += "&gt;";	break;
		default:	out += *str;	break;
		}

		str++;
	}

	return out.c_str();
}

void DisassembleBuffer( const u8 * buf, int buf_size, FILE * fh )
{
	const int	STRBUF_LEN = 1024;
	char		strbuf[STRBUF_LEN+1];

	const OpCode *	p_op( reinterpret_cast< const OpCode * >( buf ) );
	const OpCode *	p_op_end( reinterpret_cast< const OpCode * >( buf + buf_size ) );

	while( p_op < p_op_end )
	{
		u32		address( reinterpret_cast< u32 >( p_op ) );
		OpCode	op_code( *p_op );

		SprintOpCodeInfo( strbuf, address, op_code );
		fprintf( fh, "%08x: %08x %s\n", address, op_code._u32, Sanitise( strbuf ) );
		p_op++;
	}
}

#endif // defined( DAEDALUS_DEBUG_DYNAREC )

#ifdef DAEDALUS_DEBUG_DYNAREC
//*************************************************************************************
//
//*************************************************************************************
void CFragment::DumpFragmentInfoHtml( FILE * fh, u64 total_cycles ) const
{
	float	pc_total_cycles( 0.0f );

	if( total_cycles > 0 )
	{
		pc_total_cycles = 100.0f * float( GetCyclesExecuted() ) / float( total_cycles );
	}

	fputs( "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">", fh );
	fputs( "<html xmlns=\"http://www.w3.org/1999/xhtml\">\n", fh );
	fprintf( fh, "<head><title>Fragment %08x</title>\n", GetEntryAddress() );
	fputs( "<link rel=\"stylesheet\" href=\"default.css\" type=\"text/css\" media=\"all\" />\n", fh );
	fputs( "</head><body>\n", fh );
	fprintf( fh, "<h1>Fragment %8x</h1>\n", GetEntryAddress() );

	fputs( "<div align=\"center\"><table>\n", fh );
		fprintf( fh, "<tr><td>Cycles</td><td>%d</td></tr>\n", GetCyclesExecuted() );
		fprintf( fh, "<tr><td>Cycles %%</td><td>%.2f%%</td></tr>\n", pc_total_cycles );
		fprintf( fh, "<tr><td>Hit count</td><td>%d</td></tr>\n", mHitCount );

		fprintf( fh, "<tr><td>Input Bytes</td><td>%d</td></tr>\n", GetInputLength() );
		fprintf( fh, "<tr><td>Output Bytes</td><td>%d</td></tr>\n", GetOutputLength() );
		fprintf( fh, "<tr><td>Expansion ratio</td><td>%.2fx</td></tr>\n", f32( GetOutputLength() ) / f32( GetInputLength() ) );
#ifdef DAEDALUS_ENABLE_OS_HOOKS
		if (mTraceBuffer.empty())
			fprintf( fh,"<tr><td>function name</td><td>%s</td></tr>\n", Patch_GetJumpAddressName(mEntryAddress));
#endif
		fputs( "</table></div>\n", fh );

	fputs( "<h2>Register Usage</h2>\n", fh );
	fputs( "<div align=\"center\"><table>\n", fh );

	fputs( "<tr><td>Read</td><td>", fh );
	for(u32 i = 1; i < NUM_N64_REGS; ++i)
	{
		if(mRegisterUsage.RegistersRead&(1<<i)) { fprintf( fh, "%s ", RegNames[i] ); }
	}
	fputs( "</td></tr>\n", fh );

	fputs( "<tr><td>Written</td><td>", fh );
	for(u32 i = 1; i < NUM_N64_REGS; ++i)
	{
		if(mRegisterUsage.RegistersWritten&(1<<i)) { fprintf( fh, "%s ", RegNames[i] ); }
	}
	fputs( "</td></tr>\n", fh );

	fputs( "<tr><td>Bases</td><td>", fh );
	for(u32 i = 1; i < NUM_N64_REGS; ++i)
	{
		if(mRegisterUsage.RegistersAsBases&(1<<i)) { fprintf( fh, "%s ", RegNames[i] ); }
	}
	fputs( "</td></tr>\n", fh );
	fputs( "</table></div>\n", fh );

	fputs( "<h2>Spans</h2>\n", fh );
	fputs( "<div align=\"center\"><pre>\n", fh );
	for(RegisterSpanList::const_iterator span_it = mRegisterUsage.SpanList.begin(); span_it < mRegisterUsage.SpanList.end(); ++span_it )
	{
		const SRegisterSpan &	span( *span_it );

		fprintf( fh, "%s: %3d -> %3d   [", RegNames[ span.Register ], span.SpanStart, span.SpanEnd );

		// Display the span as a line
		u32 count( 0 );
		while( count < span.SpanStart )
		{
			fprintf( fh, " " );
			count++;
		}
		while( count <= span.SpanEnd )		// <= (span end is inclusive)
		{
			fprintf( fh, "-" );
			count++;
		}
		while( count < mTraceBuffer.size() )
		{
			fprintf( fh, " " );
			count++;
		}

		fprintf( fh, "]\n" );
	}
	fprintf( fh, "\n" );
	fputs( "</pre></div>\n", fh );


	//
	//	Input trace
	//
	{
		fputs( "<h2>Input Disassembly</h2>\n", fh );
		fputs( "<div align=\"center\"><table><tr><th>Address</th><th>Instruction</th><th>Exit Target</th></tr>\n", fh );
		u32			last_address( mTraceBuffer.size() > 0 ? mTraceBuffer[ 0 ].Address-4 : 0 );
		for( u32 i = 0; i < mTraceBuffer.size(); ++i )
		{
			const STraceEntry &	entry( mTraceBuffer[ i ] );
			u32					address( entry.Address );
			OpCode				op_code( entry.OpCode );
			u32					branch_index( entry.BranchIdx );

			char				buf[100];
			SprintOpCodeInfo( buf, address, op_code );

			bool				is_jump( address != last_address + 4 );

			fprintf( fh, "<tr><td><pre>%08x</pre></td><td><pre>%c%s</pre></td><td><pre>", address, is_jump ? '*' : ' ', Sanitise( buf ) );

			if( branch_index != INVALID_IDX )
			{
				DAEDALUS_ASSERT( branch_index < mBranchBuffer.size(), "The branch index is out of range" );

				const SBranchDetails &	details( mBranchBuffer[ branch_index ] );

				fprintf( fh, "<a href=\"%08x.html\">%08x</a>", details.TargetAddress, details.TargetAddress );
			}
			fprintf( fh, "</pre></td></tr>\n");
			last_address = address;
		}
		fputs( "</table></div>\n", fh );
	}


	if( mEntryPoint.IsSet() )
	{
		fputs( "<h2>Disassembly</h2>\n", fh );
		fputs( "<div align=\"center\"><table><tr><th colspan=2>Input</th><th>Output</th></tr>\n", fh );

		const u8 *	output_begin( mEntryPoint.GetTargetU8P() );
		const u8 *	buffer_end( output_begin + mFragmentFunctionLength );

		//
		//	Prologue
		//
		if( mInstructionStartLocations.size() > 0 )
		{
			const u8 * output_end( mInstructionStartLocations[ 0 ] );

			fputs( "<tr valign=top><td colspan=2><pre>(Prologue)</pre></td><td><pre>", fh );
			DisassembleBuffer( output_begin, output_end-output_begin, fh );
			fputs( "</pre></td></tr>\n", fh );
			output_begin = output_end;
		}

		//
		//	Trace
		//
		for( u32 i = 0; i < mTraceBuffer.size(); ++i )
		{
			const STraceEntry &	entry( mTraceBuffer[ i ] );
			const u8 *			output_end( i+1 < mInstructionStartLocations.size() ? mInstructionStartLocations[ i+1 ] : buffer_end );

			u32					address( entry.Address );
			OpCode				op_code( entry.OpCode );

			char				buf[100];
			SprintOpCodeInfo( buf, address, op_code );

			fprintf( fh, "<tr valign=top><td><pre>%08x</pre></td><td><pre>%s</pre></td><td><pre>", address, Sanitise( buf ) );
			DisassembleBuffer( output_begin, output_end-output_begin, fh );
			fputs( "</pre></td></tr>\n", fh );
			output_begin = output_end;
		}

		//
		//	Epilogue
		//
		const u8 * output_end( buffer_end );
		fputs( "<tr valign=top><td colspan=2><pre>(Epilogue)</pre></td><td><pre>", fh );
		DisassembleBuffer( output_begin, output_end-output_begin, fh );
		fputs( "</pre></td></tr>\n", fh );
		output_begin = output_end;

		fputs( "</table></div>\n", fh );
	}

	fputs( "</body></html>\n", fh );
}
#endif // DAEDALUS_DEBUG_DYNAREC

