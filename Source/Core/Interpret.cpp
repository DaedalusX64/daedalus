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

// Stuff to handle Processor
#include "stdafx.h"

#include "CPU.h"
#include "Registers.h"					// For REG_?? defines
#include "Memory.h"
#include "Interrupt.h"
#include "ROMBuffer.h"
#include "R4300.h"
#include "Interpret.h"

#include "Config/ConfigOptions.h"
#include "Debug/DBGConsole.h"
#include "Debug/DebugLog.h"
#include "OSHLE/patch.h"				// GetCorrectOp
#include "OSHLE/ultra_R4300.h"
#include "Utility/Macros.h"
#include "Utility/Profiler.h"
#include "Utility/Synchroniser.h"

//*****************************************************************************
//	Execute a single MIPS op. The conditionals for the templated arguments
//	are completely optimised away by the compiler.
//
//	TranslateOp:	Use this to translate breakpoints/patches to original op
//					before execution.
//*****************************************************************************
template< bool TranslateOp > DAEDALUS_FORCEINLINE void CPU_EXECUTE_OP()
{
	u8 * p_Instruction {};

	CPU_FETCH_INSTRUCTION( p_Instruction, gCPUState.CurrentPC );
	OpCode op_code = *(OpCode*)p_Instruction;

	// Cache instruction base pointer (used for SpeedHack() @ R4300.0)
	gLastAddress = p_Instruction;

#ifdef DAEDALUS_BREAKPOINTS_ENABLED
	if ( TranslateOp )
	{
		// Handle breakpoints correctly
		if (op_code.op == OP_DBG_BKPT)
		{
			// Turn temporary disable on to allow instr to be processed
			// Entry is in lower 26 bits...
			u32	breakpoint( op_code.bp_index );

			if ( breakpoint < g_BreakPoints.size() )
			{
				if (g_BreakPoints[ breakpoint ].mEnabled)
				{
					g_BreakPoints[ breakpoint ].mTemporaryDisable = true;
				}
			}
		}
		else
		{
			op_code = GetCorrectOp( op_code );
		}
	}
#endif

	SYNCH_POINT( DAED_SYNC_REG_PC, gCPUState.CurrentPC, "Program Counter doesn't match" );
	SYNCH_POINT( DAED_SYNC_FRAGMENT_PC, gCPUState.CurrentPC + gCPUState.Delay, "Program Counter/Delay doesn't match while interpreting" );

	SYNCH_POINT( DAED_SYNC_REG_PC, gCPUState.CPUControl[C0_COUNT]._u32, "Count doesn't match" );

	R4300_ExecuteInstruction(op_code);
	gGPR[0]._u64 = 0;	//Ensure r0 is zero

#ifdef DAEDALUS_PROFILE_EXECUTION
		gTotalInstructionsEmulated++;
#endif

	SYNCH_POINT( DAED_SYNC_REGS, CPU_ProduceRegisterHash(), "Registers don't match" );

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
			//bool	backwards( gCPUState.TargetPC <= gCPUState.CurrentPC );

			// We've just executed the delayed instr. Now carry out jump as stored in gCPUState.TargetPC;
			CPU_SetPC(gCPUState.TargetPC);
			gCPUState.Delay = NO_DELAY;

		}
		break;
	case NO_DELAY:
		// Normal operation - just increment the PC
		INCREMENT_PC();
		break;
	default:
		NODEFAULT;
	}
}


//*****************************************************************************
// Keep executing instructions until there are other tasks to do (i.e. gCPUState.GetStuffToDo() is set)
// Process these tasks and loop
//*****************************************************************************
void CPU_Go()
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
			CPU_EXECUTE_OP< false >();

			stuff_to_do = gCPUState.GetStuffToDo();
		}

		if (CPU_CheckStuffToDo())
			break;
	}
}


void Inter_SelectCore()
{
   g_pCPUCore = CPU_Go;
}

//*****************************************************************************
// Hacky function to use when debugging
//*****************************************************************************
void CPU_Skip()
{
	if (CPU_IsRunning())
	{
		DBGConsole_Msg(0, "Already Running");
		return;
	}

	INCREMENT_PC();
}

//*****************************************************************************
//
//*****************************************************************************
void CPU_Step()
{
	if (CPU_IsRunning())
	{
		DBGConsole_Msg(0, "Already Running");
		return;
	}

	CPU_CheckStuffToDo();

	CPU_EXECUTE_OP< true >();
}
