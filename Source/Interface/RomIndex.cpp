
#include <iostream>
#include <string>
#include <string_view>
#include <vector>
#include <array>
#include <filesystem>
#include <fstream>
#include <numeric>

// Temporary 
enum ESaveType :uint32_t
{
	UNKNOWN,
	EEP4K,
	EEP16K,
	SRAM,
	FLASH,
};
constexpr uint32_t kNumSaveTypes = static_cast<uint32_t>(ESaveType::FLASH) + 1;

// Let's fill this struct with the minimum required for the index
struct GameData
{
    std::filesystem::path file;
    std::string internalName;
    std::string CRC;
    std::string gameName;
        // ESaveType SaveType; // This will need to be the SaveType Enum
  GameData() = default;

  GameData(const std::filesystem::path& p, const std::string& n, const std::string& c, const std::string& o)
      : file(p), internalName(n), CRC(c), gameName(o) {}

};

    std::vector<GameData> gameinfo;
    GameData data;


std::string readCRC(const std::filesystem::directory_entry &entry) {
    
    const std::filesystem::path filename = entry.path();
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error opening file " << filename << std::endl;
    }

    uint32_t crc1, crc2 = 0;

    file.seekg(0x10);
    file.read(reinterpret_cast<char*>(&crc1), sizeof(crc2));

    file.seekg(0x14);
    file.read(reinterpret_cast<char*>(&crc2), sizeof(crc2));

    // Convert to string
    std::ostringstream oss;
    oss.str("");
    oss << std::hex << crc1 << crc2;
    std::string crc_result = oss.str();
    return std::string(crc_result);
}


std::string get_internal_name(const std::filesystem::directory_entry &entry) {
    // Open ROM file for binary reading

    const std::filesystem::path rom_filename = entry.path();

    std::ifstream rom_file(rom_filename, std::ios::binary);

    // Read 20-byte internal name from ROM header
    rom_file.seekg(0x20);
    char internal_name[21];
    rom_file.read(internal_name, 20);
    internal_name[20] = '\0';

    // Close ROM file
    rom_file.close();

    // Return internal name as a string
    return std::string(internal_name);
}

// Get List of Rom Files and add to struct
std::vector<GameData> index(const std::filesystem::path &file)
{
        // Just target directory Roms for now, we'll need to add
    for (const auto& entry : std::filesystem::directory_iterator(file))
    {
            if (entry.is_regular_file())
            {
                std::string CRC = "";
                // GameData data;
                // data.file = entry.path();
                // data.internalName = get_internal_name(entry);
                // data.CRC = buildCRC(data);

                // std::string internal_name = get_internal_name(entry);
                // std::filesystem::path filename = entry.path().filename().string();
                // std::string crc_value = buildCRC(entry);
               gameinfo.emplace_back(entry.path(), get_internal_name(entry), readCRC(entry), "Unknown Game"  );

            }
    }
    return gameinfo;
}


constexpr std::uint32_t fnv1a_hash(std::string_view str, std::uint32_t hash = 2166136261)
{
    for (auto c : str)
    {
        hash ^= static_cast<std::uint32_t>(c);
        hash *= 16777619;
    }

    return hash;
}



int main()
{


    std::filesystem::path romDir = "Roms";
    index("Roms");
    for (auto i : gameinfo)
    {
    std::string_view crcResult = i.CRC;

    switch (fnv1a_hash(crcResult.data()))
    {
      case fnv1a_hash("ff2b5a632623028b"):
           data.gameName = "Super Mario 64";
           std::cout << "It's a me a mario";
            // g_ROM.settings.SaveType = ESaveType::EEP4K;
			// std::cout << "SaveType in DB " << static_cast<int>(g_ROM.settings.SaveType) << std::endl;
            break;
    }
        // std::cout << i.file << " "<< i.internalName << " "<< crcResult << std::endl;
        
    }

}