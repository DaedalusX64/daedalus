/*
Copyright (C) 2001-2007 StrmnNrmn

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

#include <algorithm>
#include <string>
#include <vector>

#include "Cheats.h"
#include "Dynamo.h"
#include "Interpret.h"
#include "Interrupt.h"
#include "Memory.h"
#include "R4300.h"
#include "Registers.h"					// For REG_?? defines
#include "ROM.h"
#include "ROMBuffer.h"
#include "RSP_HLE.h"
#include "Save.h"
#include "SaveState.h"

#include "Config/ConfigOptions.h"
#include "Debug/DBGConsole.h"
#include "Debug/DebugLog.h"
#include "OSHLE/ultra_R4300.h"
#include "System/System.h"
#include "Utility/AtomicPrimitives.h"
#include "Utility/FramerateLimiter.h"
#include "Utility/Hash.h"
#include "Utility/Macros.h"
#include "Utility/PrintOpCode.h"
#include "Utility/Synchroniser.h"
#include "Utility/Thread.h"
#include "Utility/Mutex.h"

#ifdef DAEDALUS_W32
#include "Plugins/AudioPlugin.h"
#endif

extern void R4300_Init();

//
//	New dynarec engine
//
#ifdef DAEDALUS_PROFILE_EXECUTION
u64					gTotalInstructionsExecuted = 0;
u64					gTotalInstructionsEmulated = 0;
#endif

#ifdef DAEDALUS_BREAKPOINTS_ENABLED
std::vector< DBG_BreakPoint > g_BreakPoints;
#endif

volatile u32 eventQueueLocked = 0;

static bool			gCPURunning        = false;			// CPU is actively running
u32					gLastPC            = 0xffffffff;
u8 *				gLastAddress       = NULL;
std::string			gSaveStateFilename = "";

static bool			gCPUStopOnSimpleState = false;			// When stopping, try to stop in a 'simple' state (i.e. no RSP running and not in a branch delay slot)
static Mutex		gSaveStateMutex;

enum ESaveStateOperation
{
	SSO_NONE,
	SSO_SAVE,
	SSO_LOAD,
};

static ESaveStateOperation		gSaveStateOperation = SSO_NONE;

const  u32			kInitialVIInterruptCycles = 62500;
static u32			gVerticalInterrupts = 0;
static u32			VI_INTR_CYCLES = kInitialVIInterruptCycles;

#ifdef USE_SCRATCH_PAD
SCPUState *gPtrCPUState = (SCPUState*)0x10000;
#else
ALIGNED_GLOBAL(SCPUState, gCPUState, CACHE_ALIGN);
#endif

static bool	CPU_IsStateSimple()		   DAEDALUS_ATTRIBUTE_CONST;
void (* g_pCPUCore)();

typedef void (*VblCallbackFn)(void * arg);
struct VblCallback
{
	VblCallbackFn		Fn;
	void *				Arg;
};

static std::vector<VblCallback>		gVblCallbacks;


void CPU_RegisterVblCallback(VblCallbackFn fn, void * arg)
{
	VblCallback callback = { fn, arg };
	gVblCallbacks.push_back(callback);
}

void CPU_UnregisterVblCallback(VblCallbackFn fn, void * arg)
{
	for (std::vector<VblCallback>::iterator it = gVblCallbacks.begin(); it != gVblCallbacks.end(); ++it)
	{
		if (it->Fn == fn && it->Arg == arg)
		{
			gVblCallbacks.erase(it);
			break;
		}
	}
}

void CPU_SkipToNextEvent()
{
	LOCK_EVENT_QUEUE();

	DAEDALUS_ASSERT( gCPUState.NumEvents > 0, "There are no events" );
	gCPUState.CPUControl[C0_COUNT]._u32 += (gCPUState.Events[ 0 ].mCount - 1);
	gCPUState.Events[ 0 ].mCount = 1;
}

static void CPU_ResetEventList()
{
	gCPUState.Events[ 0 ].mCount     = kInitialVIInterruptCycles;
	gCPUState.Events[ 0 ].mEventType = CPU_EVENT_VBL;
	gCPUState.NumEvents = 1;

	RESET_EVENT_QUEUE_LOCK();
}

void CPU_AddEvent( s32 count, ECPUEventType event_type )
{
	LOCK_EVENT_QUEUE();

	DAEDALUS_ASSERT( count > 0, "Count is invalid" );
	DAEDALUS_ASSERT( gCPUState.NumEvents < MAX_CPU_EVENTS, "Too many events" );

	u32 event_idx;
	for( event_idx = 0; event_idx < gCPUState.NumEvents; ++event_idx )
	{
		CPUEvent & event = gCPUState.Events[ event_idx ];

		if( count <= event.mCount )
		{
			//
			//	This event belongs before the subsequent one so insert a space for it here and break out
			//	Don't forget to decrement the counter for the subsequent event
			//
			event.mCount -= count;

			u32 num_to_copy = gCPUState.NumEvents - event_idx;
			if( num_to_copy > 0 )
			{
				memmove( &gCPUState.Events[ event_idx+1 ], &gCPUState.Events[ event_idx ], num_to_copy * sizeof( CPUEvent ) );
			}
			break;
		}

		//
		//	Decrease counter by that for this event
		//
		count -= event.mCount;
	}

	DAEDALUS_ASSERT( event_idx <= gCPUState.NumEvents, "Invalid idx" );
	gCPUState.Events[ event_idx ].mCount = count;
	gCPUState.Events[ event_idx ].mEventType = event_type;
	gCPUState.NumEvents++;
}

static void CPU_SetCompareEvent( s32 count )
{
	{
		LOCK_EVENT_QUEUE();

		DAEDALUS_ASSERT( count > 0, "Count is invalid" );

		//
		//	Remove any existing compare events. Need to adjust any subsequent timer's count.
		//
		for( u32 i = 0; i < gCPUState.NumEvents; ++i )
		{
			if( gCPUState.Events[ i ].mEventType == CPU_EVENT_COMPARE )
			{
				//
				//	Check for a following event, and remove
				//
				if( i+1 < gCPUState.NumEvents )
				{
					gCPUState.Events[ i+1 ].mCount += gCPUState.Events[ i ].mCount;
					u32 num_to_copy = gCPUState.NumEvents - (i+1);
					memmove( &gCPUState.Events[ i ], &gCPUState.Events[ i+1 ], num_to_copy * sizeof( CPUEvent ) );
				}
				gCPUState.NumEvents--;
				break;
			}
		}
	}

	CPU_AddEvent( count, CPU_EVENT_COMPARE );
}

static ECPUEventType CPU_PopEvent()
{
	LOCK_EVENT_QUEUE();

	DAEDALUS_ASSERT( gCPUState.NumEvents > 0, "Event queue empty" );
	DAEDALUS_ASSERT( gCPUState.Events[ 0 ].mCount <= 0, "Popping event when cycles remain" );
	//DAEDALUS_ASSERT( gCPUState.Events[ 0 ].mCount == 0, "Popping event with a bit of underflow" );

	ECPUEventType event_type = gCPUState.Events[ 0 ].mEventType;

	u32	num_to_copy = gCPUState.NumEvents - 1;
	if( num_to_copy > 0 )
	{
		memmove( &gCPUState.Events[ 0 ], &gCPUState.Events[ 1 ], num_to_copy * sizeof( CPUEvent ) );
	}
	gCPUState.NumEvents--;

	return event_type;
}

// XXXX This is for savestate. Looks very suspicious to me
u32 CPU_GetVideoInterruptEventCount()
{
	for( u32 i = 0; i < gCPUState.NumEvents; ++i )
	{
		if(gCPUState.Events[ i ].mEventType == CPU_EVENT_VBL)
		{
			return gCPUState.Events[ i ].mCount;
		}
	}

	return 0;
}

// XXXX This is for savestate. Looks very suspicious to me
void CPU_SetVideoInterruptEventCount( u32 count )
{
	for( u32 i = 0; i < gCPUState.NumEvents; ++i )
	{
		if(gCPUState.Events[ i ].mEventType == CPU_EVENT_VBL)
		{
			gCPUState.Events[ i ].mCount = count;
			return;
		}
	}
}

void SCPUState::ClearStuffToDo()
{
	StuffToDo = 0;
	Dynarec_ClearedCPUStuffToDo();
}

void SCPUState::AddJob( u32 job )
{
	u32 stuff( AtomicBitSet( &StuffToDo, 0xffffffff, job ) );
	if( stuff != 0 )
	{
		Dynarec_SetCPUStuffToDo();
	}
}

void SCPUState::ClearJob( u32 job )
{
	u32 stuff( AtomicBitSet( &StuffToDo, ~job, 0x00000000 ) );
	if( stuff == 0 )
	{
		Dynarec_ClearedCPUStuffToDo();
	}
}

#ifdef DAEDALUS_ENABLE_SYNCHRONISATION
static const char * const kRegisterNames[] =
{
	"zr", "at", "v0", "v1", "a0", "a1", "a2", "a3",
	"t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
	"s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7",
	"t8", "t9", "k0", "k1", "gp", "sp", "fp", "ra"
};
DAEDALUS_STATIC_ASSERT(ARRAYSIZE(kRegisterNames) == 32);

void SCPUState::Dump()
{
	DBGConsole_Msg(0, "Emulation CPU State:");
	{
		for(int i=0; i<32; i+=4)
		{
			DBGConsole_Msg(0, "%s:%08X %s:%08X %s:%08X %s:%08X",
			kRegisterNames[i+0], gCPUState.CPU[i+0]._u32_0,
			kRegisterNames[i+1], gCPUState.CPU[i+1]._u32_0,
			kRegisterNames[i+2], gCPUState.CPU[i+2]._u32_0,
			kRegisterNames[i+3], gCPUState.CPU[i+3]._u32_0);
		}

		DBGConsole_Msg(0, "TargetPC: %08x", gCPUState.TargetPC);
		DBGConsole_Msg(0, "CurrentPC: %08x", gCPUState.CurrentPC);
		DBGConsole_Msg(0, "Delay: %08x", gCPUState.Delay);
	}
}
#endif

bool CPU_RomOpen()
{
	DBGConsole_Msg(0, "Resetting CPU");

	gLastPC = 0xffffffff;
	gLastAddress = NULL;

	gCPURunning = false;
	gCPUStopOnSimpleState = false;
	RESET_EVENT_QUEUE_LOCK();

	memset(&gCPUState, 0, sizeof(gCPUState));

	CPU_SetPC( 0xbfc00000 );
	gCPUState.MultHi._u64 = 0;
	gCPUState.MultLo._u64 = 0;

	for(u32 i = 0; i < 32; i++)
	{
		gCPUState.CPU[i]._u64        = 0;
		gCPUState.CPUControl[i]._u32 = 0;
		gCPUState.FPU[i]._u32        = 0;
		gCPUState.FPUControl[i]._u32 = 0;
	}

	// Init TLBs:
	for (u32 i = 0; i < 32; i++)
	{
		g_TLBs[i].Reset();
	}

	// From R4300 manual
	gCPUState.CPUControl[C0_RAND]._u32   = 32-1;			// TLBENTRIES-1
	//gCPUState.CPUControl[C0_SR]._u32   = 0x70400004;	//*SR_FR |*/ SR_ERL | SR_CU2|SR_CU1|SR_CU0;
	R4300_SetSR(0x70400004);
	gCPUState.CPUControl[C0_PRID]._u32   = 0x00000b10;	// Was 0xb00 - test rom reports 0xb10!!
	gCPUState.CPUControl[C0_CONFIG]._u32 = 0x0006E463;	// 0x00066463;
	gCPUState.CPUControl[C0_WIRED]._u32  = 0x0;

	gCPUState.FPUControl[0]._u32 = 0x00000511;

	Memory_MI_SetRegister(MI_VERSION_REG, 0x02020102);

	((u32 *)g_pMemoryBuffers[MEM_RI_REG])[3] = 1;					// RI_CONFIG_REG Skips most of init

	R4300_Init();

	gCPUState.Delay = NO_DELAY;
	gCPUState.ClearStuffToDo();
	gVerticalInterrupts = 0;

	// Clear event list:
	CPU_ResetEventList();

