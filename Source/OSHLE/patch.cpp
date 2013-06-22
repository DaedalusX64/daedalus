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

#include "stdafx.h"
#include "patch.h"

#ifdef DAEDALUS_ENABLE_OS_HOOKS

#include <stddef.h>		// offsetof

#include "patch_symbols.h"
#include "OS.h"
#include "OSMesgQueue.h"

#include "Config/ConfigOptions.h"
#include "Core/CPU.h"
#include "Core/DMA.h"
#include "Core/Memory.h"
#include "Core/R4300.h"
#include "Core/Registers.h"
#include "Core/ROM.h"
#include "Debug/DBGConsole.h"
#include "Debug/DebugLog.h"
#include "Debug/Dump.h"
#include "DynaRec/Fragment.h"
#include "DynaRec/FragmentCache.h"
#include "Math/Math.h"	// VFPU Math
#include "OSHLE/ultra_os.h"
#include "OSHLE/ultra_R4300.h"
#include "OSHLE/ultra_rcp.h"
#include "OSHLE/ultra_sptask.h"
#include "Plugins/AudioPlugin.h"
#include "Utility/CRC.h"
#include "Utility/Endian.h"
#include "Utility/FastMemcpy.h"
#include "Utility/Profiler.h"

#ifdef DAEDALUS_PSP
#include "Graphics/GraphicsContext.h"
#include "SysPSP/Graphics/intraFont/intraFont.h"
#endif

#ifdef DUMPOSFUNCTIONS
#include "Debug/Dump.h"
#include "Utility/IO.h"

static const char * const gEventStrings[23] =
{
	"OS_EVENT_SW1",
	"OS_EVENT_SW2",
	"OS_EVENT_CART",
	"OS_EVENT_COUNTER",
	"OS_EVENT_SP",
	"OS_EVENT_SI",
	"OS_EVENT_AI",
	"OS_EVENT_VI",
	"OS_EVENT_PI",
	"OS_EVENT_DP",
	"OS_EVENT_CPU_BREAK",
	"OS_EVENT_SP_BREAK",
	"OS_EVENT_FAULT",
	"OS_EVENT_THREADSTATUS",
	"OS_EVENT_PRENMI",
	"OS_EVENT_RDB_READ_DONE",
	"OS_EVENT_RDB_LOG_DONE",
	"OS_EVENT_RDB_DATA_DONE",
	"OS_EVENT_RDB_REQ_RAMROM",
	"OS_EVENT_RDB_FREE_RAMROM",
	"OS_EVENT_RDB_DBG_DONE",
	"OS_EVENT_RDB_FLUSH_PROF",
	"OS_EVENT_RDB_ACK_PROF"
};
#endif // DUMPOSFUNCTIONS

u32 gNumOfOSFunctions;

#define TEST_DISABLE_FUNCS //return PATCH_RET_NOT_PROCESSED;


#define PATCH_RET_NOT_PROCESSED RET_NOT_PROCESSED(NULL)
#define PATCH_RET_NOT_PROCESSED0(name) RET_NOT_PROCESSED(&PATCH_SYMBOL_FUNCTION_ENTRY(name))
#define PATCH_RET_JR_RA RET_JR_RA()
#define PATCH_RET_ERET RET_JR_ERET()

// Increase this number every time we changed the symbol table
static const u32 MAGIC_HEADER = 0x80000146;

static bool gPatchesApplied = false;

//u32 g_dwOSStart = 0x00240000;
//u32 g_dwOSEnd   = 0x00380000;


void Patch_ResetSymbolTable();
void Patch_RecurseAndFind();
static bool Patch_LocateFunction(PatchSymbol * ps);
static bool Patch_VerifyLocation(PatchSymbol * ps, u32 index);
static bool Patch_VerifyLocation_CheckSignature(PatchSymbol * ps, PatchSignature * psig, u32 index);
static bool Patch_GetCache();
static void Patch_FlushCache();

static void Patch_ApplyPatch(u32 i);
static u32  nPatchSymbols;
static u32  nPatchVariables;

void Patch_Reset()
{
	gPatchesApplied = false;
	gNumOfOSFunctions = 0;
	Patch_ResetSymbolTable();
}

void Patch_ResetSymbolTable()
{
	u32 i = 0;
	// Loops through all symbols, until name is null
	for (i = 0; g_PatchSymbols[i] != NULL; i++)
	{
		g_PatchSymbols[i]->Found = false;
	}
	nPatchSymbols = i;

	for (i = 0; g_PatchVariables[i] != NULL; i++)
	{
		g_PatchVariables[i]->Found = false;
		g_PatchVariables[i]->FoundHi = false;
		g_PatchVariables[i]->FoundLo = false;
	}
	nPatchVariables = i;
}

