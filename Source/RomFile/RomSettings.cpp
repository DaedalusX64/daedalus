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


#include <cstring> 

#include <set>
#include <map>
#include <fstream>
#include <filesystem>
#include <iostream>

#include "Core/ROM.h"
#include "RomFile/RomSettings.h"
#include "Debug/DBGConsole.h"
#include "Interface/RomDB.h"
#include "Utility/IniFile.h"
#include "Utility/Paths.h"

namespace
{
//

EExpansionPakUsage	ExpansionPakUsageFromString( const char * str )
{
	for( u32 i = 0; i < NUM_EXPANSIONPAK_USAGE_TYPES; ++i )
	{
		EExpansionPakUsage	pak_usage = EExpansionPakUsage( i );

		if( strcasecmp( str, ROM_GetExpansionPakUsageName( pak_usage ) ) == 0 )
		{
			return pak_usage;
		}
	}

	return PAK_STATUS_UNKNOWN;
}

ESaveType	SaveTypeFromString( const char * str )
{
	for( u32 i = 0; i < NUM_SAVE_TYPES; ++i )
	{
		ESaveType	save_type = ESaveType( i );

		if( strcasecmp( str, ROM_GetSaveTypeName( save_type ) ) == 0 )
		{
			return save_type;
		}
	}

	return SAVE_TYPE_UNKNOWN;
}

}


//

const char * ROM_GetExpansionPakUsageName( EExpansionPakUsage pak_usage )
{
	switch( pak_usage )
	{
		case PAK_STATUS_UNKNOWN:	return "Unknown";
		case PAK_UNUSED:			return "Unused";
		case PAK_USED:				return "Used";
		case PAK_REQUIRED:			return "Required";
	}

#ifdef DAEDALUS_DEBUG_CONSOLE
	DAEDALUS_ERROR( "Unknown expansion pak type" );
	#endif
	return "?";
}


// Get the name of a save type from an ESaveType enum

const char * ROM_GetSaveTypeName( ESaveType save_type )
{
	switch ( save_type )
	{
		case SAVE_TYPE_UNKNOWN:		return "Unknown";
		case SAVE_TYPE_EEP4K:		return "Eeprom4k";
		case SAVE_TYPE_EEP16K:		return "Eeprom16k";
		case SAVE_TYPE_SRAM:		return "SRAM";
		case SAVE_TYPE_FLASH:		return "FlashRam";
	}
#ifdef DAEDALUS_DEBUG_CONSOLE
	DAEDALUS_ERROR( "Unknown save type" );
	#endif
	return "?";
}


//

class IRomSettingsDB : public CRomSettingsDB
{
	public:
		IRomSettingsDB();
		virtual ~IRomSettingsDB();

		//
		// CRomSettingsDB implementation
		//
		bool			OpenSettingsFile( const std::filesystem::path &filename );
		void			Commit();												// (STRMNNRMN - Write ini back out to disk?)

		bool			GetSettings( const RomID & id, RomSettings * p_settings ) const;
		void			SetSettings( const RomID & id, const RomSettings & settings );

	private:

		void			OutputSectionDetails( const RomID & id, const RomSettings & settings, std::ostream &fh );

	private:
	using SettingsMap = std::map<RomID, RomSettings>;
		SettingsMap				mSettings;

		bool					mDirty;				// (STRMNNRMN - Changed since read from disk?)
		const std::filesystem::path		mFilename;
};




// Singleton creator

template<> bool	CSingleton< CRomSettingsDB >::Create()
{
	#ifdef DAEDALUS_ENABLE_ASSERTS
	DAEDALUS_ASSERT_Q(mpInstance == nullptr);
	#endif
	mpInstance = std::make_shared<IRomSettingsDB>();
	mpInstance->OpenSettingsFile( setBasePath("roms.ini") );
	return true;
}


IRomSettingsDB::IRomSettingsDB() :	mDirty( false ) {}


