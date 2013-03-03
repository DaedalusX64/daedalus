#include "stdafx.h"
#include "pspgu.h"

#include <vector>

#include <GL/glfw.h>

#include "Graphics/ColourValue.h"
#include "HLEGraphics/BaseRenderer.h"

/* OpenGL 3.0 */
typedef void (APIENTRY * PFN_glGenVertexArrays)(GLsizei n, GLuint *arrays);
typedef void (APIENTRY * PFN_glBindVertexArray)(GLuint array);
typedef void (APIENTRY * PFN_glDeleteVertexArrays)(GLsizei n, GLuint *arrays);

static PFN_glGenVertexArrays            pglGenVertexArrays = NULL;
static PFN_glBindVertexArray            pglBindVertexArray = NULL;
static PFN_glDeleteVertexArrays         pglDeleteVertexArrays = NULL;

// Global (duplicated) state. Eventually I expect we'll have an GLFWRenderer which
// derives from a BaseRenderer, and it'll be able to access this state directly.
static u64 gMux = 0;
static u32 gCycleType = 0;
static float gTexOffset[2]	= { 0.f, 0.f };
static float gTexScale[2]	= { 1.f, 1.f };
static float gPrimColour[4]	= { 1.f, 1.f, 1.f, 1.f };
static float gEnvColour[4] 	= { 1.f, 1.f, 1.f, 1.f };


#define RESOLVE_GL_FCN(type, var, name) \
    if (status == GL_TRUE) \
    {\
        var = (type)glfwGetProcAddress((name));\
        if ((var) == NULL)\
        {\
            status = GL_FALSE;\
        }\
    }


bool initgl()
{
    GLboolean status = GL_TRUE;
    RESOLVE_GL_FCN(PFN_glGenVertexArrays, pglGenVertexArrays, "glGenVertexArrays");
    RESOLVE_GL_FCN(PFN_glDeleteVertexArrays, pglDeleteVertexArrays, "glDeleteVertexArrays");
    RESOLVE_GL_FCN(PFN_glBindVertexArray, pglBindVertexArray, "glBindVertexArray");
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
	gTexOffset[0] = s;
	gTexOffset[1] = t;
}

