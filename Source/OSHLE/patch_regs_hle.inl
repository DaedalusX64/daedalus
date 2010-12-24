#define TEST_DISABLE_REG_FUNCS DAEDALUS_PROFILE(__FUNCTION__);

//*****************************************************************************
//
//*****************************************************************************
u32 Patch___osDisableInt_Mario()
{
TEST_DISABLE_REG_FUNCS
	u32 CurrSR = gCPUState.CPUControl[C0_SR]._u32_0;

	gCPUState.CPUControl[C0_SR]._u64 = CurrSR & ~SR_IE;
	gGPR[REG_v0]._u64 = CurrSR & SR_IE;

	return PATCH_RET_JR_RA;
}

//*****************************************************************************
//
//*****************************************************************************
u32 Patch___osDisableInt_Zelda()
{
TEST_DISABLE_REG_FUNCS
	// Same as the above
	u32 CurrSR = gCPUState.CPUControl[C0_SR]._u32_0;

	gCPUState.CPUControl[C0_SR]._u64 = CurrSR & ~SR_IE;
	gGPR[REG_v0]._u64 = CurrSR & SR_IE;

	return PATCH_RET_JR_RA;
}

//*****************************************************************************
//
//*****************************************************************************
// Todo : Optimize futher this, we can get more speed out of here :)
//
u32 Patch___osRestoreInt()
{
TEST_DISABLE_REG_FUNCS
	gCPUState.CPUControl[C0_SR]._u64 |= gGPR[REG_a0]._u64;

	// Check next interrupt, otherwise Doom64 and other games won't boot.
	//
	if (gCPUState.CPUControl[C0_SR]._u32_0 & gCPUState.CPUControl[C0_CAUSE]._u32_0 & CAUSE_IPMASK)
	{
		gCPUState.AddJob( CPU_CHECK_INTERRUPTS );
	}

	return PATCH_RET_JR_RA;
}

//*****************************************************************************
//
//*****************************************************************************
u32 Patch_osGetCount()
{
TEST_DISABLE_REG_FUNCS
	// Why is this 32bit? See R4300.cpp
	gGPR[REG_v0]._s64 = (s64)gCPUState.CPUControl[C0_COUNT]._s32_0;	

	return PATCH_RET_JR_RA;
}

//*****************************************************************************
//
//*****************************************************************************
u32 Patch___osGetCause()
{
TEST_DISABLE_REG_FUNCS
	// Why is this 32bit? See R4300.cpp
	gGPR[REG_v0]._s64 = (s64)gCPUState.CPUControl[C0_CAUSE]._s32_0;

	return PATCH_RET_JR_RA;
}

//*****************************************************************************
//
//*****************************************************************************
u32 Patch___osSetCompare()
{
TEST_DISABLE_REG_FUNCS

	CPU_SetCompare(gGPR[REG_a0]._u64);

	return PATCH_RET_JR_RA;
}

//*****************************************************************************
//
//*****************************************************************************
u32 Patch___osSetSR()
{
TEST_DISABLE_REG_FUNCS

	R4300_SetSR(gGPR[REG_a0]._u32_0);

	//DBGConsole_Msg(0, "__osSetSR()");
	
	return PATCH_RET_JR_RA;
}

//*****************************************************************************
//
//*****************************************************************************
u32 Patch___osGetSR()
{
TEST_DISABLE_REG_FUNCS
	// Why is this 32bit?
	gGPR[REG_v0]._s64 = (s64)gCPUState.CPUControl[C0_SR]._s32_0;

	//DBGConsole_Msg(0, "__osGetSR()");
	
	return PATCH_RET_JR_RA;
}

//*****************************************************************************
//
//*****************************************************************************
u32 Patch___osSetFpcCsr()
{
TEST_DISABLE_REG_FUNCS
	// Why is the CFC1 32bit?
	gGPR[REG_v0]._s64 = (s64)gCPUState.FPUControl[31]._s32_0;
	gCPUState.FPUControl[31] = gGPR[REG_a0];
	
	DBGConsole_Msg(0, "__osSetFpcCsr()");

	return PATCH_RET_JR_RA;
}

