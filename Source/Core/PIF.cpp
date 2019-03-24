/*
Copyright (C) 2001 StrmnNrmn

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

// Check patents: U.S. Pat. No. 6,394,905
//                U.S. Pat. No. 4,799,635
//                U.S. Pat. No. 5,426,762


/*
In the exemplary embodiment, if processor 100 writes "0x00", "0xFD", "0xFE" or "0xFF" as TxData Size,
the data is not recognized as TxData size but has a special function as indicated below. They become
effective when processor 100 sets format bit (0x1FC007FC b0) by using Wr64B or Wr4B.

"0x00"=Channel Skip

If 0x00 is written as TxData Size, respective JoyChannel transaction is not executed.

"0xFD"=Channel Reset

If 0xFD is written as TxData Size, PIF outputs reset signal to respective JoyChannel.

"0xFE"=Format End

If 0xFE is written as TxData Size, TxData/RxData assignment is end at this ")xFE". In other words,
the TxData Size or RxData Size after "0xFE" is ignored.

"0xFF"=Dummy Data

TxData Size's 0xFF is used as the dummy data for word aligning the data area.

Each Channel has four flags. Two of them have information from processor 100 to JoyChannel and
others from JoyChannel to processor 100.

Skip=Channel Skip

If processor 100 sets this flag to "1", respective JoyChannel transaction is not executed. This
flag becomes effective without formal flag.

Res=Channel Reset

If 64 bit CPU set this flag to "1", PIF outputs reset signal to respective JoyChannel. This flag
becomes effective without format flag.

NR=No Response to JoyChannel

When each JoyChannel's peripheral device does not respond, the respective NR bit is set to "1".
This is the way to detect the number of currently connected peripheral devices.

Err=JoyChannel Error

When communication error has occurred between PIF and peripheral device, Err flag is set to "1".

If the 64 bit CPU 100 wants to change JoyChannel's Tx/RxData assignment, a 32 bit format flag is
used, where a certain bit(s) specify the desired format. For example, when Wr64B or Wr4B is
issued when this flag is "1", PIF executes each JoyChannel's Tx/RxData assignment based on each
channel's Tx/Rx Size. In other words, unless this flag is set to "1" with Wr64B or Wr4B, Tx/RxData
area assignment does not change. After Tx/RxData assignment, this flag is reset to "0" automatically.
*/

// Typical command for a channel:

// TxDataSize: 0x01
// RxDataSize: 0x03
// TxData:     0x00 : Command: GetStatus
// RxData0:
// RxData1:
// RxData2:


// Stuff to handle controllers

#include "stdafx.h"

#include "PIF.h"
#include "CPU.h"
#include "Memory.h"
#include "ROM.h"
#include "Save.h"

#include "Math/MathUtil.h"
#include "Utility/Preferences.h"

#include "Debug/DBGConsole.h"
#include "Input/InputManager.h"

#include "OSHLE/ultra_os.h"

#include <time.h>

#ifdef _MSC_VER
#pragma warning(default : 4002)
#endif

#ifdef DAEDALUS_DEBUG_PIF
	#define DPF_PIF( ... )		{ if ( mDebugFile ) { fprintf( mDebugFile, __VA_ARGS__ ); fprintf( mDebugFile, "\n" ); } }
#else
	#define DPF_PIF( ... )
#endif

bool gRumblePakActive {false};


//

class	IController : public CController
{
	public:
		IController();
		~IController();

		//
		// CController implementation
		//
		bool			OnRomOpen();
		void			OnRomClose();

		void			Process();

	private:
		//
		//
		//
		bool			ProcessController(u8 *cmd, u32 device);
		bool			ProcessEeprom(u8 *cmd);

		void			CommandReadEeprom(u8 *cmd);
		void			CommandWriteEeprom(u8* cmd);
		void			CommandReadMemPack(u32 channel, u8 *cmd);
		void			CommandWriteMemPack(u32 channel, u8 *cmd);
		void			CommandReadRumblePack(u8 *cmd);
		void			CommandWriteRumblePack(u8 *cmd);
		void			CommandReadRTC(u8 *cmd);

		u8				CalculateDataCrc(const u8 * pBuf) DAEDALUS_ATTRIBUTE_CONST;
		bool			IsEepromPresent() const						{ return mpEepromData != NULL; }

