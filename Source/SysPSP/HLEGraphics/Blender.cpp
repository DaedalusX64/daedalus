/*
Copyright (C) 2010 StrmnNrmn

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


#include "Base/Types.h"

#include "Core/ROM.h"
#include "HLEGraphics/DLDebug.h"
#include "SysPSP/HLEGraphics/RendererPSP.h"
#include "HLEGraphics/RDPStateManager.h"
#include <pspgu.h>


#ifdef DAEDALUS_DEBUG_DISPLAYLIST

const char * sc_szBlClr[4] = { "In",  "Mem",  "Bl",     "Fog" };
const char * sc_szBlA1[4]  = { "AIn", "AFog", "AShade", "0" };
const char * sc_szBlA2[4]  = { "1-A", "AMem", "1",      "?" };

inline void DebugBlender(u32 blender)
{
	static u32 mBlender = 0;

	if (mBlender != blender)
	{
		printf("********************************\n\n");
		printf(
			"Unknown Blender: %04x - %s * %s + %s * %s || %s * %s + %s * %s\n",
			blender,
			sc_szBlClr[(blender >> 14) & 0x3],
			sc_szBlA1[(blender >> 10) & 0x3],
			sc_szBlClr[(blender >> 6) & 0x3],
			sc_szBlA2[(blender >> 2) & 0x3],
			sc_szBlClr[(blender >> 12) & 0x3],
			sc_szBlA1[(blender >> 8) & 0x3],
			sc_szBlClr[(blender >> 4) & 0x3],
			sc_szBlA2[blender & 0x3]
		);
		printf("********************************\n\n");

		mBlender = blender;
	}
}

#endif


namespace
{

//
// N64 framebuffer blender inputs.
//

enum EBlenderColour
{
	BL_CLR_IN  = 0,
	BL_CLR_MEM = 1,
	BL_CLR_BL  = 2,
	BL_CLR_FOG = 3
};


enum EBlenderAlphaA
{
	BL_A_IN    = 0,
	BL_A_FOG   = 1,
	BL_A_SHADE = 2,
	BL_A_ZERO  = 3
};


enum EBlenderAlphaB
{
	BL_B_1MA  = 0,
	BL_B_MEM  = 1,
	BL_B_ONE  = 2,
	BL_B_ZERO = 3
};


struct SRDPBlenderCycle
{
	u8 ColourA;
	u8 AlphaA;
	u8 ColourB;
	u8 AlphaB;
};


struct SRDPBlenderMode
{
	SRDPBlenderCycle Cycle1;
	SRDPBlenderCycle Cycle2;
};


struct SBlenderState
{
	bool Enabled;

	int Op;
	int Src;
	int Dst;

	u32 SrcFix;
	u32 DstFix;

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	bool DebugUnknown;
#endif
};


#ifdef DAEDALUS_DEBUG_DISPLAYLIST

constexpr SBlenderState kBlendDisabled =
{
	false,
	GU_ADD,
	GU_SRC_ALPHA,
	GU_ONE_MINUS_SRC_ALPHA,
	0,
	0,
	false
};


constexpr SBlenderState kBlendAlpha =
{
	true,
	GU_ADD,
	GU_SRC_ALPHA,
	GU_ONE_MINUS_SRC_ALPHA,
	0,
	0,
	false
};


constexpr SBlenderState kBlendAlphaUnknown =
{
	true,
	GU_ADD,
	GU_SRC_ALPHA,
	GU_ONE_MINUS_SRC_ALPHA,
	0,
	0,
	true
};

#else

constexpr SBlenderState kBlendDisabled =
{
	false,
	GU_ADD,
	GU_SRC_ALPHA,
	GU_ONE_MINUS_SRC_ALPHA,
	0,
	0
};


constexpr SBlenderState kBlendAlpha =
{
	true,
	GU_ADD,
	GU_SRC_ALPHA,
	GU_ONE_MINUS_SRC_ALPHA,
	0,
	0
};

#endif


//*****************************************************************************
//
// Decode the 16-bit N64 framebuffer blender.
//
//*****************************************************************************
static SRDPBlenderMode DecodeBlender(u16 blender)
{
	SRDPBlenderMode mode{};

	mode.Cycle1.ColourA = (blender >> 14) & 0x3;
	mode.Cycle1.AlphaA  = (blender >> 10) & 0x3;
	mode.Cycle1.ColourB = (blender >> 6) & 0x3;
	mode.Cycle1.AlphaB  = (blender >> 2) & 0x3;

	mode.Cycle2.ColourA = (blender >> 12) & 0x3;
	mode.Cycle2.AlphaA  = (blender >> 8) & 0x3;
	mode.Cycle2.ColourB = (blender >> 4) & 0x3;
	mode.Cycle2.AlphaB  = blender & 0x3;

	return mode;
}


//*****************************************************************************
//
// In * AIn + Mem * (1 - A)
//
//*****************************************************************************
static bool IsStandardAlphaBlend(const SRDPBlenderCycle & cycle)
{
	return cycle.ColourA == BL_CLR_IN &&
		   cycle.AlphaA == BL_A_IN &&
		   cycle.ColourB == BL_CLR_MEM &&
		   cycle.AlphaB == BL_B_1MA;
}


//*****************************************************************************
//
// Detect equations which simplify to the input colour.
//
//*****************************************************************************
static bool IsPassThrough(const SRDPBlenderCycle & cycle)
{
	//
	// In * 0 + In * 1 = In
	//
	if (cycle.ColourA == BL_CLR_IN &&
		cycle.AlphaA == BL_A_ZERO &&
		cycle.ColourB == BL_CLR_IN &&
		cycle.AlphaB == BL_B_ONE)
	{
		return true;
	}

	//
	// In * A + In * (1 - A) = In
	//
	if (cycle.ColourA == BL_CLR_IN &&
		cycle.AlphaA == BL_A_IN &&
		cycle.ColourB == BL_CLR_IN &&
		cycle.AlphaB == BL_B_1MA)
	{
		return true;
	}

	return false;
}


//*****************************************************************************
//
// ISS64 state logger.
//
// Keep the working ISS64 hacks enabled while collecting this information.
// We want to determine whether these workarounds correspond to a generic
// RDP coverage/blender state rather than to ISS64 itself.
//
//*****************************************************************************
static void LogISS64BlenderState(u16 blendmode, u64 mux, bool two_cycles)
{
	if (g_ROM.GameHacks != ISS64)
		return;

	if (blendmode != 0xff5a && blendmode != 0x0050)
		return;

	static u16 last_blender = 0xffff;
	static u64 last_mux = ~0ULL;
	static u32 last_other_mode = 0xffffffff;
	static bool last_two_cycles = false;

	const u32 other_mode = gRDPOtherMode.L;

	if (blendmode == last_blender &&
		mux == last_mux &&
		other_mode == last_other_mode &&
		two_cycles == last_two_cycles)
	{
		return;
	}

	printf(
		"ISS64 BL=%04x "
		"MUX=%08x%08x "
		"L=%08x "
		"CYCLES=%u "
		"AA=%u "
		"IMRD=%u "
		"CLR_CVG=%u "
		"CVG_DST=%u "
		"CVG_X_A=%u "
		"A_CVG_SEL=%u "
		"FORCE=%u "
		"ZMODE=%u\n",
		blendmode,
		static_cast<u32>(mux >> 32),
		static_cast<u32>(mux),
		other_mode,
		two_cycles ? 2 : 1,
		gRDPOtherMode.aa_en,
		gRDPOtherMode.im_rd,
		gRDPOtherMode.clr_on_cvg,
		gRDPOtherMode.cvg_dst,
		gRDPOtherMode.cvg_x_alpha,
		gRDPOtherMode.alpha_cvg_sel,
		gRDPOtherMode.force_bl,
		gRDPOtherMode.zmode
	);

	last_blender = blendmode;
	last_mux = mux;
	last_other_mode = other_mode;
	last_two_cycles = two_cycles;
}


//*****************************************************************************
//
// Compatibility overrides.
//
//*****************************************************************************
static bool GetBlenderOverride(u16 blendmode, SBlenderState & state)
{
	//
	// Existing PSP compatibility exceptions.
	//
	switch (blendmode)
	{
		case 0x0fa5:
		case 0x8410:		// Paper Mario menu
		case 0xfa00:
			state = kBlendDisabled;
			return true;

		case 0xcb02:
			state = kBlendAlpha;
			return true;

		default:
			break;
	}

	//
	// ISS64 compatibility.
	//
	// Do NOT remove these during this test.
	//
	// 0xff5a is required for the player shadow.
	// 0x0050 removes the square/box rendering and is also involved in
	// correct translucent object rendering.
	//
	if (g_ROM.GameHacks == ISS64)
	{
		if (blendmode == 0xff5a)
		{
			state =
			{
				true,
				GU_REVERSE_SUBTRACT,
				GU_SRC_ALPHA,
				GU_FIX,
				0,
				0
#ifdef DAEDALUS_DEBUG_DISPLAYLIST
				, false
#endif
			};

			return true;
		}

		if (blendmode == 0x0050)
		{
			state =
			{
				true,
				GU_ADD,
				GU_SRC_ALPHA,
				GU_FIX,
				0,
				0x00ffffff
#ifdef DAEDALUS_DEBUG_DISPLAYLIST
				, false
#endif
			};

			return true;
		}
	}

	return false;
}


//*****************************************************************************
//
// Translate a simple RDP blender cycle into PSP GU state.
//
//*****************************************************************************
static bool TranslateBlenderCycle(const SRDPBlenderCycle & cycle, SBlenderState & state)
{
	if (IsPassThrough(cycle))
	{
		state = kBlendDisabled;
		return true;
	}

	if (IsStandardAlphaBlend(cycle))
	{
		state = kBlendAlpha;
		return true;
	}

	return false;
}


//*****************************************************************************
//
// Automatically translate the N64 framebuffer blender.
//
//*****************************************************************************
static SBlenderState GetAutomaticBlenderState(u16 blendmode)
{
	SBlenderState state{};

	if (GetBlenderOverride(blendmode, state))
		return state;

	const SRDPBlenderMode mode = DecodeBlender(blendmode);

	//
	// Keep Cycle2 for now while we validate the existing decoder.
	//
	const SRDPBlenderCycle & cycle = mode.Cycle2;

	if (TranslateBlenderCycle(cycle, state))
		return state;

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	return kBlendAlphaUnknown;
#else
	return kBlendAlpha;
#endif
}


//*****************************************************************************
//
// Original PSP implementation.
//
// IMPORTANT:
//
// Keep this function unchanged while validating the automatic translator.
//
//*****************************************************************************
static SBlenderState GetLegacyBlenderState(u16 blendmode)
{
	switch (blendmode)
	{
		case 0x0c08:
		case 0x0f0a:
		case 0x0fa5:
		case 0x8410:
		case 0xc302:
		case 0xc702:
		case 0xfa00:
			return kBlendDisabled;

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
		case 0x0150:
		case 0x0f5a:
		case 0x0010:
		case 0x0040:
		case 0x04d0:
		case 0x0c18:
		case 0xc410:
		case 0xc810:
		case 0xcb02:
			return kBlendAlpha;
#endif

		default:
			break;
	}

	if (g_ROM.GameHacks == ISS64)
	{
		if (blendmode == 0xff5a)
		{
			return
			{
				true,
				GU_REVERSE_SUBTRACT,
				GU_SRC_ALPHA,
				GU_FIX,
				0,
				0
#ifdef DAEDALUS_DEBUG_DISPLAYLIST
				, false
#endif
			};
		}

		if (blendmode == 0x0050)
		{
			return
			{
				true,
				GU_ADD,
				GU_SRC_ALPHA,
				GU_FIX,
				0,
				0x00ffffff
#ifdef DAEDALUS_DEBUG_DISPLAYLIST
				, false
#endif
			};
		}
	}

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	return kBlendAlphaUnknown;
#else
	return kBlendAlpha;
#endif
}


//*****************************************************************************
//
// Compare PSP rendering state.
//
// DebugUnknown is deliberately ignored.
//
//*****************************************************************************
static bool BlenderStatesEqual(const SBlenderState & a, const SBlenderState & b)
{
	return a.Enabled == b.Enabled &&
		   a.Op == b.Op &&
		   a.Src == b.Src &&
		   a.Dst == b.Dst &&
		   a.SrcFix == b.SrcFix &&
		   a.DstFix == b.DstFix;
}


//*****************************************************************************
//
// Apply translated state through RendererPSP's GU state cache.
//
//*****************************************************************************
static void ApplyBlenderState(const SBlenderState & state)
{
	if (!state.Enabled)
	{
		gRendererPSP->SetBlendEnabled(false);
		return;
	}

	gRendererPSP->SetBlendEnabled(true);
	gRendererPSP->SetBlendFunc(state.Op, state.Src, state.Dst, state.SrcFix, state.DstFix);
}

}


//*****************************************************************************
//
// Set N64 framebuffer blender mode.
//
//*****************************************************************************
void InitBlenderMode(u32 blendmode, u64 mux, bool two_cycles)
{
	const u16 blender = static_cast<u16>(blendmode);

	//
	// Log the complete state while keeping the known-good ISS64
	// compatibility behaviour enabled.
	//
	LogISS64BlenderState(blender, mux, two_cycles);

	const SBlenderState legacy_state = GetLegacyBlenderState(blender);
	const SBlenderState automatic_state = GetAutomaticBlenderState(blender);

	if (!BlenderStatesEqual(legacy_state, automatic_state))
	{
		printf(
			"BLENDER MISMATCH %04x "
			"legacy=%d auto=%d "
			"legacy_op=%d legacy_src=%d legacy_dst=%d "
			"auto_op=%d auto_src=%d auto_dst=%d\n",
			blender,
			legacy_state.Enabled,
			automatic_state.Enabled,
			legacy_state.Op,
			legacy_state.Src,
			legacy_state.Dst,
			automatic_state.Op,
			automatic_state.Src,
			automatic_state.Dst
		);

		//
		// Preserve known-good behaviour whenever the automatic
		// implementation disagrees.
		//
		ApplyBlenderState(legacy_state);
		return;
	}

	ApplyBlenderState(automatic_state);
}