void Patch_ApplyPatches()
{
	gPatchesApplied = true;

	if (!gOSHooksEnabled)
		return;

	if (!Patch_GetCache())
	{
		Patch_RecurseAndFind();

		// Tip : Disable this when working on oshle funcs, you save the time to delete hle cache everyttime you need to test :p
		Patch_FlushCache();
	}

	// Do this every time or just when originally patched
	/*result = */OS_Reset();
}


void Patch_PatchAll()
{
	gNumOfOSFunctions = 0;

	if (!gPatchesApplied)
	{
		Patch_ApplyPatches();
	}
#ifdef DUMPOSFUNCTIONS
	FILE *fp;
	IO::Filename path;
	Dump_GetDumpDirectory(path, "");
	IO::Path::Append(path, "n64.cfg");
	fp = fopen(path, "w");
#endif
	for (u32 i = 0; i < nPatchSymbols; i++)
	{
		if (g_PatchSymbols[i]->Found)
		{
#ifdef DUMPOSFUNCTIONS
			IO::Filename buf;
			PatchSymbol * ps = g_PatchSymbols[i];
			Dump_GetDumpDirectory(buf, "oshle");
			IO::Path::Append(buf, ps->Name);

			Dump_Disassemble(PHYS_TO_K0(ps->Location), PHYS_TO_K0(ps->Location) + ps->Signatures->NumOps * sizeof(OpCode),
				buf);

			fprintf(fp, "%s 0x%08x\n", ps->Name, PHYS_TO_K0(ps->Location));
#endif
			gNumOfOSFunctions++;
			Patch_ApplyPatch(i);
		}
	}
#ifdef DUMPOSFUNCTIONS
	fclose(fp);
#endif
}

void Patch_ApplyPatch(u32 i)
{
#ifdef DAEDALUS_ENABLE_DYNAREC
	u32 pc = g_PatchSymbols[i]->Location;

	CFragment *frag = new CFragment(gFragmentCache.GetCodeBufferManager(),
									PHYS_TO_K0(pc),
									g_PatchSymbols[i]->Signatures->NumOps,
									(void*)g_PatchSymbols[i]->Function);

	gFragmentCache.InsertFragment(frag);
#endif
}

#ifndef DAEDALUS_SILENT
// Return the location of a symbol
u32 Patch_GetSymbolAddress(const char * name)
{
	// Search new list
	for (u32 p = 0; p < nPatchSymbols; p++)
	{
		// Skip symbol if already found, or if it is a variable
		if (!g_PatchSymbols[p]->Found)
			continue;

		if (_strcmpi(g_PatchSymbols[p]->Name, name) == 0)
			return PHYS_TO_K0(g_PatchSymbols[p]->Location);

	}

	// The patch was not found
	return u32(~0);

}

// Given a location, this function returns the name of the matching
// symbol (if there is one)

const char * Patch_GetJumpAddressName(u32 jump)
{
	u32 * pdwOpBase;
	u32 * pdwPatchBase;

	if (!Memory_GetInternalReadAddress(jump, (void **)&pdwOpBase))
		return "??";

	// Search new list
	for (u32 p = 0; p < nPatchSymbols; p++)
	{
		// Skip symbol if already found, or if it is a variable
		if (!g_PatchSymbols[p]->Found)
			continue;

		pdwPatchBase = g_pu32RamBase + (g_PatchSymbols[p]->Location>>2);

		// Symbol not found, attempt to locate on this pass. This may
		// fail if all dependent symbols are not found
		if (pdwPatchBase == pdwOpBase)
		{
			return g_PatchSymbols[p]->Name;
		}

	}

	// The patch was not found
	return "?";
}
#endif //DAEDALUS_SILENT

#ifdef DUMPOSFUNCTIONS