		void			n64_cic_nus_6105();
		u8				Byte2Bcd(s32 n)								{ n %= 100; return ((n / 10) << 4) | (n % 10); }


#ifdef DAEDALUS_DEBUG_PIF
		void			DumpInput() const;
#endif

	private:

		enum
		{
			CONT_GET_STATUS      = 0x00,
			CONT_READ_CONTROLLER = 0x01,
			CONT_READ_MEMPACK    = 0x02,
			CONT_WRITE_MEMPACK   = 0x03,
			CONT_READ_EEPROM     = 0x04,
			CONT_WRITE_EEPROM    = 0x05,
			CONT_RTC_STATUS		 = 0x06,
			CONT_RTC_READ		 = 0x07,
			CONT_RTC_WRITE		 = 0x08,
			CONT_RESET           = 0xff
		};

		enum
		{
			CONT_TX_SIZE_CHANSKIP   = 0x00,					// Channel Skip
			CONT_TX_SIZE_DUMMYDATA  = 0xFF,					// Dummy Data
			CONT_TX_SIZE_FORMAT_END = 0xFE,					// Format End
			CONT_TX_SIZE_CHANRESET  = 0xFD,					// Channel Reset
		};

		u8 *			mpPifRam;

#ifdef DAEDALUS_DEBUG_PIF
		u8				mpInput[ 64 ];
#endif

		enum EPIFChannels
		{
			PC_CONTROLLER_0 = 0,
			PC_CONTROLLER_1,
			PC_CONTROLLER_2,
			PC_CONTROLLER_3,
			PC_EEPROM,
			PC_UNKNOWN_1,
			NUM_CHANNELS,
		};

		// Please update the memory allocated for mempack if we change this value
		static const u32 NUM_CONTROLLERS {4};

		OSContPad		mContPads[ NUM_CONTROLLERS ];
		bool			mContPresent[ NUM_CONTROLLERS ];
		bool			mContMemPackPresent[ NUM_CONTROLLERS ];

		u8 *			mpEepromData;
		u8				mEepromContType;					// 0, CONT_EEPROM or CONT_EEP16K

		u8 *			mMemPack[ NUM_CONTROLLERS ];

#ifdef DAEDALUS_DEBUG_PIF
		FILE *			mDebugFile;
#endif

};



// Singleton creator

template<> bool	CSingleton< CController >::Create()
{
	DAEDALUS_ASSERT_Q(mpInstance == NULL);

	mpInstance = new IController();

	return true;
}


// Constructor

IController::IController() :
	mpPifRam( NULL ),
	mpEepromData( NULL )
{
#ifdef DAEDALUS_DEBUG_PIF
	mDebugFile = fopen( "controller.txt", "w" );
#endif
	for ( u32 i {}; i < NUM_CONTROLLERS; i++ )
	{
		mContPresent[ i ] = false;
		mContMemPackPresent[ i ] = false;
	}

	mContMemPackPresent[ 0 ] = true;
	// Only one controller is enabled, this has to be revised once mltiplayer is introduced
	mContPresent[ 0 ] = true;
}


// Destructor

IController::~IController()
{
#ifdef DAEDALUS_DEBUG_PIF
	if( mDebugFile != NULL )
	{
		fclose( mDebugFile );
	}
#endif
}


// Called whenever a new rom is opened

bool IController::OnRomOpen()
{
	ESaveType save_type  = g_ROM.settings.SaveType;
	mpPifRam = (u8 *)g_pMemoryBuffers[MEM_PIF_RAM];

	//gRumblePakActive = false;

	if ( mpEepromData )
	{
		mpEepromData = NULL;
	}

	if ( save_type == SAVE_TYPE_EEP4K )
	{
		mpEepromData = (u8*)g_pMemoryBuffers[MEM_SAVE];
		mEepromContType = 0x80;
		#ifdef DAEDALUS_DEBUG_CONSOLE
		DBGConsole_Msg( 0, "Initialising EEPROM to [M%d] bytes", 4096/8 );	// 4k bits
		#endif
	}
	else if ( save_type == SAVE_TYPE_EEP16K )
	{

		mpEepromData = (u8*)g_pMemoryBuffers[MEM_SAVE];
		mEepromContType = 0xC0;
		#ifdef DAEDALUS_DEBUG_CONSOLE
		DBGConsole_Msg( 0, "Initialising EEPROM to [M%d] bytes", 16384/8 );	// 16 kbits
		#endif
	}
	else
	{
		mpEepromData = NULL;
		mEepromContType = 0x00;
	}


	for ( u32 channel {}; channel < NUM_CONTROLLERS; channel++ )
	{
		mMemPack[channel] = (u8*)g_pMemoryBuffers[MEM_MEMPACK] + channel * 0x400 * 32;
	}

	return true;
}

