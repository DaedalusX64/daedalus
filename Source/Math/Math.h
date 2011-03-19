#ifndef DAEDMATHS_H__
#define DAEDMATHS_H__

#include <math.h>

// VFPU Math :D
//
// Todo : Move to SysPSP ?
//
// Note : Matrix math => check Matrix4x4.cpp
//

/* Cycles

- sinf(v) = 0.389418, cycles: 856
- vfpu_sinf(v) = 0.389418, cycles: 160
 
- cosf(v) = 0.921061, cycles: 990
- vfpu_cosf(v) = 0.921061, cycles: 154
 
- acosf(v) = 1.159279, cycles: 1433
- vfpu_acosf(v) = 1.159280, cycles: 107
 
- coshf(v) = 1.081072, cycles: 1885
- vfpu_coshf(v) = 1.081072, cycles: 246
 
- powf(v, v) = 0.693145, cycles: 3488
- vfpu_powf(v, v) = 0.693145, cycles: 412

- fabsf(v) = 0.400000, cycles: 7
- vfpu_fabsf(v) = 0.400000, cycles: 93	<== Slower on VFPU !
 
- sqrtf(v) = 0.632456, cycles: 40
- vfpu_sqrtf(v) = 0.632455, cycles: 240	<== Slower on VFPU !

*/

// Interesting note, if we don't make thse funcs inline, we'll get compiling errors.. weird
// Its because its a .h file, but inlining these short functions is ok. Means also that they take no space unless used.

//#define DOUBLE_CONVERSION // Define to use double and other related conversions.
#define PI   3.141592653589793f

//Do ACOS(x) ACOS(y) and save in 2D vector on VFPU //Corn
inline void vfpu_Acos_2Dvec(float x, float y, float *s) {
	__asm__ volatile (
		"mtv      %1, S000\n"
		"mtv      %2, S001\n"
		"vasin.p  C100, C000\n"
		"vocp.p	  C000, C100\n"
		"vmul.p	  C000, C100[1/2,1/2], C000\n"
		"sv.s     S000, 0 + %0\n"	//save result.x
		"sv.s     S001, 4 + %0\n"	//save result.y
		: "=m"(*s): "r"(x), "r"(y) : "memory");
}

//Do SIN/COS in one go on VFPU //Corn
inline void vfpu_sincos(float r, float *s, float *c) {
	__asm__ volatile (
		"mtv      %2, S002\n"
		"vcst.s   S003, VFPU_2_PI\n"
		"vmul.s   S002, S002, S003\n"
		"vrot.p   C000, S002, [s, c]\n"
		"mfv      %0, S000\n"
		"mfv      %1, S001\n"
	: "=r"(*s), "=r"(*c): "r"(r));
}

inline float vfpu_randf(float min, float max) {
    float result;
    __asm__ volatile (
		"mtv      %1, S000\n"
        "mtv      %2, S001\n"
        "vsub.s   S001, S001, S000\n"
        "vrndf1.s S002\n"
        "vone.s	  S003\n"
        "vsub.s   S002, S002, S003\n"
        "vmul.s   S001, S002, S001\n"
        "vadd.s   S000, S000, S001\n"
        "mfv      %0, S000\n"
        : "=r"(result) : "r"(min), "r"(max));
    return result;
}

//VFPU 4D Dot product //Corn
inline float vfpu_dot_4Dvec(const float x, const float y, const float z, const float w, const float a, const float b, const float c, const float d)
{
	float result;
	__asm__ volatile (
       "mtv   %1, S000\n"
       "mtv   %2, S001\n"
       "mtv   %3, S002\n"
       "mtv   %4, S003\n"
       "mtv   %5, S010\n"
       "mtv   %6, S011\n"
       "mtv   %7, S012\n"
       "mtv   %8, S013\n"
       "vdot.q S020, C000, C010\n"
       "mfv   %0, S020\n"
	   : "=r"(result)
	   : "r"(x), "r"(y), "r"(z), "r"(w),
	     "r"(a), "r"(b), "r"(c), "r"(d));
	return result;
}

