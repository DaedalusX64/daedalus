/*
Copyright (C) 2006 StrmnNrmn

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
#include "RomFile/RomFileUncompressed.h"
#include <iostream>
#include <system_error> // For std::error_code and std::system_category


//*****************************************************************************
//
//*****************************************************************************
ROMFileUncompressed::ROMFileUncompressed( const std::filesystem::path filename )
:	ROMFile( filename )
,	mFH( filename, std::ios::in | std::ios::binary)
,	mRomSize( 0 )
{

}

//*****************************************************************************
//
//*****************************************************************************
ROMFileUncompressed::~ROMFileUncompressed()
{
    // mFH will automatically be closed by its destructor
}

//*****************************************************************************
//
//*****************************************************************************
bool ROMFileUncompressed::Open( COutputStream & messages [[maybe_unused]] )
{
    #ifdef DAEDALUS_ENABLE_ASSERTS
    // DAEDALUS_ASSERT( mFH == NULL, "Opening the file twice?" );
    #endif

    if (!mFH.is_open()) {
        std::cerr << "File not open: " << mFilename << std::endl;
        return false;
    }

    // Determine which byteswapping mode to use
    u32 header;
    mFH.read(reinterpret_cast<char*>(&header), sizeof(u32));
    if (mFH.gcount() != sizeof(u32)) {
        std::cerr << "Failed to read header: " << mFilename << std::endl;
        return false;
    }
    if (!SetHeaderMagic(header)) {
        return false;
    }

    // Determine the rom size
    mFH.seekg(0, std::ios::end);
    if (!mFH) {
        std::cerr << "Failed to seek to end of file: " << mFilename << std::endl;
        return false;
    }
    
    mRomSize = mFH.tellg();
    if (mRomSize == -1) {
        std::cerr << "Failed to get file size: " << mFilename << std::endl;
        return false;
    }

    mFH.seekg(0, std::ios::beg);
    if (!mFH) {
        std::cerr << "Failed to seek to beginning of file: " << mFilename << std::endl;
        return false;
    }

    return true;
}

//*****************************************************************************
//
//*****************************************************************************
bool ROMFileUncompressed::LoadRawData( u32 bytes_to_read, u8 *p_bytes, COutputStream & messages [[maybe_unused]] )
{
    #ifdef DAEDALUS_ENABLE_ASSERTS
    // DAEDALUS_ASSERT( mFH != NULL, "Reading data when Open failed?" );
    #endif
    if (p_bytes == NULL) {
        std::cerr << "Null pointer for data buffer" << std::endl;
        return false;
    }

    // Try and read in data - reset to the start of the file
    mFH.seekg(0, std::ios::beg);
    if (!mFH) {
        std::cerr << "Failed to seek to beginning of file" << std::endl;
        return false;
    }

    mFH.read(reinterpret_cast<char*>(p_bytes), bytes_to_read);
    if (mFH.gcount() != bytes_to_read) {
        std::cerr << "Failed to read expected number of bytes from LoadRawData. Read " << mFH.gcount() << " out of " << bytes_to_read << std::endl;
        return false;
    }

    // Apply the bytesswapping before returning the buffer
    CorrectSwap(p_bytes, bytes_to_read);

    return true;
}

//*****************************************************************************
//
//*****************************************************************************
bool	ROMFileUncompressed::ReadChunk( u32 offset, u8 * p_dst, u32 length )
{
    #ifdef DAEDALUS_ENABLE_ASSERTS
    // DAEDALUS_ASSERT( mFH != NULL, "Reading data when Open failed?" );
    #endif
    // Try and read in data - reset to the specified offset
    mFH.seekg(offset, std::ios::beg);
    if (!mFH) {
        std::cerr << "Failed to seek to offset " << offset << std::endl;
        return false;
    }

    mFH.read(reinterpret_cast<char*>(p_dst), length);
    if (mFH.gcount() != length) {
        std::cerr << "Failed to read expected number of bytes from ReadChunk. Read " << mFH.gcount() << " out of " << length << std::endl;
        return false;
    }

    // Apply the bytesswapping before returning the buffer
    CorrectSwap(p_dst, length);
    return true;
}
