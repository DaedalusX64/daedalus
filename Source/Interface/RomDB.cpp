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

#include "stdafx.h"
#include "RomDB.h"

#include <stdio.h>

#include <vector>
#include <algorithm>

#include "Core/ROM.h"
#include "Core/ROMImage.h"
#include "Debug/DBGConsole.h"
#include "Math/MathUtil.h"
#include "System/Paths.h"
#include "Utility/IO.h"
#include "Utility/ROMFile.h"
#include "Utility/Stream.h"

static const u64 ROMDB_MAGIC_NO	{0x42444D5244454144LL}; //DAEDRMDB		// 44 41 45 44 52 4D 44 42
static const u32 ROMDB_CURRENT_VERSION {4};

static const u32 MAX_SENSIBLE_FILES {2048};
static const u32 MAX_SENSIBLE_DETAILS {2048};

CRomDB::~CRomDB() {}

class IRomDB : public CRomDB
{
	public:
		IRomDB();
		~IRomDB();

		//
		// CRomDB implementation
		//
		void			Reset();
		bool			Commit();

		void			AddRomDirectory(const char * directory);

		bool			QueryByFilename( const char * filename, RomID * id, u32 * rom_size, ECicType * cic_type );
		bool			QueryByID( const RomID & id, u32 * rom_size, ECicType * cic_type ) const;
		const char *	QueryFilenameFromID( const RomID & id ) const;

	private:
		void			AddRomFile(const char * filename);

		void			AddRomEntry( const char * filename, const RomID & id, u32 rom_size, ECicType cic_type );
		bool			OpenDB( const char * filename );

	private:

		// For serialisation we used a fixed size struct for ease of reading
		struct RomFilesKeyValue
		{
			RomFilesKeyValue()
			{
				memset( FileName, 0, sizeof( FileName ) );
			}
			RomFilesKeyValue( const RomFilesKeyValue & rhs )
			{
				memset( FileName, 0, sizeof( FileName ) );
				strcpy( FileName, rhs.FileName );
				ID = rhs.ID;
			}
			RomFilesKeyValue & operator=( const RomFilesKeyValue & rhs )
			{
				if( this == &rhs )
					return *this;
				memset( FileName, 0, sizeof( FileName ) );
				strcpy( FileName, rhs.FileName );
				ID = rhs.ID;
				return *this;
			}

			RomFilesKeyValue( const char * filename, const RomID & id )
			{
				memset( FileName, 0, sizeof( FileName ) );
				strcpy( FileName, filename );
				ID = id;
			}

			// This is actually IO::Path::kMaxPathLen+1, but we need to ensure that it doesn't change if we ever change the kMaxPathLen constant.
			static const u32 kMaxFilenameLen {260};
			char		FileName[ kMaxFilenameLen + 1 ];
			RomID		ID;
		};

		struct SSortByFilename
		{
			bool operator()( const RomFilesKeyValue & a, const RomFilesKeyValue & b ) const
			{
				return strcmp( a.FileName, b.FileName ) < 0;
			}
			bool operator()( const char * a, const RomFilesKeyValue & b ) const
			{
				return strcmp( a, b.FileName ) < 0;
			}
			bool operator()( const RomFilesKeyValue & a, const char * b ) const
			{
				return strcmp( a.FileName, b ) < 0;
			}
		};

		struct RomDetails
		{
			RomDetails()
				:	ID()
				,	RomSize( 0 )
				,	CicType( CIC_UNKNOWN )
			{
			}

			RomDetails( const RomID & id, u32 rom_size, ECicType cic_type )
				:	ID( id )
				,	RomSize( rom_size )
				,	CicType( cic_type )
			{
			}

			RomID				ID;
			u32					RomSize;
			ECicType			CicType;
		};

		struct SSortDetailsByID
		{
			bool operator()( const RomDetails & a, const RomDetails & b ) const
			{
				return a.ID < b.ID;
			}
			bool operator()( const RomID & a, const RomDetails & b ) const
			{
				return a < b.ID;
			}
			bool operator()( const RomDetails & a, const RomID & b ) const
			{
				return a.ID < b;
			}
		};

		typedef std::vector< RomFilesKeyValue >	FilenameVec;
		typedef std::vector< RomDetails >		DetailsVec;

		IO::Filename					mRomDBFileName;
		FilenameVec						mRomFiles;
		DetailsVec						mRomDetails;
		bool							mDirty;
};

template<> bool	CSingleton< CRomDB >::Create()
{
	DAEDALUS_ASSERT_Q(mpInstance == NULL);

	mpInstance = new IRomDB();

	IO::Filename romdb_filename;
	IO::Path::Combine( romdb_filename, gDaedalusExePath, "rom.db" );
	/*ret = */mpInstance->OpenDB( romdb_filename );
	// Ignore failure - this file might not exist on first run.

	return true;
}

