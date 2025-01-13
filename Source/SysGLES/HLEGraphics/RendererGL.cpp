#include "Base/Types.h"

#include <vector>
#include <fstream>
#include <iostream>
#include <filesystem>    

#include <GLES3/gl3.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>  
#include <SDL2/SDL_syswm.h>

#include "Core/ROM.h"
#include "Debug/DBGConsole.h"
#include "Graphics/ColourValue.h"
#include "Graphics/GraphicsContext.h"
#include "Graphics/NativeTexture.h"
#include "HLEGraphics/DLDebug.h"
#include "HLEGraphics/RDPStateManager.h"
#include "Ultra/ultra_gbi.h"
#include "SysGLES/HLEGraphics/RendererGL.h"

#include "Base/Macros.h"
#include "Utility/Paths.h"
#include "Utility/Profiler.h"

BaseRenderer* gRenderer   = nullptr;
RendererGL*   gRendererGL = nullptr;

static bool gAccurateUVPipe = true;

std::string gN64FragmentLibrary;

static const u32 kNumTextures = 2;

const float kShiftScales[] =
{
    1.f / (1 << 0),
    1.f / (1 << 1),
    1.f / (1 << 2),
    1.f / (1 << 3),
    1.f / (1 << 4),
    1.f / (1 << 5),
    1.f / (1 << 6),
    1.f / (1 << 7),
    1.f / (1 << 8),
    1.f / (1 << 9),
    1.f / (1 << 10),
    float(1 << 5),
    float(1 << 4),
    float(1 << 3),
    float(1 << 2),
    float(1 << 1),
};
DAEDALUS_STATIC_ASSERT(std::size(kShiftScales) == 16);

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

static float    gPositionBuffer[kMaxVertices][3];
static TexCoord gTexCoordBuffer[kMaxVertices];
static u32      gColorBuffer[kMaxVertices];

extern bool initgl(); 

bool loadShader(const std::filesystem::path& shader_path)
{
    std::ifstream shader_file(shader_path);
    if (!shader_file.is_open())
    {
        std::cerr << "ERROR: Could not load shader source: " << shader_path << std::endl;
        return false;
    }

    shader_file.seekg(0, std::ios::end);
    size_t length = shader_file.tellg();
    shader_file.seekg(0, std::ios::beg);

    std::string shader_code(length, '\0');
    shader_file.read(&shader_code[0], length);

    // Trim trailing newlines
    while (!shader_code.empty() && shader_code.back() == '\n')
    {
        shader_code.pop_back();
    }

    gN64FragmentLibrary = shader_code;
    return true;
}

bool initgl()
{
    std::filesystem::path p = setBasePath("n64.psh");
    if (!loadShader(p))
    {
        std::cerr << "Failed to load N64 fragment library.\n";
        return false;
    }

    // Toggle mirror emulation
    gRDPStateManager.SetEmulateMirror(!gAccurateUVPipe);

    // Generate VAO
    glGenVertexArrays(1, &gVAO);
    glBindVertexArray(gVAO);

    // Generate VBOs
    glGenBuffers(kNumBuffers, gVBOs);

    // Position buffer
    glBindBuffer(GL_ARRAY_BUFFER, gVBOs[kPositionBuffer]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(gPositionBuffer), gPositionBuffer, GL_DYNAMIC_DRAW);

    // Texcoord buffer
    glBindBuffer(GL_ARRAY_BUFFER, gVBOs[kTexCoordBuffer]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(gTexCoordBuffer), gTexCoordBuffer, GL_DYNAMIC_DRAW);

    // Color buffer
    glBindBuffer(GL_ARRAY_BUFFER, gVBOs[kColorBuffer]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(gColorBuffer), gColorBuffer, GL_DYNAMIC_DRAW);

    return true;
}

void sceGuFog(f32 mn [[maybe_unused]], f32 mx [[maybe_unused]], u32 col [[maybe_unused]])
{
    // DAEDALUS_ERROR( "%s: Not implemented", __FUNCTION__ );
}

ScePspFMatrix4 gProjection;
void sceGuSetMatrix(EGuMatrixType type, const ScePspFMatrix4 * mtx)
{
    if (type == GL_PROJECTION)
    {
        memcpy(&gProjection, mtx, sizeof(gProjection));
    }
}

struct ShaderConfiguration
{
    u64 Mux;
    u32 CycleType : 2;
    u32 BilerpFilter : 1;
    u32 ClampS0 : 1;
    u32 ClampT0 : 1;
    u32 ClampS1 : 1;
    u32 ClampT1 : 1;
    u8  AlphaThreshold;
};

