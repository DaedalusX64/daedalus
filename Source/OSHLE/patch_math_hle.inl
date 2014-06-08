
#define TEST_DISABLE_MATH_FUNCS DAEDALUS_PROFILE(__FUNCTION__);

//*****************************************************************************
//
//*****************************************************************************
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

//*****************************************************************************
//
//*****************************************************************************
// By Jun Su
// Used by SM64 and Space station Silicon Valley
u32 Patch___ll_div()
{
TEST_DISABLE_MATH_FUNCS
	// Fixed by  StrmnNrmn - regs cast to 32 bits so shift didn't work
	// This was breaking 007
	// Check if data is 32bit(cheap) or 64bit(expensive) //Corn
	// (note. complete check should include top most bit in low 32bit value)

	if( ((gGPR[REG_a0]._u32_0 + (gGPR[REG_a1]._u32_0 >> 31)) +
		 (gGPR[REG_a2]._u32_0 + (gGPR[REG_a3]._u32_0 >> 31)) == 0) )
	{
		//Do 32bit
		s32 op1 = (s32)gGPR[REG_a1]._u32_0;
		s32 op2 = (s32)gGPR[REG_a3]._u32_0;

		if (op2 == 0)
		{
			return PATCH_RET_NOT_PROCESSED;
		}
		else
		{
			s32 result = op1 / op2;

			// StrmnNrmn - the s32 casts were originally u32. Not sure if this is needed
			gGPR[REG_v1]._s64 = (s64)result;
			gGPR[REG_v0]._s64 = (s64)result >> 32;
			return PATCH_RET_JR_RA;
		}
	}
	else
	{
		//Do 64bit
		s64 op1 = (s64)((u64)gGPR[REG_a0]._u32_0 << 32 | (u64)gGPR[REG_a1]._u32_0);
		s64 op2 = (s64)((u64)gGPR[REG_a2]._u32_0 << 32 | (u64)gGPR[REG_a3]._u32_0);

		if (op2 == 0)
		{
			return PATCH_RET_NOT_PROCESSED;
		}
		else
		{
			s64 result = op1 / op2;

			// StrmnNrmn - the s32 casts were originally u32. Not sure if this is needed
			gGPR[REG_v1]._s64 = (s64)(s32)(result & 0xffffffff);
			gGPR[REG_v0]._s64 = (s64)(s32)(result >> 32);
			return PATCH_RET_JR_RA;
		}
	}
}

//*****************************************************************************
//
//*****************************************************************************
// By Jun Su
//Used by AeroGauge
u32 Patch___ull_div()
{
TEST_DISABLE_MATH_FUNCS
	// Fixed by  StrmnNrmn - regs cast to 32 bits so shift didn't work
	// Check if data is 32bit(cheap) or 64bit(expensive) //Corn

	if( (gGPR[REG_a0]._u32_0 | gGPR[REG_a2]._u32_0) == 0 )
	{
		//Do 32bit
		u32 op1 = gGPR[REG_a1]._u32_0;
		u32 op2 = gGPR[REG_a3]._u32_0;

		if (op2==0)
		{
			return PATCH_RET_NOT_PROCESSED;
		}
		else
		{
			u32 result = op1 / op2;

			// StrmnNrmn - the s32 casts were originally u32. Not sure if this is needed
			gGPR[REG_v1]._s64 = (s64)result;
			gGPR[REG_v0]._s64 = (s64)result >> 32;
			return PATCH_RET_JR_RA;
		}
	}
	else
	{
		//Do 64bit
		u64 op1 = (u64)((u64)gGPR[REG_a0]._u32_0 << 32 | (u64)gGPR[REG_a1]._u32_0);
		u64 op2 = (u64)((u64)gGPR[REG_a2]._u32_0 << 32 | (u64)gGPR[REG_a3]._u32_0);

		if (op2==0)
		{
			return PATCH_RET_NOT_PROCESSED;
		}
		else
		{
			u64 result = op1 / op2;

			// StrmnNrmn - the s32 casts were originally u32. Not sure if this is needed
			gGPR[REG_v1]._s64 = (s64)(s32)(result & 0xffffffff);
			gGPR[REG_v0]._s64 = (s64)(s32)(result >> 32);
			return PATCH_RET_JR_RA;
		}
	}
}

//*****************************************************************************
//
//*****************************************************************************
// By Jun Su
//Used heavily by Super Smash Brothers
u32 Patch___ull_rshift()
{
TEST_DISABLE_MATH_FUNCS

	// Fixed by  StrmnNrmn - regs cast to 32 bits so shift didn't work
	u64 op1 = (u64)((u64)gGPR[REG_a0]._u32_0 << 32 | (u64)gGPR[REG_a1]._u32_0);
	//u64 op2 = (u64)((u64)gGPR[REG_a2]._u32_0 << 32 | (u64)gGPR[REG_a3]._u32_0);
	u32 op2 = gGPR[REG_a3]._u32_0;

	u64 result = op1 >> op2;

	// StrmnNrmn - the s32 casts were originally u32. Not sure if this is needed
	//gGPR[REG_v1]._s64 = (s64)(s32)(result & 0xffffffff);
	//gGPR[REG_v0]._s64 = (s64)(s32)(result>>32);
	gGPR[REG_v1]._u64 = result & 0xffffffff;
	gGPR[REG_v0]._u64 = result >> 32;
	return PATCH_RET_JR_RA;
}

