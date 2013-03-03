// This is a dummy version of pspgu.h to get code compiling on OSX.

#ifndef PSPGU_H__
#define PSPGU_H__


#define GL_FALSE 0
#define GL_TRUE 1

void sceGuFog(float mn, float mx, u32 col);

void sceGuViewport(int x, int y, int w, int h);
void sceGuOffset(float s, float t);

void sceGuScissor(int x0, int y0, int x1, int y1);

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
	GU_VIEW,
	GU_MODEL,
};

struct ScePspFMatrix4
{
	float m[16];
};


void sceGuSetMatrix(EGuMatrixType type, const ScePspFMatrix4 * mtx);

enum
{
	GU_SPRITES,
	GU_TRIANGLES,
	GU_TRIANGLE_STRIP,
	GU_TRIANGLE_FAN,
};

enum
{
	GU_TEXTURE_32BITF,
	GU_VERTEX_32BITF,
	GU_COLOR_8888,
	GU_TRANSFORM_2D,
	GU_TRANSFORM_3D,
};

void sceGuDrawArray(int t, int flag, int num_v, void * x, void * y);

#endif // PSPGU_H__
