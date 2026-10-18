#!/usr/bin/env python3
"""Audit a linked PSP ME build for renderer instructions the ME cannot execute.

This checks generated code (including LTO inlining), not just source macros.
It supplements real PSP testing; cache coherency and GE timing require hardware.
"""

import argparse
import re
import subprocess
import sys


def audit(disassembly, audio=False):
    functions = re.split(r"(?m)^[0-9a-f]+ <(.+)>:\n", disassembly)
    checked = 0
    errors = []
    has_worker = False
    has_submit = False
    has_completion = False
    audio_entries = set()
    prefixes = ("BaseRenderer::", "RendererPSP::", "DLParser_", "RDP_", "c32::", "meLibOnProcess(",
                "CTextureCache::GetOrCreateCachedTexture(", "CachedTexture::IsFresh(",
                "CachedTexture::UpdateIfNecessary(", "CachedTexture::UpdateTextureHash(",
                "TextureInfo::GenerateHashValue(")
    helpers = {"memcpy", "memmove", "memcmp", "memset", "pspFpuAbs", "pspFpuRound", "pspFpuSqrt", "pspFpuTrunc"}
    audio_handlers = set("ADDMIXER ADPCM ADPCM2 ADPCM3 CLEARBUFF CLEARBUFF2 CLEARBUFF3 DEINTERLEAVE2 DMEMMOVE DMEMMOVE2 DMEMMOVE3 DUPLICATE2 ENVMIXER ENVMIXER2 ENVMIXER3 ENVMIXER_GE ENVSETUP1 ENVSETUP2 FILTER2 HILOGAIN INTERLEAVE INTERLEAVE2 INTERLEAVE3 LOADADPCM LOADADPCM2 LOADADPCM3 LOADBUFF LOADBUFF2 LOADBUFF3 MIXER MIXER2 MIXER3 MP3 MP3ADDY POLEF RESAMPLE RESAMPLE2 RESAMPLE3 SAVEBUFF SAVEBUFF2 SAVEBUFF3 SEGMENT SEGMENT2 SETBUFF SETBUFF2 SETLOOP SETLOOP2 SETLOOP3 SETVOL SETVOL3 SPNOOP UNKNOWN".split())
    vfpu = re.compile(r"(?m)^\s+[0-9a-f]+:\s+[0-9a-f]+\s+((?:v\w+|[ls]v\w*\.[sptq]|[ms][ft]v\w*|[mc][ft]c2|[ls]wc2|cop2)\b[^\n]*)")
    for index in range(1, len(functions), 2):
        name, body = functions[index:index + 2]
        has_worker |= name.startswith("meLibOnProcess(")
        has_completion |= name.startswith("DLParser_CompleteME(")
        if name in {"MEAudio_Interrupt", "MEAudio_ExceptionEntry"}:
            audio_entries.add(name)
        if audio and name == "MEAudio_ExceptionEntry":
            # Timer callbacks can interrupt any graphics/DSP instruction.
            # Check the linked handler, including assembler register aliases.
            for reg in ("at v0 v1 a0 a1 a2 a3 t0 t1 t2 t3 t4 t5 t6 t7 s0 s1 s2 s3 s4 s5 s6 s7 t8 t9 k0 k1 gp s8 ra".split()):
                saved = re.search(rf"\bsw\s+{reg},(-?\d+)\(sp\)", body)
                restored = re.findall(rf"\blw\s+{reg},(-?\d+)\(sp\)", body)
                if not saved or saved[1] not in restored:
                    errors.append(f"ME timer handler does not preserve {reg}")
            for reg in range(32):
                saved = re.search(rf"\bswc1\s+\$f{reg},(\d+)\(sp\)", body)
                restored = re.search(rf"\blwc1\s+\$f{reg},(\d+)\(sp\)", body)
                if not saved or not restored or saved[1] != restored[1]:
                    errors.append(f"ME timer handler does not preserve f{reg}")
            for instruction in ("mfhi", "mthi", "mflo", "mtlo", "cfc1", "ctc1", "eret"):
                if not re.search(rf"\b{instruction}\b", body):
                    errors.append(f"ME timer handler missing {instruction}")
            if re.search(r"\bmtc0\s+[^\n]*\bc0_epc\b", body):
                errors.append("ME timer handler changes the interrupted branch EPC")
        if name.startswith("MEGraphics_Submit("):
            has_submit = True
            calls = re.findall(r"(?m)^\s+[0-9a-f]+:\s+[0-9a-f]+\s+jal(?:r)?\s+[^\n]*", body)
            published = next((i for i, call in enumerate(calls) if "sceKernelDcacheWritebackInvalidateAll" in call), None)
            if published is None or any("Wait" in call or "DelayThread" in call for call in calls[published + 1:]):
                errors.append("ME submission waits after publishing the task")
        if name.startswith("meLibOnProcess("):
            calls = re.findall(r"(?m)^\s+([0-9a-f]+):\s+[0-9a-f]+\s+(jal(?:r)?\s+[^\n]*)", body)
            indirect = next((i for i, (_, call) in enumerate(calls) if call.startswith("jalr")), None)
            if (indirect is None or indirect == 0
                    or "meLibDcacheWritebackInvalidateAll" not in calls[indirect - 1][1]
                    or int(calls[indirect][0], 16) - int(calls[indirect - 1][0], 16) > 128):
                errors.append("ME task entry does not refresh cache after idle polling")
        audio_function = audio and (name.split("(")[0] in audio_handlers or name.startswith((
            "AudioHLEState::", "Audio_Ucode(", "Audio_ReadMemory(", "Audio_WriteMemory(",
            "Audio_ReadHalf(", "Audio_WriteHalf(", "AudioPacketStream<", "AudioMemoryJournal<",
            "(anonymous namespace)::CMP3Decode::", "(anonymous namespace)::ProcessAudio(",
            "MEAudio_Interrupt", "MEAudio_ExceptionEntry")) or name == "memcpy_swizzle")
        if name.startswith(prefixes) or name in helpers or audio_function:
            checked += 1
            for instruction in vfpu.findall(body):
                errors.append(f"{name}: {instruction}")
        if name in {"InitME", "BeginME", "CallME", "WaitME", "KillME", "CheckME", "SignalME"}:
            errors.append(f"Legacy ME wrapper still linked: {name}")
    if not has_worker:
        errors.append("me-core graphics worker missing; audit an ME-enabled release build")
    if not has_submit or not has_completion:
        errors.append("Asynchronous ME submission/completion path missing")
    if audio and audio_entries != {"MEAudio_Interrupt", "MEAudio_ExceptionEntry"}:
        errors.append("Direct ME audio FIFO/timer entry missing")
    return checked, errors


