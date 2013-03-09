#include "stdafx.h"
#include "DLDebug.h"

#ifdef DAEDALUS_DEBUG_DISPLAYLIST

#include "RDP.h"
#include "Core/ROM.h"

#include "Debug/DBGConsole.h"
#include "Debug/Dump.h"
#include "OSHLE/ultra_gbi.h"
#include "Utility/IO.h"


FILE * 				gDisplayListFile 			= NULL;
static bool 		gDumpNextDisplayList 		= false;
static const char *	gDisplayListRootPath 		= "DisplayLists";
static const char *	gDisplayListDumpPathFormat 	= "dl%04d.txt";

static const char * const sc_colcombtypes32[32] =
{
	"Combined    ", "Texel0      ",
	"Texel1      ", "Primitive   ",
	"Shade       ", "Env         ",
	"1           ", "CombAlp     ",
	"Texel0_Alp  ", "Texel1_Alp  ",
	"Prim_Alpha  ", "Shade_Alpha ",
	"Env_Alpha   ", "LOD_Frac    ",
	"PrimLODFrac ", "K5          ",
	"?           ", "?           ",
	"?           ", "?           ",
	"?           ", "?           ",
	"?           ", "?           ",
	"?           ", "?           ",
	"?           ", "?           ",
	"?           ", "?           ",
	"?           ",	"0           "
};

static const char * const sc_colcombtypes16[16] =
{
	"Combined    ", "Texel0      ",
	"Texel1      ", "Primitive   ",
	"Shade       ", "Env         ",
	"1           ", "CombAlp     ",
	"Texel0_Alp  ", "Texel1_Alp  ",
	"Prim_Alp    ", "Shade_Alpha ",
	"Env_Alpha   ", "LOD_Frac    ",
	"PrimLOD_Frac", "0           "
};

static const char * const sc_colcombtypes8[8] =
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

	DL_PF("    RGB0: (%s - %s) * %s + %s", sc_colcombtypes16[aRGB0], sc_colcombtypes16[bRGB0], sc_colcombtypes32[cRGB0], sc_colcombtypes8[dRGB0]);
	DL_PF("    A0  : (%s - %s) * %s + %s", sc_colcombtypes8[aA0], sc_colcombtypes8[bA0], sc_colcombtypes8[cA0], sc_colcombtypes8[dA0]);
	DL_PF("    RGB1: (%s - %s) * %s + %s", sc_colcombtypes16[aRGB1], sc_colcombtypes16[bRGB1], sc_colcombtypes32[cRGB1], sc_colcombtypes8[dRGB1]);
	DL_PF("    A1  : (%s - %s) * %s + %s", sc_colcombtypes8[aA1],  sc_colcombtypes8[bA1], sc_colcombtypes8[cA1],  sc_colcombtypes8[dA1]);
}

static const char * sc_szBlClr[4] = { "In",  "Mem",  "Bl",     "Fog" };
static const char * sc_szBlA1[4]  = { "AIn", "AFog", "AShade", "0" };
static const char * sc_szBlA2[4]  = { "1-A", "AMem", "1",      "?" };

#define	G_BL_CLR_IN	0
#define	G_BL_CLR_MEM	1
#define	G_BL_CLR_BL	2
#define	G_BL_CLR_FOG	3
#define	G_BL_1MA	0
#define	G_BL_A_MEM	1
#define	G_BL_A_IN	0
#define	G_BL_A_FOG	1
#define	G_BL_A_SHADE	2
#define	G_BL_1		2
#define	G_BL_0		3

#define	GBL_c1(m1a, m1b, m2a, m2b)	\
	(m1a) << 30 | (m1b) << 26 | (m2a) << 22 | (m2b) << 18
#define	GBL_c2(m1a, m1b, m2a, m2b)	\
	(m1a) << 28 | (m1b) << 24 | (m2a) << 20 | (m2b) << 16

#define	RM_AA_ZB_OPA_SURF(clk)					\
	AA_EN | Z_CMP | Z_UPD | IM_RD | CVG_DST_CLAMP |		\
	ZMODE_OPA | ALPHA_CVG_SEL |				\
	GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_A_MEM)

#define	RM_RA_ZB_OPA_SURF(clk)					\
	AA_EN | Z_CMP | Z_UPD | CVG_DST_CLAMP |			\
	ZMODE_OPA | ALPHA_CVG_SEL |				\
	GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_A_MEM)

