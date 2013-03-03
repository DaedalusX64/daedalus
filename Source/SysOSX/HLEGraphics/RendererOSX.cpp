#include "stdafx.h"
#include "RendererOSX.h"

#include <pspgu.h>

#include <vector>

#include <GL/glfw.h>

#include "Core/ROM.h"
#include "Graphics/ColourValue.h"
#include "Graphics/NativeTexture.h"
#include "HLEGraphics/Texture.h"
#include "OSHLE/ultra_gbi.h"

BaseRenderer * gRenderer    = NULL;
RendererOSX *  gRendererOSX = NULL;

extern void InitBlenderMode( u32 blender );



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

const int kMaxVertices = 100;

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
	//DAEDALUS_ERROR( "%s: Not implemented", __FUNCTION__ );
}

void sceGuShadeModel(EGuShadeMode mode)
{
	glShadeModel(mode);
}

void sceGuDepthMask(int enable)
{
	// NB: psp seems to flip the sense of this!
	glDepthMask(enable ? GL_FALSE : GL_TRUE);
}

void sceGuDepthFunc(EGuCompareOp op)
{
	//glDepthFunc(op);

	// FIXME: psp build has reversed depth.
	if (op == GL_GEQUAL)
		glDepthFunc(GL_LEQUAL);

}

void sceGuDepthRange(int a, int b)
{
	// NB: ignore this - psp build has flipped z buffer (0 is near)
	//glDepthRange(a / 65536.f, b / 65536.f);
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

void sceGuTexWrap(int u, int v)
{
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, u);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, v);
}

void sceGuTexOffset(float s, float t)
{
}

void sceGuTexScale(float s, float t)
{
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

void sceGuTexFunc(int fn, int b)
{
	//DAEDALUS_ERROR( "%s: Not implemented", __FUNCTION__ );
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


struct ShaderProgram
{
	u64					mMux;
	u32					mCycleType;
	GLuint 				program;

	GLint				uloc_project;
	GLint				uloc_primcol;
	GLint				uloc_envcol;

	GLint				uloc_texscale[kNumTextures];
	GLint				uloc_texoffset[kNumTextures];
	GLint				uloc_texture[kNumTextures];
};


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


std::vector<ShaderProgram *>		gShaders;


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

static void PrintMux( char (&body)[1024], u64 mux, u32 cycle_type )
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
"out     vec4 fragcol;\n"
"in      vec2 v_uv0;\n"
"in      vec2 v_uv1;\n"
"in      vec4 v_col;\n"
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


static ShaderProgram * GetShaderForCurrentMode(u64 mux, u32 cycle_type)
{
	for (u32 i = 0; i < gShaders.size(); ++i)
	{
		ShaderProgram * program = gShaders[i];
		if (program->mMux == mux && program->mCycleType == cycle_type)
			return program;
	}

	char body[1024];
	PrintMux(body, mux, cycle_type);

	char frag_shader[2048];
	sprintf(frag_shader, default_fragment_shader_fmt, body);

	GLuint shader_program = make_shader_program(default_vertex_shader, frag_shader);
	if (shader_program == 0)
	{
		fprintf(stderr, "ERROR: during creation of the shader program\n");
		return NULL;
	}

	ShaderProgram * program = new ShaderProgram;
	program->mMux           = mux;
	program->mCycleType     = cycle_type;
	program->program        = shader_program;
	program->uloc_project   = glGetUniformLocation(shader_program, "uProject");
	program->uloc_primcol   = glGetUniformLocation(shader_program, "uPrimColour");
	program->uloc_envcol    = glGetUniformLocation(shader_program, "uEnvColour");

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


	gShaders.push_back(program);

	return program;
}


ScePspFMatrix4		gProjection;
void sceGuSetMatrix(EGuMatrixType type, const ScePspFMatrix4 * mtx)
{
	if (type == GL_PROJECTION)
	{
		memcpy(&gProjection, mtx, sizeof(gProjection));
	}
}

void sceGuDrawArray(int prim, int vtype, int count, const void * indices, const void * vertices)
{
	DAEDALUS_ERROR("Unhandled render");
	exit(1);
}


void RendererOSX::RenderDaedalusVtx(int prim, const DaedalusVtx * vertices, int count)
{
	// Strip out vertex stream into separate buffers.
	// TODO(strmnnrmn): Renderer should support generating this data directly.

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

	glBindBuffer(GL_ARRAY_BUFFER, gVBOs[kPositionBuffer]);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * 3 * count, gPositionBuffer);

	glBindBuffer(GL_ARRAY_BUFFER, gVBOs[kTexCoordBuffer]);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * 2 * count, gTexCoordBuffer);

	glBindBuffer(GL_ARRAY_BUFFER, gVBOs[kColorBuffer]);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(u32) * count, gColorBuffer);

	glDrawArrays(prim, 0, count);
}



