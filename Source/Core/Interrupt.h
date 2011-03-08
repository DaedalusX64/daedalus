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

#ifndef __INTERRUPT_H__
#define __INTERRUPT_H__

#include "CPU.h"
#include "OSHLE/ultra_rcp.h"
#include "OSHLE/ultra_R4300.h"
/*
enum ETLBExceptionReason
{
	EXCEPTION_TLB_REFILL_LOAD,
	EXCEPTION_TLB_REFILL_STORE,
	EXCEPTION_TLB_INVALID_LOAD,
	EXCEPTION_TLB_INVALID_STORE
};
*/
enum ETLBExceptionReason
{ 
	EXCEPTION_TLB_LOAD, 
	EXCEPTION_TLB_STORE 
};

void R4300_Exception_TLB_Invalid( u32 virtual_address, ETLBExceptionReason reason );
void R4300_Exception_TLB_Refill( u32 virtual_address, ETLBExceptionReason reason );

void R4300_JumpToInterruptVector(u32 exception_vector);

void R4300_Exception_Break();
void R4300_Exception_Syscall();
void R4300_Exception_FP();
void R4300_Exception_CopUnusuable();
//void R4300_Exception_TLB( u32 virtual_address, ETLBExceptionReason reason );

//void R4300_Interrupt_UpdateCause3();		// Update the CAUSE_IP3 value after MI_INTR_MASK_REG or MI_INTR_REG changes
inline void R4300_Interrupt_UpdateCause3()
{
	//
	// If any interrupts pending when they are unmasked, the interrupt fires
	//
	if ((Memory_MI_GetRegister(MI_INTR_MASK_REG) &
		 Memory_MI_GetRegister(MI_INTR_REG)) == 0)
	{
		// Clear the Cause register
		gCPUState.CPUControl[C0_CAUSE]._u32_0 &= ~CAUSE_IP3;
	}
	else
	{
		gCPUState.CPUControl[C0_CAUSE]._u32_0 |= CAUSE_IP3;
		gCPUState.AddJob( CPU_CHECK_INTERRUPTS );
	}
}
inline void Trigger_SIInterrupt(void)
{
	Memory_MI_SetRegisterBits(MI_INTR_REG, MI_INTR_SI);

	if((Memory_MI_GetRegister(MI_INTR_MASK_REG)) & Memory_MI_GetRegister(MI_INTR_MASK_SI))
	{
		gCPUState.CPUControl[C0_CAUSE]._u32_0 |= CAUSE_IP3;
		gCPUState.AddJob( CPU_CHECK_INTERRUPTS );
	}
}

inline void Trigger_PIInterrupt(void)
{
	Memory_MI_SetRegisterBits(MI_INTR_REG, MI_INTR_PI);	

	if((Memory_MI_GetRegister(MI_INTR_MASK_REG)) & Memory_MI_GetRegister(MI_INTR_MASK_PI))
	{
		gCPUState.CPUControl[C0_CAUSE]._u32_0 |= CAUSE_IP3;
		gCPUState.AddJob( CPU_CHECK_INTERRUPTS );
	}
}
inline void Trigger_SPInterrupt(void)
{
	Memory_MI_SetRegisterBits(MI_INTR_REG, MI_INTR_SP);

	if((Memory_MI_GetRegister(MI_INTR_MASK_REG)) & Memory_MI_GetRegister(MI_INTR_MASK_SP))
	{
		gCPUState.CPUControl[C0_CAUSE]._u32_0 |= CAUSE_IP3;
		gCPUState.AddJob( CPU_CHECK_INTERRUPTS );
	}
}

//void R4300_Interrupt_CheckPostponed();

void R4300_Handle_Exception();
void R4300_Handle_Interrupt();

#ifdef DAEDALUS_PROFILE_EXECUTION
extern u32 gNumExceptions;
extern u32 gNumInterrupts;
#endif


#endif
