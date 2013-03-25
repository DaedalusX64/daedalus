#include "stdafx.h"
#include "RendererGL.h"

#include <pspgu.h>

#include <vector>

#include <GL/glfw.h>

#include "Core/ROM.h"
#include "Debug/DBGConsole.h"
#include "Graphics/ColourValue.h"
#include "Graphics/GraphicsContext.h"
#include "Graphics/NativeTexture.h"
#include "HLEGraphics/DLDebug.h"
#include "HLEGraphics/RDPStateManager.h"
#include "OSHLE/ultra_gbi.h"

BaseRenderer * gRenderer   = NULL;
RendererGL *   gRendererGL = NULL;

/* OpenGL 3.0 */
typedef void (APIENTRY * PFN_glGenVertexArrays)(GLsizei n, GLuint *arrays);
typedef void (APIENTRY * PFN_glBindVertexArray)(GLuint array);
typedef void (APIENTRY * PFN_glDeleteVertexArrays)(GLsizei n, GLuint *arrays);

static PFN_glGenVertexArrays            pglGenVertexArrays = NULL;
static PFN_glBindVertexArray            pglBindVertexArray = NULL;
static PFN_glDeleteVertexArrays         pglDeleteVertexArrays = NULL;

static const u32 kNumTextures = 2;

#define RESOLVE_GL_FCN(type, var, name) \
    if (status == GL_TRUE) \
    {\
        var = (type)glfwGetProcAddress((name));\
        if ((var) == NULL)\
        {\
            status = GL_FALSE;\
        }\
    }



enum
{
	kPositionBuffer,
	kTexCoordBuffer,
	kColorBuffer,

	kNumBuffers,
};

static GLuint gVAO;
static GLuint gVBOs[kNumBuffers];

const int kMaxVertices = 1000;

static float 	gPositionBuffer[kMaxVertices][3];
static float 	gTexCoordBuffer[kMaxVertices][2];
static u32 		gColorBuffer[kMaxVertices];

bool initgl()
{
    GLboolean status = GL_TRUE;
    RESOLVE_GL_FCN(PFN_glGenVertexArrays, pglGenVertexArrays, "glGenVertexArrays");
    RESOLVE_GL_FCN(PFN_glDeleteVertexArrays, pglDeleteVertexArrays, "glDeleteVertexArrays");
    RESOLVE_GL_FCN(PFN_glBindVertexArray, pglBindVertexArray, "glBindVertexArray");

	pglGenVertexArrays(1, &gVAO);
	pglBindVertexArray(gVAO);

	glGenBuffers(kNumBuffers, gVBOs);

	glBindBuffer(GL_ARRAY_BUFFER, gVBOs[kPositionBuffer]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(gPositionBuffer), gPositionBuffer, GL_DYNAMIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, gVBOs[kTexCoordBuffer]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(gTexCoordBuffer), gTexCoordBuffer, GL_DYNAMIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, gVBOs[kColorBuffer]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(gColorBuffer), gColorBuffer, GL_DYNAMIC_DRAW);
	return true;
}


void sceGuFog(float mn, float mx, u32 col)
{
	//DAEDALUS_ERROR( "%s: Not implemented", __FUNCTION__ );
}


ScePspFMatrix4		gProjection;
void sceGuSetMatrix(EGuMatrixType type, const ScePspFMatrix4 * mtx)
{
	if (type == GL_PROJECTION)
	{
		memcpy(&gProjection, mtx, sizeof(gProjection));
	}
}



struct ShaderProgram
{
	u64					mMux;
	u32					mCycleType;
	u32					mAlphaThreshold;
	GLuint 				program;

	GLint				uloc_project;
	GLint				uloc_primcol;
	GLint				uloc_envcol;
	GLint				uloc_primlodfrac;

	GLint				uloc_texscale[kNumTextures];
	GLint				uloc_texoffset[kNumTextures];
	GLint				uloc_texture[kNumTextures];
};
static std::vector<ShaderProgram *>		gShaders;


/* Creates a shader object of the specified type using the specified text
 */
static GLuint make_shader(GLenum type, const char* shader_src)
{
	//printf("%d - %s\n", type, shader_src);

	GLuint shader = glCreateShader(type);
	if (shader != 0)
	{
		glShaderSource(shader, 1, (const GLchar**)&shader_src, NULL);
		glCompileShader(shader);

		GLint shader_ok;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &shader_ok);
		if (shader_ok != GL_TRUE)
		{
			GLsizei log_length;
			char info_log[8192];

			fprintf(stderr, "ERROR: Failed to compile %s shader\n", (type == GL_FRAGMENT_SHADER) ? "fragment" : "vertex" );
			glGetShaderInfoLog(shader, 8192, &log_length,info_log);
			fprintf(stderr, "ERROR: \n%s\n\n", info_log);
			glDeleteShader(shader);
			shader = 0;
		}
	}
	return shader;
}

/* Creates a program object using the specified vertex and fragment text
 */