void Patch_DumpOsThreadInfo()
{
	u32 dwThread;
	u32 dwCurrentThread;

	u32 dwPri;
	u32 dwQueue;
	u16 wState;
	u16 wFlags;
	u32 dwID;
	u32 dwFP;

	u32 dwFirstThread;

	dwCurrentThread = Read32Bits(VAR_ADDRESS(osActiveThread));

	dwFirstThread = Read32Bits(VAR_ADDRESS(osGlobalThreadList));

	dwThread = dwFirstThread;
	DBGConsole_Msg(0, "");
	  DBGConsole_Msg(0, "Threads:      Pri   Queue       State   Flags   ID          FP Used");
	//DBGConsole_Msg(0, "  0x01234567, xxxx, 0x01234567, 0x0123, 0x0123, 0x01234567, 0x01234567",
	while (dwThread)
	{
		dwPri = Read32Bits(dwThread + offsetof(OSThread, priority));
		dwQueue = Read32Bits(dwThread + offsetof(OSThread, queue));
		wState = Read16Bits(dwThread + offsetof(OSThread, state));
		wFlags = Read16Bits(dwThread + offsetof(OSThread, flags));
		dwID = Read32Bits(dwThread + offsetof(OSThread, id));
		dwFP = Read32Bits(dwThread + offsetof(OSThread, fp));

		// Hack to avoid null thread
		if (dwPri == 0xFFFFFFFF)
			break;

		if (dwThread == dwCurrentThread)
		{
			DBGConsole_Msg(0, "->0x%08x, % 4d, 0x%08x, 0x%04x, 0x%04x, 0x%08x, 0x%08x",
				dwThread, dwPri, dwQueue, wState, wFlags, dwID, dwFP);
		}
		else
		{
			DBGConsole_Msg(0, "  0x%08x, % 4d, 0x%08x, 0x%04x, 0x%04x, 0x%08x, 0x%08x",
				dwThread, dwPri, dwQueue, wState, wFlags, dwID, dwFP);
		}
		dwThread = Read32Bits(dwThread + offsetof(OSThread, tlnext));

		if (dwThread == dwFirstThread)
			break;
	}
}

void Patch_DumpOsQueueInfo()
{
#ifdef DAED_OS_MESSAGE_QUEUES
	u32 dwQueue;


	// List queue info:
	u32 i;
	u32 dwEmptyQ;
	u32 dwFullQ;
	u32 dwValidCount;
	u32 dwFirst;
	u32 dwMsgCount;
	u32 dwMsg;


	DBGConsole_Msg(0, "There are %d Queues", g_MessageQueues.size());
	  DBGConsole_Msg(0, "Queues:   Empty     Full      Valid First MsgCount Msg");
	//DBGConsole_Msg(0, "01234567, 01234567, 01234567, xxxx, xxxx, xxxx, 01234567",
	for (i = 0; i <	g_MessageQueues.size(); i++)
	{
		char fullqueue_buffer[30];
		char emptyqueue_buffer[30];
		char type_buffer[60] = "";

		dwQueue = g_MessageQueues[i];

		COSMesgQueue q(dwQueue);

		dwEmptyQ	 = q.GetEmptyQueue();
		dwFullQ		 = q.GetFullQueue();
		dwValidCount = q.GetValidCount();
		dwFirst      = q.GetFirst();
		dwMsgCount   = q.GetMsgCount();
		dwMsg        = q.GetMesgArray();

		if ((s32)dwFirst < 0 ||
			(s32)dwValidCount < 0 ||
			(s32)dwMsgCount < 0)
		{
			continue;
		}

		if (dwFullQ == VAR_ADDRESS(osNullMsgQueue))
			sprintf(fullqueue_buffer, "       -");
		else
			sprintf(fullqueue_buffer, "%08x", dwFullQ);

		if (dwEmptyQ == VAR_ADDRESS(osNullMsgQueue))
			sprintf(emptyqueue_buffer, "       -");
		else
			sprintf(emptyqueue_buffer, "%08x", dwEmptyQ);

		if (dwQueue == VAR_ADDRESS(osSiAccessQueue))
		{
			sprintf(type_buffer, "<- Si Access");

		}
		else if (dwQueue == VAR_ADDRESS(osPiAccessQueue))
		{
			sprintf(type_buffer, "<- Pi Access");
		}


		// Try and find in the event mesg array
		if (strlen(type_buffer) == 0 && VAR_FOUND(osEventMesgArray))
		{
			for (u32 j = 0; j <	23; j++)
			{
				if (dwQueue == Read32Bits(VAR_ADDRESS(osEventMesgArray) + (j * 8) + 0x0))
				{
					sprintf(type_buffer, "<- %s", gEventStrings[j]);
					break;
				}
			}
		}
		DBGConsole_Msg(0, "%08x, %s, %s, % 4d, % 4d, % 4d, %08x %s",
			dwQueue, emptyqueue_buffer, fullqueue_buffer, dwValidCount, dwFirst, dwMsgCount, dwMsg, type_buffer);
	}
#endif
}


void Patch_DumpOsEventInfo()
{
	u32 dwQueue;
	u32 dwMsg;

	if (!VAR_FOUND(osEventMesgArray))
	{
		DBGConsole_Msg(0, "osSetEventMesg not patched, event table unknown");
		return;
	}

	DBGConsole_Msg(0, "");
	DBGConsole_Msg(0, "Events:                      Queue      Message");
	for (u32 i = 0; i <	23; i++)
	{
		dwQueue = Read32Bits(VAR_ADDRESS(osEventMesgArray) + (i * 8) + 0x0);
		dwMsg   = Read32Bits(VAR_ADDRESS(osEventMesgArray) + (i * 8) + 0x4);

		DBGConsole_Msg(0, "  %-26s 0x%08x 0x%08x",
			gEventStrings[i], dwQueue, dwMsg);
	}
}


