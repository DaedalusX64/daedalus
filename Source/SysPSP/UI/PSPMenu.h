#include "Utility/DaedalusTypes.h"
#include "SysPSP/Utility/PathsPSP.h"


// New UI values
const s16 SCREEN_WIDTH {480};
const s16 SCREEN_HEIGHT {272};


// to do adjust values to suit multiple screens
const s16 MENU_HEADER_X {13};
const s16 MENU_HEADER_Y {394};
const s16 MENU_HEADER_LINE_XY {15};
const s16 BELOW_MENU_MIN {33};

const s16 LIST_TEXT_LEFT {13};
const s16 LIST_TEXT_WIDTH {SCREEN_WIDTH - LIST_TEXT_LEFT};
const s16 LIST_TEXT_HEIGHT {216};



const s16 PREVIEW_IMAGE_LEFT {309};
const s16 PREVIEW_IMAGE_BOTTOM {140};
const s16 PREVIEW_IMAGE_RIGHT {464};
const s16 PREVIEW_IMAGE_WIDTH {PREVIEW_IMAGE_RIGHT - PREVIEW_IMAGE_LEFT};
const s16 PREVIEW_IMAGE_HEIGHT {PREVIEW_IMAGE_BOTTOM - BELOW_MENU_MIN};

const s16 ROM_INFO_TEXT_X {318};
const s16 ROM_INFO_TEXT_Y {154};


// Old values
const u32				SAVESTATE_ICON_AREA_LEFT = 240;
const u32				SAVESTATE_ICON_AREA_WIDTH = 220;
const u32				SAVESTATE_ICON_AREA_HEIGHT = 136;

const u32		TITLE_AREA_TOP = 10;
const u32				PAUSE_TEXT_AREA_TOP = 10;
const u32				PAUSE_TEXT_AREA_LEFT = 20;
const u32				PAUSE_TEXT_AREA_RIGHT = 460;


const u32				SS_TEXT_AREA_TOP = 272 / 2;

const s32				SS_DESCRIPTION_AREA_TOP = 272-20;		// We render text aligned from the bottom, so this is largely irrelevant


const u32		TEXT_AREA_TOP = 40;
const u32		TEXT_AREA_LEFT = 40;
const u32		TEXT_AREA_RIGHT = 440;
const s32		TEXT_AREA_BOTTOM = 272 - 10;



const s32		DESCRIPTION_AREA_TOP = 0;		// We render text aligned from the bottom, so this is largely irrelevant
const s32		DESCRIPTION_AREA_BOTTOM = 272-10;
const s32		DESCRIPTION_AREA_LEFT = 16;
const s32		DESCRIPTION_AREA_RIGHT = 480-16;

const u32		GLOBALSETTINGS_TEXT_AREA_TOP = 47;


const char * const		SAVING_TITLE_TEXT  = "Select a Slot to Save To";
const char * const		LOADING_TITLE_TEXT = "Select a Slot to Load From";


// About components
#define MAX_PSP_MODEL 6

const char * const DAEDALUS_VERSION_TEXT = "DaedalusX64 Revision ";
//const char * const DAEDALUS_VERSION_TEXT = "DaedalusX64 Beta 3 Update";

const char * const		DATE_TEXT = "Built ";

const char * const		INFO_TEXT[] =
{
  "Copyright (C) 2008-2012 DaedalusX64 Team",
  "Copyright (C) 2001-2009 StrmnNrmn",
  "Audio HLE code by Azimer",
  "",
  "For news and updates visit:",
};

const char * const		pspModel[ MAX_PSP_MODEL ] =
{
  "PSP PHAT", "PSP SLIM", "PSP BRITE", "PSP BRITE", "PSP GO", "UNKNOWN PSP"
};

const char * const		URL_TEXT_1 = "http://DaedalusX64.com/";
const char * const		URL_TEXT_2 = "http://sf.net/projects/daedalusx64/";

const char * const		LOGO_FILENAME = DAEDALUS_PSP_PATH( "Resources/logo.png" );


// Adjust Dead Zone Screen