void IController::OnRomClose() {}

void IController::Process()
{
#ifdef DAEDALUS_DEBUG_PIF
	memcpy(mpInput, mpPifRam, 64);
	DPF_PIF("");
	DPF_PIF("");
	DPF_PIF("*********************************************");
	DPF_PIF("**                                         **");
#endif

	u32 count {}, channel {};
	u32 *tmp {(u32*)mpPifRam};
	if ((tmp[0] == 0xFFFFFFFF) &&
		(tmp[1] == 0xFFFFFFFF) &&
		(tmp[2] == 0xFFFFFFFF) &&
		(tmp[3] == 0xFFFFFFFF))
	{
		#ifdef DAEDALUS_DEBUG_CONSOLE
		DBGConsole_Msg(0, "[YDecrypting PifRam]");
		#endif
		n64_cic_nus_6105();
		return;
	}

	// Read controller data here (here gets called fewer times than CONT_READ_CONTROLLER)
	CInputManager::Get()->GetState( mContPads );

	while(count < 64)
	{
		u8 *cmd {&mpPifRam[count]};

		// command is ready
		if(cmd[0] == CONT_TX_SIZE_FORMAT_END)
		{
			break;
		}

		// dummy data..
		if((cmd[0] == CONT_TX_SIZE_DUMMYDATA) || (cmd[0] == CONT_TX_SIZE_CHANRESET))
		{
			count++;
			continue;
		}

		#ifdef DAEDALUS_ENABLE_ASSERTS
		DAEDALUS_ASSERT( (cmd[0] !=  0xB4) || (cmd[0] != 0x56) || (cmd[0] != 0xB8), "PIF : NOP command? %02x", cmd[0] );
		#endif
		// next channel
		if(cmd[0] == CONT_TX_SIZE_CHANSKIP)
		{
			count++;
			channel++;
			continue;
		}

		// 0-3 = controller channel
		if( channel < PC_EEPROM )
		{
			if ( !ProcessController(cmd, channel) )
				break;
		}
		// 4 = eeprom channel
		else if( channel == PC_EEPROM)
		{
			if ( !ProcessEeprom(cmd) )
				break;
		}
		#ifdef DAEDALUS_DEBUG_CONSOLE
		else
		{
			DAEDALUS_ERROR( "Trying to write from invalid controller channel! %d", channel );
			break;
		}
		#endif

		channel++;
		count += cmd[0] + (cmd[1] & 0x3f) + 2;

	}

	mpPifRam[63] = 0;	// Set the last bit is 0 as successfully return

#ifdef DAEDALUS_DEBUG_PIF
	DPF_PIF("Before | After:");

	for ( u32 x {}; x < 64; x+=8 )
	{
		DPF_PIF( "0x%02x%02x%02x%02x : 0x%02x%02x%02x%02x  |  0x%02x%02x%02x%02x : 0x%02x%02x%02x%02x",
			mpInput[(x + 0)],  mpInput[(x + 1)],  mpInput[(x + 2)],  mpInput[(x + 3)],
			mpInput[(x + 4)],  mpInput[(x + 5)],  mpInput[(x + 6)],  mpInput[(x + 7)],
			mpPifRam[(x + 0)], mpPifRam[(x + 1)], mpPifRam[(x + 2)], mpPifRam[(x + 3)],
			mpPifRam[(x + 4)], mpPifRam[(x + 5)], mpPifRam[(x + 6)], mpPifRam[(x + 7)] );
	}
	DPF_PIF("");
	DPF_PIF("");
	DPF_PIF("**                                         **");
	DPF_PIF("*********************************************");
#endif
}

#ifdef DAEDALUS_DEBUG_PIF

// Dump the PIF input

void IController::DumpInput() const
{
	DBGConsole_Msg( 0, "PIF:" );
	for ( u32 x = 0; x < 64; x+=8 )
	{
		DBGConsole_Msg( 0, "0x%02x%02x%02x%02x : 0x%02x%02x%02x%02x",
			mpInput[(x + 0)],  mpInput[(x + 1)],  mpInput[(x + 2)],  mpInput[(x + 3)],
			mpInput[(x + 4)],  mpInput[(x + 5)],  mpInput[(x + 6)],  mpInput[(x + 7)] );
	}
}
#endif