def audit_layout(symbols, audio=False):
    errors = []
    found_main = False
    found_audio = False
    for line in symbols.splitlines():
        fields = line.split(None, 3)
        if len(fields) != 4:
            continue
        if "::sMain" in fields[3] or "L5sMainE" in fields[3]:
            found_main = True
            address, size = int(fields[0], 16), int(fields[1], 16)
            if address % 64 or size != 64:
                errors.append("SC-only ME flags are not isolated on a complete 64-byte cache line")
        if audio and (fields[3] in {"gAudioHLEState", "audioABI", "audioFlags", "envelope", "MEAudio_PreviousExceptionVector"}
                or "L7sWritesE" in fields[3] or "L7sStreamE" in fields[3] or fields[3].endswith("::sCursor")):
            address, size = int(fields[0], 16), int(fields[1], 16)
            found_audio |= fields[3] == "gAudioHLEState"
            if address % 64 or size % 64:
                errors.append(f"Audio state shares a cache line: {fields[3]}")
    if not found_main:
        errors.append("SC-only ME state missing from the linked ELF")
    if audio and not found_audio:
        errors.append("Aligned ME audio DSP state missing")
    return errors


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("elf", help="Linked ELF, e.g. build/Source/daedalus")
    parser.add_argument("--objdump", default="psp-objdump")
    parser.add_argument("--nm", help="Defaults to nm beside the selected objdump")
    parser.add_argument("--audio", action="store_true", help="Also audit ME DSP code and FIFO interrupt preservation")
    args = parser.parse_args()
    disassembly = subprocess.check_output([args.objdump, "-d", "-C", args.elf], text=True)
    checked, errors = audit(disassembly, args.audio)
    nm = args.nm or args.objdump.removesuffix("objdump") + "nm"
    symbols = subprocess.check_output([nm, "-S", "-C", args.elf], text=True)
    errors.extend(audit_layout(symbols, args.audio))
    if errors:
        print("\n".join(errors), file=sys.stderr)
        return 1
    print(f"ME audit passed: {checked} functions, no VFPU/legacy wrappers; cache handoffs and state isolation verified" + ("; audio timer registers preserved." if args.audio else "."))
    return 0


if __name__ == "__main__":
    sys.exit(main())