static GLuint make_shader_program(const char* vertex_shader_src, const char* fragment_shader_src)
{
	GLuint program = 0u;
	GLint program_ok;
	GLuint vertex_shader = 0u;
	GLuint fragment_shader = 0u;
	GLsizei log_length;
	char info_log[8192];

	vertex_shader = make_shader(GL_VERTEX_SHADER, vertex_shader_src);
	if (vertex_shader != 0u)
	{
		fragment_shader = make_shader(GL_FRAGMENT_SHADER, fragment_shader_src);
		if (fragment_shader != 0u)
		{
			/* make the program that connect the two shader and link it */
			program = glCreateProgram();
			if (program != 0u)
			{
				/* attach both shader and link */
				glAttachShader(program, vertex_shader);
				glAttachShader(program, fragment_shader);

				glLinkProgram(program);
				glGetProgramiv(program, GL_LINK_STATUS, &program_ok);

				if (program_ok != GL_TRUE)
				{
					fprintf(stderr, "ERROR, failed to link shader program\n");
					glGetProgramInfoLog(program, 8192, &log_length, info_log);
					fprintf(stderr, "ERROR: \n%s\n\n", info_log);
					glDeleteProgram(program);
					glDeleteShader(fragment_shader);
					glDeleteShader(vertex_shader);
					program = 0u;
				}
			}
		}
		else
		{
			fprintf(stderr, "ERROR: Unable to load fragment shader\n");
			glDeleteShader(vertex_shader);
		}
	}
	else
	{
		fprintf(stderr, "ERROR: Unable to load vertex shader\n");
	}
	return program;
}


static const char * kRGBParams32[] =
{
	"combined.rgb",  "tex0.rgb",
	"tex1.rgb",      "prim.rgb",
	"shade.rgb",     "env.rgb",
	"one.rgb",       "combined.a",
	"tex0.a",        "tex1.a",
	"prim.a",        "shade.a",
	"env.a",         "lod_frac",
	"prim_lod_frac", "k5",
	"?",             "?",
	"?",             "?",
	"?",             "?",
	"?",             "?",
	"?",             "?",
	"?",             "?",
	"?",             "?",
	"?",             "zero.rgb",
};

static const char * kRGBParams16[] = {
	"combined.rgb", "tex0.rgb",
	"tex1.rgb",     "prim.rgb",
	"shade.rgb",    "env.rgb",
	"one.rgb",      "combined.a",
	"tex0.a",       "tex1.a",
	"prim.a",       "shade.a",
	"env.a",        "lod_frac",
	"prim_lod_frac", "zero.rgb",
};

static const char * kRGBParams8[8] = {
	"combined.rgb", "tex0.rgb",
	"tex1.rgb",     "prim.rgb",
	"shade.rgb",    "env.rgb",
	"one.rgb",      "zero.rgb",
};

static const char * kAlphaParams8[8] = {
	"combined.a", "tex0.a",
	"tex1.a",     "prim.a",
	"shade.a",    "env.a",
	"one.a",      "zero.a"
};

static void SprintMux(char (&body)[1024], u64 mux, u32 cycle_type, u32 alpha_threshold)
{
	u32 mux0 = (u32)(mux>>32);
	u32 mux1 = (u32)(mux);

	u32 aRGB0  = (mux0>>20)&0x0F;	// c1 c1		// a0
	u32 bRGB0  = (mux1>>28)&0x0F;	// c1 c2		// b0
	u32 cRGB0  = (mux0>>15)&0x1F;	// c1 c3		// c0
	u32 dRGB0  = (mux1>>15)&0x07;	// c1 c4		// d0

	u32 aA0    = (mux0>>12)&0x07;	// c1 a1		// Aa0
	u32 bA0    = (mux1>>12)&0x07;	// c1 a2		// Ab0
	u32 cA0    = (mux0>>9 )&0x07;	// c1 a3		// Ac0
	u32 dA0    = (mux1>>9 )&0x07;	// c1 a4		// Ad0

	u32 aRGB1  = (mux0>>5 )&0x0F;	// c2 c1		// a1
	u32 bRGB1  = (mux1>>24)&0x0F;	// c2 c2		// b1
	u32 cRGB1  = (mux0    )&0x1F;	// c2 c3		// c1
	u32 dRGB1  = (mux1>>6 )&0x07;	// c2 c4		// d1

	u32 aA1    = (mux1>>21)&0x07;	// c2 a1		// Aa1
	u32 bA1    = (mux1>>3 )&0x07;	// c2 a2		// Ab1
	u32 cA1    = (mux1>>18)&0x07;	// c2 a3		// Ac1
	u32 dA1    = (mux1    )&0x07;	// c2 a4		// Ad1

	if (cycle_type == CYCLE_FILL)
	{
		strcpy(body, "\tcol = shade;\n");
	}
	else if (cycle_type == CYCLE_COPY)
	{
		strcpy(body, "\tcol = tex0;\n");
	}
	else if (cycle_type == CYCLE_1CYCLE)
	{
		sprintf(body, "\tcol.rgb = (%s - %s) * %s + %s;\n"
					  "\tcol.a   = (%s - %s) * %s + %s;\n",
					  kRGBParams16[aRGB0], kRGBParams16[bRGB0], kRGBParams32[cRGB0], kRGBParams8[dRGB0],
					  kAlphaParams8[aA0],  kAlphaParams8[bA0],  kAlphaParams8[cA0],  kAlphaParams8[dA0]);
	}
	else
	{
		sprintf(body, "\tcol.rgb = (%s - %s) * %s + %s;\n"
					  "\tcol.a   = (%s - %s) * %s + %s;\n"
					  "\tcombined = col;\n"
					  "\ttex0 = tex1;\n"		// NB: tex0 becomes tex1 on the second cycle - see mame.
					  "\tcol.rgb = (%s - %s) * %s + %s;\n"
					  "\tcol.a   = (%s - %s) * %s + %s;\n",
					  kRGBParams16[aRGB0], kRGBParams16[bRGB0], kRGBParams32[cRGB0], kRGBParams8[dRGB0],
					  kAlphaParams8[aA0],  kAlphaParams8[bA0],  kAlphaParams8[cA0],  kAlphaParams8[dA0],
					  kRGBParams16[aRGB1], kRGBParams16[bRGB1], kRGBParams32[cRGB1], kRGBParams8[dRGB1],
					  kAlphaParams8[aA1],  kAlphaParams8[bA1],  kAlphaParams8[cA1],  kAlphaParams8[dA1]);
	}

	if (alpha_threshold > 0)
	{
		char * p = body + strlen(body);
		sprintf(p, "\tif(col.a < %f) discard;\n", (float)alpha_threshold / 255.f);
	}
}