inline bool operator==(const ShaderConfiguration & a, const ShaderConfiguration & b)
{
    return
        a.Mux            == b.Mux &&
        a.CycleType      == b.CycleType &&
        a.BilerpFilter   == b.BilerpFilter &&
        a.ClampS0        == b.ClampS0 &&
        a.ClampT0        == b.ClampT0 &&
        a.ClampS1        == b.ClampS1 &&
        a.ClampT1        == b.ClampT1 &&
        a.AlphaThreshold == b.AlphaThreshold;
}

struct ShaderProgram
{
    ShaderConfiguration config;
    GLuint              program;

    GLint uloc_project;
    GLint uloc_primcol;
    GLint uloc_envcol;
    GLint uloc_primlodfrac;

    GLint uloc_tileclamp[kNumTextures];
    GLint uloc_tiletl[kNumTextures];
    GLint uloc_tilebr[kNumTextures];
    GLint uloc_tileshift[kNumTextures];
    GLint uloc_tilemask[kNumTextures];
    GLint uloc_tilemirror[kNumTextures];

    GLint uloc_texscale[kNumTextures];
    GLint uloc_texture[kNumTextures];

    GLint uloc_foo;
};

static std::vector<ShaderProgram*> gShaders;

static GLuint make_shader(GLenum type, const char** lines, size_t num_lines)
{
    GLuint shader = glCreateShader(type);
    if (shader != 0)
    {
        glShaderSource(shader, num_lines, lines, NULL);
        glCompileShader(shader);

        GLint shader_ok = GL_FALSE;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &shader_ok);
        if (shader_ok != GL_TRUE)
        {
            GLsizei log_length;
            char info_log[8192];
            fprintf(stderr, "ERROR: Failed to compile %s shader\n",
                    (type == GL_FRAGMENT_SHADER) ? "fragment" : "vertex" );
            glGetShaderInfoLog(shader, 8192, &log_length, info_log);
            fprintf(stderr, "ERROR: \n%s\n\n", info_log);
            glDeleteShader(shader);
            shader = 0;
        }
    }
    return shader;
}

static GLuint make_shader_program(const char** vertex_lines, size_t num_vertex_lines,
                                  const char** fragment_lines, size_t num_fragment_lines)
{
    GLuint program = 0u;
    GLint program_ok = GL_FALSE;

    // Create vertex shader
    GLuint vertex_shader = make_shader(GL_VERTEX_SHADER, vertex_lines, num_vertex_lines);
    if (!vertex_shader)
    {
        fprintf(stderr, "ERROR: Unable to load vertex shader\n");
        return 0u;
    }

    // Create fragment shader
    GLuint fragment_shader = make_shader(GL_FRAGMENT_SHADER, fragment_lines, num_fragment_lines);
    if (!fragment_shader)
    {
        fprintf(stderr, "ERROR: Unable to load fragment shader\n");
        glDeleteShader(vertex_shader);
        return 0u;
    }

    // Link them
    program = glCreateProgram();
    if (program != 0u)
    {
        glAttachShader(program, vertex_shader);
        glAttachShader(program, fragment_shader);

        glLinkProgram(program);
        glGetProgramiv(program, GL_LINK_STATUS, &program_ok);
        if (program_ok != GL_TRUE)
        {
            GLsizei log_length;
            char info_log[8192];
            fprintf(stderr, "ERROR: failed to link shader program\n");
            glGetProgramInfoLog(program, 8192, &log_length, info_log);
            fprintf(stderr, "ERROR: \n%s\n\n", info_log);
            glDeleteProgram(program);
            program = 0u;
        }
    }
    else
    {
        fprintf(stderr, "ERROR: Unable to create program object\n");
    }

    // Cleanup
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    return program;
}

static const char* default_vertex_shader =
    "#version 310 es\n"
    "precision highp float;\n"
    "precision highp int;\n"
    "uniform mat4 uProject;\n"
    "in      vec3 in_pos;\n"
    "in      vec2 in_uv;\n"
    "in      vec4 in_col;\n"
    "out     vec2 v_st;\n"
    "out     vec4 v_col;\n"
    "void main()\n"
    "{\n"
    "    v_st = in_uv;\n"
    "    v_col = in_col;\n"
    "    gl_Position = uProject * vec4(in_pos, 1.0);\n"
    "}\n";

static const char* default_fragment_shader_fmt =
    "#version 310 es\n"
    "precision highp float;\n"
    "precision highp int;\n"
    "out vec4 fragcol;\n"
    "in  vec4 v_col;\n"
    "void main()\n"
    "{\n"
    "    // Minimal example: just pass the vertex color\n"
    "    fragcol = v_col;\n"
    "}\n";


