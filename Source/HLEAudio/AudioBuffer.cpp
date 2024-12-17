/*
Copyright (C) 2007 StrmnNrmn

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
#include "Interface/ConfigOptions.h"
#include "Debug/DBGConsole.h"
#include "HLEAudio/AudioBuffer.h"
#include "System/Thread.h"
#include <cstring>
#include <fstream>

#ifdef DAEDALUS_PSP
#include "SysPSP/Utility/CacheUtil.h"
#endif

CAudioBuffer::CAudioBuffer(u32 buffer_size)
    : mBufferBegin(new Sample[buffer_size]),
      mBufferEnd(mBufferBegin + buffer_size), mReadPtr(mBufferBegin),
      mWritePtr(mBufferBegin) {}

CAudioBuffer::~CAudioBuffer() { delete[] mBufferBegin; }

u32 CAudioBuffer::GetNumBufferedSamples() const {
    s32 diff = mWritePtr - mReadPtr;
    if (diff < 0) {
        diff += (mBufferEnd - mBufferBegin); // Add on buffer length
    }
    return diff;
}

void CAudioBuffer::AddSamples(const Sample *samples, u32 num_samples,
                              u32 frequency, u32 output_freq) {
#ifdef DAEDALUS_ENABLE_ASSERTS
    DAEDALUS_ASSERT(frequency <= output_freq, "Input frequency is too high");
#endif

#ifdef DAEDALUS_DEBUG_AUDIO
    std::ofstream fh;
    if (!fh.is_open()) {
        fh.open("audio_in.raw", std::ios::binary);
        fh.write(reinterpret_cast<const char *>(samples), sizeof(Sample) * num_samples);
        fh.flush();
    }
#endif


    Sample *write_ptr = mWritePtr;
    const Sample *read_ptr = mReadPtr;

    const s32 r = (frequency << 12) / output_freq;
    s32 s = 0;
    u32 in_idx = 0;

    u32 output_samples = std::min(((num_samples * output_freq) / frequency),
                                  (u32)(mBufferEnd - mWritePtr)); // Avoid overflow

    for (u32 i = 0; i < output_samples; i++) {
        Sample out;
        out.L = samples[in_idx].L + (((samples[in_idx + 1].L - samples[in_idx].L) * s) >> 12);
        out.R = samples[in_idx].R + (((samples[in_idx + 1].R - samples[in_idx].R) * s) >> 12);

        s += r;
        in_idx += s >> 12;
        s &= 4095;

        *write_ptr++ = out;

        if (write_ptr >= mBufferEnd) {
            write_ptr = mBufferBegin;
        }

        // Prevent buffer overflow by advancing read_ptr if needed
        if (write_ptr == read_ptr) {
            read_ptr++;
            if (read_ptr >= mBufferEnd) {
                read_ptr = mBufferBegin;
            }
        }
    }

    mWritePtr = write_ptr;
    mReadPtr = read_ptr;
}

u32 CAudioBuffer::Drain(Sample *samples, u32 num_samples) {

    const Sample *read_ptr = mReadPtr;
    Sample *out_ptr = samples;
    u32 samples_required = num_samples;

    while (samples_required > 0) {
        if (read_ptr == mWritePtr) {
            break; // Buffer is empty
        }

        *out_ptr++ = *read_ptr++;
        if (read_ptr >= mBufferEnd) {
            read_ptr = mBufferBegin; // Circular buffer wrap-around
        }

        samples_required--;
    }

#ifdef DAEDALUS_DEBUG_AUDIO
    std::ofstream fh;
    if (!fh.is_open()) {
        fh.open("audio_out.raw", std::ios::binary);
        fh.write(reinterpret_cast<const char *>(samples), sizeof(Sample) * (num_samples - samples_required));
        fh.flush();
    }
#endif

    mReadPtr = read_ptr;

    // Zero out remaining samples if buffer was empty
    if (samples_required > 0) {
        memset(out_ptr, 0, samples_required * sizeof(Sample));
    }

    return num_samples - samples_required;
}
