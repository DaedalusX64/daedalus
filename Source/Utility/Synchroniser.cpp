/*
Copyright (C) 2004 StrmnNrmn

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

#include "stdafx.h"
#include "Synchroniser.h"

#ifdef DAEDALUS_ENABLE_SYNCHRONISATION

#include "ZlibWrapper.h"

#include "Math/MathUtil.h"

#include "Utility/IO.h"

#include "Debug/DBGConsole.h"
#include "Debug/Dump.h"

#include "Core/CPU.h"
#include "Core/ROM.h"


//*****************************************************************************
//
//*****************************************************************************
CSynchroniser * CSynchroniser::mpSynchroniser( NULL );

//*****************************************************************************
//
//*****************************************************************************
class ISynchProducer : public CSynchroniser
{
	public:
		ISynchProducer( const char * filename );
		~ISynchProducer();

		virtual ESynchResult	SynchPoint( const void * data, u32 length );
		virtual ESynchResult	SynchData( void * data, u32 length );
		virtual void			ResetData() { mStream.Reset(); }

		virtual bool			IsOpen() const;

	private:
		Zlib::COutStream 		mStream;
};

class ISynchConsumer : public CSynchroniser
{
	public:
		ISynchConsumer( const char * filename );
		~ISynchConsumer();

		virtual ESynchResult	SynchPoint( const void * data, u32 length );
		virtual ESynchResult	SynchData( void * data, u32 length );

		virtual bool			IsOpen() const;
		virtual void			ResetData() { mStream.Reset(); }

	private:
		Zlib::CInStream			mStream;
};

//*****************************************************************************
// Creation
//*****************************************************************************
CSynchroniser *	CSynchroniser::CreateProducer( const char * filename )
{
	mpSynchroniser = new ISynchProducer( filename );
	return mpSynchroniser;
}

CSynchroniser *	CSynchroniser::CreateConsumer( const char * filename )
{
	mpSynchroniser = new ISynchConsumer( filename );
	return mpSynchroniser;
}

void CSynchroniser::Destroy()
{
	delete mpSynchroniser;
	mpSynchroniser = NULL;
}

//*****************************************************************************
//
//*****************************************************************************
void	CSynchroniser::HandleOutOfStorage()
{
	const char *	message( "Out of storage for synchroniser" );
	DBGConsole_Msg( 0, message );
	Destroy();
}

//*****************************************************************************
//
//*****************************************************************************
void	CSynchroniser::HandleOutOfInput()
{
	const char *	message( "Out of input for synchroniser" );
	DBGConsole_Msg( 0, message );
	Destroy();
}

//*****************************************************************************
//
//*****************************************************************************
void	CSynchroniser::HandleOutOfSynch( const char * msg )
{
	char			message[ 512 ];
	sprintf( message, "Synchronisation Failed at 0x%08x: %s", gCPUState.CurrentPC, msg );
	gCPUState.Dump();
	CPU_Halt( message );
	Destroy();
}

//*****************************************************************************
//
//*****************************************************************************
ISynchProducer::ISynchProducer( const char * filename )
:	mStream( filename )
{
}

//*****************************************************************************
//
//*****************************************************************************
ISynchProducer::~ISynchProducer()
{
}

//*****************************************************************************
//
//*****************************************************************************
bool	ISynchProducer::IsOpen() const
{
	return mStream.IsOpen();
}

//*****************************************************************************
//
//*****************************************************************************
CSynchroniser::ESynchResult	ISynchProducer::SynchPoint( const void * data, u32 length )
{
	if( mStream.IsOpen() )
	{
		return mStream.WriteData( data, length ) ? SR_OK : SR_OUT_OF_STORAGE;
	}

	return SR_OUT_OF_STORAGE;
}

//*****************************************************************************
//
//*****************************************************************************
CSynchroniser::ESynchResult	ISynchProducer::SynchData( void * data, u32 length )
{
	if( mStream.IsOpen() )
	{
		return mStream.WriteData( data, length ) ? SR_OK : SR_OUT_OF_STORAGE;
	}

	return SR_OUT_OF_STORAGE;
}

//*****************************************************************************
//
//*****************************************************************************
ISynchConsumer::ISynchConsumer( const char * filename )
:	mStream( filename )
{
}

//*****************************************************************************
//
//*****************************************************************************
ISynchConsumer::~ISynchConsumer()
{
}

//*****************************************************************************
//
//*****************************************************************************
bool	ISynchConsumer::IsOpen() const
{
	return mStream.IsOpen();
}

//*****************************************************************************
//
//*****************************************************************************
CSynchroniser::ESynchResult	ISynchConsumer::SynchPoint( const void * data, u32 length )
{
	if ( mStream.IsOpen() )
	{
		//
		//	Can't compare entire buffer at once, so compare in chunks
		//
		const u8 *	current_ptr( reinterpret_cast< const u8 * >( data ) );
		u32			bytes_remaining( length );

		const u32	BUFFER_SIZE = 512;
		u8			buffer[ BUFFER_SIZE ];

		while( bytes_remaining > 0 )
		{
			u32		bytes_to_process( Min( bytes_remaining, BUFFER_SIZE ) );

			if( !mStream.ReadData( buffer, bytes_to_process ) )
			{
				// Actually, we may have gone out of synch just before running out of data..
				return SR_OUT_OF_INPUT;
			}

			if( memcmp( current_ptr, buffer, bytes_to_process ) != 0 )
			{
				if (bytes_to_process == sizeof(u32))
					DBGConsole_Msg(0, "Expect 0x%08x but get 0x%08x", *(u32*)buffer,
					*(u32 *)current_ptr);
				return SR_OUT_OF_SYNCH;
			}

			current_ptr += bytes_to_process;
			bytes_remaining -= bytes_to_process;
		}

		return SR_OK;
	}

	return SR_OUT_OF_INPUT;
}

//*****************************************************************************
//
//*****************************************************************************
CSynchroniser::ESynchResult	ISynchConsumer::SynchData( void * data, u32 length )
{
	if ( mStream.IsOpen() )
	{
		return mStream.ReadData( data, length ) ? SR_OK : SR_OUT_OF_INPUT;
	}

	return SR_OUT_OF_INPUT;
}


void CSynchroniser::InitialiseSynchroniser()
{
	const char *name = g_ROM.szFileName;
	CSynchroniser *	p_synch;
	char filename[MAX_PATH + 1];

	Dump_GetSaveDirectory(filename, name, ".syn");

	if ( !IO::File::Exists(filename) )
	{
		p_synch = CSynchroniser::CreateProducer( filename );
		DBGConsole_Msg(0, "Start Synchroniser in [Gproducer] mode to create [C%s]", filename);
	}
	else
	{
		p_synch = CSynchroniser::CreateConsumer( filename );
		DBGConsole_Msg(0, "Start Synchroniser in [Gconsumer] mode to read [C%s]", filename);
	}

	//return p_synch != NULL && p_synch->IsOpen();
}

#endif	//DAEDALUS_ENABLE_SYNCHRONISATION
