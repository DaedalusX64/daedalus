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

// Manages textures for RDP code
// Uses a HashTable (hashing on TImg) to allow quick access
//  to previously used textures

#include "stdafx.h"

//#include "memory.h"
#include "TextureCache.h"
#include "ConvertImage.h"
#include "RDP.h"

#include "ultra_gbi.h"		// For image size/formats
#include "Utility/Profiler.h"
#include "Core/ROM.h"
#include "Core/Memory.h"

#include "Debug/DaedalusAssert.h"
#include "Debug/DBGConsole.h"

#include "DaedHash.h"

extern u32 gRDPFrame;

//*************************************************************************************
//
//*************************************************************************************
namespace
{
	u32 pixels2bytes( u32 pixels, u32 size )
	{
		return ((pixels << size)+1) / 2;
	}
	const char * const	pszImgFormat[8] = {"RGBA", "YUV", "CI", "IA", "I", "?1", "?2", "?3"};
	const u32			pnImgSize[4]   = {4, 8, 16, 32};

}
u32 TextureInfo::GetHashCode() const
{
	return daedalus::hash( reinterpret_cast< const u8 * >( this ), sizeof(TextureInfo), 0 );
}

const char * TextureInfo::GetFormatName() const
{
	return pszImgFormat[ Format ];
}

u32 TextureInfo::GetSizeInBits() const	
{
	return pnImgSize[ Size ];
}

void TextureInfo::SetTLutFormat( u32 format )
{
	TLutFmt = format >> G_MDSFT_TEXTLUT;
}

u32	TextureInfo::GetTLutFormat() const
{
	return TLutFmt << G_MDSFT_TEXTLUT;
}

u32	TextureInfo::GetWidthInBytes() const
{
	return pixels2bytes( Width, Size );
}

u32	TextureInfo::GetLeftInBytes() const
{
	return pixels2bytes( Left, Size );
}



// Implementation

struct TextureEntry
{
	TextureEntry()
		:	FrameLastUpdated( 0 )
		,	FrameLastUsed( 0 )
		,	pTexture(NULL)
		,	pMirroredTexture(NULL)
	{
		mHashCode = mTextureInfo.GetHashCode();
	}

	~TextureEntry()
	{
		if (pTexture)
		{
			pTexture->Release();
			pTexture = NULL;
		}
		if (pMirroredTexture)
		{
			pMirroredTexture->Release();
			pMirroredTexture = NULL;
		}
	}

	const TextureInfo &	GetTextureInfo() const
	{
		return mTextureInfo;
	}

	void	SetTextureInfo( const TextureInfo & ti )
	{
		mTextureInfo = ti;
		mHashCode = ti.GetHashCode();
	}

	u32 GetHashCode() const
	{
		return mHashCode;
	}

	//
	// Data
	//
	struct TextureEntry * pNext;

	u32			HashValue;

	u32			NumUses;			// Total times used (for stats)
	u32			FrameLastUpdated;	// Frame # that this was last updated
	u32			FrameLastUsed;		// Frame # that this was last used

	CTexture	* pTexture;
	// TODO: when nXTimes and nYTimes in the mirror emulator are correctly determined, these should be put here
	CTexture * pMirroredTexture;

private:
	TextureInfo mTextureInfo;
	u32			mHashCode;
};



class ITextureCache : public CTextureCache
{
public:
	ITextureCache();
	~ITextureCache();
	
			bool			Initialise(u32 size);

			void			PurgeOldTextures();
			u32				CountOldTextures() const;
			void			DropTextures();

			void			SetDumpTextures( bool dump_textures )	{ mDumpTextures = dump_textures; }
			bool			GetDumpTextures( ) const				{ return mDumpTextures; }
	
			CTexture *		GetTexture(const TextureInfo * pti);

protected:
			TextureEntry *	CreateEntry(const TextureInfo * pti);
			void			AddTextureEntry(TextureEntry *p_entry);
			void			RemoveTextureEntry(TextureEntry * p_entry);
			
			void			AddToRecycleList(TextureEntry *p_entry);
			TextureEntry *	ReviveUsedTexture(u32 width, u32 height);
			
			
			TextureEntry *	GetEntry(const TextureInfo * pti);
			TextureEntry *	GetEntryAndUpdate(const TextureInfo * pti);
			
