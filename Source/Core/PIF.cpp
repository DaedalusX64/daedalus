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

#include "Debug/DBGConsole.h"
#include "Input/InputManager.h"
#include "Debug/Dump.h"		// Dump_GetSaveDirectory()

#include "OSHLE/ultra_os.h"

#ifdef _MSC_VER
#pragma warning(default : 4002) 
#endif

#ifdef DAEDALUS_DEBUG_PIF
	#define DPF_PIF( ... )		{ if ( mDebugFile ) { fprintf( mDebugFile, __VA_ARGS__ ); fprintf( mDebugFile, "\n" ); } }
#else
	#define DPF_PIF( ... )
#endif

//*****************************************************************************
//
//*****************************************************************************
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
		void			FormatChannels();

		bool			ProcessCommand(u32 i, u32 iError, u32 channel, u32 ucWrite, u32 ucRead);

		bool			CommandStatusEeprom(u32 i, u32 iError, u32 ucWrite, u32 ucRead);
		bool			CommandReadEeprom(u32 i, u32 iError, u32 ucWrite, u32 ucRead);
		bool			CommandWriteEeprom(u32 i, u32 iError, u32 ucWrite, u32 ucRead);
		bool			CommandReadMemPack(u32 i, u32 iError, u32 channel, u32 ucWrite, u32 ucRead);
		bool			CommandWriteMemPack(u32 i, u32 iError, u32 channel, u32 ucWrite, u32 ucRead);

		u8				CalculateDataCrc(u8 * pBuf);

		bool			IsEepromPresent() const						{ return mpEepromData != NULL; }
		u16				GetEepromContType() const					{ return mEepromContType; }

		void			SetPifByte( u32 index, u8 value )			{ mpPifRam[ index ^ U8_TWIDDLE ] = value; }
		u8				GetPifByte( u32 index ) const				{ return mpPifRam[ index ^ U8_TWIDDLE ]; }

		void			WriteStatusBits( u32 index, u8 value );
		void			Write8Bits( u32 index, u8 value )			{ mpPifRam[ index ^ U8_TWIDDLE ] = value; }
		void			Write16Bits( u32 index, u16 value );
		void			Write16Bits_Swapped( u32 index, u16 value );

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

		struct SChannelFormat
		{
			bool		Skipped;
			u32			StatusByte;
			u32			RxDataStart;
			u32			RxDataLength;
			u32			TxDataStart;
			u32			TxDataLength;
		};

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
		static const u32	NUM_CONTROLLERS = 4;

		OSContPad		mContPads[ NUM_CONTROLLERS ];
		bool			mContPresent[ NUM_CONTROLLERS ];
		bool			mContMemPackPresent[ NUM_CONTROLLERS ];

		u8 *			mpEepromData;
		u16				mEepromContType;					// 0, CONT_EEPROM or CONT_EEP16K
		u32				mEepromSize;

		u8				*mMemPack[ NUM_CONTROLLERS ];

		SChannelFormat	mChannelFormat[ NUM_CHANNELS ];
		u8				rumble[32];

	#ifdef DAEDALUS_DEBUG_PIF
		FILE *			mDebugFile;
	#endif

};


//*****************************************************************************
// Singleton creator
//*****************************************************************************
template<> bool	CSingleton< CController >::Create()
{
	DAEDALUS_ASSERT_Q(mpInstance == NULL);
	
	mpInstance = new IController();

	return true;
}

//*****************************************************************************
// Constructor
//*****************************************************************************
IController::IController() :
	mpPifRam( NULL ),
	mpEepromData( NULL ),
	mEepromSize( 0 )
{
#ifdef DAEDALUS_DEBUG_PIF
	mDebugFile = fopen( "controller.txt", "w" );
#endif
	for ( u32 i = 0; i < NUM_CONTROLLERS; i++ )
	{
		mContPresent[ i ] = true;
		mContMemPackPresent[ i ] = false;
	}
	mContMemPackPresent[ 0 ] = true;

	memset( mChannelFormat, 0, sizeof( mChannelFormat ) );
	for ( u32 i = 0; i < NUM_CHANNELS; i++ )
	{
		mChannelFormat[ i ].Skipped = true;
	}
}