static void InitShaderProgram(ShaderProgram* program, const ShaderConfiguration & config, GLuint shader_program)
{
    program->config    = config;
    program->program   = shader_program;

    program->uloc_project     = glGetUniformLocation(shader_program, "uProject");
    program->uloc_primcol     = glGetUniformLocation(shader_program, "uPrimColour");
    program->uloc_envcol      = glGetUniformLocation(shader_program, "uEnvColour");
    program->uloc_primlodfrac = glGetUniformLocation(shader_program, "uPrimLODFrac");
    program->uloc_foo         = glGetUniformLocation(shader_program, "uFoo");

    // For both textures (0 & 1):
    for (u32 i = 0; i < kNumTextures; i++)
    {
        char uniform_name[64];
        program->uloc_tileclamp[i] = glGetUniformLocation(shader_program, uniform_name);

        program->uloc_tiletl[i] = glGetUniformLocation(shader_program, uniform_name);

        program->uloc_tilebr[i] = glGetUniformLocation(shader_program, uniform_name);

        program->uloc_tileshift[i] = glGetUniformLocation(shader_program, uniform_name);

        program->uloc_tilemask[i] = glGetUniformLocation(shader_program, uniform_name);

        program->uloc_tilemirror[i] = glGetUniformLocation(shader_program, uniform_name);

        program->uloc_texscale[i] = glGetUniformLocation(shader_program, uniform_name);

        program->uloc_texture[i] = glGetUniformLocation(shader_program, uniform_name);
    }

    // Setup vertex attributes
    GLuint attrloc;
    // Position
    attrloc = glGetAttribLocation(program->program, "in_pos");
    glBindBuffer(GL_ARRAY_BUFFER, gVBOs[kPositionBuffer]);
    glEnableVertexAttribArray(attrloc);
    glVertexAttribPointer(attrloc, 3, GL_FLOAT, GL_FALSE, 0, 0);

    // UV
    attrloc = glGetAttribLocation(program->program, "in_uv");
    glBindBuffer(GL_ARRAY_BUFFER, gVBOs[kTexCoordBuffer]);
    glEnableVertexAttribArray(attrloc);
    glVertexAttribPointer(attrloc, 2, GL_SHORT, GL_FALSE, 0, 0);

    // Color
    attrloc = glGetAttribLocation(program->program, "in_col");
    glBindBuffer(GL_ARRAY_BUFFER, gVBOs[kColorBuffer]);
    glEnableVertexAttribArray(attrloc);
    glVertexAttribPointer(attrloc, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, 0);
}

void RendererGL::MakeShaderConfigFromCurrentState(ShaderConfiguration * config) const
{
	config->Mux = mMux;
	config->CycleType = gRDPOtherMode.cycle_type;
	config->AlphaThreshold = 0;
	config->BilerpFilter = true;
	config->ClampS0 = false;
	config->ClampT0 = false;
	config->ClampS1 = false;
	config->ClampT1 = false;

	// Initiate Alpha test
	if( (gRDPOtherMode.alpha_compare == G_AC_THRESHOLD) && !gRDPOtherMode.alpha_cvg_sel )
	{
		// G_AC_THRESHOLD || G_AC_DITHER
		// FIXME(strmnnrmn): alpha func: (mAlphaThreshold | g_ROM.ALPHA_HACK) ? GL_GEQUAL : GL_GREATER
		config->AlphaThreshold = mBlendColour.GetA();
	}
	else if (gRDPOtherMode.cvg_x_alpha)
	{
		// Going over 0x70 brakes OOT, but going lesser than that makes lines on games visible...ex: Paper Mario.
		// ALso going over 0x30 breaks the birds in Tarzan :(. Need to find a better way to leverage this.
		config->AlphaThreshold = 0x70;
	}
	else
	{
		// Use CVG for pixel alpha
		config->AlphaThreshold = 0;
	}

	// In fill/cycle modes, we ignore the mux. Set it to zero so we don't create unecessary shaders.
	u32 cycle_type = config->CycleType;
	if (cycle_type == CYCLE_FILL || cycle_type == CYCLE_COPY)
		config->Mux = 0;

	// Not sure about this. Should CYCLE_FILL have alpha kill?
	if (cycle_type == CYCLE_FILL)
		config->AlphaThreshold = 0;

	config->BilerpFilter = (gRDPOtherMode.text_filt != G_TF_POINT) || (gGlobalPreferences.ForceLinearFilter);

	// If running the bilinear filter, check if we need to clamp in S or T.
	// Really, this is checking to see how we set mTexWrap in PrepareTexRectUVs.
	// Fixes California Speed, Mario Kart backgrounds.
	// (NB: better fix for California Speed is just to force a point filter...)
	if (config->BilerpFilter)
	{
		config->ClampS0 = mTexWrap[0].u == GU_CLAMP;
		config->ClampT0 = mTexWrap[0].v == GU_CLAMP;

		config->ClampS1 = mTexWrap[1].u == GU_CLAMP;
		config->ClampT1 = mTexWrap[1].v == GU_CLAMP;
	}
}