			u32				GetHashIndex(u32 value) const;
			
			void			DecompressTexture(TextureEntry * p_entry);

			u32				GenerateHashValue(const TextureInfo * pti) const;
protected:
	TextureEntry *			mpFirstUsedSurface;
	TextureEntry **			mpTextureHashTable;
	u32						mTextureHashSize;
	bool					mDumpTextures;
	
};


// Interface

//*****************************************************************************
//
//*****************************************************************************

template<> bool CSingleton< CTextureCache >::Create()
{
	ITextureCache * pNewInstance;
	
	DAEDALUS_ASSERT_Q(mpInstance == NULL);
	
	pNewInstance = new ITextureCache();
	if (!pNewInstance)
	{
		return false;
	}
	else
	{
		if (!pNewInstance->Initialise(8192))
		{
			delete pNewInstance;
			pNewInstance = NULL;
		}
	}
	
	mpInstance = pNewInstance;
	return true;
}

//*************************************************************************************
//
//*************************************************************************************
ITextureCache::ITextureCache() :
	mpFirstUsedSurface(NULL),
	mpTextureHashTable(NULL),
	mTextureHashSize(0),
	mDumpTextures(false)
{
}

//*************************************************************************************
//
//*************************************************************************************
ITextureCache::~ITextureCache()
{
	DropTextures();
	
	while (mpFirstUsedSurface)
	{
		TextureEntry * pVictim = mpFirstUsedSurface;
		mpFirstUsedSurface = pVictim->pNext;
		
		delete pVictim;
	}

	delete []mpTextureHashTable;
	mpTextureHashTable = NULL;	
}

//*************************************************************************************
//
//*************************************************************************************
bool ITextureCache::Initialise(u32 size)
{
	mpTextureHashTable = new TextureEntry *[size];
	if (mpTextureHashTable == NULL)
		return false;
	
	mTextureHashSize = size;
	
	for( u32 i = 0; i < size; i++ )
		mpTextureHashTable[i] = NULL;
	
	return true;	// Success!
	
}

//*************************************************************************************
// Purge any textures whos last usage was over 5 seconds ago
//*************************************************************************************
void ITextureCache::PurgeOldTextures()
{
	if (mpTextureHashTable == NULL)
		return;
	
	Lock();

	// XXXX Suspect these need to be a LOT smaller - e.g. potentially 1...
	static const u32 FRAMES_TO_KILL = 5;
	static const u32 FRAMES_TO_DELETE = 30;
	
	for (u32 i = 0; i < mTextureHashSize; i++)
	{
		TextureEntry * p_entry( mpTextureHashTable[i] );
		while (p_entry)
		{
			TextureEntry * p_next( p_entry->pNext );
			
			if ( gRDPFrame - p_entry->FrameLastUsed > FRAMES_TO_KILL )
			{
				RemoveTextureEntry(p_entry);
			}
			p_entry = p_next;
		}
	}
	
	
	// Remove any old textures that haven't been recycled in 1 minute or so
	// Normally these would be reused
	TextureEntry * p_prev( NULL );
	TextureEntry * p_current( mpFirstUsedSurface );
	while( p_current != NULL )
	{
		TextureEntry * p_next( p_current->pNext );
		
		if ( gRDPFrame - p_current->FrameLastUsed > FRAMES_TO_DELETE )
		{
			// Everything from this point on should be too old!
			// Remove from list
			if (p_prev != NULL) p_prev->pNext      = p_current->pNext;
			else				mpFirstUsedSurface = p_current->pNext;
			
			delete p_current;
			
			// p_prev remains the same
			p_current = p_next;	
		}
		else
		{
			p_prev = p_current;
			p_current = p_next;
		}
	}
	
	Unlock();
}

//*************************************************************************************
//
//*************************************************************************************
void ITextureCache::DropTextures()
{
	if (mpTextureHashTable == NULL)
		return;
	
	Lock();
	
	u32 count( 0 );
	u32 total_uses( 0 );
	
	for (u32 i = 0; i < mTextureHashSize; i++)
	{
		while (mpTextureHashTable[i])
		{
			TextureEntry *pTVictim = mpTextureHashTable[i];
			mpTextureHashTable[i] = pTVictim->pNext;
			
			total_uses += pTVictim->NumUses;
			count++;
			
			AddToRecycleList(pTVictim);
		}
	}
	
	DBGConsole_Msg(0, "Texture Handler: %d entries in hash table, %d uses.", count, total_uses);
	
	Unlock();
	
}

