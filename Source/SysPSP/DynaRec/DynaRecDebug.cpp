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


#ifdef DAEDALUS_PROFILE_EXECUTION

static void	DumpDynarecStats( float elapsed_time )
{
	// Temp dynarec stats
	extern u64 gTotalInstructionsEmulated;
	extern u64 gTotalInstructionsExecuted;
	extern u32 gTotalRegistersCached;
	extern u32 gTotalRegistersUncached;
	extern u32 gFragmentLookupSuccess;
	extern u32 gFragmentLookupFailure;

	u32		dynarec_ratio( 0 );

	if(gTotalInstructionsExecuted + gTotalInstructionsEmulated > 0)
	{
		float fRatio = float(gTotalInstructionsExecuted * 100.0f / float(gTotalInstructionsEmulated+gTotalInstructionsExecuted));

		dynarec_ratio = u32( fRatio );

		//gTotalInstructionsExecuted = 0;
		//gTotalInstructionsEmulated = 0;
	}

	u32		cached_regs_ratio( 0 );
	if(gTotalRegistersCached + gTotalRegistersUncached > 0)
	{
		float fRatio = float(gTotalRegistersCached * 100.0f / float(gTotalRegistersCached+gTotalRegistersUncached));

		cached_regs_ratio = u32( fRatio );
	}

	const char * const TERMINAL_SAVE_CURSOR			= "\033[s";
	const char * const TERMINAL_RESTORE_CURSOR		= "\033[u";
//	const char * const TERMINAL_TOP_LEFT			= "\033[2A\033[2K";
	const char * const TERMINAL_TOP_LEFT			= "\033[H\033[2K";

	printf( TERMINAL_SAVE_CURSOR );
	printf( TERMINAL_TOP_LEFT );

	printf( "Frame: %dms, DynaRec %d%%, Regs cached %d%%, Lookup success %d/%d", u32(elapsed_time * 1000.0f), dynarec_ratio, cached_regs_ratio, gFragmentLookupSuccess, gFragmentLookupFailure );

	printf( TERMINAL_RESTORE_CURSOR );
	fflush( stdout );

	gFragmentLookupSuccess = 0;
	gFragmentLookupFailure = 0;
}
#endif