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


static ETLBExceptionReason g_nTLBExceptionReason;
#ifdef DAEDALUS_PROFILE_EXECUTION
u32 gNumExceptions = 0;
u32 gNumInterrupts = 0;
#endif
u32		gExceptionPC( ~0 );
bool	gExceptionWasDelay( false );		// Was exception operation in a branch-delay slot?
u32		gExceptionVector( ~0 );

// CAUSE_IP8 <- This is the compare interrupt
// CAUSE_IP7 <- This if for the RDB (debugger thinger?)
// CAUSE_IP6 <- This is for the RDB (debugger thinger?)
// CAUSE_IP5 <- For PRE NMI
// CAUSE_IP4 <- OS_EVENT_CART - used by RMON
// CAUSE_IP3 <- This is a AI/VI/SI interrupt

//*****************************************************************************
//
//*****************************************************************************
void R4300_Interrupt_CheckPostponed()
{
	u32 intr_bits = Memory_MI_GetRegister(MI_INTR_REG);
	u32 intr_mask = Memory_MI_GetRegister(MI_INTR_MASK_REG);

	if ((intr_bits & intr_mask) != 0)
	{
		gCPUState.CPUControl[C0_CAUSE]._u64 |= CAUSE_IP3;
	}

	if(gCPUState.CPUControl[C0_SR]._u64 & gCPUState.CPUControl[C0_CAUSE]._u64 & CAUSE_IPMASK)		// Are interrupts pending/wanted
	{
		if(gCPUState.CPUControl[C0_SR]._u64 & SR_IE)								// Are interrupts enabled
		{
			if((gCPUState.CPUControl[C0_SR]._u64 & (SR_EXL|SR_ERL)) == 0x0000)		// Ensure ERL/EXL are not set
			{
				R4300_JumpToInterruptVector(E_VEC);								// Go
				return;
			}
		}
	}
}

//*****************************************************************************
//
//*****************************************************************************
//void R4300_Interrupt_UpdateCause3()
//{
//	//
//	// If any interrupts pending when they are unmasked, the interrupt fires
//	//
//	if ((Memory_MI_GetRegister(MI_INTR_MASK_REG) &
//		 Memory_MI_GetRegister(MI_INTR_REG)) == 0)
//	{
//		// Clear the Cause register
//		gCPUState.CPUControl[C0_CAUSE]._u32_0 &= ~CAUSE_IP3;
//	}
//	else
//	{
//		gCPUState.CPUControl[C0_CAUSE]._u32_0 |= CAUSE_IP3;
//		gCPUState.AddJob( CPU_CHECK_INTERRUPTS );
//	}
//}

//*****************************************************************************
//
//*****************************************************************************
void R4300_Exception_Break()
{
	DAEDALUS_ASSERT( gExceptionVector == u32(~0), "Exception vector already set" );
	DAEDALUS_ASSERT( gExceptionPC == u32(~0), "Exception PC already set" );

	// Clear CAUSE_EXCMASK
	gCPUState.CPUControl[C0_CAUSE]._u64 &= ~CAUSE_EXCMASK;
	gCPUState.CPUControl[C0_CAUSE]._u64 |= EXC_BREAK;

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

	// Clear CAUSE_EXCMASK
	gCPUState.CPUControl[C0_CAUSE]._u64 &= ~CAUSE_EXCMASK;
	gCPUState.CPUControl[C0_CAUSE]._u64 |= EXC_SYSCALL;
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

	// Clear CAUSE_EXCMASK
	// XXXX check we're not inside exception handler before snuffing CAUSE reg?
	gCPUState.CPUControl[C0_CAUSE]._u64 &= ~(CAUSE_EXCMASK|CAUSE_CEMASK);
	gCPUState.CPUControl[C0_CAUSE]._u64 |= (EXC_CPU | 0x10000000);
	gExceptionVector = E_VEC;
	gExceptionPC = gCPUState.CurrentPC;
	gExceptionWasDelay = gCPUState.Delay == EXEC_DELAY;
	gCPUState.AddJob( CPU_CHECK_EXCEPTIONS );
}

//*****************************************************************************
//
//*****************************************************************************
void R4300_Exception_FP()
{
	DAEDALUS_ASSERT( gExceptionVector == u32(~0), "Exception vector already set" );
	DAEDALUS_ASSERT( gExceptionPC == u32(~0), "Exception PC already set" );

	// Clear CAUSE_EXCMASK
	gCPUState.CPUControl[C0_CAUSE]._u64 &= ~CAUSE_EXCMASK;
	gCPUState.CPUControl[C0_CAUSE]._u64 |= EXC_FPE;
	gExceptionVector = E_VEC;
	gExceptionPC = gCPUState.CurrentPC;
	gExceptionWasDelay = gCPUState.Delay == EXEC_DELAY;
	gCPUState.AddJob( CPU_CHECK_EXCEPTIONS );
}

