#define TEST_DISABLE_THREAD_FUNCS DAEDALUS_PROFILE(__FUNCTION__);
//*****************************************************************************
//
//*****************************************************************************
u32 Patch_osCreateThread_Mario()
{
TEST_DISABLE_THREAD_FUNCS
	u32 thread = gGPR[REG_a0]._u32_0;
	u32 id     = gGPR[REG_a1]._u32_0;
	u32 func  = gGPR[REG_a2]._u32_0;
	u32 arg    = gGPR[REG_a3]._u32_0;

	// Other variables are on the stack - dig them out!
	u8 * pStackBase	  = (u8 *)ReadAddress(gGPR[REG_sp]._u32_0);

	// Stack is arg 4
	u32 stack = QuickRead32Bits(pStackBase, 4*4);

	// Pri is arg 5
	u32 pri = QuickRead32Bits(pStackBase, 4*5);

	DBGConsole_Msg(0, "[WosCreateThread](0x%08x, %d, 0x%08x(), 0x%08x, 0x%08x, %d)",
		thread, id, func, arg, stack, pri );

	// fp used - we now HLE the Cop1 Unusable exception and set this
	// when the thread first accesses the FP unit
	Write32Bits(thread + offsetof(OSThread, fp), 0);						// pThread->fp


	Write16Bits(thread + offsetof(OSThread, state), OS_STATE_STOPPED);	// pThread->state
	Write16Bits(thread + offsetof(OSThread, flags), 0);					// pThread->flags

	Write32Bits(thread + offsetof(OSThread, id), id);
	Write32Bits(thread + offsetof(OSThread, priority), pri);


	Write32Bits(thread + offsetof(OSThread, next), 0);					// pThread->next
	Write32Bits(thread + offsetof(OSThread, queue), 0);					// Queue
	Write32Bits(thread + offsetof(OSThread, context.pc), func);		// state.pc

	s64 sArg = (s64)(s32)arg;
	Write64Bits(thread + offsetof(OSThread, context.a0), sArg);			// a0

	s64 sStack = (s64)(s32)stack;
	Write64Bits(thread + offsetof(OSThread, context.sp), sStack - 16);	// sp (sub 16 for a0 arg etc)

	s64 ra = (s64)(s32)VAR_ADDRESS(osThreadDieRA);
	Write64Bits(thread + offsetof(OSThread, context.ra), ra);			// ra

	Write32Bits(thread + offsetof(OSThread, context.sr), (SR_IMASK|SR_EXL|SR_IE));					// state.sr
	Write32Bits(thread + offsetof(OSThread, context.rcp), (OS_IM_ALL & RCP_IMASK)>>RCP_IMASKSHIFT);	// state.rcp
	Write32Bits(thread + offsetof(OSThread, context.fpcsr), (FPCSR_FS|FPCSR_EV));	// state.fpcsr

	// Set us as head of global list
	u32 NextThread = Read32Bits(VAR_ADDRESS(osGlobalThreadList));
	Write32Bits(thread + offsetof(OSThread, tlnext), NextThread);				// pThread->next
	Write32Bits(VAR_ADDRESS(osGlobalThreadList), thread);

	return PATCH_RET_JR_RA;
}
//*****************************************************************************
//
//*****************************************************************************
// Identical to Mario code - just more optimised
u32 Patch_osCreateThread_Rugrats()
{
TEST_DISABLE_THREAD_FUNCS
	return Patch_osCreateThread_Mario();
}

//*****************************************************************************
//
//*****************************************************************************
// ToDo : Implement me
u32 Patch_osSetThreadPri()
{
TEST_DISABLE_THREAD_FUNCS
	u32 thread = gGPR[REG_a0]._u32_0;
//	u32 pri    = gGPR[REG_a1]._u32_0;

	u32 ActiveThread = Read32Bits(VAR_ADDRESS(osActiveThread));

	if (thread == 0x00000000)
	{
		thread = ActiveThread;
	}

	//DBGConsole_Msg(0, "[WosSetThreadPri](0x%08x, %d) 0x%08x", thread, pri, ActiveThread);

	return PATCH_RET_NOT_PROCESSED;
}

//*****************************************************************************
//
//*****************************************************************************
u32 Patch_osGetThreadPri()
{
TEST_DISABLE_THREAD_FUNCS
	u32 thread = gGPR[REG_a0]._u32_0;
	u32 pri;

	if (thread == 0)
	{
		thread = Read32Bits(VAR_ADDRESS(osActiveThread));
	}

	pri = Read32Bits(thread + offsetof(OSThread, priority));

	gGPR[REG_v0]._s64 = (s64)(s32)pri;
	return PATCH_RET_JR_RA;
}