// i points to start of command

bool	IController::ProcessController(u8 *cmd, u32 channel)
{

	if( !mContPresent[channel] )
	{
		#ifdef DAEDALUS_DEBUG_PIF
		DPF_PIF("Controller %d is not connected",channel);
		#endif
        cmd[1] |= 0x80;
        cmd[3] = 0xFF;
        cmd[4] = 0xFF;
        cmd[5] = 0xFF;			// Not connected
		return true;
	}

	// From the patent, it says that if a controller is plugged in and the memory card is removed, the CONT_CARD_PULL flag will be set.
	// Cleared if CONT_RESET or CONT_GET_STATUS is issued.
	// Might need to set this if mContMemPackPresent is false?

	switch ( cmd[2] )
	{
	case CONT_RESET:
	case CONT_GET_STATUS:
	#ifdef DAEDALUS_DEBUG_PIF
		DPF_PIF("Controller: Command is RESET/STATUS");
		#endif
		cmd[3] = 0x05;
		cmd[4] = 0x00;
		cmd[5] = mContMemPackPresent[channel] ? 0x01 : 0x00;
		break;

	case CONT_READ_CONTROLLER:		// Controller
	#ifdef DAEDALUS_DEBUG_PIF
		DPF_PIF("Controller: Executing READ_CONTROLLER");
		#endif
		cmd[3] = (u8)(mContPads[channel].button >> 8);
		cmd[4] = (u8)mContPads[channel].button;
		cmd[5] = (u8)mContPads[channel].stick_x;
		cmd[6] = (u8)mContPads[channel].stick_y;
		break;

	case CONT_READ_MEMPACK:
	#ifdef DAEDALUS_DEBUG_PIF
		DPF_PIF("Controller: Command is READ_MEMPACK");
		#endif
		if(gGlobalPreferences.RumblePak)
			CommandReadRumblePack(cmd);
		else
			CommandReadMemPack(channel, cmd);
		return false;
		break;

	case CONT_WRITE_MEMPACK:
	#ifdef DAEDALUS_DEBUG_PIF
		DPF_PIF("Controller: Command is WRITE_MEMPACK");
		#endif
		if(gGlobalPreferences.RumblePak)
			CommandWriteRumblePack(cmd);
		else
			CommandWriteMemPack(channel, cmd);
		return false;
		break;

#ifdef DAEDALUS_DEBUG_CONSOLE
	default:
		DAEDALUS_ERROR( "Unknown controller command: %02x", cmd[2] );
		//DPF_PIF( DSPrintf("Unknown controller command: %02x", command) );
		break;
		#endif
	}

	return true;
}


// i points to start of command

bool	IController::ProcessEeprom(u8 *cmd)
{
	#ifdef DAEDALUS_ENABLE_ASSERTS
	DAEDALUS_ASSERT( IsEepromPresent(), "ROM is accessing the EEPROM, but none is present");
#endif
	switch(cmd[2])
	{
	case CONT_RESET:
	case CONT_GET_STATUS:
		cmd[3] = 0x00;
		cmd[4] = mEepromContType;
		cmd[5] = 0x00;
		break;

	case CONT_READ_EEPROM:
		CommandReadEeprom( cmd );
		break;

	case CONT_WRITE_EEPROM:
		CommandWriteEeprom( cmd );
		break;

	// RTC credit: Mupen64 source
	//
	case CONT_RTC_STATUS: // RTC status query
	    cmd[3] = 0x00;
	    cmd[4] = 0x10;
	    cmd[5] = 0x00;
		break;

	case CONT_RTC_READ:	// read RTC block
		CommandReadRTC( cmd );
		break;

	case CONT_RTC_WRITE:	// write RTC block
	#ifdef DAEDALUS_DEBUG_CONSOLE
		DAEDALUS_ERROR("RTC Write : %02x Not Implemented", cmd[2]);
		#endif
		break;

	default:
		#ifdef DAEDALUS_DEBUG_CONSOLE
		DAEDALUS_ERROR( "Unknown Eeprom command: %02x", cmd[2] );
		#endif
		//DPF_PIF( DSPrintf("Unknown controller command: %02x", command) );
		break;
	}

	return false;
}


//

