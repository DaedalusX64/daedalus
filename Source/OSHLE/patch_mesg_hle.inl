#define TEST_DISABLE_MESG_FUNCS //return PATCH_RET_NOT_PROCESSED;

//*****************************************************************************
//
//*****************************************************************************
u32 Patch_osSetEventMesg_Mario()
{
TEST_DISABLE_MESG_FUNCS
	u32 vent = gGPR[REG_a0]._u32_0;
	u32 queue = gGPR[REG_a1]._u32_0;
	u32 msg   = gGPR[REG_a2]._u32_0;

	/*if (vent < 23)
	{
		DBGConsole_Msg(0, "osSetEventMesg(%s, 0x%08x, 0x%08x)", 
			g_szEventStrings[vent], queue, msg);
	}
	else
	{
		DBGConsole_Msg(0, "osSetEventMesg(%d, 0x%08x, 0x%08x)", 
			vent, queue, msg);
	}*/
		

	u32 p = VAR_ADDRESS(osEventMesgArray) + (vent * 8);

	Write32Bits(p + 0x0, queue);
	Write32Bits(p + 0x4, msg);

	return PATCH_RET_JR_RA;
}

//*****************************************************************************
//
//*****************************************************************************
u32 Patch_osSetEventMesg_Zelda()
{
TEST_DISABLE_MESG_FUNCS

	u32 vent = gGPR[REG_a0]._u32_0;
	u32 queue = gGPR[REG_a1]._u32_0;
	u32 msg   = gGPR[REG_a2]._u32_0;

	/*if (vent < 23)
	{
		DBGConsole_Msg(0, "osSetEventMesg(%s, 0x%08x, 0x%08x)", 
			g_szEventStrings[vent], queue, msg);
	}
	else
	{
		DBGConsole_Msg(0, "osSetEventMesg(%d, 0x%08x, 0x%08x)", 
			vent, queue, msg);
	}*/
		

	u32 p = VAR_ADDRESS(osEventMesgArray) + (vent * 8);

	Write32Bits(p + 0x0, queue);
	Write32Bits(p + 0x4, msg);

	return PATCH_RET_JR_RA;
}

//*****************************************************************************
//
//*****************************************************************************
u32 Patch_osCreateMesgQueue_Mario()
{
TEST_DISABLE_MESG_FUNCS

#ifdef DAED_OS_MESSAGE_QUEUES
	u32 queue    = gGPR[REG_a0]._u32_0;
	u32 MsgBuf   = gGPR[REG_a1]._u32_0;
	u32 MsgCount = gGPR[REG_a2]._u32_0;

	OS_HLE_osCreateMesgQueue(queue, MsgBuf, MsgCount);

	return PATCH_RET_JR_RA;
#else
	return PATCH_RET_NOT_PROCESSED;
#endif
}

//*****************************************************************************
//
//*****************************************************************************
// Exactly the same - just optimised slightly
u32 Patch_osCreateMesgQueue_Rugrats()
{
TEST_DISABLE_MESG_FUNCS

#ifdef DAED_OS_MESSAGE_QUEUES
	u32 queue    = gGPR[REG_a0]._u32_0;
	u32 MsgBuf   = gGPR[REG_a1]._u32_0;
	u32 MsgCount = gGPR[REG_a2]._u32_0;

	OS_HLE_osCreateMesgQueue(queue, MsgBuf, MsgCount);

	return PATCH_RET_JR_RA;
#else
	return PATCH_RET_NOT_PROCESSED;
#endif
}

//*****************************************************************************
//
//*****************************************************************************
u32 Patch_osRecvMesg()
{
TEST_DISABLE_MESG_FUNCS

	// osRecvMesg brakes OOT's in-game menu
	// ToDo : Fix Me
	/*if( g_ROM.GameHacks == ZELDA_OOT ) 
	{
		return PATCH_RET_NOT_PROCESSED0(osRecvMesg);
	}*/

	u32 queue     = gGPR[REG_a0]._u32_0;
	u32 msg       = gGPR[REG_a1]._u32_0;
	u32 BlockFlag = gGPR[REG_a2]._u32_0;

	u32 ValidCount = Read32Bits(queue + 0x8);
	u32 MsgCount = Read32Bits(queue + 0x10);

	/*if (queue == 0x80007d40)
	{
	DBGConsole_Msg(0, "Thread: 0x%08x", Read32Bits(VAR_ADDRESS(osActiveThread)));
	DBGConsole_Msg(0, "osRecvMsg(0x%08x, 0x%08x, %s) (%d/%d pending)", 
		queue, msg, BlockFlag == OS_MESG_BLOCK ? "Block" : "Don't Block",
		ValidCount, MsgCount);
	}*/

	// If there are no valid messages, then we either block until 
	// one becomes available, or return immediately
	if (ValidCount == 0)
	{
		if (BlockFlag == OS_MESG_NOBLOCK)
		{
			// Don't block
			gGPR[REG_v0]._s64 = (s64)(s32)~0;
			return PATCH_RET_JR_RA;
		}
		else
		{
			// We can't handle, as this would result in a yield (tricky)
			return PATCH_RET_NOT_PROCESSED0(osRecvMesg);
		}
	}

	//DBGConsole_Msg(0, "  Processing Pending");

	u32 first = Read32Bits(queue + 0x0c);
	
	//Store message in pointer
	if (msg != 0)
	{
		//DBGConsole_Msg(0, "  Retrieving message");
		
		u32 MsgBase = Read32Bits(queue + 0x14);

		// Offset to first valid message
		MsgBase += first * 4;

		Write32Bits(msg, Read32Bits(MsgBase));

	}
	first = (first + 1) % MsgCount;

	DAEDALUS_ASSERT( MsgCount != 0, "Invalid message count" );
	DAEDALUS_ASSERT( MsgCount != u32(~0) && first+ValidCount != 0x80000000, "Invalid message count" );
	// Point first to the next valid message
	/*if (MsgCount == 0)
	{
		DBGConsole_Msg(0, "Invalid message count");
		// We would break here!
	}
	else if (MsgCount == u32(~0) && first+1 == 0x80000000)
	{
		DBGConsole_Msg(0, "Invalid message count/first");
		// We would break here!
	}
	else*/
	//{
	//DBGConsole_Msg(0, "  Generating next valid message number");

	Write32Bits(queue + 0x0c, first);
	//}

	// Decrease the number of valid messages
	ValidCount--;

	Write32Bits(queue + 0x8, ValidCount);

	// Start thread pending on the fullqueue
	u32 FullQueueThread = Read32Bits(queue + 0x04);
	u32 NextThread = Read32Bits(FullQueueThread + 0x00);



	// If the first thread is not the idle thread, start it
	if (NextThread != 0)
	{
		//DBGConsole_Msg(0, "  Activating sleeping thread");

		// From Patch___osPopThread():
		Write32Bits(queue + 0x04, NextThread);

		gGPR[REG_a0]._u32_0 = FullQueueThread;

		// FIXME - How to we set the status flag here?
		return Patch_osStartThread();
	}

	// Set success status
	gGPR[REG_v0]._u64 = 0;

	return PATCH_RET_JR_RA;
}