//*****************************************************************************
//
//*****************************************************************************
u32 Patch___osDequeueThread()
{
TEST_DISABLE_THREAD_FUNCS
	u32 queue = gGPR[REG_a0]._u32_0;
	u32 thread = gGPR[REG_a1]._u32_0;

	//DBGConsole_Msg(0, "Dequeuing Thread");

	u32 CurThread = Read32Bits(queue + 0x0);
	while (CurThread != 0)
	{
		if (CurThread == thread)
		{
			// Set the next pointer of the previous thread
			// to the next pointer of this thread
			Write32Bits(queue, Read32Bits(thread + offsetof(OSThread, next)));
			break;
		}
		else
		{
			// Set queue pointer to next in list
			queue = CurThread;
			CurThread = Read32Bits(queue + 0x0);
		}
	}

	return PATCH_RET_JR_RA;
}

//*****************************************************************************
//
//*****************************************************************************
u32 Patch___osDispatchThread_Mario()
{
TEST_DISABLE_THREAD_FUNCS
	// First pop the first thread off the stack (copy of osPopThread code):
	u32 thread = Read32Bits(VAR_ADDRESS(osThreadQueue));

	u8 * pThreadBase = (u8 *)ReadAddress(thread);

	// Update queue to point to next thread:
	Write32Bits(VAR_ADDRESS(osThreadQueue), QuickRead32Bits(pThreadBase, offsetof(OSThread, next)));

	// Set the current active thread:
	Write32Bits(VAR_ADDRESS(osActiveThread), thread);

	// Set the current thread's status to OS_STATE_RUNNING:
	Write16Bits(thread + offsetof(OSThread, state), OS_STATE_RUNNING);

#if 1	//1->better cache efficiency //Corn
	// CPU regs
	for(u32 Reg = 1; Reg < 26; Reg++)	//AT -> T9
	{
		gGPR[Reg]._u64 = QuickRead64Bits(pThreadBase, 0x0018 + (Reg << 3));
	}
	gGPR[REG_gp]._u64 = QuickRead64Bits(pThreadBase, 0x00e8);
	gGPR[REG_sp]._u64 = QuickRead64Bits(pThreadBase, 0x00f0);
	gGPR[REG_s8]._u64 = QuickRead64Bits(pThreadBase, 0x00f8);
	gGPR[REG_ra]._u64 = QuickRead64Bits(pThreadBase, 0x0100);

#else
	// Restore all registers:
	// For speed, we cache the base pointer!!!
	gGPR[REG_at]._u64 = QuickRead64Bits(pThreadBase, 0x0020);
	gGPR[REG_v0]._u64 = QuickRead64Bits(pThreadBase, 0x0028);
	gGPR[REG_v1]._u64 = QuickRead64Bits(pThreadBase, 0x0030);
	gGPR[REG_a0]._u64 = QuickRead64Bits(pThreadBase, 0x0038);
	gGPR[REG_a1]._u64 = QuickRead64Bits(pThreadBase, 0x0040);
	gGPR[REG_a2]._u64 = QuickRead64Bits(pThreadBase, 0x0048);
	gGPR[REG_a3]._u64 = QuickRead64Bits(pThreadBase, 0x0050);
	gGPR[REG_t0]._u64 = QuickRead64Bits(pThreadBase, 0x0058);
	gGPR[REG_t1]._u64 = QuickRead64Bits(pThreadBase, 0x0060);
	gGPR[REG_t2]._u64 = QuickRead64Bits(pThreadBase, 0x0068);
	gGPR[REG_t3]._u64 = QuickRead64Bits(pThreadBase, 0x0070);
	gGPR[REG_t4]._u64 = QuickRead64Bits(pThreadBase, 0x0078);
	gGPR[REG_t5]._u64 = QuickRead64Bits(pThreadBase, 0x0080);
	gGPR[REG_t6]._u64 = QuickRead64Bits(pThreadBase, 0x0088);
	gGPR[REG_t7]._u64 = QuickRead64Bits(pThreadBase, 0x0090);
	gGPR[REG_s0]._u64 = QuickRead64Bits(pThreadBase, 0x0098);
	gGPR[REG_s1]._u64 = QuickRead64Bits(pThreadBase, 0x00a0);
	gGPR[REG_s2]._u64 = QuickRead64Bits(pThreadBase, 0x00a8);
	gGPR[REG_s3]._u64 = QuickRead64Bits(pThreadBase, 0x00b0);
	gGPR[REG_s4]._u64 = QuickRead64Bits(pThreadBase, 0x00b8);
	gGPR[REG_s5]._u64 = QuickRead64Bits(pThreadBase, 0x00c0);
	gGPR[REG_s6]._u64 = QuickRead64Bits(pThreadBase, 0x00c8);
	gGPR[REG_s7]._u64 = QuickRead64Bits(pThreadBase, 0x00d0);
	gGPR[REG_t8]._u64 = QuickRead64Bits(pThreadBase, 0x00d8);
	gGPR[REG_t9]._u64 = QuickRead64Bits(pThreadBase, 0x00e0);
	gGPR[REG_gp]._u64 = QuickRead64Bits(pThreadBase, 0x00e8);
	gGPR[REG_sp]._u64 = QuickRead64Bits(pThreadBase, 0x00f0);
	gGPR[REG_s8]._u64 = QuickRead64Bits(pThreadBase, 0x00f8);
	gGPR[REG_ra]._u64 = QuickRead64Bits(pThreadBase, 0x0100);
#endif

	gCPUState.MultLo._u64 = QuickRead64Bits(pThreadBase, offsetof(OSThread, context.lo));
	gCPUState.MultHi._u64 = QuickRead64Bits(pThreadBase, offsetof(OSThread, context.hi));

	// Set the EPC
	gCPUState.CPUControl[C0_EPC]._u32 = QuickRead32Bits(pThreadBase, offsetof(OSThread, context.pc));

	// Set the STATUS register. Normally this would trigger a
	// Check for pending interrupts, but we're running in kernel mode
	// So SR_ERL or SR_EXL is probably set. Don't think that a check is
	// necessary

	u32 NewSR = QuickRead32Bits(pThreadBase, offsetof(OSThread, context.sr));

	R4300_SetSR(NewSR);

	// Don't restore CAUSE

	// Check if the FP unit was used
	u32 RestoreFP = QuickRead32Bits(pThreadBase, offsetof(OSThread, fp));
	if (RestoreFP != 0)
	{
		// Restore control reg
		gCPUState.FPUControl[31]._u32 = QuickRead32Bits(pThreadBase, offsetof(OSThread, context.fpcsr));

		// Floats - can probably optimise this to eliminate 64 bits reads...
		for (u32 FPReg = 0; FPReg < 16; FPReg++)
		{
			gCPUState.FPU[(FPReg*2)+1]._u32 = QuickRead32Bits(pThreadBase, 0x0130 + (FPReg << 3));
			gCPUState.FPU[(FPReg*2)+0]._u32 = QuickRead32Bits(pThreadBase, 0x0134 + (FPReg << 3));
		}
	}



	// Set interrupt mask...does this do anything???
	u32 rcp = QuickRead32Bits(pThreadBase, 0x0128);

	u16 TempVal = Read16Bits(VAR_ADDRESS(osDispatchThreadRCPThingamy) + (rcp*2));
	Write32Bits(PHYS_TO_K1(MI_INTR_MASK_REG), (u32)TempVal);		// MI_INTR_MASK_REG

	// Done - when we exit we should ERET
	return PATCH_RET_ERET;
}

