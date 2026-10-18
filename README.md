# DaedalusX64
 
DaedalusX64 is a Nintendo 64 emulator for PSP, 3DS, Vita, Linux, macOS and Windows
 
## Features:
 
- Fast emulation
- High compatibility
- Support for PSP / 3DS / macOS / Linux / Windows
- Active support and updates

## Things that don't work right now:
- Big Endian (So no builds for PowerPC at the moment sorry).
 
## Usage
 
Installing Daedalus:
Download the latest release for your platform from https://github.com/DaedalusX64/daedalus/releases or if you dare, use the latest builds in the GitHub Action section

Extra Steps:
PSP: Copy the entire DaedalusX64 Folder to your Memory Stick and put into PSP/Game

macOS / Linux:
 Prerequisites: libpng / zlib / minizip / glew / SDL2 (You'll need to do this via a package manager.
 Execute the emulator ./daedalus 'Path to Rom' 
Note - Native Apple Silicon build via GitHub is not available yet however it compiles fine and works (Just a bit slow as expected)
## Building
All Versions require these libraries: zlib, libpng, minizip
 ## Building for PSP
    Fetch the latest PSP Toolchain from https://github.com/pspdev/pspdev and install required libraries

Install [me-core](https://github.com/mcidclan/psp-media-engine-custom-core) into your PSP toolchain first (the revision below is also used by CI):

```sh
git clone https://github.com/mcidclan/psp-media-engine-custom-core.git /tmp/me-core
git -C /tmp/me-core checkout f153f0de12d42377b4f7ecec7e328dd2f0e825bb
psp-cmake -S /tmp/me-core -B /tmp/me-core-build
cmake --build /tmp/me-core-build -j2
cmake --install /tmp/me-core-build
```

    Run    ./build_daedalus.sh PSP and CMake will do the required fun stuff :)

PSP release builds use `DAEDALUS_PSP_USE_ME=ON` to process N64 display-list commands, RDP state and scalar vertex transforms on ME. ME writes the shared GU lists through their cached alias and flushes them at every CPU handoff; the main CPU submits them to GE, waits before reusing their memory, and handles texture allocation/uploads, VRAM texture merging and new combiner allocation. Graphics submission returns immediately so N64 CPU emulation overlaps ME processing. An emulated CPU event polls completion and services the ME mailbox; SP/DP completion is deferred until ME releases the task. The single renderer, list presentation, save states, emulation stop, and ROM shutdown are fenced before their state is reused. With `DAEDALUS_PSP_ME_AUDIO=ON` (default), audio HLE, PCM resampling and direct hardware FIFO playback run on ME. Async audio returns immediately and signals SP completion after publishing its DSP output; sync audio explicitly waits. Configure `-DDAEDALUS_PSP_ME_AUDIO=OFF` to retain main-CPU audio.

me-core embeds its kernel bridge and extracts `kcall.prx` beside the EBOOT at runtime; `Plugins/mediaengine.prx` is no longer needed. A failed ME startup falls back to CPU graphics. Display-list debugging, logging, assertions and profiling builds also use CPU graphics because their command handlers require OS services. Configure with `-DDAEDALUS_PSP_USE_ME=OFF` to build without me-core and retain the VFPU graphics path.

Run `python3 Tools/tests/check_me_graphics.py build/Source/daedalus` to audit the linked ME renderer for VFPU instructions (also checked by CI). Real PSP testing must cover rendered games, audio, both display-list buffering modes, dense lists that exceed the list buffer, closing/reopening ROMs, and suspend/resume.

The CPU/ME handoff explicitly transfers libgu's write cursor along with each completed task and texture service, switches both start/cursor pointers between the ME cached alias and main-CPU uncached alias, invalidates idle cache entries at each handoff, and isolates CPU-only flags on a full cache line. Texture-cache hits and source hash checks stay on ME; allocation and changed-texture uploads use the main CPU. Short handoffs briefly poll the mailbox before yielding. ME faults report Cause/EPC/BadVAddr; stalled ME tasks fall back to CPU graphics. Use `-DDAEDALUS_PSP_ME_DIAGNOSTICS=ON` to trace the first draw and last display-list command and enable the GE timeout watchdog. Normal builds use the kernel's GE completion event to avoid per-frame polling latency.

ME-writable renderer/cache/texture objects and shared command buffers occupy complete 64-byte cache lines, including aligned shared-pointer allocations. Graphics commands that modify N64 RAM run through the main CPU so they cannot write back stale ME cache lines over CPU writes. Games retain ownership of submitted task inputs until their deferred SP/DP completion; this is asynchronous HLE scheduling, not a cycle-accurate RSP/RDP pipeline. Scalar vertex work can still cost more than the main-CPU VFPU path. Graphics and audio tasks share one ME worker, matching the single emulated RSP; they do not execute simultaneously.

Use `-DDAEDALUS_PSP_ME_TIMING=ON` for a performance capture with ME still active and command tracing off. It reports average total task time, main-CPU service time (including cache handoffs), service calls, and time blocked at explicit fences every 300 completed tasks. These timers run only on the main CPU; task time includes ME work and scheduling delays.

## Building for Windows 

NB: Windows build is broken at the moment 

1) Clone and open the repo in Visual studio 2019

