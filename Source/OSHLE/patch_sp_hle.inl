#define TEST_DISABLE_SP_FUNCS DAEDALUS_PROFILE(__FUNCTION__);

//*****************************************************************************
//
//*****************************************************************************
inline bool IsSpDeviceBusy()
{
	u32 status = Memory_SP_GetRegister( SP_STATUS_REG );

	if (status & (SP_STATUS_IO_FULL | SP_STATUS_DMA_FULL | SP_STATUS_DMA_BUSY))
		return true;
	else
		return false;
}
//*****************************************************************************
//
//*****************************************************************************
inline u32 SpGetStatus()
{
	return Memory_SP_GetRegister( SP_STATUS_REG );
}

//*****************************************************************************
//
//*****************************************************************************
u32 Patch___osSpRawStartDma()
{
TEST_DISABLE_SP_FUNCS
	u32 RWflag = gGPR[REG_a0]._u32_0;
	u32 SPAddr = gGPR[REG_a1]._u32_0;
	u32 VAddr  = gGPR[REG_a2]._u32_0;
	u32 len    = gGPR[REG_a3]._u32_0;

	/*
	DBGConsole_Msg(0, "osSpRawStartDma(%d, 0x%08x, 0x%08x (0x%08x), %d)", 
		RWflag,
		SPAddr,
		VAddr, PAddr,
		len);
	*/

	DAEDALUS_ASSERT( !IsSpDeviceBusy(), "Sp Device is BUSY, Need to handle!");
	/*
	if (IsSpDeviceBusy())
	{
		gGPR[REG_v0]._u32_0 = ~0;
		return PATCH_RET_JR_RA;
	}
	*/
	u32 PAddr = ConvertToPhysics(VAddr);

	//FIXME
	DAEDALUS_ASSERT( PAddr,"Address Translation necessary!");

	Memory_SP_SetRegister( SP_MEM_ADDR_REG, SPAddr);
	Memory_SP_SetRegister( SP_DRAM_ADDR_REG, PAddr);

	// Decrement.. since when DMA'ing to/from SP we increase SP len by 1
	len--;
		
	// This is correct - SP_WR_LEN_REG is a read (from RDRAM to device!)
	if (RWflag == OS_READ)  
	{
		Memory_SP_SetRegister( SP_WR_LEN_REG, len );
		DMA_SP_CopyToRDRAM();
	}
	else
	{
		Memory_SP_SetRegister( SP_RD_LEN_REG, len );
		DMA_SP_CopyFromRDRAM();
	}

	gGPR[REG_v0]._u32_0 = 0;

	return PATCH_RET_JR_RA;
}

//*****************************************************************************
//
//*****************************************************************************
u32 Patch___osSpDeviceBusy_Mario()
{
TEST_DISABLE_SP_FUNCS
	
	gGPR[REG_v0]._u32_0 = IsSpDeviceBusy();

	return PATCH_RET_JR_RA;
}

//*****************************************************************************
//
//*****************************************************************************
// Identical, but optimised
u32 Patch___osSpDeviceBusy_Rugrats()
{
TEST_DISABLE_SP_FUNCS

	gGPR[REG_v0]._u32_0 = IsSpDeviceBusy();

	return PATCH_RET_JR_RA;
}

//*****************************************************************************
//
//*****************************************************************************
// Very similar to osSpDeviceBusy, 
// Used in Pokemon Stadium 1
u32 Patch___osSpGetStatus_Mario()
{
TEST_DISABLE_SP_FUNCS

	gGPR[REG_v0]._u32_0 = SpGetStatus();

	return PATCH_RET_JR_RA;
}

//*****************************************************************************
//
//*****************************************************************************
// Ogre Battle uses this
u32 Patch___osSpGetStatus_Rugrats()
{
TEST_DISABLE_SP_FUNCS
	
	gGPR[REG_v0]._u32_0 = SpGetStatus();

	return PATCH_RET_JR_RA;
}

extern void MemoryUpdateSPStatus( u32 flags );
//*****************************************************************************
//
//*****************************************************************************
u32 Patch___osSpSetStatus_Mario()
{
TEST_DISABLE_SP_FUNCS
	u32 status = gGPR[REG_a0]._u32_0;
	
	MemoryUpdateSPStatus( status );
	return PATCH_RET_JR_RA;
}