//*************************************************************************************
// Add to the recycle list
//*************************************************************************************
void ITextureCache::AddToRecycleList(TextureEntry *p_entry)
{
	if (p_entry->pTexture == NULL)
	{
		// No point in saving!
		delete p_entry;
	}
	else
	{
		// Add to the list
		p_entry->pNext = mpFirstUsedSurface;
		mpFirstUsedSurface = p_entry;
	}
}

//*************************************************************************************
// Search for a texture of the specified dimensions to recycle
//*************************************************************************************
TextureEntry * ITextureCache::ReviveUsedTexture(u32 width, u32 height)
{
	TextureEntry * p_prev( NULL );
	TextureEntry * p_current( mpFirstUsedSurface );
	while (p_current != NULL)
	{
		const TextureInfo &	current_info( p_current->GetTextureInfo() );

		if (current_info.Width == width &&
			current_info.Height == height)
		{
			// Remove from list
			if (p_prev != NULL) p_prev->pNext      = p_current->pNext;
			else				mpFirstUsedSurface = p_current->pNext;
			
			//DBGConsole_Msg(0, "Reviving used texture (%d x %d)", width, height);
			// Initialise any fields:
			return p_current;
		}
		
		p_prev = p_current;
		p_current = p_current->pNext;
	}

	return NULL;
}

//*************************************************************************************
//
//*************************************************************************************
u32 ITextureCache::GetHashIndex(u32 value) const
{
	// Divide by four, because most textures will be on a 4 byte boundry, so bottom four
	// bits are null
	return value & (mTextureHashSize-1);
}

//*************************************************************************************
//
//*************************************************************************************
void ITextureCache::AddTextureEntry(TextureEntry *p_entry)
{	
	if (mpTextureHashTable == NULL)
		return;
		
	u32	hash_index( GetHashIndex( p_entry->GetHashCode() ) );
	
	Lock();
	
	// Add to head (not tail, for speed - new textures are more likely to be accessed next)
	p_entry->pNext = mpTextureHashTable[hash_index];
	mpTextureHashTable[hash_index] = p_entry;
	
	Unlock();
	
}

//*************************************************************************************
//
//*************************************************************************************
TextureEntry * ITextureCache::GetEntry(const TextureInfo * pti)
{
	if (mpTextureHashTable == NULL)
		return NULL;
	
	// See if it is already in the hash table
	u32 hash_index( GetHashIndex( pti->GetHashCode() ) );
	for( TextureEntry *p_entry = mpTextureHashTable[hash_index]; p_entry != NULL; p_entry = p_entry->pNext )
	{
		const TextureInfo &	current_info( p_entry->GetTextureInfo() );
		if ( pti->IsEqual( current_info ) )
		{
			//printf( "GetEntry %08x, returning\n", hash_index );
			return p_entry;
		}
	}
	return NULL;
}

//*************************************************************************************
//
//*************************************************************************************
void ITextureCache::RemoveTextureEntry(TextureEntry * p_entry)
{
	DAEDALUS_PROFILE( "ITextureCache::RemoveTextureEntry" );
	
	if (mpTextureHashTable == NULL)
		return;
	
	Lock();
	
	//DBGConsole_Msg(0, "Remove Texture entry!");
	
	// See if it is already in the hash table
	const TextureInfo &	texture_info( p_entry->GetTextureInfo() );

	u32 hash_index( GetHashIndex( p_entry->GetHashCode() ) );
	
	TextureEntry * p_prev( NULL );
	TextureEntry * p_current( mpTextureHashTable[hash_index] );
	while( p_current != NULL )
	{
		const TextureInfo &	current_info( p_current->GetTextureInfo() );

		// Check that the attributes match
		if( current_info.IsEqual( texture_info ) )
		{
			if (p_prev != NULL) p_prev->pNext = p_current->pNext;
			else				mpTextureHashTable[hash_index] = p_current->pNext;

			break;
		}
		
		p_prev = p_current;
		p_current = p_current->pNext;
	}
	
	Unlock();
	
	if (p_current == NULL)
	{
		DBGConsole_Msg(0, "Entry not found!!!");
	}
	
	AddToRecycleList(p_entry);
}

