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

#include "stdafx.h"
#include "RomSettings.h"

#include "Utility/IniFile.h"
#include "Utility/IO.h"

#include "Core/ROM.h"
#include "Interface/RomDB.h"

#include <set>
#include <map>

namespace
{

//*****************************************************************************
//
//*****************************************************************************
EExpansionPakUsage	ExpansionPakUsageFromString( const char * str )
{
	for( u32 i = 0; i < NUM_EXPANSIONPAK_USAGE_TYPES; ++i )
	{
		EExpansionPakUsage	pak_usage = EExpansionPakUsage( i );

		if( _strcmpi( str, ROM_GetExpansionPakUsageName( pak_usage ) ) == 0 )
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

		if( _strcmpi( str, ROM_GetSaveTypeName( save_type ) ) == 0 )
		{
			return save_type;
		}
	}

	return SAVE_TYPE_UNKNOWN;
}

}

//*****************************************************************************
//
//*****************************************************************************
const char * ROM_GetExpansionPakUsageName( EExpansionPakUsage pak_usage )
{
	switch( pak_usage )
	{
		case PAK_STATUS_UNKNOWN:	return "Unknown";
		case PAK_UNUSED:			return "Unused";
		case PAK_USED:				return "Used";
		case PAK_REQUIRED:			return "Required";
	}

	DAEDALUS_ERROR( "Unknown expansion pak type" );
	return "?";
}

//*****************************************************************************
// Get the name of a save type from an ESaveType enum
//*****************************************************************************
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

	DAEDALUS_ERROR( "Unknown save type" );
	return "?";
}

//*****************************************************************************
//
//*****************************************************************************
class IRomSettingsDB : public CRomSettingsDB
{
	public:
		IRomSettingsDB();
		virtual ~IRomSettingsDB();

		//
		// CRomSettingsDB implementation
		//
		bool			OpenSettingsFile( const char * filename );
		void			Commit();												// (STRMNNRMN - Write ini back out to disk?)

		bool			GetSettings( const RomID & id, RomSettings * p_settings ) const;
		void			SetSettings( const RomID & id, const RomSettings & settings );

	private:

		void			OutputSectionDetails( const RomID & id, const RomSettings & settings, FILE * fh );

	private:
		typedef std::map<RomID, RomSettings>		SettingsMap;

		SettingsMap				mSettings;

		bool					mDirty;				// (STRMNNRMN - Changed since read from disk?)
		char					mFilename[MAX_PATH + 1];
};



//*****************************************************************************
// Singleton creator
//*****************************************************************************
template<> bool	CSingleton< CRomSettingsDB >::Create()
{
	DAEDALUS_ASSERT_Q(mpInstance == NULL);

	mpInstance = new IRomSettingsDB();

	char		ini_filename[MAX_PATH+1];
	IO::Path::Combine( ini_filename, gDaedalusExePath, "roms.ini" );
	mpInstance->OpenSettingsFile( ini_filename );

	return true;
}


//*****************************************************************************
// Constructor
//*****************************************************************************
IRomSettingsDB::IRomSettingsDB()
:	mDirty( false )
{
}

//*****************************************************************************
//
//*****************************************************************************
IRomSettingsDB::~IRomSettingsDB()
{
	if ( mDirty )
	{
		Commit();
	}
}