static const char* default_vertex_shader =
"#version 150\n"
"uniform mat4 uProject;\n"
"uniform vec2 uTexScale0;\n"
"uniform vec2 uTexScale1;\n"
"uniform vec2 uTexOffset0;\n"
"uniform vec2 uTexOffset1;\n"
"in      vec3 in_pos;\n"
"in      vec2 in_uv;\n"
"in      vec4 in_col;\n"
"out     vec2 v_uv0;\n"
"out     vec2 v_uv1;\n"
"out     vec4 v_col;\n"
"\n"
"void main()\n"
"{\n"
"	v_uv0 = (in_uv - uTexOffset0) / uTexScale0;\n"
"	v_uv1 = (in_uv - uTexOffset1) / uTexScale1;\n"
"	v_col = in_col;\n"
"	gl_Position = uProject * vec4(in_pos, 1.0);\n"
"}\n";

static const char* default_fragment_shader_fmt =
"#version 150\n"
"uniform sampler2D uTexture0;\n"
"uniform sampler2D uTexture1;\n"
"uniform vec4 uPrimColour;\n"
"uniform vec4 uEnvColour;\n"
"uniform float uPrimLODFrac;\n"
"in      vec2 v_uv0;\n"
"in      vec2 v_uv1;\n"
"in      vec4 v_col;\n"
"out     vec4 fragcol;\n"
"void main()\n"
"{\n"
"	vec4 shade = v_col;\n"
"	vec4 prim  = uPrimColour;\n"
"	vec4 env   = uEnvColour;\n"
"	vec4 one   = vec4(1,1,1,1);\n"
"	vec4 zero  = vec4(0,0,0,0);\n"
"	vec4 tex0  = texture(uTexture0, v_uv0);\n"
"	vec4 tex1  = texture(uTexture1, v_uv1);\n"
"	vec4 col;\n"
"	vec4 combined = vec4(0,0,0,1);\n"
"	float lod_frac      = 0.0;\n"		// FIXME
"	float prim_lod_frac = uPrimLODFrac;\n"
"	float k5            = 0.0;\n"		// FIXME
"%s\n"		// Body is injected here
"	fragcol = col;\n"
"}\n";

static void InitShaderProgram(ShaderProgram * program, u64 mux, u32 cycle_type, u32 alpha_threshold, GLuint shader_program)
{
	program->mMux              = mux;
	program->mCycleType        = cycle_type;
	program->mAlphaThreshold   = alpha_threshold;
	program->program           = shader_program;
	program->uloc_project      = glGetUniformLocation(shader_program, "uProject");
	program->uloc_primcol      = glGetUniformLocation(shader_program, "uPrimColour");
	program->uloc_envcol       = glGetUniformLocation(shader_program, "uEnvColour");
	program->uloc_primlodfrac  = glGetUniformLocation(shader_program, "uPrimLODFrac");

	program->uloc_texoffset[0] = glGetUniformLocation(shader_program, "uTexOffset0");
	program->uloc_texscale[0]  = glGetUniformLocation(shader_program, "uTexScale0");
	program->uloc_texture [0]  = glGetUniformLocation(shader_program, "uTexture0");

	program->uloc_texoffset[1] = glGetUniformLocation(shader_program, "uTexOffset1");
	program->uloc_texscale[1]  = glGetUniformLocation(shader_program, "uTexScale1");
	program->uloc_texture[1]   = glGetUniformLocation(shader_program, "uTexture1");

	GLuint attrloc;
	attrloc = glGetAttribLocation(program->program, "in_pos");
	glBindBuffer(GL_ARRAY_BUFFER, gVBOs[kPositionBuffer]);
	glEnableVertexAttribArray(attrloc);
	glVertexAttribPointer(attrloc, 3, GL_FLOAT, GL_FALSE, 0, 0);

	attrloc = glGetAttribLocation(program->program, "in_uv");
	glBindBuffer(GL_ARRAY_BUFFER, gVBOs[kTexCoordBuffer]);
	glEnableVertexAttribArray(attrloc);
	glVertexAttribPointer(attrloc, 2, GL_FLOAT, GL_FALSE, 0, 0);

	attrloc = glGetAttribLocation(program->program, "in_col");
	glBindBuffer(GL_ARRAY_BUFFER, gVBOs[kColorBuffer]);
	glEnableVertexAttribArray(attrloc);
	glVertexAttribPointer(attrloc, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, 0);
}

