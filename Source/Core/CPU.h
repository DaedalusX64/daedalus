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

#ifndef CPU_H_
#define CPU_H_

#include "R4300Instruction.h"
#include "R4300OpCode.h"
#include "Memory.h"
#include "TLB.h"
#include "Utility/SpinLock.h"
//*****************************************************************************
//
//*****************************************************************************
enum EDelayType
{
	NO_DELAY = 0,
	EXEC_DELAY,
	DO_DELAY
};

#ifdef DAEDALUS_BREAKPOINTS_ENABLED
struct DBG_BreakPoint
{
	OpCode mOriginalOp;
	bool  mEnabled;
	bool  mTemporaryDisable;		// Set to true when BP is activated - this lets us type
									// go immediately after a bp and execution will resume.
									// otherwise it would keep activating the bp
									// The patch bp r4300 instruction clears this
									// when it is next executed
};
#endif
//
// CPU Jobs
//
#define CPU_CHECK_EXCEPTIONS				0x00000001
#define CPU_CHECK_INTERRUPTS				0x00000002
#define CPU_STOP_RUNNING					0x00000008
#define CPU_CHANGE_CORE						0x00000010

//*****************************************************************************
// External declarations
//*****************************************************************************
#ifdef DAEDALUS_BREAKPOINTS_ENABLED
#include <vector>
extern std::vector< DBG_BreakPoint > g_BreakPoints;
#endif

// Arbitrary unique numbers for different timing related events:
enum	ECPUEventType
{
	CPU_EVENT_VBL = 1,
	CPU_EVENT_COMPARE,
	CPU_EVENT_AUDIO,
	CPU_EVENT_SPINT,
};

struct CPUEvent
{
	s32						mCount;
	ECPUEventType			mEventType;
};
DAEDALUS_STATIC_ASSERT( sizeof( CPUEvent ) == 8 );

typedef REG64 register_set[32];

//
//	We declare various bits of the CPU state in a struct.
//	During dynarec compilation we can keep the base address of this
//	structure cached in a spare register to avoid expensive
//	address-calculations (primarily on the PSP)
//
ALIGNED_TYPE(struct, SCPUState, CACHE_ALIGN)
{
	static const u32	MAX_CPU_EVENTS = 4;		// In practice there should only ever be 2

	register_set	CPU;				// 0x000 .. 0x100
	register_set	CPUControl;			// 0x100 .. 0x200
	register_set	FPU;				// 0x200 .. 0x300
	register_set	FPUControl;			// 0x300 .. 0x400
	u32				CurrentPC;			// 0x400 ..			The current program counter
	u32				TargetPC;			// 0x404 ..			The PC to branch to
	u32				Delay;				// 0x408 ..			Delay state (NO_DELAY, EXEC_DELAY, DO_DELAY)
	volatile u32	StuffToDo;			// 0x40c ..			CPU jobs (see above)

	REG64			MultLo;				// 0x410 ..
	REG64			MultHi;				// 0x418

	CPUEvent		Events[ MAX_CPU_EVENTS ];	// 0x420
	u32				NumEvents;

	void			AddJob( u32 job );
	void			ClearJob( u32 job );
	inline u32		GetStuffToDo() const			{ return StuffToDo; }
	inline bool		IsJobSet( u32 job ) const		{ return ( StuffToDo & job ) != 0; }
	void			ClearStuffToDo();
#ifdef DAEDALUS_ENABLE_SYNCHRONISATION
	void			Dump();
#endif
};

ALIGNED_EXTERN(SCPUState, gCPUState, CACHE_ALIGN);

#define gGPR (gCPUState.CPU)
//*****************************************************************************
//
//*****************************************************************************
void	CPU_Reset();
void	CPU_Finalise();
//void	CPU_Step();
//void	CPU_Skip();
bool	CPU_Run();
//bool	CPU_StartThread( char * p_failure_reason, u32 length );
//void	CPU_StopThread();
bool	CPU_SaveState( const char * filename );
bool	CPU_LoadState( const char * filename );
void	CPU_Halt( const char * reason );
void	CPU_SelectCore();
u32		CPU_GetVideoInterruptEventCount();
void	CPU_SetVideoInterruptEventCount( u32 count );
//void	CPU_DynarecEnable();
void	R4300_CALL_TYPE CPU_InvalidateICacheRange( u32 address, u32 length );
void	R4300_CALL_TYPE CPU_InvalidateICache();
void	CPU_SetCompare(u32 value);
#ifdef DAEDALUS_BREAKPOINTS_ENABLED
void	CPU_AddBreakPoint( u32 address );						// Add a break point at address dwAddress
void	CPU_EnableBreakPoint( u32 address, bool enable );		// Enable/Disable the breakpoint as the specified address
#endif
bool	CPU_IsRunning();
//u32		CPU_GetVerticalInterruptCount();
void	CPU_AddEvent( s32 count, ECPUEventType event_type );
void	CPU_SkipToNextEvent();
bool	CPU_CheckStuffToDo();
//void	CPU_WaitFinish();

