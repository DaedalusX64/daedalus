#include "stdafx.h"
#include "DLDebug.h"

#ifdef DAEDALUS_DEBUG_DISPLAYLIST

#include <stdarg.h>

#include "RDP.h"
#include "Core/ROM.h"

#include "Debug/DBGConsole.h"
#include "Debug/Dump.h"
#include "OSHLE/ultra_gbi.h"
#include "Utility/IO.h"

void DLDebug_Printf(const char * fmt, ...)
{
	static const u32 kBufferLen = 1024;
	char buffer[kBufferLen];

	va_list va;
	va_start(va, fmt);

	// I've never been confident that this returns a sane value across platforms.
	/*len = */vsnprintf( buffer, kBufferLen, fmt, va );

	// This should be guaranteed...
	buffer[kBufferLen-1] = 0;
	va_end(va);

	size_t len = strlen(buffer);

	// Append a newline, if there's space in the buffer.
	if (len < 1024)
	{
		buffer[len] = '\n';
		++len;
	}

	gDisplayListSink->Write(buffer, len);
}


DataSink *			gDisplayListSink 			= NULL;
static bool 		gDumpNextDisplayList 		= false;
static const char *	gDisplayListRootPath 		= "DisplayLists";
static const char *	gDisplayListDumpPathFormat 	= "dl%04d.txt";

static const char * const kMulInputRGB[32] =
{
	"Combined    ", "Texel0      ",
	"Texel1      ", "Primitive   ",
	"Shade       ", "Env         ",
	"KeyScale    ", "CombinedAlph",
	"Texel0_Alpha", "Texel1_Alpha",
	"Prim_Alpha  ", "Shade_Alpha ",
	"Env_Alpha   ", "LOD_Frac    ",
	"PrimLODFrac ", "K5          ",
	"0           ", "0           ",
	"0           ", "0           ",
	"0           ", "0           ",
	"0           ", "0           ",
	"0           ", "0           ",
	"0           ", "0           ",
	"0           ", "0           ",
	"0           ",	"0           "
};

static const char * const kSubAInputRGB[16] =
{
	"Combined    ", "Texel0      ",
	"Texel1      ", "Primitive   ",
	"Shade       ", "Env         ",
	"1           ", "Noise       ",
	"0           ", "0           ",
	"0           ", "0           ",
	"0           ", "0           ",
	"0           ", "0           ",
};

static const char * const kSubBInputRGB[16] =
{
	"Combined    ", "Texel0      ",
	"Texel1      ", "Primitive   ",
	"Shade       ", "Env         ",
	"KeyCenter   ", "K4          ",
	"0           ", "0           ",
	"0           ", "0           ",
	"0           ", "0           ",
	"0           ", "0           ",
};

static const char * const kAddInputRGB[8] =
{
	"Combined    ", "Texel0      ",
	"Texel1      ", "Primitive   ",
	"Shade       ", "Env         ",
	"1           ", "0           ",
};

static const char * const kSubInputAlpha[8] =
{
	"Combined    ", "Texel0      ",
	"Texel1      ", "Primitive   ",
	"Shade       ", "Env         ",
	"1           ", "0           ",
};

static const char * const kMulInputAlpha[8] =
{
	"LOD_Frac    ", "Texel0      ",
	"Texel1      ", "Primitive   ",
	"Shade       ", "Env         ",
	"PrimLOD_Frac", "0           ",
};

static const char * const kAddInputAlpha[8] =
{
	"Combined    ", "Texel0      ",
	"Texel1      ", "Primitive   ",
	"Shade       ", "Env         ",
	"1           ", "0           ",
};