#define	RM_AA_ZB_XLU_SURF(clk)					\
	AA_EN | Z_CMP | IM_RD | CVG_DST_WRAP | CLR_ON_CVG |	\
	FORCE_BL | ZMODE_XLU |					\
	GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA)

#define	RM_AA_ZB_OPA_DECAL(clk)					\
	AA_EN | Z_CMP | IM_RD | CVG_DST_WRAP | ALPHA_CVG_SEL |	\
	ZMODE_DEC |						\
	GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_A_MEM)

#define	RM_RA_ZB_OPA_DECAL(clk)					\
	AA_EN | Z_CMP | CVG_DST_WRAP | ALPHA_CVG_SEL |		\
	ZMODE_DEC |						\
	GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_A_MEM)

#define	RM_AA_ZB_XLU_DECAL(clk)					\
	AA_EN | Z_CMP | IM_RD | CVG_DST_WRAP | CLR_ON_CVG |	\
	FORCE_BL | ZMODE_DEC |					\
	GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA)

#define	RM_AA_ZB_OPA_INTER(clk)					\
	AA_EN | Z_CMP | Z_UPD | IM_RD | CVG_DST_CLAMP |		\
	ALPHA_CVG_SEL |	ZMODE_INTER |				\
	GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_A_MEM)

#define	RM_RA_ZB_OPA_INTER(clk)					\
	AA_EN | Z_CMP | Z_UPD | CVG_DST_CLAMP |			\
	ALPHA_CVG_SEL |	ZMODE_INTER |				\
	GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_A_MEM)

#define	RM_AA_ZB_XLU_INTER(clk)					\
	AA_EN | Z_CMP | IM_RD | CVG_DST_WRAP | CLR_ON_CVG |	\
	FORCE_BL | ZMODE_INTER |				\
	GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA)

#define	RM_AA_ZB_XLU_LINE(clk)					\
	AA_EN | Z_CMP | IM_RD | CVG_DST_CLAMP | CVG_X_ALPHA |	\
	ALPHA_CVG_SEL | FORCE_BL | ZMODE_XLU |			\
	GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA)

#define	RM_AA_ZB_DEC_LINE(clk)					\
	AA_EN | Z_CMP | IM_RD | CVG_DST_SAVE | CVG_X_ALPHA |	\
	ALPHA_CVG_SEL | FORCE_BL | ZMODE_DEC |			\
	GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA)

#define	RM_AA_ZB_TEX_EDGE(clk)					\
	AA_EN | Z_CMP | Z_UPD | IM_RD | CVG_DST_CLAMP |		\
	CVG_X_ALPHA | ALPHA_CVG_SEL | ZMODE_OPA | TEX_EDGE |	\
	GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_A_MEM)

#define	RM_AA_ZB_TEX_INTER(clk)					\
	AA_EN | Z_CMP | Z_UPD | IM_RD | CVG_DST_CLAMP |		\
	CVG_X_ALPHA | ALPHA_CVG_SEL | ZMODE_INTER | TEX_EDGE |	\
	GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_A_MEM)

#define	RM_AA_ZB_SUB_SURF(clk)					\
	AA_EN | Z_CMP | Z_UPD | IM_RD | CVG_DST_FULL |		\
	ZMODE_OPA | ALPHA_CVG_SEL |				\
	GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_A_MEM)

#define	RM_AA_ZB_PCL_SURF(clk)					\
	AA_EN | Z_CMP | Z_UPD | IM_RD | CVG_DST_CLAMP |		\
	ZMODE_OPA | G_AC_DITHER | 				\
	GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA)

#define	RM_AA_ZB_OPA_TERR(clk)					\
	AA_EN | Z_CMP | Z_UPD | IM_RD | CVG_DST_CLAMP |		\
	ZMODE_OPA | ALPHA_CVG_SEL |				\
	GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA)

#define	RM_AA_ZB_TEX_TERR(clk)					\
	AA_EN | Z_CMP | Z_UPD | IM_RD | CVG_DST_CLAMP |		\
	CVG_X_ALPHA | ALPHA_CVG_SEL | ZMODE_OPA | TEX_EDGE |	\
	GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA)

#define	RM_AA_ZB_SUB_TERR(clk)					\
	AA_EN | Z_CMP | Z_UPD | IM_RD | CVG_DST_FULL |		\
	ZMODE_OPA | ALPHA_CVG_SEL |				\
	GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA)


#define	RM_AA_OPA_SURF(clk)					\
	AA_EN | IM_RD | CVG_DST_CLAMP |				\
	ZMODE_OPA | ALPHA_CVG_SEL |				\
	GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_A_MEM)

