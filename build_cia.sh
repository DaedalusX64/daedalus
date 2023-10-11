
	# APP_TITLE "DaedalusX64"
	# APP_DESCRIPTION "DaedalusX64 port for 3DS"
	# APP_AUTHOR "MasterFeizz / DaedalusX64 Team"

	# APP_SMDH "${PROJECT_SOURCE_DIR}/SysCTR/Resources/daedalus.smdh"
	# APP_BANNER "${PROJECT_SOURCE_DIR}/SysCTR/Resources/banner.bnr"
	# APP_AUDIO "${PROJECT_SOURCE_DIR}/SysCTR/Resources/audio_silent.wav"
	# APP_RSF "${PROJECT_SOURCE_DIR}/SysCTR/Resources/template.rsf"
	# APP_ROMFS_DIR "${PROJECT_SOURCE_DIR}/SysCTR/Resources/RomFS"
	# APP_ROMFS_BIN "${PROJECT_SOURCE_DIR}/SysCTR/Resources/romfs.bin"
    cp build/Source/daedalus.elf .
    3dsxtool daedalus.elf DaedalusX64.3dsx --romfs="$PWD/Source/SysCTR/Resources/RomFS" --smdh="$PWD/Source/SysCTR/Resources/daedalus.smdh"  
    makerom -f cia -target t -exefslogo -o Daedalus.cia -elf daedalus.elf -rsf $PWD/Source/SysCTR/Resources/template.rsf -banner $PWD/Source/SysCTR/Resources/banner.bnr -icon $PWD/Source/SysCTR/Resources/daedalus.smdh -romfs $PWD/Source/SysCTR/Resources/romfs.bin