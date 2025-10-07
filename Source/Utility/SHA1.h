#pragma once

#include <string>
#include <filesystem>


std::string sha1_from_file(const std::filesystem::path& filename);