void DLDebug_DumpMux( u64 mux )
{
	u32 mux0 = (u32)(mux>>32);
	u32 mux1 = (u32)(mux);

	u32 aRGB0  = (mux0>>20)&0x0F;	// c1 c1		// a0
	u32 bRGB0  = (mux1>>28)&0x0F;	// c1 c2		// b0
	u32 cRGB0  = (mux0>>15)&0x1F;	// c1 c3		// c0
	u32 dRGB0  = (mux1>>15)&0x07;	// c1 c4		// d0

	u32 aA0    = (mux0>>12)&0x07;	// c1 a1		// Aa0
	u32 bA0    = (mux1>>12)&0x07;	// c1 a2		// Ab0
	u32 cA0    = (mux0>>9 )&0x07;	// c1 a3		// Ac0
	u32 dA0    = (mux1>>9 )&0x07;	// c1 a4		// Ad0

	u32 aRGB1  = (mux0>>5 )&0x0F;	// c2 c1		// a1
	u32 bRGB1  = (mux1>>24)&0x0F;	// c2 c2		// b1
	u32 cRGB1  = (mux0    )&0x1F;	// c2 c3		// c1
	u32 dRGB1  = (mux1>>6 )&0x07;	// c2 c4		// d1

	u32 aA1    = (mux1>>21)&0x07;	// c2 a1		// Aa1
	u32 bA1    = (mux1>>3 )&0x07;	// c2 a2		// Ab1
	u32 cA1    = (mux1>>18)&0x07;	// c2 a3		// Ac1
	u32 dA1    = (mux1    )&0x07;	// c2 a4		// Ad1

	DL_PF("    Mux: 0x%08x%08x", mux0, mux1);

	DL_PF("    RGB0: (%s - %s) * %s + %s", kSubAInputRGB[aRGB0], kSubBInputRGB[bRGB0], kMulInputRGB[cRGB0], kAddInputRGB[dRGB0]);
	DL_PF("    A0  : (%s - %s) * %s + %s", kSubInputAlpha[aA0],  kSubInputAlpha[bA0],  kMulInputAlpha[cA0], kAddInputAlpha[dA0]);
	DL_PF("    RGB1: (%s - %s) * %s + %s", kSubAInputRGB[aRGB1], kSubBInputRGB[bRGB1], kMulInputRGB[cRGB1], kAddInputRGB[dRGB1]);
	DL_PF("    A1  : (%s - %s) * %s + %s", kSubInputAlpha[aA1],  kSubInputAlpha[bA1],  kMulInputAlpha[cA1], kAddInputAlpha[dA1]);
}

void DLDebug_PrintMux( FILE * fh, u64 mux )
{
	u32 mux0 = (u32)(mux>>32);
	u32 mux1 = (u32)(mux);

	u32 aRGB0  = (mux0>>20)&0x0F;	// c1 c1		// a0
	u32 bRGB0  = (mux1>>28)&0x0F;	// c1 c2		// b0
	u32 cRGB0  = (mux0>>15)&0x1F;	// c1 c3		// c0
	u32 dRGB0  = (mux1>>15)&0x07;	// c1 c4		// d0

	u32 aA0    = (mux0>>12)&0x07;	// c1 a1		// Aa0
	u32 bA0    = (mux1>>12)&0x07;	// c1 a2		// Ab0
	u32 cA0    = (mux0>>9 )&0x07;	// c1 a3		// Ac0
	u32 dA0    = (mux1>>9 )&0x07;	// c1 a4		// Ad0

	u32 aRGB1  = (mux0>>5 )&0x0F;	// c2 c1		// a1
	u32 bRGB1  = (mux1>>24)&0x0F;	// c2 c2		// b1
	u32 cRGB1  = (mux0    )&0x1F;	// c2 c3		// c1
	u32 dRGB1  = (mux1>>6 )&0x07;	// c2 c4		// d1

	u32 aA1    = (mux1>>21)&0x07;	// c2 a1		// Aa1
	u32 bA1    = (mux1>>3 )&0x07;	// c2 a2		// Ab1
	u32 cA1    = (mux1>>18)&0x07;	// c2 a3		// Ac1
	u32 dA1    = (mux1    )&0x07;	// c2 a4		// Ad1

	fprintf(fh, "//case 0x%08x%08xLL:\n", mux0, mux1);
	fprintf(fh, "//aRGB0: (%s - %s) * %s + %s\n", kSubAInputRGB[aRGB0], kSubBInputRGB[bRGB0], kMulInputRGB[cRGB0], kAddInputRGB[dRGB0]);
	fprintf(fh, "//aA0  : (%s - %s) * %s + %s\n", kSubInputAlpha[aA0],  kSubInputAlpha[bA0],  kMulInputAlpha[cA0], kAddInputAlpha[dA0]);
	fprintf(fh, "//aRGB1: (%s - %s) * %s + %s\n", kSubAInputRGB[aRGB1], kSubBInputRGB[bRGB1], kMulInputRGB[cRGB1], kAddInputRGB[dRGB1]);
	fprintf(fh, "//aA1  : (%s - %s) * %s + %s\n", kSubInputAlpha[aA1],  kSubInputAlpha[bA1],  kMulInputAlpha[cA1], kAddInputAlpha[dA1]);
	fprintf(fh, "void BlendMode_0x%08x%08xLL( BLEND_MODE_ARGS )\n{\n}\n\n", mux0, mux1);
}