void RendererOSX::RenderUsingCurrentBlendMode( DaedalusVtx * p_vertices, u32 num_vertices, u32 triangle_mode, u32 render_mode, bool disable_zbuffer )
{
	static bool	ZFightingEnabled = false;

	DAEDALUS_PROFILE( "RendererOSX::RenderUsingCurrentBlendMode" );

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

	const ShaderProgram * program = GetShaderForCurrentMode(mMux, gRDPOtherMode.cycle_type);

	// Bind all the uniforms
	glUseProgram(program->program);


	if ((render_mode & GU_TRANSFORM_2D) == GU_TRANSFORM_2D)
	{
		glUniformMatrix4fv(program->uloc_project, 1, GL_FALSE, gMatrixIdentity.mRaw);
	}
	else
	{
		glUniformMatrix4fv(program->uloc_project, 1, GL_FALSE, gProjection.m);
	}

	glUniform4f(program->uloc_primcol, mPrimitiveColour.GetRf(), mPrimitiveColour.GetGf(), mPrimitiveColour.GetBf(), mPrimitiveColour.GetAf());
	glUniform4f(program->uloc_envcol,  mEnvColour.GetRf(),       mEnvColour.GetGf(),       mEnvColour.GetBf(),       mEnvColour.GetAf());

	// Initiate Alpha test
	if( (gRDPOtherMode.alpha_compare == G_AC_THRESHOLD) && !gRDPOtherMode.alpha_cvg_sel )
	{
		// G_AC_THRESHOLD || G_AC_DITHER
		glAlphaFunc( (mAlphaThreshold | g_ROM.ALPHA_HACK) ? GL_GEQUAL : GL_GREATER, (float)mAlphaThreshold / 255.f);
		glEnable(GL_ALPHA_TEST);
	}
	else if (gRDPOtherMode.cvg_x_alpha)
	{
		// Going over 0x70 brakes OOT, but going lesser than that makes lines on games visible...ex: Paper Mario.
		// ALso going over 0x30 breaks the birds in Tarzan :(. Need to find a better way to leverage this.
		glAlphaFunc(GL_GREATER, (float)0x70 / 255.f);
		glEnable(GL_ALPHA_TEST);
	}
	else
	{
		// Use CVG for pixel alpha
        glDisable(GL_ALPHA_TEST);
	}

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
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GU_NEAREST);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GU_NEAREST);
				}

				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, mTexWrap[i].u);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, mTexWrap[i].v);
			}
		}
	}

	if (triangle_mode != 0)
	{
		RenderDaedalusVtx( triangle_mode, p_vertices, num_vertices );
	}
}