void sceGuTexScale(float s, float t)
{
	gTexScale[0] = s;
	gTexScale[1] = t;
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


struct ShaderProgram
{
	u64					mMux;
	u32					mCycleType;
	GLuint 				program;

	GLint				uloc_project;
	GLint				uloc_texscale;
	GLint				uloc_texoffset;
	GLint				uloc_texture;
	GLint				uloc_primcol;
	GLint				uloc_envcol;
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

static float gPositionBuffer[kMaxVertices][3];
static float gTexCoordBuffer[kMaxVertices][2];
static u32 gColorBuffer[kMaxVertices];


void init_buffers(ShaderProgram * program)
{
	pglGenVertexArrays(1, &gVAO);
	pglBindVertexArray(gVAO);

	glGenBuffers(kNumBuffers, gVBOs);

	/* Prepare the attributes for rendering */
	GLuint attrloc;
	attrloc = glGetAttribLocation(program->program, "in_pos");
	glBindBuffer(GL_ARRAY_BUFFER, gVBOs[kPositionBuffer]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(gPositionBuffer), gPositionBuffer, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(attrloc);
	glVertexAttribPointer(attrloc, 3, GL_FLOAT, GL_FALSE, 0, 0);

	attrloc = glGetAttribLocation(program->program, "in_uv");
	glBindBuffer(GL_ARRAY_BUFFER, gVBOs[kTexCoordBuffer]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(gTexCoordBuffer), gTexCoordBuffer, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(attrloc);
	glVertexAttribPointer(attrloc, 2, GL_FLOAT, GL_FALSE, 0, 0);

	attrloc = glGetAttribLocation(program->program, "in_col");
	glBindBuffer(GL_ARRAY_BUFFER, gVBOs[kColorBuffer]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(gColorBuffer), gColorBuffer, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(attrloc);
	glVertexAttribPointer(attrloc, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, 0);
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
"uniform vec2 uTexScale;\n"
"uniform vec2 uTexOffset;\n"
"uniform mat4 uProject;\n"
"in  vec3 in_pos;\n"
"in  vec2 in_uv;\n"
"in  vec4 in_col;\n"
"out vec2 v_uv;\n"
"out vec4 v_col;\n"
"\n"
"void main()\n"
"{\n"
"	v_uv = (in_uv - uTexOffset) * uTexScale;\n"
"	v_col = in_col;\n"
"	gl_Position = uProject * vec4(in_pos, 1.0);\n"
"}\n";

static const char* default_fragment_shader_fmt =
"#version 150\n"
"uniform sampler2D uTexture;\n"
"uniform vec4 uPrimColour;\n"
"uniform vec4 uEnvColour;\n"
"out vec4 fragcol;\n"
"in  vec2 v_uv;\n"
"in  vec4 v_col;\n"
"void main()\n"
"{\n"
"	vec4 shade = v_col;\n"
"	vec4 prim  = uPrimColour;\n"
"	vec4 env   = uEnvColour;\n"
"	vec4 one   = vec4(1,1,1,1);\n"
"	vec4 zero  = vec4(0,0,0,0);\n"
"	vec4 tex0  = texture(uTexture, v_uv);\n"
"	vec4 tex1  = tex0;\n"  			// FIXME
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
	program->uloc_texoffset = glGetUniformLocation(shader_program, "uTexOffset");
	program->uloc_texscale  = glGetUniformLocation(shader_program, "uTexScale");
	program->uloc_texture   = glGetUniformLocation(shader_program, "uTexture");
	program->uloc_primcol   = glGetUniformLocation(shader_program, "uPrimColour");
	program->uloc_envcol    = glGetUniformLocation(shader_program, "uEnvColour");

	init_buffers(program);

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
	else
	{
	}
}

void GLRenderer_SetMux(u64 mux, u32 cycle_type, const c32 & prim_col, const c32 & env_col)
{
	gMux = mux;
	gCycleType = cycle_type;

	gPrimColour[0] = prim_col.GetRf();
	gPrimColour[1] = prim_col.GetGf();
	gPrimColour[2] = prim_col.GetBf();
	gPrimColour[3] = prim_col.GetAf();

	gEnvColour[0] = env_col.GetRf();
	gEnvColour[1] = env_col.GetGf();
	gEnvColour[2] = env_col.GetBf();
	gEnvColour[3] = env_col.GetAf();
}

void sceGuDrawArray(int prim, int vtype, int count, const void * indices, const void * vertices)
{
	int stride = 0;
	int tex_off = 0;
	int col_off = 0;
	int pos_off = 0;

	const u8 * pv = static_cast<const u8 *>(vertices);

	// Figure out strides and offsets
	if (vtype & GU_TEXTURE_32BITF)	{ tex_off = stride; stride += 8; }
	if (vtype & GU_COLOR_8888)		{ col_off = stride; stride += 4; }
	if (vtype & GU_VERTEX_32BITF)	{ pos_off = stride; stride += 12; }

	const ShaderProgram * program = GetShaderForCurrentMode(gMux, gCycleType);

	// Bind all the uniforms
	glUseProgram(program->program);

	glUniformMatrix4fv(program->uloc_project, 1, GL_FALSE, gProjection.m);
	glUniform2fv(program->uloc_texoffset, 1, gTexOffset);
	glUniform2fv(program->uloc_texscale, 1, gTexScale);
	glUniform1i(program->uloc_texture, 0);

	glUniform4fv(program->uloc_primcol, 1, gPrimColour);
	glUniform4fv(program->uloc_envcol, 1, gEnvColour);

	// Strip out vertex stream into separate buffers.
	// TODO(strmnnrmn): Renderer should support generating this data directly.

	if (vtype & GU_TEXTURE_32BITF)
	{
		for (int i = 0; i < count; ++i)
		{
			const float * tex = (const float*)(pv + stride*i + tex_off);

			gTexCoordBuffer[i][0] = tex[0];
			gTexCoordBuffer[i][1] = tex[1];
		}
		glBindBuffer(GL_ARRAY_BUFFER, gVBOs[kTexCoordBuffer]);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * 2 * count, gTexCoordBuffer);
	}

	if (vtype & GU_COLOR_8888)
	{
		for (int i = 0; i < count; ++i)
		{
			const u32 * p = (const u32*)(pv + stride*i + col_off);

			gColorBuffer[i] = p[0];
		}
		glBindBuffer(GL_ARRAY_BUFFER, gVBOs[kColorBuffer]);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(u32) * count, gColorBuffer);
	}

	if (vtype & GU_VERTEX_32BITF)
	{
		for (int i = 0; i < count; ++i)
		{
			const float * p = (const float*)(pv + stride*i + pos_off);

			gPositionBuffer[i][0] = p[0];
			gPositionBuffer[i][1] = p[1];
			gPositionBuffer[i][2] = p[2];
		}
		glBindBuffer(GL_ARRAY_BUFFER, gVBOs[kPositionBuffer]);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * 3 * count, gPositionBuffer);
	}

	switch (prim)
	{
	case GU_SPRITES:
	//	printf( "Draw SPRITES %d %d\n", count, vtype );
		break;
	case GU_TRIANGLES:
		glDrawArrays(GL_TRIANGLES, 0, count);
		break;
	case GU_TRIANGLE_STRIP:
		printf( "Draw TRIANGLE_STRIP %d %d\n", count, vtype );
		break;
	case GU_TRIANGLE_FAN:
		printf( "Draw TRIANGLE_FAN %d %d\n", count, vtype );
		break;
	}
}