void	IController::CommandReadEeprom(u8* cmd)
{
	memcpy(&cmd[4], mpEepromData + cmd[3] * 8, 8);
}


//

void	IController::CommandWriteEeprom(u8* cmd)
{
	Save_MarkSaveDirty();
	memcpy(mpEepromData + cmd[3] * 8, &cmd[4], 8);
}


//

#if 1	//1-> Unrolled fast 0-> old way //Corn
u8 IController::CalculateDataCrc(const u8 * pBuf)
{
	u32 c {};
	for (u32 i {}; i < 32; i++)
	{
		u32 s {pBuf[i]};

		c = (((c << 1) | ((s >> 7) & 1))) ^ ((c & 0x80) ? 0x85 : 0);
		c = (((c << 1) | ((s >> 6) & 1))) ^ ((c & 0x80) ? 0x85 : 0);
		c = (((c << 1) | ((s >> 5) & 1))) ^ ((c & 0x80) ? 0x85 : 0);
		c = (((c << 1) | ((s >> 4) & 1))) ^ ((c & 0x80) ? 0x85 : 0);
		c = (((c << 1) | ((s >> 3) & 1))) ^ ((c & 0x80) ? 0x85 : 0);
		c = (((c << 1) | ((s >> 2) & 1))) ^ ((c & 0x80) ? 0x85 : 0);
		c = (((c << 1) | ((s >> 1) & 1))) ^ ((c & 0x80) ? 0x85 : 0);
		c = (((c << 1) | ((s >> 0) & 1))) ^ ((c & 0x80) ? 0x85 : 0);
	}

	for (u32 i {8}; i != 0; i--)
	{
		c = (c << 1) ^ ((c & 0x80) ? 0x85 : 0);
	}

	return c;
}

#else
u8 IController::CalculateDataCrc(u8 * pBuf)
{
	u8 c {}, x {}, s {}, i {};
	s8 z {};

	c = 0;
	for (i {}; i < 33; i++)
	{
		s = pBuf[i];

		for (z {7}; z >= 0; z--)
		{
			x = (c & 0x80) ? 0x85 : 0;

			c <<= 1;

			if (i < 32)
			{
				if (s & (1<<z))
					c |= 1;
			}

			c = c ^ x;
		}
	}

	return c;
}
#endif


// Returns new position to continue reading
// i is the address of the first write info (after command itself)

void	IController::CommandReadMemPack(u32 channel, u8 *cmd)
{
	u32 addr {((cmd[3] << 8) | (u32)cmd[4])};
	u8* data {&cmd[5]};

	if (addr == 0x8001)
	{
		memset(data, 0, 32);
	}
	else
	{
		addr &= 0xFFE0;
		if (addr <= 0x7FE0)
		{
			memcpy(data, &mMemPack[channel][addr], 32);
		}
		else
		{
			// RumblePak
			memset( data, 0, 32 );
		}
	}

	cmd[37] = CalculateDataCrc(data);
}


// Returns new position to continue reading
// i is the address of the first write info (after command itself)

void	IController::CommandWriteMemPack(u32 channel, u8 *cmd)
{
	u32 addr {((cmd[3] << 8) | (u32)cmd[4])};
	u8* data {&cmd[5]};

	if (addr != 0x8001)
	{
		addr &= 0xFFE0;

		if (addr <= 0x7FE0)
		{
			Save_MarkMempackDirty();
			memcpy(&mMemPack[channel][addr], data, 32);
		}
		else
		{
			// Do nothing, eventually enable rumblepak
		}
	}

	cmd[37] = CalculateDataCrc(data);
}


//
//

void	IController::CommandReadRumblePack(u8 *cmd)
{
	u32 addr {((cmd[3] << 8) | (u32)cmd[4]) & 0xFFE0};

	memset( &cmd[5], (addr == 0x8000) ? 0x80 : 0x00, 32 );
	cmd[37] = CalculateDataCrc(&cmd[5]);
}


//
//

void	IController::CommandWriteRumblePack(u8 *cmd)
{
	u32 addr {((cmd[3] << 8) | (u32)cmd[4]) & 0xFFE0};

	if ( addr == 0xC000 )
	{
		gRumblePakActive = cmd[5] ? true : false;	//0 inactive and 1 for active
	}

	cmd[37] = CalculateDataCrc(&cmd[5]);
}


//
//