IRomDB::IRomDB()
:	mDirty( false )
{
	mRomDBFileName[ 0 ] = '\0';
}

IRomDB::~IRomDB()
{
	Commit();
}

void IRomDB::Reset()
{
	mRomFiles.clear();
	mRomDetails.clear();
	mDirty = true;
}

bool IRomDB::OpenDB( const char * filename )
{
	u32 num_read;

	//
	// Remember the filename
	//
	IO::Path::Assign( mRomDBFileName, filename );

	FILE * fh = fopen( filename, "rb" );
	if ( !fh )
	{
		DBGConsole_Msg( 0, "Failed to open RomDB from %s\n", mRomDBFileName );
		return false;
	}

	//
	// Check the magic number
	//
	u64 magic;
	num_read = fread( &magic, sizeof( magic ), 1, fh );
	if ( num_read != 1 || magic != ROMDB_MAGIC_NO )
	{
		DBGConsole_Msg( 0, "RomDB has wrong magic number." );
		goto fail;
	}

	//
	// Check the version number
	//
	u32 version;
	num_read = fread( &version, sizeof( version ), 1, fh );
	if ( num_read != 1 || version != ROMDB_CURRENT_VERSION )
	{
		DBGConsole_Msg( 0, "RomDB has wrong version for this build of Daedalus." );
		goto fail;
	}

	u32		num_files;
	num_read = fread( &num_files, sizeof( num_files ), 1, fh );
	if ( num_read != 1 )
	{
		DBGConsole_Msg( 0, "RomDB EOF reading number of files." );
		goto fail;
	}
	else if ( num_files > MAX_SENSIBLE_FILES )
	{
		DBGConsole_Msg( 0, "RomDB has unexpectedly large number of files (%d).", num_files );
		goto fail;
	}

	mRomFiles.resize( num_files );
	if( fread( &mRomFiles[0], sizeof(RomFilesKeyValue), num_files, fh ) != num_files )
	{
		goto fail;
	}
	// Redundant?
	std::sort( mRomFiles.begin(), mRomFiles.end(), SSortByFilename() );

	u32		num_details;
	num_read = fread( &num_details, sizeof( num_details ), 1, fh );
	if ( num_read != 1 )
	{
		DBGConsole_Msg( 0, "RomDB EOF reading number of details." );
		goto fail;
	}
	else if ( num_details > MAX_SENSIBLE_DETAILS )
	{
		DBGConsole_Msg( 0, "RomDB has unexpectedly large number of details (%d).", num_details );
		goto fail;
	}

	mRomDetails.resize( num_details );
	if( fread( &mRomDetails[0], sizeof(RomDetails), num_details, fh ) != num_details )
	{
		goto fail;
	}
	// Redundant?
	std::sort( mRomDetails.begin(), mRomDetails.end(), SSortDetailsByID() );

	DBGConsole_Msg( 0, "RomDB initialised with %d files and %d details.", mRomFiles.size(), mRomDetails.size() );

	fclose( fh );
	return true;

fail:
	fclose( fh );
	return false;
}

bool IRomDB::Commit()
{
	if( !mDirty )
		return true;

	//
	// Check if we have a valid filename
	//
	if ( strlen( mRomDBFileName ) <= 0 )
		return false;

	FILE * fh = fopen( mRomDBFileName, "wb" );

	if ( !fh )
		return false;

	//
	// Write the magic
	//
	fwrite( &ROMDB_MAGIC_NO, sizeof( ROMDB_MAGIC_NO ), 1, fh );

	//
	// Write the version
	//
	fwrite( &ROMDB_CURRENT_VERSION, sizeof( ROMDB_CURRENT_VERSION ), 1, fh );

	{
		u32 num_files( mRomFiles.size() );
		fwrite( &num_files, sizeof( num_files ), 1, fh );
		fwrite( &mRomFiles[0], sizeof(RomFilesKeyValue), num_files, fh );
	}

	{
		u32 num_details( mRomDetails.size() );
		fwrite( &num_details, sizeof( num_details ), 1, fh );
		fwrite( &mRomDetails[0], sizeof(RomDetails), num_details, fh );
	}

	fclose( fh );

	mDirty = true;
	return true;
}