//*****************************************************************************
//	Remove the specified characters from p_string
//*****************************************************************************
static bool	trim( char * p_string, const char * p_trim_chars )
{
	u32 num_trims = strlen( p_trim_chars );
	char * pin = p_string;
	char * pout = p_string;
	bool found;
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

//*****************************************************************************
//
//*****************************************************************************
static RomID	RomIDFromString( const char * str )
{
	u32 crc1, crc2, country;
	sscanf( str, "%08x%08x-%02x", &crc1, &crc2, &country );
	return RomID( crc1, crc2, (u8)country );
}

//*****************************************************************************
//
//*****************************************************************************
bool IRomSettingsDB::OpenSettingsFile( const char * filename )
{
	//
	// Remember the filename
	//
	strcpy(mFilename, filename);

	CIniFile * p_ini_file( CIniFile::Create( filename ) );
	if( p_ini_file == NULL )
	{
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

	delete p_ini_file;
	return true;
}

//*****************************************************************************
//	Write out the .ini file, keeping the original comments intact
//*****************************************************************************
void IRomSettingsDB::Commit()
{
	char szFileNameOut[MAX_PATH+1];
	char szFileNameDelete[MAX_PATH+1];
	FILE * fhIn;
	FILE * fhOut;
	char szBuf[1024+1];
	char trim_chars[]="{}\n\r"; //remove first and last character

	sprintf(szFileNameOut, "%s.tmp", mFilename);
	sprintf(szFileNameDelete, "%s.del", mFilename);

	fhIn = fopen(mFilename, "r");
	if (fhIn == NULL)
		return;

	fhOut = fopen(szFileNameOut, "w");
	if (fhOut == NULL)
	{
		fclose(fhIn);
		return;
	}

	//
	//	Keep track of visited sections in a set
	//
	std::set<RomID>		visited;

	while (fgets(szBuf, 1024, fhIn))
	{
		if (szBuf[0] == '{')
		{
			// Start of section
			trim( szBuf,trim_chars );

			RomID id( RomIDFromString( szBuf ) );

			// Avoid duplicated entries for this id
			if ( visited.find( id ) != visited.end() )
				continue;

			visited.insert( id );

			SettingsMap::const_iterator	it( mSettings.find( id ) );
			if( it != mSettings.end() )
			{
				// Output this CRC
				OutputSectionDetails( id, it->second, fhOut );
			}
			else
			{
				// Do what? This should never happen, unless the user
				// replaces the inifile while Daedalus is running!
			}
		}
		else if (szBuf[0] == '/')
		{
			// Comment
			fputs(szBuf, fhOut);
			continue;
		}

	}

	// Input buffer done-  process any new entries!
	for ( SettingsMap::const_iterator it = mSettings.begin(); it != mSettings.end(); ++it )
	{
		// Skip any that have not been done.
		if ( visited.find( it->first ) == visited.end() )
		{
			OutputSectionDetails( it->first, it->second, fhOut );
		}
	}

	fclose( fhOut );
	fclose( fhIn );

	// Create the new file
	IO::File::Move( mFilename, szFileNameDelete );
	IO::File::Move( szFileNameOut, mFilename );
	IO::File::Delete( szFileNameDelete );

	mDirty = false;
}

//*****************************************************************************
//
//*****************************************************************************
void IRomSettingsDB::OutputSectionDetails( const RomID & id, const RomSettings & settings, FILE * fh )
{
	// Generate the CRC-ID for this rom:
	fprintf(fh, "{%08x%08x-%02x}\n", id.CRC[0], id.CRC[1], id.CountryID );

	fprintf(fh, "Name=%s\n", settings.GameName.c_str());

	if( !settings.Comment.empty() )				fprintf(fh, "Comment=%s\n", settings.Comment.c_str());
	if( !settings.Info.empty() )				fprintf(fh, "Info=%s\n", settings.Info.c_str());
	if( !settings.Preview.empty() )				fprintf(fh, "Preview=%s\n", settings.Preview.c_str());
	if( !settings.PatchesEnabled )				fprintf(fh, "PatchesEnabled=no\n");
	if( !settings.SpeedSyncEnabled )			fprintf(fh, "SpeedSyncEnabled=%d\n", settings.SpeedSyncEnabled);
	if( !settings.DynarecSupported )			fprintf(fh, "DynarecSupported=no\n");
	if( !settings.DynarecLoopOptimisation )		fprintf(fh, "DynarecLoopOptimisation=yes\n");
	if( !settings.DynarecDoublesOptimisation )	fprintf(fh, "DynarecDoublesOptimisation=yes\n");
	if( !settings.DoubleDisplayEnabled )		fprintf(fh, "DoubleDisplayEnabled=no\n");
	if( settings.CleanSceneEnabled )			fprintf(fh, "CleanSceneEnabled=yes\n");
	if( settings.AudioRateMatch )				fprintf(fh, "AudioRateMatch=yes\n");
	if( settings.VideoRateMatch )				fprintf(fh, "VideoRateMatch=yes\n");
	if( settings.FogEnabled )					fprintf(fh, "FogEnabled=yes\n");
	if( settings.MemoryAccessOptimisation )		fprintf(fh, "MemoryAccessOptimisation=yes\n");
	if( settings.CheatsEnabled )				fprintf(fh, "CheatsEnabled=yes\n");

	if ( settings.ExpansionPakUsage != PAK_STATUS_UNKNOWN )	fprintf(fh, "ExpansionPakUsage=%s\n", ROM_GetExpansionPakUsageName( settings.ExpansionPakUsage ) );
	if ( settings.SaveType != SAVE_TYPE_UNKNOWN )			fprintf(fh, "SaveType=%s\n", ROM_GetSaveTypeName( settings.SaveType ) );

	fprintf(fh, "\n");			// Spacer
}

//*****************************************************************************
// Retreive the settings for the specified rom. Returns false if the rom is
// not in the database
//*****************************************************************************
bool	IRomSettingsDB::GetSettings( const RomID & id, RomSettings * p_settings ) const
{
	SettingsMap::const_iterator	it( mSettings.find( id ) );
	if ( it != mSettings.end() )
	{
		*p_settings = it->second;
		return true;
	}
	else
	{
		return false;
	}
}

//*****************************************************************************
// Update the settings for the specified rom - creates a new entry if necessary
//*****************************************************************************
void	IRomSettingsDB::SetSettings( const RomID & id, const RomSettings & settings )
{
	SettingsMap::iterator	it( mSettings.find( id ) );
	if ( it != mSettings.end() )
	{
		it->second = settings;
	}
	else
	{
		mSettings[ id ] = settings;
	}

	mDirty = true;
}

//*****************************************************************************
//
//*****************************************************************************
RomSettings::RomSettings()
:	ExpansionPakUsage( PAK_STATUS_UNKNOWN )
,	SaveType( SAVE_TYPE_UNKNOWN )
,	PatchesEnabled( true )
,	SpeedSyncEnabled( 0 )
,	DynarecSupported( true )
,	DynarecLoopOptimisation( false )
,	DynarecDoublesOptimisation( false )
,	DoubleDisplayEnabled( true )
,	CleanSceneEnabled( false )
,	AudioRateMatch( false )
,	VideoRateMatch( false )
,	FogEnabled( false )
,   MemoryAccessOptimisation( false )
,   CheatsEnabled( false )
,	RescanCount(0)
{
}

//*****************************************************************************
//
//*****************************************************************************
RomSettings::~RomSettings()
{
}

//*****************************************************************************
//
//*****************************************************************************
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
	AudioRateMatch = false;
	VideoRateMatch = false;
	FogEnabled = false;
	CheatsEnabled = false;
	MemoryAccessOptimisation = false;
	RescanCount = 0;
}