IRomSettingsDB::~IRomSettingsDB()
{
	// if ( mDirty )
	// {
	// 	Commit();
	// }
}


//	Remove the specified characters from p_string
static bool	trim( char * p_string, const char * p_trim_chars )
{
	u32 num_trims = (u32)strlen( p_trim_chars );
	char * pin = p_string;
	char * pout = p_string;
	bool found = false;
	while ( *pin )
	{
		char c = *pin;

		found = false;
		for ( u32 i = 0; i < num_trims; i++ )
		{
			if ( p_trim_chars[ i ] == c )
			{
				// Skip
				found = true;
				break;
			}
		}

		if ( found )
		{
			pin++;
		}
		else
		{
			// Copy
			*pout++ = *pin++;
		}
	}
	*pout = '\0';
	return true;
}


//

static RomID	RomIDFromString( const char * str )
{
	u32 crc1, crc2, country;
	sscanf( str, "%08x%08x-%02x", &crc1, &crc2, &country );
	return RomID( crc1, crc2, (u8)country );
}

bool IRomSettingsDB::OpenSettingsFile( const std::filesystem::path &filename )
{

	std::filesystem::path mFilename = filename;
	
	auto p_ini_file = CIniFile::Create(filename);
	if( p_ini_file == nullptr )
	{
		DBGConsole_Msg( 0, "Failed to open roms.ini from %s\n", filename.c_str() );
		return false;
	}

	for( u32 section_idx = 0; section_idx < p_ini_file->GetNumSections(); ++section_idx )
	{
		const CIniFileSection * p_section( p_ini_file->GetSection( section_idx ) );

		RomID			id( RomIDFromString( p_section->GetName() ) );
		RomSettings	settings;

		const CIniFileProperty * p_property;
		if( p_section->FindProperty( "Comment", &p_property ) )
		{
			settings.Comment = p_property->GetValue();
		}
		if( p_section->FindProperty( "Info", &p_property ) )
		{
			settings.Info = p_property->GetValue();
		}
		if( p_section->FindProperty( "Name", &p_property ) )
		{
			settings.GameName = p_property->GetValue();
		}
		if( p_section->FindProperty( "Preview", &p_property ) )
		{
			settings.Preview = p_property->GetValue();
		}
		if( p_section->FindProperty( "ExpansionPakUsage", &p_property ) )
		{
			settings.ExpansionPakUsage = ExpansionPakUsageFromString( p_property->GetValue() );
		}
		if( p_section->FindProperty( "SaveType", &p_property ) )
		{
			settings.SaveType = SaveTypeFromString( p_property->GetValue() );
		}
		if( p_section->FindProperty( "PatchesEnabled", &p_property ) )
		{
			settings.PatchesEnabled = p_property->GetBooleanValue( true );
		}
		if( p_section->FindProperty( "SpeedSyncEnabled", &p_property ) )
		{
			settings.SpeedSyncEnabled = atoi( p_property->GetValue() );
		}
		if( p_section->FindProperty( "DynarecSupported", &p_property ) )
		{
			settings.DynarecSupported = p_property->GetBooleanValue( true );
		}
		if( p_section->FindProperty( "DynarecLoopOptimisation", &p_property ) )
		{
			settings.DynarecLoopOptimisation = p_property->GetBooleanValue( false );
		}
		if( p_section->FindProperty( "DynarecDoublesOptimisation", &p_property ) )
		{
			settings.DynarecDoublesOptimisation = p_property->GetBooleanValue( false );
		}
		if( p_section->FindProperty( "DoubleDisplayEnabled", &p_property ) )
		{
			settings.DoubleDisplayEnabled = p_property->GetBooleanValue( true );
		}
		if( p_section->FindProperty( "CleanSceneEnabled", &p_property ) )
		{
			settings.CleanSceneEnabled = p_property->GetBooleanValue( false );
		}
		if( p_section->FindProperty( "ClearDepthFrameBuffer", &p_property ) )
		{
			settings.ClearDepthFrameBuffer = p_property->GetBooleanValue( false );
		}
		if( p_section->FindProperty( "AudioRateMatch", &p_property ) )
		{
			settings.AudioRateMatch = p_property->GetBooleanValue( false );
		}
		if( p_section->FindProperty( "VideoRateMatch", &p_property ) )
		{
			settings.VideoRateMatch = p_property->GetBooleanValue( false );
		}
		if( p_section->FindProperty( "FogEnabled", &p_property ) )
		{
			settings.FogEnabled = p_property->GetBooleanValue( false );
		}
		if( p_section->FindProperty( "MemoryAccessOptimisation", &p_property ) )
		{
			settings.MemoryAccessOptimisation = p_property->GetBooleanValue( false );
		}
		if( p_section->FindProperty( "CheatsEnabled", &p_property ) )
		{
			settings.CheatsEnabled = p_property->GetBooleanValue( false );
		}
		SetSettings( id, settings );
	}

	mDirty = false;

	return true;
}