void IRomDB::AddRomEntry( const char * filename, const RomID & id, u32 rom_size, ECicType cic_type )
{
	// Update filename/id map
	FilenameVec::iterator fit( std::lower_bound( mRomFiles.begin(), mRomFiles.end(), filename, SSortByFilename() ) );
	if( fit != mRomFiles.end() && strcmp( fit->FileName, filename ) == 0 )
	{
		fit->ID = id;
	}
	else
	{
		RomFilesKeyValue	filename_id( filename, id );
		mRomFiles.insert( fit, filename_id );
	}

	// Update id/details map
	DetailsVec::iterator dit( std::lower_bound( mRomDetails.begin(), mRomDetails.end(), id, SSortDetailsByID() ) );
	if( dit != mRomDetails.end() && dit->ID == id )
	{
		dit->RomSize = rom_size;
		dit->CicType = cic_type;
	}
	else
	{
		RomDetails	id_details( id, rom_size, cic_type );
		mRomDetails.insert( dit, id_details );
	}

	mDirty = true;
}

void IRomDB::AddRomDirectory(const char * directory)
{
	DBGConsole_Msg(0, "Adding roms directory [C%s]", directory);
	std::string			full_path;

	IO::FindHandleT		find_handle;
	IO::FindDataT		find_data;
	if(IO::FindFileOpen( directory, &find_handle, find_data ))
	{
		do
		{
			const char * rom_filename = find_data.Name;
			if(IsRomfilename( rom_filename ))
			{
				IO::Filename full_path;
				IO::Path::Combine(full_path, directory, rom_filename);

				AddRomFile(full_path);
			}
		}
		while(IO::FindFileNext( find_handle, find_data ));

		IO::FindFileClose( find_handle );
	}
}

void IRomDB::AddRomFile(const char * filename)
{
	RomID id;
	u32 rom_size;
	ECicType boot_type;

	QueryByFilename(filename, &id, &rom_size, &boot_type);
}

static bool GenerateRomDetails( const char * filename, RomID * id, u32 * rom_size, ECicType * cic_type )
{
	//
	//	Haven't seen this rom before - try to add it to the database
	//
	ROMFile * rom_file( ROMFile::Create( filename ) );
	if( rom_file == NULL )
	{
		return false;
	}

	CNullOutputStream messages;

	if( !rom_file->Open( messages ) )
	{
		delete rom_file;
		return false;
	}

	//
	// They weren't there - so we need to find this info out for ourselves
	// Only read in the header + bootcode
	//
	u32		bytes_to_read( RAMROM_GAME_OFFSET );
	u32		size_aligned( AlignPow2( bytes_to_read, 4 ) );	// Needed?
	u8 *	bytes( new u8[size_aligned] );

	if( !rom_file->LoadData( bytes_to_read, bytes, messages ) )
	{
		// Lots of files don't have any info - don't worry about it
		delete [] bytes;
		delete rom_file;
		return false;
	}

	//
	// Get the address of the rom header
	// Setup the rom id and size
	//
	*rom_size = rom_file->GetRomSize();
	if (*rom_size >= RAMROM_GAME_OFFSET)
	{
		*cic_type = ROM_GenerateCICType( bytes );
	}
	else
	{
		*cic_type = CIC_UNKNOWN;
	}

	//
	//	Swap into native format
	//
	ROMFile::ByteSwap_3210( bytes, bytes_to_read );

	const ROMHeader * prh( reinterpret_cast<const ROMHeader *>( bytes ) );
	*id = RomID( prh->CRC1, prh->CRC2, prh->CountryID );

	delete [] bytes;
	delete rom_file;
	return true;
}

bool IRomDB::QueryByFilename( const char * filename, RomID * id, u32 * rom_size, ECicType * cic_type )
{
	//
	// First of all, check if we have these details cached in the rom database
	//
	FilenameVec::const_iterator fit( std::lower_bound( mRomFiles.begin(), mRomFiles.end(), filename, SSortByFilename() ) );
	if( fit != mRomFiles.end() && strcmp( fit->FileName, filename ) == 0 )
	{
		if( QueryByID( fit->ID, rom_size, cic_type ) )
		{
			*id = fit->ID;
			return true;
		}
	}

	if( GenerateRomDetails( filename, id, rom_size, cic_type ) )
	{
		//
		// Store this information for future reference
		//
		AddRomEntry( filename, *id, *rom_size, *cic_type );
		return true;
	}

	return false;
}

bool IRomDB::QueryByID( const RomID & id, u32 * rom_size, ECicType * cic_type ) const
{
	DetailsVec::const_iterator it( std::lower_bound( mRomDetails.begin(), mRomDetails.end(), id, SSortDetailsByID() ) );
	if( it != mRomDetails.end() && it->ID == id )
	{
		*rom_size = it->RomSize;
		*cic_type = it->CicType;
		return true;
	}

	// Can't generate the details, as we have no filename to work with
	return false;
}

const char * IRomDB::QueryFilenameFromID( const RomID & id ) const
{
	for( u32 i = 0; i < mRomFiles.size(); ++i )
	{
		if( mRomFiles[ i ].ID == id )
		{
			return mRomFiles[ i ].FileName;
		}
	}

	return NULL;
}
