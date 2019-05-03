/*
Copyright (C) 2013 StrmnNrmn

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

#ifndef HLEGRAPHICS_UCODES_UCODE_FB_H_
#define HLEGRAPHICS_UCODES_UCODE_FB_H_

#ifndef DAEDALUS_PSP
static inline CRefPtr<CNativeTexture> LoadFrameBuffer(u32 origin)
{
	u32 width  = Memory_VI_GetRegister( VI_WIDTH_REG );
	if( width == 0 )
	{
		//DAEDALUS_ERROR("Loading 0 size frame buffer?");
		return NULL;
	}

	if( origin <= width*2 )
	{
		//DAEDALUS_ERROR("Loading small frame buffer not supported");
		return NULL;
	}
	//ToDO: We should use uViWidth+1 and uViHeight+1
#define FB_WIDTH  320
#define FB_HEIGHT 240

	DAEDALUS_ASSERT(g_CI.Size == G_IM_SIZ_16b,"32b frame buffer is not supported");
	//DAEDALUS_ASSERT((uViWidth+1) == FB_WIDTH,"Variable width is not handled");
	//DAEDALUS_ASSERT((uViHeight+1) == FB_HEIGHT,"Variable height is not handled");

	TextureInfo ti;

	ti.SetSwapped			(0);
	ti.SetPalette			(0);
	ti.SetTlutAddress		(TLUT_BASE);
	ti.SetTLutFormat		(kTT_RGBA16);
	ti.SetFormat			(0);
	ti.SetSize				(2);

	ti.SetLoadAddress		(origin - width*2);
	ti.SetWidth				(FB_WIDTH);
	ti.SetHeight			(FB_HEIGHT);
	ti.SetPitch				(width << 2 >> 1);

	return gRenderer->LoadTextureDirectly(ti);
}

//Borrowed from StrmnNrmn's N64js
static inline void DrawFrameBuffer(u32 origin, const CNativeTexture * texture)
{

	u16 * pixels = (u16*)malloc(FB_WIDTH*FB_HEIGHT * sizeof(u16));	// TODO: should cache this, but at some point we'll need to deal with variable framebuffer size, so do this later.
	u32 src_offset = 0;

	for (u32 y = 0; y < FB_HEIGHT; ++y)
	{
		u32 dst_row_offset = y * FB_WIDTH;
		u32 dst_offset     = dst_row_offset;

		for (u32 x = 0; x < FB_WIDTH; ++x)
		{
			pixels[dst_offset] = (g_pu8RamBase[(origin + src_offset)^U8_TWIDDLE]<<8) | g_pu8RamBase[(origin + src_offset+  1)^U8_TWIDDLE] | 1;  // NB: or 1 to ensure we have alpha
			dst_offset += 1;
			src_offset += 2;
		}
	}
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, FB_WIDTH, FB_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, pixels);

	//ToDO: Implement me PSP
	//Doesn't work
	//sceGuTexMode( GU_PSM_5551, 0, 0, 1 );		// maxmips/a2/swizzle = 0
	//sceGuTexImage(0, texture->GetCorrectedWidth(), texture->GetCorrectedHeight(), texture->GetBlockWidth(), pixels);

	gRenderer->Draw2DTexture(0, 0, FB_WIDTH, FB_HEIGHT, 0, 0, FB_WIDTH, FB_HEIGHT, texture);

	free(pixels);
}


void RenderFrameBuffer(u32 origin)
{
	gRenderer->SetVIScales();
	gRenderer->BeginScene();

	CRefPtr<CNativeTexture> texture = LoadFrameBuffer(origin);
	if(texture != NULL)
		DrawFrameBuffer(origin, texture);

	gRenderer->EndScene();
	gGraphicsPlugin->UpdateScreen();
}

#endif // DAEDALUS_PSP
#endif // HLEGRAPHICS_UCODES_UCODE_FB_H_
