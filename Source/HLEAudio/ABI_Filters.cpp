#include "Base/Types.h"


#include <cstring>
#include <algorithm>
#include <iostream>
#include <vector>

#include "Utility/MathUtil.h"
#include "Core/Memory.h"
#include "Debug/DBGConsole.h"
#include "HLEAudio/HLEAudioInternal.h"
#include "HLEAudio/HLEAudioState.h"

extern bool isMKABI;
extern bool isZeldaABI;

void FILTER2(AudioHLECommand command) {
#ifdef DEBUG_AUDIO
  DBGConsole_Msg(0, "FILTER2");
#endif

  static int cnt = 0;
  static s16 *lutt6;
  static s16 *lutt5;
  u8 *save = (rdram + (command.cmd1 & 0xFFFFFF));
  u8 t4 = (u8)((command.cmd0 >> 0x10) & 0xFF);

  if (t4 > 1) { // Then set the cnt variable
    cnt = (command.cmd0 & 0xFFFF);
    lutt6 = (s16 *)save;
    //				memmove (dmem+0xFE0,
    // rdram+(command.cmd1&0xFFFFFF), 0x10);
    return;
  }

  if (t4 == 0) {
    //				memmove (dmem+0xFB0,
    // rdram+(command.cmd1&0xFFFFFF), 0x20);
    lutt5 = (short *)(save + 0x10);
  }

  lutt5 = (short *)(save + 0x10);

  //			lutt5 = (short *)(dmem + 0xFC0);
  //			lutt6 = (short *)(dmem + 0xFE0);
  for (int x = 0; x < 8; x++) {
    s32 a;
    a = (lutt5[x] + lutt6[x]) >> 1;
    lutt5[x] = lutt6[x] = (short)a;
  }
  short *inp1, *inp2;
  s32 out1[8];
  s16 outbuff[0x3c0], *outp;
  u32 inPtr = (u32)(command.cmd0 & 0xffff);
  inp1 = (short *)(save);
  outp = outbuff;
  inp2 = (short *)(gAudioHLEState.Buffer + inPtr);
  for (int x = 0; x < cnt; x += 0x10) {
    out1[1] = inp1[0] * lutt6[6];
    out1[1] += inp1[3] * lutt6[7];
    out1[1] += inp1[2] * lutt6[4];
    out1[1] += inp1[5] * lutt6[5];
    out1[1] += inp1[4] * lutt6[2];
    out1[1] += inp1[7] * lutt6[3];
    out1[1] += inp1[6] * lutt6[0];
    out1[1] += inp2[1] * lutt6[1]; // 1

    out1[0] = inp1[3] * lutt6[6];
    out1[0] += inp1[2] * lutt6[7];
    out1[0] += inp1[5] * lutt6[4];
    out1[0] += inp1[4] * lutt6[5];
    out1[0] += inp1[7] * lutt6[2];
    out1[0] += inp1[6] * lutt6[3];
    out1[0] += inp2[1] * lutt6[0];
    out1[0] += inp2[0] * lutt6[1];

    out1[3] = inp1[2] * lutt6[6];
    out1[3] += inp1[5] * lutt6[7];
    out1[3] += inp1[4] * lutt6[4];
    out1[3] += inp1[7] * lutt6[5];
    out1[3] += inp1[6] * lutt6[2];
    out1[3] += inp2[1] * lutt6[3];
    out1[3] += inp2[0] * lutt6[0];
    out1[3] += inp2[3] * lutt6[1];

    out1[2] = inp1[5] * lutt6[6];
    out1[2] += inp1[4] * lutt6[7];
    out1[2] += inp1[7] * lutt6[4];
    out1[2] += inp1[6] * lutt6[5];
    out1[2] += inp2[1] * lutt6[2];
    out1[2] += inp2[0] * lutt6[3];
    out1[2] += inp2[3] * lutt6[0];
    out1[2] += inp2[2] * lutt6[1];

    out1[5] = inp1[4] * lutt6[6];
    out1[5] += inp1[7] * lutt6[7];
    out1[5] += inp1[6] * lutt6[4];
    out1[5] += inp2[1] * lutt6[5];
    out1[5] += inp2[0] * lutt6[2];
    out1[5] += inp2[3] * lutt6[3];
    out1[5] += inp2[2] * lutt6[0];
    out1[5] += inp2[5] * lutt6[1];

    out1[4] = inp1[7] * lutt6[6];
    out1[4] += inp1[6] * lutt6[7];
    out1[4] += inp2[1] * lutt6[4];
    out1[4] += inp2[0] * lutt6[5];
    out1[4] += inp2[3] * lutt6[2];
    out1[4] += inp2[2] * lutt6[3];
    out1[4] += inp2[5] * lutt6[0];
    out1[4] += inp2[4] * lutt6[1];

    out1[7] = inp1[6] * lutt6[6];
    out1[7] += inp2[1] * lutt6[7];
    out1[7] += inp2[0] * lutt6[4];
    out1[7] += inp2[3] * lutt6[5];
    out1[7] += inp2[2] * lutt6[2];
    out1[7] += inp2[5] * lutt6[3];
    out1[7] += inp2[4] * lutt6[0];
    out1[7] += inp2[7] * lutt6[1];

    out1[6] = inp2[1] * lutt6[6];
    out1[6] += inp2[0] * lutt6[7];
    out1[6] += inp2[3] * lutt6[4];
    out1[6] += inp2[2] * lutt6[5];
    out1[6] += inp2[5] * lutt6[2];
    out1[6] += inp2[4] * lutt6[3];
    out1[6] += inp2[7] * lutt6[0];
    out1[6] += inp2[6] * lutt6[1];

    // XXXX correct?
    outp[1] = /*CLAMP*/ s16((out1[1] + 0x4000) >> 0xF);
    outp[0] = /*CLAMP*/ s16((out1[0] + 0x4000) >> 0xF);
    outp[3] = /*CLAMP*/ s16((out1[3] + 0x4000) >> 0xF);
    outp[2] = /*CLAMP*/ s16((out1[2] + 0x4000) >> 0xF);
    outp[5] = /*CLAMP*/ s16((out1[5] + 0x4000) >> 0xF);
    outp[4] = /*CLAMP*/ s16((out1[4] + 0x4000) >> 0xF);
    outp[7] = /*CLAMP*/ s16((out1[7] + 0x4000) >> 0xF);
    outp[6] = /*CLAMP*/ s16((out1[6] + 0x4000) >> 0xF);
    inp1 = inp2;
    inp2 += 8;
    outp += 8;
  }
  //			memmove (rdram+(command.cmd1&0xFFFFFF), dmem+0xFB0,
  // 0x20);
  // std::copy(outbuff, gAudioHLEState.Buffer + (command.cmd0 & 0xffff) );
  // std::copy(inp2 - 8, save, 0x10);
  memmove(save, inp2 - 8, 0x10);
  memmove(gAudioHLEState.Buffer + (command.cmd0 & 0xffff), outbuff, cnt);
}