2) Build All

## Building for Posix OS (Linux & macOS)
Posix OSes additionally require glew / SDL2

1) Clone this repo 

2) ./build_daedalus.sh (This will auto detect the OS you are on)
 
 ## CI
 DaedalusX64 now has CI, you can get the latest nightlies from the Actions Tab.
 Warning: these builds are sporatic at times.

## More Info
 
For information about compatibility, optimal settings and more about the emulator, visit the actively maintained GitHub wiki page: https://github.com/DaedalusX64/daedalus/wiki Feel free to submit reports for how well your favourite games run if they have not already been listed!
 
We're on Discord, come catch up with the latest news. Chaos ensured. https://discord.gg/FrVTpBV
 
## Credits
- StrmnNrmn - For bringing this project to us
- Kreationz, Howard0Su salvy, Corn, Chilly Willy, Azimer, Shinydude100 and missed folks for past contributions
- MasterFeizz for 3DS Support / ARM DynaRec
- Xerpi / Rinnegatamante / TheOfficialFloW for PS Vita and ARM Contributions
- z2442 & Wally: Compilation improvements and updating, optimizations
- TheMrIron2 & mrneo240 Optimizations, wiki maintenance
- re4thewin for continuous testing over the duration of DaedalusX64
- motolegacy for help with site and emulator itself
- fjtrujy for help with getting CI going.
- Our Valued Communities for helping making Daedalus what it is :)

### ME audio output

The FIFO setup is adapted from mcidclan’s [audio-local sample](https://github.com/mcidclan/psp-media-engine-custom-core/tree/main/samples/audio-local); its MIT notice is retained in `Source/SysPSP/ME/audio-local-LICENSE.txt`. SC performs kernel clock setup and publishes copied AI PCM packets. ME resamples and feeds the audio FIFO directly, with no SC playback thread in this mode. A timer interrupt preserves GPRs, FPU registers, FCSR, HI/LO and the branch EPC while feeding audio during graphics tasks; other interrupts continue through me-core’s suspend handler.

Audio DSP memory writes are staged in a bounded journal. Later commands read earlier staged writes, and SP completion publishes only the changed RDRAM bytes on SC. An oversized transaction can request an SC publication before continuing. `ME audio timing:` logs report task latency with `DAEDALUS_PSP_ME_TIMING=ON`. Validate sound, stereo/pitch, menus, ROM switching and suspend/resume on hardware.

Run `g++ -std=c++23 -O2 -pthread -ISource Tools/tests/test_me_audio.cpp -o /tmp/test_me_audio && /tmp/test_me_audio` for RAM-publication, resampling and concurrent PCM-packet regression tests.
