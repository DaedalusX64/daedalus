/*
Copyright (C) 2024 DaedalusX64

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
#include "Utility/Paths.h"
#include <iostream>

/*
    Declare the base DaedalusX64 directory for your device.
    If it doesn't require a custom path then all files will appear in the current directory.
*/

#ifdef DAEDALUS_CTR
const std::filesystem::path baseDir = "sdmc:/3ds/DaedalusX64";
#elif defined(DAEDALUS_VITA)
const std::filesystem::path baseDir = "ux0:/data";
#else
const std::filesystem::path baseDir = std::filesystem::current_path();
#endif

// TODO Create directory structure here

std::filesystem::path setBasePath(const std::filesystem::path& path)
{
    auto combinedPath = baseDir / path;
    if (! std::filesystem::is_regular_file(combinedPath))
    {
        std::cout << "Directory :" << combinedPath << "Created";
        std::filesystem::create_directories(combinedPath);
    }
    std::cout << "Path set for:" << path << "to:" << combinedPath << std::endl;
    return combinedPath;
}