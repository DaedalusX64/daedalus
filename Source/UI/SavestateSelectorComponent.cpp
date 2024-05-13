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


#include "Base/Types.h"


#include <stdio.h>
#include <filesystem>
#include <chrono>
#include <format>

#include "Core/ROM.h"
#include "Interface/SaveState.h"
#include "Graphics/NativeTexture.h"
#include "Math/Vector2.h"
#include "DrawTextUtilities.h"
#include "UICommand.h"
#include "UIContext.h"
#include "UIElement.h"
#include "UIScreen.h"
#include "PSPMenu.h"
#include "SavestateSelectorComponent.h"
#include "System/IO.h"
#include "Utility/Stream.h"
#include "Utility/Translate.h"


class ISavestateSelectorComponent : public CSavestateSelectorComponent
{
	public:

		ISavestateSelectorComponent( CUIContext * p_context, EAccessType accetype, std::function<void (const char *)> on_slot_selected, const char *running_rom );
		~ISavestateSelectorComponent();

		// CUIScreen
		virtual void				Update( float elapsed_time, const v2 & stick, u32 old_buttons, u32 new_buttons );
		virtual void				Render();
		virtual bool				IsFinished() const									{ return mIsFinished; }
	public:
		IO::Filename			current_slot_path;
		bool					isGameRunning;

	private:
		void				OnSlotSelected( u32 slot_idx );
		void				OnFolderSelected( u32 index );
		void				LoadFolders();
		void				LoadSlots();
		void				deleteSlot(u32 id_ss);
		bool					isDeletionAllowed;

	private:
		EAccessType				mAccessType;
		std::function<void (const char *)>	mOnSlotSelected;

		u32					mSelectedSlot;
		bool					mIsFinished;
		bool	deleteButtonTriggered;

		CUIElementBag				mElements;
		std::vector<std::string> 		mElementTitle;
		bool					mSlotEmpty[ NUM_SAVESTATE_SLOTS ];
		IO::Filename			mPVFilename[ NUM_SAVESTATE_SLOTS ];
		s8						mPVExists[ NUM_SAVESTATE_SLOTS ];	//0=skip, 1=file exists, -1=show no preview
		std::shared_ptr<CNativeTexture>	mPreviewTexture;
		u32						mLastPreviewLoad;

};


CSavestateSelectorComponent::~CSavestateSelectorComponent() {}


CSavestateSelectorComponent::CSavestateSelectorComponent( CUIContext * p_context )
:	CUIComponent( p_context )
{}


CSavestateSelectorComponent *	CSavestateSelectorComponent::Create( CUIContext * p_context, EAccessType accetype, std::function<void(const char *)> on_slot_selected, const char *running_rom )
{
	return new ISavestateSelectorComponent( p_context, accetype, on_slot_selected, running_rom );
}

namespace
{
	void MakeSaveSlotPath(char * path, char * png_path, u32 slot_idx, char *slot_path )
	{
		std::filesystem::create_directory("SaveStates");
		IO::Filename	filename_png;
		IO::Filename	filename_ss;
		IO::Filename    sub_path;
		std::filesystem::path gDaedalusExePath = std::filesystem::current_path();
		snprintf( filename_png, sizeof(filename_png), "saveslot%u.ss.png", slot_idx );
		snprintf( filename_ss, sizeof(filename_ss), "saveslot%u.ss", slot_idx );
		snprintf( sub_path, sizeof(sub_path), "SaveStates/%s", slot_path);
		if(!std::filesystem::is_directory( "ms0:/n64/SaveStates/" ))
		{
			IO::Path::Combine( path, gDaedalusExePath.string().c_str(), sub_path );
			IO::Path::Combine( png_path, gDaedalusExePath.string().c_str(), sub_path );
			std::filesystem::exists(path);	// Ensure this dir exists
		}
		else
		{
			IO::Path::Combine( path, "ms0:/n64/", sub_path );
			IO::Path::Combine( png_path, "ms0:/n64/", sub_path );
			std::filesystem::exists(path);	// Ensure this dir exists
		}
		IO::Path::Append( path, filename_ss );
		IO::Path::Append( png_path, filename_png );
	}
}