//*****************************************************************************
//
//*****************************************************************************
u32 Patch___osSpSetStatus_Rugrats()
{
TEST_DISABLE_SP_FUNCS
	u32 status = gGPR[REG_a0]._u32_0;

	MemoryUpdateSPStatus( status );
	return PATCH_RET_JR_RA;
}

//*****************************************************************************
//
//*****************************************************************************
u32 Patch___osSpSetPc()
{
TEST_DISABLE_SP_FUNCS
	u32 pc = gGPR[REG_a0]._u32_0;

	//DBGConsole_Msg(0, "__osSpSetPc(0x%08x)", pc);
	
	u32 status = SpGetStatus();

	if (status & SP_STATUS_HALT)
	{
		// Halted, we can safely set the pc:
		Memory_PC_SetRegister(SP_PC_REG, pc);

		gGPR[REG_v0]._u32_0 = 0;
	}
	else
	{
		gGPR[REG_v0]._u32_0 = ~0;
	}

	return PATCH_RET_JR_RA;
}

//*****************************************************************************
//
//*****************************************************************************
// Translate task...
u32 Patch_osSpTaskLoad()
{
TEST_DISABLE_SP_FUNCS
	u32 task = gGPR[REG_a0]._u32_0;
	/*u32 status = SpGetStatus();

	if ((status & SP_STATUS_HALT) == 0 ||
		IsSpDeviceBusy())
	{
		DBGConsole_Msg(0, "Sp Device is not HALTED, or is busy");
		// We'd have to loop, and we can't do this...
		return PATCH_RET_NOT_PROCESSED;
	}*/
	DAEDALUS_ASSERT( (SpGetStatus() & SP_STATUS_HALT), "Sp Device is not HALTED, Need to handle!");
	DAEDALUS_ASSERT( !IsSpDeviceBusy(), "Sp Device is BUSY, Need to handle!");

	u32 temp = VAR_ADDRESS(osSpTaskLoadTempTask);
	OSTask * pSrcTask = (OSTask *)ReadAddress(task);
	OSTask * pDstTask = (OSTask *)ReadAddress(temp);



	// Translate virtual addresses to physical...
	memcpy_vfpu_BE(pDstTask, pSrcTask, sizeof(OSTask));
	
	if (pDstTask->t.ucode != 0)
		pDstTask->t.ucode = (u64 *)ConvertToPhysics((u32)pDstTask->t.ucode);

	if (pDstTask->t.ucode_data != 0)
		pDstTask->t.ucode_data = (u64 *)ConvertToPhysics((u32)pDstTask->t.ucode_data);

	if (pDstTask->t.dram_stack != 0)
		pDstTask->t.dram_stack = (u64 *)ConvertToPhysics((u32)pDstTask->t.dram_stack);
	
	if (pDstTask->t.output_buff != 0)
		pDstTask->t.output_buff = (u64 *)ConvertToPhysics((u32)pDstTask->t.output_buff);
	
	if (pDstTask->t.output_buff_size != 0)
		pDstTask->t.output_buff_size = (u64 *)ConvertToPhysics((u32)pDstTask->t.output_buff_size);
	
	if (pDstTask->t.data_ptr != 0)
		pDstTask->t.data_ptr = (u64 *)ConvertToPhysics((u32)pDstTask->t.data_ptr);
	
	if (pDstTask->t.yield_data_ptr != 0)
		pDstTask->t.yield_data_ptr = (u64 *)ConvertToPhysics((u32)pDstTask->t.yield_data_ptr);

	// If yielded, use the yield data info
	if (pSrcTask->t.flags & OS_TASK_YIELDED)
	{
		pDstTask->t.ucode_data = pDstTask->t.yield_data_ptr;
		pDstTask->t.ucode_data_size = pDstTask->t.yield_data_size;

		pSrcTask->t.flags &= ~(OS_TASK_YIELDED);
	}

	// Writeback the DCache for pDstTask
	Memory_SP_SetRegisterBits( SP_STATUS_REG, ~(SP_STATUS_SIG2|SP_STATUS_SIG1|SP_STATUS_SIG0), SP_STATUS_INTR_BREAK );

	// Set the PC
	Memory_PC_SetRegister(SP_PC_REG, 0x04001000);

	// Copy the task info to dmem
	Memory_SP_SetRegister(SP_MEM_ADDR_REG, 0x04000fc0);
	Memory_SP_SetRegister(SP_DRAM_ADDR_REG, temp);
	Memory_SP_SetRegister(SP_RD_LEN_REG, 64 - 1);
	DMA_SP_CopyFromRDRAM();

	//Memory_MI_SetRegisterBits(MI_INTR_REG, MI_INTR_SP);
	//R4300_Interrupt_UpdateCause3();


	//Memory_SP_SetRegisterBits(SP_STATUS_REG, SP_STATUS_HALT);	// Or just set entire?
	//CPU_SelectCore();

	// We can load the task directly here, but this causes a few bugs
	// when using audio/gfx plugins that expect the 0x04000fc0 to be set up
	// correctly
	//RSP_HLE_ProcessTask();

	// We know that we're not busy!
	Memory_SP_SetRegister(SP_MEM_ADDR_REG, 0x04001000);
	Memory_SP_SetRegister(SP_DRAM_ADDR_REG, (u32)pDstTask->t.ucode_boot);//	-> Translate boot ucode to physical address!
	Memory_SP_SetRegister(SP_RD_LEN_REG, pDstTask->t.ucode_boot_size - 1);
	DMA_SP_CopyFromRDRAM();
	
	return PATCH_RET_JR_RA;
	
}

