/*

  Copyright (C) 2001 StrmnNrmn

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


#include "Base/Types.h"
#include <format>

#include <pspgu.h>
#include <pspdisplay.h>
#include <pspdebug.h>

#include "Interface/ConfigOptions.h"
#include "Core/ROM.h"
#include "Debug/DBGConsole.h"
#include "Debug/Dump.h"
#include "Graphics/ColourValue.h"
#include "Graphics/GraphicsContext.h"
#include "Graphics/PngUtil.h"
#include "SysPSP/Graphics/VideoMemoryManager.h"

#include "UI/DrawText.h"
#include "Interface/Preferences.h"
#include "Utility/Profiler.h"
#include "Utility/VolatileMem.h"


constexpr std::string gScreenDumpDumpPathFormat = "sd{}.png";

#define DLISTSIZE 1*1024*1024	//Size of PSP Dlist

static u32 PIXEL_SIZE = 2; /* change this if you change to another screenmode */
static u32 SCR_MODE	  = GU_PSM_5650;

#define FRAME_SIZE (BUF_WIDTH * SCR_HEIGHT * PIXEL_SIZE)
#define DEPTH_SIZE (BUF_WIDTH * SCR_HEIGHT * 2)

#define LACED_HEIGHT 503
#define LACED_SIZE (BUF_WIDTH * LACED_HEIGHT * PIXEL_SIZE)

//Get Dlist memory from malloc
static u32* list[2];

//static u32 __attribute__((aligned(16))) list[2][262144];	//Some games uses huge amount here like Star Soldier - Vanishing Earth
static u32 __attribute__((aligned(16))) callList[64];
static u32 __attribute__((aligned(16))) ilist[256];

u32 listNum = 0;
extern bool g32bitColorMode;
extern bool gTakeScreenshotSS;
//////////////////////////////////////////////
//bool CGraphicsContext::CleanScene = false;
//////////////////////////////////////////////
static u32 BUF_WIDTH = 512;
static u32 SCR_WIDTH = 480;
static u32 SCR_HEIGHT = 272;

u32 LACED_DRAW;
u32 LACED_DISP;

/* Video Manager functions */
extern "C" {
int pspDveMgrCheckVideoOut();
int pspDveMgrSetVideoOut(int, int, int, int, int, int, int);
}

int HAVE_DVE = -1; // default is no DVE Manager
int PSP_TV_CABLE = -1; // default is no cable
int PSP_TV_LACED = 0; // default is not interlaced

static const ScePspIMatrix4 dither_matrixA =
		{{-2, 1,-1, 2},
		 {-1, 2,-2, 1},
		 { 2,-1, 1,-2},
		 { 1,-2, 2,-1}};
static const ScePspIMatrix4 dither_matrixB =
		{{-2,-1, 2, 1},
		 { 1, 2,-1,-2},
		 {-1,-2, 1, 2},
		 { 2, 1,-2,-1}};

// Implementation
class IGraphicsContext : public CGraphicsContext
{
public:
	IGraphicsContext();
	virtual ~IGraphicsContext();

	bool				Initialise();
	bool				IsInitialised() const { return mInitialised; }

	void				SwitchToChosenDisplay();
	void				SwitchToLcdDisplay();
	void				StoreSaveScreenData();

	void				ClearAllSurfaces();

	void				ClearToBlack();
	void				ClearZBuffer();
	void				ClearColBuffer(const c32 & colour);
	void				ClearColBufferAndDepth(const c32 & colour);

	void				BeginFrame();
	void				EndFrame();
	void				UpdateFrame( bool wait_for_vbl );
	void				GetScreenSize( u32 * width, u32 * height ) const;

	void				SetDebugScreenTarget( ETargetSurface buffer );

	void				ViewportType( u32 * d_width, u32 * d_height ) const;
	void				DumpScreenShot();
	void				DumpNextScreen()			{ mDumpNextScreen = 2; }

private:
	void				SaveScreenshot( const std::filesystem::path filename, s32 x, s32 y, u32 width, u32 height );

private:
	bool				mInitialised;

	void *				mpBuffers[2];
	void *				mpCurrentBackBuffer;

