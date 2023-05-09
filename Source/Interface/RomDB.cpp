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

#include "BuildOptions.h"
#include "Base/Types.h"

#include <algorithm>
#include <filesystem>
#include <string_view>
#include <sstream>
#include <iostream>
#include <vector>
#include <codecvt> // For the UTF Conversion


#include "Core/ROM.h"
#include "Core/ROMImage.h"
#include "Debug/DBGConsole.h"
#include "Interface/RomDB.h"
#include "Base/MathUtil.h"
#include "System/IO.h"
#include "RomFile/RomFile.h"
#include "Utility/Stream.h"
#include "Interface/Preferences.h"

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

		// CRomDB implementation
		void			Reset();

		void			AddRomDirectory(const std::filesystem::path directory);

		bool			QueryByFilename( const std::filesystem::path filename, RomID * id, u32 * rom_size, ECicType * cic_type );
		bool			QueryByID( const RomID & id, u32 * rom_size, ECicType * cic_type ) const;
		const char *	QueryFilenameFromID( const RomID & id ) const;
		void			RomIndex( std::filesystem::path& filename);
		void 			BuildIndex( std::filesystem::path& filename);
		void 			GetSettings(std::filesystem::path &filename, RomID &id);
		
	private:
		void			AddRomFile(const std::filesystem::path filename);
		void			AddRomEntry( const std::filesystem::path filename, const RomID & id, u32 rom_size, ECicType cic_type );
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
				strcpy( FileName, filename.c_str() );
				ID = id;
			}

			// This is actually IO::Path::kMaxPathLen+1, but we need to ensure that it doesn't change if we ever change the kMaxPathLen constant.
			static const u32 kMaxFilenameLen = 260;
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
	using FilenameVec = std::vector< RomFilesKeyValue >;
	using DetailsVec = std::vector< RomDetails >;
	
		IO::Filename					mRomDBFileName;
		FilenameVec						mRomFiles;
		DetailsVec						mRomDetails;
		bool							mDirty;
};

// It seems the only file that uses this is SaveState
template<> bool	CSingleton< CRomDB >::Create()
{	
	DAEDALUS_ASSERT_Q(mpInstance == nullptr);
	mpInstance = std::make_shared<IRomDB>();
		return true;
}

IRomDB::IRomDB() {}

IRomDB::~IRomDB() {}

void IRomDB::Reset()
{
	mRomFiles.clear();
	mRomDetails.clear();
	mDirty = true;
}

bool IRomDB::OpenDB( const std::filesystem::path filename )
{
}

