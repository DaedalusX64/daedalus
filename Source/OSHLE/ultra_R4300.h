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

#ifndef __ULTRA_R4300_H__
#define __ULTRA_R4300_H__

#define	KUBASE		0
#define	KUSIZE		0x80000000
#define	K0BASE		0x80000000
#define	K0SIZE		0x20000000
#define	K1BASE		0xA0000000
#define	K1SIZE		0x20000000
#define	K2BASE		0xC0000000
#define	K2SIZE		0x20000000

#define SIZE_EXCVEC	0x80
#define	UT_VEC		K0BASE
#define	R_VEC		(K1BASE+0x1fc00000)
#define	XUT_VEC		(K0BASE+0x80)
#define	ECC_VEC		(K0BASE+0x100)
#define	E_VEC		0x80000180

#define	K0_TO_K1(x)		((u32)(x)|0xA0000000)
#define	K1_TO_K0(x)		((u32)(x)&0x9FFFFFFF)
#define	K0_TO_PHYS(x)	((u32)(x)&0x1FFFFFFF)
#define	K1_TO_PHYS(x)	((u32)(x)&0x1FFFFFFF)
#define	KDM_TO_PHYS(x)	((u32)(x)&0x1FFFFFFF)
#define	PHYS_TO_K0(x)	((u32)(x)|0x80000000)
#define	PHYS_TO_K1(x)	((u32)(x)|0xA0000000)

#define	IS_KSEG0(x)		((u32)(x) >= K0BASE && (u32)(x) < K1BASE)
#define	IS_KSEG1(x)		((u32)(x) >= K1BASE && (u32)(x) < K2BASE)
#define	IS_KSEGDM(x)	((u32)(x) >= K0BASE && (u32)(x) < K2BASE)
#define	IS_KSEG2(x)		((u32)(x) >= K2BASE && (u32)(x) < KPTE_SHDUBASE)
#define	IS_KPTESEG(x)	((u32)(x) >= KPTE_SHDUBASE)
#define	IS_KUSEG(x)		((u32)(x) < K0BASE)

#define	NTLBENTRIES	31

#define	TLBHI_VPN2MASK		0xffffe000
#define	TLBHI_VPN2SHIFT		13
#define	TLBHI_PIDMASK		0xff
#define	TLBHI_PIDSHIFT		0
#define	TLBHI_NPID			255

#define	TLBLO_PFNMASK		0x3fffffc0
#define	TLBLO_PFNSHIFT		6
#define	TLBLO_CACHMASK		0x38
#define TLBLO_CACHSHIFT		3
#define TLBLO_UNCACHED		0x10
#define TLBLO_NONCOHRNT		0x18
#define TLBLO_EXLWR			0x28
#define	TLBLO_D				0x4
#define	TLBLO_V				0x2
#define	TLBLO_G				0x1

#define	TLBINX_PROBE		0x80000000
#define	TLBINX_INXMASK		0x3f
#define	TLBINX_INXSHIFT		0

#define	TLBRAND_RANDMASK	0x3f
#define	TLBRAND_RANDSHIFT	0

#define	TLBWIRED_WIREDMASK	0x3f

#define	TLBCTXT_BASEMASK	0xff800000
#define	TLBCTXT_BASESHIFT	23
#define TLBCTXT_BASEBITS	9

#define	TLBCTXT_VPNMASK		0x7ffff0
#define	TLBCTXT_VPNSHIFT	4

#define TLBPGMASK_4K		0x0
#define TLBPGMASK_16K		0x6000
#define TLBPGMASK_64K		0x1e000
#define TLBPGMASK_256K		0x0007e000
#define TLBPGMASK_1M		0x001fe000
#define TLBPGMASK_4M		0x007fe000
#define TLBPGMASK_16M		0x01ffe000

#define	SR_CUMASK	0xf0000000

#define	SR_CU3		0x80000000
#define	SR_CU2		0x40000000
#define	SR_CU1		0x20000000
#define	SR_CU0		0x10000000
#define	SR_RP		0x08000000
#define	SR_FR		0x04000000
#define	SR_RE		0x02000000
#define	SR_ITS		0x01000000
#define	SR_BEV		0x00400000
#define	SR_TS		0x00200000
#define	SR_SR		0x00100000
#define	SR_CH		0x00040000
#define	SR_CE		0x00020000
#define	SR_DE		0x00010000

#define	SR_IMASK	0x0000ff00
#define	SR_IMASK8	0x00000000
#define	SR_IMASK7	0x00008000
#define	SR_IMASK6	0x0000c000
#define	SR_IMASK5	0x0000e000
#define	SR_IMASK4	0x0000f000
#define	SR_IMASK3	0x0000f800
#define	SR_IMASK2	0x0000fc00
#define	SR_IMASK1	0x0000fe00
#define	SR_IMASK0	0x0000ff00

#define	SR_IBIT8	0x00008000
#define	SR_IBIT7	0x00004000
#define	SR_IBIT6	0x00002000
#define	SR_IBIT5	0x00001000
#define	SR_IBIT4	0x00000800
#define	SR_IBIT3	0x00000400
#define	SR_IBIT2	0x00000200
#define	SR_IBIT1	0x00000100