	void *				save_disp_rel;
	void *				save_draw_rel;
	void *				save_depth_rel;

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

IGraphicsContext::IGraphicsContext()
:	mInitialised(false)
,	mpCurrentBackBuffer(nullptr)
,	mDumpNextScreen( false )
{
	mpBuffers[ 0 ] = nullptr;
	mpBuffers[ 1 ] = nullptr;

#if 1 //1->alloc in volatile memory, 0->alloc in VRAM //Corn
	//Set up PSP Dlists in the extra 4MB space and make sure pointer are aligned to 16 bytes
	list[0] = (u32*)(((u32)malloc_volatile(DLISTSIZE) + 0xF) & ~0xF);
	list[1] = (u32*)(((u32)malloc_volatile(DLISTSIZE) + 0xF) & ~0xF);
#else
	//Set up PSP Dlists in VRAM(if available)
	void *ptr;
	bool is_videmem;
	CVideoMemoryManager::Get()->Alloc( DLISTSIZE, &ptr, &is_videmem );
	list[0] = (u32*)ptr;
	CVideoMemoryManager::Get()->Alloc( DLISTSIZE, &ptr, &is_videmem );
	list[1] = (u32*)ptr;
#endif
}

//*****************************************************************************
//
//*****************************************************************************
IGraphicsContext::~IGraphicsContext()
{
	sceGuTerm();
}

//*****************************************************************************
//Also Known as do PSP Graphics Frame
//*****************************************************************************
void IGraphicsContext::ClearAllSurfaces()
{
	/*sceGuStart(GU_CALL,list[0]);
	sceGuFinish();
	sceGuStart(GU_CALL,list[1]);
	sceGuFinish();*/

	if(gDoubleDisplayEnabled)
		sceGuStart(GU_CALL,list[listNum]); //Begin other Display List	return;

	for( u32 i = 0; i < 2; ++i )
	{
		//Start Frame
		BeginFrame();
		//Set ViewPort
		sceGuOffset(2048 - (SCR_WIDTH/2),2048 - (SCR_HEIGHT/2));
		sceGuViewport(2048,2048,SCR_WIDTH,SCR_HEIGHT);
		sceGuScissor(0,0,SCR_WIDTH,SCR_HEIGHT);
		ClearToBlack();
		EndFrame();
		//Get Ready for next Frame
		UpdateFrame( false );
	}
}

//*****************************************************************************
//Clear screen and/or Zbuffer
//*****************************************************************************
void IGraphicsContext::ClearToBlack()
{
	sceGuClearColor(0xff000000);
	sceGuClearDepth(0);
	sceGuClear(GU_COLOR_BUFFER_BIT | GU_DEPTH_BUFFER_BIT | GU_FAST_CLEAR_BIT);
}

void IGraphicsContext::ClearZBuffer()
{
	sceGuClearDepth(0);
	sceGuClear(GU_DEPTH_BUFFER_BIT | GU_FAST_CLEAR_BIT);
}

void IGraphicsContext::ClearColBuffer(const c32 & colour)
{
	sceGuClearColor(colour.GetColour());
	sceGuClear(GU_COLOR_BUFFER_BIT | GU_FAST_CLEAR_BIT);
}

void IGraphicsContext::ClearColBufferAndDepth(const c32 & colour)
{
	sceGuClearColor(colour.GetColour());
	sceGuClearDepth(0);
	sceGuClear(GU_COLOR_BUFFER_BIT | GU_DEPTH_BUFFER_BIT | GU_FAST_CLEAR_BIT);
}

//*****************************************************************************
//
//*****************************************************************************
void IGraphicsContext::BeginFrame()
{
	if(!gDoubleDisplayEnabled)
		sceGuStart(GU_DIRECT,list[listNum]);
	/*else
	{
		sceGuOffset(2048 - (SCR_WIDTH/2),2048 - (SCR_HEIGHT/2));
		sceGuViewport(2048,2048,SCR_WIDTH,SCR_HEIGHT);
		sceGuScissor(0,0,SCR_WIDTH,SCR_HEIGHT);	//Make sure we clean the whole screen
		sceGuClearDepth(0);	//Clear Zbuffer to infinity
		sceGuClear(GU_COLOR_BUFFER_BIT | GU_DEPTH_BUFFER_BIT | GU_FAST_CLEAR_BIT);	//Clear both screen & Zbuffer
		sceGuClear(GU_COLOR_BUFFER_BIT | GU_FAST_CLEAR_BIT);	//Clear screen
		sceGuClear(GU_DEPTH_BUFFER_BIT | GU_FAST_CLEAR_BIT);	//Clear Zbuffer
	}*/

	//Toggle dither matrices between frames to smooth 16bit color even further //Corn
	sceGuSetDither( listNum? &dither_matrixB : &dither_matrixA );
}

//*****************************************************************************
//
//*****************************************************************************
void IGraphicsContext::EndFrame()
{
	if(!gDoubleDisplayEnabled)
		sceGuFinish();
}

// This is used in game
//*****************************************************************************
//
//*****************************************************************************
void IGraphicsContext::UpdateFrame( bool wait_for_vbl )
{
	DAEDALUS_PROFILE( "IGraphicsContext::UpdateFrame" );

	void * p_back;

	if(gDoubleDisplayEnabled)
	{
	#ifdef DAEDALUS_ENABLE_ASSERTS
		DAEDALUS_ASSERT( sceGuFinish() < DLISTSIZE , "PSP Dlist Overflow" );
	#else
		sceGuFinish();
		//u32 num = sceGuFinish();
		//if( num > DLISTSIZE ) DLDebugger_RequestDebug();
		//printf("Dlist size %d bytes\n", num);
	#endif
	}

	sceGuSync(0,0);

	//Used for GUI menu to slow things down, in game we skip this
	if(wait_for_vbl)
	{
		sceDisplayWaitVblankStart();
	}

	if( PSP_TV_LACED )
	{
		u32 src = (u32)make_uncached_ptr((void*)LACED_DRAW);
		u32 dst = (u32)make_uncached_ptr((void*)LACED_DISP);

		sceGuStart(GU_DIRECT,ilist);
		sceGuCopyImage(SCR_MODE, 0, 0, 720, 240, 768*2, reinterpret_cast< void * >(src + 768*4), 0, 0, 768, reinterpret_cast< void * >(dst));
		sceGuTexSync();
		sceGuCopyImage(SCR_MODE, 0, 0, 720, 240, 768*2, reinterpret_cast< void * >(src), 0, 0, 768, reinterpret_cast< void * >(dst + 768*262*4));
		sceGuTexSync();
		sceGuFinish();
		sceGuSync(0,0);
		p_back = mpBuffers[ 0 ]; // back buffer always draw_buffer on interlaced
	}
	else
	{
		p_back = sceGuSwapBuffers();
	}

	if( mDumpNextScreen )
	{
		mDumpNextScreen--;
		if (!mDumpNextScreen)
		{
			if(gTakeScreenshotSS)	// We are taking a screenshot for savestate
			{
				gTakeScreenshotSS = false;
				StoreSaveScreenData();

			}
			else
			{
				DumpScreenShot();
			}
		}
	}

	mpCurrentBackBuffer = p_back;

	SetDebugScreenTarget( TS_BACKBUFFER );	//Used to print FPS and other stats

	if(gDoubleDisplayEnabled)
	{
		sceGuStart(GU_DIRECT,callList);
		sceGuCallList(list[listNum]);
		sceGuFinish(); //Display Frame
		listNum ^= 1;	//Toggle lists 0 & 1
		sceGuStart(GU_CALL,list[listNum]); //Begin other Display List
	}
	else
		listNum ^= 1;	//Toggle lists 0 & 1

	if( gCleanSceneEnabled )
	{
		sceGuScissor(0,0,SCR_WIDTH,SCR_HEIGHT);	//Make sure we clear whole screen
		ClearColBuffer( c32(0xff000000) ); // ToDo : Use gFillColor instead?
	}

	// Hack to semi-fix XG2, it uses setprimdepth for background and also does not clear zbuffer //Corn
	//
	if( g_ROM.GameHacks == EXTREME_G2 ) sceGuClear(GU_DEPTH_BUFFER_BIT | GU_FAST_CLEAR_BIT);	//Clear Zbuffer
}

//*****************************************************************************
//	Set the target for the debug screen
//*****************************************************************************
void IGraphicsContext::SetDebugScreenTarget( ETargetSurface buffer )
{
	void *	p_target;

	if( buffer == TS_BACKBUFFER )
	{
		p_target = mpCurrentBackBuffer;
	}
	else if( buffer == TS_FRONTBUFFER )
	{
		if(mpCurrentBackBuffer == mpBuffers[ 0 ])
		{
			p_target = mpBuffers[ 1 ];
		}
		else
		{
			p_target = mpBuffers[ 0 ];
		}
	}
	else
	{
		#ifdef DAEDALUS_DEBUG_CONSOLE
		DAEDALUS_ERROR( "Unknown buffer" );
		#endif
		p_target = mpCurrentBackBuffer;
	}

	pspDebugScreenSetOffset( int(p_target) & ~0x40000000 );
}

//*****************************************************************************
// Change current viewport, either for tv out or PSP itself
//
//*****************************************************************************
void IGraphicsContext::ViewportType( u32 * d_width, u32 * d_height ) const
{
	if( gGlobalPreferences.TVEnable && PSP_TV_CABLE > 0)
	{
		switch ( gGlobalPreferences.ViewportType )
		{
		case VT_UNSCALED_4_3:		// 1:1
			if ( gGlobalPreferences.TVType == TT_WIDESCREEN )
			{
				*d_width = 528;
				*d_height = 448;
			}
			else
			{
				*d_width = 640;
				*d_height = 448;
			}
			break;
		case VT_SCALED_4_3:		// Largest 4:3
			if ( gGlobalPreferences.TVType == TT_WIDESCREEN )
			{
				*d_width = 542;
				*d_height = 460;
			}
			else
			{
				*d_width = 658;
				*d_height = 460;
			}
			break;
		case VT_FULLSCREEN:		// Fullscreen
			*d_width = 720;
			*d_height = 460; // 460 seems to be the limit due to renderer conversions
			break;
		case VT_FULLSCREEN_HD:		// Fullscreen
			*d_width = 720;
			*d_height = 460; // 460 seems to be the limit due to renderer conversions
			break;
 		}
	}
	else
	{
		switch ( gGlobalPreferences.ViewportType )
		{
		case VT_UNSCALED_4_3:		// 1:1
			*d_width = 320;
			*d_height = 240;
			break;
		case VT_SCALED_4_3:		// Largest 4:3
			*d_width = 362;
			*d_height = 272;
			break;
		case VT_FULLSCREEN:		// Fullscreen
			*d_width = 480;
			*d_height = 272;
			break;
		case VT_FULLSCREEN_HD:		// Fullscreen
			*d_width = 480;
			*d_height = 272;
			break;
 		}
	}
}

//*****************************************************************************
// Save current visible screen as PNG
// From Shazz/71M - thanks guys!
//*****************************************************************************
void IGraphicsContext::SaveScreenshot( const std::filesystem::path filename, s32 x, s32 y, u32 width, u32 height )
{
	void * buffer;
	int bufferwidth;
	int pixelformat;
	int unknown = 0;

	sceDisplayWaitVblankStart();  // if framebuf was set with PSP_DISPLAY_SETBUF_NEXTFRAME, wait until it is changed
	sceDisplayGetFrameBuf( &buffer, &bufferwidth, &pixelformat, unknown );

	ETextureFormat		texture_format;
	u32					bpp;

	switch( pixelformat )
	{
	case PSP_DISPLAY_PIXEL_FORMAT_565:
		texture_format = TexFmt_5650;
		bpp = 2;
		break;
	case PSP_DISPLAY_PIXEL_FORMAT_5551:
		texture_format = TexFmt_5551;
		bpp = 2;
		break;
	case PSP_DISPLAY_PIXEL_FORMAT_4444:
		texture_format = TexFmt_4444;
		bpp = 2;
		break;
	case PSP_DISPLAY_PIXEL_FORMAT_8888:
		texture_format = TexFmt_8888;
		bpp = 4;
		break;
	default:
		texture_format = TexFmt_8888;
		bpp = 4;
		break;
	}

	u32 pitch( bufferwidth * bpp );

	buffer = reinterpret_cast< u8 * >( buffer ) + (y * pitch) + (x * bpp);

	PngSaveImage( filename, buffer, nullptr, texture_format, pitch, width, height, false );
}

//*****************************************************************************
//
//*****************************************************************************
void IGraphicsContext::DumpScreenShot()
{
	std::filesystem::create_directory( "ScreenShots" );
	std::filesystem::path dumpdir = "ScreenShots";

	std::filesystem::path file_name = g_ROM.settings.GameName.c_str();


	u32 count = 0;


	std::filesystem::path unique_filename = std::format(gScreenDumpDumpPathFormat, count++);

	u32		display_width = 0;
	u32		display_height= 0;

	u32		frame_width = 480;
	u32		frame_height= 272;

	if ( PSP_TV_CABLE > 0 )	// Tv Out
	{
		frame_width = 720;
		frame_height=  480;
	}

	ViewportType( &display_width, &display_height );

#ifdef DAEDALUS_ENABLE_ASSERTS
	DAEDALUS_ASSERT( display_width != 0 && display_height != 0, "Unhandled viewport type" );
#endif
	s32		display_x( (frame_width - display_width)/2 );
	s32		display_y( (frame_height - display_height)/2 );

	SaveScreenshot( unique_filename, display_x, display_y, display_width, display_height );
}

//*****************************************************************************
//
//*****************************************************************************
void IGraphicsContext::StoreSaveScreenData()
{
	int bufferwidth;
	int pixelformat;

	void * buffer;
	char pngbuffer[128*1024];
	std::filesystem::path pngfile;

	extern std::string	gSaveStateFilename;
	pngfile /= gSaveStateFilename;
	// snprintf( pngfile, sizeof(pngfile), "%s.png", gSaveStateFilename.c_str() );	//Add .png to filename

	u32		display_width = 0;
	u32		display_height= 0;

	u32		frame_width = 480;
	u32		frame_height= 272;

	// Not supported yet for tv out
	if ( PSP_TV_CABLE > 0 )
	{
		//TODO
		//frame_width = 720;
		//frame_height=  480;
		return;
	}

	ViewportType( &display_width, &display_height );
	#ifdef DAEDALUS_ENABLE_ASSERTS
	DAEDALUS_ASSERT( display_width != 0 && display_height != 0, "Unhandled viewport type" );
	#endif
	s32	x( (frame_width - display_width)/2 );
	s32	y( (frame_height - display_height)/2 );

	sceDisplayGetFrameBuf( &buffer, &bufferwidth, &pixelformat, 0 );

	ETextureFormat		texture_format;
	u32	bpp;
	u32 pitch;

	switch( pixelformat )
	{
	case PSP_DISPLAY_PIXEL_FORMAT_565:
		{
			texture_format = TexFmt_5650;
			bpp = 2;
			pitch = bufferwidth * bpp;
			buffer = reinterpret_cast< u8 * >( buffer ) + (y * pitch) + (x * bpp);

			//copy and reduce data to 1/4
			for(u32 ys=0; ys<display_height; ys +=2)
			{
				for(u32 xs=0; xs<display_width; xs +=2)
				{
					u32 pix = *(u16*)((u32)buffer + ys * pitch + xs * bpp);
					*(u16*)((u32)pngbuffer + (ys>>1) * (display_width>>1) * bpp + (xs>>1) * bpp) = pix;
				}
			}
			display_width = display_width >> 1;
			display_height = display_height >> 1;
			pitch = display_width * bpp;
		}
		break;
	case PSP_DISPLAY_PIXEL_FORMAT_5551:
		texture_format = TexFmt_5551;
		bpp = 2;
		pitch = bufferwidth * bpp;
		buffer = reinterpret_cast< u8 * >( buffer ) + (y * pitch) + (x * bpp);
		break;
	case PSP_DISPLAY_PIXEL_FORMAT_4444:
		texture_format = TexFmt_4444;
		bpp = 2;
		pitch = bufferwidth * bpp;
		buffer = reinterpret_cast< u8 * >( buffer ) + (y * pitch) + (x * bpp);
		break;
	case PSP_DISPLAY_PIXEL_FORMAT_8888:
		{
			texture_format = TexFmt_8888;
			bpp = 4;
			pitch = bufferwidth * bpp;
			buffer = reinterpret_cast< u8 * >( buffer ) + (y * pitch) + (x * bpp);

			//copy and reduce data to 1/4
			for(u32 ys=0; ys<display_height; ys +=2)
			{
				for(u32 xs=0; xs<display_width; xs +=2)
				{
					u32 pix = *(u32*)((u32)buffer + ys * pitch + xs * bpp);
					*(u32*)((u32)pngbuffer + (ys>>1) * (display_width>>1) * bpp + (xs>>1) * bpp) = pix;
				}
			}
			display_width = display_width >> 1;
			display_height = display_height >> 1;
			pitch = (display_width) * bpp;
		}
		break;
	default:
		texture_format = TexFmt_8888;
		bpp = 4;
		pitch = bufferwidth * bpp;
		buffer = reinterpret_cast< u8 * >( buffer ) + (y * pitch) + (x * bpp);
		break;
	}

	PngSaveImage( pngfile, (void*)pngbuffer, nullptr, texture_format, pitch, display_width, display_height, false );
}

//*****************************************************************************
//
//*****************************************************************************
void IGraphicsContext::GetScreenSize(u32 * p_width, u32 * p_height) const
{
	*p_width = SCR_WIDTH;
	*p_height = SCR_HEIGHT;
}

//*****************************************************************************
//
//*****************************************************************************
namespace
{
	void *	VramAddrAsOffset( void * ptr )
	{
		return reinterpret_cast< void * >( reinterpret_cast< u32 >( ptr ) - reinterpret_cast< u32 >( sceGeEdramGetAddr() ) );
	}
}

bool IGraphicsContext::Initialise()
{
	sceGuInit();

	bool	is_videmem;		// Result is ignored

	void *	draw_buffer;
	void *	disp_buffer;
	void *	depth_buffer;

	if( g32bitColorMode )
	{
		PIXEL_SIZE = 4; /* change this if you change to another screenmode */
		SCR_MODE = GU_PSM_8888;
	}
	else
	{
		PIXEL_SIZE = 2; /* change this if you change to another screenmode */
		SCR_MODE = GU_PSM_5650;
	}

	// compute and allocate buffers for largest possible frame
	if ( PSP_TV_CABLE > 0 )
	{
		BUF_WIDTH = 768;
		SCR_WIDTH = 720;
		SCR_HEIGHT = 480;

		// Hack, Tv out doesn't work in 16bit for some reasons so we use 32bit
		//
		PIXEL_SIZE = 4;
		SCR_MODE = GU_PSM_8888;
	}

	//Alloc all buffers with one call to save alloc list overhead //Corn
	if ( PSP_TV_CABLE > 0 )
	{
		CVideoMemoryManager::Get()->Alloc( FRAME_SIZE + LACED_SIZE + DEPTH_SIZE, &draw_buffer, &is_videmem );
		disp_buffer =  (void*)((u32)draw_buffer + FRAME_SIZE);
		depth_buffer = (void*)((u32)draw_buffer + FRAME_SIZE + LACED_SIZE);
	}
	else
	{
		CVideoMemoryManager::Get()->Alloc( FRAME_SIZE + FRAME_SIZE + DEPTH_SIZE, &draw_buffer, &is_videmem );
		disp_buffer =  (void*)((u32)draw_buffer + FRAME_SIZE);
		depth_buffer = (void*)((u32)draw_buffer + FRAME_SIZE + FRAME_SIZE);
	}

	void *	draw_buffer_rel( VramAddrAsOffset( draw_buffer ) );
	void *	disp_buffer_rel( VramAddrAsOffset( disp_buffer ) );
	void *	depth_buffer_rel( VramAddrAsOffset( depth_buffer ) );
	save_draw_rel = draw_buffer_rel;
	save_disp_rel = disp_buffer_rel;
	save_depth_rel = depth_buffer_rel;

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	printf( "Alloc %d bytes for draw buffer A at %p\n", FRAME_SIZE, draw_buffer );
	printf( "Alloc %d bytes for draw buffer B at %p\n", FRAME_SIZE, disp_buffer );
	printf( "Alloc %d bytes for Z buffer at %p\n", DEPTH_SIZE, depth_buffer );
#endif

	// buffer pointers for interlaced blits
	LACED_DRAW = reinterpret_cast< u32 >(draw_buffer);
	LACED_DISP = reinterpret_cast< u32 >(disp_buffer);

	// now initialize in LCD mode
	BUF_WIDTH = 512;
	SCR_WIDTH = 480;
	SCR_HEIGHT = 272;

	if (HAVE_DVE)
		pspDveMgrSetVideoOut(0, 0, 480, 272, 1, 15, 0); // make sure LCD active
	sceDisplaySetMode(0, 480, 272);

	sceGuStart(GU_DIRECT,list[0]);

	if( g32bitColorMode )
	{
		sceGuDisable(GU_DITHER);
	}
	else
	{
		//Do some dithering to simulate more colors //Corn
		sceGuSetDither(&dither_matrixA);
		sceGuEnable(GU_DITHER);
	}

	sceGuDrawBuffer(SCR_MODE,draw_buffer_rel,BUF_WIDTH);
	sceGuDispBuffer(SCR_WIDTH,SCR_HEIGHT,disp_buffer_rel,BUF_WIDTH);
	sceGuDepthBuffer(depth_buffer_rel,BUF_WIDTH);
	sceGuOffset(2048 - (SCR_WIDTH/2),2048 - (SCR_HEIGHT/2));
	sceGuViewport(2048,2048,SCR_WIDTH,SCR_HEIGHT);
	sceGuDepthRange(65535,0);
	sceGuScissor(0,0,SCR_WIDTH,SCR_HEIGHT);
	sceGuEnable(GU_SCISSOR_TEST);
	sceGuDepthFunc(GU_GEQUAL);
	sceGuEnable(GU_DEPTH_TEST);
	sceGuFrontFace(GU_CW);
	sceGuShadeModel(GU_SMOOTH);
	sceGuEnable(GU_CULL_FACE);
	sceGuEnable(GU_TEXTURE_2D);
	sceGuEnable(GU_CLIP_PLANES);
	sceGuFinish();
	sceGuSync(0,0);

	sceDisplayWaitVblankStart();
	sceGuDisplay(GU_TRUE);

	sceDisplaySetFrameBuf(make_uncached_ptr(disp_buffer), BUF_WIDTH, SCR_MODE, PSP_DISPLAY_SETBUF_NEXTFRAME);

	mpBuffers[ 0 ] = draw_buffer_rel;
	mpBuffers[ 1 ] = disp_buffer_rel;
	mpCurrentBackBuffer= mpBuffers[ 0 ];

	// Ensure both display lists are initialised and that we are starting with the first one not to break any roms... (DoubleDisplayList() can be called without the list being initialised..)
	sceGuStart(GU_CALL,list[0]);
	sceGuFinish();
	sceGuStart(GU_CALL,list[1]);
	sceGuFinish();

	// Initialize DrawText
	CDrawText::Initialise();

	// The app is ready to go
	mInitialised = true;
    return true;
}

void IGraphicsContext::SwitchToChosenDisplay()
{
	if (PSP_TV_CABLE <= 0)
		return;

	sceGuInit();

	if ( gGlobalPreferences.TVEnable && PSP_TV_CABLE > 0)
	{
		BUF_WIDTH = 768;
		SCR_WIDTH = 720;
		SCR_HEIGHT = 480;

		PSP_TV_LACED = ( gGlobalPreferences.TVLaced || PSP_TV_CABLE == 1 ) ? 1 : 0;
	}

	if (SCR_WIDTH == 720)
	{
		if (PSP_TV_CABLE == 1)
			pspDveMgrSetVideoOut(2, 0x1d1, 720, 503, 1, 15, 0); // composite
		else
			if (PSP_TV_LACED)
				pspDveMgrSetVideoOut(0, 0x1d1, 720, 503, 1, 15, 0); // component interlaced
			else
				pspDveMgrSetVideoOut(0, 0x1d2, 720, 480, 1, 15, 0); // component progressive
	}
	else
	{
		if (HAVE_DVE)
			pspDveMgrSetVideoOut(0, 0, 480, 272, 1, 15, 0); // make sure LCD active
		sceDisplaySetMode(0, 480, 272);
	}

	sceGuStart(GU_DIRECT,ilist);
	sceGuDrawBuffer(SCR_MODE,save_draw_rel,BUF_WIDTH);
	sceGuDispBuffer(SCR_WIDTH,SCR_HEIGHT,save_disp_rel,BUF_WIDTH);
	sceGuDepthBuffer(save_depth_rel,BUF_WIDTH);
	sceGuOffset(2048 - (SCR_WIDTH/2),2048 - (SCR_HEIGHT/2));
	sceGuViewport(2048,2048,SCR_WIDTH,SCR_HEIGHT);
	sceGuDepthRange(65535,0);
	sceGuScissor(0,0,SCR_WIDTH,SCR_HEIGHT);
	sceGuEnable(GU_SCISSOR_TEST);
	sceGuDepthFunc(GU_GEQUAL);
	sceGuEnable(GU_DEPTH_TEST);
	sceGuFrontFace(GU_CW);
	sceGuShadeModel(GU_SMOOTH);
	sceGuEnable(GU_CULL_FACE);
	sceGuEnable(GU_TEXTURE_2D);
	sceGuEnable(GU_CLIP_PLANES);
	sceGuFinish();
	sceGuSync(0,0);

	sceDisplayWaitVblankStart();
	sceGuDisplay(GU_TRUE);

	void * frame_buffer = reinterpret_cast< void * >((reinterpret_cast< u32 >(save_disp_rel) + reinterpret_cast< u32 >( sceGeEdramGetAddr() )));
	frame_buffer = make_uncached_ptr(frame_buffer);
	sceDisplaySetFrameBuf(frame_buffer, BUF_WIDTH, SCR_MODE, PSP_DISPLAY_SETBUF_NEXTFRAME);

	mpBuffers[ 0 ] = save_draw_rel;
	mpBuffers[ 1 ] = save_disp_rel;
	mpCurrentBackBuffer= mpBuffers[ 0 ];

	//sceGuStart(GU_CALL,list[listNum&1]);

}

void IGraphicsContext::SwitchToLcdDisplay()
{
	if (PSP_TV_CABLE <= 0)
		return;

	sceGuInit();

	BUF_WIDTH = 512;
	SCR_WIDTH = 480;
	SCR_HEIGHT = 272;
	PSP_TV_LACED = 0;

	if (HAVE_DVE)
		pspDveMgrSetVideoOut(0, 0, 480, 272, 1, 15, 0); // make sure LCD active
	sceDisplaySetMode(0, 480, 272);

	sceGuStart(GU_DIRECT,ilist);
	sceGuDrawBuffer(SCR_MODE,save_draw_rel,BUF_WIDTH);
	sceGuDispBuffer(SCR_WIDTH,SCR_HEIGHT,save_disp_rel,BUF_WIDTH);
	sceGuDepthBuffer(save_depth_rel,BUF_WIDTH);
	sceGuOffset(2048 - (SCR_WIDTH/2),2048 - (SCR_HEIGHT/2));
	sceGuViewport(2048,2048,SCR_WIDTH,SCR_HEIGHT);
	sceGuDepthRange(65535,0);
	sceGuScissor(0,0,SCR_WIDTH,SCR_HEIGHT);
	sceGuEnable(GU_SCISSOR_TEST);
	sceGuDepthFunc(GU_GEQUAL);
	sceGuEnable(GU_DEPTH_TEST);
	sceGuFrontFace(GU_CW);
	sceGuShadeModel(GU_SMOOTH);
	sceGuEnable(GU_CULL_FACE);
	sceGuEnable(GU_TEXTURE_2D);
	sceGuEnable(GU_CLIP_PLANES);
	sceGuFinish();
	sceGuSync(0,0);

	sceDisplayWaitVblankStart();
	sceGuDisplay(GU_TRUE);

	void * frame_buffer = reinterpret_cast< void * >((reinterpret_cast< u32 >(save_disp_rel) + reinterpret_cast< u32 >( sceGeEdramGetAddr() )));
	frame_buffer = make_uncached_ptr(frame_buffer);
	sceDisplaySetFrameBuf(frame_buffer, BUF_WIDTH, SCR_MODE, PSP_DISPLAY_SETBUF_NEXTFRAME);

	sceGuStart(GU_CALL,list[listNum]);
}
