#ifndef SYSGL_GL_H_
#define SYSGL_GL_H_

#include <GL/glew.h>
#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "Base/Types.h"

extern SDL_Window * gWindow;
extern SDL_Renderer * gSdlRenderer;
// FIXME: burn all of this with fire.

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


#endif // SYSGL_GL_H_
