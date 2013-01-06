/*
Copyright (C) 2007 StrmnNrmn

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef GRAPHICSPLUGINW32_H__
#define GRAPHICSPLUGINW32_H__

#include "Plugins/GraphicsPlugin.h"
#include "GraphicsPlugin_Spec.h"
#include "Utility/CritSect.h"
#include "Debug/DBGConsole.h"

#define PLUGIN_SPEC_CALL		__cdecl


#ifdef DAEDALUS_TRAP_PLUGIN_EXCEPTIONS
#define GFX_TRY		try
#define GFX_CATCH	catch(...)								\
		{													\
				DBGConsole_Msg(0, "Exception in [C%s]", mModuleName);	\
				Close();									\
				throw;			\
		}
#else
#define GFX_TRY
#define GFX_CATCH
#endif

#define DAEDALUS_GFX_CAPTURESCREEN "CaptureScreen"
#define DAEDALUS_GFX_DROPTEXTURES "DropTextures"
#define DAEDALUS_GFX_DUMPTEXTURES "DumpTextures"
#define DAEDALUS_GFX_NODUMPTEXTURES "NoDumpTextures"
#define DAEDALUS_GFX_USENEWCOMBINER "UseNewCombiner"
#define DAEDALUS_GFX_DUMPDL "DumpDisplayList"
#define DAEDALUS_GFX_SET_DEBUG	"SetDebug"

class CGraphicsPluginDll : public CGraphicsPlugin
{
		CGraphicsPluginDll();
	protected:
		friend class CSingleton< CGraphicsPlugin >;
		friend class CPluginsPage;

	public:
		static CGraphicsPluginDll *		Create( const char * dll_name );


		virtual ~CGraphicsPluginDll();

	public:

		virtual bool			StartEmulation();


		//EXPORT void CALL ViWidthChanged (void);
		void ViWidthChanged()
		{
			AUTO_CRIT_SECT( mCritSect );

			if (m_pViWidthChanged)
			{
				GFX_TRY { m_pViWidthChanged(); }
				GFX_CATCH
			}
		}

		//EXPORT void CALL ViStatusChanged (void);
		void ViStatusChanged()
		{
			AUTO_CRIT_SECT( mCritSect );

			if (m_pViStatusChanged)
			{
				GFX_TRY { m_pViStatusChanged(); }
				GFX_CATCH
			}
		}

		//EXPORT void CALL MoveScreen(int xpos, int ypos);
		void MoveScreen(int xpos, int ypos)
		{
			AUTO_CRIT_SECT( mCritSect );

			if (m_pMoveScreen)
			{
				GFX_TRY { m_pMoveScreen(xpos, ypos); }
				GFX_CATCH
			}
		}

		//EXPORT void CALL DrawScreen (void);
		void DrawScreen()
		{
			AUTO_CRIT_SECT( mCritSect );

			if (m_pDrawScreen)
			{
				GFX_TRY { m_pDrawScreen(); }
				GFX_CATCH
			}
		}

		//EXPORT void CALL ChangeWindow (void);
		void ChangeWindow()
		{
			AUTO_CRIT_SECT( mCritSect );

			if (m_pChangeWindow)
			{
				GFX_TRY { m_pChangeWindow(); }
				GFX_CATCH
			}
		}


		//EXPORT void CALL UpdateScreen (void);
		void UpdateScreen()
		{
			AUTO_CRIT_SECT( mCritSect );

			if (m_pUpdateScreen)
			{
				GFX_TRY { m_pUpdateScreen(); }
				GFX_CATCH
			}
		}

		//EXPORT void CALL DllAbout ( HWND hParent );
		void DllAbout(HWND hWndParent)
		{
			AUTO_CRIT_SECT( mCritSect );

			if (m_pDllAbout)
			{
				GFX_TRY { m_pDllAbout(hWndParent); }
				GFX_CATCH
			}
		}

		//EXPORT void CALL DllConfig ( HWND hParent );
		void DllConfig(HWND hWndParent)
		{
			AUTO_CRIT_SECT( mCritSect );

			if (m_pDllConfig)
			{
				GFX_TRY { m_pDllConfig(hWndParent); }
				GFX_CATCH
			}
		}

		//EXPORT void CALL DllTest ( HWND hParent );
		void DllTest(HWND hWndParent)
		{
			AUTO_CRIT_SECT( mCritSect );

			if (m_pDllTest)
			{
				GFX_TRY { m_pDllTest(hWndParent); }
				GFX_CATCH
			}
		}

		//EXPORT BOOL CALL InitiateGFX (GFX_INFO Gfx_Info);
		BOOL InitiateGFX(GFX_INFO gi)
		{
			BOOL bInit = FALSE;

			AUTO_CRIT_SECT( mCritSect );

			if (m_pInitiateGFX)
			{
				GFX_TRY { bInit = m_pInitiateGFX(gi); }
				GFX_CATCH
			}

			return bInit;
		}

		//EXPORT void CALL ProcessDList(void);
		void ProcessDList(void)
		{
			AUTO_CRIT_SECT( mCritSect );

			if (m_pProcessDList)
			{
				GFX_TRY { m_pProcessDList(); }
				GFX_CATCH
			}
		}


		//EXPORT void CALL RomClosed (void);
		void RomClosed(void)
		{
			AUTO_CRIT_SECT( mCritSect );

			if (m_pRomClosed)
			{
				GFX_TRY { m_pRomClosed(); }
				GFX_CATCH
			}
		}

		//EXPORT void CALL RomOpen (void);
		void RomOpen(void)
		{
			AUTO_CRIT_SECT( mCritSect );

			if (m_pRomOpen)
			{
				GFX_TRY { m_pRomOpen(); }
				GFX_CATCH
			}
		}

		HRESULT ExecuteCommand( const char * pszCommand, void * pVoid )
		{
			HRESULT hr = 0;	// XXXX

			AUTO_CRIT_SECT( mCritSect );

			if ( m_pExecuteCommand )
			{
				GFX_TRY { hr = m_pExecuteCommand( pszCommand, pVoid ); }
				GFX_CATCH
			}

			return hr;
		}


		BOOL LoadedOk() const { return m_bLoadedOk; }

		// szName must be 100 chars
		void GetPluginName(char * szName) const
		{
			if (!LoadedOk())
				strcpy(szName, "?");
			else
				strcpy(szName, m_pi.Name);

		}

		BOOL Open( const char * dll_name );
		BOOL Init();

		void Close();

		//EXPORT void CALL CloseDLL (void);
		void CloseDll()
		{
			AUTO_CRIT_SECT( mCritSect );

			if (m_pCloseDll)
			{
				GFX_TRY { m_pCloseDll(); }
				GFX_CATCH
			}
		}

		static BOOL GetPluginInfo( const char * szFilename, PLUGIN_INFO & pi );

	protected:
		void   (PLUGIN_SPEC_CALL * m_pViWidthChanged)(void);
		void   (PLUGIN_SPEC_CALL * m_pViStatusChanged)(void);
		void   (PLUGIN_SPEC_CALL * m_pMoveScreen)(int, int);
		void   (PLUGIN_SPEC_CALL * m_pDrawScreen)(void);
		void   (PLUGIN_SPEC_CALL * m_pChangeWindow)(void);
		void   (PLUGIN_SPEC_CALL * m_pUpdateScreen)();
		void   (PLUGIN_SPEC_CALL * m_pCloseDll)();
		void   (PLUGIN_SPEC_CALL * m_pDllAbout)(HWND);
		void   (PLUGIN_SPEC_CALL * m_pDllConfig)(HWND);
		void   (PLUGIN_SPEC_CALL * m_pDllTest)(HWND);
		BOOL   (PLUGIN_SPEC_CALL * m_pInitiateGFX)(GFX_INFO);
		void   (PLUGIN_SPEC_CALL * m_pProcessDList)(void);
		void   (PLUGIN_SPEC_CALL * m_pRomClosed)(void);
		void   (PLUGIN_SPEC_CALL * m_pRomOpen)(void);

		// Daedalus commands
		HRESULT (PLUGIN_SPEC_CALL * m_pExecuteCommand)( const CHAR * m_pszCommand, void * m_pResult );

		HINSTANCE	m_hModule;
		PLUGIN_INFO m_pi;
		BOOL		m_bLoadedOk;
		CHAR		mModuleName[MAX_PATH+1];

		CCritSect	mCritSect;
};

#endif //GRAPHICSPLUGINW32_H__
