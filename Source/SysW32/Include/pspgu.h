// This is a dummy version of pspgu.h to get code compiling on OSX.

#ifndef PSPGU_H__
#define PSPGU_H__


#define GL_FALSE 0
#define GL_TRUE 1

void sceGuFog(float mn, float mx, u32 col);

enum EGuTextureWrapMode
{
	GU_CLAMP,
	GU_REPEAT,
};

void sceGuTexOffset(float s, float t);
void sceGuTexScale(float s, float t);

enum EGuMatrixType
{
	GU_PROJECTION,
};

struct ScePspFMatrix4
{
	float m[16];
};


void sceGuSetMatrix(EGuMatrixType type, const ScePspFMatrix4 * mtx);

enum
{
	GU_TRIANGLES,
};

#endif // PSPGU_H__