void	IController::CommandReadRTC(u8 *cmd)
{
	switch (cmd[3]) // block number
	{
	case 0:
		cmd[4]	= 0x00;
		cmd[5]	= 0x02;
		cmd[12] = 0x00;
		break;
	case 1:
		//DAEDALUS_ERROR("RTC command : Read Block %d", cmd[2]);
		break;
	case 2:
		time_t curtime_time;
		struct tm curtime;

		time(&curtime_time);
		memcpy(&curtime, localtime(&curtime_time), sizeof(curtime)); // fd's fix

		cmd[4]	= Byte2Bcd(curtime.tm_sec);
		cmd[5]	= Byte2Bcd(curtime.tm_min);
		cmd[6]	= 0x80 + Byte2Bcd(curtime.tm_hour);
		cmd[7]	= Byte2Bcd(curtime.tm_mday);
		cmd[8]	= Byte2Bcd(curtime.tm_wday);
		cmd[9]	= Byte2Bcd(curtime.tm_mon + 1);
		cmd[10] = Byte2Bcd(curtime.tm_year);
		cmd[11] = Byte2Bcd(curtime.tm_year / 100);
		cmd[12] = 0x00;       // status
		break;
		#ifdef DAEDALUS_DEBUG_CONSOLE
	default:
		DAEDALUS_ERROR( "Unknown Eeprom command: %02x", cmd[3] );
		break;
		#endif
	}
}


//

//http://www.emutalk.net/threads/53217-N64-PIF-CIC-NUS-6105-Algorithm-Finally-Reversed
//
//Copyright 2011 X-Scale. All rights reserved.
//
/*
 * This software provides an algorithm that emulates the protection scheme of
 * N64 PIF/CIC-NUS-6105, by determining the proper response to each challenge.
 * It was synthesized after a careful, exhaustive and detailed analysis of the
 * challenge/response pairs stored in the 'pif2.dat' file from Project 64.
 * These challenge/response pairs were the only resource used during this
 * project. There was no kind of physical access to N64 hardware.
*/

#define CHL_LEN 0x20
void IController::n64_cic_nus_6105()
{
	// calculate the proper response for the given challenge (X-Scale's algorithm)
	char lut0[0x10] = {
		0x4, 0x7, 0xA, 0x7, 0xE, 0x5, 0xE, 0x1,
		0xC, 0xF, 0x8, 0xF, 0x6, 0x3, 0x6, 0x9
	};
	char lut1[0x10] = {
		0x4, 0x1, 0xA, 0x7, 0xE, 0x5, 0xE, 0x1,
		0xC, 0x9, 0x8, 0x5, 0x6, 0x3, 0xC, 0x9
	};
	char challenge[30] {}, response[30] {};
	u32 i {};
	switch (mpPifRam[0x3F])
	{
	case 0x02:
	{
		// format the 'challenge' message into 30 nibbles for X-Scale's CIC code
		for (i = 0; i < 15; i++)
		{
			challenge[i*2] =   (mpPifRam[48+i] >> 4) & 0x0f;
			challenge[i*2+1] =  mpPifRam[48+i]       & 0x0f;
		}

		char key {}, *lut {};
		int sgn {}, mag {}, mod {};

		for (key = 0xB, lut = lut0, i = 0; i < (CHL_LEN - 2); i++)
		{
			response[i] = (key + 5 * challenge[i]) & 0xF;
			key = lut[(int) response[i]];
			sgn = (response[i] >> 3) & 0x1;
			mag = ((sgn == 1) ? ~response[i] : response[i]) & 0x7;
			mod = (mag % 3 == 1) ? sgn : 1 - sgn;
			if (lut == lut1 && (response[i] == 0x1 || response[i] == 0x9))
				mod = 1;
			if (lut == lut1 && (response[i] == 0xB || response[i] == 0xE))
				mod = 0;
			lut = (mod == 1) ? lut1 : lut0;
		}
		mpPifRam[46] = 0;
		mpPifRam[47] = 0;

		// re-format the 'response' into a byte stream
		for (i = 0; i < 15; i++)
		{
			mpPifRam[48+i] = (response[i*2] << 4) + response[i*2+1];
		}
		// the last byte (2 nibbles) is always 0
		mpPifRam[63] = 0;
		break;
	}
	case 0x08:
	{
		mpPifRam[63] = 0;
		break;
}
		#ifdef DAEDALUS_DEBUG_CONSOLE
	default:
		DAEDALUS_ERROR("Failed to decrypt pif ram");
		break;
		#endif
	}
}