#define	SR_IMASKSHIFT	8

#define	SR_KX			0x00000080
#define	SR_SX			0x00000040
#define	SR_UX			0x00000020
#define	SR_KSU_MASK		0x00000018
#define	SR_KSU_USR		0x00000010
#define	SR_KSU_SUP		0x00000008
#define	SR_KSU_KER		0x00000000
#define SR_EXL_OR_ERL	0x00000006
#define	SR_ERL			0x00000004
#define	SR_EXL			0x00000002
#define	SR_IE			0x00000001

#define	CAUSE_BD		0x80000000
#define	CAUSE_CEMASK	0x30000000
#define	CAUSE_CESHIFT	28

#define	CAUSE_IP8	0x00008000
#define	CAUSE_IP7	0x00004000
#define	CAUSE_IP6	0x00002000
#define	CAUSE_IP5	0x00001000
#define	CAUSE_IP4	0x00000800
#define	CAUSE_IP3	0x00000400
#define	CAUSE_SW2	0x00000200
#define	CAUSE_SW1	0x00000100

#define	CAUSE_IPMASK	0x0000FF00
#define	CAUSE_IPSHIFT	8

#define NOT_CAUSE_EXCMASK	0xFFFFFF83
#define	CAUSE_EXCMASK		0x0000007C

#define	CAUSE_EXCSHIFT	2

#define EXC_INT		0
#define EXC_MOD		4
#define EXC_RMISS	8
#define EXC_WMISS	12
#define EXC_RADE	16
#define EXC_WADE	20
#define EXC_IBE		24
#define EXC_DBE		28
#define EXC_SYSCALL 32
#define EXC_BREAK	36
#define EXC_II		40
#define EXC_CPU		44
#define EXC_OV		48
#define EXC_TRAP	52
#define EXC_VCEI	56
#define EXC_FPE		60
#define EXC_WATCH	92
#define EXC_VCED	124


#define REG_r0 0x00
#define REG_at 0x01
#define REG_v0 0x02
#define REG_v1 0x03
#define REG_a0 0x04
#define REG_a1 0x05
#define REG_a2 0x06
#define REG_a3 0x07
#define REG_t0 0x08
#define REG_t1 0x09
#define REG_t2 0x0a
#define REG_t3 0x0b
#define REG_t4 0x0c
#define REG_t5 0x0d
#define REG_t6 0x0e
#define REG_t7 0x0f
#define REG_s0 0x10
#define REG_s1 0x11
#define REG_s2 0x12
#define REG_s3 0x13
#define REG_s4 0x14
#define REG_s5 0x15
#define REG_s6 0x16
#define REG_s7 0x17
#define REG_t8 0x18
#define REG_t9 0x19
#define REG_k0 0x1a
#define REG_k1 0x1b
#define REG_gp 0x1c
#define REG_sp 0x1d
#define REG_s8 0x1e
#define REG_ra 0x1f

#define	C0_INX			0
#define	C0_RAND			1
#define	C0_ENTRYLO0		2
#define	C0_ENTRYLO1		3
#define	C0_CONTEXT		4
#define	C0_PAGEMASK		5
#define	C0_WIRED		6
#define	C0_BADVADDR		8
#define	C0_COUNT		9
#define	C0_ENTRYHI		10
#define	C0_SR			12
#define	C0_CAUSE		13
#define	C0_EPC			14
#define	C0_PRID			15
#define	C0_COMPARE		11
#define	C0_CONFIG		16
#define	C0_LLADDR		17
#define	C0_WATCHLO		18
#define	C0_WATCHHI		19
#define	C0_ECC			26
#define	C0_CACHE_ERR	27
#define	C0_TAGLO		28
#define	C0_TAGHI		29
#define	C0_ERROR_EPC	30

#define FPCSR_FS		0x01000000
#define	FPCSR_C			0x00800000
#define	FPCSR_CE		0x00020000
#define	FPCSR_CV		0x00010000
#define	FPCSR_CZ		0x00008000
#define	FPCSR_CO		0x00004000
#define	FPCSR_CU		0x00002000
#define	FPCSR_CI		0x00001000
#define	FPCSR_EV		0x00000800
#define	FPCSR_EZ		0x00000400
#define	FPCSR_EO		0x00000200
#define	FPCSR_EU		0x00000100
#define	FPCSR_EI		0x00000080
#define	FPCSR_FV		0x00000040
#define	FPCSR_FZ		0x00000020
#define	FPCSR_FO		0x00000010
#define	FPCSR_FU		0x00000008
#define	FPCSR_FI		0x00000004
#define	FPCSR_RM_MASK	0x00000003
#define	FPCSR_RM_RN		0x00000000
#define	FPCSR_RM_RZ		0x00000001
#define	FPCSR_RM_RP		0x00000002
#define	FPCSR_RM_RM		0x00000003


#endif // __ULTRA_R4300_H__
