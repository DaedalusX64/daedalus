#include "stdafx.h"
#include "GraphicsPluginOSX.h"

#include <GL/glfw.h>

#include "Core/Memory.h"

#include "Debug/DBGConsole.h"

#include "Graphics/GraphicsContext.h"

#include "HLEGraphics/PSPRenderer.h"
#include "HLEGraphics/TextureCache.h"
#include "HLEGraphics/DLParser.h"
#ifdef DAEDALUS_DEBUG_DISPLAYLIST
#include "HLEGraphics/DisplayListDebugger.h"
#endif

#include "Plugins/GraphicsPlugin.h"


EFrameskipValue     gFrameskipValue = FV_DISABLED;
u32                 gVISyncRate     = 1500;
bool                gTakeScreenshot = false;

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
	if(!PSPRenderer::Create())
	{
		return false;
	}

	if(!CTextureCache::Create())
	{
		return false;
	}

	if (!DLParser_Initialise())
	{
		return false;
	}

	return true;
}

void CGraphicsPluginImpl::ProcessDList()
{
#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	if (!DLDebugger_Process())
	{
		DLParser_Process();
	}
#else
	DLParser_Process();
#endif
}

void CGraphicsPluginImpl::UpdateScreen()
{
	static u32    last_origin = 0;
	u32 current_origin = Memory_VI_GetRegister(VI_ORIGIN_REG);

	if( current_origin != last_origin )
	{
		if(gTakeScreenshot)
		{
			CGraphicsContext::Get()->DumpNextScreen();
			gTakeScreenshot = false;
		}

		CGraphicsContext::Get()->UpdateFrame( false );

		last_origin = current_origin;
	}
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

