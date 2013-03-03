// This is a dummy version of pspgu.h to get code compiling on OSX.

#ifndef PSPGU_H__
#define PSPGU_H__

#include <GL/glfw.h>

enum EGuMode
{
	GU_ALPHA_TEST		= GL_ALPHA_TEST,
	GU_BLEND			= GL_BLEND,
	GU_CULL_FACE		= GL_CULL_FACE,
	GU_DEPTH_TEST		= GL_DEPTH_TEST,
	GU_FOG				= GL_FOG,
	GU_LIGHTING			= GL_LIGHTING,
	GU_SCISSOR_TEST		= GL_SCISSOR_TEST,
	GU_TEXTURE_2D		= GL_TEXTURE_2D,

	GU_CLIP_PLANES		= 0,
};

enum EGuShadeMode
{
	GU_FLAT				= GL_FLAT,
	GU_SMOOTH			= GL_SMOOTH,
};


void sceGuDisable(EGuMode mode);
void sceGuEnable(EGuMode mode);
void sceGuFog(float mn, float mx, u32 col);
void sceGuShadeModel(EGuShadeMode mode);

void sceGuViewport(int x, int y, int w, int h);
void sceGuOffset(float s, float t);

void sceGuScissor(int x0, int y0, int x1, int y1);

enum EGuTextureWrapMode
{
	GU_CLAMP			= GL_CLAMP,
	GU_REPEAT			= GL_REPEAT,
};
enum EGuTextureFilterMode
{
	GU_NEAREST			= GL_NEAREST,
	GU_LINEAR			= GL_LINEAR,
};
void sceGuTexWrap(int u, int v);
void sceGuTexOffset(float s, float t);
void sceGuTexScale(float s, float t);
void sceGuTexFilter(EGuTextureFilterMode u, EGuTextureFilterMode v);
void sceGuTexEnvColor(u32 c);


enum EGuBlendOp
{
	GU_ADD					= GL_FUNC_ADD,
	GU_REVERSE_SUBTRACT		= GL_FUNC_REVERSE_SUBTRACT,
};

enum
{
	GU_SRC_ALPHA			= GL_SRC_ALPHA,
	GU_ONE_MINUS_SRC_ALPHA	= GL_ONE_MINUS_SRC_ALPHA,
	GU_FIX					= GL_CONSTANT_COLOR,		// CORRECT?
};

void sceGuBlendFunc(EGuBlendOp op, int sf, int df, int a, int b);

enum EGuMatrixType
{
	GU_PROJECTION		= GL_PROJECTION,
	GU_VIEW				= 0,			// FIXME
	GU_MODEL			= 0,			// FIXME
};

struct ScePspFMatrix4
{
	float m[16];
};


void sceGuSetMatrix(EGuMatrixType type, const ScePspFMatrix4 * mtx);

enum
{
	GU_SPRITES = 0,		// FIXME
	GU_TRIANGLES			= GL_TRIANGLES,
	GU_TRIANGLE_STRIP		= GL_TRIANGLE_STRIP,
	GU_TRIANGLE_FAN			= GL_TRIANGLE_FAN,
};

enum
{
	GU_TEXTURE_32BITF	= 1<<0,	// 1
	GU_VERTEX_32BITF	= 1<<1,	// 2
	GU_COLOR_8888		= 1<<2,	// 4
	GU_TRANSFORM_2D		= 1<<3,	// 8
	GU_TRANSFORM_3D		= 1<<4,	// 16
};

void sceGuDrawArray(int prim, int vtype, int count, const void * indices, const void * vertices);

#endif // PSPGU_H__