// //	Write out the .ini file, keeping the original comments intact
void IRomSettingsDB::Commit(){}
// 	mFilename = "roms.ini";


// 	std::filesystem::path filename_tmp = "roms.ini.tmp";

// 	std::fstream fh_src(mFilename, std::ios::in);
// 	if (!fh_src)
// 	{
// 		std::cerr << mFilename << " : Does not exist";
// 		return;
// 	}
// 	std::fstream fh_dst(filename_tmp, std::ios::out);


// 	//	Keep track of visited sections in a set
// 	std::set<RomID>		visited;

// 	std::string line;
//    while (std::getline(fh_src, line))
// 	{
// 		if (line[0] == '{')
// 		{
// 			const char * const trim_chars = "{}\n\r"; //remove first and last character
// 			char buffer[1024 + 1];
// 			std::strncpy(buffer, line.c_str(), sizeof(buffer) -1);
// 			buffer[sizeof(buffer) - 1 ] = '\0';

// 			// Start of section
// 			trim( buffer, trim_chars );

// 			RomID id( RomIDFromString( buffer ) );

// 			// Avoid duplicated entries for this id
// 			if ( visited.find( id ) != visited.end() )
// 				continue;

// 			visited.insert( id );

// 			SettingsMap::const_iterator	it( mSettings.find( id ) );
// 			if( it != mSettings.end() )
// 			{
// 				// Output this CRC
// 				OutputSectionDetails( id, it->second, fh_dst );
// 			}
// 			else
// 			{
// 				// Do what? This should never happen, unless the user
// 				// replaces the inifile while Daedalus is running!
// 			}
// 		}
// 		else if (line[0] == '/')
// 		{
// 			// Skip Comment
// 			fh_dst << line << '\n';
// 			continue;
// 		}
// 	}
// 	for (const auto& [id, settings] : mSettings)
// 	{
// 		// Skip any that have not been done.
// 		if ( visited.find( id) == visited.end() )
// 		{
// 			OutputSectionDetails( id, settings, fh_dst );
// 		}
// 	}


// 	// Create the new file
// 	std::filesystem::remove(mFilename);
// 	std::filesystem::rename(filename_tmp, mFilename);

// 	mDirty = false;

// 	}