static ShaderProgram * GetShaderForCurrentMode(u64 mux, u32 cycle_type, u32 alpha_threshold)
{
	// In fill/cycle modes, we ignore the mux. Set it to zero so we don't create unecessary shaders.
	if (cycle_type == CYCLE_FILL || cycle_type == CYCLE_COPY)
		mux = 0;

	// Not sure about this. Should CYCLE_FILL have alpha kill?
	if (cycle_type == CYCLE_FILL)
		alpha_threshold = 0;

	for (u32 i = 0; i < gShaders.size(); ++i)
	{
		ShaderProgram * program = gShaders[i];
		if (program->mMux == mux && program->mCycleType == cycle_type && program->mAlphaThreshold == alpha_threshold)
			return program;
	}

	char body[1024];
	SprintMux(body, mux, cycle_type, alpha_threshold);

	char frag_shader[2048];
	sprintf(frag_shader, default_fragment_shader_fmt, body);

	GLuint shader_program = make_shader_program(default_vertex_shader, frag_shader);
	if (shader_program == 0)
	{
		fprintf(stderr, "ERROR: during creation of the shader program\n");
		return NULL;
	}

	ShaderProgram * program = new ShaderProgram;
	InitShaderProgram(program, mux, cycle_type, alpha_threshold, shader_program);
	gShaders.push_back(program);

	return program;
}

void RendererGL::RestoreRenderStates()
{
	// Initialise the device to our default state

	// No fog
	glDisable(GL_FOG);

	// We do our own culling
	glDisable(GL_CULL_FACE);

	u32 width, height;
	CGraphicsContext::Get()->GetScreenSize(&width, &height);

	glScissor(0,0, width,height);
	glEnable(GL_SCISSOR_TEST);

	// We do our own lighting
	glDisable(GL_LIGHTING);

	glBlendColor(0.f, 0.f, 0.f, 0.f);
	glBlendEquation(GL_ADD);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glEnable(GL_BLEND);
	//glDisable( GL_BLEND ); // Breaks Tarzan's text in menus

	// Default is ZBuffer disabled
	glDepthMask(GL_FALSE);		// GL_FALSE to disable z-writes
	glDepthFunc(GL_LEQUAL);
	glDisable(GL_DEPTH_TEST);

	// Initialise all the renderstate to our defaults.
	glShadeModel(GL_SMOOTH);

	//glFog(near,far,mFogColour);

	// Enable this for rendering decals (glPolygonOffset).
	glEnable(GL_POLYGON_OFFSET_FILL);
}

// Strip out vertex stream into separate buffers.
// TODO(strmnnrmn): Renderer should support generating this data directly.
void RendererGL::RenderDaedalusVtx(int prim, const DaedalusVtx * vertices, int count)
{
	DAEDALUS_ASSERT(count <= kMaxVertices, "Too many vertices!");

	// Avoid crashing in the unlikely even that our buffers aren't long enough.
	if (count > kMaxVertices)
		count = kMaxVertices;

	for (int i = 0; i < count; ++i)
	{
		const DaedalusVtx * vtx = &vertices[i];

		gPositionBuffer[i][0] = vtx->Position.x;
		gPositionBuffer[i][1] = vtx->Position.y;
		gPositionBuffer[i][2] = vtx->Position.z;

		gTexCoordBuffer[i][0] = vtx->Texture.x;
		gTexCoordBuffer[i][1] = vtx->Texture.y;

		gColorBuffer[i] = vtx->Colour.GetColour();
	}

	RenderDaedalusVtxStreams(prim, &gPositionBuffer[0][0], &gTexCoordBuffer[0][0], &gColorBuffer[0], count);
}

