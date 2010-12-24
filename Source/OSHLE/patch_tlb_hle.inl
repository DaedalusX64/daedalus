#define TEST_DISABLE_TLB_FUNCS DAEDALUS_PROFILE(__FUNCTION__);

u32 Patch_osMapTLB()
{
TEST_DISABLE_TLB_FUNCS
	//osMapTLB(s32, OSPageMask, void *, u32, u32, s32)
#ifndef DAEDALUS_SILENT
	u32 w = gGPR[REG_a0]._u32_0;
	u32 x = gGPR[REG_a1]._u32_0;
	u32 y = gGPR[REG_a2]._u32_0;
	u32 z = gGPR[REG_a3]._u32_0;
	u32 a = Read32Bits(gGPR[REG_sp]._u32_0 + 0x10);
	u32 b = Read32Bits(gGPR[REG_sp]._u32_0 + 0x14);

	DBGConsole_Msg(0, "[WosMapTLB(0x%08x,0x%08x,0x%08x,0x%08x,0x%08x,0x%08x)]",
		w,x,y,z,a,b);
#endif
	return PATCH_RET_NOT_PROCESSED;
}

// ENTRYHI left untouched after call
u32 Patch___osProbeTLB()
{   
TEST_DISABLE_TLB_FUNCS	
	u32 VAddr = gGPR[REG_a0]._u32_0;

	gGPR[REG_v0]._s64 = OS_HLE___osProbeTLB(VAddr);

	//DBGConsole_Msg(0, "Probe: 0x%08x -> 0x%08x", VAddr, dwPAddr);

	return PATCH_RET_JR_RA;
}

u32 Patch_osVirtualToPhysical_Mario()
{
TEST_DISABLE_TLB_FUNCS
	u32 VAddr = gGPR[REG_a0]._u32_0;

	//DBGConsole_Msg(0, "osVirtualToPhysical(0x%08x)", (u32)gGPR[REG_a0]);

	if (IS_KSEG0(VAddr))
	{
		gGPR[REG_v0]._s64 = (s64)(s32)K0_TO_PHYS(VAddr);
	}
	else if (IS_KSEG1(VAddr))
	{
		gGPR[REG_v0]._s64 = (s64)(s32)K1_TO_PHYS(VAddr);
	}
	else
	{
		gGPR[REG_v0]._s64 = OS_HLE___osProbeTLB(VAddr);
	}

	return PATCH_RET_JR_RA;

}

// IDentical - just optimised
u32 Patch_osVirtualToPhysical_Rugrats()
{
TEST_DISABLE_TLB_FUNCS
	//DBGConsole_Msg(0, "osVirtualToPhysical(0x%08x)", (u32)gGPR[REG_a0]);
	return Patch_osVirtualToPhysical_Mario();
}
