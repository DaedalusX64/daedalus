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

#include "Graphics/GraphicsContext.h"
#include "../Graphics/intraFont/intraFont.h"

#ifdef DAEDALUS_ENABLE_OS_HOOKS

#include "patch_symbols.h"
#include "OS.h"
#include "OSMesgQueue.h"

#include "Core/Memory.h"
#include "Core/CPU.h"
#include "Core/R4300.h"
#include "Core/RSP.h"
#include "Core/Registers.h"
#include "Core/ROM.h"

#include "Utility/Profiler.h"
#include "Utility/CRC.h"

#include "Plugins/AudioPlugin.h"

#include "Debug/Dump.h"
#include "Debug/DebugLog.h"
#include "Debug/DBGConsole.h"

#include "Math/Math.h"	// VFPU Math

#include "DynaRec/Fragment.h"
#include "DynaRec/FragmentCache.h"

#include "ultra_os.h"
#include "ultra_rcp.h"
#include "ultra_sptask.h"
#include "ultra_R4300.h"

#include "ConfigOptions.h"

#include <stddef.h>		// offsetof

//#define DUMPOSFUNCTIONS

#ifdef DUMPOSFUNCTIONS
#include "Debug/dump.h"
#include "Utility/IO.h"

static const char * g_szEventStrings[23] =
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
#endif	// DUMPOSFUNCTIONS

u32 gCurrentLength;
u32 gNumOfOSFunctions;

#define TEST_DISABLE_FUNCS //return PATCH_RET_NOT_PROCESSED;


#define PATCH_RET_NOT_PROCESSED RET_NOT_PROCESSED(NULL)
#define PATCH_RET_NOT_PROCESSED0(name) RET_NOT_PROCESSED(&PATCH_SYMBOL_FUNCTION_ENTRY(name))
#define PATCH_RET_JR_RA RET_JR_RA()
#define PATCH_RET_ERET RET_JR_ERET()

// Increase this number every time we changed the symbol table
static const u32 MAGIC_HEADER = 0x80000121;

bool gPatchesInstalled = false;

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
u32  nPatchSymbols;
static u32  nPatchVariables;

void Patch_Reset()
{
	gPatchesInstalled = false;
	gCurrentLength = 0;
	gNumOfOSFunctions = 0;
	Patch_ResetSymbolTable();
}

void Patch_ResetSymbolTable()
{
	u32 i = 0;
	// Loops through all symbols, until name is null
	for (i = 0; g_PatchSymbols[i] != NULL; i++)
	{
		g_PatchSymbols[i]->bFound = false;
	}
	nPatchSymbols = i;

	for (i = 0; g_PatchVariables[i] != NULL; i++)
	{
		g_PatchVariables[i]->bFound = false;
		g_PatchVariables[i]->bFoundHi = false;
		g_PatchVariables[i]->bFoundLo = false;
	}
	nPatchVariables = i;
}

static bool applied = false;
void Patch_ApplyPatches()
{
	applied = true;

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

	// This may already be set if this function has been called before
	gPatchesInstalled = true;
}


