/*
Copyright (C) 2006 StrmnNrmn

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

#include "stdafx.h"
#include "RomSelectorComponent.h"
#include "UIContext.h"
#include "UIScreen.h"

#include <psptypes.h>
#include <pspkernel.h>
#include <pspctrl.h>
#include <pspdisplay.h>
#include <pspgu.h>

#include "Math/Vector2.h"
#include "SysPSP/Graphics/DrawText.h"
#include "Graphics/ColourValue.h"
#include "Graphics/NativeTexture.h"

#include "EasyMsg/easymessage.h"

#include "Core/ROM.h"
#include "Core/RomSettings.h"

#include "../../Input/InputManager.h"
#include "../../Utility/Preferences.h"

#include "Utility/IO.h"
#include "Utility/ROMFile.h"

#include "SysPSP/Utility/Buttons.h"
#include "SysPSP/Utility/PathsPSP.h"

#include "Math/MathUtil.h"

#include <string>
#include <vector>
#include <map>
#include <algorithm>

int romselmenuani = 0;
int romselmenufs = 31;
int romselmenudir = 0;
bool sortbyletter = 0;
float romseltextoffset = 0.0f;
float romseltextrepos = 0.0f;
float romseltextscale = 0.0f;
bool isnextset = 0;
char catstr[85] = " #  a  b  c  d  e  f  g  h  i  j  k  l  m  n  o  p  q  r  s  t  u  v  w  x  y  z  ? ";

namespace
{
	const char * const		gRomsDirectories[] = 
	{
		"ms0:/n64/",
		DAEDALUS_PSP_PATH( "Roms/" ),
#ifndef DAEDALUS_PUBLIC_RELEASE
		// For ease of developing with multiple source trees, common folder for roms can be placed at host1: in usbhostfs
		"host1:/",
#endif
	};

	const char		gCategoryLetters[] = "#abcdefghijklmnopqrstuvwxyz?";

	enum ECategory
	{
		C_NUMBERS = 0,
		C_A, C_B, C_C, C_D, C_E, C_F, C_G, C_H, C_I, C_J, C_K, C_L, C_M,
		C_N, C_O, C_P, C_Q, C_R, C_S, C_T, C_U, C_V, C_W, C_X, C_Y, C_Z,
		C_UNK,
		NUM_CATEGORIES,
	};

	DAEDALUS_STATIC_ASSERT( ARRAYSIZE( gCategoryLetters ) == NUM_CATEGORIES +1 );

	ECategory		GetCategory( char c )
	{
		if( isalpha( c ) )
		{
			c = tolower( c );
			return ECategory( C_A + (c - 'a') );
		}
		else if( c >= '0' && c <= '9' )
		{
			return C_NUMBERS;
		}
		else
		{
			return C_UNK;
		}
	}

	char	GetCategoryLetter( ECategory category )
	{
		DAEDALUS_ASSERT( category >= 0 && category < NUM_CATEGORIES, "Invalid category" );
		return gCategoryLetters[ category ];
	}

	u32	ICON_AREA_TOP;
	u32	ICON_AREA_LEFT;
	u32	ICON_AREA_WIDTH;
	u32	ICON_AREA_HEIGHT;

	u32	TEXT_AREA_TOP;
	u32	TEXT_AREA_LEFT;
	u32	TEXT_AREA_WIDTH;
	u32	TEXT_AREA_HEIGHT;

	u32	CATEGORY_AREA_TOP;
	u32	CATEGORY_AREA_LEFT;

	f32	PREVIEW_SCROLL_WAIT;	// seconds to wait for scrolling to stop before loading preview (prevent thrashing)
	f32	PREVIEW_FADE_TIME;		// seconds

	const char * const		gNoRomsText[] =
	{
		"Daedalus could not find any roms to load.",
		"You can add roms to the \\N64\\ directory on your memory stick,",
		"(e.g. P:\\N64\\)",
		"or the Roms directory within the Daedalus folder.",
		"(e.g. P:\\PSP\\GAME\\Daedalus\\Roms\\)",
		"Daedalus recognises a number of different filetypes,",
		"including .zip, .z64, .v64, .rom, .bin, .pal, .usa and .jap.",
	};

	const char * const		gPreviewDirectory = DAEDALUS_PSP_PATH( "Resources/Preview/" );
}

//*************************************************************************************
//
//*************************************************************************************
struct SRomInfo
{
	CFixedString<100>		mFilename;

	RomID			mRomID;
	u32				mRomSize;
	ECicType		mCicType;

	RomSettings		mSettings;

	SRomInfo( const char * filename )
		:	mFilename( filename )
	{
		if ( ROM_GetRomDetailsByFilename( filename, &mRomID, &mRomSize, &mCicType ) )
		{
			if ( !CRomSettingsDB::Get()->GetSettings( mRomID, &mSettings ) )
			{
				// Create new entry, add
				mSettings.Reset();
				mSettings.Comment = "Unknown";

				//
				// We want to get the "internal" name for this rom from the header
				// Failing that, use the filename
				//
				std::string game_name;
				if ( !ROM_GetRomName( filename, game_name ) )
				{
					game_name = IO::Path::FindFileName( filename );
				}
				game_name = game_name.substr(0, 63);
				mSettings.GameName = game_name.c_str();
				CRomSettingsDB::Get()->SetSettings( mRomID, mSettings );
			}
		}
		else
		{
			mSettings.GameName = "Can't get rom info";
		}

	}
};

//*************************************************************************************
//
//*************************************************************************************
static ECategory Categorise( const char * name )
{
	char	c( name[ 0 ] );
	return GetCategory( c );
}

static bool SortByGameName( const SRomInfo * a, const SRomInfo * b )
{
	// Sort by the category first, then on the actual string.
	ECategory	cat_a( Categorise( a->mSettings.GameName.c_str() ) );
	ECategory	cat_b( Categorise( b->mSettings.GameName.c_str() ) );

	if( cat_a != cat_b )
	{
		return cat_a < cat_b;
	}

	return ( strcmp( a->mSettings.GameName.c_str(), b->mSettings.GameName.c_str() ) < 0 );
}

//*************************************************************************************
//
//*************************************************************************************
//Lifting this out makes it remmember last choosen ROM
//Could probably be fixed better but C++ is giving me an attitude //Corn
static u32 mCurrentSelection = 0;

class IRomSelectorComponent : public CRomSelectorComponent
{
		typedef std::vector<SRomInfo*>	RomInfoList;
		typedef std::map< ECategory, u32 >	AlphaMap;
	public:

		IRomSelectorComponent( CUIContext * p_context, CFunctor1< const char * > * on_rom_selected );
		~IRomSelectorComponent();

		// CUIComponent
		virtual void				Update( float elapsed_time, const v2 & stick, u32 old_buttons, u32 new_buttons );
		virtual void				Render();

	private:
				void				UpdateROMList();
				void				Update_old( float elapsed_time, const v2 & stick, u32 old_buttons, u32 new_buttons );
				void				Render_old();
				void				RenderPreview();
				void				RenderPreview_old();
				void				RenderRomList();
				void				RenderRomList_old();
				void				RenderCategoryList();
				void				RenderCategoryList_old();

				void				AddRomDirectory(const char * p_roms_dir, RomInfoList & roms);

				ECategory			GetCurrentCategory() const;

				void				DrawInfoText( CUIContext * p_context, s32 y, const char * field_txt, const char * value_txt );
				void				DrawInfoText_old( CUIContext * p_context, s32 y, const char * field_txt, const char * value_txt );

	private:
		CFunctor1< const char * > *	OnRomSelected; 
		RomInfoList					mRomsList;
		AlphaMap					mRomCategoryMap;
		s32							mCurrentScrollOffset;
		float						mSelectionAccumulator;
		std::string					mSelectedRom;

		bool						mDisplayFilenames;
		bool						mDisplayInfo;

		CRefPtr<CNativeTexture>		mpPreviewTexture;
		u32							mPreviewIdx;
		float						mPreviewLoadedTime;		// How long the preview has been loaded (so we can fade in)
		float						mTimeSinceScroll;		// 

		bool						mRomDelete;
};

//*************************************************************************************
//
//*************************************************************************************
CRomSelectorComponent::CRomSelectorComponent( CUIContext * p_context )
:	CUIComponent( p_context )
{
}

//*************************************************************************************
//
//*************************************************************************************
CRomSelectorComponent::~CRomSelectorComponent()
{
}

//*************************************************************************************
//
//*************************************************************************************
CRomSelectorComponent *	CRomSelectorComponent::Create( CUIContext * p_context, CFunctor1< const char * > * on_rom_selected )
{
	return new IRomSelectorComponent( p_context, on_rom_selected );
}

//*************************************************************************************
//
//*************************************************************************************
IRomSelectorComponent::IRomSelectorComponent( CUIContext * p_context, CFunctor1< const char * > * on_rom_selected )
:	CRomSelectorComponent( p_context )
,	OnRomSelected( on_rom_selected )
//,	mCurrentSelection( 0 )
,	mCurrentScrollOffset( 0 )
,	mSelectionAccumulator( 0 )
,	mpPreviewTexture( NULL )
,	mPreviewIdx( u32(-1) )
,	mPreviewLoadedTime( 0.0f )
,	mTimeSinceScroll( 0.0f )
,	mRomDelete(false)
{
	for( u32 i = 0; i < ARRAYSIZE( gRomsDirectories ); ++i )
	{
		AddRomDirectory( gRomsDirectories[ i ], mRomsList );
	}

	stable_sort( mRomsList.begin(), mRomsList.end(), SortByGameName );

	// Build up a map of the first location for each initial letter
	for( u32 i = 0; i < mRomsList.size(); ++i )
	{
		const char *	p_gamename( mRomsList[ i ]->mSettings.GameName.c_str() );
		ECategory		category( Categorise( p_gamename ) );

		if( mRomCategoryMap.find( category ) == mRomCategoryMap.end() )
		{
			mRomCategoryMap[ category ] = i;
		}
	}
}

//*************************************************************************************
//
//*************************************************************************************
IRomSelectorComponent::~IRomSelectorComponent()
{
	for(RomInfoList::iterator it = mRomsList.begin(); it != mRomsList.end(); ++it)
	{
		SRomInfo *	p_rominfo( *it );

		delete p_rominfo;
	}
	mRomsList.clear();

	delete OnRomSelected;
}

//*************************************************************************************
//Refresh ROM list //Corn
//*************************************************************************************
void	IRomSelectorComponent::UpdateROMList()
{
	for(RomInfoList::iterator it = mRomsList.begin(); it != mRomsList.end(); ++it)
	{
		SRomInfo *	p_rominfo( *it );

		delete p_rominfo;
	}
	mRomsList.clear();

	mCurrentScrollOffset = 0;
	mSelectionAccumulator = 0;
	mpPreviewTexture = NULL;
	mPreviewIdx= u32(-1);

	for( u32 i = 0; i < ARRAYSIZE( gRomsDirectories ); ++i )
	{
		AddRomDirectory( gRomsDirectories[ i ], mRomsList );
	}

	stable_sort( mRomsList.begin(), mRomsList.end(), SortByGameName );

	// Build up a map of the first location for each initial letter
	for( u32 i = 0; i < mRomsList.size(); ++i )
	{
		const char *	p_gamename( mRomsList[ i ]->mSettings.GameName.c_str() );
		ECategory		category( Categorise( p_gamename ) );

		if( mRomCategoryMap.find( category ) == mRomCategoryMap.end() )
		{
			mRomCategoryMap[ category ] = i;
		}
	}
}

//*************************************************************************************
//
//*************************************************************************************
void	IRomSelectorComponent::AddRomDirectory(const char * p_roms_dir, RomInfoList & roms)
{
	std::string			full_path;

	IO::FindHandleT		find_handle;
	IO::FindDataT		find_data;
	if(IO::FindFileOpen( p_roms_dir, &find_handle, find_data ))
	{
		do
		{
			const char * rom_filename( find_data.Name );
			if(IsRomfilename( rom_filename ))
			{
				full_path = p_roms_dir;
				full_path += rom_filename;

				SRomInfo *	p_rom_info = new SRomInfo( full_path.c_str() );

				roms.push_back( p_rom_info );
			}
		}
		while(IO::FindFileNext( find_handle, find_data ));

		IO::FindFileClose( find_handle );
	}
}

//*************************************************************************************
//
//*************************************************************************************
ECategory	IRomSelectorComponent::GetCurrentCategory() const
{
	if( !mRomsList.empty() )
	{
		return Categorise( mRomsList[ mCurrentSelection ]->mSettings.GameName.c_str() );
	}

	return C_NUMBERS;
}

//*************************************************************************************
//
//*************************************************************************************
void IRomSelectorComponent::DrawInfoText_old(  CUIContext * p_context, s32 y, const char * field_txt, const char * value_txt  )
{
	c32			colour(	p_context->GetDefaultTextColour() );

	p_context->DrawTextAlign( ICON_AREA_LEFT, ICON_AREA_LEFT + ICON_AREA_WIDTH, AT_LEFT, y, field_txt, colour );
	p_context->DrawTextAlign( ICON_AREA_LEFT, ICON_AREA_LEFT + ICON_AREA_WIDTH, AT_RIGHT, y, value_txt, colour );
}

//*************************************************************************************
//
//*************************************************************************************
void IRomSelectorComponent::DrawInfoText(  CUIContext * p_context, s32 y, const char * field_txt, const char * value_txt  )
{
	c32			colour(	p_context->GetDefaultTextColour() );

	p_context->DrawTextAlign( TEXT_AREA_LEFT, TEXT_AREA_LEFT + TEXT_AREA_WIDTH, AT_LEFT, y, field_txt, colour );
	p_context->DrawTextAlign( TEXT_AREA_LEFT, TEXT_AREA_LEFT + TEXT_AREA_WIDTH, AT_RIGHT, y, value_txt, colour );
}

//*************************************************************************************
//
//*************************************************************************************
void IRomSelectorComponent::RenderPreview_old()
{
	c32	clrYELLOW = c32( 255, 255, 0, 0 );

	//mpContext->DrawRect( ICON_AREA_LEFT-2, ICON_AREA_TOP-2, ICON_AREA_WIDTH+4, ICON_AREA_HEIGHT+4, c32::White );
	//mpContext->DrawRect( ICON_AREA_LEFT-1, ICON_AREA_TOP-1, ICON_AREA_WIDTH+2, ICON_AREA_HEIGHT+2, mpContext->GetBackgroundColour() );

	v2	tl( ICON_AREA_LEFT, ICON_AREA_TOP );
	v2	wh( ICON_AREA_WIDTH, ICON_AREA_HEIGHT );

	if( mpPreviewTexture != NULL )
	{
		c32		colour( c32::White );

		if ( mPreviewLoadedTime < PREVIEW_FADE_TIME )
		{
			colour = c32( 255, 255, 255, u8( mPreviewLoadedTime * 255.f / PREVIEW_FADE_TIME ) );
		}

		mpContext->DrawRect( ICON_AREA_LEFT, ICON_AREA_TOP, ICON_AREA_WIDTH, ICON_AREA_HEIGHT, c32::Black );
		mpContext->RenderTexture( mpPreviewTexture, tl, wh, colour );
	}
	else
	{
		//mpContext->DrawRect( ICON_AREA_LEFT, ICON_AREA_TOP, ICON_AREA_WIDTH, ICON_AREA_HEIGHT, c32::Black );
		mpContext->DrawRect( ICON_AREA_LEFT, ICON_AREA_TOP, ICON_AREA_WIDTH, ICON_AREA_HEIGHT, c32::White );
		mpContext->DrawRect( ICON_AREA_LEFT+2, ICON_AREA_TOP+2, ICON_AREA_WIDTH-4, ICON_AREA_HEIGHT-4, c32::Black );
		mpContext->DrawTextAlign( ICON_AREA_LEFT, ICON_AREA_LEFT + ICON_AREA_WIDTH, AT_CENTRE, ICON_AREA_TOP+ICON_AREA_HEIGHT/2, "No Preview Available", c32::White );
	}


	u32		font_height( mpContext->GetFontHeight() );
	u32		line_height( font_height + 1 );

	s32 y = ICON_AREA_TOP + ICON_AREA_HEIGHT + 1 + font_height;

	if( mCurrentSelection < mRomsList.size() )
	{
		SRomInfo *	p_rominfo( mRomsList[ mCurrentSelection ] );

		const char *	cic_name( ROM_GetCicName( p_rominfo->mCicType ) );
		const char *	country( ROM_GetCountryNameFromID( p_rominfo->mRomID.CountryID ) );
		u32				rom_size( p_rominfo->mRomSize );

		char buffer[ 32 ];
		sprintf( buffer, "%d MB", rom_size / (1024*1024) );

		DrawInfoText_old( mpContext, y, "Boot:", cic_name );	y += line_height;
		DrawInfoText_old( mpContext, y, "Country:", country );	y += line_height;
		DrawInfoText_old( mpContext, y, "Size:", buffer );	y += line_height;

		DrawInfoText_old( mpContext, y, "Save:", ROM_GetSaveTypeName( p_rominfo->mSettings.SaveType ) ); y += line_height;
		DrawInfoText_old( mpContext, y, "EPak:", ROM_GetExpansionPakUsageName( p_rominfo->mSettings.ExpansionPakUsage ) ); y += line_height;
		//DrawInfoText_old( mpContext, y, "Dynarec:", p_rominfo->mSettings.DynarecSupported ? "Supported" : "Unsupported" ); y += line_height;
	}
	else
	{
		DrawInfoText_old( mpContext, y, "Boot:", "" );		y += line_height;
		DrawInfoText_old( mpContext, y, "Country:", "" );	y += line_height;
		DrawInfoText_old( mpContext, y, "Size:", "" );		y += line_height;

		DrawInfoText_old( mpContext, y, "Save:", "" );		y += line_height;
		DrawInfoText_old( mpContext, y, "EPak:", "" );		y += line_height;
		//DrawInfoText_old( mpContext, y, "Dynarec:", "" );	y += line_height;
	}
	
	if (mDisplayInfo)
	{
		mpContext->DrawRect( ICON_AREA_LEFT, ICON_AREA_TOP, ICON_AREA_WIDTH + 1, TEXT_AREA_HEIGHT + 3, c32::Black ); 

		SRomInfo *	p_rominfo( mRomsList[ mCurrentSelection ] );

		s32 y = ICON_AREA_TOP + font_height;

		if (( p_rominfo->mSettings.Comment[0] != '0' ) &&( p_rominfo->mSettings.Comment[0] != '1' ) && ( p_rominfo->mSettings.Comment[0] != '2' ) && ( p_rominfo->mSettings.Comment[0] != '3' ) && ( p_rominfo->mSettings.Comment[0] != '4' ) && ( p_rominfo->mSettings.Comment[0] != '5' ))
		{
			DrawInfoText_old( mpContext, y, "    Compatibility Info", "" ); y += line_height + 1;				
			DrawInfoText_old( mpContext, y, "       Not Available", "" ); y += line_height + 1;
		}
		else
		{
			if ( p_rominfo->mSettings.Comment[0] == '0' ) 
			{
				DrawInfoText_old( mpContext, y, "Compatibility:", "" );
				mpContext->DrawRect( ICON_AREA_LEFT + ICON_AREA_WIDTH - 10, y - 10, 10, 10, c32::Grey ); y += line_height + 1;
			}	
			else if ( p_rominfo->mSettings.Comment[0] == '1' )
			{
				DrawInfoText_old( mpContext, y, "Compatibility:", "" );
				mpContext->DrawRect( ICON_AREA_LEFT + ICON_AREA_WIDTH - 10, y - 10, 10, 10, c32::Red ); y += line_height + 1;
			}	
			else if ( p_rominfo->mSettings.Comment[0] == '2' )
			{
				DrawInfoText_old( mpContext, y, "Compatibility:", "" );
				mpContext->DrawRect( ICON_AREA_LEFT + ICON_AREA_WIDTH - 10, y - 10, 10, 10, c32::Orange ); y += line_height + 1;
			}	
			else if ( p_rominfo->mSettings.Comment[0] == '3' )
			{
				DrawInfoText_old( mpContext, y, "Compatibility:", "" );
				mpContext->DrawRect( ICON_AREA_LEFT + ICON_AREA_WIDTH - 10, y - 10, 10, 10, clrYELLOW ); y += line_height + 1;
			}	
			else if ( p_rominfo->mSettings.Comment[0] == '4' ) 
			{
				DrawInfoText_old( mpContext, y, "Compatibility:", "" );
				mpContext->DrawRect( ICON_AREA_LEFT + ICON_AREA_WIDTH - 10, y - 10, 10, 10, c32::Green ); y += line_height + 1;
			}	
			else if ( p_rominfo->mSettings.Comment[0] == '5' )
			{
				DrawInfoText_old( mpContext, y, "Compatibility:", "" );
				mpContext->DrawRect( ICON_AREA_LEFT + ICON_AREA_WIDTH - 10, y - 10, 10, 10, c32::Blue ); y += line_height + 1;
			}				
				//DrawInfoText_old( mpContext, y, "Hold     for more info.", "" );				
				//mpContext->DrawRect( TEXT_AREA_LEFT + 36, y - 8, 7, 7, c32::White );
				//mpContext->DrawRect( TEXT_AREA_LEFT + 37, y - 7, 5, 5, c32::Black ); y += line_height + 5;

			if ( p_rominfo->mSettings.Comment[1] != '0' ) {
				mpContext->DrawTextAlign( ICON_AREA_LEFT, ICON_AREA_LEFT + ICON_AREA_WIDTH, AT_LEFT, y, "Texture Update:", c32::White );

				if ( p_rominfo->mSettings.Comment[1] == '1' ) {
					mpContext->DrawTextAlign( ICON_AREA_LEFT, ICON_AREA_LEFT + ICON_AREA_WIDTH, AT_RIGHT, y, "Every Frame", c32::White );  y += line_height + 1;
				}
				else if ( p_rominfo->mSettings.Comment[1] == '2' ) {
					mpContext->DrawTextAlign( ICON_AREA_LEFT, ICON_AREA_LEFT + ICON_AREA_WIDTH, AT_RIGHT, y, "Every 2 Frames", c32::White );  y += line_height + 1;
				}
				else if ( p_rominfo->mSettings.Comment[1] == '3' ) {
					mpContext->DrawTextAlign( ICON_AREA_LEFT, ICON_AREA_LEFT + ICON_AREA_WIDTH, AT_RIGHT, y, "Every 3 Frames", c32::White );  y += line_height + 1;
				}
				else if ( p_rominfo->mSettings.Comment[1] == '4' ) {
					mpContext->DrawTextAlign( ICON_AREA_LEFT, ICON_AREA_LEFT + ICON_AREA_WIDTH, AT_RIGHT, y, "Every 4 Frames", c32::White );  y += line_height + 1;
				}
				else if ( p_rominfo->mSettings.Comment[1] == '5' ) {
					mpContext->DrawTextAlign( ICON_AREA_LEFT, ICON_AREA_LEFT + ICON_AREA_WIDTH, AT_RIGHT, y, "Every 5 Frames", c32::White );  y += line_height + 1;
				}
				else if ( p_rominfo->mSettings.Comment[1] == '6' ) {
					mpContext->DrawTextAlign( ICON_AREA_LEFT, ICON_AREA_LEFT + ICON_AREA_WIDTH, AT_RIGHT, y, "Every 10 Frames", c32::White );  y += line_height + 1;
				}
				else if ( p_rominfo->mSettings.Comment[1] == '7' ) {
					mpContext->DrawTextAlign( ICON_AREA_LEFT, ICON_AREA_LEFT + ICON_AREA_WIDTH, AT_RIGHT, y, "Every 15 Frames", c32::White );  y += line_height + 1;
				}
				else if ( p_rominfo->mSettings.Comment[1] == '8' ) {
					mpContext->DrawTextAlign( ICON_AREA_LEFT, ICON_AREA_LEFT + ICON_AREA_WIDTH, AT_RIGHT, y, "Every 20 Frames", c32::White );  y += line_height + 1;
				}
				else if ( p_rominfo->mSettings.Comment[1] == '9' ) {
					mpContext->DrawTextAlign( ICON_AREA_LEFT, ICON_AREA_LEFT + ICON_AREA_WIDTH, AT_RIGHT, y, "Every 30 Frames", c32::White );  y += line_height + 1;
				}
			}

			if ( p_rominfo->mSettings.Comment[2] != '0' ) {
				mpContext->DrawTextAlign( ICON_AREA_LEFT, ICON_AREA_LEFT + ICON_AREA_WIDTH, AT_LEFT, y, "FrameSkip:", c32::White );

				if ( p_rominfo->mSettings.Comment[2] == '1' ) {
					mpContext->DrawTextAlign( ICON_AREA_LEFT, ICON_AREA_LEFT + ICON_AREA_WIDTH, AT_RIGHT, y, "Auto 1", c32::White );  y += line_height + 1;
				}						
				else if ( p_rominfo->mSettings.Comment[2] == '2' ) {
					mpContext->DrawTextAlign( ICON_AREA_LEFT, ICON_AREA_LEFT + ICON_AREA_WIDTH, AT_RIGHT, y, "Auto 2", c32::White );  y += line_height + 1;
				}
				else if ( p_rominfo->mSettings.Comment[2] == '3' ) {
					mpContext->DrawTextAlign( ICON_AREA_LEFT, ICON_AREA_LEFT + ICON_AREA_WIDTH, AT_RIGHT, y, "1", c32::White );  y += line_height + 1;
				}
				else if ( p_rominfo->mSettings.Comment[2] == '4' ) {
					mpContext->DrawTextAlign( ICON_AREA_LEFT, ICON_AREA_LEFT + ICON_AREA_WIDTH, AT_RIGHT, y, "2", c32::White );  y += line_height + 1;
				}
				else if ( p_rominfo->mSettings.Comment[2] == '5' ) {
					mpContext->DrawTextAlign( ICON_AREA_LEFT, ICON_AREA_LEFT + ICON_AREA_WIDTH, AT_RIGHT, y, "3", c32::White );  y += line_height + 1;
				}
				else if ( p_rominfo->mSettings.Comment[2] == '6' ) {
					mpContext->DrawTextAlign( ICON_AREA_LEFT, ICON_AREA_LEFT + ICON_AREA_WIDTH, AT_RIGHT, y, "4", c32::White );  y += line_height + 1;
				}
				else if ( p_rominfo->mSettings.Comment[2] == '7' ) {
					mpContext->DrawTextAlign( ICON_AREA_LEFT, ICON_AREA_LEFT + ICON_AREA_WIDTH, AT_RIGHT, y, "5", c32::White );  y += line_height + 1;
				}
			}

			if ( p_rominfo->mSettings.Comment[3] != '0' ) {
				mpContext->DrawTextAlign( ICON_AREA_LEFT, ICON_AREA_LEFT + ICON_AREA_WIDTH, AT_LEFT, y, "Zoom:", c32::White );

				if ( p_rominfo->mSettings.Comment[3] == 'a' ) {
					mpContext->DrawTextAlign( ICON_AREA_LEFT, ICON_AREA_LEFT + ICON_AREA_WIDTH, AT_RIGHT, y, "101%", c32::White );  y += line_height + 1;
				}
				else if ( p_rominfo->mSettings.Comment[3] == 'b' ) {
					mpContext->DrawTextAlign( ICON_AREA_LEFT, ICON_AREA_LEFT + ICON_AREA_WIDTH, AT_RIGHT, y, "102%", c32::White );  y += line_height + 1;
				}
				else if ( p_rominfo->mSettings.Comment[3] == 'c' ) {
					mpContext->DrawTextAlign( ICON_AREA_LEFT, ICON_AREA_LEFT + ICON_AREA_WIDTH, AT_RIGHT, y, "103%", c32::White );  y += line_height + 1;
				}
				else if ( p_rominfo->mSettings.Comment[3] == 'd' ) {
					mpContext->DrawTextAlign( ICON_AREA_LEFT, ICON_AREA_LEFT + ICON_AREA_WIDTH, AT_RIGHT, y, "104%", c32::White );  y += line_height + 1;
				}
				else if ( p_rominfo->mSettings.Comment[3] == 'e' ) {
					mpContext->DrawTextAlign( ICON_AREA_LEFT, ICON_AREA_LEFT + ICON_AREA_WIDTH, AT_RIGHT, y, "105%", c32::White );  y += line_height + 1;
				}
				else if ( p_rominfo->mSettings.Comment[3] == 'f' ) {
					mpContext->DrawTextAlign( ICON_AREA_LEFT, ICON_AREA_LEFT + ICON_AREA_WIDTH, AT_RIGHT, y, "106%", c32::White );  y += line_height + 1;
				}
				else if ( p_rominfo->mSettings.Comment[3] == 'g' ) {
					mpContext->DrawTextAlign( ICON_AREA_LEFT, ICON_AREA_LEFT + ICON_AREA_WIDTH, AT_RIGHT, y, "107%", c32::White );  y += line_height + 1;
				}
				else if ( p_rominfo->mSettings.Comment[3] == 'h' ) {
					mpContext->DrawTextAlign( ICON_AREA_LEFT, ICON_AREA_LEFT + ICON_AREA_WIDTH, AT_RIGHT, y, "108%", c32::White );  y += line_height + 1;
				}
				else if ( p_rominfo->mSettings.Comment[3] == 'i' ) {
					mpContext->DrawTextAlign( ICON_AREA_LEFT, ICON_AREA_LEFT + ICON_AREA_WIDTH, AT_RIGHT, y, "109%", c32::White );  y += line_height + 1;
				}
				else if ( p_rominfo->mSettings.Comment[3] == 'j' ) {
					mpContext->DrawTextAlign( ICON_AREA_LEFT, ICON_AREA_LEFT + ICON_AREA_WIDTH, AT_RIGHT, y, "110%", c32::White );  y += line_height + 1;
				}
				else if ( p_rominfo->mSettings.Comment[3] == 'k' ) {
					mpContext->DrawTextAlign( ICON_AREA_LEFT, ICON_AREA_LEFT + ICON_AREA_WIDTH, AT_RIGHT, y, "111%", c32::White );  y += line_height + 1;
				}
				else if ( p_rominfo->mSettings.Comment[3] == 'l' ) {
					mpContext->DrawTextAlign( ICON_AREA_LEFT, ICON_AREA_LEFT + ICON_AREA_WIDTH, AT_RIGHT, y, "112%", c32::White );  y += line_height + 1;
				}
				else if ( p_rominfo->mSettings.Comment[3] == 'm' ) {
					mpContext->DrawTextAlign( ICON_AREA_LEFT, ICON_AREA_LEFT + ICON_AREA_WIDTH, AT_RIGHT, y, "113%", c32::White );  y += line_height + 1;
				}
				else if ( p_rominfo->mSettings.Comment[3] == 'n' ) {
					mpContext->DrawTextAlign( ICON_AREA_LEFT, ICON_AREA_LEFT + ICON_AREA_WIDTH, AT_RIGHT, y, "114%", c32::White );  y += line_height + 1;
				}
				else if ( p_rominfo->mSettings.Comment[3] == 'o' ) {
					mpContext->DrawTextAlign( ICON_AREA_LEFT, ICON_AREA_LEFT + ICON_AREA_WIDTH, AT_RIGHT, y, "115%", c32::White );  y += line_height + 1;
				}
				else if ( p_rominfo->mSettings.Comment[3] == 'p' ) {
					mpContext->DrawTextAlign( ICON_AREA_LEFT, ICON_AREA_LEFT + ICON_AREA_WIDTH, AT_RIGHT, y, "116%", c32::White );  y += line_height + 1;
				}
				else if ( p_rominfo->mSettings.Comment[3] == 'q' ) {
					mpContext->DrawTextAlign( ICON_AREA_LEFT, ICON_AREA_LEFT + ICON_AREA_WIDTH, AT_RIGHT, y, "117%", c32::White );  y += line_height + 1;
				}
				else if ( p_rominfo->mSettings.Comment[3] == 'r' ) {
					mpContext->DrawTextAlign( ICON_AREA_LEFT, ICON_AREA_LEFT + ICON_AREA_WIDTH, AT_RIGHT, y, "118%", c32::White );  y += line_height + 1;
				}
				else if ( p_rominfo->mSettings.Comment[3] == 's' ) {
					mpContext->DrawTextAlign( ICON_AREA_LEFT, ICON_AREA_LEFT + ICON_AREA_WIDTH, AT_RIGHT, y, "119%", c32::White );  y += line_height + 1;
				}						
				else if ( p_rominfo->mSettings.Comment[3] == 't' ) {
					mpContext->DrawTextAlign( ICON_AREA_LEFT, ICON_AREA_LEFT + ICON_AREA_WIDTH, AT_RIGHT, y, "120%", c32::White );  y += line_height + 1;
				}
				else if ( p_rominfo->mSettings.Comment[3] == 'u' ) {
					mpContext->DrawTextAlign( ICON_AREA_LEFT, ICON_AREA_LEFT + ICON_AREA_WIDTH, AT_RIGHT, y, "121%", c32::White );  y += line_height + 1;
				}						
				else if ( p_rominfo->mSettings.Comment[3] == 'v' ) {
					mpContext->DrawTextAlign( ICON_AREA_LEFT, ICON_AREA_LEFT + ICON_AREA_WIDTH, AT_RIGHT, y, "122%", c32::White );  y += line_height + 1;
				}
				else if ( p_rominfo->mSettings.Comment[3] == 'w' ) {
					mpContext->DrawTextAlign( ICON_AREA_LEFT, ICON_AREA_LEFT + ICON_AREA_WIDTH, AT_RIGHT, y, "123%", c32::White );  y += line_height + 1;
				}
				else if ( p_rominfo->mSettings.Comment[3] == 'x' ) {
					mpContext->DrawTextAlign( ICON_AREA_LEFT, ICON_AREA_LEFT + ICON_AREA_WIDTH, AT_RIGHT, y, "124%", c32::White );  y += line_height + 1;
				}
				else if ( p_rominfo->mSettings.Comment[3] == 'y' ) {
					mpContext->DrawTextAlign( ICON_AREA_LEFT, ICON_AREA_LEFT + ICON_AREA_WIDTH, AT_RIGHT, y, "125%", c32::White );  y += line_height + 1;
				}
			}
			
			if ( p_rominfo->mSettings.Comment[4] != '0' ) {
				mpContext->DrawTextAlign( ICON_AREA_LEFT, ICON_AREA_LEFT + ICON_AREA_WIDTH, AT_LEFT, y, "Limit Framerate:", c32::White );

				if ( p_rominfo->mSettings.Comment[4] == '1' ) {
					mpContext->DrawTextAlign( ICON_AREA_LEFT, ICON_AREA_LEFT + ICON_AREA_WIDTH, AT_RIGHT, y, "Yes", c32::White );  y += line_height + 1;
				}						
				else if ( p_rominfo->mSettings.Comment[4] == '2' ) {
					mpContext->DrawTextAlign( ICON_AREA_LEFT, ICON_AREA_LEFT + ICON_AREA_WIDTH, AT_RIGHT, y, "No", c32::White );  y += line_height + 1;
				}
			}

			if ( p_rominfo->mSettings.Comment[5] != '0' ) {
				mpContext->DrawTextAlign( ICON_AREA_LEFT, ICON_AREA_LEFT + ICON_AREA_WIDTH, AT_LEFT, y, "Dynamic Recomp:", c32::White );

				if ( p_rominfo->mSettings.Comment[5] == '1' ) {
					mpContext->DrawTextAlign( ICON_AREA_LEFT, ICON_AREA_LEFT + ICON_AREA_WIDTH, AT_RIGHT, y, "Enabled", c32::White );  y += line_height + 1;
				}						
				else if ( p_rominfo->mSettings.Comment[5] == '2' ) {
					mpContext->DrawTextAlign( ICON_AREA_LEFT, ICON_AREA_LEFT + ICON_AREA_WIDTH, AT_RIGHT, y, "Disabled", c32::White );  y += line_height + 1;
				}
			}

			if ( p_rominfo->mSettings.Comment[6] != '0' ) {
				mpContext->DrawTextAlign( ICON_AREA_LEFT, ICON_AREA_LEFT + ICON_AREA_WIDTH, AT_LEFT, y, "Dyn Stack Opt:", c32::White );

				if ( p_rominfo->mSettings.Comment[6] == '1' ) {
					mpContext->DrawTextAlign( ICON_AREA_LEFT, ICON_AREA_LEFT + ICON_AREA_WIDTH, AT_RIGHT, y, "Enabled", c32::White );  y += line_height + 1;
				}						
				else if ( p_rominfo->mSettings.Comment[6] == '2' ) {
					mpContext->DrawTextAlign( ICON_AREA_LEFT, ICON_AREA_LEFT + ICON_AREA_WIDTH, AT_RIGHT, y, "Disabled", c32::White );  y += line_height + 1;
				}
			}

			if ( p_rominfo->mSettings.Comment[7] != '0' ) {
				mpContext->DrawTextAlign( ICON_AREA_LEFT, ICON_AREA_LEFT + ICON_AREA_WIDTH, AT_LEFT, y, "Hi Level Emu:", c32::White );

				if ( p_rominfo->mSettings.Comment[7] == '1' ) {
					mpContext->DrawTextAlign( ICON_AREA_LEFT, ICON_AREA_LEFT + ICON_AREA_WIDTH, AT_RIGHT, y, "Enabled", c32::White );  y += line_height + 1;
				}						
				else if ( p_rominfo->mSettings.Comment[7] == '2' ) {
					mpContext->DrawTextAlign( ICON_AREA_LEFT, ICON_AREA_LEFT + ICON_AREA_WIDTH, AT_RIGHT, y, "Disabled", c32::White );  y += line_height + 1;
				}
			}
			
			if ( p_rominfo->mSettings.Comment[8] != '0' ) {
				mpContext->DrawTextAlign( ICON_AREA_LEFT, ICON_AREA_LEFT + ICON_AREA_WIDTH, AT_LEFT, y, "Memory Access Opt:", c32::White );

				if ( p_rominfo->mSettings.Comment[8] == '1' ) {
					mpContext->DrawTextAlign( ICON_AREA_LEFT, ICON_AREA_LEFT + ICON_AREA_WIDTH, AT_RIGHT, y, "Enabled", c32::White );  y += line_height + 1;
				}						
				else if ( p_rominfo->mSettings.Comment[8] == '2' ) {
					mpContext->DrawTextAlign( ICON_AREA_LEFT, ICON_AREA_LEFT + ICON_AREA_WIDTH, AT_RIGHT, y, "Disabled", c32::White );  y += line_height + 1;
				}
			}

			if ( p_rominfo->mSettings.Comment[9] != '0' ) {
				mpContext->DrawTextAlign( ICON_AREA_LEFT, ICON_AREA_LEFT + ICON_AREA_WIDTH, AT_LEFT, y, "Audio:", c32::White );

				if ( p_rominfo->mSettings.Comment[9] == '1' ) {
					mpContext->DrawTextAlign( ICON_AREA_LEFT, ICON_AREA_LEFT + ICON_AREA_WIDTH, AT_RIGHT, y, "Async", c32::White );  y += line_height + 1;
				}						
				else if ( p_rominfo->mSettings.Comment[9] == '2' ) {
					mpContext->DrawTextAlign( ICON_AREA_LEFT, ICON_AREA_LEFT + ICON_AREA_WIDTH, AT_RIGHT, y, "Sync", c32::White );  y += line_height + 1;
				}
				else if ( p_rominfo->mSettings.Comment[9] == '3' ) {
					mpContext->DrawTextAlign( ICON_AREA_LEFT, ICON_AREA_LEFT + ICON_AREA_WIDTH, AT_RIGHT, y, "Disabled", c32::White );  y += line_height + 1;
				}
			}
			
			if ( p_rominfo->mSettings.Comment[10] != '0' ) {
				mpContext->DrawTextAlign( ICON_AREA_LEFT, ICON_AREA_LEFT + ICON_AREA_WIDTH, AT_LEFT, y, "Ctrl:", c32::White );

				if ( p_rominfo->mSettings.Comment[10] == '1' ) {
					mpContext->DrawTextAlign( ICON_AREA_LEFT, ICON_AREA_LEFT + ICON_AREA_WIDTH, AT_RIGHT, y, "CButtons", c32::White );  y += line_height + 1;
				}						
				else if ( p_rominfo->mSettings.Comment[10] == '2' ) {
					mpContext->DrawTextAlign( ICON_AREA_LEFT, ICON_AREA_LEFT + ICON_AREA_WIDTH, AT_RIGHT, y, "Default Z+L Swap", c32::White );  y += line_height + 1;
				}
				else if ( p_rominfo->mSettings.Comment[10] == '3' ) {
					mpContext->DrawTextAlign( ICON_AREA_LEFT, ICON_AREA_LEFT + ICON_AREA_WIDTH, AT_RIGHT, y, "DPad", c32::White );  y += line_height + 1;
				}
				else if ( p_rominfo->mSettings.Comment[10] == '4' ) {
					mpContext->DrawTextAlign( ICON_AREA_LEFT, ICON_AREA_LEFT + ICON_AREA_WIDTH, AT_RIGHT, y, "DPad and Buttons", c32::White );  y += line_height + 1;
				}
				else if ( p_rominfo->mSettings.Comment[10] == '5' ) {
					mpContext->DrawTextAlign( ICON_AREA_LEFT, ICON_AREA_LEFT + ICON_AREA_WIDTH, AT_RIGHT, y, "DPad and Buttons Inv", c32::White );  y += line_height + 1;
				}
			}

			if ( p_rominfo->mSettings.Comment[11] != '0' ) {
				mpContext->DrawTextAlign( ICON_AREA_LEFT, ICON_AREA_LEFT + ICON_AREA_WIDTH, AT_LEFT, y, "Clean Scene:", c32::White );

				if ( p_rominfo->mSettings.Comment[11] == '1' ) {
					mpContext->DrawTextAlign( ICON_AREA_LEFT, ICON_AREA_LEFT + ICON_AREA_WIDTH, AT_RIGHT, y, "Enabled", c32::White );  y += line_height + 1;
				}						
				else if ( p_rominfo->mSettings.Comment[11] == '2' ) {
					mpContext->DrawTextAlign( ICON_AREA_LEFT, ICON_AREA_LEFT + ICON_AREA_WIDTH, AT_RIGHT, y, "Disabled", c32::White );  y += line_height + 1;
				}
			}

			if ( p_rominfo->mSettings.Comment[12] != '0' ) {
				mpContext->DrawTextAlign( ICON_AREA_LEFT, ICON_AREA_LEFT + ICON_AREA_WIDTH, AT_LEFT, y, "Dyn Loop Opt:", c32::White );

				if ( p_rominfo->mSettings.Comment[12] == '1' ) {
					mpContext->DrawTextAlign( ICON_AREA_LEFT, ICON_AREA_LEFT + ICON_AREA_WIDTH, AT_RIGHT, y, "Enabled", c32::White );  y += line_height + 1;
				}						
				else if ( p_rominfo->mSettings.Comment[12] == '2' ) {
					mpContext->DrawTextAlign( ICON_AREA_LEFT, ICON_AREA_LEFT + ICON_AREA_WIDTH, AT_RIGHT, y, "Disabled", c32::White );  y += line_height + 1;
				}
			}

			if ( p_rominfo->mSettings.Comment[13] != '0' ) {
				mpContext->DrawTextAlign( ICON_AREA_LEFT, ICON_AREA_LEFT + ICON_AREA_WIDTH, AT_LEFT, y, "Double Disp:", c32::White );

				if ( p_rominfo->mSettings.Comment[13] == '1' ) {
					mpContext->DrawTextAlign( ICON_AREA_LEFT, ICON_AREA_LEFT + ICON_AREA_WIDTH, AT_RIGHT, y, "Enabled", c32::White );  y += line_height + 1;
				}						
				else if ( p_rominfo->mSettings.Comment[13] == '2' ) {
					mpContext->DrawTextAlign( ICON_AREA_LEFT, ICON_AREA_LEFT + ICON_AREA_WIDTH, AT_RIGHT, y, "Disabled", c32::White );  y += line_height + 1;
				}
			}

			if ( p_rominfo->mSettings.Comment[14] != '0' ) {
				mpContext->DrawTextAlign( ICON_AREA_LEFT, ICON_AREA_LEFT + ICON_AREA_WIDTH, AT_LEFT, y, "Audio Rate Match:", c32::White );

				if ( p_rominfo->mSettings.Comment[14] == '1' ) {
					mpContext->DrawTextAlign( ICON_AREA_LEFT, ICON_AREA_LEFT + ICON_AREA_WIDTH, AT_RIGHT, y, "Yes", c32::White );  y += line_height + 1;
				}						
				else if ( p_rominfo->mSettings.Comment[14] == '2' ) {
					mpContext->DrawTextAlign( ICON_AREA_LEFT, ICON_AREA_LEFT + ICON_AREA_WIDTH, AT_RIGHT, y, "No", c32::White );  y += line_height + 1;
				}
			}
			
			if ( p_rominfo->mSettings.Comment[15] != '0' ) {
				mpContext->DrawTextAlign( ICON_AREA_LEFT, ICON_AREA_LEFT + ICON_AREA_WIDTH, AT_LEFT, y, "Fog Emulation:", c32::White );

				if ( p_rominfo->mSettings.Comment[15] == '1' ) {
					mpContext->DrawTextAlign( ICON_AREA_LEFT, ICON_AREA_LEFT + ICON_AREA_WIDTH, AT_RIGHT, y, "Enabled", c32::White );  y += line_height + 1;
				}						
				else if ( p_rominfo->mSettings.Comment[15] == '2' ) {
					mpContext->DrawTextAlign( ICON_AREA_LEFT, ICON_AREA_LEFT + ICON_AREA_WIDTH, AT_RIGHT, y, "Disabled", c32::White );  y += line_height + 1;
				}
			}
			
			if ( p_rominfo->mSettings.Comment[16] != '0' ) {
				mpContext->DrawTextAlign( ICON_AREA_LEFT, ICON_AREA_LEFT + ICON_AREA_WIDTH, AT_LEFT, y, "Notes:", c32::White );

				if ( p_rominfo->mSettings.Comment[16] == '1' ) {
					mpContext->DrawTextAlign( ICON_AREA_LEFT, ICON_AREA_LEFT + ICON_AREA_WIDTH, AT_RIGHT, y, "Crash", c32::White );  y += line_height + 1;
				}
				else if ( p_rominfo->mSettings.Comment[16] == '2' ) {
					mpContext->DrawTextAlign( ICON_AREA_LEFT, ICON_AREA_LEFT + ICON_AREA_WIDTH, AT_RIGHT, y, "Doesn't Boot", c32::White );  y += line_height + 1;
				}
				else if ( p_rominfo->mSettings.Comment[16] == '3' ) {
					mpContext->DrawTextAlign( ICON_AREA_LEFT, ICON_AREA_LEFT + ICON_AREA_WIDTH, AT_RIGHT, y, "Unplayable", c32::White );  y += line_height + 1;
				}
				else if ( p_rominfo->mSettings.Comment[16] == '4' ) {
					mpContext->DrawTextAlign( ICON_AREA_LEFT, ICON_AREA_LEFT + ICON_AREA_WIDTH, AT_RIGHT, y, "Slow", c32::White );  y += line_height + 1;
				}
				else if ( p_rominfo->mSettings.Comment[16] == '5' ) {
					mpContext->DrawTextAlign( ICON_AREA_LEFT, ICON_AREA_LEFT + ICON_AREA_WIDTH, AT_RIGHT, y, "No Sound", c32::White );  y += line_height + 1;
				}
				else if ( p_rominfo->mSettings.Comment[16] == '6' ) {
					mpContext->DrawTextAlign( ICON_AREA_LEFT, ICON_AREA_LEFT + ICON_AREA_WIDTH, AT_RIGHT, y, "Graphics Errors", c32::White );  y += line_height + 1;
				}
				else if ( p_rominfo->mSettings.Comment[16] == '7' ) {
					mpContext->DrawTextAlign( ICON_AREA_LEFT, ICON_AREA_LEFT + ICON_AREA_WIDTH, AT_RIGHT, y, "Unstable", c32::White );  y += line_height + 1;
				}
				else if ( p_rominfo->mSettings.Comment[16] == '8' ) {
					mpContext->DrawTextAlign( ICON_AREA_LEFT, ICON_AREA_LEFT + ICON_AREA_WIDTH, AT_RIGHT, y, "Error Free", c32::White );  y += line_height + 1;
				}
			}
		}
	}//end of mDisplayInfo
}
//*************************************************************************************
//
//*************************************************************************************
void IRomSelectorComponent::RenderPreview()
{
	const char * noimage = "No Image Available";
	c32	clrYELLOW = c32( 255, 255, 0, 0 );

	romselmenufs++;
	if (romselmenufs >= 3000) { romselmenufs = 51; }
	if (romselmenuani == 1) {
		romselmenufs = 1;
		romselmenuani = 0; 
	}
	
	if (romselmenufs < 31) {
		if ((romselmenufs < 2) && (romselmenudir == 1)) { ICON_AREA_LEFT = -460; TEXT_AREA_LEFT = -180; }
		else if ((romselmenufs < 4) && (romselmenudir == 1)) { ICON_AREA_LEFT = -428; TEXT_AREA_LEFT = -148; }
		else if ((romselmenufs < 6) && (romselmenudir == 1)) { ICON_AREA_LEFT = -396; TEXT_AREA_LEFT = -116; }
		else if ((romselmenufs < 8) && (romselmenudir == 1)) { ICON_AREA_LEFT = -364; TEXT_AREA_LEFT = -84; }
		else if ((romselmenufs < 10) && (romselmenudir == 1)) { ICON_AREA_LEFT = -332; TEXT_AREA_LEFT = -52; }
		else if ((romselmenufs < 12) && (romselmenudir == 1)) { ICON_AREA_LEFT = -300; TEXT_AREA_LEFT = -20; }
		else if ((romselmenufs < 14) && (romselmenudir == 1)) { ICON_AREA_LEFT = -268; TEXT_AREA_LEFT = 12; }
		else if ((romselmenufs < 16) && (romselmenudir == 1)) { ICON_AREA_LEFT = -236; TEXT_AREA_LEFT = 44; }
		else if ((romselmenufs < 18) && (romselmenudir == 1)) { ICON_AREA_LEFT = -204; TEXT_AREA_LEFT = 76; }
		else if ((romselmenufs < 20) && (romselmenudir == 1)) { ICON_AREA_LEFT = -172; TEXT_AREA_LEFT = 108; }
		else if ((romselmenufs < 22) && (romselmenudir == 1)) { ICON_AREA_LEFT = -140; TEXT_AREA_LEFT = 140; }
		else if ((romselmenufs < 24) && (romselmenudir == 1)) { ICON_AREA_LEFT = -108; TEXT_AREA_LEFT = 172; }
		else if ((romselmenufs < 26) && (romselmenudir == 1)) { ICON_AREA_LEFT = -76; TEXT_AREA_LEFT = 204; }
		else if ((romselmenufs < 28) && (romselmenudir == 1)) { ICON_AREA_LEFT = -44; TEXT_AREA_LEFT = 236; }
		else if ((romselmenufs < 30) && (romselmenudir == 1)) { ICON_AREA_LEFT = -12; TEXT_AREA_LEFT = 268; }
		if ((romselmenufs < 2) && (romselmenudir == 2)) { ICON_AREA_LEFT = 500; TEXT_AREA_LEFT = 780; }
		else if ((romselmenufs < 4) && (romselmenudir == 2)) { ICON_AREA_LEFT = 468; TEXT_AREA_LEFT = 748; }
		else if ((romselmenufs < 6) && (romselmenudir == 2)) { ICON_AREA_LEFT = 436; TEXT_AREA_LEFT = 716; }
		else if ((romselmenufs < 8) && (romselmenudir == 2)) { ICON_AREA_LEFT = 404; TEXT_AREA_LEFT = 684; }
		else if ((romselmenufs < 10) && (romselmenudir == 2)) { ICON_AREA_LEFT = 372; TEXT_AREA_LEFT = 652; }
		else if ((romselmenufs < 12) && (romselmenudir == 2)) { ICON_AREA_LEFT = 340; TEXT_AREA_LEFT = 620; }
		else if ((romselmenufs < 14) && (romselmenudir == 2)) { ICON_AREA_LEFT = 308; TEXT_AREA_LEFT = 588; }
		else if ((romselmenufs < 16) && (romselmenudir == 2)) { ICON_AREA_LEFT = 276; TEXT_AREA_LEFT = 556; }
		else if ((romselmenufs < 18) && (romselmenudir == 2)) { ICON_AREA_LEFT = 244; TEXT_AREA_LEFT = 524; }
		else if ((romselmenufs < 20) && (romselmenudir == 2)) { ICON_AREA_LEFT = 212; TEXT_AREA_LEFT = 492; }
		else if ((romselmenufs < 22) && (romselmenudir == 2)) { ICON_AREA_LEFT = 180; TEXT_AREA_LEFT = 460; }
		else if ((romselmenufs < 24) && (romselmenudir == 2)) { ICON_AREA_LEFT = 148; TEXT_AREA_LEFT = 428; }
		else if ((romselmenufs < 26) && (romselmenudir == 2)) { ICON_AREA_LEFT = 116; TEXT_AREA_LEFT = 396; }
		else if ((romselmenufs < 28) && (romselmenudir == 2)) { ICON_AREA_LEFT = 84; TEXT_AREA_LEFT = 364; }
		else if ((romselmenufs < 30) && (romselmenudir == 2)) { ICON_AREA_LEFT = 52; TEXT_AREA_LEFT = 332; }

		if ( mCurrentSelection < mRomsList.size() ) {
			if ((romselmenufs > 13) && (romselmenudir == 1)) {
			mpContext->DrawRect( ICON_AREA_LEFT-2, ICON_AREA_TOP-2, ICON_AREA_WIDTH+4, ICON_AREA_HEIGHT+4, c32::White );
			mpContext->DrawRect( ICON_AREA_LEFT-1, ICON_AREA_TOP-1, ICON_AREA_WIDTH+2, ICON_AREA_HEIGHT+2, mpContext->GetBackgroundColour() ); 
			}
			else if (romselmenudir == 2) {
			mpContext->DrawRect( ICON_AREA_LEFT-2, ICON_AREA_TOP-2, ICON_AREA_WIDTH+4, ICON_AREA_HEIGHT+4, c32::White );
			mpContext->DrawRect( ICON_AREA_LEFT-1, ICON_AREA_TOP-1, ICON_AREA_WIDTH+2, ICON_AREA_HEIGHT+2, mpContext->GetBackgroundColour() ); 
			}

			v2	tl( ICON_AREA_LEFT, ICON_AREA_TOP );
			v2	wh( ICON_AREA_WIDTH, ICON_AREA_HEIGHT );

			if( mpPreviewTexture != NULL ) {
				c32		colour( c32::White );
				mpContext->DrawRect( ICON_AREA_LEFT, ICON_AREA_TOP, ICON_AREA_WIDTH, ICON_AREA_HEIGHT, c32::Black );
			}
			else {
				mpContext->DrawRect( ICON_AREA_LEFT, ICON_AREA_TOP, ICON_AREA_WIDTH, ICON_AREA_HEIGHT, c32::Black );
				mpContext->DrawText( (ICON_AREA_LEFT + (ICON_AREA_WIDTH / 2)) - (mpContext->GetTextWidth(noimage) / 2), ICON_AREA_TOP + (ICON_AREA_HEIGHT / 2), noimage, mpContext->GetDefaultTextColour() );
			}
		}

		u32		font_height( mpContext->GetFontHeight() );
		u32		line_height( font_height + 2 );

		s32 y = TEXT_AREA_TOP;

		if( mCurrentSelection < mRomsList.size() )
		{
			SRomInfo *	p_rominfo( mRomsList[ mCurrentSelection ] );

			const char *	cic_name( ROM_GetCicName( p_rominfo->mCicType ) );
			const char *	country( ROM_GetCountryNameFromID( p_rominfo->mRomID.CountryID ) );
			u32				rom_size( p_rominfo->mRomSize );

			char buffer[ 32 ];
			sprintf( buffer, "%d MB", rom_size / (1024*1024) ); 

			DrawInfoText( mpContext, y, "Boot:", cic_name );	y += line_height + 5;
			DrawInfoText( mpContext, y, "Country:", country );	y += line_height + 5;
			DrawInfoText( mpContext, y, "Size:", buffer );	y += line_height + 5;

			DrawInfoText( mpContext, y, "Save:", ROM_GetSaveTypeName( p_rominfo->mSettings.SaveType ) ); y += line_height + 5;
			DrawInfoText( mpContext, y, "EPak:", ROM_GetExpansionPakUsageName( p_rominfo->mSettings.ExpansionPakUsage ) ); y += line_height + 5;
			DrawInfoText( mpContext, y, "Dynarec:", p_rominfo->mSettings.DynarecSupported ? "Supported" : "Unsupported" ); y += line_height + 15;
					
			if (( p_rominfo->mSettings.Comment[0] != '0' ) &&( p_rominfo->mSettings.Comment[0] != '1' ) && ( p_rominfo->mSettings.Comment[0] != '2' ) && ( p_rominfo->mSettings.Comment[0] != '3' ) && ( p_rominfo->mSettings.Comment[0] != '4' ) && ( p_rominfo->mSettings.Comment[0] != '5' )) {
				DrawInfoText( mpContext, y, "    Compatibility Info", "" ); y += line_height + 5;				
				DrawInfoText( mpContext, y, "       Not Available", "" ); y += line_height + 5; 
			} else if (romselmenufs > 10) {
				if (p_rominfo->mSettings.Comment[0] == '0') {
					DrawInfoText( mpContext, y, "Compatibility:", "" );
					mpContext->DrawRect( TEXT_AREA_LEFT + TEXT_AREA_WIDTH - 10, y - 10, 10, 10, c32::Grey ); y += line_height + 5;
				}	else if ( p_rominfo->mSettings.Comment[0] == '1' ) {
					DrawInfoText( mpContext, y, "Compatibility:", "" );
					mpContext->DrawRect( TEXT_AREA_LEFT + TEXT_AREA_WIDTH - 10, y - 10, 10, 10, c32::Red ); y += line_height + 5;
				}	else if ( p_rominfo->mSettings.Comment[0] == '2' ) {
					DrawInfoText( mpContext, y, "Compatibility:", "" );
					mpContext->DrawRect( TEXT_AREA_LEFT + TEXT_AREA_WIDTH - 10, y - 10, 10, 10, c32::Orange ); y += line_height + 5;
				}	else if ( p_rominfo->mSettings.Comment[0] == '3' ) {
					DrawInfoText( mpContext, y, "Compatibility:", "" );
					mpContext->DrawRect( TEXT_AREA_LEFT + TEXT_AREA_WIDTH - 10, y - 10, 10, 10, clrYELLOW ); y += line_height + 5;
				}	else if ( p_rominfo->mSettings.Comment[0] == '4' ) {
					DrawInfoText( mpContext, y, "Compatibility:", "" );
					mpContext->DrawRect( TEXT_AREA_LEFT + TEXT_AREA_WIDTH - 10, y - 10, 10, 10, c32::Green ); y += line_height + 5;
				}	else if ( p_rominfo->mSettings.Comment[0] == '5' ) {
					DrawInfoText( mpContext, y, "Compatibility:", "" );
					mpContext->DrawRect( TEXT_AREA_LEFT + TEXT_AREA_WIDTH - 10, y - 10, 10, 10, c32::Blue ); y += line_height + 5;
				}
					DrawInfoText( mpContext, y, "Hold     for more info.", "" );				
					mpContext->DrawRect( TEXT_AREA_LEFT + 36, y - 8, 7, 7, c32::White );
					mpContext->DrawRect( TEXT_AREA_LEFT + 37, y - 7, 5, 5, c32::Black ); y += line_height + 5;				
			}
		}
		ICON_AREA_LEFT = 20;
		TEXT_AREA_LEFT = 300;
	
	}
	else {		
		if( mCurrentSelection < mRomsList.size() ) {
			mpContext->DrawRect( ICON_AREA_LEFT-2, ICON_AREA_TOP-2, ICON_AREA_WIDTH+4, ICON_AREA_HEIGHT+4, c32::White );
			mpContext->DrawRect( ICON_AREA_LEFT-1, ICON_AREA_TOP-1, ICON_AREA_WIDTH+2, ICON_AREA_HEIGHT+2, mpContext->GetBackgroundColour() );

			v2	tl( ICON_AREA_LEFT, ICON_AREA_TOP );
			v2	wh( ICON_AREA_WIDTH, ICON_AREA_HEIGHT );

			if( mpPreviewTexture != NULL )
			{
				c32		colour( c32::White );

				if ( mPreviewLoadedTime < PREVIEW_FADE_TIME )
				{
					colour = c32( 255, 255, 255, u8( mPreviewLoadedTime * 255.f / PREVIEW_FADE_TIME ) );
				}

				mpContext->DrawRect( ICON_AREA_LEFT, ICON_AREA_TOP, ICON_AREA_WIDTH, ICON_AREA_HEIGHT, c32::Black );
				if (romselmenufs > 46) {
					mpContext->RenderTexture( mpPreviewTexture, tl, wh, colour );
				}
			}
			else
			{
				mpContext->DrawRect( ICON_AREA_LEFT, ICON_AREA_TOP, ICON_AREA_WIDTH, ICON_AREA_HEIGHT, c32::Black );
				mpContext->DrawText( (ICON_AREA_LEFT + (ICON_AREA_WIDTH / 2)) - (mpContext->GetTextWidth(noimage) / 2), ICON_AREA_TOP + (ICON_AREA_HEIGHT / 2), noimage, mpContext->GetDefaultTextColour() );
			}
		}

		u32		font_height( mpContext->GetFontHeight() );
		u32		line_height( font_height + 2 );

		s32 y = TEXT_AREA_TOP;

		if( mCurrentSelection < mRomsList.size() )
		{
			SRomInfo *	p_rominfo( mRomsList[ mCurrentSelection ] );

			const char *	cic_name( ROM_GetCicName( p_rominfo->mCicType ) );
			const char *	country( ROM_GetCountryNameFromID( p_rominfo->mRomID.CountryID ) );
			u32				rom_size( p_rominfo->mRomSize );

			char buffer[ 32 ];
			sprintf( buffer, "%d MB", rom_size / (1024*1024) ); 

			DrawInfoText( mpContext, y, "Boot:", cic_name );	y += line_height + 5; 
			DrawInfoText( mpContext, y, "Country:", country );	y += line_height + 5;
			DrawInfoText( mpContext, y, "Size:", buffer );	y += line_height + 5;

			DrawInfoText( mpContext, y, "Save:", ROM_GetSaveTypeName( p_rominfo->mSettings.SaveType ) ); y += line_height + 5;
			DrawInfoText( mpContext, y, "EPak:", ROM_GetExpansionPakUsageName( p_rominfo->mSettings.ExpansionPakUsage ) ); y += line_height + 5;
			DrawInfoText( mpContext, y, "Dynarec:", p_rominfo->mSettings.DynarecSupported ? "Supported" : "Unsupported" ); y += line_height + 15;

			if (( p_rominfo->mSettings.Comment[0] != '0' ) &&( p_rominfo->mSettings.Comment[0] != '1' ) && ( p_rominfo->mSettings.Comment[0] != '2' ) && ( p_rominfo->mSettings.Comment[0] != '3' ) && ( p_rominfo->mSettings.Comment[0] != '4' ) && ( p_rominfo->mSettings.Comment[0] != '5' )) {
				DrawInfoText( mpContext, y, "    Compatibility Info", "" ); y += line_height + 5;				
				DrawInfoText( mpContext, y, "       Not Available", "" ); y += line_height + 5; 
			} else {
				if ( p_rominfo->mSettings.Comment[0] == '0' ) {
					DrawInfoText( mpContext, y, "Compatibility:", "" );
					mpContext->DrawRect( TEXT_AREA_LEFT + TEXT_AREA_WIDTH - 10, y - 10, 10, 10, c32::Grey ); y += line_height + 5;
				}	else if ( p_rominfo->mSettings.Comment[0] == '1' ) {
					DrawInfoText( mpContext, y, "Compatibility:", "" );
					mpContext->DrawRect( TEXT_AREA_LEFT + TEXT_AREA_WIDTH - 10, y - 10, 10, 10, c32::Red ); y += line_height + 5;
				}	else if ( p_rominfo->mSettings.Comment[0] == '2' ) {
					DrawInfoText( mpContext, y, "Compatibility:", "" );
					mpContext->DrawRect( TEXT_AREA_LEFT + TEXT_AREA_WIDTH - 10, y - 10, 10, 10, c32::Orange ); y += line_height + 5;
				}	else if ( p_rominfo->mSettings.Comment[0] == '3' ) {
					DrawInfoText( mpContext, y, "Compatibility:", "" );
					mpContext->DrawRect( TEXT_AREA_LEFT + TEXT_AREA_WIDTH - 10, y - 10, 10, 10, clrYELLOW ); y += line_height + 5;
				}	else if ( p_rominfo->mSettings.Comment[0] == '4' ) {
					DrawInfoText( mpContext, y, "Compatibility:", "" );
					mpContext->DrawRect( TEXT_AREA_LEFT + TEXT_AREA_WIDTH - 10, y - 10, 10, 10, c32::Green ); y += line_height + 5;
				}	else if ( p_rominfo->mSettings.Comment[0] == '5' ) {
					DrawInfoText( mpContext, y, "Compatibility:", "" );
					mpContext->DrawRect( TEXT_AREA_LEFT + TEXT_AREA_WIDTH - 10, y - 10, 10, 10, c32::Blue ); y += line_height + 5;
				}				
					DrawInfoText( mpContext, y, "Hold     for more info.", "" );				
					mpContext->DrawRect( TEXT_AREA_LEFT + 36, y - 8, 7, 7, c32::White );
					mpContext->DrawRect( TEXT_AREA_LEFT + 37, y - 7, 5, 5, c32::Black ); y += line_height + 5;
			}
			if (( p_rominfo->mSettings.Comment[0] == '0' ) || ( p_rominfo->mSettings.Comment[0] == '1' ) || ( p_rominfo->mSettings.Comment[0] == '2' ) || ( p_rominfo->mSettings.Comment[0] == '3' ) || ( p_rominfo->mSettings.Comment[0] == '4' ) || ( p_rominfo->mSettings.Comment[0] == '5' )) {
				if(mDisplayInfo) {					
					//const char *compatver = p_rominfo->mSettings.Comment + 21;
					y = 44 + line_height;
					mpContext->DrawRect( 100, 40, 280, 192, c32::White );
					mpContext->DrawRect( 102, 42, 276, 188, c32::Black );

					if ( p_rominfo->mSettings.Comment[1] != '0' ) {
						mpContext->DrawTextAlign( 104, 376, AT_LEFT, y, "Texture Update Check:", c32::White );

						if ( p_rominfo->mSettings.Comment[1] == '1' ) {
							mpContext->DrawTextAlign( 104, 376, AT_RIGHT, y, "Every Frame", c32::White );  y += line_height + 5;
						}
						else if ( p_rominfo->mSettings.Comment[1] == '2' ) {
							mpContext->DrawTextAlign( 104, 376, AT_RIGHT, y, "Every 2 Frames", c32::White );  y += line_height + 5;
						}
						else if ( p_rominfo->mSettings.Comment[1] == '3' ) {
							mpContext->DrawTextAlign( 104, 376, AT_RIGHT, y, "Every 3 Frames", c32::White );  y += line_height + 5;
						}
						else if ( p_rominfo->mSettings.Comment[1] == '4' ) {
							mpContext->DrawTextAlign( 104, 376, AT_RIGHT, y, "Every 4 Frames", c32::White );  y += line_height + 5;
						}
						else if ( p_rominfo->mSettings.Comment[1] == '5' ) {
							mpContext->DrawTextAlign( 104, 376, AT_RIGHT, y, "Every 5 Frames", c32::White );  y += line_height + 5;
						}
						else if ( p_rominfo->mSettings.Comment[1] == '6' ) {
							mpContext->DrawTextAlign( 104, 376, AT_RIGHT, y, "Every 10 Frames", c32::White );  y += line_height + 5;
						}
						else if ( p_rominfo->mSettings.Comment[1] == '7' ) {
							mpContext->DrawTextAlign( 104, 376, AT_RIGHT, y, "Every 15 Frames", c32::White );  y += line_height + 5;
						}
						else if ( p_rominfo->mSettings.Comment[1] == '8' ) {
							mpContext->DrawTextAlign( 104, 376, AT_RIGHT, y, "Every 20 Frames", c32::White );  y += line_height + 5;
						}
						else if ( p_rominfo->mSettings.Comment[1] == '9' ) {
							mpContext->DrawTextAlign( 104, 376, AT_RIGHT, y, "Every 30 Frames", c32::White );  y += line_height + 5;
						}
					}
			
					if ( p_rominfo->mSettings.Comment[2] != '0' ) {
						mpContext->DrawTextAlign( 104, 376, AT_LEFT, y, "FrameSkip:", c32::White );

						if ( p_rominfo->mSettings.Comment[2] == '1' ) {
							mpContext->DrawTextAlign( 104, 376, AT_RIGHT, y, "Auto 1", c32::White );  y += line_height + 5;
						}						
						else if ( p_rominfo->mSettings.Comment[2] == '2' ) {
							mpContext->DrawTextAlign( 104, 376, AT_RIGHT, y, "Auto 2", c32::White );  y += line_height + 5;
						}
						else if ( p_rominfo->mSettings.Comment[2] == '3' ) {
							mpContext->DrawTextAlign( 104, 376, AT_RIGHT, y, "1", c32::White );  y += line_height + 5;
						}
						else if ( p_rominfo->mSettings.Comment[2] == '4' ) {
							mpContext->DrawTextAlign( 104, 376, AT_RIGHT, y, "2", c32::White );  y += line_height + 5;
						}
						else if ( p_rominfo->mSettings.Comment[2] == '5' ) {
							mpContext->DrawTextAlign( 104, 376, AT_RIGHT, y, "3", c32::White );  y += line_height + 5;
						}
						else if ( p_rominfo->mSettings.Comment[2] == '6' ) {
							mpContext->DrawTextAlign( 104, 376, AT_RIGHT, y, "4", c32::White );  y += line_height + 5;
						}
						else if ( p_rominfo->mSettings.Comment[2] == '7' ) {
							mpContext->DrawTextAlign( 104, 376, AT_RIGHT, y, "5", c32::White );  y += line_height + 5;
						}
					}
			
					if ( p_rominfo->mSettings.Comment[3] != '0' ) {
						mpContext->DrawTextAlign( 104, 376, AT_LEFT, y, "Zoom:", c32::White );

						if ( p_rominfo->mSettings.Comment[3] == 'a' ) {
							mpContext->DrawTextAlign( 104, 376, AT_RIGHT, y, "101%", c32::White );  y += line_height + 5;
						}						
						else if ( p_rominfo->mSettings.Comment[3] == 'b' ) {
							mpContext->DrawTextAlign( 104, 376, AT_RIGHT, y, "102%", c32::White );  y += line_height + 5;
						}
						else if ( p_rominfo->mSettings.Comment[3] == 'c' ) {
							mpContext->DrawTextAlign( 104, 376, AT_RIGHT, y, "103%", c32::White );  y += line_height + 5;
						}
						else if ( p_rominfo->mSettings.Comment[3] == 'd' ) {
							mpContext->DrawTextAlign( 104, 376, AT_RIGHT, y, "104%", c32::White );  y += line_height + 5;
						}
						else if ( p_rominfo->mSettings.Comment[3] == 'e' ) {
							mpContext->DrawTextAlign( 104, 376, AT_RIGHT, y, "105%", c32::White );  y += line_height + 5;
						}
						else if ( p_rominfo->mSettings.Comment[3] == 'f' ) {
							mpContext->DrawTextAlign( 104, 376, AT_RIGHT, y, "106%", c32::White );  y += line_height + 5;
						}
						else if ( p_rominfo->mSettings.Comment[3] == 'g' ) {
							mpContext->DrawTextAlign( 104, 376, AT_RIGHT, y, "107%", c32::White );  y += line_height + 5;
						}
						else if ( p_rominfo->mSettings.Comment[3] == 'h' ) {
							mpContext->DrawTextAlign( 104, 376, AT_RIGHT, y, "108%", c32::White );  y += line_height + 5;
						}
						else if ( p_rominfo->mSettings.Comment[3] == 'i' ) {
							mpContext->DrawTextAlign( 104, 376, AT_RIGHT, y, "109%", c32::White );  y += line_height + 5;
						}
						else if ( p_rominfo->mSettings.Comment[3] == 'j' ) {
							mpContext->DrawTextAlign( 104, 376, AT_RIGHT, y, "110%", c32::White );  y += line_height + 5;
						}						
						else if ( p_rominfo->mSettings.Comment[3] == 'k' ) {
							mpContext->DrawTextAlign( 104, 376, AT_RIGHT, y, "111%", c32::White );  y += line_height + 5;
						}
						else if ( p_rominfo->mSettings.Comment[3] == 'l' ) {
							mpContext->DrawTextAlign( 104, 376, AT_RIGHT, y, "112%", c32::White );  y += line_height + 5;
						}
						else if ( p_rominfo->mSettings.Comment[3] == 'm' ) {
							mpContext->DrawTextAlign( 104, 376, AT_RIGHT, y, "113%", c32::White );  y += line_height + 5;
						}
						else if ( p_rominfo->mSettings.Comment[3] == 'n' ) {
							mpContext->DrawTextAlign( 104, 376, AT_RIGHT, y, "114%", c32::White );  y += line_height + 5;
						}
						else if ( p_rominfo->mSettings.Comment[3] == 'o' ) {
							mpContext->DrawTextAlign( 104, 376, AT_RIGHT, y, "115%", c32::White );  y += line_height + 5;
						}
						else if ( p_rominfo->mSettings.Comment[3] == 'p' ) {
							mpContext->DrawTextAlign( 104, 376, AT_RIGHT, y, "116%", c32::White );  y += line_height + 5;
						}
						else if ( p_rominfo->mSettings.Comment[3] == 'q' ) {
							mpContext->DrawTextAlign( 104, 376, AT_RIGHT, y, "117%", c32::White );  y += line_height + 5;
						}
						else if ( p_rominfo->mSettings.Comment[3] == 'r' ) {
							mpContext->DrawTextAlign( 104, 376, AT_RIGHT, y, "118%", c32::White );  y += line_height + 5;
						}
						else if ( p_rominfo->mSettings.Comment[3] == 's' ) {
							mpContext->DrawTextAlign( 104, 376, AT_RIGHT, y, "119%", c32::White );  y += line_height + 5;
						}						
						else if ( p_rominfo->mSettings.Comment[3] == 't' ) {
							mpContext->DrawTextAlign( 104, 376, AT_RIGHT, y, "120%", c32::White );  y += line_height + 5;
						}
						else if ( p_rominfo->mSettings.Comment[3] == 'u' ) {
							mpContext->DrawTextAlign( 104, 376, AT_RIGHT, y, "121%", c32::White );  y += line_height + 5;
						}						
						else if ( p_rominfo->mSettings.Comment[3] == 'v' ) {
							mpContext->DrawTextAlign( 104, 376, AT_RIGHT, y, "122%", c32::White );  y += line_height + 5;
						}
						else if ( p_rominfo->mSettings.Comment[3] == 'w' ) {
							mpContext->DrawTextAlign( 104, 376, AT_RIGHT, y, "123%", c32::White );  y += line_height + 5;
						}						
						else if ( p_rominfo->mSettings.Comment[3] == 'x' ) {
							mpContext->DrawTextAlign( 104, 376, AT_RIGHT, y, "124%", c32::White );  y += line_height + 5;
						}
						else if ( p_rominfo->mSettings.Comment[3] == 'y' ) {
							mpContext->DrawTextAlign( 104, 376, AT_RIGHT, y, "125%", c32::White );  y += line_height + 5;
						}						
					}
					
					if ( p_rominfo->mSettings.Comment[4] != '0' ) {
						mpContext->DrawTextAlign( 104, 376, AT_LEFT, y, "Limit Framerate:", c32::White );

						if ( p_rominfo->mSettings.Comment[4] == '1' ) {
							mpContext->DrawTextAlign( 104, 376, AT_RIGHT, y, "Yes", c32::White );  y += line_height + 5;
						}						
						else if ( p_rominfo->mSettings.Comment[4] == '2' ) {
							mpContext->DrawTextAlign( 104, 376, AT_RIGHT, y, "No", c32::White );  y += line_height + 5;
						}
					}
					
					if ( p_rominfo->mSettings.Comment[5] != '0' ) {
						mpContext->DrawTextAlign( 104, 376, AT_LEFT, y, "Dynamic Recompilation:", c32::White );

						if ( p_rominfo->mSettings.Comment[5] == '1' ) {
							mpContext->DrawTextAlign( 104, 376, AT_RIGHT, y, "Enabled", c32::White );  y += line_height + 5;
						}						
						else if ( p_rominfo->mSettings.Comment[5] == '2' ) {
							mpContext->DrawTextAlign( 104, 376, AT_RIGHT, y, "Disabled", c32::White );  y += line_height + 5;
						}
					}
					
					if ( p_rominfo->mSettings.Comment[6] != '0' ) {
						mpContext->DrawTextAlign( 104, 376, AT_LEFT, y, "Dynamic Stack Optimisation:", c32::White );

						if ( p_rominfo->mSettings.Comment[6] == '1' ) {
							mpContext->DrawTextAlign( 104, 376, AT_RIGHT, y, "Enabled", c32::White );  y += line_height + 5;
						}						
						else if ( p_rominfo->mSettings.Comment[6] == '2' ) {
							mpContext->DrawTextAlign( 104, 376, AT_RIGHT, y, "Disabled", c32::White );  y += line_height + 5;
						}
					}
					
					if ( p_rominfo->mSettings.Comment[7] != '0' ) {
						mpContext->DrawTextAlign( 104, 376, AT_LEFT, y, "High Level Emulation:", c32::White );

						if ( p_rominfo->mSettings.Comment[7] == '1' ) {
							mpContext->DrawTextAlign( 104, 376, AT_RIGHT, y, "Enabled", c32::White );  y += line_height + 5;
						}						
						else if ( p_rominfo->mSettings.Comment[7] == '2' ) {
							mpContext->DrawTextAlign( 104, 376, AT_RIGHT, y, "Disabled", c32::White );  y += line_height + 5;
						}
					}
					
					if ( p_rominfo->mSettings.Comment[8] != '0' ) {
						mpContext->DrawTextAlign( 104, 376, AT_LEFT, y, "Memory Access Optimisation:", c32::White );

						if ( p_rominfo->mSettings.Comment[8] == '1' ) {
							mpContext->DrawTextAlign( 104, 376, AT_RIGHT, y, "Enabled", c32::White );  y += line_height + 5;
						}						
						else if ( p_rominfo->mSettings.Comment[8] == '2' ) {
							mpContext->DrawTextAlign( 104, 376, AT_RIGHT, y, "Disabled", c32::White );  y += line_height + 5;
						}
					}
					
					if ( p_rominfo->mSettings.Comment[9] != '0' ) {
						mpContext->DrawTextAlign( 104, 376, AT_LEFT, y, "Audio:", c32::White );

						if ( p_rominfo->mSettings.Comment[9] == '1' ) {
							mpContext->DrawTextAlign( 104, 376, AT_RIGHT, y, "Asynchronous", c32::White );  y += line_height + 5;
						}						
						else if ( p_rominfo->mSettings.Comment[9] == '2' ) {
							mpContext->DrawTextAlign( 104, 376, AT_RIGHT, y, "Synchronous", c32::White );  y += line_height + 5;
						}
						else if ( p_rominfo->mSettings.Comment[9] == '3' ) {
							mpContext->DrawTextAlign( 104, 376, AT_RIGHT, y, "Disabled", c32::White );  y += line_height + 5;
						}
					}
					
					if ( p_rominfo->mSettings.Comment[10] != '0' ) {
						mpContext->DrawTextAlign( 104, 376, AT_LEFT, y, "Controller:", c32::White );

						if ( p_rominfo->mSettings.Comment[10] == '1' ) {
							mpContext->DrawTextAlign( 104, 376, AT_RIGHT, y, "CButtons", c32::White );  y += line_height + 5;
						}						
						else if ( p_rominfo->mSettings.Comment[10] == '2' ) {
							mpContext->DrawTextAlign( 104, 376, AT_RIGHT, y, "Default Z+L Swap", c32::White );  y += line_height + 5;
						}
						else if ( p_rominfo->mSettings.Comment[10] == '3' ) {
							mpContext->DrawTextAlign( 104, 376, AT_RIGHT, y, "DPad", c32::White );  y += line_height + 5;
						}
						else if ( p_rominfo->mSettings.Comment[10] == '4' ) {
							mpContext->DrawTextAlign( 104, 376, AT_RIGHT, y, "DPad and Buttons", c32::White );  y += line_height + 5;
						}
						else if ( p_rominfo->mSettings.Comment[10] == '5' ) {
							mpContext->DrawTextAlign( 104, 376, AT_RIGHT, y, "DPad and Buttons Inverted", c32::White );  y += line_height + 5;
						}
					}
					
					if ( p_rominfo->mSettings.Comment[11] != '0' ) {
						mpContext->DrawTextAlign( 104, 376, AT_LEFT, y, "Clean Scene:", c32::White );

						if ( p_rominfo->mSettings.Comment[11] == '1' ) {
							mpContext->DrawTextAlign( 104, 376, AT_RIGHT, y, "Enabled", c32::White );  y += line_height + 5;
						}						
						else if ( p_rominfo->mSettings.Comment[11] == '2' ) {
							mpContext->DrawTextAlign( 104, 376, AT_RIGHT, y, "Disabled", c32::White );  y += line_height + 5;
						}
					}
					
					if ( p_rominfo->mSettings.Comment[12] != '0' ) {
						mpContext->DrawTextAlign( 104, 376, AT_LEFT, y, "Dynamic Loop Optimisation:", c32::White );

						if ( p_rominfo->mSettings.Comment[12] == '1' ) {
							mpContext->DrawTextAlign( 104, 376, AT_RIGHT, y, "Enabled", c32::White );  y += line_height + 5;
						}						
						else if ( p_rominfo->mSettings.Comment[12] == '2' ) {
							mpContext->DrawTextAlign( 104, 376, AT_RIGHT, y, "Disabled", c32::White );  y += line_height + 5;
						}
					}
					
					if ( p_rominfo->mSettings.Comment[13] != '0' ) {
						mpContext->DrawTextAlign( 104, 376, AT_LEFT, y, "Double Display Lists:", c32::White );

						if ( p_rominfo->mSettings.Comment[13] == '1' ) {
							mpContext->DrawTextAlign( 104, 376, AT_RIGHT, y, "Enabled", c32::White );  y += line_height + 5;
						}						
						else if ( p_rominfo->mSettings.Comment[13] == '2' ) {
							mpContext->DrawTextAlign( 104, 376, AT_RIGHT, y, "Disabled", c32::White );  y += line_height + 5;
						}
					}
					
					if ( p_rominfo->mSettings.Comment[14] != '0' ) {
						mpContext->DrawTextAlign( 104, 376, AT_LEFT, y, "Audio Rate Match:", c32::White );

						if ( p_rominfo->mSettings.Comment[14] == '1' ) {
							mpContext->DrawTextAlign( 104, 376, AT_RIGHT, y, "Yes", c32::White );  y += line_height + 5;
						}						
						else if ( p_rominfo->mSettings.Comment[14] == '2' ) {
							mpContext->DrawTextAlign( 104, 376, AT_RIGHT, y, "No", c32::White );  y += line_height + 5;
						}
					}
					
					if ( p_rominfo->mSettings.Comment[15] != '0' ) {
						mpContext->DrawTextAlign( 104, 376, AT_LEFT, y, "Fog Emulation:", c32::White );

						if ( p_rominfo->mSettings.Comment[15] == '1' ) {
							mpContext->DrawTextAlign( 104, 376, AT_RIGHT, y, "Enabled", c32::White );  y += line_height + 5;
						}
						else if ( p_rominfo->mSettings.Comment[15] == '2' ) {
							mpContext->DrawTextAlign( 104, 376, AT_RIGHT, y, "Disabled", c32::White );  y += line_height + 5;				
						}
					}
			
					if ( p_rominfo->mSettings.Comment[16] != '0' ) {
						mpContext->DrawTextAlign( 104, 376, AT_LEFT, y, "Notes:", c32::White );

						if ( p_rominfo->mSettings.Comment[16] == '1' ) {
							mpContext->DrawTextAlign( 104, 376, AT_RIGHT, y, "Crash", c32::White );  y += line_height + 5;
						}						
						else if ( p_rominfo->mSettings.Comment[16] == '2' ) {
							mpContext->DrawTextAlign( 104, 376, AT_RIGHT, y, "No Boot", c32::White );  y += line_height + 5;
						}
						else if ( p_rominfo->mSettings.Comment[16] == '3' ) {
							mpContext->DrawTextAlign( 104, 376, AT_RIGHT, y, "Unplayable", c32::White );  y += line_height + 5;
						}
						else if ( p_rominfo->mSettings.Comment[16] == '4' ) {
							mpContext->DrawTextAlign( 104, 376, AT_RIGHT, y, "Slow", c32::White );  y += line_height + 5;
						}
						else if ( p_rominfo->mSettings.Comment[16] == '5' ) {
							mpContext->DrawTextAlign( 104, 376, AT_RIGHT, y, "No Sound", c32::White );  y += line_height + 5;
						}
						else if ( p_rominfo->mSettings.Comment[16] == '6' ) {
							mpContext->DrawTextAlign( 104, 376, AT_RIGHT, y, "Graphics Errors", c32::White );  y += line_height + 5;
						}
						else if ( p_rominfo->mSettings.Comment[16] == '7' ) {
							mpContext->DrawTextAlign( 104, 376, AT_RIGHT, y, "Unstable", c32::White );  y += line_height + 5;
						}
						else if ( p_rominfo->mSettings.Comment[16] == '8' ) {
							mpContext->DrawTextAlign( 104, 376, AT_RIGHT, y, "Error Free", c32::White );  y += line_height + 5;
						}
					}
				}
			}
		}
	}
}

//*************************************************************************************
//
//*************************************************************************************
void IRomSelectorComponent::RenderRomList_old()
{
	u32		font_height( mpContext->GetFontHeight() );
	u32		line_height( font_height + 2 );

	s32		x,y;
	x = TEXT_AREA_LEFT;
	y = TEXT_AREA_TOP + mCurrentScrollOffset + font_height;

	sceGuEnable(GU_SCISSOR_TEST);
	sceGuScissor(TEXT_AREA_LEFT, TEXT_AREA_TOP, TEXT_AREA_LEFT+TEXT_AREA_WIDTH, TEXT_AREA_TOP+TEXT_AREA_HEIGHT);

	const char * const	ptr_text( ">>" );
	u32					ptr_text_width( mpContext->GetTextWidth( ptr_text ) );

	for(u32 i = 0; i < mRomsList.size(); ++i)
	{
		const char *	p_gamename;
		if( mDisplayFilenames )
		{
			p_gamename = mRomsList[ i ]->mFilename.c_str();
		}
		else
		{
			p_gamename = mRomsList[ i ]->mSettings.GameName.c_str();
		}

		//
		// Check if this entry would be onscreen
		//
		if(s32(y+line_height) >= s32(TEXT_AREA_TOP) && s32(y-line_height) < s32(TEXT_AREA_TOP + TEXT_AREA_HEIGHT))
		{
			c32		colour;

			if(i == mCurrentSelection)
			{
				colour = mpContext->GetSelectedTextColour();
				mpContext->DrawText( x, y, ptr_text, colour );
			}
			else
			{
				colour = mpContext->GetDefaultTextColour();
			}
			mpContext->DrawText( x + ptr_text_width, y, p_gamename, colour );
		}
		y += line_height;
	}

	// Restore scissoring
	sceGuScissor(0,0, 480,272);
}
//*************************************************************************************
//
//*************************************************************************************
void IRomSelectorComponent::RenderRomList()
{		
	const char *	pre3p_gamename = NULL;
	const char *	pre2p_gamename = NULL;
	const char *	prevp_gamename = NULL;
	const char *	p_gamename;
	const char *	nextp_gamename = NULL;
	const char *	nex2p_gamename = NULL;	
	const char *	nex3p_gamename = NULL;
	c32		colour;
	
	if( mDisplayFilenames )
	{		
		if (mCurrentSelection > 2) {
			pre3p_gamename = mRomsList[ mCurrentSelection - 3 ]->mFilename.c_str();
		}
		if (mCurrentSelection > 1) {
			pre2p_gamename = mRomsList[ mCurrentSelection - 2 ]->mFilename.c_str();
		}
		if (mCurrentSelection > 0) {
			prevp_gamename = mRomsList[ mCurrentSelection - 1 ]->mFilename.c_str();
		}
		
		p_gamename = mRomsList[ mCurrentSelection ]->mFilename.c_str();
		
		if (mCurrentSelection < mRomsList.size() - 1) {
			nextp_gamename = mRomsList[ mCurrentSelection + 1 ]->mFilename.c_str();
		}
		if (mCurrentSelection < mRomsList.size() - 2) {
			nex2p_gamename = mRomsList[ mCurrentSelection + 2 ]->mFilename.c_str();
		}
		if (mCurrentSelection < mRomsList.size() - 3) {
			nex3p_gamename = mRomsList[ mCurrentSelection + 3 ]->mFilename.c_str();
		}
	}
	else
	{
		if (mCurrentSelection > 2) {
			pre3p_gamename = mRomsList[ mCurrentSelection - 3 ]->mSettings.GameName.c_str();
		}
		if (mCurrentSelection > 1) {
			pre2p_gamename = mRomsList[ mCurrentSelection - 2 ]->mSettings.GameName.c_str();
		}
		if (mCurrentSelection > 0) {
			prevp_gamename = mRomsList[ mCurrentSelection - 1 ]->mSettings.GameName.c_str();
		}
		
		p_gamename = mRomsList[ mCurrentSelection ]->mSettings.GameName.c_str();
		
		if (mCurrentSelection < mRomsList.size() - 1) {
			nextp_gamename = mRomsList[ mCurrentSelection + 1 ]->mSettings.GameName.c_str();
		}
		if (mCurrentSelection < mRomsList.size() - 2) {
			nex2p_gamename = mRomsList[ mCurrentSelection + 2 ]->mSettings.GameName.c_str();
		}
		if (mCurrentSelection < mRomsList.size() - 3) {
			nex3p_gamename = mRomsList[ mCurrentSelection + 3 ]->mSettings.GameName.c_str();
		}
	}
		
	if (romselmenufs < 31)  {		
		if (romselmenudir == 1) {			
			if (romseltextoffset == 0) {
				romseltextoffset = 20 + ((mpContext->GetTextWidth( nextp_gamename ) * 1.2) / 2) + ((mpContext->GetTextWidth( p_gamename ) * 0.8) / 2);
				romseltextrepos = (romseltextoffset / 30);
				romseltextscale = (0.4 / 30);
			}
			if (mCurrentSelection > 1)
			{
				colour = mpContext->GetDefaultTextColour();
				mpContext->DrawTextScale( 200 - (((mpContext->GetTextWidth( p_gamename ) * (0.8 + (romseltextscale * romselmenufs)) / 2)) + (mpContext->GetTextWidth( prevp_gamename ) * 0.8) + (mpContext->GetTextWidth( pre2p_gamename ) * 0.8)) - romseltextoffset, 260, 0.8, pre2p_gamename, colour );
			}
			if (mCurrentSelection > 0)
			{
				colour = mpContext->GetDefaultTextColour();
				mpContext->DrawTextScale( 220 - (((mpContext->GetTextWidth( p_gamename ) * (0.8 + (romseltextscale * romselmenufs)) / 2)) + (mpContext->GetTextWidth( prevp_gamename ) * 0.8)) - romseltextoffset, 260, 0.8, prevp_gamename, colour );
			}

			colour = mpContext->GetSelectedTextColour();
			mpContext->DrawTextScale( 240 - ((mpContext->GetTextWidth( p_gamename ) * (0.8 + (romseltextscale * romselmenufs)) / 2)) - romseltextoffset, 260, 0.8 + (romseltextscale * romselmenufs),p_gamename, colour );	
			
			colour = mpContext->GetDefaultTextColour();
			mpContext->DrawTextScale( 260 + ((mpContext->GetTextWidth( p_gamename ) * (0.8 + (romseltextscale * romselmenufs)) / 2)) - romseltextoffset, 260, 1.2 - (romseltextscale * romselmenufs),nextp_gamename, colour );
			
			if (mCurrentSelection < mRomsList.size() - 2)
			{
				colour = mpContext->GetDefaultTextColour();
				mpContext->DrawTextScale( 280 + (((mpContext->GetTextWidth( p_gamename ) * (0.8 + (romseltextscale * romselmenufs)) / 2)) + ((mpContext->GetTextWidth( nextp_gamename )) * (1.2 - (romseltextscale * romselmenufs)))) - romseltextoffset, 260, 0.8, nex2p_gamename, colour );
			}
			//#SALVY# Look at the below statement while only having 2 roms.
			//"if (mCurrentSelection < mRomsList.size() - 3)" is returning true when it shouldnt. 
			//mCurrentSelection = 0 and mRomsList.size() = 2. Thus it should return false. I dont get it. if (0 < -1) = false...
			if ((mCurrentSelection < mRomsList.size() - 3) && (mRomsList.size() > 2)) //"&& (mRomsList.size() > 2))" is a quick fix.
			{
				colour = mpContext->GetDefaultTextColour();
				//printf("1-%i\n", mCurrentSelection);
				//printf("2-%i\n", mRomsList.size());
				mpContext->DrawTextScale( 300 + (((mpContext->GetTextWidth( p_gamename ) * (0.8 + (romseltextscale * romselmenufs)) / 2)) + ((mpContext->GetTextWidth( nextp_gamename )) * (1.2 - (romseltextscale * romselmenufs))) + (mpContext->GetTextWidth( nex2p_gamename ) * 0.8)) - romseltextoffset, 260, 0.8, nex3p_gamename, colour );
			}
	
			romseltextoffset -= romseltextrepos;
		}
		else if (romselmenudir == 2) {
			
			if (romseltextoffset == 0) {
				romseltextoffset = 20 + ((mpContext->GetTextWidth( prevp_gamename ) * 1.2) / 2) + ((mpContext->GetTextWidth( p_gamename ) * 0.8) / 2);
				romseltextrepos = (romseltextoffset / 30);
				romseltextscale = (0.4 / 30);
			}
			if (mCurrentSelection > 2)
			{
				colour = mpContext->GetDefaultTextColour();
				mpContext->DrawTextScale( 180 - (((mpContext->GetTextWidth( p_gamename ) * (0.8 + (romseltextscale * romselmenufs)) / 2)) + (mpContext->GetTextWidth( prevp_gamename ) * (1.2 - (romseltextscale * romselmenufs))) + (mpContext->GetTextWidth( pre2p_gamename ) * 0.8) + (mpContext->GetTextWidth( pre3p_gamename ) * 0.8)) + romseltextoffset, 260, 0.8, pre3p_gamename, colour );
			}
			if (mCurrentSelection > 1)
			{
				colour = mpContext->GetDefaultTextColour();
				mpContext->DrawTextScale( 200 - (((mpContext->GetTextWidth( p_gamename ) * (0.8 + (romseltextscale * romselmenufs)) / 2)) + (mpContext->GetTextWidth( prevp_gamename ) * (1.2 - (romseltextscale * romselmenufs))) + (mpContext->GetTextWidth( pre2p_gamename ) * 0.8)) + romseltextoffset, 260, 0.8, pre2p_gamename, colour );
			}			
			if (mCurrentSelection > 0)
			{
				colour = mpContext->GetDefaultTextColour();
				mpContext->DrawTextScale( 220 - (((mpContext->GetTextWidth( p_gamename ) * (0.8 + (romseltextscale * romselmenufs)) / 2)) + (mpContext->GetTextWidth( prevp_gamename ) * (1.2 - (romseltextscale * romselmenufs)))) + romseltextoffset, 260, 1.2 - (romseltextscale * romselmenufs), prevp_gamename, colour );
			}

			colour = mpContext->GetSelectedTextColour();
			mpContext->DrawTextScale( 240 - ((mpContext->GetTextWidth( p_gamename ) * (0.8 + (romseltextscale * romselmenufs)) / 2)) + romseltextoffset, 260, 0.8 + (romseltextscale * romselmenufs),p_gamename, colour );	
			
			if (mCurrentSelection < mRomsList.size() - 1)
			{
				colour = mpContext->GetDefaultTextColour();
				mpContext->DrawTextScale( 260 + ((mpContext->GetTextWidth( p_gamename ) * (0.8 + (romseltextscale * romselmenufs)) / 2)) + romseltextoffset, 260, 0.8,nextp_gamename, colour );
			}

			if (mCurrentSelection < mRomsList.size() - 2)
			{
				colour = mpContext->GetDefaultTextColour();
				mpContext->DrawTextScale( 280 + (((mpContext->GetTextWidth( p_gamename ) * (0.8 + (romseltextscale * romselmenufs)) / 2)) + ((mpContext->GetTextWidth( nextp_gamename )) * (0.8 + (romseltextscale * romselmenufs)))) + romseltextoffset, 260, 0.8, nex2p_gamename, colour );
			}
	
			romseltextoffset -= romseltextrepos;
		}
	 }	
	else if (mRomsList.size() == 1) {
		colour = mpContext->GetSelectedTextColour();
		mpContext->DrawTextScale( 240 - ((mpContext->GetTextWidth( p_gamename ) * 1.2) / 2), 260, 1.2, p_gamename, colour );	
	}
	else {
		if (mCurrentSelection > 1)
		{
			colour = mpContext->GetDefaultTextColour();
			mpContext->DrawTextScale( 200 - (((mpContext->GetTextWidth( p_gamename ) * 1.2) / 2) + (mpContext->GetTextWidth( prevp_gamename ) * 0.8) + (mpContext->GetTextWidth( pre2p_gamename ) * 0.8)), 260, 0.8, pre2p_gamename, colour );
		}
		if (mCurrentSelection > 0)
		{
			colour = mpContext->GetDefaultTextColour();
			mpContext->DrawTextScale( 220 - (((mpContext->GetTextWidth( p_gamename ) * 1.2) / 2) + (mpContext->GetTextWidth( prevp_gamename ) * 0.8)), 260, 0.8, prevp_gamename, colour );
		}

		colour = mpContext->GetSelectedTextColour();
		mpContext->DrawTextScale( 240 - ((mpContext->GetTextWidth( p_gamename ) * 1.2) / 2) ,260 , 1.2, p_gamename, colour );	
		
		if (mCurrentSelection < mRomsList.size() - 1)
		{
			colour = mpContext->GetDefaultTextColour();
			mpContext->DrawTextScale( 260 + ((mpContext->GetTextWidth( p_gamename ) * 1.2) / 2) , 260, 0.8, nextp_gamename, colour );
		}
		if (mCurrentSelection < mRomsList.size() - 2)
		{
			colour = mpContext->GetDefaultTextColour();
			mpContext->DrawTextScale( 280 + (((mpContext->GetTextWidth( p_gamename ) * 1.2) / 2) + (mpContext->GetTextWidth( nextp_gamename ) * 0.8)), 260, 0.8, nex2p_gamename, colour );
		}
	}
}
//*************************************************************************************
//
//*************************************************************************************
void IRomSelectorComponent::RenderCategoryList_old()
{
	s32 x = CATEGORY_AREA_LEFT;
	s32 y = CATEGORY_AREA_TOP + mpContext->GetFontHeight();

	ECategory current_category( GetCurrentCategory() );

	for( u32 i = 0; i < NUM_CATEGORIES; ++i )
	{
		ECategory	category = ECategory( i );
		c32			colour;

		AlphaMap::const_iterator it( mRomCategoryMap.find( category ) );
		if( it != mRomCategoryMap.end() )
		{
			if( current_category == category )
			{
				colour = mpContext->GetSelectedTextColour();
			}
			else
			{
				colour = mpContext->GetDefaultTextColour();
			}
		}
		else
		{
			colour = c32( 180, 180, 180 );
		}

		char str[ 16 ];
		sprintf( str, "%c ", GetCategoryLetter( category ) );
		x += mpContext->DrawText( x, y, str, colour );
	}
}

//*************************************************************************************
//
//*************************************************************************************
void IRomSelectorComponent::RenderCategoryList()
{
	s32 centerx = CATEGORY_AREA_LEFT;
	s32 x = CATEGORY_AREA_LEFT;
	s32 y = CATEGORY_AREA_TOP;
	char selstr[16];
	char str[16];
	float centerwidth = 0.0f;
	float catwidth = 0.0f;
	int centercategory = 1;	
	int prevcategory = 1;
	

	ECategory current_category( GetCurrentCategory() );
	if (romselmenufs < 31) {	
		if (!isnextset) {
			for( int i = 0; i < NUM_CATEGORIES; ++i ) {
				ECategory	category = ECategory( i );
				if ((i > -1) && (i < NUM_CATEGORIES)) {
					sprintf( str, " %c ", GetCategoryLetter( category ) );
					centerwidth += mpContext->GetTextWidth( str );
				}
				AlphaMap::const_iterator it( mRomCategoryMap.find( category ) );
				if ((it != mRomCategoryMap.end()) && ( current_category == category )) {
					sprintf( selstr, " %c ", GetCategoryLetter( category ) );
					prevcategory = i;
					break;
				}
			}
			if (romselmenudir == 1) {
				for( int i = prevcategory - 1; i > -1; --i )	{
					ECategory	category = ECategory( i );				
					AlphaMap::const_iterator it( mRomCategoryMap.find( category ) );
					if( it != mRomCategoryMap.end() ) {						
						sprintf( str, " %c ", GetCategoryLetter( category ) );	
						catwidth += mpContext->GetTextWidth( str );
						isnextset = 1;
						break;
					} else {
						sprintf( str, " %c ", GetCategoryLetter( category ) );
						catwidth += mpContext->GetTextWidth( str );
					}
				}
			} else if (romselmenudir == 2) {
				for( u32 i = prevcategory + 1; i < NUM_CATEGORIES; ++i ) {
					ECategory	category = ECategory( i );
					AlphaMap::const_iterator it( mRomCategoryMap.find( category ) );
					if( it != mRomCategoryMap.end() ) {						
						sprintf( str, " %c ", GetCategoryLetter( category ) );	
						catwidth += mpContext->GetTextWidth( str );
						isnextset = 1;
						break;
					} else {
						sprintf( str, " %c ", GetCategoryLetter( category ) );
						catwidth += mpContext->GetTextWidth( str );
					}
				}
			}
			if (romselmenudir == 1) {		
				// Search for the next valid predecessor
				while(current_category > 0)	{				
					current_category = ECategory( current_category - 1 );
					AlphaMap::const_iterator it( mRomCategoryMap.find( current_category ) );
					if ( it != mRomCategoryMap.end() ) {
						mCurrentSelection = it->second;		
						break;
					}
				}
				romseltextoffset = (240 - centerwidth) + 5;
				romseltextrepos = catwidth / 30.0f;
				romseltextscale = (0.4 / 30);
			}
			if (romselmenudir == 2) {		
				// Search for the next valid predecessor	
				while(current_category < NUM_CATEGORIES-1) {
					current_category = ECategory( current_category + 1 );
					AlphaMap::const_iterator it( mRomCategoryMap.find( current_category ) );
					if( it != mRomCategoryMap.end() ) {
						mCurrentSelection = it->second;		
						break;
					}
				}
				romseltextoffset = (240 - centerwidth) + 5;
				romseltextrepos = catwidth / 30.0f;
				romseltextscale = (0.4 / 30);
			}
		}
		if (romselmenudir == 1) { 						
			romseltextoffset += romseltextrepos;
			mpContext->DrawText(romseltextoffset, y, catstr, c32(180, 180, 180)); 
		}
		else if (romselmenudir == 2) { 	
			romseltextoffset -= romseltextrepos;
			mpContext->DrawText(romseltextoffset, y, catstr, c32(180, 180, 180)); 
		}

	} else {
		centerx = CATEGORY_AREA_LEFT;

		ECategory current_category( GetCurrentCategory() );
		for( u32 i = 0; i < NUM_CATEGORIES; ++i )
		{
			ECategory	category = ECategory( i );
			c32			colour;

			AlphaMap::const_iterator it( mRomCategoryMap.find( category ) );
			if ((it != mRomCategoryMap.end()) && ( current_category == category )) {
				colour = mpContext->GetSelectedTextColour();
				sprintf( str, " %c ", GetCategoryLetter( category ) );
				centercategory = i;
				centerx -= (mpContext->GetTextWidth( str ) / 2); 
				mpContext->DrawText( centerx, y, str, colour );
				break;
			}
		}

		x = centerx;

		for( int i = centercategory; i > -1; --i )
		{
			ECategory	category = ECategory( i );
			c32			colour;

			if( current_category == category ) { 
				sprintf( str, " %c ", GetCategoryLetter( category ) ); 
				continue; 
			}
			AlphaMap::const_iterator it( mRomCategoryMap.find( category ) );
			if( it != mRomCategoryMap.end() ) {
				colour = mpContext->GetDefaultTextColour();
			} else {
				colour = c32( 180, 180, 180 );
			}

			sprintf( str, " %c ", GetCategoryLetter( category ) );
			x -= mpContext->GetTextWidth(str);
			mpContext->DrawText( x, y, str, colour );
		}

		x = centerx;

		for( u32 i = centercategory; i < NUM_CATEGORIES; ++i )
		{
			ECategory	category = ECategory( i );
			c32			colour;
			
			if( current_category == category ) { 
				sprintf( str, " %c ", GetCategoryLetter( category ) ); 
				x += mpContext->GetTextWidth( str ); 
				continue; 
			}
			AlphaMap::const_iterator it( mRomCategoryMap.find( category ) );
			if( it != mRomCategoryMap.end() ) {			
				colour = mpContext->GetDefaultTextColour();
			} else {
				colour = c32( 180, 180, 180 );
			}

			sprintf( str, " %c ", GetCategoryLetter( category ) );
			x += mpContext->DrawText( x, y, str, colour );
		}
	}
}

//*************************************************************************************
//
//*************************************************************************************
void IRomSelectorComponent::Render_old()
{
	static u32 count=0;

	const char * const		message[] =
	{	"(X) -> Load",
		"([ ]) -> Settings",
		"(/\\) -> File Names",
		"(HOME) -> Quit",
		"(SELECT) -> Delete"};

	ICON_AREA_TOP = 32;
	ICON_AREA_LEFT = 10;
	ICON_AREA_WIDTH = 220;
	ICON_AREA_HEIGHT = 152;

	TEXT_AREA_TOP = 32;
	TEXT_AREA_LEFT = ICON_AREA_LEFT + ICON_AREA_WIDTH + 10;
	TEXT_AREA_WIDTH = 480 - TEXT_AREA_LEFT;
	TEXT_AREA_HEIGHT = 216;

	CATEGORY_AREA_TOP = TEXT_AREA_TOP + TEXT_AREA_HEIGHT + 5;
	CATEGORY_AREA_LEFT = ICON_AREA_LEFT;

	PREVIEW_SCROLL_WAIT = 0.075f;	// seconds to wait for scrolling to stop before loading preview (prevent thrashing)
	PREVIEW_FADE_TIME = 0.050f;		// seconds

	RenderPreview_old();

	if( mRomsList.empty() )
	{
		s32 offset( 0 );
		for( u32 i = 0; i < ARRAYSIZE( gNoRomsText ); ++i )
		{
			offset += mpContext->DrawTextArea( TEXT_AREA_LEFT, TEXT_AREA_TOP + offset, TEXT_AREA_WIDTH, TEXT_AREA_HEIGHT - offset, gNoRomsText[ i ], DrawTextUtilities::TextWhite, VA_TOP );
			offset += 4;
		}
	}
	else
	{
		RenderRomList_old();
	}

	RenderCategoryList_old();


	//Show tool tip
	c32 color;
	if(count & 0x80) color = c32( ~count<<1, 0, 0, 255);
	else color = c32( count<<1, 0, 0, 255);
	
	if(mRomDelete)
	{
		mpContext->DrawTextAlign(0,470,AT_RIGHT,CATEGORY_AREA_TOP + mpContext->GetFontHeight(),"(X) -> Confirm", color);
	}
	else
	{
		mpContext->DrawTextAlign(0,470,AT_RIGHT,CATEGORY_AREA_TOP + mpContext->GetFontHeight(),	message[(count >> 8) % ARRAYSIZE( message )], color);
	}
	
	count++;
}
//*************************************************************************************
//
//*************************************************************************************
void IRomSelectorComponent::Render()
{
	if(gGlobalPreferences.GuiType == CLASSIC)	//Use old menu if chosen
	{
		Render_old();
		return;
	}

	sceGuEnable(GU_SCISSOR_TEST);
	sceGuScissor(0,0, 480,272);

	ICON_AREA_TOP = 48;
	ICON_AREA_LEFT = 20;
	ICON_AREA_WIDTH = 256;
	ICON_AREA_HEIGHT = 177;

	TEXT_AREA_TOP = 70;
	TEXT_AREA_LEFT = 300;
	TEXT_AREA_WIDTH = 148;
	TEXT_AREA_HEIGHT = 216;

	CATEGORY_AREA_TOP = 255;
	CATEGORY_AREA_LEFT = 240;

	PREVIEW_SCROLL_WAIT = 0.65f;	// seconds to wait for scrolling to stop before loading preview (prevent thrashing)
	PREVIEW_FADE_TIME = 0.50f;		// seconds

	RenderPreview();

	if( mRomsList.empty() )	{
		s32 offset( 0 );
		for( u32 i = 0; i < ARRAYSIZE( gNoRomsText ); ++i )	{
			mpContext->DrawText(240 - (mpContext->GetTextWidth( gNoRomsText[ i ]) / 2 ), 75 + offset,
					gNoRomsText[ i ], c32::White );
			offset += 10;
			if ((i == 0) || (i == 4)) { offset += 10; }
		}
	} else if (!sortbyletter) {
		RenderRomList();
	} else if (sortbyletter) { 
		RenderCategoryList();
	}

	if(mRomDelete)
	{
		mpContext->DrawTextAlign(0,480,AT_CENTRE,242,"Press X to Confirm ROM Deletion",
					DrawTextUtilities::TextRed);
	}
	else
	{
		mpContext->DrawTextAlign(0,480,AT_CENTRE,242,"SELECT: Delete ROM",
					DrawTextUtilities::TextRed);
	}
	
}

//*************************************************************************************
//
//*************************************************************************************
void	IRomSelectorComponent::Update_old( float elapsed_time, const v2 & stick, u32 old_buttons, u32 new_buttons )
{
	static const float	SCROLL_RATE_PER_SECOND = 25.0f;		// 25 roms/second
	
	/*Apply stick deadzone preference in the RomSelector menu*/
	v2 stick_dead(ApplyDeadzone( stick, gGlobalPreferences.StickMinDeadzone, gGlobalPreferences.StickMaxDeadzone ));
	
	mSelectionAccumulator += stick_dead.y * SCROLL_RATE_PER_SECOND * elapsed_time; 
	
	/*Tricky thing to get the stick to work in every cases
	  for the 100/100 case for example
	  without it, the accumulator gets weirdly set to a NaN value and
	  everything is blocked... So it keeps the accumulator out of a NaN value.
	  */
	if( !(mSelectionAccumulator<0) && !(mSelectionAccumulator>0))
	  mSelectionAccumulator=0.0f;

	ECategory current_category( GetCurrentCategory() );

	u32				initial_selection( mCurrentSelection );

	mDisplayFilenames = (new_buttons & PSP_CTRL_TRIANGLE) != 0;
	
	mDisplayInfo = (new_buttons & PSP_CTRL_SQUARE) != 0;

	if(old_buttons != new_buttons)
	{
		if(new_buttons & PSP_CTRL_LEFT)
		{
			// Search for the next valid predecessor
			while(current_category > 0)
			{
				current_category = ECategory( current_category - 1 );
				AlphaMap::const_iterator it( mRomCategoryMap.find( current_category ) );
				if( it != mRomCategoryMap.end() )
				{
					mCurrentSelection = it->second;
					break;
				}
			}
		}

		if(new_buttons & PSP_CTRL_RIGHT)
		{
			// Search for the next valid predecessor
			while(current_category < NUM_CATEGORIES-1)
			{
				current_category = ECategory( current_category + 1 );
				AlphaMap::const_iterator it( mRomCategoryMap.find( current_category ) );
				if( it != mRomCategoryMap.end() )
				{
					mCurrentSelection = it->second;
					break;
				}
			}
		}

		if(new_buttons & PSP_CTRL_UP)
		{
			if(mCurrentSelection > 0)
			{
				mCurrentSelection--;
			}
		}

		if(new_buttons & PSP_CTRL_DOWN)
		{
			if(mCurrentSelection < mRomsList.size() - 1)
			{
				mCurrentSelection++;
			}
		}
		if(new_buttons & PSP_CTRL_HOME)
		{
#ifdef DAEDALUS_DIALOGS
			if(ShowMessage("Do you want to quit Daedalus?", 1))
			{
				sceKernelExitGame();
			}
#else
			sceKernelExitGame();
#endif
		}
		if(new_buttons & PSP_CTRL_CROSS && mRomDelete)	// DONT CHANGE ORDER
		{
			remove( mSelectedRom.c_str() );
			mRomDelete = false;
			UpdateROMList();
		} 
		else if((new_buttons & PSP_CTRL_START) ||
			(new_buttons & PSP_CTRL_CROSS))
		{
			if(mCurrentSelection < mRomsList.size())
			{
				mSelectedRom = mRomsList[ mCurrentSelection ]->mFilename;

				if(OnRomSelected != NULL)
				{
					(*OnRomSelected)( mSelectedRom.c_str() );
				}
			}
		}

		if(new_buttons != 0) mRomDelete = false; // DONT CHANGE ORDER clear it if any button has been pressed
		if(new_buttons & PSP_CTRL_SELECT)
		{
			if(mCurrentSelection < mRomsList.size())
			{
				mSelectedRom = mRomsList[ mCurrentSelection ]->mFilename;
				mRomDelete = true;
			}
		}
	}
	//
	//	Apply the selection accumulator
	//
	f32		current_vel( mSelectionAccumulator );
	while(mSelectionAccumulator >= 1.0f)
	{
		if(mCurrentSelection < mRomsList.size() - 1)
		{
			mCurrentSelection++;
		}
		mSelectionAccumulator -= 1.0f;
		mRomDelete = false;
	}
	while(mSelectionAccumulator <= -1.0f)
	{
		if(mCurrentSelection > 0)
		{
			mCurrentSelection--;
		}
		mSelectionAccumulator += 1.0f;
		mRomDelete = false;
	}

	//
	//	Scroll to keep things in view
	//	We add on 'current_vel * 2' to keep the selection highlight as close to the
	//	center as possible (as if we're predicting 2 frames ahead)
	//
	const u32		font_height( mpContext->GetFontHeight() );
	const u32		line_height( font_height + 2 );

	if( mRomsList.size() * line_height > TEXT_AREA_HEIGHT )
	{
		s32		current_selection_y = s32((mCurrentSelection + current_vel * 2) * line_height) + (line_height/2) + mCurrentScrollOffset;

		s32		adjust_amount( (TEXT_AREA_HEIGHT/2) - current_selection_y );

		float d( 1.0f - powf(0.993f, elapsed_time * 1000.0f) );

		u32		total_height( mRomsList.size() * line_height );
		s32		min_offset( TEXT_AREA_HEIGHT - total_height );

		s32	new_scroll_offset = mCurrentScrollOffset + s32(float(adjust_amount) * d);

		mCurrentScrollOffset = Clamp( new_scroll_offset, min_offset, s32(0) );
	}
	else
	{
		mCurrentScrollOffset = 0;
	}

	//
	//	Increase a timer is the current selection is still the same (i.e. if we've not scrolled)
	//
	if( initial_selection == mCurrentSelection )
	{
		mTimeSinceScroll += elapsed_time;
	}
	else
	{
		mTimeSinceScroll = 0;
	}

	//
	//	If the current selection is different from the preview, invalidate the picture.
	//	
	//
	if( mCurrentSelection < mRomsList.size() && mPreviewIdx != mCurrentSelection )
	{
		//mPreviewIdx = u32(-1);

		mPreviewLoadedTime -= elapsed_time;
		if(mPreviewLoadedTime < 0.0f)
			mPreviewLoadedTime = 0.0f;
	
		//
		//	If we've waited long enough since starting to scroll, try and load the preview image
		//	Note that it may fail, so we sort out the other flags regardless.
		//
		if( mTimeSinceScroll > PREVIEW_SCROLL_WAIT )
		{
			mpPreviewTexture = NULL;
			mPreviewLoadedTime = 0.0f;
			mPreviewIdx = mCurrentSelection;

			if( !mRomsList[ mCurrentSelection ]->mSettings.Preview.empty() )
			{
				char		preview_filename[ MAX_PATH + 1 ];
				IO::Path::Combine( preview_filename, gPreviewDirectory, mRomsList[ mCurrentSelection ]->mSettings.Preview.c_str() );
				
				mpPreviewTexture = CNativeTexture::CreateFromPng( preview_filename, TexFmt_8888 );
			}
		}
	}

	//
	//	Once the preview has been loaded, increase a timer to fade us in.
	//
	if( mPreviewIdx == mCurrentSelection )
	{
		mPreviewLoadedTime += elapsed_time;
		if(mPreviewLoadedTime > PREVIEW_FADE_TIME)
			mPreviewLoadedTime = PREVIEW_FADE_TIME;
	}
}
//*************************************************************************************
//
//*************************************************************************************
void	IRomSelectorComponent::Update( float elapsed_time, const v2 & stick, u32 old_buttons, u32 new_buttons )
{
	if(gGlobalPreferences.GuiType == CLASSIC)	//Use old menu if chosen
	{
		Update_old( elapsed_time, stick, old_buttons, new_buttons );
		return;
	}

	static const float	SCROLL_RATE_PER_SECOND = 25.0f;		// 25 roms/second	
	
	/*Apply stick deadzone preference in the RomSelector menu*/	
	v2 stick_dead(ApplyDeadzone( stick, gGlobalPreferences.StickMinDeadzone, gGlobalPreferences.StickMaxDeadzone ));
		
	if ((!(new_buttons & PSP_CTRL_LEFT)) && (!(new_buttons & PSP_CTRL_RIGHT))) {
		mSelectionAccumulator += stick_dead.x * SCROLL_RATE_PER_SECOND * elapsed_time; 
		
		/*Keeps the accumulator out of a NaN value. */
		if( !(mSelectionAccumulator<0) && !(mSelectionAccumulator>0))
		  mSelectionAccumulator=0.0f;
	} 
	ECategory current_category( GetCurrentCategory() );

	u32	initial_selection( mCurrentSelection );

	mDisplayFilenames = (new_buttons & PSP_CTRL_TRIANGLE) != 0;		

	mDisplayInfo = (new_buttons & PSP_CTRL_SQUARE) != 0;		

	if (new_buttons & PSP_CTRL_CIRCLE) 
	{	
		sortbyletter = 1;		
		if ((new_buttons & PSP_CTRL_LEFT) && !(old_buttons & PSP_CTRL_LEFT) && (!stick_dead.x)) {	
			// Search for the next valid predecessor
			for( int i = current_category - 1; i > -1; --i ) {
				ECategory	category = ECategory( i );
				AlphaMap::const_iterator it( mRomCategoryMap.find( category ) );
				if( it != mRomCategoryMap.end() )
				{					
					romselmenuani = 1;
					romselmenudir = 1;
					romseltextoffset = 0;
					isnextset = 0;
					break;
				}
			}
		}
		if ((new_buttons & PSP_CTRL_RIGHT) && !(old_buttons & PSP_CTRL_RIGHT) && (!stick_dead.x)) {	
			for( int i = current_category + 1; i < NUM_CATEGORIES - 1; ++i ) {
				ECategory	category = ECategory( i );
				AlphaMap::const_iterator it( mRomCategoryMap.find( category ) );
				if( it != mRomCategoryMap.end() )
				{					
					romselmenuani = 1;
					romselmenudir = 2;
					romseltextoffset = 0;
					isnextset = 0;
					break;
				}
			}
		}
	}
	else 
	{ 
		sortbyletter = 0; 
	}
			
	if (old_buttons != new_buttons)	
	{
		if (!(new_buttons & PSP_CTRL_CIRCLE)) 
		{
			if (new_buttons & PSP_CTRL_LEFT)
			{
				if ((mCurrentSelection > 0) && (!stick_dead.x))
				{
					mCurrentSelection--;
					romselmenuani = 1;
					romselmenudir = 1;
					romseltextoffset = 0;
				}
			}
			if(new_buttons & PSP_CTRL_RIGHT)
			{
				if ((mCurrentSelection < mRomsList.size() - 1) && (!stick_dead.x))
				{
					mCurrentSelection++;
					romselmenuani = 1;
					romselmenudir = 2;
					romseltextoffset = 0;
				}
			}
		}
		if(new_buttons & PSP_CTRL_HOME)
		{
#ifdef DAEDALUS_DIALOGS
			if(ShowMessage("Do you want to quit Daedalus?", 1))
			{
				sceKernelExitGame();
			}
#else
			sceKernelExitGame();
#endif
		}
		if(new_buttons & PSP_CTRL_CROSS && mRomDelete)	// DONT CHANGE ORDER
		{
			remove( mSelectedRom.c_str() );
			mRomDelete = false;
			UpdateROMList();
		}
		else if((new_buttons & PSP_CTRL_START) ||
			(new_buttons & PSP_CTRL_CROSS))
		{
			if(mCurrentSelection < mRomsList.size())
			{
				mSelectedRom = mRomsList[ mCurrentSelection ]->mFilename;

				if(OnRomSelected != NULL)
				{
					(*OnRomSelected)( mSelectedRom.c_str() );
				}
			}
		}	

		if(new_buttons != 0) mRomDelete = false; // DONT CHANGE ORDER clear it if any button has been pressed
		if(new_buttons & PSP_CTRL_SELECT)
		{
			if(mCurrentSelection < mRomsList.size())
			{
				mSelectedRom = mRomsList[ mCurrentSelection ]->mFilename;
				mRomDelete = true;
			}
		}

	}
	
	//
	//	Apply the selection accumulator
	//
	f32		current_vel( mSelectionAccumulator );
	if ((!(new_buttons & PSP_CTRL_LEFT)) && (!(new_buttons & PSP_CTRL_RIGHT))) {
		while(mSelectionAccumulator >= 1.0f)
		{
			if(mCurrentSelection < mRomsList.size() - 1)
			{
				mCurrentSelection++;
			}
			mSelectionAccumulator -= 1.0f;
			mRomDelete = false;
		}
		while(mSelectionAccumulator <= -1.0f)
		{
			if(mCurrentSelection > 0)
			{
				mCurrentSelection--;
			}
			mSelectionAccumulator += 1.0f;
			mRomDelete = false;
		}
	}

	//
	//	Scroll to keep things in view
	//	We add on 'current_vel * 2' to keep the selection highlight as close to the
	//	center as possible (as if we're predicting 2 frames ahead)
	//
	const u32		font_height( mpContext->GetFontHeight() );
	const u32		line_height( font_height + 2 );

	if( mRomsList.size() * line_height > TEXT_AREA_HEIGHT )
	{
		s32		current_selection_y = s32((mCurrentSelection + current_vel * 2) * line_height) + (line_height/2) + mCurrentScrollOffset;

		s32		adjust_amount( (TEXT_AREA_HEIGHT/2) - current_selection_y );

		f32 d( 1.0f - vfpu_powf(0.993f, elapsed_time * 1000.0f) );

		u32		total_height( mRomsList.size() * line_height );
		s32		min_offset( TEXT_AREA_HEIGHT - total_height );

		s32	new_scroll_offset = mCurrentScrollOffset + s32(float(adjust_amount) * d);

		mCurrentScrollOffset = Clamp( new_scroll_offset, min_offset, s32(0) );
	}
	else
	{
		mCurrentScrollOffset = 0;
	}

	//
	//	Increase a timer is the current selection is still the same (i.e. if we've not scrolled)
	//
	if( initial_selection == mCurrentSelection )
	{
		mTimeSinceScroll += elapsed_time;
	}
	else
	{
		mTimeSinceScroll = 0;
	}

	//
	//	If the current selection is different from the preview, invalidate the picture.
	//	
	//
	if( mCurrentSelection < mRomsList.size() && mPreviewIdx != mCurrentSelection )
	{
		//mPreviewIdx = u32(-1);

		mPreviewLoadedTime -= elapsed_time;
		if(mPreviewLoadedTime < 0.0f)
			mPreviewLoadedTime = 0.0f;
	
		//
		//	If we've waited long enough since starting to scroll, try and load the preview image
		//	Note that it may fail, so we sort out the other flags regardless.
		//
		if( mTimeSinceScroll > PREVIEW_SCROLL_WAIT )
		{
			mpPreviewTexture = NULL;
			mPreviewLoadedTime = 0.0f;
			mPreviewIdx = mCurrentSelection;

			if( !mRomsList[ mCurrentSelection ]->mSettings.Preview.empty() )
			{
				char		preview_filename[ MAX_PATH + 1 ];
				IO::Path::Combine( preview_filename, gPreviewDirectory, mRomsList[ mCurrentSelection ]->mSettings.Preview.c_str() );
				
				mpPreviewTexture = CNativeTexture::CreateFromPng( preview_filename, TexFmt_8888 );
			}
		}
	}

	//
	//	Once the preview has been loaded, increase a timer to fade us in.
	//
	if( mPreviewIdx == mCurrentSelection )
	{
		mPreviewLoadedTime += elapsed_time;
		if(mPreviewLoadedTime > PREVIEW_FADE_TIME)
			mPreviewLoadedTime = PREVIEW_FADE_TIME;
	}
}