void RendererGL::RenderDaedalusVtxStreams(int prim, const float * positions, const float * uvs, const u32 * colours, int count)
{
	glBindBuffer(GL_ARRAY_BUFFER, gVBOs[kPositionBuffer]);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * 3 * count, positions);

	glBindBuffer(GL_ARRAY_BUFFER, gVBOs[kTexCoordBuffer]);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * 2 * count, uvs);

	glBindBuffer(GL_ARRAY_BUFFER, gVBOs[kColorBuffer]);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(u32) * count, colours);

	glDrawArrays(prim, 0, count);
}

/*

Possible Blending Inputs:

    In  -   Input from color combiner
    Mem -   Input from current frame buffer
    Fog -   Fog generator
    BL  -   Blender

Possible Blending Factors:
    A-IN    -   Alpha from color combiner
    A-MEM   -   Alpha from current frame buffer
    (1-A)   -
    A-FOG   -   Alpha of fog color
    A-SHADE -   Alpha of shade
    1   -   1
    0   -   0

*/

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
const char * sc_szBlClr[4] = { "In",  "Mem",  "Bl",     "Fog" };
const char * sc_szBlA1[4]  = { "AIn", "AFog", "AShade", "0" };
const char * sc_szBlA2[4]  = { "1-A", "AMem", "1",      "?" };

static inline void DebugBlender( u32 blender )
{
	static u32 mBlender = 0;

	if(mBlender != blender)
	{
		printf( "********************************\n\n" );
		printf( "Unknown Blender: %04x - %s * %s + %s * %s || %s * %s + %s * %s\n",
				blender,
				sc_szBlClr[(blender>>14) & 0x3], sc_szBlA1[(blender>>10) & 0x3], sc_szBlClr[(blender>>6) & 0x3], sc_szBlA2[(blender>>2) & 0x3],
				sc_szBlClr[(blender>>12) & 0x3], sc_szBlA1[(blender>> 8) & 0x3], sc_szBlClr[(blender>>4) & 0x3], sc_szBlA2[(blender   ) & 0x3]);
		printf( "********************************\n\n" );
		mBlender = blender;
	}
}
#endif

static void InitBlenderMode( u32 blendmode )					// Set Alpha Blender mode
{
	switch ( blendmode )
	{
	//case 0x0044:					// ?
	//case 0x0055:					// ?
	case 0x0c08:					// In * 0 + In * 1 || :In * AIn + In * 1-A				Tarzan - Medalion in bottom part of the screen
	//case 0x0c19:					// ?
	case 0x0f0a:					// In * 0 + In * 1 || :In * 0 + In * 1					SSV - ??? and MM - Walls, Wipeout - Mountains
	case 0x0fa5:					// In * 0 + Bl * AMem || :In * 0 + Bl * AMem			OOT Menu
	//case 0x5f50:					// ?
	case 0x8410:					// Bl * AFog + In * 1-A || :In * AIn + Mem * 1-A		Paper Mario Menu
	case 0xc302:					// Fog * AIn + In * 1-A || :In * 0 + In * 1				ISS64 - Ground
	case 0xc702:					// Fog * AFog + In * 1-A || :In * 0 + In * 1			Donald Duck - Sky
	//case 0xc811:					// ?
	case 0xfa00:					// Fog * AShade + In * 1-A || :Fog * AShade + In * 1-A	F-Zero - Power Roads
	//case 0x07c2:					// In * AFog + Fog * 1-A || In * 0 + In * 1				Conker - ??
		glDisable(GL_BLEND);
		break;

	//
	// Add here blenders which work fine with default case but causes too much spam, this disabled in release mode
	//
#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	//case 0x55f0:					// Mem * AFog + Fog * 1-A || :Mem * AFog + Fog * 1-A	Bust a Move 3 - ???
	case 0x0150:					// In * AIn + Mem * 1-A || :In * AFog + Mem * 1-A		Spiderman - Waterfall Intro
	case 0x0f5a:					// In * 0 + Mem * 1 || :In * 0 + Mem * 1				Starwars Racer
	case 0x0010:					// In * AIn + In * 1-A || :In * AIn + Mem * 1-A			Hey You Pikachu - Shadow
	case 0x0040:					// In * AIn + Mem * 1-A || :In * AIn + In * 1-A			Mario - Princess peach text
	case 0x0050:					// In * AIn + Mem * 1-A || :In * AIn + Mem * 1-A:		SSV - TV Screen and SM64 text
	case 0x04d0:					// In * AFog + Fog * 1-A || In * AIn + Mem * 1-A		Conker's Eyes
	case 0x0c18:					// In * 0 + In * 1 || :In * AIn + Mem * 1-A:			SSV - WaterFall and dust
	case 0xc410:					// Fog * AFog + In * 1-A || :In * AIn + Mem * 1-A		Donald Duck - Stars
	case 0xc810:					// Fog * AShade + In * 1-A || :In * AIn + Mem * 1-A		SSV - Fog? and MM - Shadows
	case 0xcb02:					// Fog * AShade + In * 1-A || :In * 0 + In * 1			Doom 64 - Weapons
		glBlendColor(0.f, 0.f, 0.f, 0.f);
		glBlendEquation(GL_FUNC_ADD);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_BLEND);
		break;
#endif
	//
	// Default case should handle most blenders, ignore most unknown blenders unless something is messed up
	//
	default:
		// Hack for shadows in ISS64
		// FIXME(strmnnrmn): not sure about these.
		// if(g_ROM.GameHacks == ISS64)
		// {
		// 	if (blendmode == 0xff5a)	// Actual shadow
		// 	{
		//		glBlendFunc(GL_FUNC_REVERSE_SUBTRACT, GL_SRC_ALPHA, GL_FIX, 0, 0);
		// 		glEnable(GL_BLEND);
		// 	}
		// 	else if (blendmode == 0x0050) // Box that appears under the players..
		// 	{
		// 		glBlendFunc(GL_FUNC_ADD, GL_SRC_ALPHA, GL_FIX, 0, 0x00ffffff);
		// 		glEnable(GL_BLEND);
		// 	}
		// }
		// else
		{
#ifdef DAEDALUS_DEBUG_DISPLAYLIST
			DebugBlender( blendmode );
			DL_PF( "		 Blend: SRCALPHA/INVSRCALPHA (default: 0x%04x)", blendmode );
#endif
			glBlendColor(0.f, 0.f, 0.f, 0.f);
			glBlendEquation(GL_FUNC_ADD);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glEnable(GL_BLEND);
		}
		break;
	}
}