static const char * const kBlendCl[] = { "In",  "Mem",  "Bl",     "Fog" };
static const char * const kBlendA1[] = { "AIn", "AFog", "AShade", "0" };
static const char * const kBlendA2[] = { "1-A", "AMem", "1",      "?" };

void DLDebug_DumpRDPOtherMode(const RDP_OtherMode & mode)
{
	if (DLDebug_IsActive())
	{
		// High
		static const char *alphadithertypes[4]	= {"Pattern", "NotPattern", "Noise", "Disable"};
		static const char *rgbdithertype[4]		= {"MagicSQ", "Bayer", "Noise", "Disable"};
		static const char *convtype[8]			= {"Conv", "?", "?", "?",   "?", "FiltConv", "Filt", "?"};
		static const char *filtertype[4]		= {"Point", "?", "Bilinear", "Average"};
		static const char *textluttype[4]		= {"None", "?", "RGBA16", "IA16"};
		static const char *cycletype[4]			= {"1Cycle", "2Cycle", "Copy", "Fill"};
		static const char *detailtype[4]		= {"Clamp", "Sharpen", "Detail", "?"};
		static const char *alphacomptype[4]		= {"None", "Threshold", "?", "Dither"};
		static const char * szCvgDstMode[4]		= { "Clamp", "Wrap", "Full", "Save" };
		static const char * szZMode[4]			= { "Opa", "Inter", "XLU", "Decal" };
		static const char * szZSrcSel[2]		= { "Pixel", "Primitive" };

		u32 dwM1A_1 = (mode.blender>>14) & 0x3;
		u32 dwM1B_1 = (mode.blender>>10) & 0x3;
		u32 dwM2A_1 = (mode.blender>>6) & 0x3;
		u32 dwM2B_1 = (mode.blender>>2) & 0x3;

		u32 dwM1A_2 = (mode.blender>>12) & 0x3;
		u32 dwM1B_2 = (mode.blender>>8) & 0x3;
		u32 dwM2A_2 = (mode.blender>>4) & 0x3;
		u32 dwM2B_2 = (mode.blender   ) & 0x3;

		DL_PF( "    alpha_compare: %s", alphacomptype[ mode.alpha_compare ]);
		DL_PF( "    depth_source:  %s", szZSrcSel[ mode.depth_source ]);
		DL_PF( "    aa_en:         %d", mode.aa_en );
		DL_PF( "    z_cmp:         %d", mode.z_cmp );
		DL_PF( "    z_upd:         %d", mode.z_upd );
		DL_PF( "    im_rd:         %d", mode.im_rd );
		DL_PF( "    clr_on_cvg:    %d", mode.clr_on_cvg );
		DL_PF( "    cvg_dst:       %s", szCvgDstMode[ mode.cvg_dst ] );
		DL_PF( "    zmode:         %s", szZMode[ mode.zmode ] );
		DL_PF( "    cvg_x_alpha:   %d", mode.cvg_x_alpha );
		DL_PF( "    alpha_cvg_sel: %d", mode.alpha_cvg_sel );
		DL_PF( "    force_bl:      %d", mode.force_bl );
		DL_PF( "    tex_edge:      %d", mode.tex_edge );
		DL_PF( "    blender:       %04x - %s*%s + %s*%s | %s*%s + %s*%s", mode.blender,
										kBlendCl[dwM1A_1], kBlendA1[dwM1B_1], kBlendCl[dwM2A_1], kBlendA2[dwM2B_1],
										kBlendCl[dwM1A_2], kBlendA1[dwM1B_2], kBlendCl[dwM2A_2], kBlendA2[dwM2B_2]);
		DL_PF( "    blend_mask:    %d", mode.blend_mask );
		DL_PF( "    alpha_dither:  %s", alphadithertypes[ mode.alpha_dither ] );
		DL_PF( "    rgb_dither:    %s", rgbdithertype[ mode.rgb_dither ] );
		DL_PF( "    comb_key:      %s", mode.comb_key ? "Key" : "None" );
		DL_PF( "    text_conv:     %s", convtype[ mode.text_conv ] );
		DL_PF( "    text_filt:     %s", filtertype[ mode.text_filt ] );
		DL_PF( "    text_tlut:     %s", textluttype[ mode.text_tlut ] );
		DL_PF( "    text_lod:      %s", mode.text_lod ? "LOD": "Tile" );
		DL_PF( "    text_detail:   %s", detailtype[ mode.text_detail ] );
		DL_PF( "    text_persp:    %s", mode.text_persp ? "On" : "Off" );
		DL_PF( "    cycle_type:    %s", cycletype[ mode.cycle_type ] );
		DL_PF( "    color_dither:  %d", mode.color_dither );
		DL_PF( "    pipeline:      %s", mode.pipeline ? "1Primitive" : "NPrimitive" );
	}
}

