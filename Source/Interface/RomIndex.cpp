
#include <iostream>
#include <string>
#include <string_view>
#include <vector>
#include <array>
#include <filesystem>
#include <fstream>
#include <numeric>
#include <sstream>
#include <unordered_map>

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
    ESaveType saveType;
        // ESaveType SaveType; // This will need to be the SaveType Enum
  GameData() = default;

  GameData(const std::filesystem::path& p, const std::string& n, const std::string& c, const std::string& o, const ESaveType& t =ESaveType::UNKNOWN)
      : file(p), internalName(n), CRC(c), gameName(o), saveType(t) {}

};

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
std::unordered_map<std::string, GameData> index(const std::filesystem::path& file) {
    std::unordered_map<std::string, GameData> gameinfo;
    for (const auto& entry : std::filesystem::directory_iterator(file)) {
        if (entry.is_regular_file()) {
            std::string crc = readCRC(entry);
            GameData data(file.path(), get_internal_name(entry), crc, "Unknown Game",ESaveType::UNKNOWN);
            gameinfo.emplace(crc, std::move(data));
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

       std::unordered_map<std::string, GameData> gameinfo = index("Roms");


    // Access game info by CRC value // Doesn't look like we have much choice here but to do it this way
    std::string crc = "";
    for (auto& it : gameinfo)
    {
        switch(fnv1a_hash(crc))
        {
  case fnv1a_hash("abd8f7d146043b29"):
            gameinfo["abd8f7d146043b29"].gameName = "Asteroids Hyper 64";
            	break;
        }
    }

// if (gameinfo.count("ff2b5a632623028b")) 
//     {
//     gameinfo["ff2b5a632623028b"].gameName = "Super Mario 64";
//     gameinfo["ff2b5a632623028b"].saveType = ESaveType::EEP4K;
//     }
// if (gameinfo.count("b5426c3a1bdaca1a"))
//     {
//         gameinfo["b5426c3a1bdaca1a"].gameName = "Mario Tennis";
//     }


for (const auto& pair : gameinfo) {
    const GameData& data = pair.second;
    std::cout << "File: " << data.file << std::endl;
    std::cout << "Internal Name: " << data.internalName << std::endl;
    std::cout << "CRC: " << data.CRC << std::endl;
    std::cout << "Game Name: " << data.gameName << std::endl;
    std::cout << "Save Type: " << data.saveType << std::endl;
    std::cout << std::endl;
                                    }
    return 0;

}
