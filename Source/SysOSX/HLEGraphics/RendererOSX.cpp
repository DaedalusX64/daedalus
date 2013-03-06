#include "stdafx.h"
#include "RendererOSX.h"

#include <pspgu.h>

#include <vector>

#include <GL/glfw.h>

#include "Core/ROM.h"
#include "Graphics/ColourValue.h"
#include "Graphics/GraphicsContext.h"
#include "Graphics/NativeTexture.h"
#include "HLEGraphics/DLDebug.h"
#include "HLEGraphics/Texture.h"
#include "OSHLE/ultra_gbi.h"

BaseRenderer * gRenderer    = NULL;
RendererOSX *  gRendererOSX = NULL;

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

void sceGuTexOffset(float s, float t)
{
}

void sceGuTexScale(float s, float t)
{
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

	GLint				uloc_texscale[kNumTextures];
	GLint				uloc_texoffset[kNumTextures];
	GLint				uloc_texture[kNumTextures];
};
static std::vector<ShaderProgram *>		gShaders;


/* Creates a shader object of the specified type using the specified text
 */
static GLuint make_shader(GLenum type, const char* shader_src)
{
	printf("%d - %s\n", type, shader_src);

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

static void PrintMux(char (&body)[1024], u64 mux, u32 cycle_type, u32 alpha_threshold)
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
		strcpy(body, "col = shade;\n");
	}
	else if (cycle_type == CYCLE_COPY)
	{
		strcpy(body, "col = tex0;\n");
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
					  "\tcombined = vec4(col.rgb, col.a);\n"
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
"	v_uv0 = (in_uv - uTexOffset0) * uTexScale0;\n"
"	v_uv1 = (in_uv - uTexOffset1) * uTexScale1;\n"
"	v_col = in_col;\n"
"	gl_Position = uProject * vec4(in_pos, 1.0);\n"
"}\n";

static const char* default_fragment_shader_fmt =
"#version 150\n"
"uniform sampler2D uTexture0;\n"
"uniform sampler2D uTexture1;\n"
"uniform vec4 uPrimColour;\n"
"uniform vec4 uEnvColour;\n"
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
"	float prim_lod_frac = 0.0;\n"		// FIXME
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
	PrintMux(body, mux, cycle_type, alpha_threshold);

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

void RendererOSX::RestoreRenderStates()
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
}

// Strip out vertex stream into separate buffers.
// TODO(strmnnrmn): Renderer should support generating this data directly.
void RendererOSX::RenderDaedalusVtx(int prim, const DaedalusVtx * vertices, int count)
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

void RendererOSX::RenderDaedalusVtxStreams(int prim, const float * positions, const float * uvs, const u32 * colours, int count)
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
	//case 0x0050:					// In * AIn + Mem * 1-A || :In * AIn + Mem * 1-A:		SSV - TV Screen and SM64 text
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