//*****************************************************************************
//
//*****************************************************************************
u32 Patch_osSendMesg()
{
TEST_DISABLE_MESG_FUNCS

	// Zelda Hack handled now in Patch_RecurseAndFind
	// osSendMesg brakes OOT's in-game menu
	// ToDo : Fix Me
	/*if( g_ROM.GameHacks == ZELDA_OOT )
	{
		return PATCH_RET_NOT_PROCESSED0(osSendMesg);
	}*/

	u32 queue     = gGPR[REG_a0]._u32_0;
	u32 msg       = gGPR[REG_a1]._u32_0;
	u32 BlockFlag = gGPR[REG_a2]._u32_0;

	u32 ValidCount = Read32Bits(queue + 0x8);
	u32 MsgCount = Read32Bits(queue + 0x10);
	
	/*if (queue == 0x80007d40)
	{
		DBGConsole_Msg(0, "Thread: 0x%08x", Read32Bits(VAR_ADDRESS(osActiveThread)));
	DBGConsole_Msg(0, "osSendMsg(0x%08x, 0x%08x, %s) (%d/%d pending)", 
		queue, msg, BlockFlag == OS_MESG_BLOCK ? "Block" : "Don't Block",
		ValidCount, MsgCount);
	}*/

	// If the message queue is full, then we either block until 
	// space becomes available, or return immediately
	if (ValidCount >= MsgCount)
	{
		if (BlockFlag == OS_MESG_NOBLOCK)
		{
			// Don't block
			gGPR[REG_v0]._s64 = (s64)(s32)~0;
			return PATCH_RET_JR_RA;
		}
		else
		{
			// We can't handle, as this would result in a yield (tricky)
			return PATCH_RET_NOT_PROCESSED0(osSendMesg);
		}
	}

	u32 first = Read32Bits(queue + 0x0c);

	//DBGConsole_Msg(0, "  Processing Pending");
	DAEDALUS_ASSERT( MsgCount != 0, "Invalid message count" );
	DAEDALUS_ASSERT( MsgCount != u32(~0) && first+ValidCount != 0x80000000, "Invalid message count" );
	
	// Point first to the next valid message
	/*if (MsgCount == 0)
	{
		DBGConsole_Msg(0, "Invalid message count");
		// We would break here!
	}
	else if (MsgCount == u32(~0) && first+ValidCount == 0x80000000)
	{
		DBGConsole_Msg(0, "Invalid message count/first");
		// We would break here!
	}
	else*/
	//{
	u32 slot = (first + ValidCount) % MsgCount;
	
	u32 MsgBase = Read32Bits(queue + 0x14);

	// Offset to first valid message
	MsgBase += slot * 4;

	Write32Bits(MsgBase, msg);

	//}
	
	// Increase the number of valid messages
	ValidCount++;

	Write32Bits(queue + 0x8, ValidCount);

	// Start thread pending on the fullqueue
	u32 EmptyQueueThread = Read32Bits(queue + 0x00);
	u32 NextThread = Read32Bits(EmptyQueueThread + 0x00);


	// If the first thread is not the idle thread, start it
	if (NextThread != 0)
	{
		//DBGConsole_Msg(0, "  Activating sleeping thread");

		// From Patch___osPopThread():
		Write32Bits(queue + 0x00, NextThread);

		gGPR[REG_a0]._s64 = (s64)(s32)EmptyQueueThread;

		// FIXME - How to we set the status flag here?
		return Patch_osStartThread();
	}

	// Set success status
	gGPR[REG_v0]._u64 = 0;

	return PATCH_RET_JR_RA;
}