#ifdef DAEDALUS_BREAKPOINTS_ENABLED
	g_BreakPoints.clear();
#endif

	Dynamo_Reset();

	CPU_SelectCore();
	return true;
}

void CPU_RomClose()
{
#ifdef DAEDALUS_ENABLE_DYNAREC
	#ifdef DAEDALUS_DEBUG_DYNAREC
		//This will dump the fragment cache on exit to ROMs menu
		//CPU_DumpFragmentCache();
	#endif
#endif
}

static bool	CPU_IsStateSimple()
{
	bool rsp_halted = !RSP_IsRunning();

	return rsp_halted && (gCPUState.Delay == NO_DELAY);
}

void CPU_SelectCore()
{
#ifdef DAEDALUS_ENABLE_DYNAREC
	if (gDynarecEnabled)
		Dynamo_SelectCore();
	else
#endif
		Inter_SelectCore();

	if( gCPUStopOnSimpleState && CPU_IsStateSimple() )
	{
		gCPUState.AddJob( CPU_STOP_RUNNING );
	}
	else
	{
		gCPUState.AddJob( CPU_CHANGE_CORE );
	}
}

bool CPU_RequestSaveState( const char * filename )
{
	// Call SaveState_SaveToFile directly if the CPU is not running.
	DAEDALUS_ASSERT(gCPURunning, "Expecting the CPU to be running at this point");

	MutexLock lock( &gSaveStateMutex );

	// Abort if already in the process of loading/saving
	if( gSaveStateOperation != SSO_NONE )
	{
		return false;
	}

	gSaveStateOperation = SSO_SAVE;
	gSaveStateFilename = filename;
	gCPUState.AddJob(CPU_CHANGE_CORE);

	return true;
}