//*************************************************************************************
//
//*************************************************************************************
TextureEntry * ITextureCache::CreateEntry(const TextureInfo * pti)
{
	DAEDALUS_PROFILE( "ITextureCache::CreateEntry" );

	// Find a used texture
	TextureEntry * p_entry( ReviveUsedTexture(pti->Width, pti->Height) );
	if (p_entry == NULL)
	{
		// Couldn't find on - recreate!
		p_entry = new TextureEntry;
		if (p_entry == NULL)
			return NULL;
		
		p_entry->pTexture = CTexture::Create(pti->Width, pti->Height);

		// Ignore failure for now. This prevents us trying to recreate
		// the texture each frame, which would really slow things down
	}
	
	// Initialise
	p_entry->pNext = NULL;
	p_entry->SetTextureInfo( *pti );
	p_entry->NumUses = 0;
	p_entry->FrameLastUsed = gRDPFrame;
	p_entry->FrameLastUpdated = gRDPFrame;

	// If this we're performing Texture updated checks, randomly offset the
	// 'FrameLastUpdated' time. This ensures when lots of textures are
	// created on the same frame we update them over a nice distribution of frames.
	if(gCheckTextureHashFrequency > 0)
	{
		p_entry->FrameLastUpdated += rand() % gCheckTextureHashFrequency;
	}
	p_entry->HashValue = GenerateHashValue(pti);
	
	// Add to the hash table
	AddTextureEntry(p_entry);
	return p_entry;	
}

//*************************************************************************************
//
//*************************************************************************************
u32 ITextureCache::GenerateHashValue(const TextureInfo * pti) const
{
	DAEDALUS_PROFILE( "ITextureCache::GenerateHashValue" );

	// If CRC checking is disabled, always return 0
	if ( gCheckTextureHashFrequency == 0 )
		return 0;
	
	u32 bytes_per_line( pti->GetWidthInBytes() );

	//DBGConsole_Msg(0, "BytesPerLine: %d", bytes_per_line);
	
	// A very simple crc - just summation
	u32 hash_value( 0 );

	u32 offset( (pti->Top * pti->Pitch) + pti->GetLeftInBytes() ); 

	//DAEDALUS_ASSERT( (pti->Address + offset + pti->Height * pti->Pitch) < 4*1024*1024, "Address of texture is out of bounds" );

	const u8 * p_bytes( g_pu8RamBase + pti->Address + offset );
	for (u32 y = 0; y < pti->Height; y++)		// Do every nth line?
	{
		// Byte fiddling won't work, but this probably doesn't matter
		hash_value = daedalus::hash( p_bytes, bytes_per_line, hash_value );
		p_bytes += pti->Pitch;
	}

	if (pti->GetFormat() == G_IM_FMT_CI)
	{
		u32 bytes;
		if ( pti->GetSize() == G_IM_SIZ_4b )	bytes = 16  * 4;
		else									bytes = 256 * 4;

		p_bytes = &gTextureMemory[ pti->TmemPalAddress << 3 ];
		hash_value = daedalus::hash( p_bytes, bytes, hash_value );
	}

	return hash_value;
}

//*************************************************************************************
//
//*************************************************************************************
CTexture * ITextureCache::GetTexture(const TextureInfo * pti)
{
	DAEDALUS_PROFILE( "ITextureCache::GetTexture" );

	TextureEntry * p_entry( GetEntryAndUpdate(pti) );
	if (p_entry == NULL)
		return NULL;

	return p_entry->pTexture;
}

