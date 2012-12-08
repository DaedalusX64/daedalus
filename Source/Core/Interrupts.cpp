/*
Copyright (C) 2001 StrmnNrmn

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

// Handle interrupts etc
#include "stdafx.h"

#include "Debug/DebugLog.h"

#include "CPU.h"
#include "Interrupt.h"
#include "OSHLE/ultra_rcp.h"
#include "OSHLE/ultra_R4300.h"
#include "R4300.h"

#include "Debug/DBGConsole.h"

inline void SET_EXCEPTION(u32 mask, u32 exception)
{
	gCPUState.CPUControl[C0_CAUSE]._u32 &= ~mask;
	gCPUState.CPUControl[C0_CAUSE]._u32 |= exception;
}

#ifdef DAEDALUS_PROFILE_EXECUTION
u32 gNumExceptions = 0;
u32 gNumInterrupts = 0;
#endif

static u32		gExceptionPC( ~0 );
static bool		gExceptionWasDelay( false );		// Was exception operation in a branch-delay slot?
static u32		gExceptionVector( ~0 );

// CAUSE_IP8 <- This is the compare interrupt
// CAUSE_IP7 <- This if for the RDB (debugger thinger?)
// CAUSE_IP6 <- This is for the RDB (debugger thinger?)
// CAUSE_IP5 <- For PRE NMI
// CAUSE_IP4 <- OS_EVENT_CART - used by RMON
// CAUSE_IP3 <- This is a AI/VI/SI interrupt

//*****************************************************************************
//
//*****************************************************************************
inline void R4300_JumpToInterruptVector(u32 exception_vector)
{

#if defined(DAEDALUS_ENABLE_ASSERTS) || defined(DAEDALUS_PROFILE_EXECUTION)
	bool	mi_interrupt_set( (Memory_MI_GetRegister(MI_INTR_MASK_REG) & Memory_MI_GetRegister(MI_INTR_REG)) != 0 );
	bool	cause_int_3_set( (gCPUState.CPUControl[C0_CAUSE]._u32 & CAUSE_IP3) != 0 );

	DAEDALUS_ASSERT( mi_interrupt_set == cause_int_3_set, "CAUSE_IP3 inconsistant with MI_INTR_REG" );
#endif

	gCPUState.CPUControl[C0_SR]._u32 |= SR_EXL;
	gCPUState.CPUControl[C0_EPC]._u32  = gCPUState.CurrentPC;


	if(gCPUState.Delay == EXEC_DELAY)
	{
		gCPUState.CPUControl[C0_CAUSE]._u32 |= CAUSE_BD;
		gCPUState.CPUControl[C0_EPC]._u32   -= 4;
	}
	else
	{
		gCPUState.CPUControl[C0_CAUSE]._u32 &= ~CAUSE_BD;
	}

	CPU_SetPC( exception_vector );
	gCPUState.Delay = NO_DELAY;
}

//*****************************************************************************
//
//*****************************************************************************
void R4300_Exception_Break()
{
	DAEDALUS_ASSERT( gExceptionVector == u32(~0), "Exception vector already set" );
	DAEDALUS_ASSERT( gExceptionPC == u32(~0), "Exception PC already set" );

	SET_EXCEPTION( CAUSE_EXCMASK, EXC_BREAK );

	gExceptionVector = E_VEC;
	gExceptionPC = gCPUState.CurrentPC;
	gExceptionWasDelay = gCPUState.Delay == EXEC_DELAY;
	gCPUState.AddJob( CPU_CHECK_EXCEPTIONS );
}

//*****************************************************************************
//
//*****************************************************************************
void R4300_Exception_Syscall()
{
	DAEDALUS_ASSERT( gExceptionVector == u32(~0), "Exception vector already set" );
	DAEDALUS_ASSERT( gExceptionPC == u32(~0), "Exception PC already set" );

	SET_EXCEPTION( CAUSE_EXCMASK, EXC_SYSCALL );

	gExceptionVector = E_VEC;
	gExceptionPC = gCPUState.CurrentPC;
	gExceptionWasDelay = gCPUState.Delay == EXEC_DELAY;
	gCPUState.AddJob( CPU_CHECK_EXCEPTIONS );
}

//*****************************************************************************
//
//*****************************************************************************
void R4300_Exception_CopUnusuable()
{
	DAEDALUS_ASSERT( gExceptionVector == u32(~0), "Exception vector already set" );
	DAEDALUS_ASSERT( gExceptionPC == u32(~0), "Exception PC already set" );

	// XXXX check we're not inside exception handler before snuffing CAUSE reg?
	SET_EXCEPTION( (CAUSE_EXCMASK|CAUSE_CEMASK), (EXC_CPU|SR_CU0) );

    //gCPUState.CPUControl[C0_CAUSE]._u32 &= 0xCFFFFFFF;
	//gCPUState.CPUControl[C0_CAUSE]._u32 |= SR_CU0;

	gExceptionVector = E_VEC;
	gExceptionPC = gCPUState.CurrentPC;
	gExceptionWasDelay = gCPUState.Delay == EXEC_DELAY;
	gCPUState.AddJob( CPU_CHECK_EXCEPTIONS );
}

//*****************************************************************************
//
//*****************************************************************************
void R4300_Exception_TLB( u32 virtual_address, u32 exception_code, u32 exception_vector )
{
	DAEDALUS_ASSERT( gExceptionVector == u32(~0), "Exception vector already set" );
	DAEDALUS_ASSERT( gExceptionPC == u32(~0), "Exception PC already set" );

	gCPUState.CPUControl[C0_BADVADDR]._u32 = virtual_address;

	gCPUState.CPUControl[C0_CONTEXT]._u32 &= 0xFF800000;	// Mask off bottom 23 bits
	gCPUState.CPUControl[C0_CONTEXT]._u32 |= ((virtual_address >> 13) << 4);

	gCPUState.CPUControl[C0_ENTRYHI]._u32 &= 0x00001FFF;	// Mask off the top bit 13-31
	gCPUState.CPUControl[C0_ENTRYHI]._u32 |= (virtual_address & 0xFFFFE000);

	SET_EXCEPTION( CAUSE_EXCMASK, exception_code );

	gExceptionVector = exception_vector;
	gExceptionPC = gCPUState.CurrentPC;
	gExceptionWasDelay = gCPUState.Delay == EXEC_DELAY;
	gCPUState.AddJob( CPU_CHECK_EXCEPTIONS );
}

//*****************************************************************************
//
//*****************************************************************************
void R4300_Handle_Exception()
{
	// These should be set before we end up here...
	DAEDALUS_ASSERT( gExceptionVector != u32(~0), "Exception vector not set: %08x", gCPUState.GetStuffToDo() );
	DAEDALUS_ASSERT( gExceptionPC != u32(~0), "Exception PC not set" );

#ifdef DAEDALUS_PROFILE_EXECUTION
	gNumExceptions++;
#endif

	gCPUState.CurrentPC = gExceptionPC;									// Restore this...
	gCPUState.Delay = gExceptionWasDelay ? EXEC_DELAY : NO_DELAY;	// And this...
	R4300_JumpToInterruptVector( gExceptionVector );

	// Reset these, to ensure we set them correctly before next call to this function
	gExceptionVector = ~0;
	gExceptionPC = ~0;
}

//*****************************************************************************
//
//*****************************************************************************
void R4300_Handle_Interrupt()
{
#ifdef DAEDALUS_ENABLE_ASSERTS
	bool	mi_interrupt_set( (Memory_MI_GetRegister(MI_INTR_MASK_REG) & Memory_MI_GetRegister(MI_INTR_REG)) != 0 );
	bool	cause_int_3_set( (gCPUState.CPUControl[C0_CAUSE]._u32 & CAUSE_IP3) != 0 );

	DAEDALUS_ASSERT( mi_interrupt_set == cause_int_3_set, "CAUSE_IP3 inconsistant with MI_INTR_REG (%08x)", Memory_MI_GetRegister(MI_INTR_MASK_REG) & Memory_MI_GetRegister(MI_INTR_REG) );
#endif

	if( (gCPUState.CPUControl[C0_SR]._u32 & (SR_EXL | SR_ERL | SR_IE)) == SR_IE ) // Ensure ERL/EXL are "0" and IE is "1"
	{
		if(gCPUState.CPUControl[C0_SR]._u32 & gCPUState.CPUControl[C0_CAUSE]._u32 & CAUSE_IPMASK)  // Are interrupts pending/wanted?
		{
#ifdef DAEDALUS_PROFILE_EXECUTION
			gNumInterrupts++;
#endif
			SET_EXCEPTION( CAUSE_EXCMASK, EXC_INT );
			R4300_JumpToInterruptVector( E_VEC );
		}
	}
}