bool CPU_RequestLoadState( const char * filename )
{
	// Call SaveState_SaveToFile directly if the CPU is not running.
	DAEDALUS_ASSERT(gCPURunning, "Expecting the CPU to be running at this point");

	MutexLock lock( &gSaveStateMutex );

	// Abort if already in the process of loading/saving
	if( gSaveStateOperation != SSO_NONE )
	{
		return false;
	}

	gSaveStateOperation = SSO_LOAD;
	gSaveStateFilename = filename;
	gCPUState.AddJob(CPU_CHANGE_CORE);

	return true;	// XXXX could fail
}

static void HandleSaveStateOperationOnVerticalBlank()
{
	DAEDALUS_ASSERT(gCPURunning, "Expecting the CPU to be running at this point");

	if( gSaveStateOperation == SSO_NONE )
		return;

	MutexLock lock( &gSaveStateMutex );

	//
	// Handle the save state
	//
	switch( gSaveStateOperation )
	{
	case SSO_NONE:
		DAEDALUS_ERROR( "Unreachable" );
		break;
	case SSO_SAVE:
		DBGConsole_Msg(0, "Saving '%s'\n", gSaveStateFilename.c_str());
		SaveState_SaveToFile( gSaveStateFilename.c_str() );
		gSaveStateOperation = SSO_NONE;
		break;
	case SSO_LOAD:
		DBGConsole_Msg(0, "Loading '%s'\n", gSaveStateFilename.c_str());

		// Try to load the savestate immediately. If this fails, it
		// usually means that we're running the correct rom (we should have a
		// separate return code to check this case). In that case we
		// stop the cpu and handle the load in
		// HandleSaveStateOperationOnCPUStopRunning.
		if (SaveState_LoadFromFile( gSaveStateFilename.c_str() ))
		{
			CPU_ResetFragmentCache();
			gSaveStateOperation = SSO_NONE;
		}
		else
		{
			// Halt the CPU so that we can swap the rom safely and load the savesate.
			CPU_Halt("Load SaveSate");
			// NB: return without clearing gSaveStateOperation
		}
		break;
	}
}