void RendererGL::PrepareRenderState(const float (&mat_project)[16], bool disable_zbuffer, bool identity_uv_transform)
{
	DAEDALUS_PROFILE( "RendererGL::PrepareRenderState" );

	if ( disable_zbuffer )
	{
		glDisable(GL_DEPTH_TEST);
		glDepthMask(GL_FALSE);
	}
	else
	{
		// Decal mode
		if( gRDPOtherMode.zmode == 3 )
		{
			glPolygonOffset(-1.0, -1.0);
		}
		else
		{
			glPolygonOffset(0.0, 0.0);
		}

		// Enable or Disable ZBuffer test
		if ( (mTnL.Flags.Zbuffer & gRDPOtherMode.z_cmp) | gRDPOtherMode.z_upd )
		{
			glEnable(GL_DEPTH_TEST);
		}
		else
		{
			glDisable(GL_DEPTH_TEST);
		}

		glDepthMask(gRDPOtherMode.z_upd ? GL_TRUE : GL_FALSE);
	}


	u32 cycle_mode = gRDPOtherMode.cycle_type;

	// Initiate Blender
	if(cycle_mode < CYCLE_COPY)
	{
		gRDPOtherMode.force_bl ? InitBlenderMode( gRDPOtherMode.blender ) : glDisable( GL_BLEND );
	}

	u32 alpha_threshold = 0;

	// Initiate Alpha test
	if( (gRDPOtherMode.alpha_compare == G_AC_THRESHOLD) && !gRDPOtherMode.alpha_cvg_sel )
	{
		// G_AC_THRESHOLD || G_AC_DITHER
		// FIXME(strmnnrmn): alpha func: (mAlphaThreshold | g_ROM.ALPHA_HACK) ? GL_GEQUAL : GL_GREATER
		alpha_threshold = mAlphaThreshold;
	}
	else if (gRDPOtherMode.cvg_x_alpha)
	{
		// Going over 0x70 brakes OOT, but going lesser than that makes lines on games visible...ex: Paper Mario.
		// ALso going over 0x30 breaks the birds in Tarzan :(. Need to find a better way to leverage this.
		alpha_threshold = 0x70;
	}
	else
	{
		// Use CVG for pixel alpha
		alpha_threshold = 0;
	}

	const ShaderProgram * program = GetShaderForCurrentMode(mMux, cycle_mode, alpha_threshold);
	if (program == NULL)
	{
		// There must have been some failure to compile the shader. Abort!
		DBGConsole_Msg(0, "Couldn't generate a shader for mux %llx, cycle %d, alpha %d\n", mMux, cycle_mode, alpha_threshold);
		return;
	}

	glUseProgram(program->program);

	glUniformMatrix4fv(program->uloc_project, 1, GL_FALSE, mat_project);

	glUniform4f(program->uloc_primcol, mPrimitiveColour.GetRf(), mPrimitiveColour.GetGf(), mPrimitiveColour.GetBf(), mPrimitiveColour.GetAf());
	glUniform4f(program->uloc_envcol,  mEnvColour.GetRf(),       mEnvColour.GetGf(),       mEnvColour.GetBf(),       mEnvColour.GetAf());
	glUniform1f(program->uloc_primlodfrac, mPrimLODFraction);

	// Second texture is sampled in 2 cycle mode if text_lod is clear (when set,
	// gRDPOtherMode.text_lod enables mipmapping, but we just set lod_frac to 0.
	bool use_t1 = cycle_mode == CYCLE_2CYCLE;

	bool install_textures[] = { true, use_t1 };

	for (u32 i = 0; i < kNumTextures; ++i)
	{
		if (!install_textures[i])
			continue;

		CNativeTexture * texture = mBoundTexture[i];

		if (texture != NULL)
		{
			glActiveTexture(GL_TEXTURE0 + i);

			texture->InstallTexture();

			// NB: think this can be done just once per program.
			glUniform1i(program->uloc_texture[i], i);

			if (identity_uv_transform)
			{
				glUniform2f(program->uloc_texoffset[i], 0.f, 0.f);
				glUniform2f(program->uloc_texscale[i], 1.f, 1.f);
			}
			else
			{
				float shifts = kShiftScales[mTexShift[i].s];
				float shiftt = kShiftScales[mTexShift[i].t];

				glUniform2f(program->uloc_texoffset[i], mTileTopLeft[i].x * shifts, mTileTopLeft[i].y * shiftt);
				glUniform2f(program->uloc_texscale[i], (float)texture->GetCorrectedWidth() * shifts, (float)texture->GetCorrectedHeight() * shiftt);
			}

			if( (gRDPOtherMode.text_filt != G_TF_POINT) | (gGlobalPreferences.ForceLinearFilter) )
			{
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			}
			else
			{
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			}

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, mTexWrap[i].u);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, mTexWrap[i].v);
		}
	}
}