static ShaderProgram* GetShaderForConfig(const ShaderConfiguration & config)
{
    // Try existing:
    for (auto* existing : gShaders)
    {
        if (existing->config == config)
            return existing;
    }

    const char* vertex_lines[] = { default_vertex_shader };
    const char* fragment_lines[] = { default_fragment_shader_fmt };

    GLuint shader_program = make_shader_program(
        vertex_lines,   1,
        fragment_lines, 1
    );
    if (shader_program == 0)
    {
        fprintf(stderr, "ERROR: during creation of the shader program\n");
        return nullptr;
    }

    // 2) Create the program struct
    ShaderProgram* program = new ShaderProgram;
    InitShaderProgram(program, config, shader_program);
    gShaders.push_back(program);

    return program;
}

// -----------------------------------------------------------------------------
// RendererGL - the class from your snippet
// -----------------------------------------------------------------------------
RendererGL::RendererGL()  {}
RendererGL::~RendererGL() {}

// -----------------------------------------------------------------------------
// RestoreRenderStates() - remove desktop-only calls like glShadeModel, glDisable(GL_FOG)
// -----------------------------------------------------------------------------
void RendererGL::RestoreRenderStates()
{
    // In ES 3.2, no GL_FOG, no fixed-function lighting, no glShadeModel:
    // glDisable(GL_FOG);
    // glDisable(GL_LIGHTING);
    // glShadeModel(GL_SMOOTH);

    // We do our own culling
    glDisable(GL_CULL_FACE);

    // Scissor
    u32 width, height;
    CGraphicsContext::Get()->GetScreenSize(&width, &height);
    glScissor(0, 0, width, height);
    glEnable(GL_SCISSOR_TEST);

    // Blend
    glBlendColor(0.f, 0.f, 0.f, 0.f);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_BLEND);

    // Depth
    glDepthMask(GL_FALSE);
    glDepthFunc(GL_LEQUAL);
    glDisable(GL_DEPTH_TEST);

    // Polygon offset
    glEnable(GL_POLYGON_OFFSET_FILL);
}

// -----------------------------------------------------------------------------
// RenderDaedalusVtx - fill CPU buffers and upload
// -----------------------------------------------------------------------------
void RendererGL::RenderDaedalusVtx(int prim, const DaedalusVtx* vertices, int count)
{
    DAEDALUS_ASSERT(count <= kMaxVertices, "Too many vertices!");

    if (count > kMaxVertices) count = kMaxVertices;

    // Hack to fix the sun in Zelda OOT/MM
	const f32 scale = ( g_ROM.ZELDA_HACK &&(gRDPOtherMode.L == 0x0c184241) ) ? 16.f : 32.f;

    for (int i = 0; i < count; ++i)
    {
        gPositionBuffer[i][0] = vertices[i].Position.x;
        gPositionBuffer[i][1] = vertices[i].Position.y;
        gPositionBuffer[i][2] = vertices[i].Position.z;

        gTexCoordBuffer[i].s = (int)(vertices[i].Texture.x * 32.f);
        gTexCoordBuffer[i].t = (int)(vertices[i].Texture.y * 32.f);

        gColorBuffer[i] = vertices[i].Colour.GetColour();
    }

    // Upload to VBO
    glBindBuffer(GL_ARRAY_BUFFER, gVBOs[kPositionBuffer]);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float)*3*count, gPositionBuffer);

    glBindBuffer(GL_ARRAY_BUFFER, gVBOs[kTexCoordBuffer]);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(TexCoord)*count, gTexCoordBuffer);

    glBindBuffer(GL_ARRAY_BUFFER, gVBOs[kColorBuffer]);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(u32)*count, gColorBuffer);

    // Draw
    glDrawArrays(prim, 0, count);
}

void RendererGL::RenderDaedalusVtxStreams(int prim, const float * positions, const TexCoord * uvs, const u32 * colours, int count)
{
	glBindBuffer(GL_ARRAY_BUFFER, gVBOs[kPositionBuffer]);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * 3 * count, positions);

	glBindBuffer(GL_ARRAY_BUFFER, gVBOs[kTexCoordBuffer]);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(TexCoord) * count, uvs);

	glBindBuffer(GL_ARRAY_BUFFER, gVBOs[kColorBuffer]);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(u32) * count, colours);

	glDrawArrays(prim, 0, count);
}

extern u32 gRDPFrame; 

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
static const char * const kBlendCl[] = { "In",  "Mem",  "Bl",     "Fog" };
static const char * const kBlendA1[] = { "AIn", "AFog", "AShade", "0" };
static const char * const kBlendA2[] = { "1-A", "AMem", "1",      "?" };

