#include "stdafx.h"
#include "pspgu.h"

#include <GL/glfw.h>

#include "Graphics/ColourValue.h"

void sceGuDisable(EGuMode mode)
{
	if (mode != 0)
	{
		glDisable(mode);
	}
}

void sceGuEnable(EGuMode mode)
{
	if (mode != 0)
	{
		glEnable(mode);
	}
}

void sceGuFog(float mn, float mx, u32 col)
{
	DAEDALUS_ERROR( "%s: Not implemented", __FUNCTION__ );
}

void sceGuShadeModel(EGuShadeMode mode)
{
	glShadeModel(mode);
}

void sceGuDepthMask(int enable)
{
	glDepthMask(enable);
}

void sceGuDepthFunc(EGuCompareOp op)
{
	glDepthFunc(op);
}

void sceGuDepthRange(int a, int b)
{
	DAEDALUS_ERROR( "%s: Not implemented", __FUNCTION__ );
}


void sceGuViewport(int x, int y, int w, int h)
{
	glViewport(x, y, w, h);
}

void sceGuOffset(float s, float t)
{
	//DAEDALUS_ERROR( "%s: Not implemented", __FUNCTION__ );
}


void sceGuScissor(int x0, int y0, int x1, int y1)
{
	// NB: psp scissor passes absolute coords, OpenGL scissor wants width/height
	glScissor(x0, y0, x1-x0, y1-y0);
}



void sceGuTexMode(EGuTexMode mode, int maxmips, int a2, int swiz)
{
	DAEDALUS_ERROR( "%s: Not implemented", __FUNCTION__ );
}

void sceGuTexWrap(int u, int v)
{
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, u);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, v);
}

void sceGuTexOffset(float s, float t)
{
	//DAEDALUS_ERROR( "%s: Not implemented", __FUNCTION__ );
}

void sceGuTexScale(float s, float t)
{
	//DAEDALUS_ERROR( "%s: Not implemented", __FUNCTION__ );
}

void sceGuTexFilter(EGuTextureFilterMode u, EGuTextureFilterMode v)
{
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, u);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, v);

}

void sceGuTexEnvColor(u32 c)
{
	c32 colour( c );
	const float cv[] = { colour.GetRf(), colour.GetGf(), colour.GetBf(), colour.GetAf() };

	glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, cv);
}

void sceGuTexImage(int a, int w, int h, int p, void * d)
{
	//DAEDALUS_ERROR( "%s: Not implemented", __FUNCTION__ );
}

void sceGuTexFunc(int fn, int b)
{
	//DAEDALUS_ERROR( "%s: Not implemented", __FUNCTION__ );
}


void * sceGuGetMemory(size_t len)
{
	// FIXME: leaky!
	return malloc(len);
}



void sceGuBlendFunc(EGuBlendOp op, int sf, int df, int a, int b)
{
	if (op != 0)
	{
		c32 colour( a );
		glBlendColor( colour.GetRf(), colour.GetGf(), colour.GetBf(), colour.GetAf() );
		glBlendEquation(op);
		glBlendFunc(sf, df);
	}
}


void sceGuAlphaFunc(EGuCompareOp op, int a, int b)
{
	glAlphaFunc(op, (float)a/255.f);
}


void sceGuSetMatrix(EGuMatrixType type, const ScePspFMatrix4 * mtx)
{
	glMatrixMode( type == 0 ? GL_MODELVIEW : (int)type );
    glLoadMatrixf( mtx->m );
}



void sceGuDrawArray(int t, int flag, int num_v, void * x, void * y)
{
	// FIXME!
}