// FIXME(strmnnrmn): for fill/copy modes this does more work than needed.
// It ends up copying colour/uv coords when not needed, and can use a shader uniform for the fill colour.
void RendererGL::RenderTriangles( DaedalusVtx * p_vertices, u32 num_vertices, bool disable_zbuffer )
{
	if (mTnL.Flags.Texture)
	{
		UpdateTileSnapshots( mTextureTile );
	}

	bool identity_uv_transform = (mTnL.Flags._u32 & (TNL_LIGHT|TNL_TEXGEN)) == (TNL_LIGHT|TNL_TEXGEN);

	PrepareRenderState(gProjection.m, disable_zbuffer, identity_uv_transform);
	RenderDaedalusVtx(GL_TRIANGLES, p_vertices, num_vertices);
}

void RendererGL::TexRect( u32 tile_idx, const v2 & xy0, const v2 & xy1, const v2 & uv0_, const v2 & uv1_ )
{
	// FIXME(strmnnrmn): in copy mode, depth buffer is always disabled. Might not need to check this explicitly.

	UpdateTileSnapshots( tile_idx );

	// NB: we have to do this after UpdateTileSnapshot, as it set up mTileTopLeft etc.
	// We have to do it before PrepareRenderState, because those values are applied to the graphics state.
	v2 uv0 = uv0_;
	v2 uv1 = uv1_;
	PrepareTexRectUVs(&uv0, &uv1);

	PrepareRenderState(mScreenToDevice.mRaw, gRDPOtherMode.depth_source ? false : true, false);

	v2 screen0;
	v2 screen1;
	ConvertN64ToScreen( xy0, screen0 );
	ConvertN64ToScreen( xy1, screen1 );

	DL_PF( "    Screen:  %.1f,%.1f -> %.1f,%.1f", screen0.x, screen0.y, screen1.x, screen1.y );
	DL_PF( "    Texture: %.1f,%.1f -> %.1f,%.1f", uv0.x, uv0.y, uv1.x, uv1.y );

	const f32 depth = gRDPOtherMode.depth_source ? mPrimDepth : 0.0f;

	float positions[] = {
		screen0.x, screen0.y, depth,
		screen1.x, screen0.y, depth,
		screen0.x, screen1.y, depth,
		screen1.x, screen1.y, depth,
	};

	float uvs[] = {
		uv0.x, uv0.y,
		uv1.x, uv0.y,
		uv0.x, uv1.y,
		uv1.x, uv1.y,
	};

	u32 colours[] = {
		0xffffffff,
		0xffffffff,
		0xffffffff,
		0xffffffff,
	};

	RenderDaedalusVtxStreams(GL_TRIANGLE_STRIP, positions, uvs, colours, 4);

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	++mNumRect;
#endif
}

void RendererGL::TexRectFlip( u32 tile_idx, const v2 & xy0, const v2 & xy1, const v2 & uv0_, const v2 & uv1_ )
{
	UpdateTileSnapshots( tile_idx );

	// NB: we have to do this after UpdateTileSnapshot, as it set up mTileTopLeft etc.
	// We have to do it before PrepareRenderState, because those values are applied to the graphics state.
	v2 uv0 = uv0_;
	v2 uv1 = uv1_;
	PrepareTexRectUVs(&uv0, &uv1);

	PrepareRenderState(mScreenToDevice.mRaw, gRDPOtherMode.depth_source ? false : true, false);

	v2 screen0;
	v2 screen1;
	ConvertN64ToScreen( xy0, screen0 );
	ConvertN64ToScreen( xy1, screen1 );

	DL_PF( "    Screen:  %.1f,%.1f -> %.1f,%.1f", screen0.x, screen0.y, screen1.x, screen1.y );
	DL_PF( "    Texture: %.1f,%.1f -> %.1f,%.1f", uv0.x, uv0.y, uv1.x, uv1.y );

	const f32 depth = gRDPOtherMode.depth_source ? mPrimDepth : 0.0f;

	float positions[] = {
		screen0.x, screen0.y, depth,
		screen1.x, screen0.y, depth,
		screen0.x, screen1.y, depth,
		screen1.x, screen1.y, depth,
	};

	float uvs[] = {
		uv0.x, uv0.y,
		uv0.x, uv1.y,
		uv1.x, uv0.y,
		uv1.x, uv1.y,
	};

	u32 colours[] = {
		0xffffffff,
		0xffffffff,
		0xffffffff,
		0xffffffff,
	};

	RenderDaedalusVtxStreams(GL_TRIANGLE_STRIP, positions, uvs, colours, 4);

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	++mNumRect;
#endif
}