inline float vfpu_dot_3Dvec(const float x, const float y, const float z, const float a, const float b, const float c)
{
	float result;
	__asm__ volatile (
       "mtv   %1, S000\n"
       "mtv   %2, S001\n"
       "mtv   %3, S002\n"
       "mtv   %4, S010\n"
       "mtv   %5, S011\n"
       "mtv   %6, S012\n"
       "vdot.t S020, C000, C010\n"
       "mfv   %0, S020\n"
	   : "=r"(result)
	   : "r"(x), "r"(y), "r"(z),
	     "r"(a), "r"(b), "r"(c));
	return result;
}

//VFPU 3D Normalize vector //Corn
inline void vfpu_norm_3Dvec(float *x, float *y, float *z)
{
   __asm__ volatile (
       "mtv   %0, S000\n"
       "mtv   %1, S001\n"
       "mtv   %2, S002\n"
       "vdot.t S010, C000, C000\n"
       "vrsq.s S010, S010\n"
       "vscl.t C000, C000, S010\n"
       "mfv   %0, S000\n"
       "mfv   %1, S001\n"
       "mfv   %2, S002\n"
       : "+r"(*x), "+r"(*y), "+r"(*z));
}

#if 1	//0=fast, 1=original //Corn
inline float vfpu_invSqrt(float x)
{
//	return 1.0f/sqrtf(x);

	float result;
	__asm__ volatile (
		"mtv		%0, S000\n"
		"vrsq.s		S000, S000\n"
		"mfv		%0, S000\n"
	: "=r"(result): "r"(x));
	return result;
}
#else
inline float vfpu_invSqrt(float x)	//Trick using int/float to get 1/SQRT() fast on FPU/CPU //Corn
{
 	union
		{
		int itg;
		float flt;
		} c;
	c.flt = x;
	c.itg = 0x5f375a86 - (c.itg >> 1);
	return 0.5f * c.flt *(3.0f - x * c.flt * c.flt );
}
#endif

inline float vfpu_cosf(float rad) {
    float result;
    __asm__ volatile (
        "mtv     %1, S000\n"
        "vcst.s  S001, VFPU_2_PI\n"
        "vmul.s  S000, S000, S001\n"
        "vcos.s  S000, S000\n"
        "mfv     %0, S000\n"
        : "=r"(result) : "r"(rad));
    return result;
}

inline float vfpu_sinf(float rad) {
    float result;
    __asm__ volatile (
        "mtv     %1, S000\n"
        "vcst.s  S001, VFPU_2_PI\n"
        "vmul.s  S000, S000, S001\n"
        "vsin.s  S000, S000\n"
        "mfv     %0, S000\n"
        : "=r"(result) : "r"(rad));
    return result;
}

inline float vfpu_round(float x)
{
	float result;

	__asm__ volatile (
		"mtv      %1, S000\n"
		"vf2in.s  S000, S000, 0\n"
		"vi2f.s	S000, S000, 0\n"
		"mfv      %0, S000\n"
	: "=r"(result) : "r"(x));
	
	return result;
}

inline void vfpu_N64_2_PSP(float *Pcoord, const float *Ncoord, const float *Pscale, const float *Ptrans)
{
	__asm__ volatile (
		"lv.s     S000, 0 + %1\n"	//load Ncord.x
		"lv.s     S001, 4 + %1\n"	//load Ncord.y
		"vf2in.p  C000, C000, 0\n"	//conv Ncord -> int
		"lv.s     S010, 0 + %2\n"	//load Pscale.x
		"lv.s     S011, 4 + %2\n"	//load Pscale.y
		"vi2f.p	  C000, C000, 0\n"	//conv Ncord -> float
		"lv.s     S020, 0 + %3\n"	//load Ptrans.x
		"lv.s     S021, 4 + %3\n"	//load Ptrans.y
		"vmul.p	  C000, C000, C010\n"	//Ncord * Pscale
		"vadd.p   C000, C000, C020\n"	// + Ptrans
		"vf2in.p  C000, C000, 0\n"	//conv result -> int
		"vi2f.p	  C000, C000, 0\n"	//conv result -> float
		"sv.s     S000, 0 + %0\n"	//save result.x
		"sv.s     S001, 4 + %0\n"	//save result.y
		: "=m"(*Pcoord) : "m"(*Ncoord), "m"(*Pscale), "m"(*Ptrans) : "memory" );
}