//*****************************************************************************
// Destructor
//*****************************************************************************
IController::~IController()
{
#ifdef DAEDALUS_DEBUG_PIF
	if( mDebugFile != NULL )
	{
		fclose( mDebugFile );
	}
#endif
}

//*****************************************************************************
// Called whenever a new rom is opened
//*****************************************************************************
bool IController::OnRomOpen()
{
	ESaveType save_type  = g_ROM.settings.SaveType;
	mpPifRam = (u8 *)g_pMemoryBuffers[MEM_PIF_RAM] + 0x7C0;
	
	if ( mpEepromData )
	{
		mpEepromData = NULL;
	}

	if ( save_type == SAVE_TYPE_EEP4K )
	{
		mEepromSize = 4096/8;					// 4k bits
		mpEepromData = (u8*)g_pMemoryBuffers[MEM_SAVE];
		mEepromContType = CONT_EEPROM;

		DBGConsole_Msg( 0, "Initialising EEPROM to [M%d] bytes", mEepromSize );
	}
	else if ( save_type == SAVE_TYPE_EEP16K )
	{
		mEepromSize = 16384/8;					// 16 kbits
		mpEepromData = (u8*)g_pMemoryBuffers[MEM_SAVE];
		mEepromContType = CONT_EEP16K | CONT_EEPROM;

		DBGConsole_Msg( 0, "Initialising EEPROM to [M%d] bytes", mEepromSize );
	}
	else
	{
		mEepromSize = 0;
		mpEepromData = NULL;
		mEepromContType = 0;
	}

	u32 channel;

	for ( channel = 0; channel < NUM_CONTROLLERS; channel++ )
	{
		mMemPack[channel] = (u8*)g_pMemoryBuffers[MEM_MEMPACK] + channel * 0x400 * 32;
	}


	return true;
}

//*****************************************************************************
// Called as a rom is closed
//*****************************************************************************
void IController::OnRomClose()
{
}

