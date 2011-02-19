#define TEST_DISABLE_UTIL_FUNCS //DAEDALUS_PROFILE(__FUNCTION__);

u32 Patch___osAtomicDec()
{
TEST_DISABLE_UTIL_FUNCS
	DBGConsole_Msg(0, "osAtomicDec");

	u32 p = gGPR[REG_a0]._u32_0;
	u32 value = Read32Bits(p);

	if (value != 0)
	{
		Write32Bits(p, value - 1);
		gGPR[REG_v0]._u32_0 = 1;
	}
	else
	{
		gGPR[REG_v0]._u32_0 = 0;
	}
	
	return PATCH_RET_JR_RA;
}

//*****************************************************************************
//
//*****************************************************************************
u32 Patch_memcpy()
{
TEST_DISABLE_UTIL_FUNCS
	u32 dst = gGPR[REG_a0]._u32_0;
	u32 src = gGPR[REG_a1]._u32_0;
	u32 len = gGPR[REG_a2]._u32_0;
	u32 i;

	//DBGConsole_Msg(0, "memcpy(0x%08x, 0x%08x, %d)", dst, src, len);
	for (i = 0; i < len; i++)
	{
		Write8Bits(dst + i,  Read8Bits(src + i));
	}

	// return value of dest
	gGPR[REG_v0]._u32_0 = gGPR[REG_a0]._u32_0;	

	return PATCH_RET_JR_RA;
}

//*****************************************************************************
//
//*****************************************************************************
u32 Patch_strlen()
{
TEST_DISABLE_UTIL_FUNCS
	u32 i;
	u32 string = gGPR[REG_a0]._u32_0;

	for (i = 0; Read8Bits(string+i) != 0; i++)
	{}

	gGPR[REG_v0]._s64 = (s64)(s32)i;

	return PATCH_RET_JR_RA;

}

//*****************************************************************************
//
//*****************************************************************************
u32 Patch_strchr()
{
TEST_DISABLE_UTIL_FUNCS
	u32 string = gGPR[REG_a0]._u32_0;
	u8 MatchChar = (u8)(gGPR[REG_a1]._u32_0 & 0xFF);
	u32 MatchAddr = 0;
	u8 SrcChar;
	u32 i;

	for (i = 0; ; i++)
	{
		SrcChar = Read8Bits(string + i);

		if (SrcChar == MatchChar)
		{
			MatchAddr = string + i;
			break;
		}

		if (SrcChar == 0)
		{
			MatchAddr = 0;
			break;
		}
	}

	gGPR[REG_v0]._u32_0 = MatchAddr;

	return PATCH_RET_JR_RA;
}

//*****************************************************************************
//
//*****************************************************************************
u32 Patch_strcmp()
{
	u32 i;
	u32 sA = gGPR[REG_a0]._u32_0;
	u32 sB = gGPR[REG_a1]._u32_0;
	u32 len = gGPR[REG_a2]._u32_0;
	u8 A, B;

	DBGConsole_Msg(0, "strcmp(%s,%s,%d)", sA, sB, len);

	for (i = 0; (A = Read8Bits(sA+i)) != 0 && i < len; i++)
	{
		B = Read8Bits(sB + i);
		if ( A != B)
			break;
	}
	
	if (i == len || (A == 0 && Read8Bits(sB + i) == 0))
		i = 0;

	gGPR[REG_v0]._s64 = (s64)(s32)i;

	return PATCH_RET_JR_RA;
}

//*****************************************************************************
//
//*****************************************************************************
/*void * bcopy(const void * src, void * dst, len) */
// Note different order to src/dst than memcpy!
u32 Patch_bcopy()
{
TEST_DISABLE_UTIL_FUNCS
	u32 src = gGPR[REG_a0]._u32_0;
	u32 dst = gGPR[REG_a1]._u32_0;
	u32 len = gGPR[REG_a2]._u32_0;

	// Set return val here (return dest)
	gGPR[REG_v0]._u32_0 = gGPR[REG_a1]._u32_0;

	if (len == 0)
		return PATCH_RET_JR_RA;

	if (src == dst)
		return PATCH_RET_JR_RA;

	//DBGConsole_Msg(0, "bcopy(0x%08x,0x%08x,%d)", src, dst, len);

	if (dst > src && dst < src + len)
	{
		for (int i = len - 1; i >= 0; i--)
		{
			Write8Bits(dst + i,  Read8Bits(src + i));
		}
	}
	else
	{
		for (u32 i = 0; i < len; i++)
		{
			Write8Bits(dst + i,  Read8Bits(src + i));
		}
	}

	gGPR[REG_v0]._u32_0 = 0;

	return PATCH_RET_JR_RA;
}

//*****************************************************************************
//
//*****************************************************************************
// By Jun Su
u32 Patch_bzero() 
{ 
	u32 dst = gGPR[REG_a0]._u32_0; 
	u32 len = gGPR[REG_a1]._u32_0; 

	//Faster but breaks Chameleon Twist 2
	//memset( (void *)ReadAddress(dst), 0, len);

#if 0 //1->Normal, 0->Optimized //Corn
	// Assume we will only access RAM range
	//Todo optimize unaligned/odd destinations and lengths //Corn
	if( (dst & 0x3) | (len & 0x3) ) for(u32 i = 0; i < len; i++) Write8Bits(dst + i, 0);
	else memset( (void *)ReadAddress(dst), 0, len);
#else
	//Write(0) to the unaligned start(if any), byte by byte...
	while((dst & 0x3) && len)
	{
		Write8Bits(dst++ , 0);
		len--;
	}
	
	//Write(0) to the aligned part
	memset( (void *)ReadAddress(dst), 0, len & ~0x3);

	dst += len & ~0x3;
	len &= 0x3;

	//Write(0) to the unaligned remains(if any), byte by byte...
	while(len--)
	{
		Write8Bits(dst++ , 0);
	}

#endif	

	// return value of dest 
	gGPR[REG_v0]._u32_0 = gGPR[REG_a0]._u32_0; 
	
	return PATCH_RET_JR_RA; 
} 