#endif // DUMPOSFUNCTIONS

bool Patch_Hacks( PatchSymbol * ps )
{
	bool	Found( false );

	// Hacks to disable certain os funcs in games that causes issues
	// This alot cheaper than adding a check on the func itself, this is only checked once -Salvy
	// Eventually we should fix them though
	//
	// osSendMesg - Breaks the in game menu in Zelda OOT
	// osSendMesg - Causes Animal Corssing to freeze after the N64 logo
	// osSendMesg - Causes Clay Fighter 63 1-3 to not boot
	//
	switch( g_ROM.GameHacks )
	{
	case ZELDA_OOT:
	case ANIMAL_CROSSING:
	case CLAY_FIGHTER_63:

		if( strcmp("osSendMesg", ps->Name) == 0)
		{
			Found = true;
			break;
		}
		break;

	//
	// __osDispatchThread and __osEnqueueAndYield causes Body Harvest to not boot
	// This game is very sensitive with IRQs, see DMA.cpp (DMA_SI_CopyToDRAM)
	case BODY_HARVEST:
		if( strcmp("__osDispatchThread", ps->Name) == 0)
		{
			Found = true;
			break;

		}
		if( strcmp("__osEnqueueAndYield", ps->Name) == 0)
		{
			Found = true;
			break;

		}
		break;
	default:
		break;
	}

	return Found;
}


