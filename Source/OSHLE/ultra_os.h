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

#ifndef __ULTRA_OS__
#define __ULTRA_OS__

// Definitions for N64 Operating System structures

/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
//                    Types                        //
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////


typedef s32	OSPri;
typedef s32	OSId;
typedef
	union
	{
		struct
		{
			f32 f_odd;
			f32 f_even;
		} f;
		f32 d;//f64 d;
	}
__OSfp;

typedef struct {
	u64	at, v0, v1, a0, a1, a2, a3;
	u64	t0, t1, t2, t3, t4, t5, t6, t7;
	u64	s0, s1, s2, s3, s4, s5, s6, s7;
	u64	t8, t9,         gp, sp, s8, ra;
	u64	lo, hi;
	u32	sr, pc, cause, badvaddr, rcp;
	u32	fpcsr;
	__OSfp	 fp0,  fp2,  fp4,  fp6,  fp8, fp10, fp12, fp14;
	__OSfp	fp16, fp18, fp20, fp22, fp24, fp26, fp28, fp30;
} __OSThreadContext;

typedef struct OSThread_s
{
	struct		OSThread_s	*next;		// run/mesg queue link
	OSPri		priority;				// run/mesg queue priority
	struct		OSThread_s	**queue;	// queue thread is on
	struct		OSThread_s	*tlnext;	// all threads queue link
	u16			state;					// OS_STATE_*
	u16			flags;					// flags for rmon
	OSId		id;						// id for debugging
	int			fp;						// thread has used fp unit
	__OSThreadContext	context;		// register/interrupt mask
} OSThread;

typedef u32 OSEvent;
typedef u32 OSIntMask;
typedef u32 OSPageMask;
typedef u32 OSHWIntr;


//
// Structure for message
//
typedef void *	OSMesg;

//
// Structure for message queue
//
typedef struct OSMesgQueue_s
{
	OSThread	*mtqueue;		// Queue to store threads blocked
								//   on empty mailboxes (receive)
	OSThread	*fullqueue;		// Queue to store threads blocked
								//   on full mailboxes (send)
	s32			validCount;		// Contains number of valid message
	s32			first;			// Points to first valid message
	s32			msgCount;		// Contains total # of messages
	OSMesg		*msg;			// Points to message buffer array
} OSMesgQueue;


typedef struct {
	u16     button;
	s8      stick_x;		// -80 <= stick_x <= 80
	s8      stick_y;		// -80 <= stick_y <= 80
	u8	errno;
} OSContPad;


/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
//                     VI                          //
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////

//
// Structure to store VI register values that remain the same between 2 fields
//
typedef struct {
    u32	ctrl;
    u32	width;
    u32	burst;
    u32	vSync;
    u32	hSync;
    u32	leap;
    u32	hStart;
    u32	xScale;
    u32	vCurrent;
} OSViCommonRegs;


//
// Structure to store VI register values that change between fields
//
typedef struct {
    u32	origin;
    u32	yScale;
    u32	vStart;
    u32	vBurst;
    u32	vIntr;
} OSViFieldRegs;


//
// Structure for VI mode
//
typedef struct {
    u8			type;			// Mode type
    OSViCommonRegs	comRegs;	// Common registers for both fields
    OSViFieldRegs	fldRegs[2];	// Registers for Field 1  & 2
} OSViMode;



/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
//                    Timer                        //
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////

//
// Structure for time value
//
typedef u64	OSTime;

//
// Structure for interval timer
//
typedef struct OSTimer_s {
	struct OSTimer_s *	next;		// point to next timer in list
	struct OSTimer_s *	prev;		// point to previous timer in list
	OSTime				interval;	// duration set by user
	OSTime				value;		// time remaining before timer fires
	OSMesgQueue *		mq;			// Message Queue
	OSMesg				msg;		// Message to send
} OSTimer;


/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
//              Global Definitions                 //
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////

//
// Thread states
//
#define OS_STATE_STOPPED	1
#define OS_STATE_RUNNABLE	2
#define OS_STATE_RUNNING	4
#define OS_STATE_WAITING	8

//
// Events
//
#define OS_NUM_EVENTS           23

#define OS_EVENT_SW1              0     // CPU SW1 interrupt
#define OS_EVENT_SW2              1     // CPU SW2 interrupt
#define OS_EVENT_CART             2     // Cartridge interrupt: used by rmon
#define OS_EVENT_COUNTER          3     // Counter int: used by VI/Timer Mgr
#define OS_EVENT_SP               4     // SP task done interrupt
#define OS_EVENT_SI               5     // SI (controller) interrupt
#define OS_EVENT_AI               6     // AI interrupt
#define OS_EVENT_VI               7     // VI interrupt: used by VI/Timer Mgr
#define OS_EVENT_PI               8     // PI interrupt: used by PI Manager
#define OS_EVENT_DP               9     // DP full sync interrupt
#define OS_EVENT_CPU_BREAK        10    // CPU breakpoint: used by rmon
#define OS_EVENT_SP_BREAK         11    // SP breakpoint:  used by rmon
#define OS_EVENT_FAULT            12    // CPU fault event: used by rmon
#define OS_EVENT_THREADSTATUS     13    // CPU thread status: used by rmon
#define OS_EVENT_PRENMI           14    // Pre NMI interrupt
#define OS_EVENT_RDB_READ_DONE    15    // RDB read ok event: used by rmon
#define OS_EVENT_RDB_LOG_DONE     16    // read of log data complete
#define OS_EVENT_RDB_DATA_DONE    17    // read of hostio data complete
#define OS_EVENT_RDB_REQ_RAMROM   18    // host needs ramrom access
#define OS_EVENT_RDB_FREE_RAMROM  19    // host is done with ramrom access
#define OS_EVENT_RDB_DBG_DONE     20
#define OS_EVENT_RDB_FLUSH_PROF   21
#define OS_EVENT_RDB_ACK_PROF     22