// Returns true if we handled a load request and should keep running.
static bool HandleSaveStateOperationOnCPUStopRunning()
{
	if (gSaveStateOperation != SSO_LOAD)
		return false;

	MutexLock lock( &gSaveStateMutex );

	gSaveStateOperation = SSO_NONE;

	if (const char * rom_filename = SaveState_GetRom(gSaveStateFilename.c_str()))
	{
		System_Close();
		System_Open(rom_filename);
		SaveState_LoadFromFile(gSaveStateFilename.c_str());
	}
	else
	{
		DBGConsole_Msg(0, "Couldn't find matching rom for %s\n", gSaveStateFilename.c_str());
		// Keep running with the current rom.
	}

	return true;
}

bool CPU_Run()
{
	if (!RomBuffer::IsRomLoaded())
		return false;

	while (1)
	{
		gCPURunning = true;
		gCPUStopOnSimpleState = false;
		DAEDALUS_ASSERT(gSaveStateOperation == SSO_NONE, "Shouldn't have a save state operation queued.");

		RESET_EVENT_QUEUE_LOCK();

		while (gCPURunning)
		{
			g_pCPUCore();
		}

		if (!HandleSaveStateOperationOnCPUStopRunning())
			break;
	}

	DAEDALUS_ASSERT(!gCPURunning, "gCPURunning should be false by now.");

	return true;
}