//*****************************************************************************
//
//*****************************************************************************
u32 Patch_osSpTaskStartGo()
{
TEST_DISABLE_SP_FUNCS
	DAEDALUS_ASSERT( !IsSpDeviceBusy(), "Sp Device is BUSY, Need to handle!");
	/*
	if (IsSpDeviceBusy())	// Device busy? 
	{
		// LOOP Until device not busy -
		// we can't do this, so we just exit. What we could to is
		// a speed hack and jump to the next interrupt
		DBGConsole_Msg(0, "Sp Device is BUSY, looping until not busy");
		return PATCH_RET_NOT_PROCESSED;
	}
	*/
	//DBGConsole_Msg(0, "__osSpTaskStartGo()");
	Memory_SP_SetRegisterBits( SP_STATUS_REG, ~(SP_STATUS_SSTEP|SP_STATUS_BROKE|SP_STATUS_HALT), SP_STATUS_INTR_BREAK );
	RSP_HLE_ProcessTask();

	return PATCH_RET_JR_RA;
}

//*****************************************************************************
//
//*****************************************************************************
// ToDo Implement me:
// Doesn't seem to be documented, it seems its purpose is to initialize the OSTask structure members?
// It's called quiet often, it can be worth to implement it
u32 Patch___osSpTaskLoadInitTask()
{
TEST_DISABLE_SP_FUNCS
	
	return PATCH_RET_NOT_PROCESSED;
}

//*****************************************************************************
//
//*****************************************************************************
//
//osSpTaskYield / osSpTaskYielded
//These shouldn't be called, these are function calls to osSpTaskStartGo/osSpTaskLoad
// Their purpose is to yield the (GFX) task and save its state so it can be restarted by the the above functions
// Which is already handled in Patch_osSpTaskLoad :)
//Best example can be observed IN LoZ:OOT
//
u32 Patch_osSpTaskYield_Mario()
{
TEST_DISABLE_SP_FUNCS

	gGPR[REG_v0]._u32_0 = 0;
	return PATCH_RET_JR_RA;
}

//*****************************************************************************
//
//*****************************************************************************
u32 Patch_osSpTaskYield_Rugrats()
{
TEST_DISABLE_SP_FUNCS

	gGPR[REG_v0]._u32_0 = 0;
	return PATCH_RET_JR_RA;
}

//*****************************************************************************
//
//*****************************************************************************
// Yoshi uses this
u32 Patch_osSpTaskYielded()
{
TEST_DISABLE_SP_FUNCS

	OSTask * pSrcTask = (OSTask *)ReadAddress(gGPR[REG_a0]._u32_0);

	gGPR[REG_v0]._u32_0 = (pSrcTask->t.flags & OS_TASK_YIELDED);
	return PATCH_RET_JR_RA;
}
