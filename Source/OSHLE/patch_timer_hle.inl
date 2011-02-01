#define TEST_DISABLE_TIMER_FUNCS DAEDALUS_PROFILE(__FUNCTION__);

//*****************************************************************************
//
//*****************************************************************************
u32 Patch___osSetTimerIntr()
{
TEST_DISABLE_TIMER_FUNCS
	s64 sum;
	s64 count;
	s64 TimeLo = (s64)gGPR[REG_a1]._s32_0;
	//s64 qwTimeHi = (s64)(s32)gGPR[REG_a0];

	count = (s64)gCPUState.CPUControl[C0_COUNT]._s32_0;	

	Write32Bits(VAR_ADDRESS(osSystemLastCount), (u32)count);	

	sum = (s64)(s32)((s32)count + (s32)TimeLo);

	CPU_SetCompare(sum);

	return PATCH_RET_JR_RA;	
}

//*****************************************************************************
//
//*****************************************************************************
u32 Patch___osInsertTimer()
{
TEST_DISABLE_TIMER_FUNCS
	u32 NewTimer = gGPR[REG_a0]._u32_0;

	u64 NewValue = Read64Bits(NewTimer + 0x10);	// Check ordering is correct?!

	u32 TopTimer = Read32Bits(VAR_ADDRESS(osTopTimer));
	u32 InsertTimer;
	u64 InsertValue;

	/*
	u64 qwNewInterval = Read64Bits(NewTimer + 0x08);
	DBGConsole_Msg(0, "osInsertTimer(0x%08x)", NewTimer);
	DBGConsole_Msg(0, "  Timer->value = 0x%08x%08x", (u32)(NewValue>>32), (u32)NewValue);
	DBGConsole_Msg(0, "  Timer->interval = 0x%08x%08x", (u32)(qwNewInterval>>32), (u32)qwNewInterval);
	*/
	
	InsertTimer = Read32Bits(TopTimer + 0x00);	// Read next
	InsertValue = Read64Bits(InsertTimer + 0x10);
	
	if ( InsertTimer == 0 )
	{
		// What gives? 
		DBGConsole_Msg( 0, "[W__osInsertTimer with NULL insert timer" );

		// We can quit, because we've not written anything
		return PATCH_RET_NOT_PROCESSED0(__osInsertTimer);
	}

	while (InsertValue < NewValue)
	{
		// Decrease by the pause for this timer
		NewValue -= InsertValue;

		InsertTimer = Read32Bits(InsertTimer + 0x0);	// Read next timer
		InsertValue = Read64Bits(InsertTimer + 0x10);

		if (InsertTimer == TopTimer)	// At the end of the list?
			break;
	}

	/// Save the modified time value
	Write64Bits(NewTimer + 0x10, NewValue);

	// Inserting before InsertTimer

	// Modify InsertTimer's values if this is not the sentinel node
	if (InsertTimer != TopTimer)
	{
		InsertValue -= NewValue;

		Write64Bits(InsertTimer + 0x10, InsertValue);
	}

	// pNewTimer->next = pInsertTimer
	Write32Bits(NewTimer + 0x00, InsertTimer);

	// pNewTimer->prev = pInsertTimer->prev
	u32 InsertTimerPrev = Read32Bits(InsertTimer + 0x04);
	Write32Bits(NewTimer + 0x04, InsertTimerPrev);

	// pInsertTimer->prev->next = pNewTimer
	Write32Bits(InsertTimerPrev + 0x00, NewTimer);

	// pInsertTimer->prev = pNewTimer
	Write32Bits(InsertTimer + 0x04, NewTimer);

	gGPR[REG_v0]._s64 = (s64)(s32)(NewValue >> 32);
	gGPR[REG_v1]._s64 = (s64)(s32)((u32)NewValue);
	
	return PATCH_RET_JR_RA;
}

//*****************************************************************************
//
//*****************************************************************************
u32 Patch___osTimerServicesInit_Mario()
{
TEST_DISABLE_TIMER_FUNCS

	u32 TimeHi		= VAR_ADDRESS(osSystemTimeHi);
	u32 TimeLo		= VAR_ADDRESS(osSystemTimeLo);
	u32 SystemCount = VAR_ADDRESS(osSystemCount);
	u32 FrameCount  = VAR_ADDRESS(osFrameCount);

	// In some games we can't obtain TimeLo, which results in a bsod, ex Killer Instinct
	// Try to calculate TimeLo and writeback the result obtained
	//
	if(TimeLo == 0)
	{
		DBGConsole_Msg( 0, "TimeLo NULL, trying to caluclate.." );
		TimeLo = SystemCount-(FrameCount-SystemCount);
	}
	
	DBGConsole_Msg(0, "Initialising Timer Services");

	Write32Bits(TimeHi, 0);
	Write32Bits(TimeLo, 0);

	Write32Bits(SystemCount, 0);
	Write32Bits(FrameCount, 0);

	u32 timer = Read32Bits(VAR_ADDRESS(osTopTimer));

	// Make list empty
	Write32Bits(timer + offsetof(OSTimer, next), timer);
	Write32Bits(timer + offsetof(OSTimer, prev), timer);

	Write64Bits(timer + offsetof(OSTimer, interval), 0);
	Write64Bits(timer + offsetof(OSTimer, value), 0);
	Write64Bits(timer + offsetof(OSTimer, mq), 0);
	Write64Bits(timer + offsetof(OSTimer, msg), 0);

	return PATCH_RET_JR_RA;
}

//*****************************************************************************
//
//*****************************************************************************
// Same as above, but optimised
u32 Patch___osTimerServicesInit_Rugrats()
{
TEST_DISABLE_TIMER_FUNCS
	return Patch___osTimerServicesInit_Mario();
}

//*****************************************************************************
//
//*****************************************************************************
u32 Patch_osSetTime()
{
TEST_DISABLE_TIMER_FUNCS
	u32 TimeHi = gGPR[REG_a0]._u32_0;
	u32 TimeLo = gGPR[REG_a1]._u32_0;

	//DBGConsole_Msg(0, "osSetTime(0x%08x%08x)", TimeHi, TimeLo);

	Write32Bits(VAR_ADDRESS(osSystemTimeLo), TimeLo);
	Write32Bits(VAR_ADDRESS(osSystemTimeHi), TimeHi);

	return PATCH_RET_JR_RA;
}

//*****************************************************************************
//
//*****************************************************************************
u32 Patch_osGetTime()
{
TEST_DISABLE_TIMER_FUNCS
	u32 count;
	u32 LastCount;
	u32 TimeLo;
	u32 TimeHi;

	count = gCPUState.CPUControl[C0_COUNT]._u32_0;
	LastCount = Read32Bits(VAR_ADDRESS(osSystemCount));
	
	TimeHi = Read32Bits(VAR_ADDRESS(osSystemTimeHi));
	TimeLo = Read32Bits(VAR_ADDRESS(osSystemTimeLo));
	
	TimeLo += count - LastCount;		// Increase by elapsed time
	
	if (LastCount > count)				// If an overflow has occurred, increase top timer
		TimeHi++;

	gGPR[REG_v0]._s64 = (s64)(s32)TimeHi;
	gGPR[REG_v1]._s64 = (s64)(s32)TimeLo;

	return PATCH_RET_JR_RA;
}

//*****************************************************************************
//
//*****************************************************************************
//ToDo : Implement
u32 Patch_osSetTimer()
{
	return PATCH_RET_NOT_PROCESSED;
}