void RendererOSX::PrepareRenderState(const float (&mat_project)[16], bool disable_zbuffer)
{
	static bool	ZFightingEnabled = false;

	DAEDALUS_PROFILE( "RendererOSX::PrepareRenderState" );

	if ( disable_zbuffer )
	{
		glDisable(GL_DEPTH_TEST);
		glDepthMask(GL_FALSE);
	}
	else
	{
		// Fixes Zfighting issues we have on the PSP.
		if( gRDPOtherMode.zmode == 3 )
		{
			if( !ZFightingEnabled )
			{
				ZFightingEnabled = true;
				//FIXME
				//glDepthRange(65535 / 65536.f, 80 / 65536.f);
			}
		}
		else if( ZFightingEnabled )
		{
			ZFightingEnabled = false;
			//FIXME
			//glDepthRange(65535 / 65536.f, 0 / 65536.f);
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
		return;
	}

	glUseProgram(program->program);

	glUniformMatrix4fv(program->uloc_project, 1, GL_FALSE, mat_project);

	glUniform4f(program->uloc_primcol, mPrimitiveColour.GetRf(), mPrimitiveColour.GetGf(), mPrimitiveColour.GetBf(), mPrimitiveColour.GetAf());
	glUniform4f(program->uloc_envcol,  mEnvColour.GetRf(),       mEnvColour.GetGf(),       mEnvColour.GetBf(),       mEnvColour.GetAf());

	// FIXME - figure out from mux
	bool install_textures[] = { true, false };

	for (u32 i = 0; i < kNumTextures; ++i)
	{
		if (!install_textures[i])
			continue;

		if (mpTexture[i] != NULL)
		{
			CRefPtr<CNativeTexture> texture = mpTexture[i]->GetTexture();
			if (texture != NULL)
			{
				texture->InstallTexture();

				glUniform1i(program->uloc_texture[i], i);

				if( (mTnL.Flags._u32 & (TNL_LIGHT|TNL_TEXGEN)) == (TNL_LIGHT|TNL_TEXGEN) )
				{
					glUniform2f(program->uloc_texoffset[i], 0.f, 0.f);
					glUniform2f(program->uloc_texscale[i], 1.f, 1.f);
				}
				else
				{
					glUniform2f(program->uloc_texoffset[i], mTileTopLeft[i].x, mTileTopLeft[i].y);
					glUniform2f(program->uloc_texscale[i], mTileScale[i].x, mTileScale[i].y);
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
}

// FIXME(strmnnrmn): for fill/copy modes this does more work than needed.
// It ends up copying colour/uv coords when not needed, and can use a shader uniform for the fill colour.
void RendererOSX::RenderTriangles( DaedalusVtx * p_vertices, u32 num_vertices, bool disable_zbuffer )
{
	PrepareRenderState(gProjection.m, disable_zbuffer);
	RenderDaedalusVtx(GL_TRIANGLES, p_vertices, num_vertices);
}

void RendererOSX::TexRect( u32 tile_idx, const v2 & xy0, const v2 & xy1, const v2 & uv0, const v2 & uv1 )
{
	EnableTexturing( tile_idx );
	PrepareRenderState(mScreenToDevice.mRaw, gRDPOtherMode.depth_source ? false : true);

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

void RendererOSX::TexRectFlip( u32 tile_idx, const v2 & xy0, const v2 & xy1, const v2 & uv0, const v2 & uv1 )
{
	EnableTexturing( tile_idx );
	PrepareRenderState(mScreenToDevice.mRaw, gRDPOtherMode.depth_source ? false : true);

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

void RendererOSX::FillRect( const v2 & xy0, const v2 & xy1, u32 color )
{
	PrepareRenderState(mScreenToDevice.mRaw, gRDPOtherMode.depth_source ? false : true);

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
		0.f, 1.f,
		1.f, 0.f,
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

void RendererOSX::Draw2DTexture(f32 frameX, f32 frameY, f32 frameW, f32 frameH, f32 imageX, f32 imageY, f32 imageW, f32 imageH)
{
	DAEDALUS_PROFILE( "RendererOSX::Draw2DTexture" );

	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	glShadeModel(GL_FLAT);
	//glTexFunc(GL_TFX_REPLACE, GL_TCC_RGBA);
	glEnable(GL_BLEND);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	TextureVtx verts[4];
	verts[0].pos.x = N64ToScreenX(frameX);	// Frame X Offset * X Scale Factor + Screen X Offset
	verts[0].pos.y = N64ToScreenY(frameY);	// Frame Y Offset * Y Scale Factor + Screen Y Offset
	verts[0].pos.z = 0.0f;
	verts[0].t0.x  = imageX;				// X coordinates
	verts[0].t0.y  = imageY;

	verts[1].pos.x = N64ToScreenX(frameW);	// Translated X Offset + (Image Width  * X Scale Factor)
	verts[1].pos.y = N64ToScreenY(frameY);	// Translated Y Offset + (Image Height * Y Scale Factor)
	verts[1].pos.z = 0.0f;
	verts[1].t0.x  = imageW;				// X dimentions
	verts[1].t0.y  = imageY;

	verts[2].pos.x = N64ToScreenX(frameX);	// Frame X Offset * X Scale Factor + Screen X Offset
	verts[2].pos.y = N64ToScreenY(frameH);	// Frame Y Offset * Y Scale Factor + Screen Y Offset
	verts[2].pos.z = 0.0f;
	verts[2].t0.x  = imageX;				// X coordinates
	verts[2].t0.y  = imageH;

	verts[3].pos.x = N64ToScreenX(frameW);	// Translated X Offset + (Image Width  * X Scale Factor)
	verts[3].pos.y = N64ToScreenY(frameH);	// Translated Y Offset + (Image Height * Y Scale Factor)
	verts[3].pos.z = 0.0f;
	verts[3].t0.x  = imageW;				// X dimentions
	verts[3].t0.y  = imageH;				// Y dimentions

	DAEDALUS_ERROR("Draw2DTexture FIXME");
	//sceGuDrawArray( GL_TRIANGLE_STRIP, GU_TEXTURE_32BITF | GU_VERTEX_32BITF | GU_TRANSFORM_2D, 4, 0, verts );
}

void RendererOSX::Draw2DTextureR(f32 x0, f32 y0, f32 x1, f32 y1, f32 x2, f32 y2, f32 x3, f32 y3, f32 s, f32 t)	// With Rotation
{
	DAEDALUS_PROFILE( "RendererOSX::Draw2DTextureR" );

	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	glShadeModel(GL_FLAT);
	//glTexFunc(GL_TFX_REPLACE, GL_TCC_RGBA);
	glEnable(GL_BLEND);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	TextureVtx verts[4];
	verts[0].pos.x = N64ToScreenX(x0);
	verts[0].pos.y = N64ToScreenY(y0);
	verts[0].pos.z = 0.0f;
	verts[0].t0.x  = 0.0f;
	verts[0].t0.y  = 0.0f;

	verts[1].pos.x = N64ToScreenX(x1);
	verts[1].pos.y = N64ToScreenY(y1);
	verts[1].pos.z = 0.0f;
	verts[1].t0.x  = s;
	verts[1].t0.y  = 0.0f;

	verts[2].pos.x = N64ToScreenX(x2);
	verts[2].pos.y = N64ToScreenY(y2);
	verts[2].pos.z = 0.0f;
	verts[2].t0.x  = s;
	verts[2].t0.y  = t;

	verts[3].pos.x = N64ToScreenX(x3);
	verts[3].pos.y = N64ToScreenY(y3);
	verts[3].pos.z = 0.0f;
	verts[3].t0.x  = 0.0f;
	verts[3].t0.y  = t;

	DAEDALUS_ERROR("Draw2DTextureR FIXME");
	//sceGuDrawArray( GL_TRIANGLE_FAN, GU_TEXTURE_32BITF | GU_VERTEX_32BITF | GU_TRANSFORM_2D, 4, 0, verts );
}

//*****************************************************************************
//
//	The following blitting code was taken from The TriEngine.
//	See http://www.assembla.com/code/openTRI for more information.
//
//*****************************************************************************
void RendererOSX::Draw2DTextureBlit(f32 x, f32 y, f32 width, f32 height, f32 u0, f32 v0, f32 u1, f32 v1, CNativeTexture * texture)
{
	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	glShadeModel(GL_FLAT);
	//glTexFunc(GL_TFX_REPLACE, GL_TCC_RGBA);
	glEnable(GL_BLEND);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

#ifdef DAEDALUS_OSX
	DAEDALUS_ERROR( "Draw2DTextureBlit is not implemented on OSX" );
#else

// 0 Simpler blit algorithm, but doesn't handle big textures as good? (see StarSoldier)
// 1 More complex algorithm. used in newer versions of TriEngine, fixes the main screen in StarSoldier
// Note : We ignore handling height > 512 textures for now
#if 1
	if ( u1 > 512.f )
	{
		s32 off = (u1>u0) ? ((int)u0 & ~31) : ((int)u1 & ~31);

		const u8* data = static_cast<const u8*>( texture->GetData()) + off * GetBitsPerPixel( texture->GetFormat() );
		u1 -= off;
		u0 -= off;
		sceGuTexImage( 0, Min<u32>(512,texture->GetCorrectedWidth()), Min<u32>(512,texture->GetCorrectedHeight()), texture->GetBlockWidth(), data );
	}

	f32 start, end;
	f32 cur_u = u0;
	f32 cur_x = x;
	f32 x_end = width;
	f32 slice = 64.f;
	f32 ustep = (u1-u0)/width * slice;

	// blit maximizing the use of the texture-cache
	for( start=0, end=width; start<end; start+=slice )
	{
		f32 poly_width = ((cur_x+slice) > x_end) ? (x_end-cur_x) : slice;
		f32 source_width = ((cur_u+ustep) > u1) ? (u1-cur_u) : ustep;

		TextureVtx verts[2];

		verts[0].t0.x = cur_u;
		verts[0].t0.y = v0;
		verts[0].pos.x = N64ToScreenX(cur_x);
		verts[0].pos.y = N64ToScreenY(y);
		verts[0].pos.z = 0;

		cur_u += source_width;
		cur_x += poly_width;

		verts[1].t0.x = cur_u;
		verts[1].t0.y = v1;
		verts[1].pos.x = N64ToScreenX(cur_x);
		verts[1].pos.y = N64ToScreenY(height);
		verts[1].pos.z = 0;

		sceGuDrawArray( GU_SPRITES, GU_TEXTURE_32BITF | GU_VERTEX_32BITF | GU_TRANSFORM_2D, 2, 0, verts );
	}
#else
	f32 cur_v = v0;
	f32 cur_y = y;
	f32 v_end = v1;
	f32 y_end = height;
	f32 vslice = 512.f;
	f32 ystep = (height/(v1-v0) * vslice);
	f32 vstep = ((v1-v0) > 0 ? vslice : -vslice);

	f32 x_end = width;
	f32 uslice = 64.f;
	//f32 ustep = (u1-u0)/width * xslice;
	f32 xstep = (width/(u1-u0) * uslice);
	f32 ustep = ((u1-u0) > 0 ? uslice : -uslice);

	const u8* data = static_cast<const u8*>(texture->GetData());

	for ( ; cur_y < y_end; cur_y+=ystep, cur_v+=vstep )
	{
		f32 cur_u = u0;
		f32 cur_x = x;
		f32 u_end = u1;

		f32 poly_height = ((cur_y+ystep) > y_end) ? (y_end-cur_y) : ystep;
		f32 source_height = vstep;

		// support negative vsteps
		if ((vstep > 0) && (cur_v+vstep > v_end))
		{
			source_height = (v_end-cur_v);
		}
		else if ((vstep < 0) && (cur_v+vstep < v_end))
		{
			source_height = (cur_v-v_end);
		}

		const u8* udata = data;
		// blit maximizing the use of the texture-cache
		for( ; cur_x < x_end; cur_x+=xstep, cur_u+=ustep )
		{
			// support large images (width > 512)
			if (cur_u>512.f || cur_u+ustep>512.f)
			{
				s32 off = (ustep>0) ? ((int)cur_u & ~31) : ((int)(cur_u+ustep) & ~31);

				udata += off * GetBitsPerPixel( texture->GetFormat() );
				cur_u -= off;
				u_end -= off;
				sceGuTexImage(0, Min<u32>(512,texture->GetCorrectedWidth()), Min<u32>(512,texture->GetCorrectedHeight()), texture->GetBlockWidth(), udata);
			}

			//f32 poly_width = ((cur_x+xstep) > x_end) ? (x_end-cur_x) : xstep;
			f32 poly_width = xstep;
			f32 source_width = ustep;

			// support negative usteps
			if ((ustep > 0) && (cur_u+ustep > u_end))
			{
				source_width = (u_end-cur_u);
			}
			else if ((ustep < 0) && (cur_u+ustep < u_end))
			{
				source_width = (cur_u-u_end);
			}

			TextureVtx verts[2];

			verts[0].t0.x = cur_u;
			verts[0].t0.y = cur_v;
			verts[0].pos.x = cur_x;
			verts[0].pos.y = cur_y;
			verts[0].pos.z = 0;

			verts[1].t0.x = cur_u + source_width;
			verts[1].t0.y = cur_v + source_height;
			verts[1].pos.x = cur_x + poly_width;
			verts[1].pos.y = cur_y + poly_height;
			verts[1].pos.z = 0;

			sceGuDrawArray( GU_SPRITES, GU_TEXTURE_32BITF | GU_VERTEX_32BITF | GU_TRANSFORM_2D, 2, 0, verts );
		}
	}
#endif

#endif // DAEDALUS_OSX
}

bool CreateRenderer()
{
	DAEDALUS_ASSERT_Q(gRenderer == NULL);
	gRendererOSX = new RendererOSX();
	gRenderer    = gRendererOSX;
	return true;
}
void DestroyRenderer()
{
	delete gRendererOSX;
	gRendererOSX = NULL;
	gRenderer    = NULL;
}
