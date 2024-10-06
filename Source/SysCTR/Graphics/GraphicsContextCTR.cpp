
#include "Graphics/GraphicsContext.h"

#include <3ds.h>
#include <GL/picaGL.h>

#include "Interface/ConfigOptions.h"
#include "Core/ROM.h"
#include "Debug/DBGConsole.h"
#include "Debug/Dump.h"
#include "Graphics/ColourValue.h"
#include "Graphics/PngUtil.h"

#include "Interface/Preferences.h"
#include "Utility/Profiler.h"


#include "SysCTR/UI/InGameMenu.h"

extern void HandleEndOfFrame();

#define SCR_WIDTH 400
#define SCR_HEIGHT 240

static bool newFrame = true;

uint32_t  gVertexCount = 0;

float    *gVertexBuffer;
uint32_t *gColorBuffer;
float    *gTexCoordBuffer;

float    *gVertexBufferPtr;
uint32_t *gColorBufferPtr;
float    *gTexCoordBufferPtr;

class IGraphicsContext : public CGraphicsContext
{
public:
	IGraphicsContext();
	virtual ~IGraphicsContext();

	bool				Initialise();
	bool				IsInitialised() const { return mInitialised; }

	void				ClearAllSurfaces();

	void				ClearToBlack();
	void				ClearZBuffer();
	void				ClearColBuffer(const c32 &colour);
	void				ClearColBufferAndDepth(const c32 &colour);

	void				ResetVertexBuffer();

	void				BeginFrame();
	void				EndFrame();
	void				UpdateFrame(bool wait_for_vbl);
	void				GetScreenSize(u32 * width, u32 * height) const;

	void				SetDebugScreenTarget( ETargetSurface buffer );

	void				ViewportType(u32 *d_width, u32 *d_height) const;
	void				DumpScreenShot();
	void				DumpNextScreen()			{ mDumpNextScreen = 2; }

private:
	void				SaveScreenshot(const char* filename, s32 x, s32 y, u32 width, u32 height);

private:
	bool				mInitialised;

	u32					mDumpNextScreen;
};

//*************************************************************************************
//
//*************************************************************************************
template<> bool CSingleton< CGraphicsContext >::Create()
{
	#ifdef DAEDALUS_ENABLE_ASSERTS
	DAEDALUS_ASSERT_Q(mpInstance == nullptr);
#endif
	 mpInstance = std::make_shared<IGraphicsContext>();
	return mpInstance->Initialise();
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

uint32_t gMaxVertices = 30000;

IGraphicsContext::IGraphicsContext() : mInitialised(false), mDumpNextScreen(false)
{	
	gVertexBufferPtr   =    (float*)linearAlloc(gMaxVertices * sizeof(float) * 3);
	gTexCoordBufferPtr =    (float*)linearAlloc(gMaxVertices * sizeof(float) * 2);
	gColorBufferPtr    = (uint32_t*)linearAlloc(gMaxVertices * sizeof(uint32_t) );
	
	gVertexBuffer   = gVertexBufferPtr;
	gColorBuffer    = gColorBufferPtr;
	gTexCoordBuffer = gTexCoordBufferPtr;
}

IGraphicsContext::~IGraphicsContext()
{
	linearFree(gVertexBufferPtr);
	linearFree(gColorBufferPtr);
	linearFree(gTexCoordBufferPtr);
}

bool IGraphicsContext::Initialise()
{
	mInitialised = true;

	pglSelectScreen(GFX_TOP, GFX_LEFT);
	ClearAllSurfaces();

	return true;
}

void IGraphicsContext::ClearAllSurfaces()
{
	ClearToBlack();
	pglSwapBuffers();
	ClearToBlack();
	pglSwapBuffers();
}

void IGraphicsContext::ClearToBlack()
{
	glViewport(0,0,400,240);
	glDisable(GL_SCISSOR_TEST);

	glDepthMask(GL_TRUE);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClearDepth( 1.0f );
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void IGraphicsContext::ClearZBuffer()
{
	glDepthMask(GL_TRUE);
	glClearDepth( 1.0f );
	glClear( GL_DEPTH_BUFFER_BIT );
}

void IGraphicsContext::ClearColBuffer(const c32 & colour)
{
	glClearColor( colour.GetRf(), colour.GetGf(), colour.GetBf(), colour.GetAf() );
	glClear( GL_COLOR_BUFFER_BIT );
}

void IGraphicsContext::ClearColBufferAndDepth(const c32 & colour)
{
	glDepthMask(GL_TRUE);
	glClearDepth( 1.0f );
	glClearColor( colour.GetRf(), colour.GetGf(), colour.GetBf(), colour.GetAf() );
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
}

void IGraphicsContext::ResetVertexBuffer()
{
	gVertexBuffer   = gVertexBufferPtr;
	gColorBuffer    = gColorBufferPtr;
	gTexCoordBuffer = gTexCoordBufferPtr;

	gVertexCount      = 0;
}

void IGraphicsContext::BeginFrame()
{
	if(newFrame)
	{
		UI::DrawInGameMenu();
		ClearToBlack();
		newFrame = false;
	}

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	glVertexPointer(3, GL_FLOAT, 0, gVertexBufferPtr);
	glColorPointer(4, GL_UNSIGNED_BYTE, 0, gColorBufferPtr);
	glTexCoordPointer(2, GL_FLOAT, 0, gTexCoordBufferPtr);
}

void IGraphicsContext::EndFrame()
{
	HandleEndOfFrame();
}

void IGraphicsContext::UpdateFrame(bool wait_for_vbl)
{
	pglSwapBuffers();

	newFrame = true;
}

void IGraphicsContext::ViewportType(u32 *d_width, u32 *d_height) const
{
	switch ( gGlobalPreferences.ViewportType )
	{
		case VT_UNSCALED_4_3:
			*d_width = 320;
			*d_height = 240;
			break;
		default:
			*d_width = SCR_WIDTH;
			*d_height = SCR_HEIGHT;
			break;
	}
}

void IGraphicsContext::GetScreenSize(u32 * p_width, u32 * p_height) const
{
	*p_width = SCR_WIDTH;
	*p_height = SCR_HEIGHT;
}

void IGraphicsContext::SetDebugScreenTarget(ETargetSurface buffer){}

void IGraphicsContext::SaveScreenshot(const char* filename, s32 x, s32 y, u32 width, u32 height){}

void IGraphicsContext::DumpScreenShot(){}