void IRomDB::AddRomEntry( const std::filesystem::path filename, const RomID & id, u32 rom_size, ECicType cic_type )
{
	// Update filename/id map
	FilenameVec::iterator fit( std::lower_bound( mRomFiles.begin(), mRomFiles.end(), filename.c_str(), SSortByFilename() ) );
	if( fit != mRomFiles.end() && strcmp( fit->FileName, filename.c_str() ) == 0 )
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

void IRomDB::AddRomDirectory(const std::filesystem::path directory)
{
	DBGConsole_Msg(0, "Adding roms directory [C%s]", directory.c_str());

	std::string			full_path;

	IO::FindHandleT		find_handle;
	IO::FindDataT		find_data;
	if(IO::FindFileOpen( directory.c_str(), &find_handle, find_data ))
	{
		do
		{
			const std::filesystem::path rom_filename = find_data.Name;
			if(IsRomfilename( rom_filename.c_str() ))
			{
				IO::Filename full_path;
				IO::Path::Combine(full_path, directory.c_str(), rom_filename.c_str());

				AddRomFile(full_path);
			}
		}
		while(IO::FindFileNext( find_handle, find_data ));

		IO::FindFileClose( find_handle );
	}
}

void IRomDB::AddRomFile(const std::filesystem::path filename)
{
	RomID id;
	u32 rom_size;
	ECicType boot_type;

	QueryByFilename(filename, &id, &rom_size, &boot_type);
}

static bool GenerateRomDetails( const std::filesystem::path filename, RomID * id, u32 * rom_size, ECicType * cic_type )
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

bool IRomDB::QueryByFilename( std::filesystem::path filename, RomID * id, u32 * rom_size, ECicType * cic_type )
{	
	// First of all, check if we have these details cached in the rom database
	// FilenameVec::const_iterator fit( std::lower_bound( mRomFiles.begin(), mRomFiles.end(), filename.c_str(), SSortByFilename() ) );
	// if( fit != mRomFiles.end() && strcmp( fit->FileName, filename.c_str() ) == 0 )
	// {
	// 	if( QueryByID( fit->ID, rom_size, cic_type ) )
	// 	{
	// 		*id = fit->ID;
	// 		return true;
	// 	}
	// }

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
	// Move this to a utility file
constexpr std::uint32_t fnv1a_hash(std::string_view str, std::uint32_t hash = 2166136261)
{
    for (auto c : str)
    {
        hash ^= static_cast<std::uint32_t>(c);
        hash *= 16777619;
    }

    return hash;
}


/*
We really need to build up the information about the ROM here and initialise the gROM Struct with this data

This needs to be called early so we can setup the values below
gROM.GameName
gROM.FileName
gROM.Preview // Will be setup in the RomIndex Function.
gROM.SaveType // Setup in the RomIndex Function
gROM.rh.CRC1
gROM.rh.CRC2
gROM.ExpansionPakUsage // Needed??

// Open Rom
*/

std::vector<std::filesystem::path> romList;

// Build the Rom Index and Sort
void IRomDB::BuildIndex( std::filesystem::path& filename)
{
	std::filesystem::path romDir = "Roms";

	for (const auto& entry : std::filesystem::directory_iterator(romDir))
	{
			// Just filter out directories for now
		if (entry.is_regular_file())
		{
			filename = entry.path().filename().string();
			romList.push_back(filename);
		
		}
	}
	std::sort(romList.begin(), romList.end());
}

void IRomDB::GetSettings(std::filesystem::path &filename, RomID &id)
{

	// Load Rom File 
	
	for (auto i : romList)
	{
	ROM_LoadFile(i);
	}
}


void IRomDB::RomIndex( std::filesystem::path& filename) 
{
	RomID id;
	BuildIndex(filename);
	GetSettings(filename, id);


g_ROM.mFileName = filename;
std::ostringstream oss;
oss.str("");

// Initialise some Settings

// Append CRC1 and CRC2 values and convert to a hexadecimal string

std::cout << g_ROM.rh.CRC1;
std::cout << g_ROM.rh.CRC2;

oss << std::hex << g_ROM.rh.CRC1 << g_ROM.rh.CRC2;
std::string crcCountry = oss.str();

// Convert the string to UTF-8 encoding
std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> convert;
std::u32string utf32 = convert.from_bytes(crcCountry);

// Print the UTF-8 string to the console
std::cout << std::string(utf32.begin(), utf32.end()) << std::endl;

std::cout << "CRC Country is: " << crcCountry << std::endl;
    // Determine game name and save type based on the joined string
    switch (fnv1a_hash(crcCountry.data()))
    {
        case fnv1a_hash("ff2b5a632623028b"):
            g_ROM.settings.GameName = "Super Mario 64";
            g_ROM.settings.SaveType = ESaveType::EEP4K;
			std::cout << "SaveType in DB " << static_cast<int>(g_ROM.settings.SaveType) << std::endl;
            break;
		case fnv1a_hash("134c3f03a7e79e31"):
        case fnv1a_hash("66092ea912391c34"):
		case fnv1a_hash("9516943beb5e0af9"):
            g_ROM.settings.GameName = "007 - The World is not Enough (Beta)";
			g_ROM.settings.Preview = "007_The World_Is_Not_Enough.png";
            break;
		case fnv1a_hash("61f1ba1ff1541c2c"):
		case fnv1a_hash("253ffd588daa2ed9"):
            g_ROM.settings.GameName = "1080 Snowboarding";
			g_ROM.settings.SaveType = ESaveType::SRAM;
			g_ROM.settings.Preview = "1080_Snowboarding.png";
            	break;
		case fnv1a_hash("091da5abd9ba68c"):
            g_ROM.settings.GameName = "40 Winks";
			g_ROM.settings.SaveType = ESaveType::EEP4K;
			g_ROM.settings.Preview = "40_Winks.png";
            	break;
		case fnv1a_hash("56a48bb9af762b5b"):
            g_ROM.settings.GameName = "64 de Hakken!! Tamagotchi - Minna de Tamagotchi World";
			g_ROM.settings.SaveType = ESaveType::EEP4K;
			g_ROM.settings.Preview = "Tamagotchi_World_64.png";
            	break;
		case fnv1a_hash("bf2ff236f2128931"):
            g_ROM.settings.GameName = "64 Hanafuda - Tenshi no Yakusoku";
			g_ROM.settings.SaveType = ESaveType::EEP4K;
			g_ROM.settings.Preview = "64_Hanafuda_-_Tenshi_no_Yakusoku.png";
            	break;
		case fnv1a_hash("6910969c8d48eaf5"):
            g_ROM.settings.GameName = "64 Oozumou";
			g_ROM.settings.SaveType = ESaveType::EEP4K;
			g_ROM.settings.Preview = "Ohzumou_64.png";
            	break;
		case fnv1a_hash("168bc185af2296df"):
            g_ROM.settings.GameName = "64 Oozumo 2";
			// g_ROM.settings.SaveType = ESaveType::EEP4K;;
			g_ROM.settings.Preview = "Ohzumou_64_2.png";
			/// Oozumou 1 uses EEP4K?
            	break;
		case fnv1a_hash("fc81607a787a8fff"):
            g_ROM.settings.GameName = "64 Trump Collection - Alice no Wakuwaku Trump World";
			g_ROM.settings.SaveType = ESaveType::EEP4K;;
			g_ROM.settings.Preview = "Alice_no_Wakuwaku_Trump_World.png";
            	break;
		case fnv1a_hash("d927f2df69814d0d"):
		case fnv1a_hash("fd04dc82f4822dcf"):
		case fnv1a_hash("c0ae382b10b85063"):
		case fnv1a_hash("ce893bf67dd58245"):
		case fnv1a_hash("96c0128fe117dc45"):
            g_ROM.settings.GameName = "A Bug's Life";
			g_ROM.settings.Preview = "A_Bugs_Life.png";
            	break;
		case fnv1a_hash("f18b591b459ba2ec"):
		case fnv1a_hash("95bef662d6d602f1"):
            g_ROM.settings.GameName = "Aero Fighters Assault";
			g_ROM.settings.SaveType = ESaveType::EEP4K;
			g_ROM.settings.Preview = "AeroFighters_Assault.png";
            	break;	
		case fnv1a_hash("3e46beae4b4671cc"):
		case fnv1a_hash("c90309b046c11639"):
		case fnv1a_hash("3111f480f6454638"):
		case fnv1a_hash("c84530d8363a9df2"):
            g_ROM.settings.GameName = "AeroGauge";
			g_ROM.settings.SaveType = ESaveType::EEP4K;
			g_ROM.settings.Preview = "Aero_Gauge.png";
            	break;
		case fnv1a_hash("a682c18cb0cad0c2"):
            g_ROM.settings.GameName = "AI Shougi 3";
			g_ROM.settings.SaveType = ESaveType::EEP4K;
			g_ROM.settings.Preview = "AI_Shogi_3.png";
            	break;
		case fnv1a_hash("d2512011acf8be68"):
		case fnv1a_hash("ccffc42d215affc8"):
		case fnv1a_hash("4f5aa9e623ead2ba"):
            g_ROM.settings.GameName = "Aidyn Chronicles - The First Mage";
			g_ROM.settings.Preview = "Aidyn_Chronicles_The_First_Mage.png";
			// Requires clean scene enabled
            	break;
		case fnv1a_hash("0cb6456c300ee5dc"):
		case fnv1a_hash("d025c427c1992d8c"):
            g_ROM.settings.GameName = "Airboarder 64";
			g_ROM.settings.SaveType = ESaveType::EEP4K;
			g_ROM.settings.Preview = "Airboarder_64.png";
            	break;
		case fnv1a_hash("941a95b6af49c863"):
            g_ROM.settings.GameName = "Akumajou Dracula Mokushiroku - Real Action Adventure";
			g_ROM.settings.SaveType = ESaveType::EEP16K;
			g_ROM.settings.Preview = "Akumajou_Dracula_Mokushiroku_Real_Action_Adventure.png";
            	break;
		case fnv1a_hash("063153a55b5ef2b9"):
            g_ROM.settings.GameName = "Akumajou Dracula Mokushiroku Gaiden - Legend of Cornell";
			g_ROM.settings.SaveType = ESaveType::EEP16K;
			g_ROM.settings.Preview = "Akumajou_Dracula_Mokushiroku_Gaiden_Legend_of_Cornell.png";
            	break;
		case fnv1a_hash("ad84d7df036642ae"):
		case fnv1a_hash("91e285e16d76504e"):
            g_ROM.settings.GameName = "All Star Tennis '99";
			g_ROM.settings.SaveType = ESaveType::EEP4K;
			g_ROM.settings.Preview = "All_Star_Tennis_99.png";
            	break;
		case fnv1a_hash("a7233ec41a68b140"):
		case fnv1a_hash("4dd5edd974e2b86b"):
            g_ROM.settings.GameName = "All Star Baseball 99";
			g_ROM.settings.Preview = "All_Star_Baseball_99.png";
            	break;
		case fnv1a_hash("4d7a545e9507e690"):
            g_ROM.settings.GameName = "All Star Baseball 2000";
			g_ROM.settings.Preview = "All_Star_Baseball_2000.png";
            	break;
		case fnv1a_hash("efc64654bb478ee1"):
            g_ROM.settings.GameName = "All Star Baseball 2001";
			g_ROM.settings.Preview = "All_Star_Baseball_2001.png";
            	break;
		case fnv1a_hash("17dbf6c032d5e080"):
		case fnv1a_hash("32d9b51f1b48a93b"):
		case fnv1a_hash("5071c73c87b9cd21"):
            g_ROM.settings.GameName = "Armorines - Project S.W.A.R.M.";
			g_ROM.settings.Preview = "Armorines_Project_SWARM.png";
            	break;
		case fnv1a_hash("b2bb524c6b0fabce"):
            g_ROM.settings.GameName = "Army Men - Air Combat";
			g_ROM.settings.Preview = "Army_Men_Air_Combat.png";
            	break;
		case fnv1a_hash("57062c866d89fd8d"):
		case fnv1a_hash("19df10b21a8db598"):
            g_ROM.settings.GameName = "Army Men - Sarge's Heroes";
			g_ROM.settings.Preview = "Army_Men_Sarges_Heroes.png";
            	break;
		case fnv1a_hash("b6730fb234fc7529"):
            g_ROM.settings.GameName = "Army Men - Sarge's Heroes 2";
			g_ROM.settings.Preview = "Army_Men_Sarges_Heroes_2.png";
            	break;
		case fnv1a_hash("abd8f7d146043b29"):
            g_ROM.settings.GameName = "Asteroids Hyper 64";
			g_ROM.settings.Preview = "Asteroids_Hyper_64.png";
            	break;
		case fnv1a_hash("c45db2418667721b"):
		case fnv1a_hash("bf9777fc3ce8954a"):
            g_ROM.settings.GameName = "Automobili Lamborghini";
			g_ROM.settings.Preview = "Automobili_Lamborghini.png";
            	break;
		case fnv1a_hash("9ca440e3418d3174"):
            g_ROM.settings.GameName = "Baku Bomberman";
			g_ROM.settings.SaveType = ESaveType::EEP4K;
			g_ROM.settings.Preview = "Baku_Bomberman.png";
            	break;
		case fnv1a_hash("4f7c3ce738b893af"):
            g_ROM.settings.GameName = "Baku Bomberman 2";
			g_ROM.settings.SaveType = ESaveType::EEP4K;
			g_ROM.settings.Preview = "Baku_Bomberman_2.png";
            	break;
		case fnv1a_hash("5db998df78098458"):
            g_ROM.settings.GameName = "Bakuretsu Muteki Bangaioh";
			g_ROM.settings.SaveType = ESaveType::EEP4K;		
			g_ROM.settings.Preview = "Bakuretsu_Muteki_Bangaioh.png";
            	break;
		case fnv1a_hash("20d568510dcd5fca"):
            g_ROM.settings.GameName = "Banjo to Kazooie no Daibouken";
			g_ROM.settings.SaveType = ESaveType::EEP4K;		
			g_ROM.settings.Preview = "Banjo_to_Kazooie_no_Daibouken.png";
            	break;
		case fnv1a_hash("b1cc3f73f9924844"):
		case fnv1a_hash("0693bfa4d1df0cbf"):
		case fnv1a_hash("ac5975cdaef56cb2"):
            g_ROM.settings.GameName = "Banjo-Kazooie";
			g_ROM.settings.SaveType = ESaveType::EEP4K;	
			g_ROM.settings.Preview = "Banjo_Kazooie.png";
            	break;
		case fnv1a_hash("396d17c9d17947ea"):
		case fnv1a_hash("9aaae9c2aa705d47"):
		case fnv1a_hash("df7c5b152573daf0"):
            g_ROM.settings.GameName = "Banjo-Tooie";
			g_ROM.settings.SaveType = ESaveType::EEP16K;	
			g_ROM.settings.Preview = "Banjo_Tooie.png";
            	break;	
		case fnv1a_hash("00694b518198b1b4"):
            g_ROM.settings.GameName = "Banjo to Kazooie no Daibouken 2";
			g_ROM.settings.SaveType = ESaveType::EEP16K;
			g_ROM.settings.Preview = "Banjo_to_Kazooie_no_Daibouken_2.png";
            	break;
		case fnv1a_hash("ac3363d79d21b60c"):
            g_ROM.settings.GameName = "Bass Rush - ECOGEAR PowerWorm Championship";
			g_ROM.settings.Preview = "Bass_Tsuri_No._1.png";
				
				
        default:
			g_ROM.settings.GameName = "Unknown";
            g_ROM.settings.SaveType = ESaveType::EEP4K; // As a sanity check, set to the most popular rom
            break;
    }
	std::cout << g_ROM.settings.GameName << std::endl;

}
