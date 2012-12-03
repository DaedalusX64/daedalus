#include "stdafx.h"

//
//	Exclude this from public release builds to save a little on the elf size
//
#if defined(DAEDALUS_DEBUG_CONSOLE) || !defined(DAEDALUS_SILENT)

const char *Cop1WOpCodeNames[64] = {
	"-C1W?", "-C1W?", "-C1W?", "-C1W?", "-C1W?", "-C1W?", "-C1W?", "-C1W?",
	"-C1W?", "-C1W?", "-C1W?", "-C1W?", "-C1W?", "-C1W?", "-C1W?", "-C1W?",
	"-C1W?", "-C1W?", "-C1W?", "-C1W?", "-C1W?", "-C1W?", "-C1W?", "-C1W?",
	"-C1W?", "-C1W?", "-C1W?", "-C1W?", "-C1W?", "-C1W?", "-C1W?", "-C1W?",
	"CVT.S.W", "CVT.D.W", "-C1W?", "-C1W?", "-C1W?", "-C1W?", "-C1W?", "-C1W?",
	"-C1W?", "-C1W?", "-C1W?", "-C1W?", "-C1W?", "-C1W?", "-C1W?", "-C1W?",
	"-C1W?", "-C1W?", "-C1W?", "-C1W?", "-C1W?", "-C1W?", "-C1W?", "-C1W?",
	"-C1W?", "-C1W?", "-C1W?", "-C1W?", "-C1W?", "-C1W?", "-C1W?", "-C1W?"
};
const  char *Cop1LOpCodeNames[64] =
{
	"-C1L?", "-C1L?", "-C1L?", "-C1L?", "-C1L?", "-C1L?", "-C1L?", "-C1L?",
	"-C1L?", "-C1L?", "-C1L?", "-C1L?", "-C1L?", "-C1L?", "-C1L?", "-C1L?",
	"-C1L?", "-C1L?", "-C1L?", "-C1L?", "-C1L?", "-C1L?", "-C1L?", "-C1L?",
	"-C1L?", "-C1L?", "-C1L?", "-C1L?", "-C1L?", "-C1L?", "-C1L?", "-C1L?",
	"CVT.S.L", "CVT.D.L", "-C1L?", "-C1L?", "-C1L?", "-C1L?", "-C1L?", "-C1L?",
	"-C1L?", "-C1L?", "-C1L?", "-C1L?", "-C1L?", "-C1L?", "-C1L?", "-C1L?",
	"-C1L?", "-C1L?", "-C1L?", "-C1L?", "-C1L?", "-C1L?", "-C1L?", "-C1L?",
	"-C1L?", "-C1L?", "-C1L?", "-C1L?", "-C1L?", "-C1L?", "-C1L?", "-C1L?"
};

const char *RegNames[32] = {
	"r0", "at", "v0", "v1", "a0", "a1", "a2", "a3",
	"t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
	"s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7",
	"t8", "t9", "k0", "k1", "gp", "sp", "s8", "ra"
};


const char *Cop0RegNames[32] = {
	"Index", "Random", "EntryLo0", "EntryLo1", "Context", "PageMask", "Wired", "*RESERVED1*",
	"BadVAddr", "Count", "EntryHi", "Compare", "Status", "Cause", "EPC", "PRid",
	"Config", "LLAddr", "WatchLo", "WatchHi", "XContext", "*RESERVED2*", "*RESERVED3*", "*RESERVED4*",
	"*RESERVED5*", "*RESERVED6*", "PErr", "CacheErr", "TagLo", "TagHi", "ErrorEPC", "*RESERVED7*"
};
const char *ShortCop0RegNames[32] = {
	"Idx", "Rnd", "ELo0", "ELo1", "Ctx", "PMsk", "Wrd", "*",
	"BadV", "Cnt", "EHi", "Cmp", "Stat", "Caus", "EPC", "PRid",
	"Cfg", "LLA", "WLo", "WHi", "XCtx", "*", "*", "*",
	"*", "*", "PErr", "CErr", "TLo", "THi", "EEPC", "*"
};

#endif