void CPU_Halt( const char * reason )
{
	DBGConsole_Msg( 0, "CPU Halting: %s", reason );
	gCPUStopOnSimpleState = true;
	gCPUState.AddJob( CPU_STOP_RUNNING );
}

#ifdef DAEDALUS_BREAKPOINTS_ENABLED
void CPU_AddBreakPoint( u32 address )
{
	OpCode * pdwOp;

	// Force 4 byte alignment
	address &= 0xFFFFFFFC;

	if (!Memory_GetInternalReadAddress(address, (void**)&pdwOp))
	{
		DBGConsole_Msg(0, "Invalid Address for BreakPoint: 0x%08x", address);
	}
	else
	{
		DBG_BreakPoint bpt;
		DBGConsole_Msg(0, "[YInserting BreakPoint at 0x%08x]", address);

		bpt.mOriginalOp       = *pdwOp;
		bpt.mEnabled          = true;
		bpt.mTemporaryDisable = false;
		g_BreakPoints.push_back(bpt);

		pdwOp->op       = OP_DBG_BKPT;
		pdwOp->bp_index = (g_BreakPoints.size() - 1);
	}
}
#endif

#ifdef DAEDALUS_BREAKPOINTS_ENABLED
void CPU_EnableBreakPoint( u32 address, bool enable )
{
	OpCode * pdwOp;

	// Force 4 byte alignment
	address &= 0xFFFFFFFC;

	if (!Memory_GetInternalReadAddress(address, (void**)&pdwOp))
	{
		DBGConsole_Msg(0, "Invalid Address for BreakPoint: 0x%08x", address);
	}
	else
	{
		OpCode op_code = *pdwOp;

		if (op_code.op != OP_DBG_BKPT)
		{
			DBGConsole_Msg(0, "[YNo breakpoint is set at 0x%08x]", address);
			return;
		}

		// Entry is in lower 26 bits...
		u32 breakpoint_idx = op_code.bp_index;

		if (breakpoint_idx < g_BreakPoints.size())
		{
			g_BreakPoints[breakpoint_idx].mEnabled = enable;
			// Alwyas disable
			g_BreakPoints[breakpoint_idx].mTemporaryDisable = false;
		}
	}
}
#endif

