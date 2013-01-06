#include "stdafx.h"
#include "Graphics/NativeTexture.h"

#include "HLEGraphics/ConvertImage.h"

void PngSaveImage( const char* filename, const void * data, const void * palette, ETextureFormat pixelformat, u32 pitch, u32 width, u32 height, bool use_alpha )
{
}

bool	ConvertTexture( const TextureDestInfo & dst, const TextureInfo & ti )
{
	return true;
}

u32 CNativeTexture::GetSystemMemoryUsage() const
{
	return 0;
}

u32 CNativeTexture::GetVideoMemoryUsage() const
{
	return 0;
}

bool CNativeTexture::HasData() const
{
	return true;
}

//*****************************************************************************
//
//*****************************************************************************
u32		CNativeTexture::GetBytesRequired() const
{
	return GetStride() * mCorrectedHeight;
}

//*****************************************************************************
//
//*****************************************************************************
u32	CNativeTexture::GetStride() const
{
	return CalcBytesRequired( mTextureBlockWidth, mTextureFormat );
}

//*****************************************************************************
//
//*****************************************************************************
void	CNativeTexture::SetData( void * data, void * palette )
{
}

void	CNativeTexture::InstallTexture() const
{
}

//*****************************************************************************
//
//*****************************************************************************
CRefPtr<CNativeTexture>	CNativeTexture::Create( u32 width, u32 height, ETextureFormat texture_format )
{
	return new CNativeTexture( width, height, texture_format );
}

CNativeTexture::CNativeTexture( u32 w, u32 h, ETextureFormat texture_format )
{
}


//*****************************************************************************
//
//*****************************************************************************
CNativeTexture::~CNativeTexture()
{
}