//*****************************************************************************
//
//*****************************************************************************
void IController::FormatChannels()
{
#ifdef DAEDALUS_DEBUG_PIF
	DPF_PIF("");
	DPF_PIF("");
	DPF_PIF("*********************************************");
	DPF_PIF("**                                         **");
	DPF_PIF("Formatting:");

	for ( u32 x = 0; x < 64; x+=8 )
	{
		DPF_PIF( "0x%02x%02x%02x%02x : 0x%02x%02x%02x%02x",
			mpPifRam[(x + 0) ^ U8_TWIDDLE],  mpPifRam[(x + 1) ^ U8_TWIDDLE],  mpPifRam[(x + 2) ^ U8_TWIDDLE],  mpPifRam[(x + 3) ^ U8_TWIDDLE],
			mpPifRam[(x + 4) ^ U8_TWIDDLE],  mpPifRam[(x + 5) ^ U8_TWIDDLE],  mpPifRam[(x + 6) ^ U8_TWIDDLE],  mpPifRam[(x + 7) ^ U8_TWIDDLE] );
	}
	DPF_PIF("");
	DPF_PIF("");

#endif

	// Ignore the status - just use the previous values
	u32		i( 0 );
	u32		channel( 0 );
	bool	finished( false );

	while ( i < 64 && channel < NUM_CHANNELS && !finished )
	{
		u8 tx_data_size;
		u8 rx_data_size;
		tx_data_size = GetPifByte( i );

		switch ( tx_data_size )
		{
		case CONT_TX_SIZE_CHANSKIP:
			DPF_PIF( "Code 0x%02x - ChanSkip(%d)", tx_data_size, channel );
			mChannelFormat[ channel ].Skipped = true;
			channel++;
			i++;
			break;

		case CONT_TX_SIZE_DUMMYDATA:
			DPF_PIF( "Code 0x%02x - DummyData", tx_data_size );
			i++;
			break;

		case CONT_TX_SIZE_FORMAT_END:
			DPF_PIF( "Code 0x%02x - FormatEnd", tx_data_size );
			finished = true;
			break;
			
		case CONT_TX_SIZE_CHANRESET:
			DPF_PIF( "Code 0x%02x - ChanReset (Ignoring)", tx_data_size );
			mChannelFormat[ channel ].Skipped = true;
			channel++;
			i++;
			break;

		default:
			rx_data_size = GetPifByte( i + 1 );
			if (rx_data_size == CONT_TX_SIZE_FORMAT_END)
			{
				finished = true;
				break;
			}
			// Set up error pointer and skip read/write bytes of input
			u32 iError = i + 1;
			i += 2;

			// Mask off high 2 bits. In rx this is used for error status. Not sure about tx, but we can only reference 1<6 bits anyway.
			tx_data_size &= 0x3f;
			rx_data_size &= 0x3f;

			mChannelFormat[ channel ].Skipped = false;
			mChannelFormat[ channel ].StatusByte = iError;
			mChannelFormat[ channel ].TxDataStart = i;
			mChannelFormat[ channel ].TxDataLength = tx_data_size;
			mChannelFormat[ channel ].RxDataStart = i + tx_data_size;
			mChannelFormat[ channel ].RxDataLength = rx_data_size;

			// Did skip 1 byte for write/read mempack here. Think this was wrong
			//if( GetPifByte( i ) == CONT_READ_MEMPACK || GetPifByte( i ) == CONT_WRITE_MEMPACK )
			//{
			//	i++;
			//}

			DPF_PIF( "Channel %d, TxSize %d (@%d), RxSize %d (@%d)", channel, tx_data_size, i, rx_data_size, i+tx_data_size );

			i += tx_data_size + rx_data_size;
			channel++;
			break;
		}
	}

	// Set the remaining channels to skipped
	while( channel < NUM_CHANNELS )
	{
		mChannelFormat[ channel ].Skipped = true;
		channel++;
	}

#ifdef DAEDALUS_DEBUG_PIF

	DPF_PIF("Results:");

	for( u32 channel = 0; channel < NUM_CHANNELS; ++channel )
	{
		const SChannelFormat & format( mChannelFormat[ channel ] );
		if( format.Skipped )
		{
			DPF_PIF( "%02d: Skipped", channel );
		}
		else
		{
			DPF_PIF( "%02d: StatusIdx: %02d, TxSize %02d (@%02d), RxSize %02d (@%02d)",
								channel,
								format.StatusByte, 
								format.TxDataLength, format.TxDataStart,
								format.RxDataLength, format.RxDataStart );
		}
	}

	DPF_PIF("");
	DPF_PIF("");
	DPF_PIF("**                                         **");
	DPF_PIF("*********************************************");

#endif
}

//*****************************************************************************
//
//*****************************************************************************
void IController::Process()
{
#ifdef DAEDALUS_DEBUG_PIF
	memcpy(mpInput, mpPifRam, 64);
	DPF_PIF("");
	DPF_PIF("");
	DPF_PIF("*********************************************");
	DPF_PIF("**                                         **");
#endif
	
	// Clear to indicate success - we might set this again in the handler code
	if( GetPifByte( 63 ) == 1 )
	{
		FormatChannels();
	}
	SetPifByte( 63, 0 );

	CInputManager::Get()->GetState( mContPads );

	// Ignore the status - just use the previous values

	for( u32 channel = 0; channel < NUM_CHANNELS; ++channel )
	{
		const SChannelFormat & format( mChannelFormat[ channel ] );

		if( format.Skipped )
		{
			DPF_PIF( "Skipping channel %d", channel );
			continue;
		}

		// XXXX Check tx/rx status flags here??

		if ( !ProcessCommand( format.TxDataStart, format.StatusByte, channel, format.TxDataLength, format.RxDataLength ) )
		{
			break;
		}
	}

#ifdef DAEDALUS_DEBUG_PIF
	DPF_PIF("Before | After:");

	for ( u32 x = 0; x < 64; x+=8 )
	{
		DPF_PIF( "0x%02x%02x%02x%02x : 0x%02x%02x%02x%02x  |  0x%02x%02x%02x%02x : 0x%02x%02x%02x%02x",
			mpInput[(x + 0) ^ U8_TWIDDLE],  mpInput[(x + 1) ^ U8_TWIDDLE],  mpInput[(x + 2) ^ U8_TWIDDLE],  mpInput[(x + 3) ^ U8_TWIDDLE],
			mpInput[(x + 4) ^ U8_TWIDDLE],  mpInput[(x + 5) ^ U8_TWIDDLE],  mpInput[(x + 6) ^ U8_TWIDDLE],  mpInput[(x + 7) ^ U8_TWIDDLE],
			mpPifRam[(x + 0) ^ U8_TWIDDLE], mpPifRam[(x + 1) ^ U8_TWIDDLE], mpPifRam[(x + 2) ^ U8_TWIDDLE], mpPifRam[(x + 3) ^ U8_TWIDDLE],
			mpPifRam[(x + 4) ^ U8_TWIDDLE], mpPifRam[(x + 5) ^ U8_TWIDDLE], mpPifRam[(x + 6) ^ U8_TWIDDLE], mpPifRam[(x + 7) ^ U8_TWIDDLE] );
	}
	DPF_PIF("");
	DPF_PIF("");
	DPF_PIF("**                                         **");
	DPF_PIF("*********************************************");
#endif
}