//ToDo: Add Status bar for loading OSHLE Patch Symbols.
void Patch_RecurseAndFind()
{
	s32 nFound;
	u32 first;
	u32 last;

	DBGConsole_Msg(0, "Searching for os functions. This may take several seconds...");

	// Keep looping until a pass does not resolve any more symbols
	nFound = 0;

#ifdef DAEDALUS_DEBUG_CONSOLE
	CDebugConsole::Get()->MsgOverwriteStart();
#else
#ifdef DAEDALUS_PSP
	// Load our font here, Intrafont used in UI is destroyed when emulation starts
	intraFont* ltn8  = intraFontLoad( "flash0:/font/ltn8.pgf", INTRAFONT_CACHE_ASCII);
	intraFontSetStyle( ltn8, 1.0f, 0xFF000000, 0xFFFFFFFF, INTRAFONT_ALIGN_CENTER );
#endif
#endif

	// Loops through all symbols, until name is null
	for (u32 i = 0; i < nPatchSymbols && !gCPUState.IsJobSet( CPU_STOP_RUNNING ); i++)
	{

#ifdef DAEDALUS_DEBUG_CONSOLE
		CDebugConsole::Get()->MsgOverwrite(0, "OS HLE: %d / %d Looking for [G%s]",
			i, nPatchSymbols, g_PatchSymbols[i]->Name);
		fflush(stdout);
#else
#ifdef DAEDALUS_PSP
		//Update patching progress on PSPscreen
		CGraphicsContext::Get()->BeginFrame();
		CGraphicsContext::Get()->ClearToBlack();
		//intraFontPrintf( ltn8, 480/2, (272>>1)-50, "Searching for os functions. This may take several seconds...");
		intraFontPrintf( ltn8, 480/2, (272>>1), "OS HLE Patching: %d%%", i * 100 / (nPatchSymbols-1));
		intraFontPrintf( ltn8, 480/2, (272>>1)-50, "Searching for %s", g_PatchSymbols[i]->Name );
		CGraphicsContext::Get()->EndFrame();
		CGraphicsContext::Get()->UpdateFrame( true );
#endif
#endif //DAEDALUS_DEBUG_CONSOLE
		// Skip symbol if already found, or if it is a variable
		if (g_PatchSymbols[i]->Found)
			continue;

		// Symbol not found, attempt to locate on this pass. This may
		// fail if all dependent symbols are not found
		if (Patch_LocateFunction(g_PatchSymbols[i]))
			nFound++;
	}

	if ( gCPUState.IsJobSet( CPU_STOP_RUNNING ) )
	{
#ifdef DAEDALUS_DEBUG_CONSOLE
		CDebugConsole::Get()->MsgOverwrite( 0, "OS HLE: Aborted" );
		CDebugConsole::Get()->MsgOverwriteEnd();
#endif

		return;
	}
#ifdef DAEDALUS_DEBUG_CONSOLE
	CDebugConsole::Get()->MsgOverwrite(0, "OS HLE: %d / %d All done",
		nPatchSymbols, nPatchSymbols);

	CDebugConsole::Get()->MsgOverwriteEnd();
#endif

	first = u32(~0);
	last = 0;

	nFound = 0;
	for (u32 i = 0; i < nPatchSymbols; i++)
	{
		if (!g_PatchSymbols[i]->Found)
		{
			//DBGConsole_Msg(0, "[W%s] not found", g_PatchSymbols[i]->Name);
		}
		else
		{

			// Find duplicates! (to avoid showing the same clash twice, only scan up to the first symbol)
			bool found_duplicate( false );
			for (u32 j = 0; j < i; j++)
			{
				if (g_PatchSymbols[i]->Found &&
					g_PatchSymbols[j]->Found &&
					(g_PatchSymbols[i]->Location ==
					 g_PatchSymbols[j]->Location))
				{
						DBGConsole_Msg(0, "Warning [C%s==%s]",
							g_PatchSymbols[i]->Name,
							g_PatchSymbols[j]->Name);

					// Don't patch!
					g_PatchSymbols[i]->Found = false;
					g_PatchSymbols[j]->Found = false;
					found_duplicate = true;
					break;
				}
			}
			// Disable certain os funcs where it causes issues in some games ex Zelda
			//
			if( Patch_Hacks(g_PatchSymbols[i]) )
			{
				DBGConsole_Msg(0, "[ROS Hack : Disabling %s]", g_PatchSymbols[i]->Name);
				g_PatchSymbols[i]->Found = false;
			}

			if (!found_duplicate)
			{
				u32 location = g_PatchSymbols[i]->Location;
				if (location < first) first = location;
				if (location > last)  last = location;

				// Actually patch:
				Patch_ApplyPatch(i);
				nFound++;
			}
		}
#ifdef DAEDALUS_DEBUG_CONSOLE
		DBGConsole_Msg(0, "%d/%d symbols identified, in range 0x%08x -> 0x%08x",
		nFound, nPatchSymbols, first, last);
#else
#ifdef DAEDALUS_PSP
		//Update patching progress on PSPscreen
		CGraphicsContext::Get()->BeginFrame();
		CGraphicsContext::Get()->ClearToBlack();
		intraFontPrintf( ltn8, 480/2, (272>>1), "Symbols Identified: %d%%", 100 * nFound / (nPatchSymbols-1));
		intraFontPrintf( ltn8, 480/2, (272>>1)+50, "Range 0x%08x -> 0x%08x", first, last );
		CGraphicsContext::Get()->EndFrame();
		CGraphicsContext::Get()->UpdateFrame( true );
#endif
#endif
	}

	nFound = 0;
	for (u32 i = 0; i < nPatchVariables; i++)
	{
		if (!g_PatchVariables[i]->Found)
		{
			//DBGConsole_Msg(0, "[W%s] not found", g_PatchVariables[i]->Name);
		}
		else
		{

			// Find duplicates! (to avoid showing the same clash twice, only scan up to the first symbol)
			for (u32 j = 0; j < i; j++)
			{
				if (g_PatchVariables[i]->Found &&
					g_PatchVariables[j]->Found &&
					(g_PatchVariables[i]->Location ==
					 g_PatchVariables[j]->Location))
				{
						DBGConsole_Msg(0, "Warning [C%s==%s]",
							g_PatchVariables[i]->Name,
							g_PatchVariables[j]->Name);
				}
			}

			nFound++;
		}
#ifdef DAEDALUS_DEBUG_CONSOLE
		DBGConsole_Msg(0, "%d/%d variables identified", nFound, nPatchVariables);
#else
#ifdef DAEDALUS_PSP
		//Update patching progress on PSPscreen
		CGraphicsContext::Get()->BeginFrame();
		CGraphicsContext::Get()->ClearToBlack();
		intraFontPrintf( ltn8, 480/2, 272>>1, "Variables Identified: %d%%", 100 * nFound / (nPatchVariables-1) );
		CGraphicsContext::Get()->EndFrame();
		CGraphicsContext::Get()->UpdateFrame( true );
#endif
#endif
	}

#ifndef DAEDALUS_DEBUG_CONSOLE
#ifdef DAEDALUS_PSP
	// Unload font after we done patching progress
	intraFontUnload( ltn8 );
#endif
#endif

}

// Attempt to locate this symbol.
bool Patch_LocateFunction(PatchSymbol * ps)
{
	OpCode op;
	const u32 * code_base( g_pu32RamBase );

	for (u32 s = 0; s < ps->Signatures[s].NumOps; s++)
	{
		PatchSignature * psig;
		psig = &ps->Signatures[s];

		// Sweep through OS range
		for (u32 i = 0; i < (gRamSize>>2); i++)
		{
			op._u32 = code_base[i];
			op = GetCorrectOp( op );

			// First op must match!
			if ( psig->FirstOp != op.op )
				continue;

			// See if function i exists at this location
			if (Patch_VerifyLocation_CheckSignature(ps, psig, i))
			{
				return true;
			}

		}
	}

	return false;
}