void Patch_PatchAll()
{
	if (!applied)
		Patch_ApplyPatches();

#ifdef DUMPOSFUNCTIONS
	FILE *fp;
	char path[MAX_PATH + 1];
	Dump_GetDumpDirectory(path, "");
	IO::Path::Append(path, "n64.cfg");
	fp = fopen(path, "w");
#endif
	for (u32 i = 0; i < nPatchSymbols; i++)
	{
		if (g_PatchSymbols[i]->bFound)
		{
#ifdef DUMPOSFUNCTIONS
			char buf[MAX_PATH + 1];
			PatchSymbol * ps = g_PatchSymbols[i];
			Dump_GetDumpDirectory(buf, "oshle");
			IO::Path::Append(buf, ps->szName);

			Dump_Disassemble(PHYS_TO_K0(ps->location), PHYS_TO_K0(ps->location) + ps->pSignatures->nNumOps * sizeof(OpCode),
				buf);

			fprintf(fp, "%s 0x%08x\n", ps->szName, PHYS_TO_K0(ps->location));
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
	u32 pc = g_PatchSymbols[i]->location;

#ifdef DAEDALUS_ENABLE_DYNAREC
	CFragment *frag = new CFragment(gFragmentCache.GetCodeBufferManager(),
		PHYS_TO_K0(pc), g_PatchSymbols[i]->pSignatures->nNumOps,
									(void*)g_PatchSymbols[i]->pFunction
									);

	gFragmentCache.InsertFragment(frag);
#endif
}

// Return the location of a symbol
u32 Patch_GetSymbolAddress(const char * szName)
{
	// Search new list
	for (u32 p = 0; p < nPatchSymbols; p++)
	{
		// Skip symbol if already found, or if it is a variable
		if (!g_PatchSymbols[p]->bFound)
			continue;

		if (_strcmpi(g_PatchSymbols[p]->szName, szName) == 0)
			return PHYS_TO_K0(g_PatchSymbols[p]->location);

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
		if (!g_PatchSymbols[p]->bFound)
			continue;

		pdwPatchBase = g_pu32RamBase + (g_PatchSymbols[p]->location>>2);

		// Symbol not found, attempt to locate on this pass. This may
		// fail if all dependent symbols are not found
		if (pdwPatchBase == pdwOpBase)
		{
			return g_PatchSymbols[p]->szName;
		}

	}

	// The patch was not found
	return "?";
}

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
		char szFullQueue[30];
		char szEmptyQueue[30];
		char szType[60] = "";

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
			sprintf(szFullQueue, "       -");
		else
			sprintf(szFullQueue, "%08x", dwFullQ);

		if (dwEmptyQ == VAR_ADDRESS(osNullMsgQueue))
			sprintf(szEmptyQueue, "       -");
		else
			sprintf(szEmptyQueue, "%08x", dwEmptyQ);

		if (dwQueue == VAR_ADDRESS(osSiAccessQueue))
		{
			sprintf(szType, "<- Si Access");

		}
		else if (dwQueue == VAR_ADDRESS(osPiAccessQueue))
		{
			sprintf(szType, "<- Pi Access");
		}


		// Try and find in the event mesg array
		if (strlen(szType) == 0 && VAR_FOUND(osEventMesgArray))
		{
			for (u32 j = 0; j <	23; j++)
			{
				if (dwQueue == Read32Bits(VAR_ADDRESS(osEventMesgArray) + (j * 8) + 0x0))
				{
					sprintf(szType, "<- %s", g_szEventStrings[j]);
					break;
				}
			}
		}
		DBGConsole_Msg(0, "%08x, %s, %s, % 4d, % 4d, % 4d, %08x %s",
			dwQueue, szEmptyQueue, szFullQueue, dwValidCount, dwFirst, dwMsgCount, dwMsg, szType);
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
	//DBGConsole_Msg(0, "  xxxxxxxxxxxxxxxxxxxxxxxxxx 0x01234567 0x01234567",..);
	for (u32 i = 0; i <	23; i++)
	{
		dwQueue = Read32Bits(VAR_ADDRESS(osEventMesgArray) + (i * 8) + 0x0);
		dwMsg   = Read32Bits(VAR_ADDRESS(osEventMesgArray) + (i * 8) + 0x4);


		DBGConsole_Msg(0, "  %-26s 0x%08x 0x%08x",
			g_szEventStrings[i], dwQueue, dwMsg);
	}
}


#endif	// DUMPOSFUNCTIONS


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
	// Load our font here, Intrafont used in UI is destroyed when emulation starts
	intraFont* ltn8  = intraFontLoad( "flash0:/font/ltn8.pgf", INTRAFONT_CACHE_ASCII);
	intraFontSetStyle( ltn8, 1.0f, 0xFF000000, 0xFFFFFFFF, INTRAFONT_ALIGN_CENTER );
#endif

	// Loops through all symbols, until name is null
	for (u32 i = 0; i < nPatchSymbols && !gCPUState.IsJobSet( CPU_STOP_RUNNING ); i++)
	{

#ifdef DAEDALUS_DEBUG_CONSOLE
		CDebugConsole::Get()->MsgOverwrite(0, "OS HLE: %d / %d Looking for [G%s]",
			i, nPatchSymbols, g_PatchSymbols[i]->szName);
		fflush(stdout);
#else

		//Update patching progress on PSPscreen
		CGraphicsContext::Get()->BeginFrame();
		CGraphicsContext::Get()->Clear(true,true);
		//intraFontPrintf( ltn8, 480/2, (272>>1)-50, "Searching for os functions. This may take several seconds...");
		intraFontPrintf( ltn8, 480/2, (272>>1), "OS HLE Patching: %d%%", i * 100 / (nPatchSymbols-1));
		intraFontPrintf( ltn8, 480/2, (272>>1)-50, "Searching for %s", g_PatchSymbols[i]->szName );
		CGraphicsContext::Get()->EndFrame();
		CGraphicsContext::Get()->UpdateFrame( true );
#endif
		// Skip symbol if already found, or if it is a variable
		if (g_PatchSymbols[i]->bFound)
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
		if (!g_PatchSymbols[i]->bFound)
		{
			//DBGConsole_Msg(0, "[W%s] not found", g_PatchSymbols[i]->szName);
		}
		else
		{

			// Find duplicates! (to avoid showing the same clash twice, only scan up to the first symbol)
			bool found_duplicate( false );
			for (u32 j = 0; j < i; j++)
			{
				if (g_PatchSymbols[i]->bFound &&
					g_PatchSymbols[j]->bFound &&
					(g_PatchSymbols[i]->location ==
					g_PatchSymbols[j]->location))
				{
						DBGConsole_Msg(0, "Warning [C%s==%s]",
							g_PatchSymbols[i]->szName,
							g_PatchSymbols[j]->szName);

					// Don't patch!
					g_PatchSymbols[i]->bFound = false;
					g_PatchSymbols[j]->bFound = false;
					found_duplicate = true;
					break;
				}
			}
			// Hacks to disable certain os funcs in games that causes issues
			// This alot cheaper than adding a check on the func itself, this is only checked once -Salvy
			// Eventually we should fix them though 
			//
			// osSendMesg - Breaks the in game menu in Zelda OOT
			//
			if( ( g_ROM.GameHacks == ZELDA_OOT ) && ( strcmp("osSendMesg",g_PatchSymbols[i]->szName) == 0) )
			{
				DBGConsole_Msg(0, "Zelda OOT Hack : Disabling [R%s]",g_PatchSymbols[i]->szName);
				g_PatchSymbols[i]->bFound = false;
				break;
			}
			// osRestoreInt causes Ridge Racer to BSOD when quick race is about to start
			//
			else if( ( g_ROM.GameHacks == RIDGE_RACER ) && ( strcmp("__osRestoreInt",g_PatchSymbols[i]->szName) == 0) )
			{
				DBGConsole_Msg(0, "Ridge Racer Hack : Disabling [R%s]",g_PatchSymbols[i]->szName);
				g_PatchSymbols[i]->bFound = false;
				break;
			}

			if (!found_duplicate)
			{
				u32 location = g_PatchSymbols[i]->location;
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
		//Update patching progress on PSPscreen
		CGraphicsContext::Get()->BeginFrame();
		CGraphicsContext::Get()->Clear(true,true);
		intraFontPrintf( ltn8, 480/2, (272>>1), "Symbols Identified: %d%%",100 * nFound / (nPatchSymbols-1));
		intraFontPrintf( ltn8, 480/2, (272>>1)+50, "Range 0x%08x -> 0x%08x", first, last );
		CGraphicsContext::Get()->EndFrame();
		CGraphicsContext::Get()->UpdateFrame( true );
#endif
	}

	nFound = 0;
	for (u32 i = 0; i < nPatchVariables; i++)
	{
		if (!g_PatchVariables[i]->bFound)
		{
			//DBGConsole_Msg(0, "[W%s] not found", g_PatchVariables[i]->szName);
		}
		else
		{

			// Find duplicates! (to avoid showing the same clash twice, only scan up to the first symbol)
			for (u32 j = 0; j < i; j++)
			{
				if (g_PatchVariables[i]->bFound &&
					g_PatchVariables[j]->bFound &&
					(g_PatchVariables[i]->location ==
					g_PatchVariables[j]->location))
				{
						DBGConsole_Msg(0, "Warning [C%s==%s]",
							g_PatchVariables[i]->szName,
							g_PatchVariables[j]->szName);
				}
			}

			nFound++;
		}
#ifdef DAEDALUS_DEBUG_CONSOLE
		DBGConsole_Msg(0, "%d/%d variables identified", nFound, nPatchVariables);
#else

		//Update patching progress on PSPscreen
		CGraphicsContext::Get()->BeginFrame();
		CGraphicsContext::Get()->Clear(true,true);
		intraFontPrintf( ltn8, 480/2, 272>>1, "Variables Identified: %d%%", 100 * nFound / (nPatchVariables-1) );
		CGraphicsContext::Get()->EndFrame();
		CGraphicsContext::Get()->UpdateFrame( true );
#endif
	}

#ifndef DAEDALUS_DEBUG_CONSOLE
	// Unload font after we done patching progress 
	intraFontUnload( ltn8 );
#endif

}

// Attempt to locate this symbol.
bool Patch_LocateFunction(PatchSymbol * ps)
{
	OpCode op;
	const u32 * code_base( g_pu32RamBase );

	for (u32 s = 0; s < ps->pSignatures[s].nNumOps; s++)
	{
		PatchSignature * psig;
		psig = &ps->pSignatures[s];

		// Sweep through OS range
		for (u32 i = 0; i < (gRamSize>>2); i++)
		{
			op._u32 = code_base[i];
			op = GetCorrectOp( op );

			// First op must match!
			if ( psig->firstop != op.op )
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
	if (ps->bFound)
	{
		// The location must match!
		return (ps->location == (index<<2));
	}

	// Fail if index is outside of indexable memory
	if (index > gRamSize>>2)
		return false;


	for (u32 s = 0; s < ps->pSignatures[s].nNumOps; s++)
	{
		if (Patch_VerifyLocation_CheckSignature(ps, &ps->pSignatures[s], index))
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
	PatchCrossRef * pcr = psig->pCrossRefs;
	bool cross_ref_var_set( false );
	u32 crc;
	u32 partial_crc;

	if ( ( index + psig->nNumOps ) * 4 > gRamSize )
	{
		return false;
	}

	const u32 * code_base( g_pu32RamBase );

	PatchCrossRef dummy_cr = {~0, PX_JUMP, NULL };

	if (pcr == NULL)
		pcr = &dummy_cr;



	u32 last = pcr->offset;

	crc = 0;
	partial_crc = 0;
	for (u32 m = 0; m < psig->nNumOps; m++)
	{
		// Get the actual opcode at this address, not patched/compiled code
		op._u32 = code_base[index+m];
		op = GetCorrectOp( op );
		// This should be ok - so long as we patch all functions at once.

		// Check if a cross reference is in effect here
		if (pcr->offset == m)
		{
			// This is a cross reference.
			switch (pcr->nType)
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
					if (!Patch_VerifyLocation(pcr->pSymbol, TargetIndex))
						goto fail_find;

					op.target = 0;		// Mask out jump location
				}
				break;
			case PX_VARIABLE_HI:
				{
					// The data element should be consistant with the symbol
					if (pcr->pVariable->bFoundHi)
					{
						if (pcr->pVariable->wHiPart != (op._u32 & 0xFFFF))
							goto fail_find;

					}
					else
					{
						// Assume this is the correct symbol
						pcr->pVariable->bFoundHi = true;
						pcr->pVariable->wHiPart = (u16)(op._u32 & 0xFFFF);

						cross_ref_var_set = true;

						// If other half has been identified, set the location
						if (pcr->pVariable->bFoundLo)
							pcr->pVariable->location = (pcr->pVariable->wHiPart<<16) + (short)(pcr->pVariable->wLoPart);
					}

					op._u32 &= ~0x0000ffff;		// Mask out low halfword
				}
				break;
			case PX_VARIABLE_LO:
				{
					// The data element should be consistant with the symbol
					if (pcr->pVariable->bFoundLo)
					{
						if (pcr->pVariable->wLoPart != (op._u32 & 0xFFFF))
							goto fail_find;

					}
					else
					{
						// Assume this is the correct symbol
						pcr->pVariable->bFoundLo = true;
						pcr->pVariable->wLoPart = (u16)(op._u32 & 0xFFFF);

						cross_ref_var_set = true;

						// If other half has been identified, set the location
						if (pcr->pVariable->bFoundHi)
							pcr->pVariable->location = (pcr->pVariable->wHiPart<<16) + (short)(pcr->pVariable->wLoPart);
					}

					op._u32 &= ~0x0000ffff;		// Mask out low halfword
				}
				break;
			}

			// We've handled this cross ref - point to the next one
			// ready for the next match.
			pcr++;

			// If pcr->offset == ~0, then there are no more in the array
			// This is okay, as the comparison with m above will never match
#ifndef DAEDALUS_SILENT
			if (pcr->offset < last)
			{
				DBGConsole_Msg(0, "%s: CrossReference offsets out of order", ps->szName);
			}
#endif
			last = pcr->offset;

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
			if (partial_crc != psig->partial_crc)
			{
				goto fail_find;
			}
		}

		// Add to the total crc
		crc = daedalus_crc32(crc, (u8*)&op, 4);


	}

	// Check if the complete crc matches!
	if (crc != psig->crc)
	{
		goto fail_find;
	}


	// We have located the symbol
	ps->bFound = true;
	ps->location = index<<2;
	ps->pFunction = psig->pFunction;		// Install this function

	if (cross_ref_var_set)
	{
		// Loop through symbols, setting variables if both high/low found
		for (pcr = psig->pCrossRefs; pcr->offset != u32(~0); pcr++)
		{
			if (pcr->nType == PX_VARIABLE_HI ||
				pcr->nType == PX_VARIABLE_LO)
			{
				if (pcr->pVariable->bFoundLo && pcr->pVariable->bFoundHi)
				{
					pcr->pVariable->bFound = true;
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
		for (pcr = psig->pCrossRefs; pcr->offset != u32(~0); pcr++)
		{
			if (pcr->nType == PX_VARIABLE_HI ||
				pcr->nType == PX_VARIABLE_LO)
			{
				if (!pcr->pVariable->bFound)
				{
					pcr->pVariable->bFoundLo = false;
					pcr->pVariable->bFoundHi = false;
				}
			}
		}
	}

	return false;
}

static void Patch_FlushCache()
{
	char name[MAX_PATH + 1];

	Dump_GetSaveDirectory(name, g_ROM.szFileName, ".hle");
	DBGConsole_Msg(0, "Write OSHLE cache: %s", name);

	FILE *fp = fopen(name, "wb");

	if (fp != NULL)
	{
		u32 data = MAGIC_HEADER;
		fwrite(&data, 1, sizeof(data), fp);

		for (u32 i = 0; i < nPatchSymbols; i++)
		{
			if (g_PatchSymbols[i]->bFound )
			{
				data = g_PatchSymbols[i]->location;
				fwrite(&data, 1, sizeof(data), fp);
				for(data = 0; ;data++)
				{
					if (g_PatchSymbols[i]->pSignatures[data].pFunction ==
						g_PatchSymbols[i]->pFunction)
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
			if (g_PatchVariables[i]->bFound )
			{
				data = g_PatchVariables[i]->location;
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
	char name[MAX_PATH + 1];

	Dump_GetSaveDirectory(name, g_ROM.szFileName, ".hle");
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
				g_PatchSymbols[i]->bFound = true;
				g_PatchSymbols[i]->location = data;
				fread(&data, 1, sizeof(data), fp);
				g_PatchSymbols[i]->pFunction = g_PatchSymbols[i]->pSignatures[data].pFunction;
			}
			else
				g_PatchSymbols[i]->bFound = false;
		}

		for (u32 i = 0; i < nPatchVariables; i++)
		{
			fread(&data, 1, sizeof(data), fp);
			if (data != 0)
			{
				g_PatchVariables[i]->bFound = true;
				g_PatchVariables[i]->location = data;
			}
			else
			{
				g_PatchVariables[i]->bFound = false;
				g_PatchVariables[i]->location = 0;
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

	gCPUState.CurrentPC = PHYS_TO_K0(ps->location);
	//DBGConsole_Msg(0, "%s RET_NOT_PROCESSED PC=0x%08x RA=0x%08x", ps->szName, gCPUState.TargetPC, gGPR[REG_ra]._u32_0);

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

static u32 inline RET_JR_RA()
{
		gCPUState.TargetPC = gGPR[REG_ra]._u32_0;
		return 1;
}

static u32 RET_JR_ERET()
{
	if( gCPUState.CPUControl[C0_SR]._u64 & SR_ERL )
	{
		// Returning from an error trap
		CPU_SetPC( gCPUState.CPUControl[C0_ERROR_EPC]._u32_0 );
		gCPUState.CPUControl[C0_SR]._u64 &= ~SR_ERL;
	}
	else
	{
		// Returning from an exception
		CPU_SetPC( gCPUState.CPUControl[C0_EPC]._u32_0 );
		gCPUState.CPUControl[C0_SR]._u64 &= ~SR_EXL;
	}
	// Point to previous instruction (as we increment the pointer immediately afterwards
	DECREMENT_PC();

	// Ensure we don't execute this in the delay slot
	gCPUState.Delay = NO_DELAY;
	
	return 0;
}

static u32 ConvertToPhysics(u32 addr)
{
	if (IS_KSEG0(addr))
	{
		return K0_TO_PHYS(addr);
	}
	else if (IS_KSEG1(addr))
	{
		return  K1_TO_PHYS(addr);
	}
	else
	{
		return OS_HLE___osProbeTLB(addr);
	}
}

//////////////////////////////////////////////////////////////
// Quick Read/Write methods that require a base returned by
// ReadAddress or Memory_GetInternalReadAddress etc

inline u64 QuickRead64Bits( u8 *p_base, u32 offset )
{
	u64 data = *(u64 *)(p_base + offset);
	return (data>>32) + (data<<32);
}

inline u32 QuickRead32Bits( u8 *p_base, u32 offset )
{
	return *(u32 *)(p_base + offset);
}

inline void QuickWrite64Bits( u8 *p_base, u32 offset, u64 value )
{
	u64 data = (value>>32) + (value<<32);
	*(u64 *)(p_base + offset) = data;
}

inline void QuickWrite32Bits( u8 *p_base, u32 offset, u32 value )
{
	*(u32 *)(p_base + offset) = value;
}

typedef struct { u32 value[8]; } u256;
inline void QuickWrite512Bits( u8 *p_base, u8 *p_source )
{
	*(u256 *)(p_base     ) = *(u256 *)(p_source     );
	*(u256 *)(p_base + 32) = *(u256 *)(p_source + 32);
}

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
u32 Patch_osContInit()
{
TEST_DISABLE_FUNCS
	//s32		osContInit(OSMesgQueue * mq, u8 *, OSContStatus * cs);
#ifndef DAEDALUS_SILENT
	u32 mq       = gGPR[REG_a0]._u32_0;
	u32 attached = gGPR[REG_a1]._u32_0;
	u32 cs       = gGPR[REG_a2]._u32_0;

	DBGConsole_Msg(0, "osContInit(0x%08x, 0x%08x, 0x%08x), ra = 0x%08x",
		mq, attached, cs, gGPR[REG_ra]._u32_0);
#endif

	return PATCH_RET_NOT_PROCESSED;
}

u32 Patch___osContAddressCrc()
{
TEST_DISABLE_FUNCS
#ifndef DAEDALUS_SILENT
	u32 address = gGPR[REG_a0]._u32_0;
	DBGConsole_Msg(0, "__osContAddressCrc(0x%08x)", address);
#endif
	return PATCH_RET_NOT_PROCESSED;
}

u32 Patch___osPackRequestData()
{
	return PATCH_RET_NOT_PROCESSED;
}

u32 Patch_osSetIntMask()
{
	return PATCH_RET_NOT_PROCESSED;
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
