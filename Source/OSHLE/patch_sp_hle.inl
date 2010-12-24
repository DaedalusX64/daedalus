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

	u32 PAddr = ConvertToPhysics(VAddr);

	/*
	DBGConsole_Msg(0, "osSpRawStartDma(%d, 0x%08x, 0x%08x (0x%08x), %d)", 
		RWflag,
		SPAddr,
		VAddr, PAddr,
		len);
		*/

	if (IsSpDeviceBusy())
	{
		gGPR[REG_v0]._s64 = (s64)(s32)~0;
	}
	else
	{
#ifndef DAEDALUS_SILENT
		if (PAddr == 0)
		{
			//FIXME
			DBGConsole_Msg(0, "Address Translation necessary!");
		}
#endif
		Memory_SP_SetRegister( SP_MEM_ADDR_REG, SPAddr);
		Memory_SP_SetRegister( SP_DRAM_ADDR_REG, PAddr);
		
		if (RWflag == OS_READ)  
		{
			Write32Bits( SP_WR_LEN_REG | 0xA0000000, len - 1 );
		}
		else
		{
			Write32Bits( SP_RD_LEN_REG | 0xA0000000, len - 1 );
		}

		gGPR[REG_v0]._u64 = 0;
	}

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
u32 Patch___osSpGetStatus_Mario()
{
TEST_DISABLE_SP_FUNCS
	u32 status = SpGetStatus();

	gGPR[REG_v0]._s64 = (s64)(s32)status;
	return PATCH_RET_JR_RA;
}

//*****************************************************************************
//
//*****************************************************************************
u32 Patch___osSpGetStatus_Rugrats()
{
TEST_DISABLE_SP_FUNCS
	u32 status = SpGetStatus();

	gGPR[REG_v0]._s64 = (s64)(s32)status;
	return PATCH_RET_JR_RA;
}

//*****************************************************************************
//
//*****************************************************************************
u32 Patch___osSpSetStatus_Mario()
{
TEST_DISABLE_SP_FUNCS
	u32 status = gGPR[REG_a0]._u32_0;

	//Memory_SP_SetRegister(SP_STATUS_REG, status);
	Write32Bits(0xa4040010, status);
	return PATCH_RET_JR_RA;
}

//*****************************************************************************
//
//*****************************************************************************
u32 Patch___osSpSetStatus_Rugrats()
{
TEST_DISABLE_SP_FUNCS
	u32 status = gGPR[REG_a0]._u32_0;

	//Memory_SP_SetRegister(SP_STATUS_REG, status);
	Write32Bits(0xa4040010, status);
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
		gRSPState.CurrentPC = pc;

		gGPR[REG_v0]._s64 = (s64)(s32)0;
	}
	else
	{
		gGPR[REG_v0]._s64 = (s64)(s32)~0;
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
	u32 status = SpGetStatus();

	if ((status & SP_STATUS_HALT) == 0 ||
		IsSpDeviceBusy())
	{
		DBGConsole_Msg(0, "Sp Device is not HALTED, or is busy");
		// We'd have to loop, and we can't do this...
		return PATCH_RET_NOT_PROCESSED;
	}
	
	OSTask * pSrcTask = (OSTask *)ReadAddress(task);
	OSTask * pDstTask = (OSTask *)ReadAddress(VAR_ADDRESS(osSpTaskLoadTempTask));

	// Translate virtual addresses to physical...
	memcpy(pDstTask, pSrcTask, sizeof(OSTask));

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

	Write32Bits(PHYS_TO_K1(SP_STATUS_REG),
		SP_CLR_SIG2|SP_CLR_SIG1|SP_CLR_SIG0|SP_SET_INTR_BREAK);	


	// Set the PC
	Write32Bits(PHYS_TO_K1(SP_PC_REG), 0x04001000);

	// Copy the task info to dmem
	Write32Bits(PHYS_TO_K1(SP_MEM_ADDR_REG), 0x04000fc0);
	Write32Bits(PHYS_TO_K1(SP_DRAM_ADDR_REG), VAR_ADDRESS(osSpTaskLoadTempTask));
	Write32Bits(PHYS_TO_K1(SP_RD_LEN_REG), 64 - 1);


	//Memory_MI_SetRegisterBits(MI_INTR_REG, MI_INTR_SP);
	//R4300_Interrupt_UpdateCause3();


	//Memory_SP_SetRegisterBits(SP_STATUS_REG, SP_STATUS_HALT);	// Or just set entire?
	//CPU_SelectCore();

	// We can load the task directly here, but this causes a few bugs
	// when using audio/gfx plugins that expect the 0x04000fc0 to be set up
	// correctly
	//RSP_HLE_ProcessTask();

	// We know that we're not busy!
	Write32Bits(PHYS_TO_K1(SP_MEM_ADDR_REG), 0x04001000);
	Write32Bits(PHYS_TO_K1(SP_DRAM_ADDR_REG), (u32)pDstTask->t.ucode_boot);//	-> Translate boot ucode to physical address!
	Write32Bits(PHYS_TO_K1(SP_RD_LEN_REG), pDstTask->t.ucode_boot_size - 1);
	
	return PATCH_RET_JR_RA;
	
}

//*****************************************************************************
//
//*****************************************************************************
u32 Patch_osSpTaskStartGo()
{
TEST_DISABLE_SP_FUNCS
	// Device busy? 
	if (IsSpDeviceBusy())
	{
		// LOOP Until device not busy -
		// we can't do this, so we just exit. What we could to is
		// a speed hack and jump to the next interrupt

		return PATCH_RET_NOT_PROCESSED;
	}

	//DBGConsole_Msg(0, "__osSpTaskStartGo()");

	//Memory_SP_SetRegister(SP_STATUS_REG, (SP_SET_INTR_BREAK|SP_CLR_SSTEP|SP_CLR_BROKE|SP_CLR_HALT));
	Write32Bits(PHYS_TO_K1(SP_STATUS_REG), (SP_SET_INTR_BREAK|SP_CLR_SSTEP|SP_CLR_BROKE|SP_CLR_HALT));


	//RSP_HLE_ProcessTask();

	return PATCH_RET_JR_RA;

}

//*****************************************************************************
//
//*****************************************************************************
//
// This is really weird, In Mario 64 osSpTaskYield_Mario / osSpTaskYielded most of the time isn't patched..
// There seems to be a bug in our scanning procedure? Maybe do two phase to be sure we find all the symbols / variables.
//

u32 Patch_osSpTaskYield_Mario()
{
	DAEDALUS_ERROR("osSpTaskYield_Mario");
	gGPR[REG_v0]._s64 = 0;

	return PATCH_RET_JR_RA;
}

//*****************************************************************************
//
//*****************************************************************************
u32 Patch_osSpTaskYield_Rugrats()
{
	DAEDALUS_ERROR("osSpTaskYield_Rugrats");
	gGPR[REG_v0]._s64 = 0;

	return PATCH_RET_JR_RA;
}

//*****************************************************************************
//
//*****************************************************************************
u32 Patch_osSpTaskYielded()
{
	DAEDALUS_ERROR("SpTaskYielded");
	gGPR[REG_v0]._s64 = 1; // OS_TASK_YIELDED

	return PATCH_RET_JR_RA;
}