extern u32 gVISyncRate;
extern "C"
{
void CPU_HANDLE_COUNT_INTERRUPT()
{
	DAEDALUS_ASSERT( gCPUState.NumEvents > 0, "Should always have at least one event queued up" );

	switch (CPU_PopEvent())
	{
	case CPU_EVENT_VBL:
		{
			//Todo: Work on VI_INTR_CYCLES should be 62500 * (60/Real game FPS)
			u32 vertical_sync_reg = Memory_VI_GetRegister( VI_V_SYNC_REG );
			if (vertical_sync_reg == 0)
			{
				VI_INTR_CYCLES = 62500;
			}
			else
			{
				VI_INTR_CYCLES = (vertical_sync_reg+1) * ( gVideoRateMatch ? gVISyncRate : 1500 );
			}

			// Apply cheatcodes, if enabled
			if( gCheatsEnabled )
			{
				CheatCodes_Activate( IN_GAME );
			}

			// Add another Interrupt at the next time:
			CPU_AddEvent(VI_INTR_CYCLES, CPU_EVENT_VBL);

			gVerticalInterrupts++;

			FramerateLimiter_Limit();
#ifdef DAEDALUS_W32
			if (gAudioPlugin != NULL)
				gAudioPlugin->Update(false);
#endif
			Memory_MI_SetRegisterBits(MI_INTR_REG, MI_INTR_VI);
			R4300_Interrupt_UpdateCause3();

			// ToDo: Has to be a better way than this???
			// Maybe After each X frames instead of each 60 VI?
			// (strmnnrmn): I don't see what's so bad about checking these on a vbl,
			//   because it means we can remain responsive even if the game is not rendering frames
			//   (e.g. if it's slow starting up)
			//   Alternatively, we could add a special-purpose CPU even that triggers every
			//   N cycles, but that would have a small impact on framerate (it would
			//   interrupt the dynamo tracer for instance)
			// TODO(strmnnrmn): should register this with CPU_RegisterVblCallback.
			if ((gVerticalInterrupts & 0x3F) == 0) // once every 60 VBLs
				Save_Flush();

			for (size_t i = 0; i < gVblCallbacks.size(); ++i)
			{
				VblCallback & callback = gVblCallbacks[i];
				callback.Fn(callback.Arg);
			}

			HandleSaveStateOperationOnVerticalBlank();
		}
		break;
	case CPU_EVENT_COMPARE:
		{
			gCPUState.CPUControl[C0_CAUSE]._u32 |= CAUSE_IP8;
			gCPUState.AddJob( CPU_CHECK_INTERRUPTS );
		}
		break;
	case CPU_EVENT_AUDIO:
		{
			u32 status = Memory_SP_SetRegisterBits(SP_STATUS_REG, SP_STATUS_TASKDONE|SP_STATUS_YIELDED|SP_STATUS_BROKE|SP_STATUS_HALT);
			if( status & SP_STATUS_INTR_BREAK )
				CPU_AddEvent(4000, CPU_EVENT_SPINT);
		}
		break;
	case CPU_EVENT_SPINT:
		Memory_MI_SetRegisterBits(MI_INTR_REG, MI_INTR_SP);
		R4300_Interrupt_UpdateCause3();
		break;
	default:
		NODEFAULT;
	}

	DAEDALUS_ASSERT( gCPUState.NumEvents > 0, "Should always have at least one event queued up" );
}
}

void CPU_SetCompare(u32 value)
{
	gCPUState.CPUControl[C0_CAUSE]._u32 &= ~CAUSE_IP8;

	DPF( DEBUG_REGS, "COMPARE set to 0x%08x.", value );
	//DBGConsole_Msg(0, "COMPARE set to 0x%08x Count is 0x%08x.", value, gCPUState.CPUControl[C0_COUNT]._u32);

	// Add an event for this compare:
	if (value == gCPUState.CPUControl[C0_COMPARE]._u32)
	{
		//DBGConsole_Msg(0, "Clear");
	}
	else
	{
		if (value != 0)
		{
			// NB, value can be less than COUNT here, which indicates that the counter is close to wrapping.
			// Don't do anything special to handle this - just treat delta as an unsigned value.
			u32 delta = value - gCPUState.CPUControl[C0_COUNT]._u32;

			// This fires a lot for Zelda OoT. It's benign.
			// If seems to keep setting a delta of 140624981 when the counter is close to wrapping.
			// if (value < gCPUState.CPUControl[C0_COUNT]._u32)
			// {
			// 	DBGConsole_Msg(0, "SetCompare wrapping: %d -> %d = %d", gCPUState.CPUControl[C0_COUNT]._u32, value, delta);
			// }
			CPU_SetCompareEvent( delta );
		}
		else
		{
			//DBGConsole_Msg(0, "[RIgnoring SetCompare 0] - is this right?");
		}

		gCPUState.CPUControl[C0_COMPARE]._u32 = value;
	}
}

#ifdef DAEDALUS_ENABLE_SYNCHRONISATION
u32	CPU_ProduceRegisterHash()
{
	u32	hash = 0;

	if ( DAED_SYNC_MASK & DAED_SYNC_REG_GPR )
	{
		hash = murmur2_hash( (u8*)&(gCPUState.CPU[0]), sizeof( gCPUState.CPU ), hash );
	}

	if ( DAED_SYNC_MASK & DAED_SYNC_REG_CCR0 )
	{
		hash = murmur2_hash( (u8*)&(gCPUState.CPUControl[0]), sizeof( gCPUState.CPUControl ), hash );
	}

	if ( DAED_SYNC_MASK & DAED_SYNC_REG_CPU1 )
	{
		hash = murmur2_hash( (u8*)&(gCPUState.FPU[0]), sizeof( gCPUState.FPU ), hash );
	}
	if ( DAED_SYNC_MASK & DAED_SYNC_REG_CCR1 )
	{
		hash = murmur2_hash( (u8*)&(gCPUState.FPUControl[0]), sizeof( gCPUState.FPUControl ), hash );
	}

	return hash;
}
#endif

