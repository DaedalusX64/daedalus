#include <stdafx.h>
#include "Utility/FastMemcpy.h"
#include "Utility/Endian.h"
#include "Utility/Alignment.h"

#include <gtest/gtest.h>

TEST(memcpy_byteswap, WorksWithZeroLength)
{
	u8 dst = 92;
	const u8 src = 14;
	memcpy_byteswap(&dst, &src, 0);
	EXPECT_EQ(92, dst);
}

class MemcpyByteSwapTest : public ::testing::TestWithParam< ::std::tr1::tuple<u32, u32, u32> >
{
protected:
	virtual void SetUp()
	{
		for (u32 i = 0; i < 64; ++i)
			mSrc[i] = i;
	memset(mDst, 0, sizeof(mDst));
	memset(mExpected, 0, sizeof(mExpected));


	}

	ALIGNED_MEMBER(u8, mSrc[64], 64);
	ALIGNED_MEMBER(u8, mDst[64], 64);
	ALIGNED_MEMBER(u8, mExpected[64], 64);
};

static void memcpy_byteswap_reference( void * dst, u32 dst_off, const void * src, u32 src_off, size_t size )
{
	const u8* src8 = (const u8*)src;
	u8* dst8 = (u8*)dst;

	for (size_t i = 0; i < size; ++i)
	{
		dst8[(dst_off + i) ^ U8_TWIDDLE] = src8[(src_off + i) ^ U8_TWIDDLE];
	}
}

TEST_P(MemcpyByteSwapTest, WorksWithSmallCopies)
{
	u32 src_off = ::std::tr1::get<0>(GetParam());
	u32 dst_off = ::std::tr1::get<1>(GetParam());
	u32 len = ::std::tr1::get<2>(GetParam());

	memcpy_byteswap(&mDst[dst_off], &mSrc[src_off], len);
	memcpy_byteswap_reference(mExpected, dst_off, mSrc, src_off, len);
	for (u32 i = 0; i < 64; ++i)
		EXPECT_EQ(mExpected[i], mDst[i]);
}

INSTANTIATE_TEST_CASE_P(X, MemcpyByteSwapTest, ::testing::Combine(::testing::Values(0,1,2,3),
																  ::testing::Values(0,1,2,3), 
																  ::testing::Values(0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16)));