//*****************************************************************************
//
//*****************************************************************************
void R4300_Exception_TLB( u32 virtual_address, ETLBExceptionReason reason )
{
	DAEDALUS_ASSERT( gExceptionVector == u32(~0), "Exception vector already set" );
	DAEDALUS_ASSERT( gExceptionPC == u32(~0), "Exception PC already set" );

	u32 dwVPN2;

	gCPUState.CPUControl[C0_BADVADDR]._u64 = (s64)(s32)virtual_address;			//!! XXXX Added sign extension - Check this!

	dwVPN2 = (virtual_address>>TLBHI_VPN2SHIFT);	// Shift off odd/even indicator

	gCPUState.CPUControl[C0_CONTEXT]._u64 &= ~0x007FFFFF;	// Mask off bottom 23 bits
	gCPUState.CPUControl[C0_CONTEXT]._u64 |= (dwVPN2<<4);

	gCPUState.CPUControl[C0_ENTRYHI]._u64 = (s64)(s32)(dwVPN2 << TLBHI_VPN2SHIFT);

	g_nTLBExceptionReason = reason;

	u32		exception_code( 0 );
	u32		exception_vector( E_VEC );

	// Jump to common exception vector, use TLBL or TLBS in ExcCode field
	switch ( reason )
	{
	case EXCEPTION_TLB_REFILL_LOAD:		exception_code = EXC_RMISS; exception_vector = UT_VEC; break;
	case EXCEPTION_TLB_REFILL_STORE:	exception_code = EXC_WMISS;	exception_vector = UT_VEC; break;
	case EXCEPTION_TLB_INVALID_LOAD:	exception_code = EXC_RMISS;	exception_vector = E_VEC; break;
	case EXCEPTION_TLB_INVALID_STORE:	exception_code = EXC_WMISS;	exception_vector = E_VEC; break;

	default:
		DAEDALUS_ERROR( "Unknown TLB error" );
	}

	gCPUState.CPUControl[C0_CAUSE]._u64 &= ~CAUSE_EXCMASK;
	gCPUState.CPUControl[C0_CAUSE]._u64 |= exception_code;

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
	bool	cause_int_3_set( (gCPUState.CPUControl[C0_CAUSE]._u32_0 & CAUSE_IP3) != 0 );

	DAEDALUS_ASSERT( mi_interrupt_set == cause_int_3_set, "CAUSE_IP3 inconsistant with MI_INTR_REG (%08x)", Memory_MI_GetRegister(MI_INTR_MASK_REG) & Memory_MI_GetRegister(MI_INTR_REG) );
#endif

	if(gCPUState.CPUControl[C0_SR]._u64 & gCPUState.CPUControl[C0_CAUSE]._u64 & CAUSE_IPMASK)	// Are interrupts pending/wanted?
	{
		if(gCPUState.CPUControl[C0_SR]._u64 & SR_IE)									// Are interrupts enabled?
		{
			if((gCPUState.CPUControl[C0_SR]._u64 & (SR_EXL|SR_ERL)) == 0x0000)		// Ensure ERL/EXL are not set
			{
			#ifdef DAEDALUS_PROFILE_EXECUTION
				gNumInterrupts++;
			#endif

				// Clear CAUSE_EXCMASK
				gCPUState.CPUControl[C0_CAUSE]._u64 &= ~CAUSE_EXCMASK;
				gCPUState.CPUControl[C0_CAUSE]._u64 |= EXC_INT;			// This is actually 0

				R4300_JumpToInterruptVector(E_VEC);
			}
		}
	}
}

//*****************************************************************************
//
//*****************************************************************************
void R4300_JumpToInterruptVector(u32 exception_vector)
{
#ifdef DAEDALUS_PROFILE_EXECUTION
#ifdef DAEDALUS_ENABLE_ASSERTS
	bool	mi_interrupt_set( (Memory_MI_GetRegister(MI_INTR_MASK_REG) & Memory_MI_GetRegister(MI_INTR_REG)) != 0 );
	bool	cause_int_3_set( (gCPUState.CPUControl[C0_CAUSE]._u32_0 & CAUSE_IP3) != 0 );

	DAEDALUS_ASSERT( mi_interrupt_set == cause_int_3_set, "CAUSE_IP3 inconsistant with MI_INTR_REG" );
#endif
#endif
	// If EXL is already set, we jump
	if (gCPUState.CPUControl[C0_SR]._u64 & SR_EXL)
	{
		CPU_Halt("Exception in exception");
		DBGConsole_Msg(0, "[MException within exception] at [R%08x]", gCPUState.CurrentPC);

		// Force default exception handler
		exception_vector = E_VEC;
	}
	else
	{
		gCPUState.CPUControl[C0_SR]._u64 |= SR_EXL;		// Set the EXeption Level (EXL) in STATUS reg (in Kernel mode)

		DAEDALUS_ASSERT( gCPUState.Delay == NO_DELAY || gCPUState.Delay == EXEC_DELAY, "Unexpected branch delay status when handling interrupt" );
		if (gCPUState.Delay == EXEC_DELAY)
		{
			gCPUState.CPUControl[C0_CAUSE]._u64 |= CAUSE_BD;	// Set the BD (Branch Delay) flag in the CAUSE reg
			gCPUState.CPUControl[C0_EPC]._u64   = gCPUState.CurrentPC - 4;	// We want the immediately preceeding instruction
		}
		else
		{
			gCPUState.CPUControl[C0_CAUSE]._u64 &= ~CAUSE_BD;	// Clear the BD (Branch Delay) flag
			gCPUState.CPUControl[C0_EPC]._u64   = gCPUState.CurrentPC;		// We want the current instruction
		}
	}

	if (gCPUState.CPUControl[C0_SR]._u64 & SR_BEV)
	{
		// Use Boot Exception Vectors...
		// 0xBFC00200 + OFFSET
		CPU_SetPC(exception_vector);		// Set the Program Counter to the interrupt vector
	}
	else
	{
		// Use normal exception vectors..
		// 0x80000000 + OFFSET
		CPU_SetPC(exception_vector);		// Set the Program Counter to the interrupt vector
	}
	// Clear CPU_DO_DELAY????
	gCPUState.Delay = NO_DELAY;				// Ensure we don't execute delayed instruction

}
