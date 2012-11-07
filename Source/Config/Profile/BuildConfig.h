/*
Copyright (C) 2008 StrmnNrmn

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

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef CONFIG_H_
#define CONFIG_H_

///////////////////////////////////////////////////////////////////////////////
//
//	Config options for the psp gprof enabled builds
//
// For usage please read pspgprof.txt in the trunc dir.
///////////////////////////////////////////////////////////////////////////////
#define DAEDALUS_CONFIG_VERSION		"PSP GPROF"

#undef	DAEDALUS_DEBUG_PIF					// Enable to enable various debugging options for PIF (Peripheral interface)
#undef	DAEDALUS_DEBUG_CONSOLE				// Enable debug console
#define DAEDALUS_ALIGN_REGISTERS
#define	DAEDALUS_FAST_TMEM					// Disable for full TMEM emulation for palettes (slower and uses more memory)
#undef  DAEDALUS_DEBUG_DYNAREC				// Enable to enable various debugging options for the dynarec
#undef  DAEDALUS_ENABLE_SYNCHRONISATION		// Enable for sync testing
#undef  DAEDALUS_LOG						// Enable various logging
#undef DAEDALUS_ENABLE_ASSERTS				// Enable asserts
#undef  DAEDALUS_DEBUG_DISPLAYLIST			// Enable the display list debugger
#undef  DAEDALUS_ENABLE_PROFILING			// Enable the built-in profiler
#undef  DAEDALUS_PROFILE_EXECUTION			// Enable to keep track of various execution stats
#undef  DAEDALUS_BATCH_TEST_ENABLED			// Enable the batch test
#undef	DAEDALUS_DEBUG_MEMORY
#undef	ALLOW_TRACES_WHICH_EXCEPT
#define DAEDALUS_SILENT						// Undef to enable debug messages 
#undef  DAEDALUS_IS_LEGACY					// Old code, unused etc.. Kept for reference, undef to save space on the elf. Will remove soon.
#undef	DAEDALUS_DIALOGS					// Enable this to show dialogs in the gui

// This must be Enable, otherwise no gprof output.
#define DAEDALUS_PSP_GPROF

#endif // CONFIG_H_
