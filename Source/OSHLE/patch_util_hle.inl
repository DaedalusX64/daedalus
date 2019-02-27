#define TEST_DISABLE_UTIL_FUNCS //DAEDALUS_PROFILE(__FUNCTION__);


u32 Patch___osAtomicDec()
{
TEST_DISABLE_UTIL_FUNCS
	DBGConsole_Msg(0, "osAtomicDec");

	u8 *p = (u8*)ReadAddress(gGPR[REG_a0]._u32_0);
	u32 value = QuickRead32Bits(p, 0x0);
	u32 result= 0;

	if (value)
	{
		value-=1;
		result=1;
		QuickWrite32Bits(p, 0x0, value);
	}

	gGPR[REG_v0]._s64 = (s64)result;

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

	// Set return val here (return dest)
	gGPR[REG_v0]._s64 = (s64)gGPR[REG_a0]._u32_0;

	//NOP! fixes SpiderMan
	if (len == 0)
		return PATCH_RET_JR_RA;

#if 1	//1->Fast, 0->Old way
	fast_memcpy_swizzle( (void *)ReadAddress(dst), (void *)ReadAddress(src), len);
#else
	//DBGConsole_Msg(0, "memcpy(0x%08x, 0x%08x, %d)", dst, src, len);
	u8 *pdst = (u8*)ReadAddress(dst);
	u8 *psrc = (u8*)ReadAddress(src);
	while(len--)
	{
		*(u8*)((u32)pdst++ ^ U8_TWIDDLE) = *(u8*)((u32)psrc++ ^ U8_TWIDDLE);
	}
#endif

	return PATCH_RET_JR_RA;
}

//*****************************************************************************
//
//*****************************************************************************
// Used by Killer Instinct
u32 Patch_strlen()
{
TEST_DISABLE_UTIL_FUNCS
	u32 string = gGPR[REG_a0]._u32_0;
	const u8 *psrc = (const u8*)ReadAddress(string);
	const u8 *start = psrc;

	while (*((u8*)((u32)psrc++^U8_TWIDDLE)) );

	gGPR[REG_v0]._s64 = (s64)(psrc - start - 1);

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

	u8 *start = (u8*)ReadAddress(string);
	u8 *psrc = start;

	for (;; psrc++)
	{
		const u8 SrcChar = *((u8*)((u32)psrc^U8_TWIDDLE));

		if( SrcChar == MatchChar )
		{
			MatchAddr = string + psrc - start;	//Return char address
			break;
		}

		if( SrcChar == 0 ) break;	//Return NULL address
	}

	gGPR[REG_v0]._s64 = (s64) MatchAddr;

	return PATCH_RET_JR_RA;
}

//*****************************************************************************
//
//*****************************************************************************
// Have yet to see a game that uses this
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

	gGPR[REG_v0]._s64 = (s64)i;

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
	gGPR[REG_v0]._s64 = (s64)gGPR[REG_a1]._u32_0;

	if (len == 0)
		return PATCH_RET_JR_RA;

	if (src == dst)
		return PATCH_RET_JR_RA;


	//DBGConsole_Msg(0, "bcopy(0x%08x,0x%08x,%d)", src, dst, len);
	u8 *pdst = (u8*)ReadAddress(dst);
	u8 *psrc = (u8*)ReadAddress(src);

	if (dst > src && dst < src + len)
	{
		pdst += len;
		psrc += len;
		while(len--)
		{
			*(u8*)((u32)pdst-- ^ U8_TWIDDLE) = *(u8*)((u32)psrc-- ^ U8_TWIDDLE);
		}
	}
	else
	{
#if 1	// 1->Fast way, 0->Old way
		fast_memcpy_swizzle( (void *)pdst, (const void *)psrc, len);
#else
		while(len--)
		{
			*(u8*)((u32)pdst++ ^ U8_TWIDDLE) = *(u8*)((u32)psrc++ ^ U8_TWIDDLE);
		}
#endif
	}

	// Mmm already returned dest... -Salvy
	//gGPR[REG_v0]._s64 = 0;

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

	u8* dst8 = (u8*)ReadAddress(dst);

#if (DAEDALUS_ENDIAN_MODE == DAEDALUS_ENDIAN_BIG)
	memset( dst8, 0, len);
#else
	// Align dst on 4 bytes or just resume if already done
	while(((u32)dst8 & 0x3) && len)
	{
		*(u8*)((u32)dst8++ ^ U8_TWIDDLE) = 0;
		len--;
	}

	u32 *dst32=(u32*)dst8;
	u32 len32 = len >> 2;
	u32 len128= 0;
	len &= 0x3;

	while (len32&0x3)
	{
		*dst32++ = 0;
		len32--;
	}

	len128 = len32 >> 2;
	while (len128--)
	{
		*dst32++ = 0;
		*dst32++ = 0;
		*dst32++ = 0;
		*dst32++ = 0;
	}

	dst8=(u8*)dst32;

	//Write(0) to the unaligned remains(if any), byte by byte...
	while(len--)
	{
		*(u8*)((u32)dst8++ ^ U8_TWIDDLE) = 0;
	}
#endif

	// return value of dest
	gGPR[REG_v0]._s64 = (s64) gGPR[REG_a0]._u32_0;

	return PATCH_RET_JR_RA;
}
