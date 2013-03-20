#ifndef CONVERTTILE_H__
#define CONVERTTILE_H__

#include "Graphics/TextureFormat.h"

struct NativePf8888;
struct TextureInfo;

bool ConvertTile(const TextureInfo & ti,
				 void * texels,
				 NativePf8888 * palette,
				 ETextureFormat texture_format,
				 u32 pitch);

#endif // CONVERTTILE_H__