//
// Flags for debugging purpose
//
#define	OS_FLAG_CPU_BREAK	1	// Break exception has occurred
#define	OS_FLAG_FAULT		2	// CPU fault has occurred


//
// Interrupt masks
//
#define	OS_IM_NONE		0x00000001
#define	OS_IM_SW1		0x00000501
#define	OS_IM_SW2		0x00000601
#define	OS_IM_CART		0x00000c01
#define	OS_IM_PRENMI	0x00001401
#define OS_IM_RDBWRITE	0x00002401
#define OS_IM_RDBREAD	0x00004401
#define	OS_IM_COUNTER	0x00008401
#define	OS_IM_CPU		0x0000ff01
#define	OS_IM_SP		0x00010401
#define	OS_IM_SI		0x00020401
#define	OS_IM_AI		0x00040401
#define	OS_IM_VI		0x00080401
#define	OS_IM_PI		0x00100401
#define	OS_IM_DP		0x00200401
#define	OS_IM_ALL		0x003fff01
#define	RCP_IMASK		0x003f0000
#define	RCP_IMASKSHIFT	16

//
// Recommended thread priorities for the system threads
//
#define OS_PRIORITY_MAX			255
#define OS_PRIORITY_VIMGR		254
#define OS_PRIORITY_RMON		250
#define OS_PRIORITY_RMONSPIN	200
#define OS_PRIORITY_PIMGR		150
#define OS_PRIORITY_SIMGR		140
#define	OS_PRIORITY_APPMAX		127
#define OS_PRIORITY_IDLE		0	// Must be 0

//
// Flags to turn blocking on/off when sending/receiving message
//
#define	OS_MESG_NOBLOCK		0
#define	OS_MESG_BLOCK		1

//
// Flags to indicate direction of data transfer
//
#define	OS_READ			0		// device -> RDRAM
#define	OS_WRITE		1		// device <- RDRAM
#define	OS_OTHERS		2		// for Leo disk only

//
// I/O message types
//
#define OS_MESG_TYPE_BASE		(10)
#define OS_MESG_TYPE_LOOPBACK	(OS_MESG_TYPE_BASE+0)
#define OS_MESG_TYPE_DMAREAD	(OS_MESG_TYPE_BASE+1)
#define OS_MESG_TYPE_DMAWRITE	(OS_MESG_TYPE_BASE+2)
#define OS_MESG_TYPE_VRETRACE	(OS_MESG_TYPE_BASE+3)
#define OS_MESG_TYPE_COUNTER	(OS_MESG_TYPE_BASE+4)
#define OS_MESG_TYPE_EDMAREAD	(OS_MESG_TYPE_BASE+5)
#define OS_MESG_TYPE_EDMAWRITE	(OS_MESG_TYPE_BASE+6)

//
// I/O message priority
//
#define OS_MESG_PRI_NORMAL	0
#define OS_MESG_PRI_HIGH	1

//
// Page size argument for TLB routines
//
#define OS_PM_4K	0x0000000
#define OS_PM_16K	0x0006000
#define OS_PM_64K	0x001e000
#define OS_PM_256K	0x007e000
#define OS_PM_1M	0x01fe000
#define OS_PM_4M	0x07fe000
#define OS_PM_16M	0x1ffe000

/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
//                 Controllers                     //
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////

// controller errors
#define CONT_NO_RESPONSE_ERROR          0x8
#define CONT_OVERRUN_ERROR              0x4

// Controller type

#define CONT_ABSOLUTE           0x0001
#define CONT_RELATIVE           0x0002
#define CONT_JOYPORT            0x0004
#define CONT_EEPROM				0x8000
#define CONT_EEP16K				0x4000
#define	CONT_TYPE_MASK			0x1f07
#define	CONT_TYPE_NORMAL		0x0005
#define	CONT_TYPE_MOUSE			0x0002

// EEPROM TYPE

#define EEPROM_TYPE_NONE		0x00
#define EEPROM_TYPE_4K			0x01
#define EEPROM_TYPE_16K			0x02

// Controller status

#define CONT_CARD_ON            0x01
#define CONT_CARD_PULL          0x02
#define CONT_ADDR_CRC_ER        0x04
#define CONT_EEPROM_BUSY		0x80

// Buttons

#define CONT_A      0x8000
#define CONT_B      0x4000
#define CONT_G	    0x2000
#define CONT_START  0x1000
#define CONT_UP     0x0800
#define CONT_DOWN   0x0400
#define CONT_LEFT   0x0200
#define CONT_RIGHT  0x0100
#define CONT_L      0x0020
#define CONT_R      0x0010
#define CONT_E      0x0008
#define CONT_D      0x0004
#define CONT_C      0x0002
#define CONT_F      0x0001

// Nintendo's official button names

#define A_BUTTON	CONT_A
#define B_BUTTON	CONT_B
#define L_TRIG		CONT_L
#define R_TRIG		CONT_R
#define Z_TRIG		CONT_G
#define START_BUTTON	CONT_START
#define U_JPAD		CONT_UP
#define L_JPAD		CONT_LEFT
#define R_JPAD		CONT_RIGHT
#define D_JPAD		CONT_DOWN
#define U_CBUTTONS	CONT_E
#define L_CBUTTONS	CONT_C
#define R_CBUTTONS	CONT_F
#define D_CBUTTONS	CONT_D

#define OS_TV_PAL		0
#define OS_TV_NTSC		1
#define OS_TV_MPAL		2

#endif //__ULTRA_OS__