/*
inline float vfpu_fmaxf(float x, float y) {
	float result;
	__asm__ volatile (
		"mtv      %1, S000\n"
		"mtv      %2, S001\n"
		"vmax.s   S002, S000, S001\n"
		"mfv      %0, S002\n"
	: "=r"(result) : "r"(x), "r"(y));
	return result;
}

inline float vfpu_fminf(float x, float y) {
	float result;
	__asm__ volatile (
		"mtv      %1, S000\n"
		"mtv      %2, S001\n"
		"vmin.s   S002, S000, S001\n"
		"mfv      %0, S002\n"
	: "=r"(result) : "r"(x), "r"(y));
	return result;
}
*/
inline float vfpu_powf(float x, float y) {
	float result;
	// result = exp2f(y * log2f(x));
	__asm__ volatile (
		"mtv      %1, S000\n"
		"mtv      %2, S001\n"
		"vlog2.s  S001, S001\n"
		"vmul.s   S000, S000, S001\n"
		"vexp2.s  S000, S000\n"
		"mfv      %0, S000\n"
	: "=r"(result) : "r"(x), "r"(y));
	return result;
}
/*

//Below Function taken from PGE - Phoenix Game Engine - Greets InsertWittyName !
inline float vfpu_abs(float x) {
    float result;

	__asm__ volatile (
		"mtv      %1, S000\n"
		"vabs.s   S000, S000\n"
		"mfv      %0, S000\n"
	: "=r"(result) : "r"(x));

	return result;
}

inline float vfpu_sqrtf(float x) {
	float result;
	__asm__ volatile (
		"mtv     %1, S000\n"
		"vsqrt.s S000, S000\n"
		"mfv     %0, S000\n"
	: "=r"(result) : "r"(x));
	return result;
}
*/


//*****************************************************************************
//FPU Math :D
//*****************************************************************************
/*
sqrtf and fabsf are alot slower on the vfpuv, so let's do em on the fpu.
Check above notes for cycles/comparison
*/

#if 1	//0=fast, 1=original
inline float pspFpuSqrt(float fs)
{
	return (__builtin_allegrex_sqrt_s(fs));
}
#else
inline float pspFpuSqrt(float fs)
{
	union
        {
        int tmp;
        float fpv;
        } uni;
	uni.fpv = fs;
	uni.tmp = (1<<29) + (uni.tmp >> 1) - (1<<22) - 311296;
	return(uni.fpv);
}
#endif

#if 1	//0=fast, 1=original //Corn
inline float pspFpuAbs(float fs)
{
	register float fd;
	asm (
		"abs.s %0, %1\n"
		: "=f"(fd)
		: "f"(fs)
	);
	return (fd);
}
#else
inline float pspFpuAbs(float fs)
{
	union		//This could be slower than the real deal absf() due to mem access?
        {
        int tmp;
        float fpv;
        } uni;
	uni.fpv = fs;
	uni.tmp = uni.tmp & 0x7FFFFFFF;
	return(uni.fpv);
}
#endif

//*****************************************************************************
//
//*****************************************************************************
// Misc

inline int pspFpuFloor(float fs)
{
	return (__builtin_allegrex_floor_w_s(fs));
}

inline int pspFpuCeil(float fs)
{
	return (__builtin_allegrex_ceil_w_s(fs));
}

inline int pspFpuTrunc(float fs)
{
	return (__builtin_allegrex_trunc_w_s(fs));
}

