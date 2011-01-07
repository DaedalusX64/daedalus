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

// Do not use CAUSE_EXCMASK, else most games will fail when CAUSE is set 32bits
//
#define SET_EXCEPTION(exception)	{ gCPUState.CPUControl[C0_CAUSE]._u32_0 &= NOT_CAUSE_EXCMASK/*CAUSE_EXCMASK*/; gCPUState.CPUControl[C0_CAUSE]._u32_0 |= exception; }

//static ETLBExceptionReason g_nTLBExceptionReason;
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
/*
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
*/
//*****************************************************************************
//
//*****************************************************************************
void R4300_Exception_Break()
{
	DAEDALUS_ASSERT( gExceptionVector == u32(~0), "Exception vector already set" );
	DAEDALUS_ASSERT( gExceptionPC == u32(~0), "Exception PC already set" );

	// Clear CAUSE_EXCMASK
	SET_EXCEPTION( EXC_BREAK ) 

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
	SET_EXCEPTION( EXC_SYSCALL ) 

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
	SET_EXCEPTION(EXC_CPU) 

    gCPUState.CPUControl[C0_CAUSE]._u32_0 &= 0xCFFFFFFF;
    gCPUState.CPUControl[C0_CAUSE]._u32_0 |= SR_CU0;

	/*if(gCPUState.Delay == EXEC_DELAY)
        gCPUState.CPUControl[C0_CAUSE]._u32_0 |= CAUSE_BD;
    else
         gCPUState.CPUControl[C0_CAUSE]._u32_0 &= ~CAUSE_BD;*/

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
	SET_EXCEPTION( EXC_FPE ) 

	gExceptionVector = E_VEC;
	gExceptionPC = gCPUState.CurrentPC;
	gExceptionWasDelay = gCPUState.Delay == EXEC_DELAY;
	gCPUState.AddJob( CPU_CHECK_EXCEPTIONS );
}

//*****************************************************************************
//
//*****************************************************************************
void R4300_Exception_TLB_Invalid( u32 virtual_address, ETLBExceptionReason reason )
{
	DAEDALUS_ASSERT( gExceptionVector == u32(~0), "Exception vector already set" );
	DAEDALUS_ASSERT( gExceptionPC == u32(~0), "Exception PC already set" );

	gCPUState.CPUControl[C0_BADVADDR]._u32_0 = virtual_address;

	gCPUState.CPUControl[C0_CONTEXT]._u32_0 &= 0xFF800000;	// Mask off bottom 23 bits
	gCPUState.CPUControl[C0_CONTEXT]._u32_0 |= ((virtual_address >> 13) << 4);

	gCPUState.CPUControl[C0_ENTRYHI]._u32_0 &= 0x00001FFF;	// Mask off the top bit 13-31 
	gCPUState.CPUControl[C0_ENTRYHI]._u32_0 |= (virtual_address & 0xFFFFE000);

	u32		exception_code( 0 );

	if( reason == EXCEPTION_TLB_STORE )
		exception_code = EXC_WMISS; 
	else
		exception_code = EXC_RMISS; 

	SET_EXCEPTION( exception_code ) 

	gExceptionVector = E_VEC;
	gExceptionPC = gCPUState.CurrentPC;
	gExceptionWasDelay = gCPUState.Delay == EXEC_DELAY;
	gCPUState.AddJob( CPU_CHECK_EXCEPTIONS );
}

//*****************************************************************************
//
//*****************************************************************************
void R4300_Exception_TLB_Refill( u32 virtual_address, ETLBExceptionReason reason )
{
	DAEDALUS_ASSERT( gExceptionVector == u32(~0), "Exception vector already set" );
	DAEDALUS_ASSERT( gExceptionPC == u32(~0), "Exception PC already set" );

	gCPUState.CPUControl[C0_BADVADDR]._u32_0 = virtual_address;

	gCPUState.CPUControl[C0_CONTEXT]._u32_0 &= 0xFF800000;	// Mask off bottom 23 bits
	gCPUState.CPUControl[C0_CONTEXT]._u32_0 |= ((virtual_address >> 13) << 4);

	gCPUState.CPUControl[C0_ENTRYHI]._u32_0 &= 0x00001FFF;	// Mask off the top bit 13-31 
	gCPUState.CPUControl[C0_ENTRYHI]._u32_0 |= (virtual_address & 0xFFFFE000);

	u32		exception_code( 0 );

	if( reason == EXCEPTION_TLB_STORE )
		exception_code = EXC_WMISS; 
	else
		exception_code = EXC_RMISS; 

	SET_EXCEPTION( exception_code ) 

	gExceptionVector = UT_VEC;
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

	if(gCPUState.CPUControl[C0_SR]._u32_0 & gCPUState.CPUControl[C0_CAUSE]._u32_0 & CAUSE_IPMASK)  // Are interrupts pending/wanted?
	{
		if(gCPUState.CPUControl[C0_SR]._u32_0 & SR_IE)												// Are interrupts enabled?
		{
			// SR_EXL + SR_ERL = 0x00000006
			if((gCPUState.CPUControl[C0_SR]._u32_0 & 0x0006/*(SR_EXL|SR_ERL)*/) == 0x0000)			// Ensure ERL/EXL are not set
			{       
#ifdef DAEDALUS_PROFILE_EXECUTION
				gNumInterrupts++;
#endif
				// Clear CAUSE_EXCMASK
				SET_EXCEPTION( EXC_INT )
				R4300_JumpToInterruptVector( E_VEC );
			} 
		} 
    } 
}
//*****************************************************************************
//
//*****************************************************************************
void R4300_JumpToInterruptVector(u32 exception_vector)
{

#if defined(DAEDALUS_ENABLE_ASSERTS) || defined(DAEDALUS_PROFILE_EXECUTION)
	bool	mi_interrupt_set( (Memory_MI_GetRegister(MI_INTR_MASK_REG) & Memory_MI_GetRegister(MI_INTR_REG)) != 0 );
	bool	cause_int_3_set( (gCPUState.CPUControl[C0_CAUSE]._u32_0 & CAUSE_IP3) != 0 );

	DAEDALUS_ASSERT( mi_interrupt_set == cause_int_3_set, "CAUSE_IP3 inconsistant with MI_INTR_REG" );
#endif

	gCPUState.CPUControl[C0_SR]._u32_0 |= SR_EXL;							
	gCPUState.CPUControl[C0_EPC]._u32_0  = gCPUState.CurrentPC;          

	if(gCPUState.Delay == EXEC_DELAY)
	{
		gCPUState.CPUControl[C0_CAUSE]._u32_0 |= CAUSE_BD;				
		gCPUState.CPUControl[C0_EPC]._u32_0   -= 4;						
	}
	else
	{
		gCPUState.CPUControl[C0_CAUSE]._u32_0 &= ~CAUSE_BD;				
	}

	CPU_SetPC( exception_vector );							
	gCPUState.Delay = NO_DELAY;											
}