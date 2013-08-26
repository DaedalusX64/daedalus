#ifndef MATH_MATH_H_
#define MATH_MATH_H_

#include <math.h>

//ToDo: Use M_PI for x86 platform?
#define PI   3.141592653589793f

#ifdef DAEDALUS_PSP

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

//Sign of Z coord from cross product normal, used for triangle front/back face culling //Corn
//Note that we pass s32 even if it is a f32! The check for <= 0.0f is valid also with signed integers(bit31 in f32 is sign bit) 
//(((B.x*ACw - AxBC)*(C.y*ABw - AyBC) - (C.x*ABw - AxBC)*(B.y*ACw - AyBC)) * ABw * C.w)
//(((Bx - Ax)*(Cy - Ay) - (Cx - Ax)*(By - Ay)) * Aw * Bw * C.w)
inline s32 vfpu_TriNormSign(u8 *Base, u32 v0, u32 v1, u32 v2) {
    u8* A= Base + (v0<<6);	//Base + v0 * sizeof( DaedalusVtx4 )
    u8* B= Base + (v1<<6);	//Base + v1 * sizeof( DaedalusVtx4 )
    u8* C= Base + (v2<<6);	//Base + v2 * sizeof( DaedalusVtx4 )
	s32 result;

    __asm__ volatile (
		"lv.q	R000, 16+%1\n"				//load projected V0 (A)
		"lv.q	R001, 16+%2\n"				//load projected V1 (B)
		"lv.q	R002, 16+%3\n"				//load projected V2 (C)
		"vcrs.t	R003, C030, C030\n"			//R003 = BCw, ACw, ABw
		"vscl.p	R000, R000, S003\n"			//scale Ax and Ay with BCw
		"vscl.p	R001, R001, S013\n"			//scale Bx and By with ACw 
		"vscl.p	R002, R002, S023\n"			//scale Cx and Cy with ABw 
		"vsub.p	R100, R000, R001\n"			//Make 2D vector with A-B
		"vsub.p R101, R001, R002\n"			//Make 2D vector with B-C 
		"vdet.p S102, R100, R101\n"			//Calc 2x2 determinant with the two 2D vectors
        "vmul.s	S003, S003, S030\n"			//create ABCw (BCw * Aw)
        "vmul.s	S102, S102, S003\n"			//determinant * ABCw
		"mfv	%0, S102\n"					//Sign determins FRONT or BACK face triangle 
        : "=r"(result) :"m"(*A), "m"(*B), "m"(*C) );
    return result;
}

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

// I'm not sure if the vfpu_fmaxf and vfpu_fminf it's slower or faster than fpu version
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


#define Sqrt(x)  pspFpuSqrt((x))
#define Round(x) pspFpuRound((x))		// FIXME(strmnnrmn): results in an int! Alternate version below results in a float!
#define Sinf(x)  vfpu_sinf((x))
#define Cosf(x)  vfpu_cosf((x))

#define Abs(x)	pspFpuAbs((x))

inline void SinCos(float x, float * s, float * c)
{
	vfpu_sincos(x, s, c);
}

#else

inline float Sqrt(float x)
{
	return sqrtf( x );
}

inline float Round(float x)
{
	return floorf(x + 0.5f);
}

#define Abs(x)	fabsf((x))

inline float InvSqrt(float x)
{
	return 1.0f / Sqrt( x );
}

inline float Sinf(float x)
{
	return sinf( x );
}

inline float Cosf(float x)
{
	return cosf( x );
}

inline void SinCos(float x, float * s, float * c)
{
	*s = sinf(x);
	*c = cosf(x);
}

#endif // DAEDALUS_PSP

#endif // MATH_MATH_H_