inline int pspFpuRound(float fs)
{
	return (__builtin_allegrex_round_w_s(fs));
}

// I'm not sure if the vfpu_fmaxf and vfpu_fminf it's slwoer or faster than fpu version
// Let's do regardless to avoid overhead.
/*
inline float pspFpuMax(float fs1, float fs2)
{
	register float fd;
	fd = (fs1 > fs2) ? fs1 : fs2;
	return (fd);
}

inline float pspFpuMin(float fs1, float fs2)
{
	register float fd;
	fd = (fs1 < fs2) ? fs1 : fs2;
	return (fd);
}
*/
inline int pspFpuIsNaN(float f)
{
	int v;
	asm (
		".set push\n"		
		".set noreorder\n"	
		"lui %0, 0x807F\n"		//
		"mfc1 $8, %1\n"			// t0 = f
		"ori %0, %0, 0xFFFF\n"		// v  = 0x807FFFFF
		"sll $9, $8, 1\n"		// t1 = t0<<1
		"and %0, %0, $8\n"		// v  = v & t0
		"srl $9, $9, 24\n"		// t1 = t1>>24
		"sll $8, $8, 9\n"		// t0 = t0<<9
		"sltiu $9, $9, 0x00FF\n"	// t1 = (t1<0xFF)
		"movz %0, $0, $8\n"		// v  = (t0==0) ? 0 : v		if (frac==0) is not NaN
		"movn %0, $0, $9\n"		// v  = (t1!=0) ? 0 : v		if (exp!=0xFF) is not NAN
		".set pop\n"		
		: "=r"(v)
		: "f"(f)
		: "$8", "$9"
	);
	return (v);
}

//Speedy random number 1 - (2^32)-1 //Corn
inline u32 pspFastRand()
{
	static u32 IO_RAND=0x12345678;
	IO_RAND = (IO_RAND << 1) | (((IO_RAND >> 31) ^ (IO_RAND >> 28)) & 1);
	return IO_RAND;
}


#ifdef DOUBLE_CONVERSION

//Yoyo games glog (Mike Dailly), modified Corn
//Convert Double (float) to 32bit signed integer
inline s32 Double2Int( f64 *d )
{
	union
	{
		f64 d;
		u32 pD[2];
	}uni = {*d};

	u32	s = uni.pD[1] & 0x80000000;
	s32	exp = ((uni.pD[1] >> 20) & 0x7ff) - 1023;		// get exponent

	if( exp < 0 ) return 0;				// ONLY a fraction

	s64 t0 = *((s64*)uni.pD) & 0x000fffffffffffffLL;
	t0 |= 0x0010000000000000LL;
	s32 shift = 52 - exp;
	t0 = t0 >> shift;
	if( s != 0 ) t0 = -t0;
	return (s32)t0;
}

//ASM reg defs
#define zero	$0	/* wired zero */
#define at	$1	/* assembler temp */
#define v0	$2	/* return value */
#define v1	$3
#define a0	$4	/* argument registers */
#define a1	$5
#define a2	$6
#define a3	$7
#define t0	$8	/* caller saved */
#define t1	$9
#define t2	$10
#define t3	$11
#define t4	$12
#define t5	$13
#define t6	$14
#define t7	$15
#define s0	$16	/* callee saved */
#define s1	$17
#define s2	$18
#define s3	$19
#define s4	$20
#define s5	$21
#define s6	$22
#define s7	$23
#define t8	$24	/* code generator */
#define t9	$25
#define k0	$26	/* kernel temporary */
#define k1	$27
#define gp	$28	/* global pointer */
#define sp	$29	/* stack pointer */
#define fp	$30	/* frame pointer */
#define ra	$31	/* return address */

