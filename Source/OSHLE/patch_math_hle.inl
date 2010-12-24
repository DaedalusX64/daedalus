
#define TEST_DISABLE_MATH_FUNCS DAEDALUS_PROFILE(__FUNCTION__);


u32 Patch___ull_mul()
{
TEST_DISABLE_MATH_FUNCS
	u32 HiA = gGPR[REG_a0]._u32_0;
	u32 LoA = gGPR[REG_a1]._u32_0;
	u32 HiB = gGPR[REG_a2]._u32_0;
	u32 LoB = gGPR[REG_a3]._u32_0;

	u64 A = ((u64)HiA << 32) | (u64)LoA;
	u64 B = ((u64)HiB << 32) | (u64)LoB;

	u64 R = A * B;

	gGPR[REG_v0]._s64 = (s64)R >> 32;
	gGPR[REG_v1]._s64 = (s64)(s32)(R & 0xFFFFFFFF);

	return PATCH_RET_JR_RA;
}


// By Jun Su
u32 Patch___ll_div() 
{ 
TEST_DISABLE_MATH_FUNCS
	s64 op1, op2, result; 
	
	// Fixed by  StrmnNrmn - regs cast to 32 bits so shift didn't work
	// This was breaking 007
	op1 = (s64)((u64)gGPR[REG_a0]._u32_0 << 32 | (u64)gGPR[REG_a1]._u32_0); 
	op2 = (s64)((u64)gGPR[REG_a2]._u32_0 << 32 | (u64)gGPR[REG_a3]._u32_0); 
	
	if (op2==0) 
	{ 
		return PATCH_RET_NOT_PROCESSED; 
	} 
	else 
	{ 
		result = op1 / op2; 
		// StrmnNrmn - the s32 casts were originally u32. Not sure if this is needed
		gGPR[REG_v1]._s64 = (s64)(s32)(result & 0xffffffff); 
		gGPR[REG_v0]._s64 = (s64)(s32)(result>>32); 
		return PATCH_RET_JR_RA; 
	} 
} 

// By Jun Su
u32 Patch___ull_div() 
{ 
TEST_DISABLE_MATH_FUNCS
	u64 op1, op2, result; 
	
	// Fixed by  StrmnNrmn - regs cast to 32 bits so shift didn't work
	op1 = (u64)((u64)gGPR[REG_a0]._u32_0 << 32 | (u64)gGPR[REG_a1]._u32_0); 
	op2 = (u64)((u64)gGPR[REG_a2]._u32_0 << 32 | (u64)gGPR[REG_a3]._u32_0); 
	
	if (op2==0) 
	{ 
		return PATCH_RET_NOT_PROCESSED; 
	} 
	else 
	{ 
		result = op1 / op2; 
		// StrmnNrmn - the s32 casts were originally u32. Not sure if this is needed
		gGPR[REG_v1]._s64 = (s64)(s32)(result & 0xffffffff); 
		gGPR[REG_v0]._s64 = (s64)(s32)(result>>32); 
		return PATCH_RET_JR_RA; 
	} 
} 


// By Jun Su
u32 Patch___ull_rshift() 
{ 
TEST_DISABLE_MATH_FUNCS
	u64 op1, op2, result; 
	
	// Fixed by  StrmnNrmn - regs cast to 32 bits so shift didn't work
	op1 = (u64)((u64)gGPR[REG_a0]._u32_0 << 32 | (u64)gGPR[REG_a1]._u32_0); 
	op2 = (u64)((u64)gGPR[REG_a2]._u32_0 << 32 | (u64)gGPR[REG_a3]._u32_0); 
	
	result = op1 >> op2; 
	// StrmnNrmn - the s32 casts were originally u32. Not sure if this is needed
	gGPR[REG_v1]._s64 = (s64)(s32)(result & 0xffffffff); 
	gGPR[REG_v0]._s64 = (s64)(s32)(result>>32); 
	return PATCH_RET_JR_RA; 
} 

// By Jun Su
u32 Patch___ll_lshift() 
{ 
TEST_DISABLE_MATH_FUNCS
	s64 op1, result; 
	u64 op2; 

	// Fixed by  StrmnNrmn - regs cast to 32 bits so shift didn't work
	op1 = (s64)((u64)gGPR[REG_a0]._u32_0 << 32 | (u64)gGPR[REG_a1]._u32_0); 
	op2 = (u64)((u64)gGPR[REG_a2]._u32_0 << 32 | (u64)gGPR[REG_a3]._u32_0); 
	
	result = op1 << op2; 
	// StrmnNrmn - the s32 casts were originally u32. Not sure if this is needed
	gGPR[REG_v1]._s64 = (s64)(s32)(result & 0xffffffff); 
	gGPR[REG_v0]._s64 = (s64)(s32)(result>>32); 
	return PATCH_RET_JR_RA; 
} 


