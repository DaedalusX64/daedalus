#include "stdafx.h"

#include "Core/Memory.h"

#include "Debug/DBGConsole.h"

#include "Graphics/GraphicsContext.h"

#include "HLEGraphics/BaseRenderer.h"
#include "HLEGraphics/TextureCache.h"
#include "HLEGraphics/DLParser.h"
#include "HLEGraphics/DisplayListDebugger.h"

#include "Plugins/GraphicsPlugin.h"


#ifdef DAEDALUS_W32
#include <GL/glew.h>
#endif
#include <GL/glfw.h>

EFrameskipValue     gFrameskipValue = FV_DISABLED;
u32                 gVISyncRate     = 1500;
bool                gTakeScreenshot = false;

class CGraphicsPluginImpl : public CGraphicsPlugin
{
	public:
		CGraphicsPluginImpl();
		~CGraphicsPluginImpl();

				bool		Initialise();

		virtual bool		StartEmulation()		{ return true; }

		virtual void		ViStatusChanged()		{}
		virtual void		ViWidthChanged()		{}
		virtual void		ProcessDList();

		virtual void		UpdateScreen();

		virtual void		RomClosed();

	private:
		u32					LastOrigin;
};

CGraphicsPluginImpl::CGraphicsPluginImpl()
:	LastOrigin( 0 )
{
}

CGraphicsPluginImpl::~CGraphicsPluginImpl()
{
}

bool CGraphicsPluginImpl::Initialise()
{
	if (!CreateRenderer())
	{
		return false;
	}

	if (!CTextureCache::Create())
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
	u32 current_origin = Memory_VI_GetRegister(VI_ORIGIN_REG);

	if (current_origin != LastOrigin)
	{
		if (gTakeScreenshot)
		{
			CGraphicsContext::Get()->DumpNextScreen();
			gTakeScreenshot = false;
		}

		CGraphicsContext::Get()->UpdateFrame( false );

		LastOrigin = current_origin;
	}
}

void CGraphicsPluginImpl::RomClosed()
{
	DBGConsole_Msg(0, "Finalising GLGraphics");
	DLParser_Finalise();
	CTextureCache::Destroy();
	DestroyRenderer();

	// Close OpenGL window and terminate GLFW
	glfwTerminate();

}

class CGraphicsPlugin *	CreateGraphicsPlugin()
{
	DBGConsole_Msg( 0, "Initialising Graphics Plugin [CGL]" );

	CGraphicsPluginImpl * plugin = new CGraphicsPluginImpl;
	if (!plugin->Initialise())
	{
		delete plugin;
		plugin = NULL;
	}

	return plugin;
}