#ifdef DAEDALUS_DEBUG_PIF
//*****************************************************************************
// Dump the PIF input
//*****************************************************************************
void IController::DumpInput() const
{
	DBGConsole_Msg( 0, "PIF:" );
	for ( u32 x = 0; x < 64; x+=8 )
	{
		DBGConsole_Msg( 0, "0x%02x%02x%02x%02x : 0x%02x%02x%02x%02x",
			mpInput[(x + 0) ^ U8_TWIDDLE],  mpInput[(x + 1) ^ U8_TWIDDLE],  mpInput[(x + 2) ^ U8_TWIDDLE],  mpInput[(x + 3) ^ U8_TWIDDLE],
			mpInput[(x + 4) ^ U8_TWIDDLE],  mpInput[(x + 5) ^ U8_TWIDDLE],  mpInput[(x + 6) ^ U8_TWIDDLE],  mpInput[(x + 7) ^ U8_TWIDDLE] );
	}
}
#endif

//*****************************************************************************
// i points to start of command
//*****************************************************************************
bool	IController::ProcessCommand(u32 i, u32 iError, u32 channel, u32 ucWrite, u32 ucRead)
{
	bool	success( true );
	u8		command( GetPifByte( i + 0 ) );			// Read command

	i++;
	ucWrite--;

	WriteStatusBits( iError, 0 );					// Clear error flags
	

	// From the patent, it says that if a controller is plugged in and the memory card is removed, the CONT_CARD_PULL flag will be set.
	// Cleared if CONT_RESET or CONT_GET_STATUS is issued.
	// Might need to set this if mContMemPackPresent is false?

	// i Currently points to data to write to
	switch ( command )
	{
	case CONT_RESET:

		if (channel < NUM_CONTROLLERS)
		{
			DPF_PIF("Controller: Command is RESET");

			if (mContPresent[channel])
			{
				DAEDALUS_ASSERT( ucRead <= 3, "Reading too many bytes for RESET command" );

				if ( ucRead < 3 )
				{
					DAEDALUS_ERROR( "Overrun on RESET" );
					WriteStatusBits( iError, CONT_OVERRUN_ERROR );				// Transfer error...
				}

				Write16Bits( i, CONT_TYPE_NORMAL );
				Write8Bits( i+2, mContMemPackPresent[channel] ? CONT_CARD_ON : 0 );	// Is the mempack plugged in?
			}
			else
			{
				WriteStatusBits( iError, CONT_NO_RESPONSE_ERROR );				// Not connected
			}
		}
		else if (channel == PC_EEPROM)
		{
			DAEDALUS_ERROR( "Executing Reset on EEPROM - is this ok?" );
			success = CommandStatusEeprom( i, iError, ucWrite, ucRead );
		}
		else
		{
			//DPF_PIF(DSPrintf("Controller: UnkStatus, Channel = %d", channel));
			DAEDALUS_ERROR( "Trying to reset for invalid controller channel!" );
		}	
		break;

	case CONT_GET_STATUS:
		if (channel < NUM_CONTROLLERS)
		{
			DPF_PIF("Controller: Executing GET_STATUS");

			if (mContPresent[channel])
			{
				DAEDALUS_ASSERT( ucRead <= 3, "Reading too many bytes for STATUS command" );

				if (ucRead < 3)
				{
					DAEDALUS_ERROR( "Overrun on GET_STATUS" );
					WriteStatusBits( iError, CONT_OVERRUN_ERROR );				// Transfer error...
				}

				Write16Bits( i, CONT_TYPE_NORMAL );
				Write8Bits( i+2, mContMemPackPresent[channel] ? CONT_CARD_ON : 0 );	// Is the mempack plugged in?
			}
			else
			{
				WriteStatusBits( iError, CONT_NO_RESPONSE_ERROR );				// Not connected
			}
		}
		else if (channel == PC_EEPROM)
		{
			// This is eeprom status?
			DPF_PIF("Controller: Executing GET_EEPROM_STATUS?");

			success = CommandStatusEeprom( i, iError, ucWrite, ucRead );
		}
		else
		{
			//DPF_PIF("Controller: UnkStatus, Channel = %d", channel);
			DAEDALUS_ERROR( "Trying to get status for invalid controller channel!" );
		}	
		break;


	case CONT_READ_CONTROLLER:		// Controller
		if ( channel < NUM_CONTROLLERS )
		{
			DPF_PIF("Controller: Executing READ_CONTROLLER");
			// This is controller status
			if (mContPresent[channel])
			{
				DAEDALUS_ASSERT( ucRead <= 4, "Reading too many bytes for READ_CONT command" );

				if (ucRead < 4)
				{
					DAEDALUS_ERROR( "Overrun on READ_CONT" );
					WriteStatusBits( iError, CONT_OVERRUN_ERROR );
				}

				// Hack - we need to only write the number of bytes asked for!
				Write16Bits_Swapped( i, mContPads[channel].button );
				Write8Bits( i+2, mContPads[channel].stick_x );
				Write8Bits( i+3, mContPads[channel].stick_y );	
			}
			else
			{
				// Not connected			
				WriteStatusBits( iError, CONT_NO_RESPONSE_ERROR );
			}
		}
		else
		{
			DAEDALUS_ERROR( "Trying to read from invalid controller channel!" );
		}
		break;

	case CONT_READ_MEMPACK:
		if ( channel < NUM_CONTROLLERS )
		{
			DPF_PIF("Controller: Command is READ_MEMPACK");
			if (mContPresent[channel] && g_ROM.GameHacks != CHAMELEON_TWIST)
			{
				DPF_PIF( "Mempack present" );
				success = CommandReadMemPack(i, iError, channel, ucWrite, ucRead);
			}
			else
			{
				DPF_PIF( "Mempack not present" );
				WriteStatusBits( iError, CONT_NO_RESPONSE_ERROR );
			}
		}
		else
		{
			DAEDALUS_ERROR( "Trying to read mempack from invalid controller channel!" );
		}
		break;

	case CONT_WRITE_MEMPACK:
		if ( channel < NUM_CONTROLLERS )
		{
			DPF_PIF("Controller: Command is WRITE_MEMPACK");
			if (mContPresent[channel])
			{
				success = CommandWriteMemPack(i, iError, channel, ucWrite, ucRead);
			}
			else
			{
				WriteStatusBits( iError, CONT_NO_RESPONSE_ERROR );
			}
		}
		else
		{
			DAEDALUS_ERROR( "Trying to write mempack to invalid controller channel!" );
		}
		break;

	case CONT_READ_EEPROM:		return CommandReadEeprom(i, iError, ucWrite, ucRead);
	case CONT_WRITE_EEPROM:		return CommandWriteEeprom(i, iError, ucWrite, ucRead);

	default:
		DAEDALUS_ERROR( "Unknown controller command: %02x", command );
		//DPF_PIF( DSPrintf("Unknown controller command: %02x", command) );
		break;
	}

	return success;
}