ISavestateSelectorComponent::ISavestateSelectorComponent( CUIContext * p_context, EAccessType accetype, std::function<void(const char *)> on_slot_selected, const char *running_rom )
:	CSavestateSelectorComponent( p_context )
,	mAccessType( accetype )
,	mOnSlotSelected( on_slot_selected )
,	mSelectedSlot( INVALID_SLOT )
,	mIsFinished( false )
,	deleteButtonTriggered(false)
{


	if(running_rom){
		isGameRunning = true;
		strcpy(current_slot_path, running_rom);
		LoadSlots();
		isDeletionAllowed=true;
	} else {
		isGameRunning = false;
		LoadFolders();
		isDeletionAllowed=false;
	}
}

void ISavestateSelectorComponent::LoadFolders(){
	IO::FindHandleT		find_handle;
	IO::FindDataT		find_data;
	u32 i = 0;
	const char * description_text = mAccessType == AT_SAVING ? "Select the slot in which to save" : "Select the slot from which to load";
	IO::Filename full_path;
	// We're using the same vector for directory names and slots, so we have to clear it
	mElements.Clear();
	for( u32 i = 0; i < NUM_SAVESTATE_SLOTS; ++i)
	{
		mPVExists[ i ] = 0;
		mLastPreviewLoad = ~0 ;
}

	if (IO::FindFileOpen( "ms0:/n64/SaveStates" , &find_handle, find_data))
	{
		do
		{
			IO::Path::Combine( full_path, "ms0:/n64/SaveStates", find_data.Name);
			if(std::filesystem::is_directory(full_path) && strlen( find_data.Name ) > 2 )
			{
				COutputStringStream str;
				// CUIElement * element;
				str << find_data.Name;
				auto element = new CUICommandImpl( [this, &i]() { OnFolderSelected(i++); }, str.c_str(), description_text );
				mElements.Add( element );
				mElementTitle.push_back(find_data.Name);
			}
		}
		while(IO::FindFileNext( find_handle, find_data ));
		IO::FindFileClose( find_handle );
	}
	else if(IO::FindFileOpen( "SaveStates" , &find_handle, find_data))
	{
		do
		{
			IO::Path::Combine( full_path, "SaveStates", find_data.Name);
			if(std::filesystem::is_directory(full_path) && strlen( find_data.Name ) > 2 )
			{
				COutputStringStream str;
				// CUIElement *element;
				str << find_data.Name;
			auto	element = new CUICommandImpl( [this, &i]() { OnFolderSelected(i++); }, str.c_str(), description_text );
				mElements.Add( element );
				mElementTitle.push_back(find_data.Name);
			}
		}
		while(IO::FindFileNext( find_handle, find_data ));
		IO::FindFileClose( find_handle );
	}
	else
	{
		auto element = new CUICommandDummy( "There are no Savestates to load", "There are no Savestates to load" );
		mElements.Add( element );
	}

}

void ISavestateSelectorComponent::LoadSlots(){
	const char * description_text = mAccessType == AT_SAVING ? "Select the slot in which to save [X:save O:back]" : "Select the slot from which to load [X:load O:back []:delete]";
	char date_string[30];
	// We're using the same vector for directory names and slots, so we have to clear it
	mElements.Clear();
	mLastPreviewLoad = ~0;

	for( auto i = 0; i < NUM_SAVESTATE_SLOTS; ++i )
	{
		COutputStringStream		str;
		str << Translate_String("Slot ") << static_cast<u32>(i+1) << ": ";

		IO::Filename filename_ss;
		MakeSaveSlotPath( filename_ss, mPVFilename[ i ], i, current_slot_path);
		mPVExists[ i ] = std::filesystem::exists( mPVFilename[ i ] ) ? 1 : -1;
		RomID	rom_id( SaveState_GetRomID( filename_ss ) );
		RomSettings	settings;
		CUIElement *element;
		if( !rom_id.Empty() && CRomSettingsDB::Get()->GetSettings( rom_id, &settings ) )
		{
            // Get the last write time of the file
            auto last_write_time = std::filesystem::last_write_time(filename_ss);

            // Convert the time duration to a system_clock::time_point
            std::chrono::system_clock::time_point time_point =
                std::chrono::time_point<std::chrono::system_clock>(std::chrono::duration_cast<std::chrono::system_clock::duration>(last_write_time.time_since_epoch()));

            // Convert the time_point to a time_t to use with std::localtime
            std::time_t tt = std::chrono::system_clock::to_time_t(time_point);

            // Convert the time_t to a local time and format it
            std::tm* timeinfo = std::localtime(&tt);
            
            // Format the date string
            std::strftime(date_string, sizeof(date_string), "%m/%d/%Y %H:%M:%S", timeinfo);

			str << date_string;
			mSlotEmpty[ i ] = false;
		}
		else
		{
			str << Translate_String("Empty");
			mSlotEmpty[ i ] = true;
		}

		//
		//	Don't allow empty slots to be loaded
		//
		if( mAccessType == AT_LOADING && mSlotEmpty[ i ] )
		{
			element = new CUICommandDummy( str.c_str(), description_text );
		}
		else
		{

			element = new CUICommandImpl( [this, &i]() { OnSlotSelected(i++); }, str.c_str(), description_text );
		}

		mElements.Add( element );
	}
}