static inline void DebugBlender(u32 cycle_type, u32 blender, u32 alpha_cvg_sel, u32 cvg_x_alpha)
{
	static u32 last_blender = 0;

	if(last_blender != blender)
	{
		printf( "********************************\n\n" );
		printf( "Unknown Blender. alpha_cvg_sel: %d cvg_x_alpha: %d\n",
				alpha_cvg_sel, cvg_x_alpha );
		printf( "0x%04x: // %s * %s + %s * %s",
				blender,
				kBlendCl[(blender>>14) & 0x3],
				kBlendA1[(blender>>10) & 0x3],
				kBlendCl[(blender>> 6) & 0x3],
				kBlendA2[(blender>> 2) & 0x3]);

		if (cycle_type == CYCLE_2CYCLE)
		{
			printf( " | %s * %s + %s * %s",
				kBlendCl[(blender>>12) & 0x3],
				kBlendA1[(blender>> 8) & 0x3],
				kBlendCl[(blender>> 4) & 0x3],
				kBlendA2[(blender    ) & 0x3]);
		}
		printf( "\n********************************\n\n" );
		last_blender = blender;
	}
}
#endif

static void InitBlenderMode()
{
	u32 cycle_type    = gRDPOtherMode.cycle_type;
	u32 cvg_x_alpha   = gRDPOtherMode.cvg_x_alpha;
	u32 alpha_cvg_sel = gRDPOtherMode.alpha_cvg_sel;
	u32 blendmode     = gRDPOtherMode.blender;

	// NB: If we're running in 1cycle mode, ignore the 2nd cycle.
	u32 active_mode = (cycle_type == CYCLE_2CYCLE) ? blendmode : (blendmode & 0xcccc);

	enum BlendType
	{
		kBlendModeOpaque,
		kBlendModeAlphaTrans,
		kBlendModeFade,
	};
	BlendType type = kBlendModeOpaque;

	// FIXME(strmnnrmn): lots of these need fog!

	switch (active_mode)
	{
	case 0x0040: // In * AIn + Mem * 1-A
		// MarioKart (spinning logo).
		type = kBlendModeAlphaTrans;
		break;
	case 0x0050: // In * AIn + Mem * 1-A | In * AIn + Mem * 1-A
		// Extreme-G.
		type = kBlendModeAlphaTrans;
		break;
	case 0x0440: // In * AFog + Mem * 1-A
		// Bomberman64. alpha_cvg_sel: 1 cvg_x_alpha: 1
		type = kBlendModeAlphaTrans;
		break;
	case 0x04d0: // In * AFog + Fog * 1-A | In * AIn + Mem * 1-A
		// Conker.
		type = kBlendModeAlphaTrans;
		break;
	case 0x0150: // In * AIn + Mem * 1-A | In * AFog + Mem * 1-A
		// Spiderman.
		type = kBlendModeAlphaTrans;
		break;
	case 0x0c08: // In * 0 + In * 1
		// MarioKart (spinning logo)
		// This blend mode doesn't use the alpha value
		type = kBlendModeOpaque;
		break;
	case 0x0c18: // In * 0 + In * 1 | In * AIn + Mem * 1-A
		// StarFox main menu.
		type = kBlendModeAlphaTrans;
		break;
	case 0x0c40: // In * 0 + Mem * 1-A
		// Extreme-G.
		type = kBlendModeFade;
		break;
	case 0x0c48: // In * 0 + Mem * 1
		// SOTE text and hud
		type = kBlendModeFade;
		break;
	case 0x0f0a: // In * 0 + In * 1 | In * 0 + In * 1
		// Zelda OoT.
		type = kBlendModeOpaque;
		break;
	case 0x4c40: // Mem * 0 + Mem * 1-A
		//Waverace - alpha_cvg_sel: 0 cvg_x_alpha: 1
		type = kBlendModeFade;
		break;
	case 0x8410: // Bl * AFog + In * 1-A | In * AIn + Mem * 1-A
		// Paper Mario.
		type = kBlendModeAlphaTrans;
		break;
	case 0xc410: // Fog * AFog + In * 1-A | In * AIn + Mem * 1-A
		// Donald Duck (Dust)
		type = kBlendModeAlphaTrans;
		break;
	case 0xc440: // Fog * AFog + Mem * 1-A
		// Banjo Kazooie
		// Banjo Tooie sun glare
		// FIXME: blends fog over existing?
		type = kBlendModeAlphaTrans;
		break;
	case 0xc800: // Fog * AShade + In * 1-A
		//Bomberman64. alpha_cvg_sel: 0 cvg_x_alpha: 1
		type = kBlendModeOpaque;
		break;
	case 0xc810: // Fog * AShade + In * 1-A | In * AIn + Mem * 1-A
		// AeroGauge (ingame)
		type = kBlendModeAlphaTrans;
		break;
	// case 0x0321: // In * 0 + Bl * AMem
	// 	// Hmm - not sure about what this is doing. Zelda OoT pause screen.
	// 	type = kBlendModeAlphaTrans;
	// 	break;

	default:
#ifdef DAEDALUS_DEBUG_DISPLAYLIST
		DebugBlender( cycle_type, active_mode, alpha_cvg_sel, cvg_x_alpha );
		DL_PF( "		 Blend: SRCALPHA/INVSRCALPHA (default: 0x%04x)", active_mode );
#endif
		break;
	}

	// NB: we only have alpha in the blender is alpha_cvg_sel is 0 or cvg_x_alpha is 1.
	bool have_alpha = !alpha_cvg_sel || cvg_x_alpha;

	if (type == kBlendModeAlphaTrans && !have_alpha)
		type = kBlendModeOpaque;

	switch (type)
	{
	case kBlendModeOpaque:
		glDisable(GL_BLEND);
		break;
	case kBlendModeAlphaTrans:
		glBlendColor(0.f, 0.f, 0.f, 0.f);
		glBlendEquation(GL_FUNC_ADD);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_BLEND);
		break;
	case kBlendModeFade:
		glBlendColor(0.f, 0.f, 0.f, 0.f);
		glBlendEquation(GL_FUNC_ADD);
		glBlendFunc(GL_ZERO, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_BLEND);
		break;
	}
}