//*****************************************************************************
// i points to start of command
//*****************************************************************************
bool	IController::CommandStatusEeprom(u32 i, u32 iError, u32 ucWrite, u32 ucRead)
{

	DPF_PIF("Controller: GetStatusEEPROM");

	if (ucWrite != 0 || ucRead > 3)
	{
		WriteStatusBits( iError, CONT_OVERRUN_ERROR );
		DAEDALUS_ERROR( "GetEepromStatus Overflow" );
		return false;
	}

	DAEDALUS_ASSERT( ucRead == 3, "Why is ucRead not 3 for an Eeprom read?" );

	if ( IsEepromPresent() )
	{
		Write16Bits(i, GetEepromContType() );
		Write8Bits(i+2, 0x00);

		i += 3;
		ucRead -= 3;
	}
	else
	{
		DAEDALUS_ERROR( "ROM is accessing the EEPROM, but none is present" );
		WriteStatusBits( iError, CONT_NO_RESPONSE_ERROR );
	}

	DAEDALUS_ASSERT( ucWrite == 0 && ucRead == 0, "GetEepromStatus Read / Write bytes remaining" );
	return true;

}

//*****************************************************************************
//
//*****************************************************************************
bool	IController::CommandReadEeprom(u32 i, u32 iError, u32 ucWrite, u32 ucRead)
{
	u8 block;

	DPF_PIF("Controller: ReadEEPROM");

	DAEDALUS_ASSERT( ucWrite+1 == 2, "Why is tx_data_size not 2 for READ_EEP?" );
	DAEDALUS_ASSERT( ucRead == 8, "Why is rx_data_size not 1 for WRITE_EEP?" );

	if (ucWrite != 1 || ucRead > 8)
	{
		WriteStatusBits( iError, CONT_OVERRUN_ERROR );
		DAEDALUS_ERROR( "ReadEeprom Overflow" );
		return false;
	}


	// Read the block 
	block = GetPifByte( i + 0 );
	i++;
	ucWrite--;

#ifdef DAEDALUS_DEBUG_PIF
	if ( ucRead < 8 )
	{
		DAEDALUS_ERROR( "Overrun on READ_EEPROM" );
	}
#endif

	if ( IsEepromPresent() )
	{
		// TODO limit block to mEepromSize / 8
#ifdef DAEDALUS_DEBUG_PIF
		if (block*8+ucRead > mEepromSize)
		{
			DAEDALUS_ERROR( "Reading outside of EEPROM bounds" );
		}
#endif
		u32 j = 0;
		while (ucRead)
		{
			SetPifByte( i, mpEepromData[(block*8 + j) ^ U8_TWIDDLE] );

			i++;
			j++;
			ucRead--;
		}
	}
	else
	{
		WriteStatusBits( iError, CONT_NO_RESPONSE_ERROR );
	}

	DAEDALUS_ASSERT( ucWrite == 0 && ucRead == 0, "ReadEeprom Read / Write bytes remaining" );
	return true;
}