ISavestateSelectorComponent::~ISavestateSelectorComponent() {}


void	ISavestateSelectorComponent::Update( float elapsed_time, const v2 & stick, u32 old_buttons, u32 new_buttons )
{
	//
	//	Trigger the save on the first update AFTER mSelectedSlot was set.
	//	This ensures we get at least one frame where we can display "Saving..." etc.
	//
	if( mSelectedSlot != INVALID_SLOT && !mIsFinished )
	{
		mIsFinished = true;

		IO::Filename filename_ss;
		IO::Filename filename_png;
		MakeSaveSlotPath( filename_ss, filename_png, mSelectedSlot, current_slot_path );

		mOnSlotSelected( filename_ss );
	}

	if(old_buttons != new_buttons)
	{
	  if(mAccessType == AT_LOADING && deleteButtonTriggered)
		{
		  if( new_buttons & PSP_CTRL_TRIANGLE )
			{
				deleteSlot(mElements.GetSelectedIndex());
			}
		}

		if( new_buttons & PSP_CTRL_UP )
		{
			mElements.SelectPrevious();
			if(mAccessType == AT_LOADING)
			    deleteButtonTriggered=false;
		}
		if( new_buttons & PSP_CTRL_DOWN )
		{
			mElements.SelectNext();
			if(mAccessType == AT_LOADING)
			    deleteButtonTriggered=false;
		}

		auto element = mElements.GetSelectedElement();
		if( element != NULL )
		{
			if( new_buttons & PSP_CTRL_LEFT )
			{
				element->OnPrevious();
			}
			if( new_buttons & PSP_CTRL_RIGHT )
			{
				element->OnNext();
			}
			if( new_buttons & (PSP_CTRL_CROSS|PSP_CTRL_START) )
			{
				// Commit settings
				element->OnSelected();
			}
			if( new_buttons & PSP_CTRL_SQUARE)
			{
				//delete savestate
				if(mAccessType == AT_LOADING && isDeletionAllowed)
				{
				  if ((mElements.GetSelectedElement())->IsSelectable())
				    deleteButtonTriggered=true;
				}

			}
			if( new_buttons & (PSP_CTRL_CIRCLE|PSP_CTRL_SELECT) )
			{
				// Discard settings
				deleteButtonTriggered=false;
				if(isGameRunning == false)
				{
				  LoadFolders();
				  isDeletionAllowed=false;
				}
				else
					mIsFinished = true;
			}
		}
	}
}