//*****************************************************************************
//
//*****************************************************************************
// By Jun Su
u32 Patch___ll_lshift()
{
TEST_DISABLE_MATH_FUNCS

	// Fixed by  StrmnNrmn - regs cast to 32 bits so shift didn't work
	s64 op1 = (s64)((u64)gGPR[REG_a0]._u32_0 << 32 | (u64)gGPR[REG_a1]._u32_0);
	//u64 op2 = (u64)((u64)gGPR[REG_a2]._u32_0 << 32 | (u64)gGPR[REG_a3]._u32_0);
	u32 op2 = gGPR[REG_a3]._u32_0;

	s64 result = op1 << op2;

	// StrmnNrmn - the s32 casts were originally u32. Not sure if this is needed
	gGPR[REG_v1]._s64 = (s64)(s32)(result & 0xffffffff);
	gGPR[REG_v0]._s64 = (s64)(s32)(result >> 32);
	return PATCH_RET_JR_RA;
}

//*****************************************************************************
//
//*****************************************************************************
// By Jun Su
//Used by SSV
u32 Patch___ll_rshift()
{
TEST_DISABLE_MATH_FUNCS

	// Fixed by  StrmnNrmn - regs cast to 32 bits so shift didn't work
	s64 op1 = (s64)((u64)gGPR[REG_a0]._u32_0 << 32 | (u64)gGPR[REG_a1]._u32_0);
	//u64 op2 = (u64)((u64)gGPR[REG_a2]._u32_0 << 32 | (u64)gGPR[REG_a3]._u32_0);
	u32 op2 = gGPR[REG_a3]._u32_0;

	s64 result = op1 >> op2;

	// StrmnNrmn - the s32 casts were originally u32. Not sure if this is needed
	gGPR[REG_v1]._s64 = (s32)(result & 0xffffffff);
	gGPR[REG_v0]._s64 = (result >> 32);
	return PATCH_RET_JR_RA;
}

//*****************************************************************************
//
//*****************************************************************************
// By Jun Su
u32 Patch___ll_mod()
{
TEST_DISABLE_MATH_FUNCS

	// Fixed by  StrmnNrmn - regs cast to 32 bits so shift didn't work
	s64 op1 = (s64)((u64)gGPR[REG_a0]._u32_0 << 32 | (u64)gGPR[REG_a1]._u32_0);
	s64 op2 = (s64)((u64)gGPR[REG_a2]._u32_0 << 32 | (u64)gGPR[REG_a3]._u32_0);

	if (op2==0)
	{
		return PATCH_RET_NOT_PROCESSED;
	}
	else
	{
		s64 result = op1 % op2;

		// StrmnNrmn - the s32 casts were originally u32. Not sure if this is needed
		gGPR[REG_v1]._s64 = (s32)(result & 0xffffffff);
		gGPR[REG_v0]._s64 = (result >> 32);
		return PATCH_RET_JR_RA;
	}
}

//*****************************************************************************
//
//*****************************************************************************
u32 Patch_sqrtf()
{
TEST_DISABLE_MATH_FUNCS
	// FP12 is input
	// FP00 is output
	f32 f = gCPUState.FPU[12]._f32;
	gCPUState.FPU[00]._f32 = sqrtf(f);

	//DBGConsole_Msg(0, "sqrtf(%f) (ra 0x%08x)", f, gGPR[REG_ra]._u32_0);
	return PATCH_RET_JR_RA;
}

//*****************************************************************************
//
//*****************************************************************************
u32 Patch_sinf()
{
TEST_DISABLE_MATH_FUNCS
	// FP12 is input
	// FP00 is output
	f32 f = gCPUState.FPU[12]._f32;
	gCPUState.FPU[00]._f32 = sinf(f);

	//DBGConsole_Msg(0, "sinf(%f) (ra 0x%08x)", f, gGPR[REG_ra]._u32_0);
	return PATCH_RET_JR_RA;
}

//*****************************************************************************
//
//*****************************************************************************
u32 Patch_cosf()
{
TEST_DISABLE_MATH_FUNCS
	// FP12 is input
	// FP00 is output
	f32 f = gCPUState.FPU[12]._f32;
	gCPUState.FPU[00]._f32 = cosf(f);

	//DBGConsole_Msg(0, "cosf(%f) (ra 0x%08x)", f, gGPR[REG_ra]._u32_0);
	return PATCH_RET_JR_RA;

}

//*****************************************************************************
//
//*****************************************************************************
u32 Patch___ull_rem()
{
	u64 op1 = (u64)((u64)gGPR[REG_a0]._u32_0 << 32 | (u64)gGPR[REG_a1]._u32_0);
	u64 op2 = (u64)((u64)gGPR[REG_a2]._u32_0 << 32 | (u64)gGPR[REG_a3]._u32_0);

	if (op2==0)
	{
		return PATCH_RET_NOT_PROCESSED;
	}
	else
	{
		u64 result = op1 % op2;
		// StrmnNrmn - the s32 casts were originally u32. Not sure if this is needed
		gGPR[REG_v1]._s64 = (s64)(s32)(result & 0xffffffff);
		gGPR[REG_v0]._s64 = (s64)(s32)(result>>32);
		return PATCH_RET_JR_RA;
	}
}

//*****************************************************************************
//
//*****************************************************************************
u32 Patch___ull_divremi()
{
	return PATCH_RET_NOT_PROCESSED;
}

//*****************************************************************************
//
//*****************************************************************************
u32 Patch___lldiv()
{
	//Calls ll_div and ull_mul
	return PATCH_RET_NOT_PROCESSED;
}

//*****************************************************************************
//
//*****************************************************************************
u32 Patch___ldiv()
{
	return PATCH_RET_NOT_PROCESSED;
}