//*****************************************************************************
//
//*****************************************************************************
// Neither of these are correct- they ignore the interrupt mask thing
u32 Patch___osDispatchThread_MarioKart()
{
TEST_DISABLE_THREAD_FUNCS
	return Patch___osDispatchThread_Mario();
}

//*****************************************************************************
//
//*****************************************************************************
u32 Patch___osDispatchThread_Rugrats()
{
TEST_DISABLE_THREAD_FUNCS

	u32 thread = Read32Bits(VAR_ADDRESS(osThreadQueue));

	u8 * pThreadBase = (u8 *)ReadAddress(thread);

	// Update queue to point to next thread:
	Write32Bits(VAR_ADDRESS(osThreadQueue), QuickRead32Bits(pThreadBase, offsetof(OSThread, next)));

	// Set the current active thread:
	Write32Bits(VAR_ADDRESS(osActiveThread), thread);

	// Set the current thread's status to OS_STATE_RUNNING:
	Write16Bits(thread + offsetof(OSThread, state), OS_STATE_RUNNING);
/*
0x80051ad0: <0x0040d021> ADDU      k0 = v0 + r0
0x80051ad4: <0x8f5b0118> LW        k1 <- 0x0118(k0)*/
	u32 k1 = QuickRead32Bits(pThreadBase, offsetof(OSThread, context.sr));

/*
0x80051ad8: <0x3c088006> LUI       t0 = 0x80060000
0x80051adc: <0x25081880> ADDIU     t0 = t0 + 0x1880
0x80051ae0: <0x8d080000> LW        t0 <- 0x0000(t0)*/
	u32 t0 = Read32Bits(VAR_ADDRESS(osInterruptMaskThingy));

/*
0x80051ae4: <0x3108ff00> ANDI      t0 = t0 & 0xff00
0x80051ae8: <0x3369ff00> ANDI      t1 = k1 & 0xff00
0x80051aec: <0x01284824> AND       t1 = t1 & t0
0x80051af0: <0x3c01ffff> LUI       at = 0xffff0000
0x80051af4: <0x342100ff> ORI       at = at | 0x00ff
0x80051af8: <0x0361d824> AND       k1 = k1 & at
0x80051afc: <0x0369d825> OR        k1 = k1 | t1
0x80051b00: <0x409b6000> MTC0      k1 -> Status*/

	t0 &= 0xFF00;
	u32 t1 = k1 & t0 & 0xFF00;

	k1 &= 0xFFFF00FF;
	k1 = k1 | t1;

	R4300_SetSR(k1);

#if 1	//1->better cache efficiency //Corn
	// CPU regs
	for(u32 Reg = 1; Reg < 26; Reg++)	//AT -> T9
	{
		gGPR[Reg]._u64 = QuickRead64Bits(pThreadBase, 0x0018 + (Reg << 3));
	}
	gGPR[REG_gp]._u64 = QuickRead64Bits(pThreadBase, 0x00e8);
	gGPR[REG_sp]._u64 = QuickRead64Bits(pThreadBase, 0x00f0);
	gGPR[REG_s8]._u64 = QuickRead64Bits(pThreadBase, 0x00f8);
	gGPR[REG_ra]._u64 = QuickRead64Bits(pThreadBase, 0x0100);

#else
	// Restore all registers:
	// For speed, we cache the base pointer!!!
	gGPR[REG_at]._u64 = QuickRead64Bits(pThreadBase, 0x0020);
	gGPR[REG_v0]._u64 = QuickRead64Bits(pThreadBase, 0x0028);
	gGPR[REG_v1]._u64 = QuickRead64Bits(pThreadBase, 0x0030);
	gGPR[REG_a0]._u64 = QuickRead64Bits(pThreadBase, 0x0038);
	gGPR[REG_a1]._u64 = QuickRead64Bits(pThreadBase, 0x0040);
	gGPR[REG_a2]._u64 = QuickRead64Bits(pThreadBase, 0x0048);
	gGPR[REG_a3]._u64 = QuickRead64Bits(pThreadBase, 0x0050);
	gGPR[REG_t0]._u64 = QuickRead64Bits(pThreadBase, 0x0058);
	gGPR[REG_t1]._u64 = QuickRead64Bits(pThreadBase, 0x0060);
	gGPR[REG_t2]._u64 = QuickRead64Bits(pThreadBase, 0x0068);
	gGPR[REG_t3]._u64 = QuickRead64Bits(pThreadBase, 0x0070);
	gGPR[REG_t4]._u64 = QuickRead64Bits(pThreadBase, 0x0078);
	gGPR[REG_t5]._u64 = QuickRead64Bits(pThreadBase, 0x0080);
	gGPR[REG_t6]._u64 = QuickRead64Bits(pThreadBase, 0x0088);
	gGPR[REG_t7]._u64 = QuickRead64Bits(pThreadBase, 0x0090);
	gGPR[REG_s0]._u64 = QuickRead64Bits(pThreadBase, 0x0098);
	gGPR[REG_s1]._u64 = QuickRead64Bits(pThreadBase, 0x00a0);
	gGPR[REG_s2]._u64 = QuickRead64Bits(pThreadBase, 0x00a8);
	gGPR[REG_s3]._u64 = QuickRead64Bits(pThreadBase, 0x00b0);
	gGPR[REG_s4]._u64 = QuickRead64Bits(pThreadBase, 0x00b8);
	gGPR[REG_s5]._u64 = QuickRead64Bits(pThreadBase, 0x00c0);
	gGPR[REG_s6]._u64 = QuickRead64Bits(pThreadBase, 0x00c8);
	gGPR[REG_s7]._u64 = QuickRead64Bits(pThreadBase, 0x00d0);
	gGPR[REG_t8]._u64 = QuickRead64Bits(pThreadBase, 0x00d8);
	gGPR[REG_t9]._u64 = QuickRead64Bits(pThreadBase, 0x00e0);
	gGPR[REG_gp]._u64 = QuickRead64Bits(pThreadBase, 0x00e8);
	gGPR[REG_sp]._u64 = QuickRead64Bits(pThreadBase, 0x00f0);
	gGPR[REG_s8]._u64 = QuickRead64Bits(pThreadBase, 0x00f8);
	gGPR[REG_ra]._u64 = QuickRead64Bits(pThreadBase, 0x0100);
#endif


	gCPUState.MultLo._u64 = QuickRead64Bits(pThreadBase, offsetof(OSThread, context.lo));
	gCPUState.MultHi._u64 = QuickRead64Bits(pThreadBase, offsetof(OSThread, context.hi));

	// Set the EPC
	gCPUState.CPUControl[C0_EPC]._u32 = QuickRead32Bits(pThreadBase, offsetof(OSThread, context.pc));


	// Check if the FP unit was used
	u32 RestoreFP = QuickRead32Bits(pThreadBase, offsetof(OSThread, fp));
	if (RestoreFP != 0)
	{
		// Restore control reg
		gCPUState.FPUControl[31]._u32 = QuickRead32Bits(pThreadBase, offsetof(OSThread, context.fpcsr));

		// Floats - can probably optimise this to eliminate 64 bits reads...
		for (u32 FPReg = 0; FPReg < 16; FPReg++)
		{
			gCPUState.FPU[(FPReg*2)+1]._u32 = QuickRead32Bits(pThreadBase, 0x0130 + (FPReg << 3));
			gCPUState.FPU[(FPReg*2)+0]._u32 = QuickRead32Bits(pThreadBase, 0x0134 + (FPReg << 3));
		}
	}
/*
0x80051be4: <0x8f5b0128> LW        k1 <- 0x0128(k0)
0x80051be8: <0x3c1a8006> LUI       k0 = 0x80060000
0x80051bec: <0x275a1880> ADDIU     k0 = k0 + 0x1880
0x80051bf0: <0x8f5a0000> LW        k0 <- 0x0000(k0)
*/
	// Set interrupt mask...does this do anything???
	u32 rcp = QuickRead32Bits(pThreadBase, 0x0128);

	u32 IntMask = Read32Bits(VAR_ADDRESS(osInterruptMaskThingy));

/*
0x80051bf4: <0x001ad402> SRL       k0 = k0 >> 0x0010
0x80051bf8: <0x037ad824> AND       k1 = k1 & k0
0x80051bfc: <0x001bd840> SLL       k1 = k1 << 0x0001
0x80051c00: <0x3c1a8006> LUI       k0 = 0x80060000
0x80051c04: <0x275a6ab0> ADDIU     k0 = k0 + 0x6ab0
0x80051c08: <0x037ad821> ADDU      k1 = k1 + k0
0x80051c0c: <0x977b0000> LHU       k1 <- 0x0000(k1)*/
	IntMask >>= 0x10;
	rcp = rcp & IntMask;

	u16 TempVal = Read16Bits(VAR_ADDRESS(osDispatchThreadRCPThingamy) + (rcp*2));

/*
0x80051c10: <0x3c1aa430> LUI       k0 = 0xa4300000
0x80051c14: <0x375a000c> ORI       k0 = k0 | 0x000c
0x80051c18: <0xaf5b0000> SW        k1 -> 0x0000(k0)
0x80051c1c: <0x00000000> NOP
0x80051c20: <0x00000000> NOP
0x80051c24: <0x00000000> NOP
0x80051c28: <0x00000000> NOP
0x80051c2c: <0x42000018> ERET*/

	Write32Bits(PHYS_TO_K1(MI_INTR_MASK_REG), (u32)TempVal);		// MI_INTR_MASK_REG

	// Done - when we exit we should ERET
	return PATCH_RET_ERET;

}

