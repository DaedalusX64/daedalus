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
	virtual void Clear(bool clear_screen, bool clear_depth)
	{
		GLbitfield mask = 0;

		if (clear_screen)
			mask |= GL_COLOR_BUFFER_BIT;
		if (clear_depth)
			mask |= GL_DEPTH_BUFFER_BIT;

		if (mask != 0)
			glClear(mask);
	}

	virtual void Clear(u32 frame_buffer_col, u32 depth)
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
	virtual bool UpdateFrame( bool wait_for_vbl )
	{
		//SwapBuffers( wglGetCurrentDC() );
		return true;
	}

	virtual bool GetBufferSize(u32 * p_width, u32 * p_height) { *p_width=640; *p_height=480; return true;}

	virtual void SetDebugScreenTarget( ETargetSurface buffer ) {}

	virtual void DumpNextScreen() {}
	virtual void DumpScreenShot() {}

private:
	HDC	hDC;
	HGLRC hRC;
	HWND hwnd;
};

template<> bool CSingleton< CGraphicsContext >::Create()
{
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