#define TEST_DISABLE_GU_FUNCS DAEDALUS_PROFILE(__FUNCTION__);

#ifndef DAEDALUS_PSP_USE_VFPU
//Fixed point matrix
static const u32 s_IdentMatrixL[16] = 
{
	0x00010000,	0x00000000,
	0x00000001,	0x00000000,
	0x00000000,	0x00010000,
	0x00000000,	0x00000001,
	0x00000000, 0x00000000,
	0x00000000,	0x00000000,
	0x00000000, 0x00000000,
	0x00000000,	0x00000000
};

static const u32 s_IdentMatrixF[16] = 
{
	0x3f800000,	0x00000000, 0x00000000,	0x00000000,
	0x00000000,	0x3f800000, 0x00000000,	0x00000000,
	0x00000000, 0x00000000, 0x3f800000,	0x00000000,
	0x00000000, 0x00000000, 0x00000000,	0x3f800000
};

static u32 s_ScaleMatrixF[16] = 
{
	0x3f800000,	0x00000000, 0x00000000,	0x00000000,
	0x00000000,	0x3f800000, 0x00000000,	0x00000000,
	0x00000000, 0x00000000, 0x3f800000,	0x00000000,
	0x00000000, 0x00000000, 0x00000000,	0x3f800000
};

static u32 s_TransMatrixF[16] = 
{
	0x3f800000,	0x00000000, 0x00000000,	0x00000000,
	0x00000000,	0x3f800000, 0x00000000,	0x00000000,
	0x00000000, 0x00000000, 0x3f800000,	0x00000000,
	0x00000000, 0x00000000, 0x00000000,	0x3f800000
};
#endif


#ifdef DAEDALUS_PSP_USE_VFPU
//Note. We use unaligned store from VFPU but this is known to corrupt floating point registers on the PHAT
//and potentially cause odd behaviour or even crash the PSP (best is not to use HLE on PHAT)