inline void CPU_SetPC( u32 pc )		{ gCPUState.CurrentPC = pc; }
inline void INCREMENT_PC()			{ gCPUState.CurrentPC += 4; }
inline void DECREMENT_PC()			{ gCPUState.CurrentPC -= 4; }

inline void CPU_TakeBranch( u32 new_pc ){	gCPUState.TargetPC = new_pc;	gCPUState.Delay = DO_DELAY;	}


#define COUNTER_INCREMENT_PER_OP			1
//*****************************************************************************
//
//*****************************************************************************
static u32		gLastPC = 0xffffffff;
static u8 *		gLastAddress = NULL;

// Take advantage of the cooperative multitasking
// of the PSP to make locking/unlocking as fast as possible.
//
static volatile u32 eventQueueLocked;

#define LOCK_EVENT_QUEUE() CSpinLock _lock( &eventQueueLocked )
#define RESET_EVENT_QUEUE_LOCK() eventQueueLocked = 0;

//*****************************************************************************
//
//*****************************************************************************
/*
enum ECPUBranchType
{
	CPU_BRANCH_DIRECT = 0,		// i.e. jump to a fixed address
	CPU_BRANCH_INDIRECT,		// i.e. jump to the contents of a register
};
*/
#ifdef FRAGMENT_SIMULATE_EXECUTION
//*****************************************************************************
//
//*****************************************************************************
void	CPU_ExecuteOpRaw( u32 count, u32 address, OpCode op_code, CPU_Instruction p_instruction, bool * p_branch_taken );
#endif
// Needs to be callable from assembly
extern "C"
{
	void	CPU_UpdateCounter( u32 ops_executed );
	void	CPU_UpdateCounterNoInterrupt( u32 ops_exexuted );
	void	CPU_HANDLE_COUNT_INTERRUPT();
}

extern	void (* g_pCPUCore)();
//***********************************************
//These two functions gets called *alot* //Corn
//CPU_FetchInstruction
//CPU_FetchInstruction_Refill
//***********************************************
inline bool CPU_FetchInstruction_Refill( u32 pc, OpCode * opcode )
{
	gLastAddress = (u8 *)ReadAddress( pc );
	gLastPC = pc;
	*opcode = *(OpCode *)gLastAddress;
	return gCPUState.GetStuffToDo() == 0;
}

//*****************************************************************************
//
//*****************************************************************************
inline bool CPU_FetchInstruction( u32 pc, OpCode * opcode )
{
	const u32 PAGE_MASK_BITS = 12;		// 1<<12 == 4096

	if( (pc>>PAGE_MASK_BITS) == (gLastPC>>PAGE_MASK_BITS) )
	{
		s32		offset( pc - gLastPC );

		gLastAddress += offset;
		DAEDALUS_ASSERT( gLastAddress == ReadAddress(pc), "Cached Instruction Pointer base is out of sync" );
		gLastPC = pc;
		*opcode = *(OpCode *)gLastAddress;
		return true;
	}

	return CPU_FetchInstruction_Refill( pc, opcode );
}

//***********************************************
//This function gets called *alot* //Corn
//CPU_ProcessEventCycles
//***********************************************
inline bool CPU_ProcessEventCycles( u32 cycles )
{
	LOCK_EVENT_QUEUE();

	DAEDALUS_ASSERT( gCPUState.NumEvents > 0, "There are no events" );
	gCPUState.Events[ 0 ].mCount -= cycles;
	return gCPUState.Events[ 0 ].mCount <= 0;
}

#ifdef DAEDALUS_PROFILE_EXECUTION
extern u64									gTotalInstructionsExecuted;
extern u64									gTotalInstructionsEmulated;
extern u32									g_HardwareInterrupt;
#endif

#ifdef DAEDALUS_ENABLE_SYNCHRONISATION
extern u32 CPU_ProduceRegisterHash();
#endif 

#endif // CPU_H_
