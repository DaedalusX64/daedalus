#include <map>
#include <filesystem>

// Temporary 
// enum ESaveType :uint32_t
// {
// 	NONE,
// 	EEP4K,
// 	EEP16K,
// 	SRAM,
// 	FLASH,
// };
// constexpr uint32_t kNumSaveTypes = static_cast<uint32_t>(ESaveType::FLASH) + 1;

// Let's fill this struct with the minimum required for the index
struct GameData
{
    std::filesystem::path file;
    std::string internalName;
    std::string CRC;
    std::string gameName;
    ESaveType saveType;
    std::string previewImage;
        // ESaveType SaveType; // This will need to be the SaveType Enum
  GameData() = default;

  GameData(const std::filesystem::path& p, const std::string& n, const std::string& c, const std::string& o, const ESaveType& t =ESaveType::NONE, const std::string& j ="")
      : file(p), internalName(n), CRC(c), gameName(o), saveType(t), previewImage(j){}

};


std::map<std::string, GameData> index(const std::filesystem::path& file);