//Use VFPU to save a IDENTITY matrix //Corn
inline void vfpu_matrix_IdentF(u8 *m) {
	__asm__ volatile (
		"vmidt.q M000\n"						// set M100 to identity
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
#endif

u32 Patch_guMtxIdentF()
{
TEST_DISABLE_GU_FUNCS
	const u32 address = gGPR[REG_a0]._u32_0;
	u8 * pMtxBase = (u8 *)ReadAddress(address);

#ifdef DAEDALUS_PSP_USE_VFPU
	vfpu_matrix_IdentF(pMtxBase);
#else
	// 0x00000000 is 0.0 in IEEE fp
	// 0x3f800000 is 1.0 in IEEE fp
	QuickWrite32Bits(pMtxBase, 0x00, 0x3f800000);
	QuickWrite32Bits(pMtxBase, 0x04, 0);
	QuickWrite32Bits(pMtxBase, 0x08, 0);
	QuickWrite32Bits(pMtxBase, 0x0c, 0);

	QuickWrite32Bits(pMtxBase, 0x10, 0);
	QuickWrite32Bits(pMtxBase, 0x14, 0x3f800000);
	QuickWrite32Bits(pMtxBase, 0x18, 0);
	QuickWrite32Bits(pMtxBase, 0x1c, 0);

	QuickWrite32Bits(pMtxBase, 0x20, 0);
	QuickWrite32Bits(pMtxBase, 0x24, 0);
	QuickWrite32Bits(pMtxBase, 0x28, 0x3f800000);
	QuickWrite32Bits(pMtxBase, 0x2c, 0);

	QuickWrite32Bits(pMtxBase, 0x30, 0);
	QuickWrite32Bits(pMtxBase, 0x34, 0);
	QuickWrite32Bits(pMtxBase, 0x38, 0);
	QuickWrite32Bits(pMtxBase, 0x3c, 0x3f800000);
#endif

	return PATCH_RET_JR_RA;
}



u32 Patch_guMtxIdent()
{
TEST_DISABLE_GU_FUNCS
	const u32 address = gGPR[REG_a0]._u32_0;
	u8 * pMtxBase = (u8 *)ReadAddress(address);

	// This is a lot faster than the real method, which calls
	// glMtxIdentF followed by guMtxF2L

	//memcpy(pMtxBase, s_IdentMatrixL, sizeof(s_IdentMatrixL));

	QuickWrite32Bits(pMtxBase, 0x00, 0x00010000);
	QuickWrite32Bits(pMtxBase, 0x04, 0x00000000);
	QuickWrite32Bits(pMtxBase, 0x08, 0x00000001);
	QuickWrite32Bits(pMtxBase, 0x0c, 0x00000000);

	QuickWrite32Bits(pMtxBase, 0x10, 0x00000000);
	QuickWrite32Bits(pMtxBase, 0x14, 0x00010000);
	QuickWrite32Bits(pMtxBase, 0x18, 0x00000000);
	QuickWrite32Bits(pMtxBase, 0x1c, 0x00000001);

	QuickWrite32Bits(pMtxBase, 0x20, 0x00000000);
	QuickWrite32Bits(pMtxBase, 0x24, 0x00000000);
	QuickWrite32Bits(pMtxBase, 0x28, 0x00000000);
	QuickWrite32Bits(pMtxBase, 0x2c, 0x00000000);

	QuickWrite32Bits(pMtxBase, 0x30, 0x00000000);
	QuickWrite32Bits(pMtxBase, 0x34, 0x00000000);
	QuickWrite32Bits(pMtxBase, 0x38, 0x00000000);
	QuickWrite32Bits(pMtxBase, 0x3c, 0x00000000);

	return PATCH_RET_JR_RA;
}

u32 Patch_guTranslateF()
{
TEST_DISABLE_GU_FUNCS
	const u32 address = gGPR[REG_a0]._u32_0;
	u8 * pMtxBase = (u8 *)ReadAddress(address);

#ifdef DAEDALUS_PSP_USE_VFPU
	const f32 fx = gGPR[REG_a1]._f32_0;
	const f32 fy = gGPR[REG_a2]._f32_0;
	const f32 fz = gGPR[REG_a3]._f32_0;

	vfpu_matrix_TranslateF(pMtxBase, fx, fy, fz);
#else
	s_TransMatrixF[12] = gGPR[REG_a1]._u32_0;
	s_TransMatrixF[13] = gGPR[REG_a2]._u32_0;
	s_TransMatrixF[14] = gGPR[REG_a3]._u32_0;

	QuickWrite512Bits(pMtxBase, (u8 *)s_TransMatrixF);
#endif 

	return PATCH_RET_JR_RA;
}

u32 Patch_guTranslate()
{
TEST_DISABLE_GU_FUNCS
	const f32 fScale = 65536.0f;

	const u32 address = gGPR[REG_a0]._u32_0;
	u8 * pMtxBase = (u8 *)ReadAddress(address);

	const f32 fx = gGPR[REG_a1]._f32_0;
	const f32 fy = gGPR[REG_a2]._f32_0;
	const f32 fz = gGPR[REG_a3]._f32_0;

	u32 x = (u32)(fx * fScale);
	u32 y = (u32)(fy * fScale);
	u32 z = (u32)(fz * fScale);

	u32 one = (u32)(1.0f * fScale);

	u32 xyhibits = (x & 0xFFFF0000) | (y >> 16);
	u32 xylobits = (x << 16) | (y & 0x0000FFFF);

	u32 z1hibits = (z & 0xFFFF0000) | (one >> 16);
	u32 z1lobits = (z << 16) | (one & 0x0000FFFF);

	QuickWrite32Bits(pMtxBase, 0x00, 0x00010000);
	QuickWrite32Bits(pMtxBase, 0x04, 0x00000000);
	QuickWrite32Bits(pMtxBase, 0x08, 0x00000001);
	QuickWrite32Bits(pMtxBase, 0x0c, 0x00000000);

	QuickWrite32Bits(pMtxBase, 0x10, 0x00000000);
	QuickWrite32Bits(pMtxBase, 0x14, 0x00010000);
	QuickWrite32Bits(pMtxBase, 0x18, xyhibits);	// xy
	QuickWrite32Bits(pMtxBase, 0x1c, z1hibits);	// z1

	QuickWrite32Bits(pMtxBase, 0x20, 0x00000000);
	QuickWrite32Bits(pMtxBase, 0x24, 0x00000000);
	QuickWrite32Bits(pMtxBase, 0x28, 0x00000000);
	QuickWrite32Bits(pMtxBase, 0x2c, 0x00000000);

	QuickWrite32Bits(pMtxBase, 0x30, 0x00000000);
	QuickWrite32Bits(pMtxBase, 0x34, 0x00000000);
	QuickWrite32Bits(pMtxBase, 0x38, xylobits);	// xy
	QuickWrite32Bits(pMtxBase, 0x3c, z1lobits);	// z1

	return PATCH_RET_JR_RA;
}

u32 Patch_guScaleF()
{
TEST_DISABLE_GU_FUNCS
	const u32 address = gGPR[REG_a0]._u32_0;
	u8 * pMtxBase = (u8 *)ReadAddress(address);

#ifdef DAEDALUS_PSP_USE_VFPU //Corn
	const f32 fx = gGPR[REG_a1]._f32_0;
	const f32 fy = gGPR[REG_a2]._f32_0;
	const f32 fz = gGPR[REG_a3]._f32_0;

	vfpu_matrix_ScaleF(pMtxBase, fx, fy, fz);

#else
	s_ScaleMatrixF[ 0] = gGPR[REG_a1]._u32_0;
	s_ScaleMatrixF[ 5] = gGPR[REG_a2]._u32_0;
	s_ScaleMatrixF[10] = gGPR[REG_a3]._u32_0;

	QuickWrite512Bits(pMtxBase, (u8 *)s_ScaleMatrixF);
//	memcpy(pMtxBase, s_ScaleMatrixF, sizeof(s_ScaleMatrixF));
#endif

	return PATCH_RET_JR_RA;
}

u32 Patch_guScale()
{
TEST_DISABLE_GU_FUNCS
	const f32 fScale = 65536.0f;

	const u32 address = gGPR[REG_a0]._u32_0;
	u8 * pMtxBase = (u8 *)ReadAddress(address);

	const f32 fx = gGPR[REG_a1]._f32_0;
	const f32 fy = gGPR[REG_a2]._f32_0;
	const f32 fz = gGPR[REG_a3]._f32_0;

	u32 x = (u32)(fx * fScale);
	u32 y = (u32)(fy * fScale);
	u32 z = (u32)(fz * fScale);

	u32 zer = (u32)(0.0f);

	u32 xzhibits = (x & 0xFFFF0000) | (zer >> 16);
	u32 xzlobits = (x << 16) | (zer & 0x0000FFFF);

	u32 zyhibits = (zer & 0xFFFF0000) | (y >> 16);
	u32 zylobits = (zer << 16) | (y & 0x0000FFFF);

	u32 zzhibits = (z & 0xFFFF0000) | (zer >> 16);
	u32 zzlobits = (z << 16) | (zer & 0x0000FFFF);

	QuickWrite32Bits(pMtxBase, 0x00, xzhibits);
	QuickWrite32Bits(pMtxBase, 0x04, 0x00000000);
	QuickWrite32Bits(pMtxBase, 0x08, zyhibits);
	QuickWrite32Bits(pMtxBase, 0x0c, 0x00000000);

	QuickWrite32Bits(pMtxBase, 0x10, 0x00000000);
	QuickWrite32Bits(pMtxBase, 0x14, zzhibits);
	QuickWrite32Bits(pMtxBase, 0x18, 0x00000000);	// xy
	QuickWrite32Bits(pMtxBase, 0x1c, 0x00000001);	// z1

	QuickWrite32Bits(pMtxBase, 0x20, xzlobits);
	QuickWrite32Bits(pMtxBase, 0x24, 0x00000000);
	QuickWrite32Bits(pMtxBase, 0x28, zylobits);
	QuickWrite32Bits(pMtxBase, 0x2c, 0x00000000);

	QuickWrite32Bits(pMtxBase, 0x30, 0x00000000);
	QuickWrite32Bits(pMtxBase, 0x34, zzlobits);
	QuickWrite32Bits(pMtxBase, 0x38, 0x00000000);	// xy
	QuickWrite32Bits(pMtxBase, 0x3c, 0x00000000);	// z1

	return PATCH_RET_JR_RA;
}

u32 Patch_guMtxF2L()
{
TEST_DISABLE_GU_FUNCS

	const f32 fScale = 65536.0f;

	u8 * pMtxFBase = (u8 *)ReadAddress(gGPR[REG_a0]._u32_0);
	u8 * pMtxBase  = (u8 *)ReadAddress(gGPR[REG_a1]._u32_0);

	u8 * pMtxLBaseHiBits = (u8 *)(pMtxBase + 0x00);
	u8 * pMtxLBaseLoBits = (u8 *)(pMtxBase + 0x20);
	
	union
	{
		u32 iFA;
		f32 fFA;
	}uA;

	union
	{
		u32 iFB;
		f32 fFB;
	}uB;

	u32 a, b;
	u32 hibits;
	u32 lobits;
	u32 row;

	for (row = 0; row < 4; row++)
	{
		uA.iFA = QuickRead32Bits(pMtxFBase, (row << 4) + 0x0);
		uB.iFB = QuickRead32Bits(pMtxFBase, (row << 4) + 0x4);
		
		// Should be TRUNC
		a = (u32)(uA.fFA * fScale);
		b = (u32)(uB.fFB * fScale);

		hibits = (a & 0xFFFF0000) | (b >> 16);
		QuickWrite32Bits(pMtxLBaseHiBits, (row << 3) , hibits);

		lobits = (a << 16) | (b & 0x0000FFFF);
		QuickWrite32Bits(pMtxLBaseLoBits, (row << 3) , lobits);
		
		/////
		uA.iFA = QuickRead32Bits(pMtxFBase, (row << 4) + 0x8);
		uB.iFB = QuickRead32Bits(pMtxFBase, (row << 4) + 0xc);

		// Should be TRUNC
		a = (u32)(uA.fFA * fScale);
		b = (u32)(uB.fFB * fScale);

		hibits = (a & 0xFFFF0000) | (b >> 16);
		QuickWrite32Bits(pMtxLBaseHiBits, (row << 3) + 4, hibits);

		lobits = (a << 16) | (b & 0x0000FFFF);
		QuickWrite32Bits(pMtxLBaseLoBits, (row << 3) + 4, lobits);
	}

	return PATCH_RET_JR_RA;
}

//Using VFPU and no memcpy (works without hack?) //Corn
u32 Patch_guNormalize_Mario()
{
TEST_DISABLE_GU_FUNCS

	u32 sX = gGPR[REG_a0]._u32_0;
	u32 sY = gGPR[REG_a1]._u32_0;
	u32 sZ = gGPR[REG_a2]._u32_0;

	union
	{
		u32 x;
		f32 fX;
	}uX;

	union
	{
		u32 y;
		f32 fY;
	}uY;

	union
	{
		u32 z;
		f32 fZ;
	}uZ;

	uX.x = Read32Bits(sX);
	uY.y = Read32Bits(sY);
	uZ.z = Read32Bits(sZ);

#ifdef DAEDALUS_PSP_USE_VFPU //Corn
	vfpu_norm_3Dvec(&uX.fX, &uY.fY, &uZ.fZ);
#else
	f32 fLenRecip = 1.0f / sqrtf((uX.x * uX.x) + (uY.y * uY.y) + (uZ.z * uZ.z));

	uX.x *= fLenRecip;
 	uY.y *= fLenRecip;
 	uZ.z *= fLenRecip;
#endif

	Write32Bits(sX, uX.x);
	Write32Bits(sY, uY.y);
	Write32Bits(sZ, uZ.z);

	return PATCH_RET_JR_RA;
}

// NOT the same function as guNormalise_Mario
// This take one pointer, not 3
u32 Patch_guNormalize_Rugrats() //Using VFPU and no memcpy //Corn
{
TEST_DISABLE_GU_FUNCS
	u32 sX = gGPR[REG_a0]._u32_0;
	u32 sY = sX + 4;
	u32 sZ = sX + 8;

	DBGConsole_Msg(0, "Rugrats");

	union
	{
		u32 x;
		f32 fX;
	}uX;

	union
	{
		u32 y;
		f32 fY;
	}uY;

	union
	{
		u32 z;
		f32 fZ;
	}uZ;

	uX.x = Read32Bits(sX);
	uY.y = Read32Bits(sY);
	uZ.z = Read32Bits(sZ);

#ifdef DAEDALUS_PSP_USE_VFPU //Corn
	vfpu_norm_3Dvec(&uX.fX, &uY.fY, &uZ.fZ);
#else
	f32 fLenRecip = 1.0f / sqrtf((uX.x * uX.x) + (uY.y * uY.y) + (uZ.z * uZ.z));

	uX.x *= fLenRecip;
 	uY.y *= fLenRecip;
 	uZ.z *= fLenRecip;
#endif

	Write32Bits(sX, uX.x);
	Write32Bits(sY, uY.y);
	Write32Bits(sZ, uZ.z);

	return PATCH_RET_JR_RA;
}

u32 Patch_guOrthoF()
{
TEST_DISABLE_GU_FUNCS
	union
	{
		u32 L;
		f32 fL;
	}uL;

	union
	{
		u32 R;
		f32 fR;
	}uR;

	union
	{
		u32 B;
		f32 fB;
	}uB;

	union
	{
		u32 T;
		f32 fT;
	}uT;

	union
	{
		u32 N;
		f32 fN;
	}uN;

	union
	{
		u32 F;
		f32 fF;
	}uF;

	union
	{
		u32 S;
		f32 fS;
	}uS;

	u8 * pMtxBase   = (u8 *)ReadAddress(gGPR[REG_a0]._u32_0);	// Base address
	u8 * pStackBase = (u8 *)ReadAddress(gGPR[REG_sp]._u32_0);	// Base stack address
	uL.L = gGPR[REG_a1]._u32_0;	//Left
	uR.R = gGPR[REG_a2]._u32_0;	//Right
	uB.B = gGPR[REG_a3]._u32_0;	//Bottom
	uT.T = QuickRead32Bits(pStackBase, 0x10);	//Top
	uN.N = QuickRead32Bits(pStackBase, 0x14);	//Near
	uF.F = QuickRead32Bits(pStackBase, 0x18);	//Far
	uS.S = QuickRead32Bits(pStackBase, 0x1c);	//Scale

#ifdef DAEDALUS_PSP_USE_VFPU //Corn
	vfpu_matrix_OrthoF(pMtxBase, uL.fL, uR.fR, uB.fB, uT.fT, uN.fN, uF.fF, uS.fS);

#else
	f32 fRmL = uR.fR - uL.fL;
	f32 fTmB = uT.fT - uB.fB;
	f32 fFmN = uF.fF - uN.fN;
	f32 fRpL = uR.fR + uL.fL;
	f32 fTpB = uT.fT + uB.fB;
	f32 fFpN = uF.fF + uN.fN;

	// Re-use unused old variables to store Matrix values
	uL.fL =  2.0f * uS.fS / fRmL;
	uR.fR =  2.0f * uS.fS / fTmB;
	uB.fB = -2.0f * uS.fS / fFmN;

	uT.fT = -fRpL * uS.fS / fRmL;
	uN.fN = -fTpB * uS.fS / fTmB;
	uF.fF = -fFpN * uS.fS / fFmN;

	/*
	0   2/(r-l)
	1                2/(t-b)
	2                            -2/(f-n)
	3 -(l+r)/(r-l) -(t+b)/(t-b) -(f+n)/(f-n)     1*/

	// 0x3f800000 is 1.0 in IEEE fp
	QuickWrite32Bits(pMtxBase, 0x00, uL.L);
	QuickWrite32Bits(pMtxBase, 0x04, 0);
	QuickWrite32Bits(pMtxBase, 0x08, 0);
	QuickWrite32Bits(pMtxBase, 0x0c, 0);

	QuickWrite32Bits(pMtxBase, 0x10, 0);
	QuickWrite32Bits(pMtxBase, 0x14, uR.R);
	QuickWrite32Bits(pMtxBase, 0x18, 0);
	QuickWrite32Bits(pMtxBase, 0x1c, 0);

	QuickWrite32Bits(pMtxBase, 0x20, 0);
	QuickWrite32Bits(pMtxBase, 0x24, 0);
	QuickWrite32Bits(pMtxBase, 0x28, uB.B);
	QuickWrite32Bits(pMtxBase, 0x2c, 0);

	QuickWrite32Bits(pMtxBase, 0x30, uT.T);
	QuickWrite32Bits(pMtxBase, 0x34, uN.N);
	QuickWrite32Bits(pMtxBase, 0x38, uF.F);
	QuickWrite32Bits(pMtxBase, 0x3c, uS.S);
#endif 

	return PATCH_RET_JR_RA;
}

//Do the float version on a temporary matrix and convert to fixed point in VFPU & CPU //Corn
u32 Patch_guOrtho()
{
TEST_DISABLE_GU_FUNCS

#ifdef DAEDALUS_PSP_USE_VFPU
	u32 s_TempMatrix[16];

	union
	{
		u32 L;
		f32 fL;
	}uL;

	union
	{
		u32 R;
		f32 fR;
	}uR;

	union
	{
		u32 B;
		f32 fB;
	}uB;

	union
	{
		u32 T;
		f32 fT;
	}uT;

	union
	{
		u32 N;
		f32 fN;
	}uN;

	union
	{
		u32 F;
		f32 fF;
	}uF;

	union
	{
		u32 S;
		f32 fS;
	}uS;

	u8 * pMtxBase   = (u8 *)ReadAddress(gGPR[REG_a0]._u32_0);	// Fixed point Base address
	u8 * pStackBase = (u8 *)ReadAddress(gGPR[REG_sp]._u32_0);	// Base stack address
	uL.L = gGPR[REG_a1]._u32_0;	//Left
	uR.R = gGPR[REG_a2]._u32_0;	//Right
	uB.B = gGPR[REG_a3]._u32_0;	//Bottom	
	uT.T = QuickRead32Bits(pStackBase, 0x10);	//Top
	uN.N = QuickRead32Bits(pStackBase, 0x14);	//Near
	uF.F = QuickRead32Bits(pStackBase, 0x18);	//Far
	uS.S = QuickRead32Bits(pStackBase,  0x1c);	//Scale

	vfpu_matrix_Ortho((u8 *)s_TempMatrix, uL.fL, uR.fR, uB.fB, uT.fT, uN.fN, uF.fF, uS.fS);

//Convert to proper N64 fixed point matrix
	u8 * pMtxLBaseHiBits = (u8 *)(pMtxBase + 0x00);
	u8 * pMtxLBaseLoBits = (u8 *)(pMtxBase + 0x20);

	u32 a, b;
	u32 hibits,lobits;
	u32 row, indx=0;

	for (row = 0; row < 4; row++)
	{
		a = s_TempMatrix[indx++];
		b = s_TempMatrix[indx++];

		hibits = (a & 0xFFFF0000) | (b >> 16);
		QuickWrite32Bits(pMtxLBaseHiBits, (row << 3) , hibits);

		lobits = (a << 16) | (b & 0x0000FFFF);
		QuickWrite32Bits(pMtxLBaseLoBits, (row << 3) , lobits);
		
		/////
		a = s_TempMatrix[indx++];
		b = s_TempMatrix[indx++];

		hibits = (a & 0xFFFF0000) | (b >> 16);
		QuickWrite32Bits(pMtxLBaseHiBits, (row << 3) + 4, hibits);

		lobits = (a << 16) | (b & 0x0000FFFF);
		QuickWrite32Bits(pMtxLBaseLoBits, (row << 3) + 4, lobits);
	}

	return PATCH_RET_JR_RA;
#else
	return PATCH_RET_NOT_PROCESSED;
#endif
}

//RotateF //Corn
u32 Patch_guRotateF()
{
TEST_DISABLE_GU_FUNCS

#ifdef DAEDALUS_PSP_USE_VFPU
	f32 s,c;

	union
	{
		u32 a;
		f32 fA;
	}uA;

	union
	{
		u32 x;
		f32 fX;
	}uX;

	union
	{
		u32 y;
		f32 fY;
	}uY;

	union
	{
		u32 z;
		f32 fZ;
	}uZ;

	union
	{
		u32 r;
		f32 fR;
	}uR;

	u8 * pMtxBase = (u8 *)ReadAddress(gGPR[REG_a0]._u32_0);	//Matrix base address
	uA.a = gGPR[REG_a1]._u32_0;	//Angle in degrees + -> CCW
	uX.x = gGPR[REG_a2]._u32_0;	//X
	uY.y = gGPR[REG_a3]._u32_0;	//Y
	uZ.z = Read32Bits(gGPR[REG_sp]._u32_0 + 0x10);	//Z

	vfpu_sincos(uA.fA*(PI/180.0f), &s, &c);

//According to the manual the vector should be normalized in this function (Seems to work fine without it but risky)
//	vfpu_norm_3Dvec(&uX.fX, &uY.fY, &uZ.fZ);

//Row #1
	uR.fR = uX.fX * uX.fX + c * (1.0f - uX.fX * uX.fX);
	QuickWrite32Bits(pMtxBase, 0x00, uR.r);

	uR.fR = uX.fX * uY.fY * (1.0f - c) + uZ.fZ * s;
	QuickWrite32Bits(pMtxBase, 0x04, uR.r);

	uR.fR = uZ.fZ * uX.fX * (1.0f - c) - uY.fY * s;
	QuickWrite32Bits(pMtxBase, 0x08, uR.r);

	QuickWrite32Bits(pMtxBase, 0x0c, 0x00000000);

//Row #2
	uR.fR = uX.fX * uY.fY * (1.0f - c) - uZ.fZ * s;
	QuickWrite32Bits(pMtxBase, 0x10, uR.r);

	uR.fR = uY.fY * uY.fY + c * (1.0f - uY.fY * uY.fY);
	QuickWrite32Bits(pMtxBase, 0x14, uR.r);

	uR.fR = uY.fY * uZ.fZ * (1.0f - c) + uX.fX * s;
	QuickWrite32Bits(pMtxBase, 0x18, uR.r);

	QuickWrite32Bits(pMtxBase, 0x1c, 0x00000000);

//Row #3
	uR.fR = uZ.fZ * uX.fX * (1.0f - c) + uY.fY * s;
	QuickWrite32Bits(pMtxBase, 0x20, uR.r);

	uR.fR = uY.fY * uZ.fZ * (1.0f - c) - uX.fX * s;
	QuickWrite32Bits(pMtxBase, 0x24, uR.r);

	uR.fR = uZ.fZ * uZ.fZ + c * (1.0f - uZ.fZ * uZ.fZ);
	QuickWrite32Bits(pMtxBase, 0x28, uR.r);

	QuickWrite32Bits(pMtxBase, 0x2c, 0x00000000);

//Row #4
	QuickWrite32Bits(pMtxBase, 0x30, 0x00000000);
	QuickWrite32Bits(pMtxBase, 0x34, 0x00000000);
	QuickWrite32Bits(pMtxBase, 0x38, 0x00000000);
	QuickWrite32Bits(pMtxBase, 0x3c, 0x3F800000);

	return PATCH_RET_JR_RA;
#else
	return PATCH_RET_NOT_PROCESSED;
#endif
}
