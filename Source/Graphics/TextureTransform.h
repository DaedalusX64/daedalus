#ifndef GRAPHICS_TEXTURETRANSFORM_H_
#define GRAPHICS_TEXTURETRANSFORM_H_

#include "Graphics/TextureFormat.h"

class c32;

void ClampTexels( void * texels, u32 n64_width, u32 n64_height, u32 native_width, u32 native_height, u32 native_stride, ETextureFormat texture_format );
void Recolour( void * data, void * palette, u32 width, u32 height, u32 stride, ETextureFormat texture_format, c32 colour );
void MirrorTexels( bool mirror_s, bool mirror_t, void * dst, u32 dst_stride, const void * src, u32 src_stride, ETextureFormat tex_fmt, u32 width, u32 height );

#endif // GRAPHICS_TEXTURETRANSFORM_H_
