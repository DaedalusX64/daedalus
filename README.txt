 ___________       __         ____           ____               __     
/\D\D\D\D\D\\Daeda/\X\lusX64D/\X\/aedalusX64/\6\6\DaedalusX64Da/\4\aedalusX64Da
\/\\\////////\D\Da\///\X\eda/\X\/lusX64Da/\/6\6///edalusX64Dae/4\\\\dalusX64Dae
D\/\D\aedalu\//\\\sX6\///\X\X\\/4Daedalu/\6\/6/sX64DaedalusX6/4\\/\4\4DaedalusX
Da\/\\\edalusX\/\D\64Dae\//\\X\dalusX64/\6\6\6\6\6\DaedalusX/4\//\/\\\64Daedalu
Dae\/\D\dalusX6\/\\\4Daeda\/\\X\lusX64D/\6\\///////\6\aedal/4\\/us\/\4\X64Daeda
Daed\/\\\alusX64\/\D\Daedal/\X\X\\usX64\/\6\Daedal\/\6\usX/\4\4\4\4\4\\4\4\64Da
Daeda\/\D\dalusX6/\\\4Daed/\X\////\X\alu\//6\sX64Da\/\6\ed\/ / / / / /\4\ /alus
Daedal\/\\\\\\\\\\\\/usX6/\X\/4Da\///\X\ed\/6\6\6\6\6\6/alusX64Daedal\/\\\usX64
Daedalu\//D/D/D/D/D/sX64D\/X/aedalus\/X/X64\/666666666/DaedalusX64Daed\/4/alusX
        '''''''''''       ''         ''     ''''''''''                 ''  
***************
*   Linux     *
***************

To make DaedalusX64 for the PSP in linux:

First you need to make the mediaengine.prx. 

Doing so will also make a file required by the
emulator. 

You must make this before making the emulator or you'll get a file not found error. 

cd Source/SysPSP/MediaEnginePRX make

Now just cd back to the root directory and make the emulator.

cd ../../..make

Linux Readme By:ChillyWillyGuru

***************
*Windows 32Bit*
***************

Compiling DaedalusX64 for PSP on Windows

-----------------Before Compiling-----------------
Make sure you have:

1.PSP DevKit (Latest) (MinPSPw may be used)
2.Visual C++ 2008 Express Edition*
3.Latest SVN Source code

*NOTE: 

	On Windows; installing MinPSPw requires you to Open Visual C++ 08 and build Dx64 under the
	SAME account that you installed MinPSPw, if not, the build will fail! It has something to
	do with the "enviornment variable setup that MinPSPw makes for that certain account.
	(This does not affect anyone who has only 1 account, only for multi account users.)
	
	(If you have any ADMINISTRATIVE problems under Windows Vista when using Visual 
            C++ 2008 Express Edition; make sure:
	
	-a. You're an Administrator
	-b. Right-Click on Visual C++ 2008 and select "Run as Administrator" 
	(Some Programs needs these rights manually elevated--Don't ask Why -.- )		
	
-----------------When Compiling-----------------

1. Browse for your DaedalusX64 SourceCode
2. Open the DaedalusX64.sln
3. Once its opened; Right-Click on "DaedalusPSP" and select "Set as Start-up Project"
4. Hit the Green Arrow and it will begin to build, or right click on DaedalusPSP and select: Build.
5.It take a bit of time to build and you should get an error message saying that it cannot run EBOOT.PBP
6. Congratulations! You've just Compiled The Best N64 Emulator that is Available for the PSP!**

**NOTE: IF for any reason you're ingenious enough to delete any prx's just build them in C++ by 
rightclicking them and selecting build. If for any reason you cannot compile this eboot and you're 
CERTAIN that you have the latest sdk,devkits, and admin rights, the make sure you didn't accidentally
modify any of the source-code, in this case; delete the entire folder containing DaedalusX64 and Download
the revision once again.

Thanks a TON and I hope I was of some help to many DaedalusX64 community members, Much Love--Shinydude100

Windows XP/Vista Readme by: Shinydude100/Kreationz