#define fv0	$f0
#define fv1	$f1
#define ft0	$f2
#define ft1	$f3
#define ft2	$f4
#define ft3	$f5
#define ft4	$f6
#define ft5	$f7
#define ft6	$f8
#define ft7	$f9
#define ft8	$f10
#define ft9	$f11
#define fa0	$f12
#define fa1	$f13
#define fa2	$f14
#define fa3	$f15
#define fa4	$f16
#define fa5	$f17
#define fa6	$f18
#define fa7	$f19
#define fs0	$f20
#define fs1	$f21
#define fs2	$f22
#define fs3	$f23
#define fs4	$f24
#define fs5	$f25
#define fs6	$f26
#define fs7	$f27
#define fs8	$f28
#define fs9	$f29
#define fs10	$f30
#define fs11	$f31

//Conversions from minpsp //Corn
inline void pspFpuFloatToDouble(f64 *r, f32 f)
{
	asm (
	".set push\n"		
	".set noreorder\n"	
	"mfc1	t0, %1\n"				/* t0 = fa0 */
	"ext	t1, t0, 23, 8\n"			/* t1 = (t0 >> 23) & 0xFF */
	"beq	t1, zero, ftod_denormal\n"		/* if (t1==0) goto ftod_denormal */
	"addiu	v0, t1, (-0x7F+0x3FF)\n"		/* v0 = t1 - 0x7F + 0x3FF */
	"xori	t1, t1, 0xFF\n"			/* t1 = t1 ^ 0xFF */
	"li	v1, 0x7FF\n"			/* v1 = 0x7FF */
	"movz	v0, v1, t1\n"			/* v0 = (t1==0) ? v1 : v0 */
	"ext	v1, t0,  3, 20\n"			/* v1 = (t0 >> 3 ) & 0x00FFFFF */
	"ins	v1, v0, 20, 11\n"			/* v1 = (v1 & 0x800FFFFF) | ((v0<<20) & 0x7FF00000) */
	"sll	%0 + 0, t0, 29\n"			/* v0 = (t0 << 29) */
	"srl	t0, t0, 31\n"			/* t0 = (t0 >> 31) & 1 */
	"jr	ra\n"				/* return */
	"ins	%0 + 4, t0, 31, 1\n"			/* v1 = (v1 & 0x7FFFFFFF) | ((t0<<31) & 0x80000000) */

"ftod_denormal:\n"
	"sll	v0, t0, 9\n"			/* v0 = t0 << 9 */
	"beql	v0, zero, ftod_zero\n"		/* if (v0==0) goto ftod_zero */
	"move	v1, zero\n"			/* v1 = 0 */
	"li	v1, 0x380\n"			/* v1 = 0x380 */
	"clz	t1, v0\n"				/* t1 = clz(v0) */
	"subu	v0, v1, t1\n"			/* v0 = v1 - v0 = 0x380 - clz(t1) */
	"sllv	t1, t0, t1\n"			/* t1 = t0 << t1 */
	"ext	v1, t1,  2, 20\n"			/* v1 = (t1 >> 2 ) & 0x00FFFFF */
	"ins	v1, v0, 20, 11\n"			/* v1 = (v1 & 0x800FFFFF) | ((v0<<20) & 0x7FF00000) */
	"sll	%0 + 0, t1, 30\n"			/* v0 = (t1 << 30) */
"ftod_zero:\n"
	"srl	t0, t0, 31\n"			/* t0 = (t0 >> 31) & 1 */
	"jr	ra\n"				/* return */
	"ins	%0 + 4, t0, 31, 1\n"			/* v1 = (v1 & 0x7FFFFFFF) | ((t0<<31) & 0x80000000) */
	".set pop\n"		
		: "=r"(*r)
		: "f"(f)
	);
}