#define JumpTarget(op, addr) (((addr) & 0xF0000000) | ( op.target << 2 ))


// Check that the function i is located at address index
bool Patch_VerifyLocation(PatchSymbol * ps, u32 index)
{
	// We may have already located this symbol.
	if (ps->Found)
	{
		// The location must match!
		return (ps->Location == (index<<2));
	}

	// Fail if index is outside of indexable memory
	if (index > gRamSize>>2)
		return false;


	for (u32 s = 0; s < ps->Signatures[s].NumOps; s++)
	{
		if (Patch_VerifyLocation_CheckSignature(ps, &ps->Signatures[s], index))
		{
			return true;
		}
	}

	// Not found!
	return false;
}


bool Patch_VerifyLocation_CheckSignature(PatchSymbol * ps,
										 PatchSignature * psig,
										 u32 index)
{
	OpCode op;
	PatchCrossRef * pcr = psig->CrossRefs;
	bool cross_ref_var_set( false );
	u32 crc;
	u32 partial_crc;

	if ( ( index + psig->NumOps ) * 4 > gRamSize )
	{
		return false;
	}

	const u32 * code_base( g_pu32RamBase );

	PatchCrossRef dummy_cr = {~0, PX_JUMP, NULL };

	if (pcr == NULL)
		pcr = &dummy_cr;

#ifdef DAEDALUS_DEBUG_CONSOLE
	u32 last = pcr->Offset;
#endif
	crc = 0;
	partial_crc = 0;
	for (u32 m = 0; m < psig->NumOps; m++)
	{
		// Get the actual opcode at this address, not patched/compiled code
		op._u32 = code_base[index+m];
		op = GetCorrectOp( op );
		// This should be ok - so long as we patch all functions at once.

		// Check if a cross reference is in effect here
		if (pcr->Offset == m)
		{
			// This is a cross reference.
			switch (pcr->Type)
			{
			case PX_JUMP:
				{
					u32 TargetIndex = JumpTarget( op, (index+m)<<2 )>>2;

					// If the opcode at this address is not a Jump/Jal then
					// this can't match
					if ( op.op != OP_JAL && op.op != OP_J )
						goto fail_find;

					// This is a jump, the jump target must match the
					// symbol pointed to by this function. Recurse
					if (!Patch_VerifyLocation(pcr->Symbol, TargetIndex))
						goto fail_find;

					op.target = 0;		// Mask out jump location
				}
				break;
			case PX_VARIABLE_HI:
				{
					// The data element should be consistant with the symbol
					if (pcr->Variable->FoundHi)
					{
						if (pcr->Variable->HiWord != (op._u32 & 0xFFFF))
							goto fail_find;
					}
					else
					{
						// Assume this is the correct symbol
						pcr->Variable->FoundHi = true;
						pcr->Variable->HiWord = (u16)(op._u32 & 0xFFFF);

						cross_ref_var_set = true;

						// If other half has been identified, set the location
						if (pcr->Variable->FoundLo)
							pcr->Variable->Location = (pcr->Variable->HiWord<<16) + (short)(pcr->Variable->LoWord);
					}

					op._u32 &= ~0x0000ffff;		// Mask out low halfword
				}
				break;
			case PX_VARIABLE_LO:
				{
					// The data element should be consistant with the symbol
					if (pcr->Variable->FoundLo)
					{
						if (pcr->Variable->LoWord != (op._u32 & 0xFFFF))
							goto fail_find;

					}
					else
					{
						// Assume this is the correct symbol
						pcr->Variable->FoundLo = true;
						pcr->Variable->LoWord = (u16)(op._u32 & 0xFFFF);

						cross_ref_var_set = true;

						// If other half has been identified, set the location
						if (pcr->Variable->FoundHi)
							pcr->Variable->Location = (pcr->Variable->HiWord<<16) + (short)(pcr->Variable->LoWord);
					}

					op._u32 &= ~0x0000ffff;		// Mask out low halfword
				}
				break;
			}

			// We've handled this cross ref - point to the next one
			// ready for the next match.
			pcr++;

			// If pcr->Offset == ~0, then there are no more in the array
			// This is okay, as the comparison with m above will never match
#ifdef DAEDALUS_DEBUG_CONSOLE
			if (pcr->Offset < last)
			{
				DBGConsole_Msg(0, "%s: CrossReference offsets out of order", ps->Name);
			}

			last = pcr->Offset;
#endif
		}
		else
		{
			if ( op.op  == OP_J )
			{
				op.target = 0;		// Mask out jump location
			}
		}

		// If this is currently less than 4 ops in, add to the partial crc
		if (m < PATCH_PARTIAL_CRC_LEN)
			partial_crc = daedalus_crc32(partial_crc, (u8*)&op, 4);

		// Here, check the partial crc if m == 3
		if (m == (PATCH_PARTIAL_CRC_LEN-1))
		{
			if (partial_crc != psig->PartialCRC)
			{
				goto fail_find;
			}
		}

		// Add to the total crc
		crc = daedalus_crc32(crc, (u8*)&op, 4);
	}

	// Check if the complete crc matches!
	if (crc != psig->CRC)
	{
		goto fail_find;
	}

	// We have located the symbol
	ps->Found = true;
	ps->Location = index<<2;
	ps->Function = psig->Function;		// Install this function

	if (cross_ref_var_set)
	{
		// Loop through symbols, setting variables if both high/low found
		for (pcr = psig->CrossRefs; pcr->Offset != u32(~0); pcr++)
		{
			if (pcr->Type == PX_VARIABLE_HI ||
				pcr->Type == PX_VARIABLE_LO)
			{
				if (pcr->Variable->FoundLo && pcr->Variable->FoundHi)
				{
					pcr->Variable->Found = true;
				}
			}
		}
	}

	return true;

// Goto - ugh
fail_find:

	// Loop through symbols, clearing variables if they have been set
	if (cross_ref_var_set)
	{
		for (pcr = psig->CrossRefs; pcr->Offset != u32(~0); pcr++)
		{
			if (pcr->Type == PX_VARIABLE_HI ||
				pcr->Type == PX_VARIABLE_LO)
			{
				if (!pcr->Variable->Found)
				{
					pcr->Variable->FoundLo = false;
					pcr->Variable->FoundHi = false;
				}
			}
		}
	}

	return false;
}

