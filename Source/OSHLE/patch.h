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

#ifndef __PATCH_H__
#define __PATCH_H__

#include "Core/Registers.h"		// For SRHACK_???? etc
#include "Core/CPU.h"		// Breakpoint stuff
#include "ultra_R4300.h"

//#define DUMPOSFUNCTIONS

// Returns return type to determine what kind of post-processing should occur
typedef u32 (*PatchFunction)();

#define VAR_ADDRESS(name)  (g_##name##_v.location)
#define VAR_FOUND(name)  (g_##name##_v.bFound)


enum PATCH_CROSSREF_TYPE
{
	PX_JUMP,
	PX_VARIABLE_HI,
	PX_VARIABLE_LO
};

struct _PatchSymbol;



typedef struct
{
	u32 offset;				// Opcode offset
	PATCH_CROSSREF_TYPE nType;	// Type - J/JAL/LUI/ORI
	struct _PatchSymbol * pSymbol;		// Only one of these is valid
	struct _PatchVariable * pVariable;
} PatchCrossRef;

#define PATCH_PARTIAL_CRC_LEN 4

typedef struct
{
	u32 nNumOps;
	PatchCrossRef * pCrossRefs;
	PatchFunction pFunction;

	u32		firstop;			// First op in signature (used for rapid scanning)
	u32		partial_crc;		// Crc after 4 ops (ignoring symbol bits)
	u32		crc;				// Crc for entire buffer (ignoring symbol bits)
} PatchSignature;


typedef struct _PatchSymbol
{
	bool bFound;				// Has this symbol been found?
	u32 location;				// What is the address of the symbol?
	const char * szName;				// Symbol name

	OpCode opReplaced;			// The op that was replaced when we were patched in

	PatchSignature * pSignatures; // Crossrefs for this code (Function symbols only)

	PatchFunction pFunction;	// The installed patch Emulated function call (Function symbols only)
} PatchSymbol;

typedef struct _PatchVariable
{
	bool bFound;				// Has this symbol been found?
	u32 location;				// What is the address of the symbol?
	const char * szName;				// Symbol name

	// For now have these as separate fields. We probably want to 
	// put them into a union
	bool bFoundHi; bool bFoundLo; 
	u16 wHiPart; u16 wLoPart;

} PatchVariable;
///////////////////////////////////////////////////////////////

#define BEGIN_PATCH_XREFS(label) \
static PatchCrossRef g_##label##_x[] = {

#define PATCH_XREF_FUNCTION(offset, symbol) \
	{ offset, PX_JUMP, &g_##symbol##_s, NULL },

#define PATCH_XREF_VAR(offsethi, offsetlo, symbol) \
{ offsethi, PX_VARIABLE_HI, NULL, &g_##symbol##_v }, \
{ offsetlo, PX_VARIABLE_LO, NULL, &g_##symbol##_v },


#define PATCH_XREF_VAR_LO(offset, symbol) \
{ offset, PX_VARIABLE_LO, NULL, &g_##symbol##_v },

#define PATCH_XREF_VAR_HI(offset, symbol) \
{ offset, PX_VARIABLE_HI, NULL, &g_##symbol##_v },

#define END_PATCH_XREFS() \
	{ ~0, PX_JUMP, NULL, NULL }		\
};

/////////////////////////////////////////////////////////////////

#define BEGIN_PATCH_SIGNATURE_LIST(name) \
static PatchSignature	g_##name##_sig[] = {

#define PATCH_SIGNATURE_LIST_ENTRY(label, numops, firstop, partial, crc) \
{ numops, g_##label##_x, Patch_##label, firstop, partial, crc },

#define END_PATCH_SIGNATURE_LIST() \
{ 0, NULL, NULL, 0, 0 }         \
};
///////////////////////////////////////////////////////////////

#define PATCH_SYMBOL_FUNCTION(name) \
PatchSymbol g_##name##_s = { false, 0, #name, {{0}}, g_##name##_sig, NULL};

#define PATCH_SYMBOL_VARIABLE(name) \
PatchVariable g_##name##_v = { false, 0, #name, false, 0, false, 0};

#define BEGIN_PATCH_SYMBOL_TABLE(name) \
PatchSymbol * name[] = {

#define PATCH_FUNCTION_ENTRY(name)  \
	&g_##name##_s,

#define END_PATCH_SYMBOL_TABLE() \
NULL};

#define PATCH_SYMBOL_FUNCTION_ENTRY(name) g_##name##_s

#define BEGIN_PATCH_VARIABLE_TABLE(name) \
PatchVariable * name[] = {

#define PATCH_VARIABLE_ENTRY(name)  \
	&g_##name##_v,

#define END_PATCH_VARIABLE_TABLE() \
NULL};

#define CALL_PATCHED_FUNCTION(name) g_##name##_s.pFunction()

#ifdef DAEDALUS_ENABLE_OS_HOOKS

extern PatchSymbol * g_PatchSymbols[];
extern PatchVariable * g_PatchVariables[];

#endif

inline OpCode GetCorrectOp( OpCode op_code )
{
#ifdef DAEDALUS_BREAKPOINTS_ENABLED
	if (op_code.op == OP_DBG_BKPT)
	{
		u32 bp_index = op_code.bp_index;

		if (bp_index < g_BreakPoints.size())
		{
			op_code = g_BreakPoints[bp_index].mOriginalOp;
		}
	}
#endif
	return op_code;
}
extern u32 gNumOfOSFunctions;
void Patch_Reset();
void Patch_ApplyPatches();
void Patch_PatchAll();

#ifndef DAEDALUS_SILENT
const char * Patch_GetJumpAddressName(u32 jump);
u32 Patch_GetSymbolAddress(const char * szName);
#endif
#ifdef DUMPOSFUNCTIONS
void Patch_DumpOsThreadInfo();
void Patch_DumpOsQueueInfo();
void Patch_DumpOsEventInfo();
#endif

#endif