void IRomSettingsDB::OutputSectionDetails( const RomID & id, const RomSettings & settings, std::ostream &out )
{
	// Generate the CRC-ID for this rom
    out << "{" << std::hex << std::uppercase << id.CRC[0] << std::hex << std::uppercase << id.CRC[1] << "-"
        << std::dec << std::setfill('0') << std::setw(2) << id.CountryID << "}\n";
    
    out << "Name=" << settings.GameName << "\n";
    
    if (!settings.Comment.empty())             out << "Comment=" << settings.Comment << "\n";
    if (!settings.Info.empty())                out << "Info=" << settings.Info << "\n";
    if (!settings.Preview.empty())             out << "Preview=" << settings.Preview << "\n";
    if (!settings.PatchesEnabled)              out << "PatchesEnabled=no\n";
    if (!settings.SpeedSyncEnabled)            out << "SpeedSyncEnabled=" << settings.SpeedSyncEnabled << "\n";
    if (!settings.DynarecSupported)            out << "DynarecSupported=no\n";
    if (settings.DynarecLoopOptimisation)      out << "DynarecLoopOptimisation=yes\n";
    if (settings.DynarecDoublesOptimisation)   out << "DynarecDoublesOptimisation=yes\n";
    if (!settings.DoubleDisplayEnabled)        out << "DoubleDisplayEnabled=no\n";
    if (settings.CleanSceneEnabled)            out << "CleanSceneEnabled=yes\n";
    if (settings.ClearDepthFrameBuffer)        out << "ClearDepthFrameBuffer=yes\n";
    if (settings.AudioRateMatch)               out << "AudioRateMatch=yes\n";
    if (settings.VideoRateMatch)               out << "VideoRateMatch=yes\n";
    if (settings.FogEnabled)                   out << "FogEnabled=yes\n";
    if (settings.MemoryAccessOptimisation)     out << "MemoryAccessOptimisation=yes\n";
    if (settings.CheatsEnabled)                out << "CheatsEnabled=yes\n";

    if (settings.ExpansionPakUsage != PAK_STATUS_UNKNOWN) 
        out << "ExpansionPakUsage=" << ROM_GetExpansionPakUsageName(settings.ExpansionPakUsage) << "\n";
    if (settings.SaveType != SAVE_TYPE_UNKNOWN) 
        out << "SaveType=" << ROM_GetSaveTypeName(settings.SaveType) << "\n";

    out << "\n";  // Spacer
}


// Retreive the settings for the specified rom. Returns false if the rom is
// not in the database
bool	IRomSettingsDB::GetSettings( const RomID & id, RomSettings * p_settings ) const
{
	for ( SettingsMap::const_iterator it = mSettings.begin(); it != mSettings.end(); ++it )
	{
		if ( it->first == id )
		{
			*p_settings = it->second;
			return true;
		}
	}
	
	return false;
}


// Update the settings for the specified rom - creates a new entry if necessary
void	IRomSettingsDB::SetSettings( const RomID & id, const RomSettings & settings )
{
	for ( SettingsMap::iterator it = mSettings.begin(); it != mSettings.end(); ++it )
	{
		if ( it->first == id )
		{
			it->second = settings;
			return;
		}
	}

	mSettings[id] = settings;
	mDirty = true;
}


//

RomSettings::RomSettings()
:	ExpansionPakUsage( PAK_STATUS_UNKNOWN )
,	SaveType( SAVE_TYPE_UNKNOWN )
,	PatchesEnabled( true )
,	SpeedSyncEnabled( 1 )
,	DynarecSupported( true )
,	DynarecLoopOptimisation( false )
,	DynarecDoublesOptimisation( false )
,	DoubleDisplayEnabled( true )
,	CleanSceneEnabled( false )
,	ClearDepthFrameBuffer( false )
,	AudioRateMatch( false )
,	VideoRateMatch( false )
,	FogEnabled( false )
,   MemoryAccessOptimisation( false )
,   CheatsEnabled( false )
{
}


//

RomSettings::~RomSettings() {}

void	RomSettings::Reset()
{
	GameName = "";
	Comment = "";
	Info = "";
	ExpansionPakUsage = PAK_STATUS_UNKNOWN;
	SaveType = SAVE_TYPE_UNKNOWN;
	PatchesEnabled = true;
	SpeedSyncEnabled = 0;
	DynarecSupported = true;
	DynarecLoopOptimisation = false;
	DynarecDoublesOptimisation = false;
	DoubleDisplayEnabled = true;
	CleanSceneEnabled = false;
	ClearDepthFrameBuffer = false;
	AudioRateMatch = false;
	VideoRateMatch = false;
	FogEnabled = false;
	CheatsEnabled = false;
	MemoryAccessOptimisation = false;
}
