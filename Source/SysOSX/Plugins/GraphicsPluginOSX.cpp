#include "stdafx.h"
#include "GraphicsPluginOSX.h"

#include <GL/glfw.h>

#include "Debug/DBGConsole.h"

#include "Plugins/GraphicsPlugin.h"


class CGraphicsPluginImpl : public CGraphicsPlugin
{
	public:
		~CGraphicsPluginImpl();

				bool		Initialise();

		virtual bool		StartEmulation()		{ return true; }

		virtual void		ViStatusChanged()		{}
		virtual void		ViWidthChanged()		{}
		virtual void		ProcessDList();

		virtual void		UpdateScreen();

		virtual void		RomClosed();
};

CGraphicsPluginImpl::~CGraphicsPluginImpl()
{
}

bool CGraphicsPluginImpl::Initialise()
{
    // Initialise GLFW
    if( !glfwInit() )
    {
        fprintf( stderr, "Failed to initialize GLFW\n" );
        return false;
    }

    // Open a window and create its OpenGL context
    if( !glfwOpenWindow( 640, 480, 0,0,0,0, 0,0, GLFW_WINDOW ) )
    {
        fprintf( stderr, "Failed to open GLFW window\n" );

        glfwTerminate();
        return false;
    }

    glfwSetWindowTitle( "Daedalus" );

    // Ensure we can capture the escape key being pressed below
    glfwEnable( GLFW_STICKY_KEYS );

    // Enable vertical sync (on cards that support it)
    glfwSwapInterval( 1 );

	return true;
}

void CGraphicsPluginImpl::ProcessDList()
{
}

void CGraphicsPluginImpl::UpdateScreen()
{
    double t = glfwGetTime();
    int x;
    glfwGetMousePos( &x, NULL );

    // Get window size (may be different than the requested size)
    int width, height;
    glfwGetWindowSize( &width, &height );

    // Special case: avoid division by zero below
    height = height > 0 ? height : 1;

    glViewport( 0, 0, width, height );

    // Clear color buffer to black
    glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );
    glClear( GL_COLOR_BUFFER_BIT );

    // Select and setup the projection matrix
    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();
    gluPerspective( 65.0f, (GLfloat)width/(GLfloat)height, 1.0f, 100.0f );

    // Select and setup the modelview matrix
    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();
    gluLookAt( 0.0f, 1.0f, 0.0f,    // Eye-position
               0.0f, 20.0f, 0.0f,   // View-point
               0.0f, 0.0f, 1.0f );  // Up-vector

    // Draw a rotating colorful triangle
    glTranslatef( 0.0f, 14.0f, 0.0f );
    glRotatef( 0.3f*(GLfloat)x + (GLfloat)t*100.0f, 0.0f, 0.0f, 1.0f );
    glBegin( GL_TRIANGLES );
      glColor3f( 1.0f, 0.0f, 0.0f );
      glVertex3f( -5.0f, 0.0f, -4.0f );
      glColor3f( 0.0f, 1.0f, 0.0f );
      glVertex3f( 5.0f, 0.0f, -4.0f );
      glColor3f( 0.0f, 0.0f, 1.0f );
      glVertex3f( 0.0f, 0.0f, 6.0f );
    glEnd();

    // Swap buffers
    glfwSwapBuffers();
}

void CGraphicsPluginImpl::RomClosed()
{
    // Close OpenGL window and terminate GLFW
    glfwTerminate();

}

class CGraphicsPlugin *	CreateGraphicsPlugin()
{
	DBGConsole_Msg( 0, "Initialising Graphics Plugin [CPSP]" );

	CGraphicsPluginImpl * plugin = new CGraphicsPluginImpl;
	if( !plugin->Initialise() )
	{
		delete plugin;
		plugin = NULL;
	}

	return plugin;
}

