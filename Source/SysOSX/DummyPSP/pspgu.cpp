#include "stdafx.h"
#include "pspgu.h"

#include <GL/glfw.h>

void sceGuDisable(EGuMode mode)
{
	DAEDALUS_ERROR( "%s: Not implemented", __FUNCTION__ );
}

void sceGuEnable(EGuMode mode)
{
	DAEDALUS_ERROR( "%s: Not implemented", __FUNCTION__ );
}

void sceGuFog(float mn, float mx, u32 col)
{
	DAEDALUS_ERROR( "%s: Not implemented", __FUNCTION__ );
}

void sceGuShadeModel(EGuShadeMode mode)
{
	DAEDALUS_ERROR( "%s: Not implemented", __FUNCTION__ );
}

void sceGuDepthMask(int enable)
{
	DAEDALUS_ERROR( "%s: Not implemented", __FUNCTION__ );
}

void sceGuDepthFunc(EGuCompareOp op)
{
	DAEDALUS_ERROR( "%s: Not implemented", __FUNCTION__ );
}

void sceGuDepthRange(int a, int b)
{
	DAEDALUS_ERROR( "%s: Not implemented", __FUNCTION__ );
}


void sceGuViewport(int x, int y, int w, int h)
{
	DAEDALUS_ERROR( "%s: Not implemented", __FUNCTION__ );
}

void sceGuOffset(float s, float t)
{
	DAEDALUS_ERROR( "%s: Not implemented", __FUNCTION__ );
}


void sceGuScissor(int x0, int y0, int x1, int y1)
{
	DAEDALUS_ERROR( "%s: Not implemented", __FUNCTION__ );
}



void sceGuTexMode(EGuTexMode mode, int maxmips, int a2, int swiz)
{
	DAEDALUS_ERROR( "%s: Not implemented", __FUNCTION__ );
}

void sceGuTexWrap(int u, int v)
{
	DAEDALUS_ERROR( "%s: Not implemented", __FUNCTION__ );
}

void sceGuTexOffset(float s, float t)
{
	DAEDALUS_ERROR( "%s: Not implemented", __FUNCTION__ );
}

void sceGuTexScale(float s, float t)
{
	DAEDALUS_ERROR( "%s: Not implemented", __FUNCTION__ );
}

void sceGuTexFilter(EGuTextureFilterMode u, EGuTextureFilterMode v)
{
	DAEDALUS_ERROR( "%s: Not implemented", __FUNCTION__ );
}

void sceGuTexEnvColor(u32 c)
{
	DAEDALUS_ERROR( "%s: Not implemented", __FUNCTION__ );
}

void sceGuTexImage(int a, int w, int h, int p, void * d)
{
	DAEDALUS_ERROR( "%s: Not implemented", __FUNCTION__ );
}

void sceGuTexFunc(int fn, int b)
{
	DAEDALUS_ERROR( "%s: Not implemented", __FUNCTION__ );
}


void * sceGuGetMemory(size_t len)
{
	DAEDALUS_ERROR( "%s: Not implemented", __FUNCTION__ );
	return NULL;
}



void sceGuBlendFunc(EGuBlendOp op, int sf, int df, int a, int b)
{
	DAEDALUS_ERROR( "%s: Not implemented", __FUNCTION__ );
}


void sceGuAlphaFunc(EGuCompareOp op, int a, int b)
{
	DAEDALUS_ERROR( "%s: Not implemented", __FUNCTION__ );
}


void sceGuSetMatrix(EGuMatrixType type, const ScePspFMatrix4 * mtx)
{
	DAEDALUS_ERROR( "%s: Not implemented", __FUNCTION__ );
}



void sceGuDrawArray(int t, int flag, int num_v, void * x, void * y)
{
	DAEDALUS_ERROR( "%s: Not implemented", __FUNCTION__ );
}