//*****************************************************************************
//
//*****************************************************************************
bool	IController::CommandWriteEeprom(u32 i, u32 iError, u32 ucWrite, u32 ucRead)
{
	u8 block;

	DPF_PIF("Controller: WriteEEPROM");
	DAEDALUS_ASSERT( ucWrite+1 == 10, "Why is tx_data_size not 10 for WRITE_EEP?" );
	// Forsaken 64
	DAEDALUS_ASSERT( ucRead == 1, "Why is rx_data_size not 1 for WRITE_EEP?" );

	// 9 bytes of input remaining - 8 bytes data + block
	if (ucWrite != 9 /*|| ucRead != 1*/)
	{
		WriteStatusBits( iError, CONT_OVERRUN_ERROR );
		DAEDALUS_ERROR( "WriteEeprom Overflow" );
		return false;
	}

	// Read the block 
	block = GetPifByte( i + 0 );
	i++;
	ucWrite--;

	if ( IsEepromPresent() )
	{
		Save::MarkSaveDirty();

		// TODO limit block to mEepromSize / 8
#ifdef DAEDALUS_DEBUG_PIF
		if (block*8+ucWrite > mEepromSize)
		{
			DAEDALUS_ERROR( "Writing outside of EEPROM bounds" );
		}
#endif
		u32 j = 0;
		while (ucWrite)
		{
			mpEepromData[(block*8 + j) ^ U8_TWIDDLE] = GetPifByte( i );

			i++;
			j++;
			ucWrite--;
		}
	}
	else
	{
		WriteStatusBits( iError, CONT_NO_RESPONSE_ERROR );
	}

	// What on earth is this for??
	ucRead--;

	DAEDALUS_ASSERT( ucWrite == 0 && ucRead == 0, "WriteEeprom Read / Write bytes remaining" );
	return true;
}

