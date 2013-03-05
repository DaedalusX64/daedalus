// This is a dummy version of pspgu.h to get code compiling on OSX.

#ifndef PSPGU_H__
#define PSPGU_H__

#include <GL/glfw.h>

void sceGuFog(float mn, float mx, u32 col);

enum EGuTextureWrapMode
{
	GU_CLAMP			= GL_CLAMP_TO_EDGE,
	GU_REPEAT			= GL_REPEAT,
};

void sceGuTexOffset(float s, float t);
void sceGuTexScale(float s, float t);

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
