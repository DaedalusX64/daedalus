#define TEST_DISABLE_REG_FUNCS DAEDALUS_PROFILE(__FUNCTION__);

//*****************************************************************************
//
//*****************************************************************************
u32 Patch___osDisableInt_Mario()
{
TEST_DISABLE_REG_FUNCS
	u32 CurrSR = gCPUState.CPUControl[C0_SR]._u32;

	gCPUState.CPUControl[C0_SR]._u32 = CurrSR & ~SR_IE;
	gGPR[REG_v0]._s64 = (s64)(CurrSR & SR_IE);

	return PATCH_RET_JR_RA;
}

//*****************************************************************************
//
//*****************************************************************************
u32 Patch___osDisableInt_Zelda()
{
TEST_DISABLE_REG_FUNCS
	// Same as the above
	u32 CurrSR = gCPUState.CPUControl[C0_SR]._u32;

	gCPUState.CPUControl[C0_SR]._u32 = CurrSR & ~SR_IE;
	gGPR[REG_v0]._s64 = (s64)(CurrSR & SR_IE);

	return PATCH_RET_JR_RA;
}

//*****************************************************************************
//
//*****************************************************************************
// This patch gives alot of speed!
u32 Patch___osRestoreInt()
{
TEST_DISABLE_REG_FUNCS
	gCPUState.CPUControl[C0_SR]._u32 |= gGPR[REG_a0]._u32_0;

	// Check if interrupts are pending, fixes Doom 64
	// ToDo, check if interrupts are enabled? ERL/EXL, or call R4300_SetSR?
	if (gCPUState.CPUControl[C0_SR]._u32 & gCPUState.CPUControl[C0_CAUSE]._u32 & CAUSE_IPMASK)
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

	gGPR[REG_v0]._s64 = (s64)gCPUState.CPUControl[C0_COUNT]._u32;

	return PATCH_RET_JR_RA;
}

//*****************************************************************************
//
//*****************************************************************************
u32 Patch___osGetCause()
{
TEST_DISABLE_REG_FUNCS

	gGPR[REG_v0]._s64 = (s64)gCPUState.CPUControl[C0_CAUSE]._u32;

	return PATCH_RET_JR_RA;
}

//*****************************************************************************
//
//*****************************************************************************
u32 Patch___osSetCompare()
{
TEST_DISABLE_REG_FUNCS

	CPU_SetCompare( gGPR[REG_a0]._u32_0 );

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

	gGPR[REG_v0]._s64 = (s64)gCPUState.CPUControl[C0_SR]._u32;
	//DBGConsole_Msg(0, "__osGetSR()");

	return PATCH_RET_JR_RA;
}

//*****************************************************************************
//
//*****************************************************************************
u32 Patch___osSetFpcCsr()
{
TEST_DISABLE_REG_FUNCS
	gGPR[REG_v0]._s64 = (s64)gCPUState.FPUControl[31]._u32;

	gCPUState.FPUControl[31]._u32 = gGPR[REG_a0]._u32_0;
	DBGConsole_Msg(0, "__osSetFpcCsr()");

	return PATCH_RET_JR_RA;
}