//*****************************************************************************
//
//*****************************************************************************
u32 Patch_osDestroyThread_Mario()
{
TEST_DISABLE_THREAD_FUNCS

	u32 thread = gGPR[REG_a0]._u32_0;
	u32 CurrThread;
	u32 NextThread;
	u32 ActiveThread;
	u16 state;

	ActiveThread = Read32Bits(VAR_ADDRESS(osActiveThread));

	if (thread == 0)
	{
		thread = ActiveThread;
	}
	DBGConsole_Msg(0, "osDestroyThread(0x%08x)", thread);

	state = Read16Bits(thread + offsetof(OSThread, state));
	if (state != OS_STATE_STOPPED)
	{
		u32 queue = Read32Bits(thread + offsetof(OSThread, queue));

		gGPR[REG_a0]._s64 = (s64)(s32)queue;
		gGPR[REG_a1]._s64 = (s64)(s32)thread;

		g___osDequeueThread_s.pFunction();
	}

	CurrThread = Read32Bits(VAR_ADDRESS(osGlobalThreadList));
	NextThread = Read32Bits(CurrThread + offsetof(OSThread, tlnext));

	if (thread == CurrThread)
	{
		Write32Bits(VAR_ADDRESS(osGlobalThreadList), NextThread);
	}
	else
	{
		while (NextThread != 0)
		{
			if (thread == NextThread)
			{
				Write32Bits(CurrThread + offsetof(OSThread, tlnext),
					Read32Bits(thread + offsetof(OSThread, tlnext)));
				break;
			}

			CurrThread = NextThread;
			NextThread = Read32Bits(CurrThread + offsetof(OSThread, tlnext));

		}
	}

	// If we're destorying the active thread, dispatch the next thread
	// Otherwise, just return control to the caller
	if (thread == ActiveThread)
	{
		return CALL_PATCHED_FUNCTION(__osDispatchThread);
	}
	else
	{
		return PATCH_RET_JR_RA;
	}

}