#define	RM_RA_OPA_SURF(clk)					\
	AA_EN | CVG_DST_CLAMP |				\
	ZMODE_OPA | ALPHA_CVG_SEL |				\
	GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_A_MEM)

#define	RM_AA_XLU_SURF(clk)					\
	AA_EN | IM_RD | CVG_DST_WRAP | CLR_ON_CVG | FORCE_BL |	\
	ZMODE_OPA |						\
	GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA)

#define	RM_AA_XLU_LINE(clk)					\
	AA_EN | IM_RD | CVG_DST_CLAMP | CVG_X_ALPHA |		\
	ALPHA_CVG_SEL | FORCE_BL | ZMODE_OPA |			\
	GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA)

#define	RM_AA_DEC_LINE(clk)					\
	AA_EN | IM_RD | CVG_DST_FULL | CVG_X_ALPHA |		\
	ALPHA_CVG_SEL | FORCE_BL | ZMODE_OPA |			\
	GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA)

#define	RM_AA_TEX_EDGE(clk)					\
	AA_EN | IM_RD | CVG_DST_CLAMP |				\
	CVG_X_ALPHA | ALPHA_CVG_SEL | ZMODE_OPA | TEX_EDGE |	\
	GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_A_MEM)

#define	RM_AA_SUB_SURF(clk)					\
	AA_EN | IM_RD | CVG_DST_FULL |				\
	ZMODE_OPA | ALPHA_CVG_SEL |				\
	GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_A_MEM)

#define	RM_AA_PCL_SURF(clk)					\
	AA_EN | IM_RD | CVG_DST_CLAMP |				\
	ZMODE_OPA | G_AC_DITHER | 				\
	GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA)

#define	RM_AA_OPA_TERR(clk)					\
	AA_EN | IM_RD | CVG_DST_CLAMP |				\
	ZMODE_OPA | ALPHA_CVG_SEL |				\
	GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA)

#define	RM_AA_TEX_TERR(clk)					\
	AA_EN | IM_RD | CVG_DST_CLAMP |				\
	CVG_X_ALPHA | ALPHA_CVG_SEL | ZMODE_OPA | TEX_EDGE |	\
	GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA)

#define	RM_AA_SUB_TERR(clk)					\
	AA_EN | IM_RD | CVG_DST_FULL |				\
	ZMODE_OPA | ALPHA_CVG_SEL |				\
	GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA)


#define	RM_ZB_OPA_SURF(clk)					\
	Z_CMP | Z_UPD | CVG_DST_FULL | ALPHA_CVG_SEL |		\
	ZMODE_OPA |						\
	GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_A_MEM)

#define	RM_ZB_XLU_SURF(clk)					\
	Z_CMP | IM_RD | CVG_DST_FULL | FORCE_BL | ZMODE_XLU |	\
	GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA)

#define	RM_ZB_OPA_DECAL(clk)					\
	Z_CMP | CVG_DST_FULL | ALPHA_CVG_SEL | ZMODE_DEC |	\
	GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_A_MEM)

#define	RM_ZB_XLU_DECAL(clk)					\
	Z_CMP | IM_RD | CVG_DST_FULL | FORCE_BL | ZMODE_DEC |	\
	GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA)

#define	RM_ZB_CLD_SURF(clk)					\
	Z_CMP | IM_RD | CVG_DST_SAVE | FORCE_BL | ZMODE_XLU |	\
	GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA)

#define	RM_ZB_OVL_SURF(clk)					\
	Z_CMP | IM_RD | CVG_DST_SAVE | FORCE_BL | ZMODE_DEC |	\
	GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA)

#define	RM_ZB_PCL_SURF(clk)					\
	Z_CMP | Z_UPD | CVG_DST_FULL | ZMODE_OPA |		\
	G_AC_DITHER | 						\
	GBL_c##clk(G_BL_CLR_IN, G_BL_0, G_BL_CLR_IN, G_BL_1)


#define	RM_OPA_SURF(clk)					\
	CVG_DST_CLAMP | FORCE_BL | ZMODE_OPA |			\
	GBL_c##clk(G_BL_CLR_IN, G_BL_0, G_BL_CLR_IN, G_BL_1)

#define	RM_XLU_SURF(clk)					\
	IM_RD | CVG_DST_FULL | FORCE_BL | ZMODE_OPA |		\
	GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA)

