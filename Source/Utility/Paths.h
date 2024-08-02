
#ifndef UTILITY_PATHS_H
#define UTILITY_PATHS_H


#include <filesystem>

extern const std::filesystem::path baseDir;

std::filesystem::path setBasePath(const std::filesystem::path& path);
#endif //UTILITY_PATHS_H