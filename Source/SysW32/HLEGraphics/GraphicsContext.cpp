#include "stdafx.h"
#include "gl/gl.h"

#include "Interface/MainWindow.h"
#include "Graphics/GraphicsContext.h"

class IGraphicsContext : public CGraphicsContext
{
	virtual ~IGraphicsContext();
	virtual bool Initialise();

	virtual bool IsInitialised() const {return true;}

	virtual void ClearAllSurfaces()
	{
	}
	virtual void ClearToBlack()
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	virtual void ClearWithColour(u32 frame_buffer_col, u32 depth)
	{
		glClearColor(0, 0, 0, 0);
		glClearDepth(depth);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}
	virtual	void BeginFrame()
	{
		//glBegin(GL_TRIANGLES);
	}
	virtual void EndFrame()
	{
		//glEnd();
	}
	virtual void UpdateFrame( bool wait_for_vbl )
	{
		//SwapBuffers( wglGetCurrentDC() );
	}

	virtual void GetScreenSize(u32 * width, u32 * height) const
	{
		*p_width=640;
		*p_height=480;
	}

	virtual void SetDebugScreenTarget( ETargetSurface buffer ) {}

	virtual void ViewportType( u32 * d_width, u32 * d_height ) {}

	virtual void DumpNextScreen() {}
	virtual void DumpScreenShot() {}

private:
	HDC	hDC;
	HGLRC hRC;
	HWND hwnd;
};

template<> bool CSingleton< CGraphicsContext >::Create()
{
	DAEDALUS_ASSERT_Q(mpInstance == NULL);

	mpInstance = new IGraphicsContext();
	return mpInstance->Initialise();
}

bool IGraphicsContext::Initialise()
{
	int     pixelFormat;

	PIXELFORMATDESCRIPTOR pfd = {
		sizeof(PIXELFORMATDESCRIPTOR),    // size of this pfd
		1,                                // version number
		PFD_DRAW_TO_WINDOW |              // support window
		PFD_SUPPORT_OPENGL |              // support OpenGL
		PFD_DOUBLEBUFFER,                 // double buffered
		PFD_TYPE_RGBA,                    // RGBA type
		32,                               // color depth
		0, 0, 0, 0, 0, 0,                 // color bits ignored
		0,                                // no alpha buffer
		0,                                // shift bit ignored
		0,                                // no accumulation buffer
		0, 0, 0, 0,                       // accum bits ignored
		32,                               // z-buffer
		0,                                // no stencil buffer
		0,                                // no auxiliary buffer
		PFD_MAIN_PLANE,                   // main layer
		0,                                // reserved
		0, 0, 0                           // layer masks ignored
	};

	hwnd = CMainWindow::Get()->GetWindow();
	hDC = ::GetDC(hwnd);
	pixelFormat = ChoosePixelFormat( hDC, &pfd );
	SetPixelFormat( hDC, pixelFormat, &pfd );
	hRC = wglCreateContext( hDC );

	wglMakeCurrent( hDC, hRC );

	return true;
}

IGraphicsContext::~IGraphicsContext()
{
	wglMakeCurrent( NULL, NULL );
	wglDeleteContext( hRC );
	ReleaseDC( hwnd, hDC );
}
