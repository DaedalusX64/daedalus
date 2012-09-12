#define TEST_DISABLE_PI_FUNCS //return PATCH_RET_NOT_PROCESSED;



u32 Patch___osPiCreateAccessQueue()
{
TEST_DISABLE_PI_FUNCS

#ifdef DAED_OS_MESSAGE_QUEUES
	Write32Bits(VAR_ADDRESS(osPiAccessQueueCreated), 1);

	DBGConsole_Msg(0, "Creating Pi Access Queue");

	OS_HLE_osCreateMesgQueue(VAR_ADDRESS(osPiAccessQueue), VAR_ADDRESS(osPiAccessQueueBuffer), 1);

	//u32 dwQueue     = gGPR[REG_a0]._u32_0;
	//u32 dwMsg       = gGPR[REG_a1]._u32_0;
	//u32 dwBlockFlag = gGPR[REG_a2]._u32_0;

	gGPR[REG_a0]._s64 = (s64)(s32)VAR_ADDRESS(osPiAccessQueue);
	gGPR[REG_a1]._s64 = 0;		// Msg value is unimportant
	gGPR[REG_a2]._s64 = (s64)(s32)OS_MESG_NOBLOCK;
	

	return Patch_osSendMesg();

	//return PATCH_RET_JR_RA;

#else
	return PATCH_RET_NOT_PROCESSED;
#endif
}



u32 Patch___osPiGetAccess()
{
TEST_DISABLE_PI_FUNCS
	u32 created = Read32Bits(VAR_ADDRESS(osPiAccessQueueCreated));

	if (created == 0)
	{
		Patch___osPiCreateAccessQueue();	// Ignore return
	}
	
	gGPR[REG_a0]._s64 = (s64)(s32)VAR_ADDRESS(osPiAccessQueue);
	gGPR[REG_a1]._s64 = gGPR[REG_sp]._s64 - 4;		// Place on stack and ignore
	gGPR[REG_a2]._s64 = (s64)(s32)OS_MESG_BLOCK;
	

	return Patch_osRecvMesg();
}

u32 Patch___osPiRelAccess()
{
TEST_DISABLE_PI_FUNCS
	gGPR[REG_a0]._s64 = (s64)(s32)VAR_ADDRESS(osPiAccessQueue);
	gGPR[REG_a1]._s64 = (s64)0;		// Place on stack and ignore
	gGPR[REG_a2]._s64 = (s64)(s32)OS_MESG_NOBLOCK;
	
	return Patch_osSendMesg();
}

inline bool IsPiDeviceBusy()
{
	u32 status = Memory_PI_GetRegister( PI_STATUS_REG );

	if (status & (PI_STATUS_DMA_BUSY | PI_STATUS_IO_BUSY))
		return true;
	else
		return false;
}

u32 Patch_osPiRawStartDma()
{
TEST_DISABLE_PI_FUNCS
	u32 RWflag = gGPR[REG_a0]._u32_0;
	u32 PiAddr = gGPR[REG_a1]._u32_0;
	u32 VAddr  = gGPR[REG_a2]._u32_0;
	u32 len    = gGPR[REG_a3]._u32_0;

	DAEDALUS_ASSERT( !IsPiDeviceBusy(), "Pi Device is BUSY, Need to handle!");
	
	/*
	if (IsPiDeviceBusy())
	{
		gGPR[REG_v0]._u32_0 = ~0;
		return PATCH_RET_JR_RA;
	}
	*/

	u32 PAddr = ConvertToPhysics(VAddr);

	Memory_PI_SetRegister(PI_CART_ADDR_REG, (PiAddr & 0x0fffffff) | 0x10000000);
	Memory_PI_SetRegister(PI_DRAM_ADDR_REG, PAddr);

	len--;

	if(RWflag == OS_READ)
	{
		Memory_PI_SetRegister(PI_WR_LEN_REG, len);
		DMA_PI_CopyToRDRAM();
	}
	else
	{
		Memory_PI_SetRegister(PI_RD_LEN_REG, len);
		DMA_PI_CopyFromRDRAM();
	}

	gGPR[REG_v0]._u32_0 = 0;

	return PATCH_RET_JR_RA;
}
