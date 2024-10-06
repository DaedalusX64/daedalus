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
  // Todo: Check Cache Routines
  // #ifdef DAEDALUS_PSP
  // 	dcache_wbinv_all();
  // #endif

  // Safe? What if we read mWrite, and then mRead moves to start of buffer?
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
 
 if (!fh.is_open())
 {
  fh.open("audio_in.raw",  std::ios::binary);
fh.write(reinterpret_cast<const char*>(samples), sizeof(Sample) * num_samples);
fh.flush();
 }
#endif 
// clear the Cache
#ifdef DAEDALUS_PSP
// sceKernelDcacheWritebackInvalidateAll();
#endif
  const Sample *read_ptr(
      mReadPtr); // No need to invalidate, as this is uncached/volatile
  Sample *write_ptr(mWritePtr);

  //
  //	'r' is the number of input samples we progress through for each output
  //sample. 	's' keeps track of how far between the current two input samples we
  //are. 	We increment it by 'r' for each output sample we generate. 	When it
  //reaches 1.0, we know we've hit the next sample, so we increment in_idx 	and
  //reduce s by 1.0 (to keep it in the range 0.0 .. 1.0) 	Principle is the same
  //but rewritten to integer mode (faster & less ASM) //Corn

  const s32 r = (frequency << 12) / output_freq;
  s32 s = 0;
  u32 in_idx = 0;
  u32 output_samples = ((num_samples * output_freq) / frequency) - 1;

  for (u32 i = output_samples; i != 0; i--) {
#ifdef DAEDALUS_ENABLE_ASSERTS
    DAEDALUS_ASSERT(in_idx + 1 < num_samples,
                    "Input index out of range - %d / %d", in_idx + 1,
                    num_samples);
#endif
    //#if 0 // 1->Sine tone, 0->Normal
    // static float c= 0.0f;
    // c += 100.0f / 44100.0f;
    // if( c >= 1.0f )
    //  c-=1.f;
    // s16 v( s16( SHRT_MAX * sinf( c * 3.141f*2 ) ) );
    // Sample	out;
    // s16 v = WriteCounter++;
    // if( WriteCounter >= MAX_COUNTER )
    // {
    // 	printf( "Loop write\n" );
    // 	WriteCounter = 0;
    // }
    // out.L = out.R = v;
    //
    // #else
    // Resample in integer mode (faster & less ASM code) //Corn
    Sample out;

    out.L = samples[in_idx].L +
            (((samples[in_idx + 1].L - samples[in_idx].L) * s) >> 12);
    out.R = samples[in_idx].R +
            (((samples[in_idx + 1].R - samples[in_idx].R) * s) >> 12);

    s += r;
    in_idx += s >> 12;
    s &= 4095;
    // #endif

    write_ptr++;
    if (write_ptr >= mBufferEnd)
      write_ptr = mBufferBegin;

    while (write_ptr == read_ptr) {
      // The buffer is full - spin until the read pointer advances.
      //    Note - spends a lot of time here if program is running
      //    fast. This loop locks the speed to the playback rate
      //    as the program winds up waiting for the buffer to empty.
      // ToDo: Adjust Audio Frequency/ Look at Turok in this regard.
      // We might want to put a Sleep in when executing on the SC?
      // Give time to other threads when using SYNC mode.
      // ThreadYield();

      read_ptr = mReadPtr;
    }

    *write_ptr = out;
  }

  // Todo: Check Cache Routines
  //  Ensure samples array is written back before mWritePtr
  // dcache_wbinv_range_unaligned( mBufferBegin, mBufferEnd );

  mWritePtr = write_ptr; // Needs cache wbinv
}

u32 CAudioBuffer::Drain(Sample *samples, u32 num_samples) {
// Todo: Check Cache Routines
//  Ideally we could just invalidate this range?
// clear the Cache
#ifdef DAEDALUS_PSP
// sceKernelDcacheWritebackInvalidateAll();
#endif

  const Sample *read_ptr(mReadPtr); // No need to invalidate, as this is uncached/volatile
  const Sample *write_ptr(mWritePtr); //

  Sample *out_ptr(samples);
  u32 samples_required(num_samples);

  while (samples_required > 0) {
    // Check if empty
    if (read_ptr == write_ptr)
      break;

    *out_ptr++ = *read_ptr++;

    if (read_ptr >= mBufferEnd)
      read_ptr = mBufferBegin;

    samples_required--;
  }

#ifdef DAEDALUS_DEBUG_AUDIO
std::ofstream fh;

 if (!fh.is_open())
 {
  fh.open("audio_out.raw",  std::ios::binary);
  fh.write(reinterpret_cast<const char*>(samples), sizeof(Sample) * num_samples - samples_required);
  fh.flush();
 }
#endif 
  mReadPtr = read_ptr; // No need to invalidate, as this is uncached
// clear the Cache
#ifdef DAEDALUS_PSP
// sceKernelDcacheWritebackInvalidateAll();
#endif
  //
  //	If there weren't enough samples, zero out the buffer
  //	FIXME(strmnnrmn): Unnecessary on OSX...
  //
  if (samples_required > 0) {
    // DBGConsole_Msg( 0, "Buffer underflow (%d samples)\n", samples_required );
    // printf( "Buffer underflow (%d samples)\n", samples_required );
    memset(out_ptr, 0, samples_required * sizeof(Sample));
  }

  // Return the number of samples written
  return num_samples - samples_required;
}