void RendererOSX::Draw2DTexture(f32 frameX, f32 frameY, f32 frameW, f32 frameH, f32 imageX, f32 imageY, f32 imageW, f32 imageH)
{
	DAEDALUS_PROFILE( "RendererOSX::Draw2DTexture" );

	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	glShadeModel(GL_FLAT);
	glDisable(GL_ALPHA_TEST);
	//glTexFunc(GL_TFX_REPLACE, GL_TCC_RGBA);
	glEnable(GL_BLEND);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	TextureVtx verts[4];
	verts[0].pos.x = N64ToNativeX(frameX);	// Frame X Offset * X Scale Factor + Screen X Offset
	verts[0].pos.y = N64ToNativeY(frameY);	// Frame Y Offset * Y Scale Factor + Screen Y Offset
	verts[0].pos.z = 0.0f;
	verts[0].t0.x  = imageX;				// X coordinates
	verts[0].t0.y  = imageY;

	verts[1].pos.x = N64ToNativeX(frameW);	// Translated X Offset + (Image Width  * X Scale Factor)
	verts[1].pos.y = N64ToNativeY(frameY);	// Translated Y Offset + (Image Height * Y Scale Factor)
	verts[1].pos.z = 0.0f;
	verts[1].t0.x  = imageW;				// X dimentions
	verts[1].t0.y  = imageY;

	verts[2].pos.x = N64ToNativeX(frameX);	// Frame X Offset * X Scale Factor + Screen X Offset
	verts[2].pos.y = N64ToNativeY(frameH);	// Frame Y Offset * Y Scale Factor + Screen Y Offset
	verts[2].pos.z = 0.0f;
	verts[2].t0.x  = imageX;				// X coordinates
	verts[2].t0.y  = imageH;

	verts[3].pos.x = N64ToNativeX(frameW);	// Translated X Offset + (Image Width  * X Scale Factor)
	verts[3].pos.y = N64ToNativeY(frameH);	// Translated Y Offset + (Image Height * Y Scale Factor)
	verts[3].pos.z = 0.0f;
	verts[3].t0.x  = imageW;				// X dimentions
	verts[3].t0.y  = imageH;				// Y dimentions

	sceGuDrawArray( GU_TRIANGLE_STRIP, GU_TEXTURE_32BITF | GU_VERTEX_32BITF | GU_TRANSFORM_2D, 4, 0, verts );
}

void RendererOSX::Draw2DTextureR(f32 x0, f32 y0, f32 x1, f32 y1, f32 x2, f32 y2, f32 x3, f32 y3, f32 s, f32 t)	// With Rotation
{
	DAEDALUS_PROFILE( "RendererOSX::Draw2DTextureR" );

	glDisable(GU_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	glShadeModel(GU_FLAT);
	glDisable(GU_ALPHA_TEST);
	//glTexFunc(GU_TFX_REPLACE, GU_TCC_RGBA);
	glEnable(GU_BLEND);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	TextureVtx verts[4];
	verts[0].pos.x = N64ToNativeX(x0);
	verts[0].pos.y = N64ToNativeY(y0);
	verts[0].pos.z = 0.0f;
	verts[0].t0.x  = 0.0f;
	verts[0].t0.y  = 0.0f;

	verts[1].pos.x = N64ToNativeX(x1);
	verts[1].pos.y = N64ToNativeY(y1);
	verts[1].pos.z = 0.0f;
	verts[1].t0.x  = s;
	verts[1].t0.y  = 0.0f;

	verts[2].pos.x = N64ToNativeX(x2);
	verts[2].pos.y = N64ToNativeY(y2);
	verts[2].pos.z = 0.0f;
	verts[2].t0.x  = s;
	verts[2].t0.y  = t;

	verts[3].pos.x = N64ToNativeX(x3);
	verts[3].pos.y = N64ToNativeY(y3);
	verts[3].pos.z = 0.0f;
	verts[3].t0.x  = 0.0f;
	verts[3].t0.y  = t;

	sceGuDrawArray( GU_TRIANGLE_FAN, GU_TEXTURE_32BITF | GU_VERTEX_32BITF | GU_TRANSFORM_2D, 4, 0, verts );
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
	glDisable(GL_ALPHA_TEST);
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
		verts[0].pos.x = N64ToNativeX(cur_x);
		verts[0].pos.y = N64ToNativeY(y);
		verts[0].pos.z = 0;

		cur_u += source_width;
		cur_x += poly_width;

		verts[1].t0.x = cur_u;
		verts[1].t0.y = v1;
		verts[1].pos.x = N64ToNativeX(cur_x);
		verts[1].pos.y = N64ToNativeY(height);
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