//*****************************************************************************
//
//*****************************************************************************
// ToDo : Implement me
u32 Patch_osDestroyThread_Zelda()
{
TEST_DISABLE_THREAD_FUNCS

#ifdef DAEDALUS_DEBUG_CONSOLE
	u32 thread = gGPR[REG_a0]._u32_0;
	use(thread);
	DBGConsole_Msg(0, "osDestroyThread(0x%08x)", thread);
#endif

	return PATCH_RET_NOT_PROCESSED0(osDestroyThread);
}

//*****************************************************************************
//
//*****************************************************************************
u32 Patch___osEnqueueThread_Mario()
{
TEST_DISABLE_THREAD_FUNCS
	u32 queue = gGPR[REG_a0]._u32_0;
	u32 thread = gGPR[REG_a1]._u32_0;
	u32 ThreadPri = Read32Bits(thread + 0x4);

	//DBGConsole_Msg(0, "osEnqueueThread(queue = 0x%08x, thread = 0x%08x)", queue, thread);
	//DBGConsole_Msg(0, "  thread->priority = 0x%08x", ThreadPri);

	u32 t9 = queue;

	u32 CurThread = Read32Bits(t9);
	u32 CurThreadPri = Read32Bits(CurThread + 0x4);

	//DBGConsole_Msg(0, curthread = 0x%08x, curthread->priority = 0x%08x", CurThread, CurThreadPri);

	while ((s32)CurThreadPri >= (s32)ThreadPri)
	{
		t9 = CurThread;
		CurThread = Read32Bits(CurThread + 0x0);		// Get next thread
		// Check if CurThread is null there?
		CurThreadPri = Read32Bits(CurThread + 0x4);
		//DBGConsole_Msg(0, "  curthread = 0x%08x, curthread->priority = 0x%08x", CurThread, CurThreadPri);
	}

	CurThread = Read32Bits(t9);
	Write32Bits(thread + 0x0, CurThread);	// Set thread->next
	Write32Bits(thread + 0x8, queue);		// Set thread->queue
	Write32Bits(t9, thread);				// Set prevthread->next

	return PATCH_RET_JR_RA;
}

