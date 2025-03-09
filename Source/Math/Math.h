#ifndef MATH_MATH_H_
#define MATH_MATH_H_

#include <math.h>
#include <utility>

//ToDo: Use M_PI for x86 platform?
#define PI   3.141592653589793f

#ifdef DAEDALUS_PSP
#include <pspfpu.h>
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

//Sign of Z coord from cross product normal, used for triangle front/back face culling //Corn
//Note that we pass s32 even if it is a f32! The check for <= 0.0f is valid also with signed integers(bit31 in f32 is sign bit)
//(((Bx - Ax)*(Cy - Ay) - (Cx - Ax)*(By - Ay)) * Aw * Bw * C.w) (BaseRenderer.h)
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
		"vscl.p	R000, R000, S003\n"			//scale Ax and Ay with BCw to avoid divide with Aw
		"vscl.p	R001, R001, S013\n"			//scale Bx and By with ACw to avoid divide with Bw
		"vscl.p	R002, R002, S023\n"			//scale Cx and Cy with ABw to avoid divide with Cw
		"vsub.p	R100, R000, R001\n"			//Make 2D vector with A-B
		"vsub.p R101, R001, R002\n"			//Make 2D vector with B-C
		"vdet.p S102, R100, R101\n"			//Calc 2x2 determinant with the two 2D vectors
        "vmul.s	S003, S003, S030\n"			//create ABCw (BCw * Aw)
        "vmul.s	S102, S102, S003\n"			//determinant * ABCw
		"mfv	%0, S102\n"					//Sign determins FRONT or BACK face triangle(Note we pass a float as s32 here since -0+ check works regardless!)
        : "=r"(result) :"m"(*A), "m"(*B), "m"(*C) );
    return result;
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


//VFPU 3D Normalize vector //Corn (Vector3.h / Patch_gu_hle.inl)
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


inline float vfpu_invSqrt(float x)	//Trick using int/float to get 1/SQRT() fast on FPU/CPU //Corn (Vector4.h)
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

//Note. We use unaligned store from VFPU but this is known to corrupt floating point registers on the PHAT
//and potentially cause odd behaviour or even crash the PSP (best is not to use HLE on PHAT)

//Use VFPU to save a IDENTITY matrix //Corn
inline void vfpu_matrix_IdentF(u8 *m) {
	__asm__ volatile (
		"vmidt.q M000\n"						// set M000 to identity
		"usv.q    C000, 0  + %0\n"
		"usv.q    C010, 16 + %0\n"
		"usv.q    C020, 32 + %0\n"
		"usv.q    C030, 48 + %0\n"
	:"=m"(*m));
}

//Use VFPU to save a TRANSLATE_F matrix //Corn
inline void vfpu_matrix_TranslateF(u8 *m, float X, float Y, float Z) {
	__asm__ volatile (
		"vmidt.q M000\n"						// set M100 to identity
		"mtv     %1, S030\n"
		"mtv     %2, S031\n"
		"mtv     %3, S032\n"
		"usv.q    C000, 0  + %0\n"
		"usv.q    C010, 16 + %0\n"
		"usv.q    C020, 32 + %0\n"
		"usv.q    C030, 48 + %0\n"
	:"=m"(*m) : "r"(X), "r"(Y), "r"(Z));
}

//Use VFPU to save a SCALE_F matrix //Corn
inline void vfpu_matrix_ScaleF(u8 *m, float X, float Y, float Z) {
	__asm__ volatile (
		"vmidt.q M000\n"						// set M100 to identity
		"mtv     %1, S000\n"
		"mtv     %2, S011\n"
		"mtv     %3, S022\n"
		"usv.q    C000, 0  + %0\n"
		"usv.q    C010, 16 + %0\n"
		"usv.q    C020, 32 + %0\n"
		"usv.q    C030, 48 + %0\n"
	:"=m"(*m) : "r"(X), "r"(Y), "r"(Z));
}

//Taken from Mr.Mr libpspmath and added scale to the EQ. (Scale usually is 1.0f tho) //Corn
inline void vfpu_matrix_OrthoF(u8 *m, float left, float right, float bottom, float top, float near, float far, float scale)
{
	__asm__ volatile (
		"vmidt.q M100\n"						// set M100 to identity
		"mtv     %2, S000\n"					// C000 = [right, ?,      ?,  ]
		"mtv     %4, S001\n"					// C000 = [right, top,    ?,  ]
		"mtv     %6, S002\n"					// C000 = [right, top,    far ]
		"mtv     %1, S010\n"					// C010 = [left,  ?,      ?,  ]
		"mtv     %3, S011\n"					// C010 = [left,  bottom, ?,  ]
		"mtv     %5, S012\n"                	// C010 = [left,  bottom, near]
		"mtv     %7, S133\n"                	// C110 = [0, 0, 0, scale]
		"vsub.t  C020, C000, C010\n"			// C020 = [  dx,   dy,   dz]
		"vrcp.t  C020, C020\n"              	// C020 = [1/dx, 1/dy, 1/dz]
		"vscl.t	 C020, C020, S133\n"			// C020 = [scale/dx, scale/dy, scale/dz]
		"vmul.s  S100, S100[2], S020\n"     	// S100 = m->x.x = 2.0 / dx
		"vmul.s  S111, S111[2], S021\n"     	// S110 = m->y.y = 2.0 / dy
		"vmul.s  S122, S122[2], S022[-x]\n"		// S122 = m->z.z = -2.0 / dz
		"vsub.t  C130, C000[-x,-y,-z], C010\n"	// C130 = m->w[x, y, z] = [-(right+left), -(top+bottom), -(far+near)]
												// we do vsub here since -(a+b) => (-1*a) + (-1*b) => -a - b
		"vmul.t  C130, C130, C020\n"			// C130 = [-(right+left)/dx, -(top+bottom)/dy, -(far+near)/dz]
		"usv.q    C100, 0  + %0\n"
		"usv.q    C110, 16 + %0\n"
		"usv.q    C120, 32 + %0\n"
		"usv.q    C130, 48 + %0\n"
	:"=m"(*m) : "r"(left), "r"(right), "r"(bottom), "r"(top), "r"(near), "r"(far), "r"(scale));
}

