#ifndef OSMESGQUEUE_H_
#define OSMESGQUEUE_H_

#include "ultra_os.h"

#include "Core/Memory.h"

#include <stddef.h>		// ofsetof

/*
typedef struct OSMesgQueue_s {
	OSThread	*mtqueue;	// Queue to store threads blocked on empty mailboxes (receive) 
	OSThread	*fullqueue;	// Queue to store threads blocked on full mailboxes (send) 
	s32			validCount;	// Contains number of valid message
	s32			first;		// Points to first valid message
	s32			msgCount;	// Contains total # of messages 
	OSMesg		*msg;		// Points to message buffer array 
} OSMesgQueue;
*/

class COSMesgQueue
{
public:

	COSMesgQueue(u32 dwQueueAddress)
	{
		m_dwBaseAddress = dwQueueAddress;
	}

	// OSThread
	u32 GetEmptyQueue()
	{
		return Read32Bits(m_dwBaseAddress + offsetof(OSMesgQueue, mtqueue));
	}

	u32 GetFullQueue()
	{
		return Read32Bits(m_dwBaseAddress + offsetof(OSMesgQueue, fullqueue));
	}

	s32 GetValidCount()
	{
		return Read32Bits(m_dwBaseAddress + offsetof(OSMesgQueue, validCount));
	}

	s32 GetFirst()
	{
		return Read32Bits(m_dwBaseAddress + offsetof(OSMesgQueue, first));
	}

	s32 GetMsgCount()
	{
		return Read32Bits(m_dwBaseAddress + offsetof(OSMesgQueue, msgCount));
	}

	u32 GetMesgArray()
	{
		return Read32Bits(m_dwBaseAddress + offsetof(OSMesgQueue, msg));
	}

	void SetEmptyQueue(u32 queue)
	{
		Write32Bits(m_dwBaseAddress + offsetof(OSMesgQueue, mtqueue), queue);
	}

	void SetFullQueue(u32 queue)
	{
		Write32Bits(m_dwBaseAddress + offsetof(OSMesgQueue, fullqueue), queue);
	}

	void SetValidCount(s32 nCount)
	{
		Write32Bits(m_dwBaseAddress + offsetof(OSMesgQueue, validCount), nCount);
	}

	void SetFirst(s32 nFirst)
	{
		Write32Bits(m_dwBaseAddress + offsetof(OSMesgQueue, first), nFirst);
	}

	void SetMsgCount(s32 nMsgCount)
	{
		Write32Bits(m_dwBaseAddress + offsetof(OSMesgQueue, msgCount), nMsgCount);
	}
	
	void SetMesgArray(u32 dwMsgBuffer)
	{
		Write32Bits(m_dwBaseAddress + offsetof(OSMesgQueue, msg), dwMsgBuffer);
	}	


protected:
	u32 m_dwBaseAddress;

};

#endif // OSMESGQUEUE_H_
