#ifndef HLEGRAPHICS_CONVERTTILE_H_
#define HLEGRAPHICS_CONVERTTILE_H_

#include "Graphics/TextureFormat.h"

struct NativePf8888;
struct TextureInfo;

bool ConvertTile(const TextureInfo & ti,
				 void * texels,
				 NativePf8888 * palette,
				 ETextureFormat texture_format,
				 u32 pitch);

#endif // HLEGRAPHICS_CONVERTTILE_H_