inline u32 MakeMask(u32 m)
{
	return m ? ((1<<m)-1) : 0xffffffff;
}

inline u32 MakeMirror(u32 mirror, u32 m)
{
	return (mirror && m) ? (1<<m) : 0;
}

void RendererGL::PrepareRenderState(const float (&mat_project)[16], bool disable_zbuffer)
{
    DAEDALUS_PROFILE("RendererGL::PrepareRenderState");

    // Depth test
    if (disable_zbuffer)
    {
        glDisable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);
    }
    else
    {
        if (gRDPOtherMode.zmode == 3)
        {
            glPolygonOffset(-1.0f, -1.0f);
        }
        else
        {
            glPolygonOffset(0.0f, 0.0f);
        }

        // e.g. enable or disable depth test based on RDP flags
        if ((mTnL.Flags.Zbuffer & gRDPOtherMode.z_cmp) | gRDPOtherMode.z_upd)
        {
            glEnable(GL_DEPTH_TEST);
        }
        else
        {
            glDisable(GL_DEPTH_TEST);
        }

        glDepthMask(gRDPOtherMode.z_upd ? GL_TRUE : GL_FALSE);
    }

    if (gRDPOtherMode.cycle_type < CYCLE_COPY && gRDPOtherMode.force_bl)
    {
        InitBlenderMode();
    }
    else
    {
        glDisable(GL_BLEND);
    }

    ShaderConfiguration config;
    MakeShaderConfigFromCurrentState(&config); 
    ShaderProgram* program = GetShaderForConfig(config);
    if (!program)
    {
        DBGConsole_Msg(0, "Couldn't generate a shader for mux %llx, cycle %d, alpha %d\n",
                       config.Mux, config.CycleType, config.AlphaThreshold);
        return;
    }

    glUseProgram(program->program);

    glUniformMatrix4fv(program->uloc_project, 1, GL_FALSE, mat_project);

    glUniform4f(program->uloc_primcol,
                mPrimitiveColour.GetRf(),
                mPrimitiveColour.GetGf(),
                mPrimitiveColour.GetBf(),
                mPrimitiveColour.GetAf());

    glUniform4f(program->uloc_envcol,
                mEnvColour.GetRf(),
                mEnvColour.GetGf(),
                mEnvColour.GetBf(),
                mEnvColour.GetAf());

    glUniform1f(program->uloc_primlodfrac, mPrimLODFraction);

    glUniform1i(program->uloc_foo, gRDPFrame);

    bool use_t1 = (gRDPOtherMode.cycle_type == CYCLE_2CYCLE);
    bool install_textures[] = { true, use_t1 };

    for (u32 i = 0; i < kNumTextures; ++i)
    {
        if (!install_textures[i]) continue;

        std::shared_ptr<CNativeTexture> texture = mBoundTexture[i];
        if (texture != nullptr)
        {
            glActiveTexture(GL_TEXTURE0 + i);
            texture->InstallTexture();

            // Setup relevant tile state from RDP
            u8 tile_idx           = mActiveTile[i];
            const RDP_Tile& rdp_tile = gRDPStateManager.GetTile(tile_idx);
            const RDP_TileSize& tile_size = gRDPStateManager.GetTileSize(tile_idx);

            glUniform1i(program->uloc_texture[i], i);

            bool clamp_s = rdp_tile.clamp_s || (rdp_tile.mask_s == 0);
            bool clamp_t = rdp_tile.clamp_t || (rdp_tile.mask_t == 0);

            auto MakeMask   = [](u32 m){ return m ? ((1<<m)-1) : 0xffffffff; };
            auto MakeMirror = [](u32 mirror, u32 m){ return (mirror && m) ? (1<<m) : 0; };

            u32 mirror_bits_s = MakeMirror(rdp_tile.mirror_s, rdp_tile.mask_s);
            u32 mirror_bits_t = MakeMirror(rdp_tile.mirror_t, rdp_tile.mask_t);
            u32 mask_bits_s   = MakeMask(rdp_tile.mask_s);
            u32 mask_bits_t   = MakeMask(rdp_tile.mask_t);

            glUniform2i(program->uloc_tileclamp[i], clamp_s, clamp_t);
            glUniform2f(program->uloc_tileshift[i],
                         kShiftScales[rdp_tile.shift_s],
                         kShiftScales[rdp_tile.shift_t]);

            glUniform2i(program->uloc_tilemask[i],   mask_bits_s,   mask_bits_t);
            glUniform2i(program->uloc_tilemirror[i], mirror_bits_s, mirror_bits_t);

            glUniform2i(program->uloc_tiletl[i], mTileTopLeft[i].s, mTileTopLeft[i].t);
            glUniform2i(program->uloc_tilebr[i], tile_size.right, tile_size.bottom);

            glUniform2f(program->uloc_texscale[i],
                         1.f / texture->GetCorrectedWidth(),
                         1.f / texture->GetCorrectedHeight());

            // Filtering
            if ((gRDPOtherMode.text_filt != G_TF_POINT) | (gGlobalPreferences.ForceLinearFilter))
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

void RendererGL::RenderTriangles(DaedalusVtx* p_vertices, u32 num_vertices, bool disable_zbuffer)
{
    if (mTnL.Flags.Texture)
    {
        UpdateTileSnapshots(mTextureTile);
        // NB: we have to do this after UpdateTileSnapshot, as it set up mTileTopLeft etc.
	// We have to do it before PrepareRenderState, because those values are applied to the graphics state.

        if (mTnL.Flags.Light && mTnL.Flags.TexGen)
        {
            if (std::shared_ptr<CNativeTexture> texture = mBoundTexture[0])
            {
                float x = float(mTileTopLeft[0].s) / 4.f;
                float y = float(mTileTopLeft[0].t) / 4.f;
                float w = float(texture->GetCorrectedWidth());
                float h = float(texture->GetCorrectedHeight());
                for (u32 i = 0; i < num_vertices; ++i)
                {
                    p_vertices[i].Texture.x = (p_vertices[i].Texture.x * w) + x;
                    p_vertices[i].Texture.y = (p_vertices[i].Texture.y * h) + y;
                }
            }
        }
    }

    PrepareRenderState(gProjection.m, disable_zbuffer);
    RenderDaedalusVtx(GL_TRIANGLES, p_vertices, num_vertices);
}

void RendererGL::TexRect(u32 tile_idx, const v2 & xy0, const v2 & xy1, TexCoord st0, TexCoord st1)
{
    UpdateTileSnapshots(tile_idx);
    PrepareTexRectUVs(&st0, &st1);

    PrepareRenderState(mScreenToDevice.mRaw, (gRDPOtherMode.depth_source) ? false : true);

    v2 screen0, screen1;
    ConvertN64ToScreen(xy0, screen0);
    ConvertN64ToScreen(xy1, screen1);

    const float depth = (gRDPOtherMode.depth_source) ? mPrimDepth : 0.0f;

    float positions[] = {
        screen0.x, screen0.y, depth,
        screen1.x, screen0.y, depth,
        screen0.x, screen1.y, depth,
        screen1.x, screen1.y, depth,
    };

    TexCoord uvs[] = {
        TexCoord(st0.s, st0.t),
        TexCoord(st1.s, st0.t),
        TexCoord(st0.s, st1.t),
        TexCoord(st1.s, st1.t),
    };

    u32 colours[] = {
        0xffffffff, 0xffffffff,
        0xffffffff, 0xffffffff,
    };

    RenderDaedalusVtxStreams(GL_TRIANGLE_STRIP, positions, uvs, colours, 4);
#ifdef DAEDALUS_DEBUG_DISPLAYLIST
    ++mNumRect;
#endif
}

void RendererGL::TexRectFlip(u32 tile_idx, const v2 & xy0, const v2 & xy1, TexCoord st0, TexCoord st1)
{
    UpdateTileSnapshots(tile_idx);
    PrepareTexRectUVs(&st0, &st1);

    PrepareRenderState(mScreenToDevice.mRaw, gRDPOtherMode.depth_source ? false : true);

    v2 screen0, screen1;
    ConvertN64ToScreen(xy0, screen0);
    ConvertN64ToScreen(xy1, screen1);

    const float depth = gRDPOtherMode.depth_source ? mPrimDepth : 0.0f;

    float positions[] = {
        screen0.x, screen0.y, depth,
        screen1.x, screen0.y, depth,
        screen0.x, screen1.y, depth,
        screen1.x, screen1.y, depth,
    };

    TexCoord uvs[] = {
        TexCoord(st0.s, st0.t),
        TexCoord(st0.s, st1.t),
        TexCoord(st1.s, st0.t),
        TexCoord(st1.s, st1.t),
    };

    u32 colours[] = {
        0xffffffff, 0xffffffff,
        0xffffffff, 0xffffffff,
    };

    RenderDaedalusVtxStreams(GL_TRIANGLE_STRIP, positions, uvs, colours, 4);
#ifdef DAEDALUS_DEBUG_DISPLAYLIST
    ++mNumRect;
#endif
}

void RendererGL::FillRect(const v2 & xy0, const v2 & xy1, u32 color)
{
    PrepareRenderState(mScreenToDevice.mRaw, gRDPOtherMode.depth_source ? false : true);

    v2 screen0, screen1;
    ConvertN64ToScreen(xy0, screen0);
    ConvertN64ToScreen(xy1, screen1);

    const float depth = gRDPOtherMode.depth_source ? mPrimDepth : 0.0f;

    float positions[] = {
        screen0.x, screen0.y, depth,
        screen1.x, screen0.y, depth,
        screen0.x, screen1.y, depth,
        screen1.x, screen1.y, depth,
    };

    // NB - these aren't needed. Could just pass NULL to RenderDaedalusVtxStreams?
    TexCoord uvs[] = {
        TexCoord(0.f, 0.f),
        TexCoord(1.f, 0.f),
        TexCoord(0.f, 1.f),
        TexCoord(1.f, 1.f),
    };

    u32 colours[] = {
        color, color, color, color
    };

    RenderDaedalusVtxStreams(GL_TRIANGLE_STRIP, positions, uvs, colours, 4);
#ifdef DAEDALUS_DEBUG_DISPLAYLIST
    ++mNumRect;
#endif
}

// Example 2D texture draws
void RendererGL::Draw2DTexture(f32 x0, f32 y0, f32 x1, f32 y1,
                               f32 u0, f32 v0, f32 u1, f32 v1,
                               std::shared_ptr<CNativeTexture> texture)
{
    DAEDALUS_PROFILE("RendererGL::Draw2DTexture");
    texture->InstallTexture();

    gRDPOtherMode.cycle_type = CYCLE_COPY; 
    
    PrepareRenderState(mScreenToDevice.mRaw, false);

    glEnable(GL_BLEND);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    float sx0 = N64ToScreenX(x0);
    float sy0 = N64ToScreenY(y0);
    float sx1 = N64ToScreenX(x1);
    float sy1 = N64ToScreenY(y1);

    const float depth = 0.0f;

    float positions[] = {
        sx0, sy0, depth,
        sx1, sy0, depth,
        sx0, sy1, depth,
        sx1, sy1, depth,
    };

    TexCoord uvs[] = {
        TexCoord(u0, v0),
        TexCoord(u1, v0),
        TexCoord(u0, v1),
        TexCoord(u1, v1),
    };

    u32 colours[] = {
        0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff
    };

    RenderDaedalusVtxStreams(GL_TRIANGLE_STRIP, positions, uvs, colours, 4);
}

void RendererGL::Draw2DTextureR(f32 x0, f32 y0,
                                f32 x1, f32 y1,
                                f32 x2, f32 y2,
                                f32 x3, f32 y3,
                                f32 s, f32 t,
                                std::shared_ptr<CNativeTexture> texture)
{
    texture->InstallTexture();
    gRDPOtherMode.cycle_type = CYCLE_COPY;
    PrepareRenderState(mScreenToDevice.mRaw, false);

    glEnable(GL_BLEND);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    float positions[] = {
        N64ToScreenX(x0), N64ToScreenY(y0), 0.0f,
        N64ToScreenX(x1), N64ToScreenY(y1), 0.0f,
        N64ToScreenX(x2), N64ToScreenY(y2), 0.0f,
        N64ToScreenX(x3), N64ToScreenY(y3), 0.0f,
    };

    TexCoord uvs[] = {
        TexCoord(0.f, 0.f),
        TexCoord(s,   0.f),
        TexCoord(s,   t),
        TexCoord(0.f, t),
    };

    u32 colours[] = {
        0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff
    };

    RenderDaedalusVtxStreams(GL_TRIANGLE_FAN, positions, uvs, colours, 4);
}

bool CreateRenderer()
{
    DAEDALUS_ASSERT_Q(gRenderer == nullptr);
    gRendererGL = new RendererGL();
    gRenderer   = gRendererGL;
    return true;
}

void DestroyRenderer()
{
    delete gRendererGL;
    gRendererGL = nullptr;
    gRenderer   = nullptr;
}