static void Patch_FlushCache()
{
	IO::Filename name;

	Dump_GetSaveDirectory(name, g_ROM.mFileName, ".hle");
	DBGConsole_Msg(0, "Write OSHLE cache: %s", name);

	FILE *fp = fopen(name, "wb");

	if (fp != NULL)
	{
		u32 data = MAGIC_HEADER;
		fwrite(&data, 1, sizeof(data), fp);

		for (u32 i = 0; i < nPatchSymbols; i++)
		{
			if (g_PatchSymbols[i]->Found )
			{
				data = g_PatchSymbols[i]->Location;
				fwrite(&data, 1, sizeof(data), fp);
				for(data = 0; ;data++)
				{
					if (g_PatchSymbols[i]->Signatures[data].Function ==
						g_PatchSymbols[i]->Function)
						break;
				}
				fwrite(&data, 1, sizeof(data), fp);
			}
			else
			{
				data = 0;
				fwrite(&data, 1, sizeof(data), fp);
			}


		}

		for (u32 i = 0; i < nPatchVariables; i++)
		{
			if (g_PatchVariables[i]->Found )
			{
				data = g_PatchVariables[i]->Location;
			}
			else
			{
				data = 0;
			}

			fwrite(&data, 1, sizeof(data), fp);
		}

		fclose(fp);
	}
}


static bool Patch_GetCache()
{
	IO::Filename name;

	Dump_GetSaveDirectory(name, g_ROM.mFileName, ".hle");
	FILE *fp = fopen(name, "rb");

	if (fp != NULL)
	{
		DBGConsole_Msg(0, "Read from OSHLE cache: %s", name);
		u32 data;

		fread(&data, 1, sizeof(data), fp);
		if (data != MAGIC_HEADER)
		{
			fclose(fp);
			return false;
		}

		for (u32 i = 0; i < nPatchSymbols; i++)
		{
			fread(&data, 1, sizeof(data), fp);
			if (data != 0)
			{
				g_PatchSymbols[i]->Found = true;
				g_PatchSymbols[i]->Location = data;
				fread(&data, 1, sizeof(data), fp);
				g_PatchSymbols[i]->Function = g_PatchSymbols[i]->Signatures[data].Function;
			}
			else
				g_PatchSymbols[i]->Found = false;
		}

		for (u32 i = 0; i < nPatchVariables; i++)
		{
			fread(&data, 1, sizeof(data), fp);
			if (data != 0)
			{
				g_PatchVariables[i]->Found = true;
				g_PatchVariables[i]->Location = data;
			}
			else
			{
				g_PatchVariables[i]->Found = false;
				g_PatchVariables[i]->Location = 0;
			}
		}

		fclose(fp);
		return true;
	}

	return false;
}