void POLEF(AudioHLECommand command )

{
// Fetch the input audio buffer
    // Assuming that command contains the address of the input buffer in cmd1
    auto* inputBuffer = reinterpret_cast<s16*>(rdram + (command.cmd1 & 0xFFFFFF));
    u32 bufferLength = command.cmd0 & 0xFFFF; // Size of the buffer

    // Coefficients for the pole filter
    // These values can be passed through the command structure or pre-defined
    constexpr float a1 = 0.5f;  // Example coefficient for 1st order
    constexpr float a2 = 0.25f; // Example coefficient for 2nd order

    // Output buffer to store filtered result
    std::vector<s16> outputBuffer(bufferLength);

    // Previous output values (initially zero)
    s16 prevY1 = 0;  // y[n-1]
    s16 prevY2 = 0;  // y[n-2]

    // Apply the pole filter to each sample
    for (u32 i = 0; i < bufferLength; ++i) {
        s16 inputSample = inputBuffer[i];

        // Recursive pole filter formula (example for a 2nd order filter)
        float currentY = inputSample + a1 * prevY1 + a2 * prevY2;

        // Clamp the result to prevent overflow (optional)
        currentY = std::clamp(currentY, static_cast<float>(INT16_MIN), static_cast<float>(INT16_MAX));

        // Store the filtered result
        outputBuffer[i] = static_cast<int16_t>(currentY);

        // Update previous values for the next iteration
        prevY2 = prevY1;
        prevY1 = static_cast<s16>(currentY);
    }

    // Store the filtered buffer back into memory
    // Replace the input buffer with the processed output
    std::memcpy(inputBuffer, outputBuffer.data(), bufferLength * sizeof(int16_t));
}