#define	RM_TEX_EDGE(clk)					\
	CVG_DST_CLAMP | CVG_X_ALPHA | ALPHA_CVG_SEL | FORCE_BL |\
	ZMODE_OPA | TEX_EDGE | AA_EN |					\
	GBL_c##clk(G_BL_CLR_IN, G_BL_0, G_BL_CLR_IN, G_BL_1)

#define	RM_CLD_SURF(clk)					\
	IM_RD | CVG_DST_SAVE | FORCE_BL | ZMODE_OPA |		\
	GBL_c##clk(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA)

#define	RM_PCL_SURF(clk)					\
	CVG_DST_FULL | FORCE_BL | ZMODE_OPA | 			\
	G_AC_DITHER | 						\
	GBL_c##clk(G_BL_CLR_IN, G_BL_0, G_BL_CLR_IN, G_BL_1)

#define	RM_ADD(clk)					\
	IM_RD | CVG_DST_SAVE | FORCE_BL | ZMODE_OPA |	\
	GBL_c##clk(G_BL_CLR_IN, G_BL_A_FOG, G_BL_CLR_MEM, G_BL_1)

#define	RM_NOOP(clk)	\
	GBL_c##clk(0, 0, 0, 0)

#define RM_VISCVG(clk) \
	IM_RD | FORCE_BL |     \
	GBL_c##clk(G_BL_CLR_IN, G_BL_0, G_BL_CLR_BL, G_BL_A_MEM)

/* for rendering to an 8-bit framebuffer */
#define RM_OPA_CI(clk)                    \
	CVG_DST_CLAMP | ZMODE_OPA |          \
	GBL_c##clk(G_BL_CLR_IN, G_BL_0, G_BL_CLR_IN, G_BL_1)



#define	G_RM_AA_ZB_OPA_SURF	RM_AA_ZB_OPA_SURF(1)
#define	G_RM_AA_ZB_OPA_SURF2	RM_AA_ZB_OPA_SURF(2)
#define	G_RM_AA_ZB_XLU_SURF	RM_AA_ZB_XLU_SURF(1)
#define	G_RM_AA_ZB_XLU_SURF2	RM_AA_ZB_XLU_SURF(2)
#define	G_RM_AA_ZB_OPA_DECAL	RM_AA_ZB_OPA_DECAL(1)
#define	G_RM_AA_ZB_OPA_DECAL2	RM_AA_ZB_OPA_DECAL(2)
#define	G_RM_AA_ZB_XLU_DECAL	RM_AA_ZB_XLU_DECAL(1)
#define	G_RM_AA_ZB_XLU_DECAL2	RM_AA_ZB_XLU_DECAL(2)
#define	G_RM_AA_ZB_OPA_INTER	RM_AA_ZB_OPA_INTER(1)
#define	G_RM_AA_ZB_OPA_INTER2	RM_AA_ZB_OPA_INTER(2)
#define	G_RM_AA_ZB_XLU_INTER	RM_AA_ZB_XLU_INTER(1)
#define	G_RM_AA_ZB_XLU_INTER2	RM_AA_ZB_XLU_INTER(2)
#define	G_RM_AA_ZB_XLU_LINE	RM_AA_ZB_XLU_LINE(1)
#define	G_RM_AA_ZB_XLU_LINE2	RM_AA_ZB_XLU_LINE(2)
#define	G_RM_AA_ZB_DEC_LINE	RM_AA_ZB_DEC_LINE(1)
#define	G_RM_AA_ZB_DEC_LINE2	RM_AA_ZB_DEC_LINE(2)
#define	G_RM_AA_ZB_TEX_EDGE	RM_AA_ZB_TEX_EDGE(1)
#define	G_RM_AA_ZB_TEX_EDGE2	RM_AA_ZB_TEX_EDGE(2)
#define	G_RM_AA_ZB_TEX_INTER	RM_AA_ZB_TEX_INTER(1)
#define	G_RM_AA_ZB_TEX_INTER2	RM_AA_ZB_TEX_INTER(2)
#define	G_RM_AA_ZB_SUB_SURF	RM_AA_ZB_SUB_SURF(1)
#define	G_RM_AA_ZB_SUB_SURF2	RM_AA_ZB_SUB_SURF(2)
#define	G_RM_AA_ZB_PCL_SURF	RM_AA_ZB_PCL_SURF(1)
#define	G_RM_AA_ZB_PCL_SURF2	RM_AA_ZB_PCL_SURF(2)
#define	G_RM_AA_ZB_OPA_TERR	RM_AA_ZB_OPA_TERR(1)
#define	G_RM_AA_ZB_OPA_TERR2	RM_AA_ZB_OPA_TERR(2)
#define	G_RM_AA_ZB_TEX_TERR	RM_AA_ZB_TEX_TERR(1)
#define	G_RM_AA_ZB_TEX_TERR2	RM_AA_ZB_TEX_TERR(2)
#define	G_RM_AA_ZB_SUB_TERR	RM_AA_ZB_SUB_TERR(1)
#define	G_RM_AA_ZB_SUB_TERR2	RM_AA_ZB_SUB_TERR(2)