void	ISavestateSelectorComponent::deleteSlot(u32 id_ss)
{
    IO::Filename	path;
    IO::Filename	png_path;
    IO::Filename	filename_ss;
    IO::Filename	filename_png;
    IO::Filename	sub_path;
    snprintf( filename_ss, sizeof(filename_ss), "saveslot%u.ss", id_ss );
    snprintf( filename_png, sizeof(filename_png), "saveslot%u.png", id_ss );
    snprintf( sub_path, sizeof(sub_path), "SaveStates/%s", current_slot_path);
			std::filesystem::path gDaedalusExePath = std::filesystem::current_path();
	if(!std::filesystem::is_directory( "ms0:/n64/SaveStates/" ))
	{
		IO::Path::Combine( path, gDaedalusExePath.string().c_str(), sub_path );
		IO::Path::Combine( png_path, gDaedalusExePath.string().c_str(), sub_path );
	}
	else
	{
		IO::Path::Combine( path, "ms0:/n64/", sub_path );
		IO::Path::Combine( png_path, "ms0:/n64/", sub_path );
	}
	IO::Path::Append( path, filename_ss );
	IO::Path::Append( png_path, filename_png );

	if (std::filesystem::exists(path))
    {
      remove(path);
      deleteButtonTriggered=false;
      LoadSlots();
    }

	if (std::filesystem::exists(png_path))
    {
      remove(png_path);
      deleteButtonTriggered=false;
      LoadSlots();
    }
}
void	ISavestateSelectorComponent::Render()
{
	const u32	font_height = mpContext->GetFontHeight();

	if( mSelectedSlot == INVALID_SLOT )
	{
		mElements.Draw( mpContext, LIST_TEXT_LEFT, LIST_TEXT_WIDTH, AT_LEFT, BELOW_MENU_MIN - mElements.GetSelectedIndex()*(font_height+2) );

		auto	element = mElements.GetSelectedElement();
		if( element != NULL )
		{

			if( mPVExists[ mElements.GetSelectedIndex() ] == 1 )
			{
				v2	tl( PREVIEW_IMAGE_LEFT+2, BELOW_MENU_MIN+2 );
				v2	wh( PREVIEW_IMAGE_WIDTH-4, PREVIEW_IMAGE_HEIGHT-4 );

				if( mPreviewTexture == NULL || mElements.GetSelectedIndex() != mLastPreviewLoad )
				{
					mPreviewTexture = CNativeTexture::CreateFromPng( mPVFilename[ mElements.GetSelectedIndex() ], TexFmt_8888 );
					mLastPreviewLoad = mElements.GetSelectedIndex();
				}

				mpContext->DrawRect( PREVIEW_IMAGE_LEFT, BELOW_MENU_MIN, PREVIEW_IMAGE_WIDTH, PREVIEW_IMAGE_HEIGHT, c32::White );
				mpContext->RenderTexture( mPreviewTexture, tl, wh, c32::White );
			}
			else if( mPVExists[ mElements.GetSelectedIndex() ] == -1 && mElements.GetSelectedIndex() < NUM_SAVESTATE_SLOTS )
			{
				mpContext->DrawRect( PREVIEW_IMAGE_LEFT, BELOW_MENU_MIN, PREVIEW_IMAGE_WIDTH, PREVIEW_IMAGE_HEIGHT, c32::White );
				mpContext->DrawRect( PREVIEW_IMAGE_LEFT+2, BELOW_MENU_MIN+2, PREVIEW_IMAGE_WIDTH-4, PREVIEW_IMAGE_HEIGHT-4, c32::Black );
				mpContext->DrawTextAlign( PREVIEW_IMAGE_LEFT, PREVIEW_IMAGE_LEFT + PREVIEW_IMAGE_WIDTH, AT_CENTRE, BELOW_MENU_MIN+PREVIEW_IMAGE_HEIGHT/2, "No Preview Available", c32::White );
			}

			const char *p_description( element->GetDescription() );
			mpContext->DrawTextArea( DESCRIPTION_AREA_LEFT,
									 DESCRIPTION_AREA_TOP,
									 DESCRIPTION_AREA_RIGHT - DESCRIPTION_AREA_LEFT,
									 DESCRIPTION_AREA_BOTTOM - DESCRIPTION_AREA_TOP,
									 p_description,
									 DrawTextUtilities::TextWhite,
									 VA_BOTTOM );
		}
	}
	else
	{
		const char * title_text( mAccessType == AT_SAVING ? SAVING_STATUS_TEXT : LOADING_STATUS_TEXT );

		s32 y = ( mpContext->GetScreenHeight() - font_height ) / 2 + font_height;
		mpContext->DrawTextAlign( 0, mpContext->GetScreenWidth(), AT_CENTRE, y, title_text, mpContext->GetDefaultTextColour() );
	}

	if(deleteButtonTriggered)
	  mpContext->DrawTextAlign(0,480,AT_CENTRE,135,"Press Triangle to delete this savestate",DrawTextUtilities::TextRed,DrawTextUtilities::TextWhite);
}


void	ISavestateSelectorComponent::OnSlotSelected( u32 slot_idx )
{
	if( slot_idx >= NUM_SAVESTATE_SLOTS )
		return;

	// Don't allow empty slots to be loaded
	if( mAccessType == AT_LOADING && mSlotEmpty[ slot_idx ] )
		return;

	mSelectedSlot = slot_idx;
}

void	ISavestateSelectorComponent::OnFolderSelected( u32 index )
{
	strcpy( current_slot_path, mElementTitle[index].c_str());
	mElementTitle.clear();
	LoadSlots();
	isDeletionAllowed=true;
}