const char * const			INSTRUCTIONS_TEXT = "Adjust the minimum and maximum deadzone regions. Up/Down: Increase or decrease the deadzone. Left/Right: Select minimum or maximum deadzone for adjusting. Triangle: Reset to defaults. Start/X: Confirm. Select/Circle: Cancel";
const char * const		TITLE_TEXT = "Adjust Stick Deadzone";

const u32				TITLE_Y = 10;

const u32				HALF_WIDTH( 480 / 2 );
const u32				CENTRE_X( 480 / 2 );
const u32				DISPLAY_WIDTH( 128 );
const u32				DISPLAY_RADIUS( DISPLAY_WIDTH / 2 );

const u32				PSP_CIRCLE_X = DISPLAY_RADIUS + ((HALF_WIDTH - DISPLAY_WIDTH) / 2);
const u32				PSP_CIRCLE_Y = 120;

const u32				N64_CIRCLE_X = CENTRE_X + DISPLAY_RADIUS + ((HALF_WIDTH - DISPLAY_WIDTH) / 2);
const u32				N64_CIRCLE_Y = 120;

const u32				PSP_TITLE_X = PSP_CIRCLE_X - DISPLAY_RADIUS;
const u32				PSP_TITLE_Y = PSP_CIRCLE_Y - DISPLAY_RADIUS - 16;
const u32				N64_TITLE_X = N64_CIRCLE_X - DISPLAY_RADIUS;
const u32				N64_TITLE_Y = N64_CIRCLE_Y - DISPLAY_RADIUS - 16;

const f32				DEADZONE_INCREMENT = 0.01f;

const f32				DEFAULT_MIN_DEADZONE = 0.28f;		// Kind of gross - share somehow with IInputManager?
const f32				DEFAULT_MAX_DEADZONE = 1.0f;


enum EMenuOption
{
  MO_GLOBAL_SETTINGS = 0,
  MO_PAUSE_OPTIONS,
  MO_ABOUT,
  NUM_MENU_OPTIONS,
};

const EMenuOption	MO_FIRST_OPTION = MO_GLOBAL_SETTINGS;
const EMenuOption	MO_LAST_OPTION = MO_ABOUT;

const char * const	gMenuOptionNames[ NUM_MENU_OPTIONS ] =
{
  "Global Settings",
  "Paused",
  "About",
};

// Rom Selector component

const char * const		gRomsDirectories[] =
{
  "ms0:/n64/",
  DAEDALUS_PSP_PATH( "Roms/" ),
#ifndef DAEDALUS_SILENT
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

// Savestate Picture component
const u32				SS_ICON_AREA_LEFT = 480/2;

const u32				ICON_AREA_TOP = 32;
const u32				ICON_AREA_LEFT = 5;
const u32				ICON_AREA_WIDTH = 256;
const u32				ICON_AREA_HEIGHT = 177;

const u32       SAVESTATE_TEXT_AREA_LEFT = 20;
const u32       SAVESTATE_TEXT_AREA_RIGHT = 460;
const u32       SAVESTATE_TEXT_AREA_TOP = SCREEN_HEIGHT / 2;



//const u32				TEXT_AREA_TOP = 32;
const u32				IMAGE_TEXT_AREA_LEFT = ICON_AREA_LEFT + ICON_AREA_WIDTH + 5;
const u32				IMAGE_TEXT_AREA_WIDTH = 480 - TEXT_AREA_LEFT;
const u32				TEXT_AREA_HEIGHT = 216;

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

const u32				CATEGORY_AREA_TOP = TEXT_AREA_TOP + TEXT_AREA_HEIGHT + 5;
const u32				CATEGORY_AREA_LEFT = ICON_AREA_LEFT;

const char * const		gPreviewDirectory = DAEDALUS_PSP_PATH( "Resources/Preview/" );

const f32				PREVIEW_SCROLL_WAIT = 0.075f;		// seconds to wait for scrolling to stop before loading preview (prevent thrashing)
const f32				PREVIEW_FADE_TIME = 0.050f;			// seconds


// Save State selector

const char * const		SAVING_STATUS_TEXT  = "Saving...";
const char * const		LOADING_STATUS_TEXT = "Loading...";


const u32				NUM_SAVESTATE_SLOTS = 64;

const u32				INVALID_SLOT = u32( -1 );