void RendererGL::FillRect( const v2 & xy0, const v2 & xy1, u32 color )
{
	PrepareRenderState(mScreenToDevice.mRaw, gRDPOtherMode.depth_source ? false : true, false);

	v2 screen0;
	v2 screen1;
	ConvertN64ToScreen( xy0, screen0 );
	ConvertN64ToScreen( xy1, screen1 );

	DL_PF( "    Screen:  %.1f,%.1f -> %.1f,%.1f", screen0.x, screen0.y, screen1.x, screen1.y );

	const f32 depth = gRDPOtherMode.depth_source ? mPrimDepth : 0.0f;

	float positions[] = {
		screen0.x, screen0.y, depth,
		screen1.x, screen0.y, depth,
		screen0.x, screen1.y, depth,
		screen1.x, screen1.y, depth,
	};

	// NB - these aren't needed. Could just pass NULL to RenderDaedalusVtxStreams?
	float uvs[] = {
		0.f, 0.f,
		1.f, 0.f,
		0.f, 1.f,
		1.f, 1.f,
	};

	u32 colours[] = {
		color,
		color,
		color,
		color,
	};

	RenderDaedalusVtxStreams(GL_TRIANGLE_STRIP, positions, uvs, colours, 4);

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	++mNumRect;
#endif
}

void RendererGL::Draw2DTexture(f32 x0, f32 y0, f32 x1, f32 y1,
							   f32 u0, f32 v0, f32 u1, f32 v1,
							   const CNativeTexture * texture)
{
	DAEDALUS_PROFILE( "RendererGL::Draw2DTexture" );

	// FIXME(strmnnrmn): is this right? Gross anyway.
	gRDPOtherMode.cycle_type = CYCLE_COPY;

	PrepareRenderState(mScreenToDevice.mRaw, false /* disable_depth */, false);

	glEnable(GL_BLEND);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	float sx0 = N64ToScreenX(x0);
	float sy0 = N64ToScreenY(y0);

	float sx1 = N64ToScreenX(x1);
	float sy1 = N64ToScreenY(y1);

	const f32 depth = 0.0f;

	float positions[] = {
		sx0, sy0, depth,
		sx1, sy0, depth,
		sx0, sy1, depth,
		sx1, sy1, depth,
	};

	float uvs[] = {
		u0, v0,
		u1, v0,
		u0, v1,
		u1, v1,
	};

	u32 colours[] = {
		0xffffffff,
		0xffffffff,
		0xffffffff,
		0xffffffff,
	};

	RenderDaedalusVtxStreams(GL_TRIANGLE_STRIP, positions, uvs, colours, 4);
}

void RendererGL::Draw2DTextureR(f32 x0, f32 y0,
								f32 x1, f32 y1,
								f32 x2, f32 y2,
								f32 x3, f32 y3,
								f32 s, f32 t)	// With Rotation
{
	DAEDALUS_PROFILE( "RendererGL::Draw2DTextureR" );

	// FIXME(strmnnrmn): is this right? Gross anyway.
	gRDPOtherMode.cycle_type = CYCLE_COPY;

	PrepareRenderState(mScreenToDevice.mRaw, false /* disable_depth */, false);

	glEnable(GL_BLEND);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	const f32 depth = 0.0f;

	float positions[] = {
		N64ToScreenX(x0), N64ToScreenY(y0), depth,
		N64ToScreenX(x1), N64ToScreenY(y1), depth,
		N64ToScreenX(x2), N64ToScreenY(y2), depth,
		N64ToScreenX(x3), N64ToScreenY(y3), depth,
	};

	float uvs[] = {
		0.f, 0.f,
		s,   0.f,
		s,   t,
		0.f, t,
	};

	u32 colours[] = {
		0xffffffff,
		0xffffffff,
		0xffffffff,
		0xffffffff,
	};

	RenderDaedalusVtxStreams(GL_TRIANGLE_FAN, positions, uvs, colours, 4);
}

bool CreateRenderer()
{
	DAEDALUS_ASSERT_Q(gRenderer == NULL);
	gRendererGL = new RendererGL();
	gRenderer   = gRendererGL;
	return true;
}
void DestroyRenderer()
{
	delete gRendererGL;
	gRendererGL = NULL;
	gRenderer   = NULL;
}