//*****************************************************************************
// Returns new position to continue reading
// i is the address of the first write info (after command itself)
//*****************************************************************************
bool	IController::CommandReadMemPack( u32 i, u32 iError, u32 channel, u32 ucWrite, u32 ucRead )
{
	DPF_PIF( "CommandReadMemPack(%d, %d, %d, %d, %d)", i, iError, channel, ucWrite, ucRead );
	u32 j;
	u32 address_crc;
	u32 address;
	u32 dwCRC;
	u8 ucDataCRC;
	u8 * pBuf;
	
	// There must be exactly 2 bytes to write, and 33 bytes to read
	if (ucWrite != 2 || ucRead != 33)
	{
		// TRANSFER ERROR!!!!
		DBGConsole_Msg( 0, "RMemPack with bad tx/rxdata size (%d/%d)\n", ucWrite, ucRead );
		WriteStatusBits( iError, CONT_OVERRUN_ERROR );
		return false;
	}
#ifdef DAEDALUS_DEBUG_PIF
	//DPF_PIF( DSPrintf("ReadMemPack: Channel %d, i is %d", channel, i) );
	DAEDALUS_ERROR( "ReadMemPack: Channel %d, i is %d", channel, i );
#endif
	// Get address..
	address_crc = (GetPifByte( i + 0 ) << 8) | GetPifByte( i + 1 );

	address = (address_crc >> 5);
	dwCRC = (address_crc & 0x1f);
	i += 2;	
	ucWrite -= 2;


	if (address > 0x400)
	{
		// SOME OTHER ERROR!
		DAEDALUS_ERROR( "ReadMemPack: Address out of range: 0x%08x", address );
		return false;
	}
	else if ( address == 0x400 )
	{
		memset( rumble, 0, 32 );

		for (j = 0; j < 32; j++)
		{
			if (i < 64)
			{
				SetPifByte( i, rumble[j] );
			}
			i++;
			ucRead--;
		}

		// For some reason this seems to be negated when operating on this address
		// If I negate the result here, Zelda OoT stops working :/
		ucDataCRC = CalculateDataCrc(rumble);
	}
	else
	{
		pBuf = &mMemPack[channel][address * 32];
#ifdef DAEDALUS_DEBUG_PIF
		//DPF_PIF( DSPrintf("Controller: Reading from block 0x%04x (crc: 0x%02x)", address, dwCRC) );
		DAEDALUS_ERROR( "Controller: Reading from block 0x%04x (crc: 0x%02x)", address, dwCRC );
#endif		
		for (j = 0; j < 32; j++)
		{
			if (i < 64)
			{
				SetPifByte( i, pBuf[j] );
			}
			i++;
			ucRead--;
		}

		ucDataCRC = CalculateDataCrc(pBuf);
	}
#ifdef DAEDALUS_DEBUG_PIF	
	//DPF_PIF( DSPrintf("Controller: data crc is 0x%02x", ucDataCRC) );
	DAEDALUS_ERROR( "Controller: data crc is 0x%02x", ucDataCRC );
#endif
	// Write the crc value:
	SetPifByte( i, ucDataCRC );
	i++;
	ucRead--;
#ifdef DAEDALUS_DEBUG_PIF	
	//DPF_PIF( DSPrintf("Returning, setting i to %d", i + 1) );
	DAEDALUS_ERROR( "Returning, setting i to %d", i + 1 );
#endif
	// With wetrix, there is still a padding byte?
	DAEDALUS_ASSERT( ucWrite == 0 && ucRead == 0, "ReadMemPack / Write bytes remaining" );
	return true;
}