//*****************************************************************************
//
//*****************************************************************************
// Identical - just different compilation
u32 Patch___osEnqueueThread_Rugrats()
{
TEST_DISABLE_THREAD_FUNCS
	u32 queue = gGPR[REG_a0]._u32_0;
	u32 thread = gGPR[REG_a1]._u32_0;
	u32 ThreadPri = Read32Bits(thread + 0x4);

	//DBGConsole_Msg(0, "osEnqueueThread(queue = 0x%08x, thread = 0x%08x)", queue, thread);
	//DBGConsole_Msg(0, "  thread->priority = 0x%08x", ThreadPri);

	u32 t9 = queue;

	u32 CurThread = Read32Bits(t9);
	u32 CurThreadPri = Read32Bits(CurThread + 0x4);

	//DBGConsole_Msg(0, curthread = 0x%08x, curthread->priority = 0x%08x", CurThread, CurThreadPri);

	while ((s32)CurThreadPri >= (s32)ThreadPri)
	{
		t9 = CurThread;
		CurThread = Read32Bits(CurThread + 0x0);		// Get next thread
		// Check if CurThread is null there?
		CurThreadPri = Read32Bits(CurThread + 0x4);
		//DBGConsole_Msg(0, "  curthread = 0x%08x, curthread->priority = 0x%08x", CurThread, CurThreadPri);
	}

	CurThread = Read32Bits(t9);
	Write32Bits(thread + 0x0, CurThread);	// Set thread->next
	Write32Bits(thread + 0x8, queue);		// Set thread->queue
	Write32Bits(t9, thread);				// Set prevthread->next

	return PATCH_RET_JR_RA;
}