// By Jun Su
u32 Patch___ll_rshift() 
{ 
TEST_DISABLE_MATH_FUNCS
	s64 op1, result; 
	u64 op2; 
	
	// Fixed by  StrmnNrmn - regs cast to 32 bits so shift didn't work
	op1 = (s64)((u64)gGPR[REG_a0]._u32_0 << 32 | (u64)gGPR[REG_a1]._u32_0); 
	op2 = (u64)((u64)gGPR[REG_a2]._u32_0 << 32 | (u64)gGPR[REG_a3]._u32_0); 
	
	result = op1 >> op2; 
	// StrmnNrmn - the s32 casts were originally u32. Not sure if this is needed
	gGPR[REG_v1]._s64 = (s64)(s32)(result & 0xffffffff); 
	gGPR[REG_v0]._s64 = (s64)(s32)(result>>32); 
	return PATCH_RET_JR_RA; 
} 

// By Jun Su
u32 Patch___ll_mod() 
{ 
TEST_DISABLE_MATH_FUNCS
	s64 op1, op2, result; 
	
	// Fixed by  StrmnNrmn - regs cast to 32 bits so shift didn't work
	op1 = (s64)((u64)gGPR[REG_a0]._u32_0 << 32 | (u64)gGPR[REG_a1]._u32_0); 
	op2 = (s64)((u64)gGPR[REG_a2]._u32_0 << 32 | (u64)gGPR[REG_a3]._u32_0); 
	
	if (op2==0) 
	{ 
		return PATCH_RET_NOT_PROCESSED; 
	} 
	else 
	{ 
		result = op1 % op2; 
		// StrmnNrmn - the s32 casts were originally u32. Not sure if this is needed
		gGPR[REG_v1]._s64 = (s64)(s32)(result & 0xffffffff); 
		gGPR[REG_v0]._s64 = (s64)(s32)(result>>32); 
		return PATCH_RET_JR_RA; 
	} 
} 

u32 Patch_sqrtf()
{
TEST_DISABLE_MATH_FUNCS
	f32 f = ToFloat(gCPUState.FPU[12]);
	ToFloat(gCPUState.FPU[00]) = pspFpuSqrt(f);	// Do not use vfpu, Check math.h !
	return PATCH_RET_JR_RA;
}

u32 Patch_sinf()
{
TEST_DISABLE_MATH_FUNCS
	// FP12 is input
	// FP00 is output
	f32 f = ToFloat(gCPUState.FPU[12]);

	f32 r = vfpu_sinf(f);

	//DBGConsole_Msg(0, "sinf(%f) = %f (ra 0x%08x)", f, r, (u32)gGPR[REG_ra]);

	ToFloat(gCPUState.FPU[00]) = r;

/*	g_dwNumCosSin++;
	if ((g_dwNumCosSin % 100000) == 0)
	{
		DBGConsole_Msg(0, "%d sin/cos calls intercepted", g_dwNumCosSin);
	}*/
	return PATCH_RET_JR_RA;
}

u32 Patch_cosf()
{
TEST_DISABLE_MATH_FUNCS
	// FP12 is input
	// FP00 is output
	f32 f = ToFloat(gCPUState.FPU[12]);

	f32 r = vfpu_cosf(f);

	//DBGConsole_Msg(0, "cosf(%f) = %f (ra 0x%08x)", f, r, (u32)gGPR[REG_ra]);

	ToFloat(gCPUState.FPU[00]) = r;

/*	g_dwNumCosSin++;
	if ((g_dwNumCosSin % 100000) == 0)
	{
		DBGConsole_Msg(0, "%d sin/cos calls intercepted", g_dwNumCosSin);
	}*/
	return PATCH_RET_JR_RA;

}

u32 Patch___ull_rem()
{
	u64 op1, op2, result; 

	op1 = (u64)((u64)gGPR[REG_a0]._u32_0 << 32 | (u64)gGPR[REG_a1]._u32_0); 
	op2 = (u64)((u64)gGPR[REG_a2]._u32_0 << 32 | (u64)gGPR[REG_a3]._u32_0); 

	if (op2==0) 
	{ 
		return PATCH_RET_NOT_PROCESSED; 
	} 
	else 
	{ 
		result = op1 % op2; 
		// StrmnNrmn - the s32 casts were originally u32. Not sure if this is needed
		gGPR[REG_v1]._s64 = (s64)(s32)(result & 0xffffffff); 
		gGPR[REG_v0]._s64 = (s64)(s32)(result>>32); 
		return PATCH_RET_JR_RA; 
	}
}



u32 Patch___ull_divremi()
{
	return PATCH_RET_NOT_PROCESSED;
}


u32 Patch___lldiv()
{
	return PATCH_RET_NOT_PROCESSED;
}

u32 Patch___ldiv()
{
	return PATCH_RET_NOT_PROCESSED;
}