inline f32 pspFpuDoubleToFloat(f64 *f)
{
	f32 v;
	asm (
	".set push\n"		
	".set noreorder\n"	
	"ext	t0, %1 + 0, 20, 11\n"		/* t0 = (a1>>20) & 0x000007FF */
	"beq	t0, zero, dtof_zero\n"	/* if (t0==0) goto dtof_zero */
	"xori	t1, t0, 0x7FF\n"		/* t1 = t0 ^ 0x7FF */
	"beq	t1, zero, dtof_naninf\n"	/* if (t1==0) goto dtof_naninf */
	"addiu	t1, t0, (+0x7F-0x3FF)\n"	/* t1 = t0 + 0x7F - 0x3FF */
	"blez	t1, dtof_denormal\n"	/* if (t1<=0) goto dtof_denormal */
	"addiu	t2, t1, -0xFE\n"		/* t2 = t1 - 0xFE */
	"bgtz	t2, dtof_inf\n"		/* if (t2 > 0) goto dtof_inf */
	"move	v0, zero\n"		/* v0 = 0 */

	"srl	v0, %1 + 0, 29\n"			/* v0 = (a0>>29) & 0x00000007 */
	"ins	v0, %1 + 4, 3, 20\n"			/* v0 = (v0 & 0xFF800007) | ((a1 & 0FFFFF)<<3) */
	"beq	t2, zero, dtof_inf_normal\n"	/* if (t2==0) goto dtof_inf_normal */
"dtof_normal:\n"
	"srl	t2, %1 + 4, 31\n"		/* t2 = (a1>>31) & 1 */
	"ins	v0, t2, 31, 1\n"		/* v0 = (v0 & 0x7FFFFFFF) | (t2 << 31) */
	"ins	v0, t1, 23, 8\n"		/* v0 = (v0 & 0x8007FFFF) | (t1 << 23) */
	"jr	ra\n"			/* return */
	"mtc1	v0, %0\n"			/* fv0 = v0 */
"dtof_denormal:\n"
	"sll	t0, %1 + 4, 12\n"		/* t0 = a1 << 12 */
	"srl	v0, t0, 10\n"		/* v0 = t0 >> 10 */
	"srl	t0, %1 + 0, 30\n"		/* t0 = t0 >> 30 */
	"or	v0, v0, t0\n"		/* v0 = v0 | t0 */
	"li	t0, 0x00400000\n"		/* t0 = 0x00400000 */
	"or	v0, v0, t0\n"		/* v0 = v0 | t0 */
	"subu	t0, zero, t1\n"		/* t0 = zero - t1 */
	"sltiu	t1, t0, 22\n"		/* t1 = (t0 < 22) */
	"beq	t1, zero, dtof_min\n"	/* if (t1==0) goto dtof_min */
	"srlv	v0, v0, t0\n"		/* v0 = v0 >> t0 */
	"srl	t2, %1 + 4, 31\n"		/* t2 = (a1>>31) & 1 */
	"ins	v0, t2, 31, 1\n"		/* v0 = (v0 & 0x7FFFFFFF) | (t2 << 31) */
	"jr	ra\n"			/* return */
	"mtc1	v0, %0\n"			/* fv0 = v0 */
"dtof_zero:\n"
	"sll	t0, %1 + 4, 12\n"		/* t0 = a1 << 12 */
	"or	t0, t0, %1 + 0\n"		/* t0 = t0 | a0 */
"dtof_min:\n"
	"li	v0, 0x00000001\n"		/* v0 = 0x00000001 */
	"movz	v0, zero, t0\n"		/* v0 = (t0==0) ? zero : v0 */
	"srl	t0, %1 + 4, 31\n"		/* t0 = (a1 >> 31) & 1 */
	"ins	v0, t0, 31, 1\n"		/* v0 = (v0 & 0x7FFFFFFF) | ((t0<<31) & 0x80000000) */
	"jr	ra\n"			/* return */
	"mtc1	v0, %0\n"			/* fv0 = v0 */
"dtof_inf_normal:\n"
	"nor	t0, zero, %1 + 4\n"		/* t0 = ~a1 */
	"sll	t0, t0, 12\n"		/* t0 = t0 << 12 */
	"bne	t0, zero, dtof_normal\n"	/* if (t0!=0) goto dtof_normal */
	"srl	t0, %1 + 0, 28\n"		/* t0 = a0 >> 28 */
	"sltiu	t0, t0, 0xF\n"		/* t0 = (t0 < 0xF) */
	"bne	t0, zero, dtof_normal\n"	/* if (t0!=0) goto dtof_normal */
	"nop\n"				/* waste delay slot */
	"j	dtof_inf\n"		/* goto dtof_inf */
	"move	v0, zero\n"		/* v0 = 0 */
"dtof_naninf:\n"
	"sll	t0, %1 + 4, 12\n"		/* t0 = a1 << 12 */
	"or	t1, t0, %1 + 0\n"		/* t1 = t0 | a0 */
	"srl	v0, t0, 9\n"		/* v0 = t0 >> 9 */
	"srl	t0, %1 + 0, 29\n"		/* t0 = t0 >> 29 */
	"or	v0, v0, t0\n"		/* v0 = v0 | t0 */
	"sltiu	t0, v0, 1\n"		/* t0 = (v0 < 1) */
	"or	v0, v0, t0\n"		/* v0 = v0 | t0 */
	"movz	v0, zero, t1\n"		/* v0 = (t1==0) ? zero : v0 */
"dtof_inf:\n"
	"li	t0, 0x7F800000\n"		/* t0 = 0x7F800000 */
	"or	v0, v0, t0\n"		/* v0 = v0 | t0 */
	"srl	t0, %1 + 4, 31\n"		/* t0 = (a1 >> 31) & 1 */
	"ins	v0, t0, 31, 1\n"		/* v0 = (v0 & 0x7FFFFFFF) | ((t0<<31) & 0x80000000) */
	"jr	ra\n"			/* return */
	"mtc1	v0, %0\n"			/* fv0 = v0 */
	".set pop\n"		
		: "=r"(v)
		: "f"(*f)
	);
	return (v);
}

