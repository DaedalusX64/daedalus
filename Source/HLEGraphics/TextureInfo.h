/*
Copyright (C) 2001,2007 StrmnNrmn

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

#ifndef HLEGRAPHICS_TEXTUREINFO_H_
#define HLEGRAPHICS_TEXTUREINFO_H_

#include "Graphics/TextureFormat.h"


enum ETLutFmt
{
	kTT_NONE = 0,	// G_TT_NONE
	kTT_UNKNOWN,	// G_TT_UNKNOWN
	kTT_RGBA16,		// G_TT_RGBA16
	kTT_IA16,		// G_TT_IA16
};


struct TextureInfo
{
private:
	u32 LoadAddress = 0;		// Address to texture surface
	u32 TlutAddress = 0;		// Address to palette
	u16 Width = 0;				// X dimensions
	u16 Height = 0;				// Y dimensions
	u16 Pitch = 0;				// Number of bytes in a texture row

	u32 TmemAddress : 9 = 0;	// TMEM address (0x000 - 0x1FF)
	u32 Palette : 4 = 0;		// Palette index (0-15)
	u32 Format : 3 = 0;			// e.g. RGBA, YUV, CI, IA, I...
	u32 Size : 2 = 0;			// e.g. 4bpp, 8bpp, 16bpp, 32bpp
	u32 TLutFmt : 2 = 0;		// e.g. ?, ?, RGBA16, IA16

#ifdef DAEDALUS_ACCURATE_TMEM
	u32 Line : 9 = 0;
#endif

	bool Swapped : 1 = false;			// Are odd lines word swapped?
	bool EmulateMirrorS : 1 = false;
	bool EmulateMirrorT : 1 = false;
	bool White : 1 = false;				// Force RGB channels to white.

public:
	TextureInfo() = default;
	TextureInfo(const TextureInfo &) = default;
	TextureInfo & operator=(const TextureInfo &) = default;

	inline u32 GetHashCode() const
	{
		u32 hash = LoadAddress;

		hash = (hash * 33) ^ TlutAddress;
		hash = (hash * 33) ^ Width;
		hash = (hash * 33) ^ Height;
		hash = (hash * 33) ^ Pitch;
		hash = (hash * 33) ^ TmemAddress;
		hash = (hash * 33) ^ Palette;
		hash = (hash * 33) ^ Format;
		hash = (hash * 33) ^ Size;
		hash = (hash * 33) ^ TLutFmt;

#ifdef DAEDALUS_ACCURATE_TMEM
		hash = (hash * 33) ^ Line;
#endif

		hash = (hash * 33) ^ Swapped;
		hash = (hash * 33) ^ EmulateMirrorS;
		hash = (hash * 33) ^ EmulateMirrorT;
		hash = (hash * 33) ^ White;

		return hash;
	}

	// Compute a hash of the contents of the texture data.
	u32 GenerateHashValue() const;

	const char * GetFormatName() const;
	u32 GetSizeInBits() const;

	inline u32 GetLoadAddress() const { return LoadAddress; }
	inline u32 GetTlutAddress() const { return TlutAddress; }
	inline u32 GetTmemAddress() const { return TmemAddress; }
	inline u32 GetFormat() const { return Format; }
	inline u32 GetSize() const { return Size; }
	inline u32 GetWidth() const { return Width; }
	inline u32 GetHeight() const { return Height; }
	inline u32 GetPitch() const { return Pitch; }
	inline ETLutFmt GetTLutFormat() const { return static_cast<ETLutFmt>(TLutFmt); }

#ifdef DAEDALUS_ACCURATE_TMEM
	inline u32 GetLine() const { return Line; }
#endif

	inline u32 GetPalette() const { return Palette; }
	inline bool IsSwapped() const { return Swapped; }
	inline bool GetEmulateMirrorS() const { return EmulateMirrorS; }
	inline bool GetEmulateMirrorT() const { return EmulateMirrorT; }
	inline bool GetWhite() const { return White; }

	inline void SetLoadAddress(u32 address) { LoadAddress = address; }
	inline void SetTlutAddress(u32 address) { TlutAddress = address; }
	inline void SetTmemAddress(u32 address) { TmemAddress = address; }
	inline void SetFormat(u32 format) { Format = format; }
	inline void SetSize(u32 size) { Size = size; }
	inline void SetWidth(u32 width) { Width = static_cast<u16>(width); }
	inline void SetHeight(u32 height) { Height = static_cast<u16>(height); }
	inline void SetPitch(u32 pitch) { Pitch = static_cast<u16>(pitch); }
	inline void SetTLutFormat(ETLutFmt format) { TLutFmt = static_cast<u32>(format); }

#ifdef DAEDALUS_ACCURATE_TMEM
	inline void SetLine(u32 line) { Line = line; }
#endif

	inline void SetPalette(u32 index) { Palette = index; }
	inline void SetSwapped(bool swapped) { Swapped = swapped; }
	inline void SetEmulateMirrorS(bool emulate) { EmulateMirrorS = emulate; }
	inline void SetEmulateMirrorT(bool emulate) { EmulateMirrorT = emulate; }
	inline void SetWhite(bool white) { White = white; }
inline bool operator==(const TextureInfo & rhs) const
{
	return LoadAddress == rhs.LoadAddress &&
		   TlutAddress == rhs.TlutAddress &&
		   Width == rhs.Width &&
		   Height == rhs.Height &&
		   Pitch == rhs.Pitch &&
		   TmemAddress == rhs.TmemAddress &&
		   Palette == rhs.Palette &&
		   Format == rhs.Format &&
		   Size == rhs.Size &&
		   TLutFmt == rhs.TLutFmt
#ifdef DAEDALUS_ACCURATE_TMEM
		   && Line == rhs.Line
#endif
		   && Swapped == rhs.Swapped &&
		   EmulateMirrorS == rhs.EmulateMirrorS &&
		   EmulateMirrorT == rhs.EmulateMirrorT &&
		   White == rhs.White;
}

inline bool operator!=(const TextureInfo & rhs) const
{
	return !(*this == rhs);
}

inline bool operator<(const TextureInfo & rhs) const
{
	if (LoadAddress != rhs.LoadAddress)
		return LoadAddress < rhs.LoadAddress;

	if (TlutAddress != rhs.TlutAddress)
		return TlutAddress < rhs.TlutAddress;

	if (Width != rhs.Width)
		return Width < rhs.Width;

	if (Height != rhs.Height)
		return Height < rhs.Height;

	if (Pitch != rhs.Pitch)
		return Pitch < rhs.Pitch;

	if (TmemAddress != rhs.TmemAddress)
		return TmemAddress < rhs.TmemAddress;

	if (Palette != rhs.Palette)
		return Palette < rhs.Palette;

	if (Format != rhs.Format)
		return Format < rhs.Format;

	if (Size != rhs.Size)
		return Size < rhs.Size;

	if (TLutFmt != rhs.TLutFmt)
		return TLutFmt < rhs.TLutFmt;

#ifdef DAEDALUS_ACCURATE_TMEM
	if (Line != rhs.Line)
		return Line < rhs.Line;
#endif

	if (Swapped != rhs.Swapped)
		return Swapped < rhs.Swapped;

	if (EmulateMirrorS != rhs.EmulateMirrorS)
		return EmulateMirrorS < rhs.EmulateMirrorS;

	if (EmulateMirrorT != rhs.EmulateMirrorT)
		return EmulateMirrorT < rhs.EmulateMirrorT;

	return White < rhs.White;
}
};

#endif // HLEGRAPHICS_TEXTUREINFO_H_
