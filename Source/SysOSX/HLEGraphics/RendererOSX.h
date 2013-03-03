#ifndef RENDEREROSX_H__
#define RENDEREROSX_H__

#include "HLEGraphics/BaseRenderer.h"

class RendererOSX : public BaseRenderer
{
public:
	virtual void		RenderUsingCurrentBlendMode( DaedalusVtx * p_vertices, u32 num_vertices, u32 triangle_mode, u32 render_mode, bool disable_zbuffer );
	virtual void		Draw2DTextureBlit(f32 x, f32 y, f32 width, f32 height, f32 u0, f32 v0, f32 u1, f32 v1, CNativeTexture * texture);
	virtual void		Draw2DTexture(f32 frameX, f32 frameY, f32 frameW, f32 frameH, f32 imageX, f32 imageY, f32 imageW, f32 imageH);
	virtual void		Draw2DTextureR(f32 x0, f32 y0, f32 x1, f32 y1, f32 x2, f32 y2, f32 x3, f32 y3, f32 s, f32 t);

private:
	void 				RenderDaedalusVtx(int prim, const DaedalusVtx * vertices, int count);
};

// NB: this is equivalent to gRenderer, but points to the implementation class, for platform-specific functionality.
extern RendererOSX * gRendererOSX;

#endif // RENDEREROSX_H__