static u32 RET_NOT_PROCESSED(PatchSymbol* ps)
{
	DAEDALUS_ASSERT( ps != NULL, "Not Supported" );

	gCPUState.CurrentPC = PHYS_TO_K0(ps->Location);
	//DBGConsole_Msg(0, "%s RET_NOT_PROCESSED PC=0x%08x RA=0x%08x", ps->Name, gCPUState.TargetPC, gGPR[REG_ra]._u32_0);

	gCPUState.Delay = NO_DELAY;
	gCPUState.TargetPC = gCPUState.CurrentPC;

	// Simulate the first op then return to dynarec. so we still can leverage dynarec.
	OpCode op_code;
	op_code._u32 = Read32Bits(gCPUState.CurrentPC);
	R4300_ExecuteInstruction(op_code);
	DAEDALUS_ASSERT(gCPUState.Delay == NO_DELAY, "OS functions' first op is a JUMP??");
	INCREMENT_PC();
	gCPUState.TargetPC = gCPUState.CurrentPC;

	return 1;
}

inline u32 RET_JR_RA()
{
		gCPUState.TargetPC = gGPR[REG_ra]._u32_0;
		return 1;
}

static u32 RET_JR_ERET()
{
	if( gCPUState.CPUControl[C0_SR]._u32 & SR_ERL )
	{
		// Returning from an error trap
		CPU_SetPC( gCPUState.CPUControl[C0_ERROR_EPC]._u32 );
		gCPUState.CPUControl[C0_SR]._u32 &= ~SR_ERL;
	}
	else
	{
		// Returning from an exception
		CPU_SetPC( gCPUState.CPUControl[C0_EPC]._u32 );
		gCPUState.CPUControl[C0_SR]._u32 &= ~SR_EXL;
	}
	// Point to previous instruction (as we increment the pointer immediately afterwards
	DECREMENT_PC();

	// Ensure we don't execute this in the delay slot
	gCPUState.Delay = NO_DELAY;

	return 0;
}

static u32 ConvertToPhysics(u32 addr)
{
	if( IS_SEG_A000_8000(addr) )
	{
		return SEG_TO_PHYS(addr);
	}
	else
	{
		return OS_HLE___osProbeTLB(addr);
	}
}

extern void MemoryUpdateMI( u32 value );
extern void MemoryUpdateSPStatus( u32 flags );

#include "patch_thread_hle.inl"
#include "patch_cache_hle.inl"
#include "patch_ai_hle.inl"
#include "patch_eeprom_hle.inl"
#include "patch_gu_hle.inl"
#include "patch_math_hle.inl"
#include "patch_mesg_hle.inl"
#include "patch_pi_hle.inl"
#include "patch_regs_hle.inl"
#include "patch_si_hle.inl"
#include "patch_sp_hle.inl"
#include "patch_timer_hle.inl"
#include "patch_tlb_hle.inl"
#include "patch_util_hle.inl"
#include "patch_vi_hle.inl"

/////////////////////////////////////////////////////

u32 Patch___osContAddressCrc()
{
TEST_DISABLE_FUNCS
	DBGConsole_Msg(0, "__osContAddressCrc(0x%08x)", gGPR[REG_a0]._u32_0);
	return PATCH_RET_NOT_PROCESSED;
}

u32 Patch___osPackRequestData()
{
	return PATCH_RET_NOT_PROCESSED;
}

// Perphaps not important for emulation? It works fine when we NOP this
u32 Patch_osSetIntMask()
{
/*
//
// Interrupt masks
//
#define	OS_IM_NONE		0x00000001
#define	OS_IM_SW1		0x00000501
#define	OS_IM_SW2		0x00000601
#define	OS_IM_CART		0x00000c01
#define	OS_IM_PRENMI	0x00001401
#define OS_IM_RDBWRITE	0x00002401
#define OS_IM_RDBREAD	0x00004401
#define	OS_IM_COUNTER	0x00008401
#define	OS_IM_CPU		0x0000ff01
#define	OS_IM_SP		0x00010401
#define	OS_IM_SI		0x00020401
#define	OS_IM_AI		0x00040401
#define	OS_IM_VI		0x00080401
#define	OS_IM_PI		0x00100401
#define	OS_IM_DP		0x00200401
#define	OS_IM_ALL		0x003fff01

#define	RCP_IMASK		0x003f0000
*/

	//u32 flag   = gGPR[REG_a0]._u32_0;
	//u32 thread = Read32Bits(VAR_ADDRESS(osActiveThread));
	//printf("%08x\n", flag );

	// The interrupt mask is part of the thread context rcp
	//Write32Bits(thread + offsetof(OSThread, context.rcp), flag);

	// Do nothing for now until I can find a test case that won't work ;)
	return PATCH_RET_JR_RA;
}
u32 Patch___osEepromRead_Prepare()
{
	return PATCH_RET_NOT_PROCESSED;
}
u32 Patch___osEepromWrite_Prepare()
{
	return PATCH_RET_NOT_PROCESSED;
}

#include "patch_symbols.inl"

#endif