#define	G_RM_RA_ZB_OPA_SURF	RM_RA_ZB_OPA_SURF(1)
#define	G_RM_RA_ZB_OPA_SURF2	RM_RA_ZB_OPA_SURF(2)
#define	G_RM_RA_ZB_OPA_DECAL	RM_RA_ZB_OPA_DECAL(1)
#define	G_RM_RA_ZB_OPA_DECAL2	RM_RA_ZB_OPA_DECAL(2)
#define	G_RM_RA_ZB_OPA_INTER	RM_RA_ZB_OPA_INTER(1)
#define	G_RM_RA_ZB_OPA_INTER2	RM_RA_ZB_OPA_INTER(2)

#define	G_RM_AA_OPA_SURF	RM_AA_OPA_SURF(1)
#define	G_RM_AA_OPA_SURF2	RM_AA_OPA_SURF(2)
#define	G_RM_AA_XLU_SURF	RM_AA_XLU_SURF(1)
#define	G_RM_AA_XLU_SURF2	RM_AA_XLU_SURF(2)
#define	G_RM_AA_XLU_LINE	RM_AA_XLU_LINE(1)
#define	G_RM_AA_XLU_LINE2	RM_AA_XLU_LINE(2)
#define	G_RM_AA_DEC_LINE	RM_AA_DEC_LINE(1)
#define	G_RM_AA_DEC_LINE2	RM_AA_DEC_LINE(2)
#define	G_RM_AA_TEX_EDGE	RM_AA_TEX_EDGE(1)
#define	G_RM_AA_TEX_EDGE2	RM_AA_TEX_EDGE(2)
#define	G_RM_AA_SUB_SURF	RM_AA_SUB_SURF(1)
#define	G_RM_AA_SUB_SURF2	RM_AA_SUB_SURF(2)
#define	G_RM_AA_PCL_SURF	RM_AA_PCL_SURF(1)
#define	G_RM_AA_PCL_SURF2	RM_AA_PCL_SURF(2)
#define	G_RM_AA_OPA_TERR	RM_AA_OPA_TERR(1)
#define	G_RM_AA_OPA_TERR2	RM_AA_OPA_TERR(2)
#define	G_RM_AA_TEX_TERR	RM_AA_TEX_TERR(1)
#define	G_RM_AA_TEX_TERR2	RM_AA_TEX_TERR(2)
#define	G_RM_AA_SUB_TERR	RM_AA_SUB_TERR(1)
#define	G_RM_AA_SUB_TERR2	RM_AA_SUB_TERR(2)

#define	G_RM_RA_OPA_SURF	RM_RA_OPA_SURF(1)
#define	G_RM_RA_OPA_SURF2	RM_RA_OPA_SURF(2)

#define	G_RM_ZB_OPA_SURF	RM_ZB_OPA_SURF(1)
#define	G_RM_ZB_OPA_SURF2	RM_ZB_OPA_SURF(2)
#define	G_RM_ZB_XLU_SURF	RM_ZB_XLU_SURF(1)
#define	G_RM_ZB_XLU_SURF2	RM_ZB_XLU_SURF(2)
#define	G_RM_ZB_OPA_DECAL	RM_ZB_OPA_DECAL(1)
#define	G_RM_ZB_OPA_DECAL2	RM_ZB_OPA_DECAL(2)
#define	G_RM_ZB_XLU_DECAL	RM_ZB_XLU_DECAL(1)
#define	G_RM_ZB_XLU_DECAL2	RM_ZB_XLU_DECAL(2)
#define	G_RM_ZB_CLD_SURF	RM_ZB_CLD_SURF(1)
#define	G_RM_ZB_CLD_SURF2	RM_ZB_CLD_SURF(2)
#define	G_RM_ZB_OVL_SURF	RM_ZB_OVL_SURF(1)
#define	G_RM_ZB_OVL_SURF2	RM_ZB_OVL_SURF(2)
#define	G_RM_ZB_PCL_SURF	RM_ZB_PCL_SURF(1)
#define	G_RM_ZB_PCL_SURF2	RM_ZB_PCL_SURF(2)