//*************************************************************************************
// If already in table, return cached copy
// Otherwise, create surfaces, and load texture into memory
//*************************************************************************************
TextureEntry * ITextureCache::GetEntryAndUpdate(const TextureInfo * pti)
{
	//
	// Retrieve the texture from the cache (if it already exists)
	//
	TextureEntry * p_entry( GetEntry(pti) );
	if (p_entry == NULL)
	{
		// We need to create a new entry, and add it to the hash table.
		p_entry = CreateEntry(pti);
		if (p_entry == NULL)
			return NULL;

	}
	else
	{
		// See if we need to update this texture.
		if (gRDPFrame == p_entry->FrameLastUsed ||
			gRDPFrame < p_entry->FrameLastUpdated + gCheckTextureHashFrequency)
		{
			p_entry->NumUses++;
			p_entry->FrameLastUsed = gRDPFrame;
			return p_entry;
		}

		//
		// Generate and check the current crc
		//
		u32 hash_value( GenerateHashValue(pti) );

		// Check if the CRCs match, if so return. Don't do this for newly created entries
		if (p_entry->HashValue == hash_value)
		{
			// Tile is ok, return
			p_entry->NumUses++;
			p_entry->FrameLastUpdated = gRDPFrame;
			p_entry->FrameLastUsed = gRDPFrame;			
			return p_entry;
		}

		// The crc needs updating
		p_entry->HashValue = hash_value;
		p_entry->FrameLastUpdated = gRDPFrame;
	}

	//
	// The texture needs to be updated for this frame
	//
	if (p_entry->pTexture != NULL)
	{
		DAEDALUS_PROFILE( "Texture Conversion" );
		DecompressTexture(p_entry);		
	}
	
	return p_entry;
}

//*************************************************************************************
// 
//*************************************************************************************
void ITextureCache::DecompressTexture(TextureEntry * p_entry)
{
	bool handled = false;	

	const TextureInfo &	texture_info( p_entry->GetTextureInfo() );
	
	//void * pSrc = g_pu8RamBase + texture_info.Address;

	switch (texture_info.GetFormat())
	{
	case G_IM_FMT_RGBA:
		switch (texture_info.GetSize())
		{
		case G_IM_SIZ_16b:
			ConvertRGBA16_16(p_entry->pTexture, texture_info);
			handled = TRUE;
			break;
		case G_IM_SIZ_32b:
			ConvertRGBA32_16(p_entry->pTexture, texture_info);
			handled = TRUE;
			break;
		}
		break;
		
	case G_IM_FMT_YUV:
		break;

	case G_IM_FMT_CI:
		switch (texture_info.GetSize())
		{
		case G_IM_SIZ_4b: // 4bpp
			switch (texture_info.GetTLutFormat())
			{
				//case G_TT_NONE:
			case G_TT_RGBA16:
				ConvertCI4_RGBA16_16(p_entry->pTexture, texture_info);
				handled = TRUE;
				break;
			case G_TT_IA16:
				ConvertCI4_IA16_16(p_entry->pTexture, texture_info);
				handled = TRUE;
				break;
			}
			break;
			
		case G_IM_SIZ_8b: // 8bpp
			switch(texture_info.GetTLutFormat())
			{
			case G_TT_RGBA16:
				ConvertCI8_RGBA16_16(p_entry->pTexture, texture_info);
				handled = TRUE;
				break;
			case G_TT_IA16:
				ConvertCI8_IA16_16(p_entry->pTexture, texture_info);
				handled = TRUE;
				break;
			}
			break;
		}
		
		break;

	case G_IM_FMT_IA:
		switch (texture_info.GetSize())
		{
		case G_IM_SIZ_4b:
			ConvertIA4_16(p_entry->pTexture, texture_info);
			handled = TRUE;
			break;
		case G_IM_SIZ_8b:
			ConvertIA8_16(p_entry->pTexture, texture_info);
			handled = TRUE;
			break;
		case G_IM_SIZ_16b:
			ConvertIA16_16(p_entry->pTexture, texture_info);
			handled = TRUE;
			break;
		case G_IM_SIZ_32b:
			break;
		}
		break;

	case G_IM_FMT_I:
		switch (texture_info.GetSize())
		{
		case G_IM_SIZ_4b:
			ConvertI4_16(p_entry->pTexture, texture_info);
			handled = TRUE;
			break;
		case G_IM_SIZ_8b:
			ConvertI8_16(p_entry->pTexture, texture_info);
			handled = TRUE;
			break;
		}
		break;
	default:
		break;
	}
	
	if (!handled)
	{
		static BOOL bWarningEmitted = FALSE;
		
		if (!bWarningEmitted)
		{			
			DBGConsole_Msg(0, "DecompressTexture: Unable to decompress %s/%dbpp", texture_info.GetFormatName(), texture_info.GetSizeInBits());	
			//bWarningEmitted = TRUE;
		}
	}
}
