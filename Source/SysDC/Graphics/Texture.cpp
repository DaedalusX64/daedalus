/*
Copyright (C) 2001 StrmnNrmn

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "stdafx.h"

#include "Texture.h"
#include "NativeTexture.h"
#include "DaedMathUtil.h"

#include "Debug\DBGConsole.h"

//*****************************************************************************
//
//*****************************************************************************
class ITexture : public CTexture
{
	public:
		ITexture();
		virtual ~ITexture();

		bool					Initialise(u32 width, u32 height);
		CNativeTexture *		GetTexture()					{ return mpTexture; }
		CNativeTexture *		GetRecolouredTexture( u32 mask, u32 colour );

#ifdef LIBPNG_SUPPORT
		bool					DumpImageAsPNG(const char * filename, u32 width, u32 height);
#endif
		virtual	u32				GetWidth() const				{ return mCorrectedWidth; }
		virtual	u32				GetHeight() const				{ return mCorrectedHeight; }
		
		virtual	u32				GetRealWidth() const			{ return mWidth; }
		virtual	u32				GetRealHeight() const			{ return mHeight; }

		// Provides access to "surface"
		bool					StartUpdate(DrawInfo *di);
		void					EndUpdate(DrawInfo *di);

	protected:

		u8 *					mpTexels;
		CNativeTexture *		mpTexture;
		CNativeTexture *		mpRecolouredTexture;
		
		u32						mWidth;			// The requested Texture w/h
		u32						mHeight;

		u32						mCorrectedWidth;	// What was actually created
		u32						mCorrectedHeight;
};



//*****************************************************************************
//
//*****************************************************************************
CTexture * CTexture::Create( u32 width, u32 height )
{
	ITexture * pTexture = new ITexture();

	if (!pTexture->Initialise(width, height))
	{
		delete pTexture;
		return NULL;
	}

	return pTexture;
}

//*****************************************************************************
//
//*****************************************************************************
ITexture::ITexture() :
	mpTexels(NULL),
	mpTexture(NULL),
	mpRecolouredTexture(NULL),
	mWidth(0),
	mHeight(0),
	mCorrectedWidth(0),
	mCorrectedHeight(0)
{
}

//*****************************************************************************
//
//*****************************************************************************
ITexture::~ITexture(void)
{
	delete [] mpTexels;

	if (mpTexture)
	{
		mpTexture->Release();
	}
	if(mpRecolouredTexture)
	{
		mpRecolouredTexture->Release();
	}
}

//*****************************************************************************
//
//*****************************************************************************
bool ITexture::Initialise(u32 width, u32 height)
{
	DAEDALUS_ASSERT_Q(mpTexture == NULL);

	CNativeTexture::CorrectDimensions( width, height, mCorrectedWidth, mCorrectedHeight );

	mpTexture = new CNativeTexture( mCorrectedWidth, mCorrectedHeight );

	if(mpTexture != NULL)
	{
		mpTexels = new u8[width * height * sizeof(u16)];

		mWidth = width;
		mHeight = height;
		return true;
	}

	return false;
}

//*****************************************************************************
//
//*****************************************************************************
CNativeTexture *	ITexture::GetRecolouredTexture( u32 mask, u32 colour )
{
	if(mpRecolouredTexture == NULL)
	{
		mpRecolouredTexture = new CNativeTexture( mCorrectedWidth, mCorrectedHeight );

		if(mpRecolouredTexture != NULL)
		{
			mpRecolouredTexture->SetDataForceColour( mpTexels, mCorrectedWidth, mCorrectedHeight, mask, colour );
		}
	}

	return mpRecolouredTexture;
}

//*****************************************************************************
// Lock the texture so we can start updating it
//*****************************************************************************
bool ITexture::StartUpdate(DrawInfo *di)
{
	if (mpTexture != NULL)
	{
		if(mpTexels != NULL)
		{
			// Return a temporary buffer to use
			di->pSurface = mpTexels;
			di->Width = mWidth;
			di->Height = mHeight;
			di->Pitch = mWidth * sizeof(u16);

			return true;
		}
	}

	return false;
}
/* Linear/iterative twiddling algorithm from Marcus' tatest */
#define TWIDTAB(x) ( (x&1)|((x&2)<<1)|((x&4)<<2)|((x&8)<<3)|((x&16)<<4)| \
	((x&32)<<5)|((x&64)<<6)|((x&128)<<7)|((x&256)<<8)|((x&512)<<9) )
#define TWIDOUT(x, y) ( TWIDTAB((y)) | (TWIDTAB((x)) << 1) )

#define MIN(a, b) ( (a)<(b)? (a):(b) )
//*****************************************************************************
// Finish updating the texture and copy the memory to the native surface
//*****************************************************************************
void ITexture::EndUpdate(DrawInfo *di)
{
	if (mpTexture != NULL)
	{
        if(mWidth != mCorrectedWidth || mHeight != mCorrectedHeight)
        {
            printf("Copy %dx%d to %dx%d (%dx%d to %dx%d)\n", mWidth, mHeight, mCorrectedWidth, mCorrectedHeight, GetWidth(), GetHeight(), GetRealWidth(), GetRealHeight());
            int size = mWidth * mHeight;
	        int min = MIN(mCorrectedWidth, mCorrectedHeight);
	        int mask = min - 1;
    		uint16* pixels = (uint16 *)mpTexels;
    		uint16* vtex = (uint16*)mpTexture->GetData();
    		for(int y=0; y<mHeight; y++)
    			for(int x=0; x<mCorrectedWidth; x++)
    				vtex[TWIDOUT(x&mask,y&mask) + (x/min + y/min)*min*min] = pixels[(y*mWidth+x) % size];
        }
        else
        {
            mpTexture->SetData( mpTexels, mWidth, mHeight );
        }
	}
}