//Taken from Mr.Mr libpspmath and added scale and output to fixed point //Corn
inline void vfpu_matrix_Ortho(u8 *m, float left, float right, float bottom, float top, float near, float far, float scale)
{
	__asm__ volatile (
		"vmidt.q M100\n"						// set M100 to identity
		"mtv     %2, S000\n"					// C000 = [right, ?,      ?,  ]
		"mtv     %4, S001\n"					// C000 = [right, top,    ?,  ]
		"mtv     %6, S002\n"					// C000 = [right, top,    far ]
		"mtv     %1, S010\n"					// C010 = [left,  ?,      ?,  ]
		"mtv     %3, S011\n"					// C010 = [left,  bottom, ?,  ]
		"mtv     %5, S012\n"                	// C010 = [left,  bottom, near]
		"mtv     %7, S133\n"                	// C110 = [0, 0, 0, scale]
		"vsub.t  C020, C000, C010\n"			// C020 = [  dx,   dy,   dz]
		"vrcp.t  C020, C020\n"              	// C020 = [1/dx, 1/dy, 1/dz]
		"vscl.t	 C020, C020, S133\n"			// C020 = [scale/dx, scale/dy, scale/dz]
		"vmul.s  S100, S100[2], S020\n"     	// S100 = m->x.x = 2.0 / dx
		"vmul.s  S111, S111[2], S021\n"     	// S110 = m->y.y = 2.0 / dy
		"vmul.s  S122, S122[2], S022[-x]\n"		// S122 = m->z.z = -2.0 / dz
		"vsub.t  C130, C000[-x,-y,-z], C010\n"	// C130 = m->w[x, y, z] = [-(right+left), -(top+bottom), -(far+near)]
												// we do vsub here since -(a+b) => (-1*a) + (-1*b) => -a - b
		"vmul.t  C130, C130, C020\n"			// C130 = [-(right+left)/dx, -(top+bottom)/dy, -(far+near)/dz]
		"vf2iz.q  C100, C100, 16\n"			// scale values to fixed point
		"usv.q    C100, 0  + %0\n"
		"vf2iz.q  C110, C110, 16\n"			// scale values to fixed point
		"usv.q    C110, 16 + %0\n"
		"vf2iz.q  C120, C120, 16\n"			// scale values to fixed point
		"usv.q    C120, 32 + %0\n"
		"vf2iz.q  C130, C130, 16\n"			// scale values to fixed point
		"usv.q    C130, 48 + %0\n"
	:"=m"(*m) : "r"(left), "r"(right), "r"(bottom), "r"(top), "r"(near), "r"(far), "r"(scale));
}

//*****************************************************************************
//FPU Math :D
//*****************************************************************************
/*
sqrtf and fabsf are alot slower on the vfpuv, so let's do em on the fpu.
Check above notes for cycles/comparison
*/

#undef sqrtf
#undef roundf
#undef sinf
#undef cosf
#undef fabsf
#undef sincosf

// We map these because the compiler doesn't use the fpu math... we have to do it manually
//
#define isnanf(x)		pspFpuIsNaN((x))
#define sqrtf(x)		pspFpuSqrt((x))
#define roundf(x)		pspFpuRound((x))		// FIXME(strmnnrmn): results in an int! Alternate version below results in a float!
#define fabsf(x)		pspFpuAbs((x))
#define sinf(x)			pspFpuSin((x))
#define cosf(x)			pspFpuCos((x))
#define sincosf(x,s,c)	vfpu_sincos(x, s, c)

// #elif defined(DAEDALUS_POSIX) // XXX Compare with PSP fpu math // Does not work with Linux.
// #include <cmath>
// #define isnanf(x)		std::isnan((float x))
// #define sqrtf(x)		std::sqrtf((x))
// #define roundf(x)		std::roundf((x))		// FIXME(strmnnrmn): results in an int! Alternate version below results in a float!
// #define fabsf(x)		std::fabsf((x))
// #define sinf(x)			std::sinf((x))
// #define cosf(x)			std::cosf((x))
// #define sincosf(x,s,c)	std::sincosf(x, s, c)

#else 


inline void sincosf(float x, float * s, float * c)
{
	*s = sinf(x);
	*c = cosf(x);
}

inline float InvSqrt(float x)
{
        float xhalf = 0.5f * x;
        int i = *(int*)&x;            // store floating-point bits in integer
        i = 0x5f3759df - (i >> 1);    // initial guess for Newton's method
        x = *(float*)&i;              // convert new bits into float
        x = x*(1.5f - xhalf*x*x);     // One round of Newton's method
        return x;
}

#endif // DAEDALUS_PSP

 
#endif // MATH_MATH_H_