//*****************************************************************************
// Returns new position to continue reading
// i is the address of the first write info (after command itself)
//*****************************************************************************
bool	IController::CommandWriteMemPack(u32 i, u32 iError, u32 channel, u32 ucWrite, u32 ucRead)
{
	u32 j;
	u32 address_crc;
	u32 address;
	u32 dwCRC;
	u8 ucDataCRC;
	u8 * pBuf;

	// There must be exactly 32+2 bytes to read

	if (ucWrite != 34 || ucRead != 1)
	{
		DAEDALUS_ERROR( "WriteMemPack with bad tx/rxdata size (%d/%d)", ucWrite, ucRead );
		DBGConsole_Msg( 0, "WMemPack with bad tx/rxdata size (%d/%d)\n", ucWrite, ucRead );
		WriteStatusBits( iError, CONT_OVERRUN_ERROR );
		return false;
	}
#ifdef DAEDALUS_DEBUG_PIF
	//DPF_PIF( DSPrintf("WriteMemPack: Channel %d, i is %d", channel, i) );
	DAEDALUS_ERROR("WriteMemPack: Channel %d, i is %d", channel, i );
#endif
	// Get address..
	address_crc = (GetPifByte( i + 0 ) << 8) | GetPifByte( i + 1 );

	address = (address_crc >>5);
	dwCRC = (address_crc & 0x1f);
	i += 2;	
	ucWrite -= 2;


	if (address > 0x400/* && address != 0x600*/)
	{
		// 0x600 is mempack enable address?
		/*if (address == 0x600)
		{
			pBuf = &arrTemp[0];
		}*/
		DAEDALUS_ERROR( "Attempting to write to non-existing block 0x%08x", address );
		return false;
	}
	else if ( address == 0x400 )
	{
		// Do nothing - enable rumblepak eventually
		for (j = 0; j < 32; j++)
		{
			if (i < 64)
			{
				rumble[j] = GetPifByte( i );
			}
			i++;
			ucWrite--;
		}
		
		// For some reason this seems to be negated when operating on this address
		// If I negate the result here, Zelda OoT stops working :/
		ucDataCRC = CalculateDataCrc(rumble);
	}
	else
	{
		pBuf = &mMemPack[channel][address * 32];
#ifdef DAEDALUS_DEBUG_PIF
		//DPF_PIF( DSPrintf("Controller: Writing block 0x%04x (crc: 0x%02x)", address, dwCRC) );
		DAEDALUS_ERROR( "Controller: data crc is 0x%02x", ucDataCRC );
#endif
		for (j = 0; j < 32; j++)
		{
			if (i < 64)
			{
				pBuf[j] = GetPifByte( i );
			}
			i++;
			ucWrite--;
		}
		ucDataCRC = CalculateDataCrc(pBuf);
	}
#ifdef DAEDALUS_DEBUG_PIF
	//DPF_PIF( DSPrintf("Controller: data crc is 0x%02x", ucDataCRC) );
	DAEDALUS_ERROR( "Controller: data crc is 0x%02x", ucDataCRC );
#endif
	// Write the crc value:
	SetPifByte( i, ucDataCRC );
	i++;
	ucRead--;
	
	// With wetrix, there is still a padding byte?
	DAEDALUS_ASSERT( ucWrite == 0 && ucRead == 0, "WriteMemPack / Write bytes remaining" );

	return true;
}

//*****************************************************************************
//
//*****************************************************************************
u8 IController::CalculateDataCrc(u8 * pBuf)
{
	u8 c;
	u8 x,s;
	u8 i;
	s8 z;

	c = 0;
	for (i = 0; i < 33; i++)
	{
		s = pBuf[i];

		for (z = 7; z >= 0; z--)
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

//*****************************************************************************
//
//*****************************************************************************
void IController::WriteStatusBits(u32 i, u8 val)
{
	mpPifRam[(i + 0) ^ U8_TWIDDLE] &= 0x3F;
	mpPifRam[(i + 0) ^ U8_TWIDDLE] |= val;
}

//*****************************************************************************
//
//*****************************************************************************
void IController::Write16Bits(u32 i, u16 val)
{
	mpPifRam[(i + 0) ^ U8_TWIDDLE] = (u8)(val&0xff);	// Lo
	mpPifRam[(i + 1) ^ U8_TWIDDLE] = (u8)(val>>8);	// Hi
}

//*****************************************************************************
//
//*****************************************************************************
void IController::Write16Bits_Swapped(u32 i, u16 val)
{
	mpPifRam[(i + 0) ^ U8_TWIDDLE] = (u8)(val>>8);	// Hi
	mpPifRam[(i + 1) ^ U8_TWIDDLE] = (u8)(val&0xff);	// Lo
}


