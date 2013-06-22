
#define TEST_DISABLE_AI_FUNCS //DAEDALUS_PROFILE(__FUNCTION__);
//*****************************************************************************
//
//*****************************************************************************
u32 Patch_osAiGetLength()
{
TEST_DISABLE_AI_FUNCS
	// Hardcoding 2880 here causes Aerogauge to get 40%+ speed up, yammy!
	gGPR[REG_v0]._s64 = (s64)Memory_AI_GetRegister(AI_LEN_REG);

	return PATCH_RET_JR_RA;
}
//*****************************************************************************
//
//*****************************************************************************
inline bool IsAiDeviceBusy()
{
	u32 status = Memory_AI_GetRegister( AI_STATUS_REG );

	if (status & (AI_STATUS_DMA_BUSY | AI_STATUS_FIFO_FULL))
		return true;
	else
		return false;
}

//*****************************************************************************
//
//*****************************************************************************
u32 Patch_osAiSetNextBuffer()
{
TEST_DISABLE_AI_FUNCS
	u32 addr = gGPR[REG_a0]._u32_0;
	u32 len  = gGPR[REG_a1]._u32_0;

	// If Ai interface is busy, stop the dma operation, can this happen??
	DAEDALUS_ASSERT( IsAiDeviceBusy()==0, "Warning: AI Interace is busy, can't DMA'd" );

	//DBGConsole_Msg(0, "osAiNextBuffer() %08X len %d bytes",addr,len);

	Memory_AI_SetRegister( AI_LEN_REG, len );
	Memory_AI_SetRegister( AI_DRAM_ADDR_REG, addr );

	DAEDALUS_ASSERT( gAudioPlugin, "Audio plugin is not initialized");
	gAudioPlugin->LenChanged();

	// Return 0 if succesfully DMA'd audio, otherwise -1 if busy
	gGPR[REG_v0]._s64 = 0;

	return PATCH_RET_JR_RA;
}

//*****************************************************************************
//
//*****************************************************************************
////// FIXME: Not implemented fully, we are missing it from the symbol table :(
u32 Patch_osAiSetFrequency()
{
TEST_DISABLE_AI_FUNCS
	return PATCH_RET_NOT_PROCESSED;

	//u32 freg = gGPR[REG_a0]._u32_0;

	//DBGConsole_Msg(0, "osAiSetFrequency(%d)", freg);
	//gGPR[REG_v1]._u64 = freg;

	//return PATCH_RET_JR_RA;
}

//*****************************************************************************
//
//*****************************************************************************
u32 Patch___osAiDeviceBusy()
{
TEST_DISABLE_AI_FUNCS
	gGPR[REG_v0]._s64 = (s64)IsAiDeviceBusy();

	return PATCH_RET_JR_RA;
}

