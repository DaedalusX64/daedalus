#include "Base/Types.h"


#include <cstring>
#include <algorithm>
#include <iostream>

#include "Utility/MathUtil.h"
#include "Core/Memory.h"
#include "Debug/DBGConsole.h"
#include "HLEAudio/HLEAudioInternal.h"
#include "HLEAudio/HLEAudioState.h"

extern bool &isMKABI;
extern bool &isZeldaABI;

void FILTER2(AudioHLECommand command) {
#ifdef DEBUG_AUDIO
  DBGConsole_Msg(0, "FILTER2");
#endif

  struct ME_SHARED_ALIGN FilterState { int count = 0; s16 *lut6 = nullptr, *lut5 = nullptr; };
  static FilterState state;
  int &cnt = state.count;
  s16 *&lutt6 = state.lut6, *&lutt5 = state.lut5;
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
  const u32 addr6 = static_cast<u32>(reinterpret_cast<u8 *>(lutt6) - rdram);
  const u32 addr5 = static_cast<u32>(reinterpret_cast<u8 *>(lutt5) - rdram);
  for (int x = 0; x < 8; x++) {
    // Observe previous coefficient writes even when the two tables overlap.
    const s32 a = (s16(Audio_ReadHalf(addr5 + x * 2)) + s16(Audio_ReadHalf(addr6 + x * 2))) >> 1;
    Audio_WriteHalf(addr5 + x * 2, s16(a));
    Audio_WriteHalf(addr6 + x * 2, s16(a));
  }
  s16 lut6[8], history[8];
  Audio_ReadMemory(addr6, lut6, sizeof(lut6));
  Audio_ReadMemory(static_cast<u32>(save - rdram), history, sizeof(history));
  short *inp1, *inp2;
  s32 out1[8];
  s16 outbuff[0x3c0], *outp;
  u32 inPtr = (u32)(command.cmd0 & 0xffff);
  inp1 = history;
  outp = outbuff;
  inp2 = (short *)(gAudioHLEState.Buffer + inPtr);
  for (int x = 0; x < cnt; x += 0x10) {
    out1[1] = inp1[0] * lut6[6];
    out1[1] += inp1[3] * lut6[7];
    out1[1] += inp1[2] * lut6[4];
    out1[1] += inp1[5] * lut6[5];
    out1[1] += inp1[4] * lut6[2];
    out1[1] += inp1[7] * lut6[3];
    out1[1] += inp1[6] * lut6[0];
    out1[1] += inp2[1] * lut6[1]; // 1

    out1[0] = inp1[3] * lut6[6];
    out1[0] += inp1[2] * lut6[7];
    out1[0] += inp1[5] * lut6[4];
    out1[0] += inp1[4] * lut6[5];
    out1[0] += inp1[7] * lut6[2];
    out1[0] += inp1[6] * lut6[3];
    out1[0] += inp2[1] * lut6[0];
    out1[0] += inp2[0] * lut6[1];

    out1[3] = inp1[2] * lut6[6];
    out1[3] += inp1[5] * lut6[7];
    out1[3] += inp1[4] * lut6[4];
    out1[3] += inp1[7] * lut6[5];
    out1[3] += inp1[6] * lut6[2];
    out1[3] += inp2[1] * lut6[3];
    out1[3] += inp2[0] * lut6[0];
    out1[3] += inp2[3] * lut6[1];

    out1[2] = inp1[5] * lut6[6];
    out1[2] += inp1[4] * lut6[7];
    out1[2] += inp1[7] * lut6[4];
    out1[2] += inp1[6] * lut6[5];
    out1[2] += inp2[1] * lut6[2];
    out1[2] += inp2[0] * lut6[3];
    out1[2] += inp2[3] * lut6[0];
    out1[2] += inp2[2] * lut6[1];

    out1[5] = inp1[4] * lut6[6];
    out1[5] += inp1[7] * lut6[7];
    out1[5] += inp1[6] * lut6[4];
    out1[5] += inp2[1] * lut6[5];
    out1[5] += inp2[0] * lut6[2];
    out1[5] += inp2[3] * lut6[3];
    out1[5] += inp2[2] * lut6[0];
    out1[5] += inp2[5] * lut6[1];

    out1[4] = inp1[7] * lut6[6];
    out1[4] += inp1[6] * lut6[7];
    out1[4] += inp2[1] * lut6[4];
    out1[4] += inp2[0] * lut6[5];
    out1[4] += inp2[3] * lut6[2];
    out1[4] += inp2[2] * lut6[3];
    out1[4] += inp2[5] * lut6[0];
    out1[4] += inp2[4] * lut6[1];

    out1[7] = inp1[6] * lut6[6];
    out1[7] += inp2[1] * lut6[7];
    out1[7] += inp2[0] * lut6[4];
    out1[7] += inp2[3] * lut6[5];
    out1[7] += inp2[2] * lut6[2];
    out1[7] += inp2[5] * lut6[3];
    out1[7] += inp2[4] * lut6[0];
    out1[7] += inp2[7] * lut6[1];

    out1[6] = inp2[1] * lut6[6];
    out1[6] += inp2[0] * lut6[7];
    out1[6] += inp2[3] * lut6[4];
    out1[6] += inp2[2] * lut6[5];
    out1[6] += inp2[5] * lut6[2];
    out1[6] += inp2[4] * lut6[3];
    out1[6] += inp2[7] * lut6[0];
    out1[6] += inp2[6] * lut6[1];

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
  Audio_WriteMemory(static_cast<u32>(save - rdram), inp2 - 8, 0x10);
  memmove(gAudioHLEState.Buffer + (command.cmd0 & 0xffff), outbuff, cnt);
}

void POLEF(AudioHLECommand command)
{
    const u32 address = command.cmd1 & 0xFFFFFF;
    const u32 length = command.cmd0 & 0xFFFF;
    s16 prevY1 = 0, prevY2 = 0;
    s16 block[128];
    for (u32 start = 0; start < length; start += 128)
    {
        const u32 count = std::min<u32>(128, length - start);
        Audio_ReadMemory(address + start * sizeof(s16), block, count * sizeof(s16));
        for (u32 i = 0; i < count; ++i)
        {
            float current = block[i] + 0.5f * prevY1 + 0.25f * prevY2;
            current = std::max(-32768.0f, std::min(32767.0f, current));
            block[i] = static_cast<s16>(current);
            prevY2 = prevY1;
            prevY1 = block[i];
        }
        Audio_WriteMemory(address + start * sizeof(s16), block, count * sizeof(s16));
    }
}