#define	G_RM_OPA_SURF		RM_OPA_SURF(1)
#define	G_RM_OPA_SURF2		RM_OPA_SURF(2)
#define	G_RM_XLU_SURF		RM_XLU_SURF(1)
#define	G_RM_XLU_SURF2		RM_XLU_SURF(2)
#define	G_RM_CLD_SURF		RM_CLD_SURF(1)
#define	G_RM_CLD_SURF2		RM_CLD_SURF(2)
#define	G_RM_TEX_EDGE		RM_TEX_EDGE(1)
#define	G_RM_TEX_EDGE2		RM_TEX_EDGE(2)
#define	G_RM_PCL_SURF		RM_PCL_SURF(1)
#define	G_RM_PCL_SURF2		RM_PCL_SURF(2)
#define G_RM_ADD       		RM_ADD(1)
#define G_RM_ADD2      		RM_ADD(2)
#define G_RM_NOOP       	RM_NOOP(1)
#define G_RM_NOOP2      	RM_NOOP(2)
#define G_RM_VISCVG    		RM_VISCVG(1)
#define G_RM_VISCVG2    	RM_VISCVG(2)
#define G_RM_OPA_CI         RM_OPA_CI(1)
#define G_RM_OPA_CI2        RM_OPA_CI(2)


#define	G_RM_FOG_SHADE_A	GBL_c1(G_BL_CLR_FOG, G_BL_A_SHADE, G_BL_CLR_IN, G_BL_1MA)
#define	G_RM_FOG_PRIM_A		GBL_c1(G_BL_CLR_FOG, G_BL_A_FOG, G_BL_CLR_IN, G_BL_1MA)
#define	G_RM_PASS		GBL_c1(G_BL_CLR_IN, G_BL_0, G_BL_CLR_IN, G_BL_1)