//*****************************************************************************
//
//*****************************************************************************
// Gets active thread in a1. Adds to queue in a0 (if specified), dispatches
u32 Patch___osEnqueueAndYield_Mario()
{
TEST_DISABLE_THREAD_FUNCS
	u32 queue = gGPR[REG_a0]._u32_0;
	// Get the active thread
	u32 thread = Read32Bits(VAR_ADDRESS(osActiveThread));
	u8 * pThreadBase = (u8 *)ReadAddress(thread);

	//DBGConsole_Msg(0, "EnqueueAndYield()");

	// Set a1 (necessary if we call osEnqueueThread
	gGPR[REG_a1]._s64 = (s64)(s32)thread;

	// Store various registers:
	// For speed, we cache the base pointer!!!

	u32 status = gCPUState.CPUControl[C0_SR]._u32;

	status |= SR_EXL;

	QuickWrite32Bits(pThreadBase, 0x118, status);

	QuickWrite64Bits(pThreadBase, 0x0098, gGPR[REG_s0]._u64);
	QuickWrite64Bits(pThreadBase, 0x00a0, gGPR[REG_s1]._u64);
	QuickWrite64Bits(pThreadBase, 0x00a8, gGPR[REG_s2]._u64);
	QuickWrite64Bits(pThreadBase, 0x00b0, gGPR[REG_s3]._u64);
	QuickWrite64Bits(pThreadBase, 0x00b8, gGPR[REG_s4]._u64);
	QuickWrite64Bits(pThreadBase, 0x00c0, gGPR[REG_s5]._u64);
	QuickWrite64Bits(pThreadBase, 0x00c8, gGPR[REG_s6]._u64);
	QuickWrite64Bits(pThreadBase, 0x00d0, gGPR[REG_s7]._u64);

	QuickWrite64Bits(pThreadBase, 0x00e8, gGPR[REG_gp]._u64);
	QuickWrite64Bits(pThreadBase, 0x00f0, gGPR[REG_sp]._u64);
	QuickWrite64Bits(pThreadBase, 0x00f8, gGPR[REG_s8]._u64);
	QuickWrite64Bits(pThreadBase, 0x0100, gGPR[REG_ra]._u64);

	QuickWrite32Bits(pThreadBase, 0x011c, gGPR[REG_ra]._u32_0);

	// Check if the FP unit was used
	u32 RestoreFP = QuickRead32Bits(pThreadBase, 0x0018);
	if (RestoreFP != 0)
	{
		// Save control reg
		QuickWrite32Bits(pThreadBase, 0x012c, gCPUState.FPUControl[31]._u32);

		// Floats - can probably optimise this to eliminate 64 bits writes...
		for (u32 FPReg = 0; FPReg < 16; FPReg++)
		{
			QuickWrite32Bits(pThreadBase, 0x0130 + (FPReg * 8), gCPUState.FPU[(FPReg*2)+1]._u32);
			QuickWrite32Bits(pThreadBase, 0x0134 + (FPReg * 8), gCPUState.FPU[(FPReg*2)+0]._u32);
		}
	}

	// Set interrupt mask...does this do anything???
	u32 rcp = Memory_MI_GetRegister( MI_INTR_MASK_REG );
	QuickWrite32Bits(pThreadBase, 0x128,  rcp);

	// Call EnqueueThread if queue is set
	if (queue != 0)
	{
		//a0/a1 set already
		CALL_PATCHED_FUNCTION(__osEnqueueThread);
	}

	return CALL_PATCHED_FUNCTION(__osDispatchThread);
}

//*****************************************************************************
//
//*****************************************************************************
u32 Patch___osEnqueueAndYield_MarioKart()
{
TEST_DISABLE_THREAD_FUNCS
	return Patch___osEnqueueAndYield_Mario();

}

