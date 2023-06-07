#include <map>
#include <filesystem>
#include "Core/RomSettings.h"
#include "Core/ROM.h"

#pragma once 

struct GameData
{
    std::filesystem::path file;
    std::string internalName;
    std::string CRC;
    std::string gameName;
    ESaveType saveType;
    std::string previewImage;
  GameData() = default;

  GameData(const std::filesystem::path& p, const std::string& n, const std::string& c, const std::string& o, const ESaveType& t =ESaveType::NONE, const std::string& j ="")
      : file(p), internalName(n), CRC(c), gameName(o), saveType(t), previewImage(j){}

};

const GameData* findGameByFilename(const GameInfo& gameinfo, const std::string& filenameToFind);

std::map<std::string, GameData> index(const std::filesystem::path& file);
std::map<std::string, GameData>::const_iterator findGameByFilename(const std::map<std::string, GameData>& gameinfo, const std::string& filename);