const char *	GetBlenderModeDescription( u32 mode )
{
	switch ( mode & ~3 )
	{
		case G_RM_AA_ZB_OPA_SURF:			return "G_RM_AA_ZB_OPA_SURF";
		case G_RM_AA_ZB_OPA_SURF2:			return "G_RM_AA_ZB_OPA_SURF2";
		case G_RM_AA_ZB_XLU_SURF:			return "G_RM_AA_ZB_XLU_SURF";
		case G_RM_AA_ZB_XLU_SURF2:			return "G_RM_AA_ZB_XLU_SURF2";
		case G_RM_AA_ZB_OPA_DECAL:			return "G_RM_AA_ZB_OPA_DECAL";
		case G_RM_AA_ZB_OPA_DECAL2:			return "G_RM_AA_ZB_OPA_DECAL2";
		case G_RM_AA_ZB_XLU_DECAL:			return "G_RM_AA_ZB_XLU_DECAL";
		case G_RM_AA_ZB_XLU_DECAL2:			return "G_RM_AA_ZB_XLU_DECAL2";
		case G_RM_AA_ZB_OPA_INTER:			return "G_RM_AA_ZB_OPA_INTER";
		case G_RM_AA_ZB_OPA_INTER2:			return "G_RM_AA_ZB_OPA_INTER2";
		case G_RM_AA_ZB_XLU_INTER:			return "G_RM_AA_ZB_XLU_INTER";
		case G_RM_AA_ZB_XLU_INTER2:			return "G_RM_AA_ZB_XLU_INTER2";
		case G_RM_AA_ZB_XLU_LINE:			return "G_RM_AA_ZB_XLU_LINE";
		case G_RM_AA_ZB_XLU_LINE2:			return "G_RM_AA_ZB_XLU_LINE2";
		case G_RM_AA_ZB_DEC_LINE:			return "G_RM_AA_ZB_DEC_LINE";
		case G_RM_AA_ZB_DEC_LINE2:			return "G_RM_AA_ZB_DEC_LINE2";
		case G_RM_AA_ZB_TEX_EDGE:			return "G_RM_AA_ZB_TEX_EDGE";
		case G_RM_AA_ZB_TEX_EDGE2:			return "G_RM_AA_ZB_TEX_EDGE2";
		case G_RM_AA_ZB_TEX_INTER:			return "G_RM_AA_ZB_TEX_INTER";
		case G_RM_AA_ZB_TEX_INTER2:			return "G_RM_AA_ZB_TEX_INTER2";
		case G_RM_AA_ZB_SUB_SURF:			return "G_RM_AA_ZB_SUB_SURF";
		case G_RM_AA_ZB_SUB_SURF2:			return "G_RM_AA_ZB_SUB_SURF2";
		case G_RM_AA_ZB_PCL_SURF:			return "G_RM_AA_ZB_PCL_SURF";
		case G_RM_AA_ZB_PCL_SURF2:			return "G_RM_AA_ZB_PCL_SURF2";
		case G_RM_AA_ZB_OPA_TERR:			return "G_RM_AA_ZB_OPA_TERR";
		case G_RM_AA_ZB_OPA_TERR2:			return "G_RM_AA_ZB_OPA_TERR2";
		case G_RM_AA_ZB_TEX_TERR:			return "G_RM_AA_ZB_TEX_TERR";
		case G_RM_AA_ZB_TEX_TERR2:			return "G_RM_AA_ZB_TEX_TERR2";
		case G_RM_AA_ZB_SUB_TERR:			return "G_RM_AA_ZB_SUB_TERR";
		case G_RM_AA_ZB_SUB_TERR2:			return "G_RM_AA_ZB_SUB_TERR2";
		case G_RM_RA_ZB_OPA_SURF:			return "G_RM_RA_ZB_OPA_SURF";
		case G_RM_RA_ZB_OPA_SURF2:			return "G_RM_RA_ZB_OPA_SURF2";
		case G_RM_RA_ZB_OPA_DECAL:			return "G_RM_RA_ZB_OPA_DECAL";
		case G_RM_RA_ZB_OPA_DECAL2:			return "G_RM_RA_ZB_OPA_DECAL2";
		case G_RM_RA_ZB_OPA_INTER:			return "G_RM_RA_ZB_OPA_INTER";
		case G_RM_RA_ZB_OPA_INTER2:			return "G_RM_RA_ZB_OPA_INTER2";
		case G_RM_AA_OPA_SURF:				return "G_RM_AA_OPA_SURF";
		case G_RM_AA_OPA_SURF2:				return "G_RM_AA_OPA_SURF2";
		case G_RM_AA_XLU_SURF:				return "G_RM_AA_XLU_SURF";
		case G_RM_AA_XLU_SURF2:				return "G_RM_AA_XLU_SURF2";
		case G_RM_AA_XLU_LINE:				return "G_RM_AA_XLU_LINE";
		case G_RM_AA_XLU_LINE2:				return "G_RM_AA_XLU_LINE2";
		case G_RM_AA_DEC_LINE:				return "G_RM_AA_DEC_LINE";
		case G_RM_AA_DEC_LINE2:				return "G_RM_AA_DEC_LINE2";
		case G_RM_AA_TEX_EDGE:				return "G_RM_AA_TEX_EDGE";
		case G_RM_AA_TEX_EDGE2:				return "G_RM_AA_TEX_EDGE2";
		case G_RM_AA_SUB_SURF:				return "G_RM_AA_SUB_SURF";
		case G_RM_AA_SUB_SURF2:				return "G_RM_AA_SUB_SURF2";
		case G_RM_AA_PCL_SURF:				return "G_RM_AA_PCL_SURF";
		case G_RM_AA_PCL_SURF2:				return "G_RM_AA_PCL_SURF2";
		case G_RM_AA_OPA_TERR:				return "G_RM_AA_OPA_TERR";
		case G_RM_AA_OPA_TERR2:				return "G_RM_AA_OPA_TERR2";
		case G_RM_AA_TEX_TERR:				return "G_RM_AA_TEX_TERR";
		case G_RM_AA_TEX_TERR2:				return "G_RM_AA_TEX_TERR2";
		case G_RM_AA_SUB_TERR:				return "G_RM_AA_SUB_TERR";
		case G_RM_AA_SUB_TERR2:				return "G_RM_AA_SUB_TERR2";
		case G_RM_RA_OPA_SURF:				return "G_RM_RA_OPA_SURF";
		case G_RM_RA_OPA_SURF2:				return "G_RM_RA_OPA_SURF2";
		case G_RM_ZB_OPA_SURF:				return "G_RM_ZB_OPA_SURF";
		case G_RM_ZB_OPA_SURF2:				return "G_RM_ZB_OPA_SURF2";
		case G_RM_ZB_XLU_SURF:				return "G_RM_ZB_XLU_SURF";
		case G_RM_ZB_XLU_SURF2:				return "G_RM_ZB_XLU_SURF2";
		case G_RM_ZB_OPA_DECAL:				return "G_RM_ZB_OPA_DECAL";
		case G_RM_ZB_OPA_DECAL2:			return "G_RM_ZB_OPA_DECAL2";
		case G_RM_ZB_XLU_DECAL:				return "G_RM_ZB_XLU_DECAL";
		case G_RM_ZB_XLU_DECAL2:			return "G_RM_ZB_XLU_DECAL2";
		case G_RM_ZB_CLD_SURF:				return "G_RM_ZB_CLD_SURF";
		case G_RM_ZB_CLD_SURF2:				return "G_RM_ZB_CLD_SURF2";
		case G_RM_ZB_OVL_SURF:				return "G_RM_ZB_OVL_SURF";
		case G_RM_ZB_OVL_SURF2:				return "G_RM_ZB_OVL_SURF2";
		case G_RM_ZB_PCL_SURF:				return "G_RM_ZB_PCL_SURF";
		case G_RM_ZB_PCL_SURF2:				return "G_RM_ZB_PCL_SURF2";
		case G_RM_OPA_SURF:					return "G_RM_OPA_SURF";
		case G_RM_OPA_SURF2:				return "G_RM_OPA_SURF2";
		case G_RM_XLU_SURF:					return "G_RM_XLU_SURF";
		case G_RM_XLU_SURF2:				return "G_RM_XLU_SURF2";
		case G_RM_CLD_SURF:					return "G_RM_CLD_SURF";
		case G_RM_CLD_SURF2:				return "G_RM_CLD_SURF2";
		case G_RM_TEX_EDGE:					return "G_RM_TEX_EDGE";
		case G_RM_TEX_EDGE2:				return "G_RM_TEX_EDGE2";
		case G_RM_PCL_SURF:					return "G_RM_PCL_SURF";
		case G_RM_PCL_SURF2:				return "G_RM_PCL_SURF2";
		case G_RM_ADD:						return "G_RM_ADD";
		case G_RM_ADD2:						return "G_RM_ADD2";
		case G_RM_NOOP:						return "G_RM_NOOP";
		//case G_RM_NOOP2:					return "G_RM_NOOP2";
		case G_RM_VISCVG:					return "G_RM_VISCVG";
		case G_RM_VISCVG2:					return "G_RM_VISCVG2";
		case G_RM_OPA_CI:					return "G_RM_OPA_CI";
		case G_RM_OPA_CI2:					return "G_RM_OPA_CI2";
		case G_RM_FOG_SHADE_A:				return "G_RM_FOG_SHADE_A";
		case G_RM_FOG_PRIM_A:				return "G_RM_FOG_PRIM_A";
	//	case G_RM_PASS:						return "G_RM_PASS";

		default:
			{
				switch ( mode & 0xff000000 )
				{
				case G_RM_FOG_SHADE_A:			return "G_RM_FOG_SHADE_A";
				case G_RM_FOG_PRIM_A:			return "G_RM_FOG_PRIM_A";
				case G_RM_PASS:					return "G_RM_PASS";
				}
				break;
			}
	}

	static char buffer[32];
	sprintf( buffer, "Unknown: %08x", mode  );
	return buffer;
}

