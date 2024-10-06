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

#include <algorithm>
#include <vector>
#include <fstream>
#include <cstring>

#include "Core/ROM.h"
#include "Core/ROMImage.h"
#include "Debug/DBGConsole.h"
#include "Interface/RomDB.h"
#include "Utility/MathUtil.h"
#include "RomFile/RomFile.h"
#include "Utility/Stream.h"
#include "Utility/Paths.h"
#include <filesystem>

static const u64 ROMDB_MAGIC_NO	= 0x42444D5244454144LL; //DAEDRMDB		// 44 41 45 44 52 4D 44 42
static const u32 ROMDB_CURRENT_VERSION = 4;

static const u32 MAX_SENSIBLE_FILES = 2048;
static const u32 MAX_SENSIBLE_DETAILS = 2048;

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

		void			AddRomDirectory(const std::filesystem::path& directory);

		bool			QueryByFilename( const std::filesystem::path& filename, RomID * id, u32 * rom_size, ECicType * cic_type );
		bool			QueryByID( const RomID & id, u32 * rom_size, ECicType * cic_type ) const;
		const char *	QueryFilenameFromID( const RomID & id ) const;

	private:
		void			AddRomFile(const std::filesystem::path& filename);

		void			AddRomEntry( const std::filesystem::path& filename, const RomID & id, u32 rom_size, ECicType cic_type );
		bool			OpenDB( const std::filesystem::path filename );

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

			RomFilesKeyValue( const std::filesystem::path filename, const RomID & id )
			{
				memset( FileName, 0, sizeof( FileName ) );
				strcpy( FileName, filename.string().c_str() );
				ID = id;
			}

			// This is actually kMaxPathLen+1, but we need to ensure that it doesn't change if we ever change the kMaxPathLen constant.
			static const u32 kMaxFilenameLen = 260;
			char		FileName[ kMaxFilenameLen + 1 ];
			RomID		ID;
		};

		struct SSortByFilename {
    bool operator()(const RomFilesKeyValue& a, const RomFilesKeyValue& b) const {
        return a.FileName < b.FileName;
    }
    bool operator()(std::string_view a, const RomFilesKeyValue& b) const {
        return a < b.FileName;
    }
    bool operator()(const RomFilesKeyValue& a, std::string_view b) const {
        return a.FileName < b;
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
	using FilenameVec = std::vector< RomFilesKeyValue >;
	using DetailsVec = std::vector< RomDetails >;
	
		std::filesystem::path			mRomDBFileName;
		FilenameVec						mRomFiles;
		DetailsVec						mRomDetails;
		bool							mDirty;
};

template<> bool	CSingleton< CRomDB >::Create()
{
	DAEDALUS_ASSERT_Q(mpInstance == nullptr);
	mpInstance = std::make_shared<IRomDB>();
	mpInstance->OpenDB(setBasePath("rom.db"));
	return true;
}

IRomDB::IRomDB()
:	mDirty( false )
{
	// mRomDBFileName[ 0 ] = '\0';
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

bool IRomDB::OpenDB( const std::filesystem::path filename )
{
	u32 num_read;

	//
	// Remember the filename
	//
	mRomDBFileName = filename;
	
   std::ofstream fh(filename, std::ios::binary);
	
	std::sort( mRomDetails.begin(), mRomDetails.end(), SSortDetailsByID() );
	DBGConsole_Msg( 0, "RomDB initialised with %d files and %d details.", mRomFiles.size(), mRomDetails.size() );
	fh.close();

	return true;

}

bool IRomDB::Commit()
{
    if (!mDirty)
        return true;

    // Check if we have a valid filename
    if (mRomDBFileName.empty()) {
        DBGConsole_Msg(0, "Empty filename.\n");
        return false;
    }

    std::ofstream fh(mRomDBFileName, std::ios::binary);
    if (!fh.is_open())
    {
        DBGConsole_Msg(0, "Failed to open RomDB file %s for writing.\n", mRomDBFileName.c_str());
        return false;
    }

    // Write the magic number
    fh.write(reinterpret_cast<const char*>(&ROMDB_MAGIC_NO), sizeof(ROMDB_MAGIC_NO));

    // Write the version number
    fh.write(reinterpret_cast<const char*>(&ROMDB_CURRENT_VERSION), sizeof(ROMDB_CURRENT_VERSION));

    // Write number of files and mRomFiles data
    {
        u32 num_files = static_cast<u32>(mRomFiles.size());
        fh.write(reinterpret_cast<const char*>(&num_files), sizeof(num_files));
        fh.write(reinterpret_cast<const char*>(mRomFiles.data()), sizeof(RomFilesKeyValue) * num_files);
    }

    // Write number of details and mRomDetails data
    {
        u32 num_details = static_cast<u32>(mRomDetails.size());
        fh.write(reinterpret_cast<const char*>(&num_details), sizeof(num_details));
        fh.write(reinterpret_cast<const char*>(mRomDetails.data()), sizeof(RomDetails) * num_details);
    }

    // Close the file
    fh.close();

    mDirty = false;
    return true;
}

void IRomDB::AddRomEntry( const std::filesystem::path& filename, const RomID & id, u32 rom_size, ECicType cic_type )
{
	// Update filename/id map
	FilenameVec::iterator fit( std::lower_bound( mRomFiles.begin(), mRomFiles.end(), filename.string().c_str(), SSortByFilename() ) );
	if( fit != mRomFiles.end() && strcmp( fit->FileName, filename.string().c_str() ) == 0 )
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

void IRomDB::AddRomDirectory(const std::filesystem::path& directory)
{
	std::filesystem::path romdir = setBasePath(directory);
	DBGConsole_Msg(0, "Adding roms directory [C%s]", romdir.c_str());

	for (const auto& entry : std::filesystem::directory_iterator(romdir))
	{
		if (entry.is_regular_file())
		{
			const std::filesystem::path rom_filename = entry.path().filename();
			if (std::find(valid_extensions.begin(), valid_extensions.end(), rom_filename.extension()) != valid_extensions.end())
			{
				std::filesystem::path rompath = romdir / rom_filename;
				AddRomFile(rompath);
			}
		}
	}

}

void IRomDB::AddRomFile(const std::filesystem::path& filename)
{
	RomID id;
	u32 rom_size;
	ECicType boot_type;

	QueryByFilename(filename, &id, &rom_size, &boot_type);
}

static bool GenerateRomDetails( const std::filesystem::path& filename, RomID * id, u32 * rom_size, ECicType * cic_type )
{
	//
	//	Haven't seen this rom before - try to add it to the database
	//
	auto rom_file = ROMFile::Create( filename );
	if( rom_file == NULL )
	{
		return false;
	}

	CNullOutputStream messages;

	if( !rom_file->Open( messages ) )
	{

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
	return true;
}

bool IRomDB::QueryByFilename( const std::filesystem::path& filename, RomID * id, u32 * rom_size, ECicType * cic_type )
{
	//
	// First of all, check if we have these details cached in the rom database
	//
	FilenameVec::const_iterator fit( std::lower_bound( mRomFiles.begin(), mRomFiles.end(), filename.string().c_str(), SSortByFilename() ) );
	if( fit != mRomFiles.end() && strcmp( fit->FileName, filename.string().c_str() ) == 0 )
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