#ifdef FRAGMENT_SIMULATE_EXECUTION
// Execute the specified opcode
void CPU_ExecuteOpRaw( u32 count, u32 address, OpCode op_code, CPU_Instruction p_instruction, bool * p_branch_taken )
{
	gCPUState.CurrentPC = address;

	SYNCH_POINT( DAED_SYNC_REG_PC, gCPUState.CurrentPC, "Program Counter doesn't match" );
	SYNCH_POINT( DAED_SYNC_REG_PC, count, "Count doesn't match" );

	p_instruction( op_code._u32 );

	SYNCH_POINT( DAED_SYNC_REGS, CPU_ProduceRegisterHash(), "Registers don't match" );

	*p_branch_taken = gCPUState.Delay == DO_DELAY;
}
#endif

extern "C"
{
void R4300_CALL_TYPE CPU_UpdateCounter( u32 ops_executed )
{
	DAEDALUS_ASSERT( ops_executed > 0, "Expecting at least one op" );
	//SYNCH_POINT( DAED_SYNC_FRAGMENT_PC, ops_executed, "Number of executed ops doesn't match" );

#ifdef DAEDALUS_PROFILE_EXECUTION
	gTotalInstructionsExecuted += ops_executed;
#endif

	const u32 cycles = ops_executed * COUNTER_INCREMENT_PER_OP;

	// Increment count register
	gCPUState.CPUControl[C0_COUNT]._u32 += cycles;

	if( CPU_ProcessEventCycles( cycles ) )
	{
		CPU_HANDLE_COUNT_INTERRUPT();
	}
}

#ifdef UPDATE_COUNTER_ON_EXCEPTION
// As above, but no interrupts are fired
void CPU_UpdateCounterNoInterrupt( u32 ops_executed )
{
	//SYNCH_POINT( DAED_SYNC_FRAGMENT_PC, ops_executed, "Number of executed ops doesn't match" );

	if( ops_executed > 0 )
	{
		const u32 cycles = ops_executed * COUNTER_INCREMENT_PER_OP;

#ifdef DAEDALUS_PROFILE_EXECUTION
		gTotalInstructionsExecuted += ops_executed;
#endif

		// Increment count register
		gCPUState.CPUControl[C0_COUNT]._u32 += cycles;

#ifdef DAEDALUS_ENABLE_ASSERTS
		bool	ready = CPU_ProcessEventCycles( cycles );
		use( ready );
		DAEDALUS_ASSERT(!ready, "Ignoring Count interrupt");	// Just a test - remove eventually (needs to handle this)
#endif
	}
}
#endif
}

// Return true if change the core
bool CPU_CheckStuffToDo()
{
	// We do this in a slightly different order to ensure that
	// any interrupts are taken care of before we execute an op
	u32	stuff_to_do = gCPUState.GetStuffToDo();
	if( stuff_to_do )
	{
		// Process Interrupts/Exceptions on a priority basis
		// Call most likely first!
		if( stuff_to_do & CPU_CHECK_INTERRUPTS )
		{
			R4300_Handle_Interrupt();
			gCPUState.ClearJob( CPU_CHECK_INTERRUPTS );
		}
		else if( stuff_to_do & CPU_CHECK_EXCEPTIONS )
		{
			R4300_Handle_Exception();
			gCPUState.ClearJob( CPU_CHECK_EXCEPTIONS );
		}
		else if( stuff_to_do & CPU_CHANGE_CORE )
		{
			gCPUState.ClearJob( CPU_CHANGE_CORE );
			return true;
		}
		else if( stuff_to_do & CPU_STOP_RUNNING )
		{
			gCPUState.ClearJob( CPU_STOP_RUNNING );
			gCPURunning = false;
			return true;
		}
		// Clear stuff_to_do?
	}

	return false;
}

// Return true if the CPU is running
bool CPU_IsRunning()
{
	return gCPURunning;
}

