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

#ifndef UCODE_FB__
#define UCODE_FB__

//ToDO: Implement me PSP
#ifndef DAEDALUS_PSP
void LoadFrameBuffer(u32 origin)
{
	u32 width  = Memory_VI_GetRegister( VI_WIDTH_REG );
	if( width == 0 )
	{
		DAEDALUS_ERROR("Loading 0 size frame buffer?");
		return;
	}

	if( origin <= width*2 )
	{
		DAEDALUS_ERROR("Loading small frame buffer not supported");
		return;
	}

	TextureInfo ti;

	//ToDO: We should use uViWidth+1 and uViHeight+1
#define FB_WIDTH  320
#define FB_HEIGHT 240	

	ti.SetSwapped			(0);
	ti.SetTLutIndex			(0);
	ti.SetTlutAddress		(TLUT_BASE);
	ti.SetTLutFormat		(kTT_RGBA16); 
	ti.SetFormat			(0);
	ti.SetSize				(2);

	ti.SetLoadAddress		(origin - width*2);
	ti.SetWidth				(FB_WIDTH);	
	ti.SetHeight			(FB_HEIGHT);
	ti.SetPitch				(width << 2 >> 1);
	CRefPtr<CNativeTexture> texture = gRenderer->LoadTextureDirectly(ti);

	//
	//Borrowed from N64js
	//
	u16 pixels[FB_WIDTH*FB_HEIGHT];	// TODO: should cache this, but at some point we'll need to deal with variable framebuffer size, so do this later.
	u32 src_offset = 0;

	for (u32 y = 0; y < FB_HEIGHT; ++y) 
	{
		u32 dst_row_offset = (FB_HEIGHT-1-y) * FB_WIDTH;
		u32 dst_offset     = dst_row_offset;

		for (u32 x = 0; x < FB_WIDTH; ++x)
		{
			pixels[dst_offset] = (g_pu8RamBase[(origin + src_offset)^U8_TWIDDLE]<<8) | g_pu8RamBase[(origin + src_offset+  1)^U8_TWIDDLE] | 1;  // NB: or 1 to ensure we have alpha
			dst_offset += 1;
			src_offset += 2;
		}
	}
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, FB_WIDTH, FB_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, pixels);
	gRenderer->Draw2DTexture(0, 0, FB_WIDTH, FB_HEIGHT, 0, 0, FB_WIDTH, FB_HEIGHT, texture);
}


void DrawFrameBuffer(u32 origin)
{
	gRenderer->SetVIScales();
	gRenderer->BeginScene();
	LoadFrameBuffer(origin);
	gRenderer->EndScene();
	gGraphicsPlugin->UpdateScreen();
	
}
#endif	// DAEDALUS_PSP
#endif // UCODE_FB__