static void	DLParser_DumpTaskInfo( const OSTask * pTask )
{
	DL_PF( "Task:         %08x",      pTask->t.type  );
	DL_PF( "Flags:        %08x",      pTask->t.flags  );
	DL_PF( "BootCode:     %08x", (u32)pTask->t.ucode_boot  );
	DL_PF( "BootCodeSize: %08x",      pTask->t.ucode_boot_size  );

	DL_PF( "uCode:        %08x", (u32)pTask->t.ucode );
	DL_PF( "uCodeSize:    %08x",      pTask->t.ucode_size );
	DL_PF( "uCodeData:    %08x", (u32)pTask->t.ucode_data );
	DL_PF( "uCodeDataSize:%08x",      pTask->t.ucode_data_size );

	DL_PF( "Stack:        %08x", (u32)pTask->t.dram_stack );
	DL_PF( "StackS:       %08x",      pTask->t.dram_stack_size );
	DL_PF( "Output:       %08x", (u32)pTask->t.output_buff );
	DL_PF( "OutputS:      %08x", (u32)pTask->t.output_buff_size );

	DL_PF( "Data( PC ):   %08x", (u32)pTask->t.data_ptr );
	DL_PF( "DataSize:     %08x",      pTask->t.data_size );
	DL_PF( "YieldData:    %08x", (u32)pTask->t.yield_data_ptr );
	DL_PF( "YieldDataSize:%08x",      pTask->t.yield_data_size );
}

void DLDebug_HandleDumpDisplayList( OSTask * pTask )
{
	if (gDumpNextDisplayList)
	{
		DBGConsole_Msg( 0, "Dumping display list" );
		static u32 count = 0;

		char szFilePath[MAX_PATH+1];
		char szFileName[MAX_PATH+1];
		char szDumpDir[MAX_PATH+1];

		IO::Path::Combine(szDumpDir, g_ROM.settings.GameName.c_str(), gDisplayListRootPath);

		Dump_GetDumpDirectory(szFilePath, szDumpDir);

		sprintf(szFileName, "dl%04d.txt", count++);

		IO::Path::Append(szFilePath, szFileName);

		FileSink * sink = new FileSink();
		if (sink->Open(szFilePath, "w"))
		{
			gDisplayListSink = sink;
			DBGConsole_Msg(0, "RDP: Dumping Display List as %s", szFilePath);
		}
		else
		{
			delete sink;
			DBGConsole_Msg(0, "RDP: Couldn't create dumpfile %s", szFilePath);
		}

		DLParser_DumpTaskInfo( pTask );

		// Clear flag as we're done
		gDumpNextDisplayList = false;
	}
}

void DLDebug_Finish()
{
	if (gDisplayListSink)
	{
		delete gDisplayListSink;
		gDisplayListSink = NULL;
	}
}

void DLDebug_DumpNextDisplayList()
{
	gDumpNextDisplayList = true;
}

#endif // DAEDALUS_DEBUG_DISPLAYLIST
