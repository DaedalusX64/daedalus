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

#ifndef SYNCHRONISER_H_
#define SYNCHRONISER_H_

#ifdef DAEDALUS_ENABLE_SYNCHRONISATION
//*****************************************************************************
//	Synchronisation flags
//*****************************************************************************
enum	ESynchFlags
{
	DAED_SYNC_NONE		= 0x00000000,

	DAED_SYNC_REG_GPR	= 0x00000001,		// e.g. r0, at, t0, t1
	DAED_SYNC_REG_CCR0	= 0x00000002,		// e.g. status, control
	DAED_SYNC_REG_CPU1	= 0x00000004,		// e.g. fp00, fp01
	DAED_SYNC_REG_CCR1	= 0x00000008,		// e.g. fcr0, fcr31

	DAED_SYNC_REG_PC	= 0x00000010,		// e.g. program counter
	DAED_SYNC_FRAGMENT_PC = 0x00000020,		// e.g. program counter on entry/exit from fragments
};

//
//	Define a union of the above flags to determine what is synchronised.
//	The compiler is clever enough to optimise away calls to SYNCH_POINT
//	if a particular flag isn't set.
//
static const u32 DAED_SYNC_REGS(DAED_SYNC_REG_GPR|DAED_SYNC_REG_CCR0|
								DAED_SYNC_REG_CPU1|DAED_SYNC_REG_CCR1);

static const u32 DAED_SYNC_MASK(DAED_SYNC_FRAGMENT_PC|DAED_SYNC_REG_GPR|DAED_SYNC_REG_CCR0);

//*****************************************************************************
// Base synchronisation interface
//*****************************************************************************
class CSynchroniser
{
	public:
		virtual ~CSynchroniser() {}

		virtual bool	IsOpen() const = 0;

	protected:
		enum	ESynchResult
		{
			SR_OK = 0,
			SR_OUT_OF_STORAGE,
			SR_OUT_OF_INPUT,
			SR_OUT_OF_SYNCH,
		};

		virtual ESynchResult	SynchPoint( const void * data, u32 length ) = 0;
		virtual ESynchResult	SynchData( void * data, u32 length ) = 0;
		virtual void			ResetData() = 0;

	public:
		static void InitialiseSynchroniser();
		static CSynchroniser * CreateProducer( const char * filename );
		static CSynchroniser * CreateConsumer( const char * filename );

		static void Destroy();

		// Either write or read some data to/from the data stream
		template < typename T >	static void SynchData( T & data )
		{
			if( mpSynchroniser != NULL )
			{
				switch( mpSynchroniser->SynchData( &data, sizeof( T ) ) )
				{
					case SR_OK:					return;
					case SR_OUT_OF_STORAGE:		HandleOutOfStorage(); return;
					case SR_OUT_OF_INPUT:		HandleOutOfInput(); return;
					case SR_OUT_OF_SYNCH:		DAEDALUS_ERROR( "Shouldn't ever be out of synch?" );	return;
					default:					NODEFAULT;
				}
			}
		}

		// Check that the data is in synch at this point
		template < typename T >	static void SynchPoint( const T & data, const char * msg )
		{
			if ( mpSynchroniser != NULL )
			{
				switch ( mpSynchroniser->SynchPoint( &data, sizeof( T ) ) )
				{
					case SR_OK:					return;
					case SR_OUT_OF_STORAGE:		HandleOutOfStorage(); return;
					case SR_OUT_OF_INPUT:		HandleOutOfInput(); return;
					case SR_OUT_OF_SYNCH:		HandleOutOfSynch( msg );	return;
					default:					NODEFAULT;
				}
			}
		}

		static void Reset()
		{
			if ( mpSynchroniser != NULL )
			{
				mpSynchroniser->ResetData();
			}
		}

		static void		HandleOutOfStorage();
		static void		HandleOutOfInput();
		static void		HandleOutOfSynch( const char * msg );

	private:
		static CSynchroniser *	mpSynchroniser;
};


#define		SYNCH_POINT( flags, x, msg )		if ( DAED_SYNC_MASK & (flags) ) CSynchroniser::SynchPoint( x, msg )
#define		SYNCH_DATA( x )						CSynchroniser::SynchData( x )

#else

#define		SYNCH_POINT( flags, x, msg )
#define		SYNCH_DATA( x )

#endif


#endif // SYNCHRONISER_H_
