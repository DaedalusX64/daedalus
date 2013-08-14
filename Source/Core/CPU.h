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

#ifndef CORE_CPU_H_
#define CORE_CPU_H_

#include <stdlib.h>

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

// In practice there should only ever be 2
#define MAX_CPU_EVENTS 4

struct CPUEvent
{
	s32						mCount;
	ECPUEventType			mEventType;
};
DAEDALUS_STATIC_ASSERT( sizeof( CPUEvent ) == 8 );

typedef REG64 register_32x64[32];
typedef REG64 register_16x64[16];
typedef REG32 register_32x32[32];

//
//	We declare various bits of the CPU state in a struct.
//	During dynarec compilation we can keep the base address of this
//	structure cached in a spare register to avoid expensive
//	address-calculations (primarily on the PSP)
//
//	Make sure to reflect changes here to DynaRecStubs.S as well //Corn
//
//	Define to use scratch pad memory located at 0x10000
//
//#define	USE_SCRATCH_PAD

#ifdef USE_SCRATCH_PAD
struct SCPUState
#else
ALIGNED_TYPE(struct, SCPUState, CACHE_ALIGN)
#endif
{
	register_32x64	CPU;				// 0x000 .. 0x100
	register_32x32	CPUControl;			// 0x100 .. 0x180
	union
	{
		register_32x32	FPU;			// 0x180 .. 0x200	Access FPU as 32 x floats
		register_16x64	FPUD;			// 0x180 .. 0x200	Access FPU as 16 x doubles
	};
	register_32x32	FPUControl;			// 0x200 .. 0x280
	u32				CurrentPC;			// 0x280 ..			The current program counter
	u32				TargetPC;			// 0x284 ..			The PC to branch to
	u32				Delay;				// 0x288 ..			Delay state (NO_DELAY, EXEC_DELAY, DO_DELAY)
	volatile u32	StuffToDo;			// 0x28c ..			CPU jobs (see above)

	REG64			MultLo;				// 0x290 ..
	REG64			MultHi;				// 0x298

	REG32			Temp1;				// 0x2A0	Temp storage Dynarec
	REG32			Temp2;				// 0x2A4	Temp storage Dynarec
	REG32			Temp3;				// 0x2A8	Temp storage Dynarec
	REG32			Temp4;				// 0x2AC	Temp storage Dynarec

	CPUEvent		Events[ MAX_CPU_EVENTS ];	// 0x2B0 //In practice there should only ever be 2 CPU_EVENTS
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

#ifdef USE_SCRATCH_PAD
extern SCPUState *gPtrCPUState;
#define gCPUState (*gPtrCPUState)
#else	//USE_SCRATCH_PAD
ALIGNED_EXTERN(SCPUState, gCPUState, CACHE_ALIGN);
#endif //USE_SCRATCH_PAD

#define gGPR (gCPUState.CPU)
//*****************************************************************************
//
//*****************************************************************************
bool	CPU_RomOpen();
void	CPU_RomClose();
void	CPU_Step();
void	CPU_Skip();
bool	CPU_Run();
bool	CPU_RequestSaveState( const char * filename );
bool	CPU_RequestLoadState( const char * filename );
void	CPU_Halt( const char * reason );
void	CPU_SelectCore();
u32		CPU_GetVideoInterruptEventCount();
void	CPU_SetVideoInterruptEventCount( u32 count );
void	CPU_DynarecEnable();
void	R4300_CALL_TYPE CPU_InvalidateICacheRange( u32 address, u32 length );
void	R4300_CALL_TYPE CPU_InvalidateICache();
void	CPU_SetCompare(u32 value);
#ifdef DAEDALUS_BREAKPOINTS_ENABLED
void	CPU_AddBreakPoint( u32 address );						// Add a break point at address dwAddress
void	CPU_EnableBreakPoint( u32 address, bool enable );		// Enable/Disable the breakpoint as the specified address
#endif
bool	CPU_IsRunning();
void	CPU_AddEvent( s32 count, ECPUEventType event_type );
void	CPU_SkipToNextEvent();
bool	CPU_CheckStuffToDo();

typedef void (*VblCallbackFn)(void * arg);
void CPU_RegisterVblCallback(VblCallbackFn fn, void * arg);
void CPU_UnregisterVblCallback(VblCallbackFn fn, void * arg);

// For PSP, we just keep running forever. For other platforms we need to bail when the user quits.
#ifdef DAEDALUS_PSP
#define CPU_KeepRunning() (1)
#else
#define CPU_KeepRunning() (CPU_IsRunning())
#endif

inline void CPU_SetPC( u32 pc )		{ gCPUState.CurrentPC = pc; }
inline void INCREMENT_PC()			{ gCPUState.CurrentPC += 4; }
inline void DECREMENT_PC()			{ gCPUState.CurrentPC -= 4; }

inline void CPU_TakeBranch( u32 new_pc ) { gCPUState.TargetPC = new_pc; gCPUState.Delay = DO_DELAY; }


#define COUNTER_INCREMENT_PER_OP			1

extern u32		gLastPC;
extern u8 *		gLastAddress;

// Take advantage of the cooperative multitasking
// of the PSP to make locking/unlocking as fast as possible.
//
extern volatile u32 eventQueueLocked;

#define LOCK_EVENT_QUEUE() CSpinLock _lock( &eventQueueLocked )
#define RESET_EVENT_QUEUE_LOCK() eventQueueLocked = 0;

#ifdef FRAGMENT_SIMULATE_EXECUTION
void	CPU_ExecuteOpRaw( u32 count, u32 address, OpCode op_code, CPU_Instruction p_instruction, bool * p_branch_taken );
#endif
// Needs to be callable from assembly
extern "C"
{
	void	R4300_CALL_TYPE CPU_UpdateCounter( u32 ops_executed );
#ifdef FRAGMENT_SIMULATE_EXECUTION
	void	CPU_UpdateCounterNoInterrupt( u32 ops_exexuted );
#endif
	void	CPU_HANDLE_COUNT_INTERRUPT();
}

extern	void (* g_pCPUCore)();

//***********************************************
// This function gets called *alot* //Corn
//***********************************************
//
// From ReadAddress
//
#define CPU_FETCH_INSTRUCTION(ptr, pc)								\
	const MemFuncRead & m( g_MemoryLookupTableRead[ pc >> 18 ] );	\
	if( DAEDALUS_EXPECT_LIKELY(m.pRead != NULL) )					\
	{																\
/* Access through pointer with no function calls at all (Fast) */	\
		ptr = ( m.pRead + pc );										\
	}																\
	else															\
	{																\
/* ROM or TLB and possible trigger for an exception (Slow)*/		\
		ptr = (u8*)m.ReadFunc( pc );								\
		if( gCPUState.GetStuffToDo() )								\
			return;													\
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

#endif // CORE_CPU_H_
