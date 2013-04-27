// This is a dummy version of pspgu.h to get code compiling on OSX.

#ifndef PSPGU_H__
#define PSPGU_H__

#include "SysGL/GL.h"

void sceGuFog(float mn, float mx, u32 col);

enum EGuTextureWrapMode
{
	GU_CLAMP			= GL_CLAMP_TO_EDGE,
	GU_REPEAT			= GL_REPEAT,
};

enum EGuMatrixType
{
	GU_PROJECTION		= GL_PROJECTION,
};

struct ScePspFMatrix4
{
	float m[16];
};

void sceGuSetMatrix(EGuMatrixType type, const ScePspFMatrix4 * mtx);

#endif // PSPGU_H__