#undef zero		/* wired zero */
#undef at		/* assembler temp */
#undef v0		/* return value */
#undef v1	
#undef a0		/* argument registers */
#undef a1	
#undef a2	
#undef a3	
#undef t0		/* caller saved */
#undef t1	
#undef t2	
#undef t3	
#undef t4	
#undef t5	
#undef t6	
#undef t7	
#undef s0		/* callee saved */
#undef s1	
#undef s2	
#undef s3	
#undef s4	
#undef s5	
#undef s6	
#undef s7	
#undef t8		/* code generator */
#undef t9	
#undef k0		/* kernel temporary */
#undef k1	
#undef gp		/* global pointer */
#undef sp		/* stack pointer */
#undef fp		/* frame pointer */
#undef ra		/* return address */

#undef fv0	
#undef fv1	
#undef ft0	
#undef ft1	
#undef ft2	
#undef ft3	
#undef ft4	
#undef ft5	
#undef ft6	
#undef ft7	
#undef ft8	
#undef ft9	
#undef fa0	
#undef fa1	
#undef fa2	
#undef fa3	
#undef fa4	
#undef fa5	
#undef fa6	
#undef fa7	
#undef fs0	
#undef fs1	
#undef fs2	
#undef fs3	
#undef fs4	
#undef fs5	
#undef fs6	
#undef fs7	
#undef fs8	
#undef fs9	
#undef fs10	
#undef fs11	



//Fast way to check IsNaN on doubles //Corn
inline bool IsNaN_Double(double x)
 {
     union
	 {
		 double val_D;
		 u64 val_I;
	 }Conv;

	 Conv.val_D = x;

	 return (Conv.val_I & 0x7fffffffffffffffLL) > 0x7ff0000000000000LL;
 }

//Fast way to check IsNaN on floats //Corn
inline bool IsNaN_Float(float x)
{
     union
	 {
		 f32 val_F;
		 u32 val_I;
	 }Conv;

	 Conv.val_F = x;

	 return (Conv.val_I & 0x7fffffff) > 0x7f800000;
}

#endif // DOUBLE_CONVERSION
#endif // DAEDMATHS_H__