void DLDebug_DumpRDPOtherMode(const RDP_OtherMode & mode)
{
	if (gDisplayListFile != NULL)
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
										sc_szBlClr[dwM1A_1], sc_szBlA1[dwM1B_1], sc_szBlClr[dwM2A_1], sc_szBlA2[dwM2B_1],
										sc_szBlClr[dwM1A_2], sc_szBlA1[dwM1B_2], sc_szBlClr[dwM2A_2], sc_szBlA2[dwM2B_2]);
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

		u32 c1_mode = (mode.blender & 0xff00) << 16;
		u32 c2_mode = (mode.blender & 0x00ff) << 16;

		DL_PF( "      %s", GetBlenderModeDescription( c1_mode ) );
		DL_PF( "      %s", GetBlenderModeDescription( c2_mode ) );
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

		gDisplayListFile = fopen( szFilePath, "w" );
		if (gDisplayListFile != NULL)
			DBGConsole_Msg(0, "RDP: Dumping Display List as %s", szFilePath);
		else
			DBGConsole_Msg(0, "RDP: Couldn't create dumpfile %s", szFilePath);

		DLParser_DumpTaskInfo( pTask );

		// Clear flag as we're done
		gDumpNextDisplayList = false;
	}
}

void DLDebug_DumpNextDisplayList()
{
	gDumpNextDisplayList = true;
}

#endif // DAEDALUS_DEBUG_DISPLAYLIST
