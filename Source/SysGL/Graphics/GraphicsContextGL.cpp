
#include "stdafx.h"

#include <stdio.h>

#include "SysGL/GL.h"
#include "Graphics/GraphicsContext.h"

#include "Graphics/ColourValue.h"


static u32 SCR_WIDTH = 640;
static u32 SCR_HEIGHT = 480;


class GraphicsContextGL : public CGraphicsContext
{
public:
	virtual ~GraphicsContextGL();


	virtual bool Initialise();
	virtual bool IsInitialised() const { return true; }

	virtual void ClearAllSurfaces();
	virtual void ClearZBuffer();
	virtual void ClearColBuffer(const c32 & colour);
	virtual void ClearToBlack();
	virtual void ClearColBufferAndDepth(const c32 & colour);
	virtual	void BeginFrame();
	virtual void EndFrame();
	virtual void UpdateFrame( bool wait_for_vbl );

	virtual void GetScreenSize(u32 * width, u32 * height) const;
	virtual void ViewportType(u32 * width, u32 * height) const;

	virtual void SetDebugScreenTarget( ETargetSurface buffer ) {}
	virtual void DumpNextScreen() {}
	virtual void DumpScreenShot() {}
};

template<> bool CSingleton< CGraphicsContext >::Create()
{
	DAEDALUS_ASSERT_Q(mpInstance == NULL);

	mpInstance = new GraphicsContextGL();
	return mpInstance->Initialise();
}


GraphicsContextGL::~GraphicsContextGL()
{
	// glew

	glfwCloseWindow();

	glfwTerminate();
}

extern bool initgl();
bool GraphicsContextGL::Initialise()
{
	// Initialise GLFW
	if( !glfwInit() )
	{
		fprintf( stderr, "Failed to initialize GLFW\n" );
		return false;
	}

	glfwOpenWindowHint(GLFW_FSAA_SAMPLES,4);
	glfwOpenWindowHint(GLFW_OPENGL_VERSION_MAJOR, 3);
	glfwOpenWindowHint(GLFW_OPENGL_VERSION_MINOR, 2);

	// Open a window and create its OpenGL context
	if( !glfwOpenWindow( SCR_WIDTH, SCR_HEIGHT,
						0,0,0,0,			// RGBA bits
						24,					// Depth bits
						0,					// Stencil bits
						GLFW_WINDOW ) )
	{
		fprintf( stderr, "Failed to open GLFW window\n" );

		glfwTerminate();
		return false;
	}

	glfwSetWindowTitle( "Daedalus" );

	// Ensure we can capture the escape key being pressed below
	//glfwEnable( GLFW_STICKY_KEYS );

	// Enable vertical sync (on cards that support it)
	glfwSwapInterval( 1 );

	// Initialise GLEW
	//glewExperimental = GL_TRUE;
	GLenum err = glewInit();
	if (err != GLEW_OK || !GLEW_VERSION_3_2)
	{
		fprintf( stderr, "Failed to initialize GLEW\n" );
		return false;
	}

	ClearAllSurfaces();
	
	if (glfwGetWindowParam(GLFW_FSAA_SAMPLES) != 0)
		fprintf( stderr, "Full Screen Anti-Aliasing 4X has been enabled\n" );

	// FIXME(strmnnrmn): this needs tidying.
	return initgl();
}

void GraphicsContextGL::GetScreenSize(u32 * width, u32 * height) const
{
	int window_width, window_height;
	glfwGetWindowSize( &window_width, &window_height );

	*width  = window_width;
	*height = window_height;
}

void GraphicsContextGL::ViewportType(u32 * width, u32 * height) const
{
	GetScreenSize(width, height);
}

void GraphicsContextGL::ClearAllSurfaces()
{
	// FIXME: this should clear/flip a couple of times to ensure the front and backbuffers are cleared.
	// Not sure if it's necessary...
	ClearToBlack();
}

void GraphicsContextGL::ClearToBlack()
{
	glDepthMask(GL_TRUE);
	glClearDepth( 1.0f );
	glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
}

void GraphicsContextGL::ClearZBuffer()
{
	glDepthMask(GL_TRUE);
	glClearDepth( 1.0f );
	glClear( GL_DEPTH_BUFFER_BIT );
}

void GraphicsContextGL::ClearColBuffer(const c32 & colour)
{
	glClearColor( colour.GetRf(), colour.GetGf(), colour.GetBf(), colour.GetAf() );
	glClear( GL_COLOR_BUFFER_BIT );
}

void GraphicsContextGL::ClearColBufferAndDepth(const c32 & colour)
{
	glDepthMask(GL_TRUE);
	glClearDepth( 1.0f );
	glClearColor( colour.GetRf(), colour.GetGf(), colour.GetBf(), colour.GetAf() );
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
}

void GraphicsContextGL::BeginFrame()
{
	// Get window size (may be different than the requested size)
	int width, height;
	glfwGetWindowSize( &width, &height );

	// Special case: avoid division by zero below
	height = height > 0 ? height : 1;

	glViewport( 0, 0, width, height );
	glScissor( 0, 0, width, height );
}

void GraphicsContextGL::EndFrame()
{
}

void GraphicsContextGL::UpdateFrame( bool wait_for_vbl )
{
	glfwSwapBuffers();
//	if( gCleanSceneEnabled ) //TODO: This should be optional
	{
		ClearColBuffer( c32(0xff000000) ); // ToDo : Use gFillColor instead?
	}
}
