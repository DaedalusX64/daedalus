/*
Copyright (C) 2001 CyRUS64 (http://www.boob.co.uk)
Copyright (C) 2006,2007 StrmnNrmn

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
#include "Interface/Preferences.h"

#include <fstream>

#include <string>
#include <set>
#include <format>
#include <map>

#include "Utility/IniFile.h"
#include "Core/FramerateLimiter.h"

#include "Config/ConfigOptions.h"
#include "Core/ROM.h"
#include "Input/InputManager.h"
#include "Interface/RomDB.h"
#include "Utility/Paths.h"


#include "Utility/Translate.h"

// Audio is disabled on the PSP by default, but enabled on other platforms.
#ifdef DAEDALUS_PSP
static const EAudioPluginMode      kDefaultAudioPluginMode      = APM_DISABLED;
static const ETextureHashFrequency kDefaultTextureHashFrequency = THF_DISABLED;
#else
static const EAudioPluginMode      kDefaultAudioPluginMode = APM_ENABLED_SYNC;
static const ETextureHashFrequency kDefaultTextureHashFrequency = THF_EVERY_FRAME;
#endif

static u32						GetTexureHashFrequencyAsFrames( ETextureHashFrequency thf );
static ETextureHashFrequency	GetTextureHashFrequencyFromFrames( u32 frames );

static u32						GetFrameskipValueAsInt( EFrameskipValue value );
static EFrameskipValue			GetFrameskipValueFromInt( u32 value );


extern EFrameskipValue			gFrameskipValue;
extern f32 						gZoomX;

SGlobalPreferences				gGlobalPreferences;

class IPreferences : public CPreferences
{
	public:
		IPreferences();
		virtual ~IPreferences();

		bool					OpenPreferencesFile( const std::filesystem::path &filename );
		void					Commit();

		bool					GetRomPreferences( const RomID & id, SRomPreferences * preferences ) const;
		void					SetRomPreferences( const RomID & id, const SRomPreferences & preferences );

	private:
		void					OutputSectionDetails( const RomID & id, const SRomPreferences & preferences, std::ofstream& fh );

	private:
	using PreferencesMap = std::map<RomID, SRomPreferences>;

		PreferencesMap			mPreferences;

		bool					mDirty;				// (STRMNNRMN - Changed since read from disk?)
		std::filesystem::path	mFilename;
};

template<> bool	CSingleton< CPreferences >::Create()
{
	#ifdef DAEDALUS_ENABLE_ASSERTS
	DAEDALUS_ASSERT_Q(mpInstance == nullptr);
#endif
	mpInstance = std::make_shared<IPreferences>();

	return true;
}

CPreferences::~CPreferences()
{
}
#include <filesystem>
IPreferences::IPreferences()
:	mDirty( false )
{
	OpenPreferencesFile( setBasePath("Preferences.ini"));
}

IPreferences::~IPreferences()
{
	if ( mDirty )
	{
		Commit();
	}
}

static RomID RomIDFromString( const char * str )
{
	u32 crc1, crc2, country;
	sscanf( str, "%08x%08x-%02x", &crc1, &crc2, &country );
	return RomID( crc1, crc2, (u8)country );
}

bool IPreferences::OpenPreferencesFile( const std::filesystem::path  &filename )
{
	mFilename = filename;

	auto p_ini_file = CIniFile::Create( filename ) ;
	// CIniFile * p_ini_file( CIniFile::Create( filename ) );
	if( p_ini_file == NULL )
	{
		return false;
	}

	const CIniFileSection *	section( p_ini_file->GetDefaultSection() );
	if( section != NULL )
	{
		const CIniFileProperty * property;

#define BOOL_SETTING( b, nm, def )	if( section->FindProperty( #nm, &property ) ) { b.nm = property->GetBooleanValue( def.nm ); }
#define INT_SETTING( b, nm, def )	if( section->FindProperty( #nm, &property ) ) {	b.nm = property->GetIntValue( def.nm ); }
#define FLOAT_SETTING( b, nm, def ) if( section->FindProperty( #nm, &property ) ) {	b.nm = property->GetFloatValue( def.nm ); }


		const SGlobalPreferences	defaults;

		BOOL_SETTING( gGlobalPreferences, DisplayFramerate, defaults );
		BOOL_SETTING( gGlobalPreferences, ForceLinearFilter, defaults );
		BOOL_SETTING( gGlobalPreferences, RumblePak, defaults );
#ifdef DAEDALUS_DEBUG_DISPLAYLIST
		BOOL_SETTING( gGlobalPreferences, HighlightInexactBlendModes, defaults );
		BOOL_SETTING( gGlobalPreferences, CustomBlendModes, defaults );
#endif
		BOOL_SETTING( gGlobalPreferences, BatteryWarning, defaults );
		BOOL_SETTING( gGlobalPreferences, LargeROMBuffer, defaults );
		FLOAT_SETTING( gGlobalPreferences, StickMinDeadzone, defaults );
		FLOAT_SETTING( gGlobalPreferences, StickMaxDeadzone, defaults );
//		INT_SETTING( gGlobalPreferences, Language, defaults );
		if( section->FindProperty( "Language", &property ) )
		{
			gGlobalPreferences.Language = Translate_IndexFromName( property->GetValue() );
		}
		if( section->FindProperty( "GuiColor", &property ) )
		{
			u32 value( property->GetIntValue(defaults.GuiColor) );
			if( value < NUM_COLOR_TYPES ) //value >= 0 && not needed as it's always True
			{
				gGlobalPreferences.GuiColor = EGuiColor( value );
			}
		}

		if( section->FindProperty( "ViewportType", &property ) )
		{
			u32	value( property->GetIntValue( defaults.ViewportType ) );
			if(value < NUM_VIEWPORT_TYPES ) //value >= 0 && Not need as it's always True
			{
				gGlobalPreferences.ViewportType = EViewportType( value );
			}
		}

		BOOL_SETTING( gGlobalPreferences, TVEnable, defaults );
		BOOL_SETTING( gGlobalPreferences, TVLaced, defaults );
		if( section->FindProperty( "TVType", &property ) )
		{
			u32	value( property->GetIntValue( defaults.TVType ) );
			if( value < 2 ) //value >= 0 && not needed as it's always True
			{
				gGlobalPreferences.TVType = ETVType( value );
			}
		}
	}

	for( u32 section_idx = 0; section_idx < p_ini_file->GetNumSections(); ++section_idx )
	{
		const CIniFileSection * section( p_ini_file->GetSection( section_idx ) );

		RomID			id( RomIDFromString( section->GetName() ) );
		SRomPreferences	preferences;

		const CIniFileProperty * property;
		if( section->FindProperty( "PatchesEnabled", &property ) )
		{
			preferences.PatchesEnabled = property->GetBooleanValue( true );
		}
		if( section->FindProperty( "SpeedSyncEnabled", &property ) )
		{
			preferences.SpeedSyncEnabled = atoi( property->GetValue() );
		}
		if( section->FindProperty( "DynarecEnabled", &property ) )
		{
			preferences.DynarecEnabled = property->GetBooleanValue( true );
		}
		if( section->FindProperty( "DynarecLoopOptimisation", &property ) )
		{
			preferences.DynarecLoopOptimisation = property->GetBooleanValue( false );
		}
		if( section->FindProperty( "DynarecDoublesOptimisation", &property ) )
		{
			preferences.DynarecDoublesOptimisation = property->GetBooleanValue( false );
		}
		if( section->FindProperty( "DoubleDisplayEnabled", &property ) )
		{
			preferences.DoubleDisplayEnabled = property->GetBooleanValue( true );
		}
		if( section->FindProperty( "CleanSceneEnabled", &property ) )
		{
			preferences.CleanSceneEnabled = property->GetBooleanValue( false );
		}
		if( section->FindProperty( "ClearDepthFrameBuffer", &property ) )
		{
			preferences.ClearDepthFrameBuffer = property->GetBooleanValue( false );
		}
		if( section->FindProperty( "AudioRateMatch", &property ) )
		{
            preferences.AudioRateMatch = property->GetBooleanValue( false );
		}
		if( section->FindProperty( "VideoRateMatch", &property ) )
		{
            preferences.VideoRateMatch = property->GetBooleanValue( false );
		}
		if( section->FindProperty( "FogEnabled", &property ) )
		{
            preferences.FogEnabled = property->GetBooleanValue( false );
		}
		if( section->FindProperty( "CheckTextureHashFrequency", &property ) )
		{
			preferences.CheckTextureHashFrequency = GetTextureHashFrequencyFromFrames( atoi( property->GetValue() ) );
		}
		if( section->FindProperty( "Frameskip", &property ) )
		{
			preferences.Frameskip = GetFrameskipValueFromInt( atoi( property->GetValue() ) );
		}
		if( section->FindProperty( "AudioEnabled", &property ) )
		{
			int audio_enabled = atoi( property->GetValue() );

			if( audio_enabled >= APM_DISABLED && audio_enabled <= APM_ENABLED_SYNC )
				preferences.AudioEnabled = static_cast<EAudioPluginMode>( audio_enabled );
			else
				preferences.AudioEnabled = APM_DISABLED;
		}
//		if( section->FindProperty( "AudioAdaptFrequency", &property ) )
//		{
//			preferences.AudioAdaptFrequency = property->GetBooleanValue( false );
//		}
		if( section->FindProperty( "ZoomX", &property ) )
		{
			preferences.ZoomX = (f32)atof( property->GetValue() );
		}

		if( section->FindProperty( "Controller", &property ) )
		{
			preferences.ControllerIndex = CInputManager::Get()->GetConfigurationFromName( property->GetValue() );
		}
		if( section->FindProperty( "MemoryAccessOptimisation", &property ) )
		{
			preferences.MemoryAccessOptimisation = property->GetBooleanValue( false );
		}
		if( section->FindProperty( "CheatsEnabled", &property ) )
		{
			preferences.CheatsEnabled = property->GetBooleanValue( false );
		}
		mPreferences[ id ] = preferences;
	}

	mDirty = false;

	return true;
}

void IPreferences::OutputSectionDetails( const RomID & id, const SRomPreferences & preferences, std::ofstream& fh )
{
	// Generate the CRC-ID for this rom:
	RomSettings		settings;
	CRomSettingsDB::Get()->GetSettings( id, &settings );

fh << std::hex << std::setfill('0');
fh << "{" << std::setw(8) << id.CRC[0] << std::setw(8) << id.CRC[1] << "-" << std::setw(2) << static_cast<int>(id.CountryID) << "}\t// " << settings.GameName << "\n";
fh << "PatchesEnabled=" << preferences.PatchesEnabled << "\n";
fh << "SpeedSyncEnabled=" << preferences.SpeedSyncEnabled << "\n";
fh << "DynarecEnabled=" << preferences.DynarecEnabled << "\n";
fh << "DynarecLoopOptimisation=" << preferences.DynarecLoopOptimisation << "\n";
fh << "DynarecDoublesOptimisation=" << preferences.DynarecDoublesOptimisation << "\n";
fh << "DoubleDisplayEnabled=" << preferences.DoubleDisplayEnabled << "\n";
fh << "CleanSceneEnabled=" << preferences.CleanSceneEnabled << "\n";
fh << "ClearDepthFrameBuffer=" << preferences.ClearDepthFrameBuffer << "\n";
fh << "AudioRateMatch=" << preferences.AudioRateMatch << "\n";
fh << "VideoRateMatch=" << preferences.VideoRateMatch << "\n";
fh << "FogEnabled=" << preferences.FogEnabled << "\n";
fh << "CheckTextureHashFrequency=" << GetTexureHashFrequencyAsFrames(preferences.CheckTextureHashFrequency) << "\n";
fh << "Frameskip=" << GetFrameskipValueAsInt(preferences.Frameskip) << "\n";
fh << "AudioEnabled=" << preferences.AudioEnabled << "\n";
fh << "ZoomX=" << preferences.ZoomX << "\n";
fh << "MemoryAccessOptimisation=" << preferences.MemoryAccessOptimisation << "\n";
fh << "CheatsEnabled=" << preferences.CheatsEnabled << "\n";
fh << "Controller=" << CInputManager::Get()->GetConfigurationName(preferences.ControllerIndex) << "\n";
fh << "\n"; // Spacer
}

// Write out the .ini file, keeping the original comments intact
void IPreferences::Commit()
{
	std::ofstream fh(mFilename);
	if (fh.is_open())
	{
		const SGlobalPreferences	defaults;

#define OUTPUT_BOOL(b, nm, def) fh << std::format("{}={}\n", #nm, b.nm ? "yes" : "no")
#define OUTPUT_FLOAT(b, nm, def) fh << std::format("{}={:.6f}\n", #nm, b.nm)
#define OUTPUT_INT(b, nm, def) fh << #nm << "=" << static_cast<int>(b.nm) << "\n"
#define OUTPUT_LANGUAGE(b, nm, def) fh << std::format("{}={}\n", #nm, Translate_NameFromIndex(b.nm))

		OUTPUT_BOOL( gGlobalPreferences, DisplayFramerate, defaults );
		OUTPUT_BOOL( gGlobalPreferences, ForceLinearFilter, defaults );
		OUTPUT_BOOL( gGlobalPreferences, RumblePak, defaults );
#ifdef DAEDALUS_DEBUG_DISPLAYLIST
		OUTPUT_BOOL( gGlobalPreferences, HighlightInexactBlendModes, defaults );
		OUTPUT_BOOL( gGlobalPreferences, CustomBlendModes, defaults );
#endif
		OUTPUT_BOOL( gGlobalPreferences, BatteryWarning, defaults );
		OUTPUT_BOOL( gGlobalPreferences, LargeROMBuffer, defaults );
		OUTPUT_INT( gGlobalPreferences, GuiColor, defaults );
		OUTPUT_FLOAT( gGlobalPreferences, StickMinDeadzone, defaults );
		OUTPUT_FLOAT( gGlobalPreferences, StickMaxDeadzone, defaults );
		OUTPUT_LANGUAGE( gGlobalPreferences, Language, defaults );
		OUTPUT_INT( gGlobalPreferences, ViewportType, defaults );
		OUTPUT_BOOL( gGlobalPreferences, TVEnable, defaults );
		OUTPUT_BOOL( gGlobalPreferences, TVLaced, defaults );
		OUTPUT_INT( gGlobalPreferences, TVType, defaults );
		fh << "\n\n";

		for ( PreferencesMap::const_iterator it = mPreferences.begin(); it != mPreferences.end(); ++it )
		{
			OutputSectionDetails( it->first, it->second, fh );
		}

		mDirty = false;
	}
}

// Retreive the preferences for the specified rom. Returns false if the rom is
// not in the database
bool IPreferences::GetRomPreferences( const RomID & id, SRomPreferences * preferences ) const
{
	for ( PreferencesMap::const_iterator it = mPreferences.begin(); it != mPreferences.end(); ++it )
	{
		if ( it->first == id )
		{
			*preferences = it->second;
			return true;
		}
	}
	
	return false;
}

// Update the preferences for the specified rom - creates a new entry if necessary
void IPreferences::SetRomPreferences( const RomID & id, const SRomPreferences & preferences )
{
	for ( PreferencesMap::iterator it = mPreferences.begin(); it != mPreferences.end(); ++it )
	{
		if ( it->first == id )
		{
			it->second = preferences;
			return;
		}
	}

	mPreferences[id] = preferences;
	mDirty = true;
}


SGlobalPreferences::SGlobalPreferences()
:	DisplayFramerate( false )
#ifdef DAEDALUS_DEBUG_DISPLAYLIST
,	HighlightInexactBlendModes( false )
,	CustomBlendModes( true )
#endif
,	BatteryWarning( false )
,	LargeROMBuffer( true )
,	ForceLinearFilter( false )
,	RumblePak ( false )
,	GuiColor( BLACK )
,	StickMinDeadzone( 0.28f )
,	StickMaxDeadzone( 1.0f )
,	Language( 0 )
,	ViewportType( VT_FULLSCREEN )
,	TVEnable( false )
,	TVLaced( false )
,	TVType( TT_4_3 )
{
}

void SGlobalPreferences::Apply() const
{

}

SRomPreferences::SRomPreferences()
	:	PatchesEnabled( true )
	,	DynarecEnabled( true )
	,	DynarecLoopOptimisation( true )
	,	DynarecDoublesOptimisation( true )
	,	DoubleDisplayEnabled( true )
	,	CleanSceneEnabled( false )
	,	ClearDepthFrameBuffer( false )
	,	AudioRateMatch( false )
	,	VideoRateMatch( false )
	,	FogEnabled( false )
	,   MemoryAccessOptimisation( true )
	,	CheatsEnabled( false )
//	,	AudioAdaptFrequency( false )
	,	CheckTextureHashFrequency( kDefaultTextureHashFrequency )
	,	Frameskip( FV_DISABLED )
	,	AudioEnabled( kDefaultAudioPluginMode )
	,	ZoomX( 1.0f )
	,	SpeedSyncEnabled( 1 )
	,	ControllerIndex( 0 )
{
}

void SRomPreferences::Reset()
{
	PatchesEnabled             = true;
	SpeedSyncEnabled           = 1;
	DynarecEnabled             = true;
	DynarecLoopOptimisation    = true;
	DynarecDoublesOptimisation = true;
	DoubleDisplayEnabled       = true;
	CleanSceneEnabled          = false;
	ClearDepthFrameBuffer	   = false;
	AudioRateMatch             = false;
	VideoRateMatch             = false;
	FogEnabled                 = false;
	MemoryAccessOptimisation   = true;
	CheckTextureHashFrequency  = kDefaultTextureHashFrequency;
	Frameskip                  = FV_DISABLED;
	AudioEnabled               = kDefaultAudioPluginMode;
	//AudioAdaptFrequency      = false;
	ZoomX                      = 1.0f;
	CheatsEnabled              = false;
	ControllerIndex            = 0;
}

void SRomPreferences::Apply() const
{
	gOSHooksEnabled             = PatchesEnabled;
	gSpeedSyncEnabled           = SpeedSyncEnabled;
	gDynarecEnabled             = g_ROM.settings.DynarecSupported && DynarecEnabled;
	gDynarecLoopOptimisation	= DynarecLoopOptimisation;	// && g_ROM.settings.DynarecLoopOptimisation;
	gDynarecDoublesOptimisation	= g_ROM.settings.DynarecDoublesOptimisation || DynarecDoublesOptimisation;
	gDoubleDisplayEnabled       = g_ROM.settings.DoubleDisplayEnabled && DoubleDisplayEnabled; // I don't know why DD won't disabled if we set ||
	gCleanSceneEnabled          = g_ROM.settings.CleanSceneEnabled || CleanSceneEnabled;
	gClearDepthFrameBuffer      = g_ROM.settings.ClearDepthFrameBuffer || ClearDepthFrameBuffer;
	gAudioRateMatch             = g_ROM.settings.AudioRateMatch || AudioRateMatch;
	gVideoRateMatch             = g_ROM.settings.VideoRateMatch || VideoRateMatch;
	gFogEnabled                 = g_ROM.settings.FogEnabled || FogEnabled;
	gCheckTextureHashFrequency  = GetTexureHashFrequencyAsFrames( CheckTextureHashFrequency );
	gMemoryAccessOptimisation   = g_ROM.settings.MemoryAccessOptimisation || MemoryAccessOptimisation;
	gFrameskipValue             = Frameskip;
	gZoomX                      = ZoomX;
	gCheatsEnabled              = g_ROM.settings.CheatsEnabled || CheatsEnabled;
	gAudioPluginEnabled         = AudioEnabled;
//	gAdaptFrequency             = AudioAdaptFrequency;
	gControllerIndex            = ControllerIndex;							//Used during ROM initialization

	CInputManager::Get()->SetConfiguration( ControllerIndex );  //Used after initialization
}


static const u32 gTextureHashFreqeuncies[] =
{
	0,	//THF_DISABLED = 0,
	1,	//THF_EVERY_FRAME,
	2,	//THF_EVERY_2,
	4,	//THF_EVERY_4,
	8,	//THF_EVERY_8,
	16,	//THF_EVERY_16,
	32,	//THF_EVERY_32,
};

static const char * const gTextureHashFreqeuncyDescriptions[] =
{
	"Disabled",			//THF_DISABLED = 0,
	"Every Frame",		//THF_EVERY_FRAME,
	"Every 2 Frames",	//THF_EVERY_2,
	"Every 4 Frames",	//THF_EVERY_4,
	"Every 8 Frames",	//THF_EVERY_8,
	"Every 16 Frames",	//THF_EVERY_16,
	"Every 32 Frames",	//THF_EVERY_32,
};

static u32 GetTexureHashFrequencyAsFrames( ETextureHashFrequency thf )
{
	if(thf >= 0 && thf < NUM_THF)
	{
		return gTextureHashFreqeuncies[ thf ];
	}

	return 0;
}

static ETextureHashFrequency GetTextureHashFrequencyFromFrames( u32 frames )
{
	for( u32 i = 0; i < NUM_THF; ++i )
	{
		if( frames <= gTextureHashFreqeuncies[ i ] )
		{
			return ETextureHashFrequency( i );
		}
	}

	return THF_EVERY_32;	// Return the maximum
}

const char * Preferences_GetTextureHashFrequencyDescription( ETextureHashFrequency thf )
{
	if(thf >= 0 && thf < NUM_THF)
	{
		return gTextureHashFreqeuncyDescriptions[ thf ];
	}

	return "?";
}

u32	GetFrameskipValueAsInt( EFrameskipValue value )
{
	return value;
}

EFrameskipValue	GetFrameskipValueFromInt( u32 value )
{
//  This is always False
//	if( value < FV_DISABLED )
//		return FV_DISABLED;

//	if( value > FV_10 )
//		return FV_10;

	return EFrameskipValue( value );
}

const char * Preferences_GetFrameskipDescription( EFrameskipValue value )
{
	switch( value )
	{
	case FV_DISABLED:		return "Disabled";
	case FV_AUTO1:			return "Auto 1";
	case FV_AUTO2:			return "Auto 2";
	case FV_1:				return "1";
	case FV_2:				return "2";
	case FV_3:				return "3";
	case FV_4:				return "4";
	case FV_5:				return "5";
	case FV_6:				return "6";
	case FV_7:				return "7";
	case FV_8:				return "8";
	case FV_9:				return "9";
#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	case FV_99:				return "99";
#endif
	case NUM_FRAMESKIP_VALUES:
		break;
	}
	#ifdef DAEDALUS_DEBUG_CONSOLE
	DAEDALUS_ERROR( "Unhandled frameskip value" );
	#endif
	return "?";
}