//*****************************************************************************
//
//*****************************************************************************
u32 Patch_osStartThread()
{
TEST_DISABLE_THREAD_FUNCS
	u32 thread = gGPR[REG_a0]._u32_0;

	// Disable interrupts

	//DBGConsole_Msg(0, "osStartThread(0x%08x)", thread)

	u32 ThreadState = Read16Bits(thread + 0x10);

	if (ThreadState == OS_STATE_WAITING)
	{
		//DBGConsole_Msg(0, "  Thread is WAITING");

		Write16Bits(thread + 0x10, OS_STATE_RUNNABLE);

		gGPR[REG_a0]._s64 = (s64)(s32)VAR_ADDRESS(osThreadQueue);
		gGPR[REG_a1]._s64 = (s64)(s32)thread;

		g___osEnqueueThread_s.pFunction();
	}
	else if (ThreadState == OS_STATE_STOPPED)
	{
		//DBGConsole_Msg(0, "  Thread is STOPPED");

		u32 queue = Read32Bits(thread + 0x08);

		if (queue == 0 || queue == VAR_ADDRESS(osThreadQueue))
		{
			//if (queue == NULL)
				//DBGConsole_Msg(0, "  Thread has NULL queue");
			//else
				//DBGConsole_Msg(0, "  Thread's queue is VAR_ADDRESS(osThreadQueue)");

			Write16Bits(thread + 0x10, OS_STATE_RUNNABLE);

			gGPR[REG_a0]._s64 = (s64)(s32)VAR_ADDRESS(osThreadQueue);
			gGPR[REG_a1]._s64 = (s64)(s32)thread;

			g___osEnqueueThread_s.pFunction();
		}
		else
		{
			//DBGConsole_Msg(0, "  Thread has it's own queue");

			Write16Bits(thread + 0x10, OS_STATE_WAITING);

			gGPR[REG_a0]._s64 = (s64)(s32)queue;
			gGPR[REG_a1]._s64 = (s64)(s32)thread;

			g___osEnqueueThread_s.pFunction();

			// Pop the highest priority thread from the queue
			u32 NewThread = Read32Bits(queue + 0x0);
			Write32Bits(queue, Read32Bits(NewThread + 0x0));

			// Enqueue the next thread to run
			gGPR[REG_a0]._s64 = (s64)(s32)VAR_ADDRESS(osThreadQueue);
			gGPR[REG_a1]._s64 = (s64)(s32)NewThread;

			g___osEnqueueThread_s.pFunction();
		}
	}
	else
	{
		DBGConsole_Msg(0, "  Thread is neither WAITING nor STOPPED");
	}

	// At this point, we check the priority of the current
	// thread and the highest priority thread on the thread queue. If
	// the current thread has a higher priority, nothing happens, else
	// the new thread is started

	u32 ActiveThread = Read32Bits(VAR_ADDRESS(osActiveThread));

	if (ActiveThread == 0)
	{
		// There is no currently active thread
		//DBGConsole_Msg(0, "  No active thread, dispatching");

		return CALL_PATCHED_FUNCTION(__osDispatchThread);

	}
	else
	{
		// A thread is currently active
		u32 QueueThread = Read32Bits(VAR_ADDRESS(osThreadQueue));

		u32 QueueThreadPri = Read32Bits(QueueThread + 0x4);
		u32 ActiveThreadPri = Read32Bits(ActiveThread + 0x4);

		if (ActiveThreadPri < QueueThreadPri)
		{
			//DBGConsole_Msg(0, "  New thread has higher priority, enqueue/yield");

			// Set the active thread's state to RUNNABLE
			Write16Bits(ActiveThread + 0x10, OS_STATE_RUNNABLE);

			gGPR[REG_a0]._s64 = (s64)(s32)VAR_ADDRESS(osThreadQueue);

			// Doing this is ok, because when the active thread is resumed, it will resume
			// after the call to osStartThread(). We don't do any processing after this
			// event (we don't bother with interrupts)
			return CALL_PATCHED_FUNCTION(__osEnqueueAndYield);
		}
		else
		{
			//DBGConsole_Msg(0, "  Thread has lower priority, continuing with active thread");
		}

	}
	// Restore interrupts?

	return PATCH_RET_JR_RA;
}

//*****************************************************************************
//
//*****************************************************************************
u32 Patch___osPopThread()
{
TEST_DISABLE_THREAD_FUNCS
	u32 queue = gGPR[REG_a0]._u32_0;
	u8 * pBase	  = (u8 *)ReadAddress(queue);

	u32 thread = QuickRead32Bits(pBase, 0x0);

	gGPR[REG_v0]._u32_0 = thread;
	QuickWrite32Bits(pBase, Read32Bits(thread + 0x0));
	//DBGConsole_Msg(0, "0x%08x = __osPopThread(0x%08x)", thread, queue);

	return PATCH_RET_JR_RA;
}


