/* ***** BEGIN LICENSE BLOCK ***** 
 * Version: RCSL 1.0/RPSL 1.0 
 *  
 * Portions Copyright (c) 1995-2002 RealNetworks, Inc. All Rights Reserved. 
 *      
 * The contents of this file, and the files included with this file, are 
 * subject to the current version of the RealNetworks Public Source License 
 * Version 1.0 (the "RPSL") available at 
 * http://www.helixcommunity.org/content/rpsl unless you have licensed 
 * the file under the RealNetworks Community Source License Version 1.0 
 * (the "RCSL") available at http://www.helixcommunity.org/content/rcsl, 
 * in which case the RCSL will apply. You may also obtain the license terms 
 * directly from RealNetworks.  You may not use this file except in 
 * compliance with the RPSL or, if you have a valid RCSL with RealNetworks 
 * applicable to this file, the RCSL.  Please see the applicable RPSL or 
 * RCSL for the rights, obligations and limitations governing use of the 
 * contents of the file.  
 *  
 * This file is part of the Helix DNA Technology. RealNetworks is the 
 * developer of the Original Code and owns the copyrights in the portions 
 * it created. 
 *  
 * This file, and the files included with this file, is distributed and made 
 * available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER 
 * EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS ALL SUCH WARRANTIES, 
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS 
 * FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT. 
 * 
 * Technology Compatibility Kit Test Suite(s) Location: 
 *    http://www.helixcommunity.org/content/tck 
 * 
 * Contributor(s): 
 *  
 * ***** END LICENSE BLOCK ***** */ 
#ifndef _CSTRMSRT_H_
#define _CSTRMSRT_H_

/****************************************************************************
 *  Defines
 */
#define ATOMIZE_ALL	0

/****************************************************************************
 *  Includes
 */
#include "hxcom.h"
#include "hxtypes.h"
#include "hxslist.h"
#include "ihxpckts.h"

// Defines
#define HX_STREAM_MERGE_SORTER_QUEUE_DEPTH_LIMIT_DEFAULT          1000
#define HX_STREAM_MERGE_SORTER_QUEUE_TIMESPAN_LIMIT_DEFAULT       5000
#define HX_STREAM_MERGE_SORTER_QUEUE_BYTE_LIMIT_DEFAULT     0xFFFFFFFF


/****************************************************************************
 * 
 *  Class:
 *	CStreamMergeSorter
 *
 *  Purpose:
 *	Merge Sorts streams
 *
 */
class CStreamMergeSorter
{
public:
    /*
     *	Constructor/Destructor
     */
    CStreamMergeSorter(void);
    ~CStreamMergeSorter();

    /*
     *	Main Interface
     */
    HX_RESULT Init(ULONG32 ulNumStreams,
		   ULONG32 ulQueueDepthLimit = HX_STREAM_MERGE_SORTER_QUEUE_DEPTH_LIMIT_DEFAULT,
		   LONG32 lQueueTimespanLimit = HX_STREAM_MERGE_SORTER_QUEUE_TIMESPAN_LIMIT_DEFAULT,
                   ULONG32 ulQueueByteLimit = HX_STREAM_MERGE_SORTER_QUEUE_BYTE_LIMIT_DEFAULT);

    HX_RESULT SetPacket(IHXPacket* pPacket);

    HX_RESULT GetPacket(IHXPacket* &pPacket, HXBOOL bRemove = TRUE, HXBOOL* pDepthLimit = NULL,
                        HXBOOL* pTimespanLimit = NULL, HXBOOL* pByteLimit = NULL);

    HX_RESULT GetPacketTime(ULONG32& ulTime)
    {
	IHXPacket* pPacket = NULL;
	HX_RESULT retVal = GetPacket(pPacket, FALSE);

	if ((retVal == HXR_OK) && pPacket)
	{
	    ulTime = pPacket->GetTime();
	    pPacket->Release();
	}

	return retVal;
    }

    void Reset();

    HX_RESULT Terminate(ULONG32 ulStreamNum);
    HXBOOL    IsTerminated(ULONG32 ulStreamNum);
    UINT32    NumPacketsQueued(ULONG32 ulStreamNum);

    UINT32 GetStreamCount() { return m_ulNumStreams; }

private:
    class StreamBuffer
    {
    public:
	StreamBuffer(void)
	    : m_ulFirstTS(0)
	    , m_ulLastTS(0)
            , m_ulQueuedBytes(0)
	    , m_bTerminated(FALSE)
	    , m_bFirstTSSet(FALSE)
	{
	    ;
	}

	~StreamBuffer(void)
	{
            ClearPacketQueue();
	}

	HX_RESULT Enqueue(IHXPacket* pPacket)
	{
	    if (!pPacket->IsLost())
	    {
		if (m_Buffer.IsEmpty() || (!m_bFirstTSSet))
		{
		    m_ulFirstTS = pPacket->GetTime();
		    m_bFirstTSSet = TRUE;
		}
		m_ulLastTS = pPacket->GetTime();
                // Add the size of this packet buffer
                IHXBuffer* pBuffer = pPacket->GetBuffer();
                if (pBuffer)
                {
                    m_ulQueuedBytes += pBuffer->GetSize();
                    pBuffer->Release();
                }
	    }
	    pPacket->AddRef();
	    m_Buffer.AddTail(pPacket);

	    return HXR_OK;
	}

	IHXPacket* Dequeue(void)
	{
	    IHXPacket* pPacket = NULL;

	    if (!m_Buffer.IsEmpty())
	    {
		pPacket = (IHXPacket*) m_Buffer.RemoveHead();

		if (!m_Buffer.IsEmpty())
		{
		    IHXPacket* pNextPacket = (IHXPacket*) m_Buffer.GetHead();

		    if (!pNextPacket->IsLost())
		    {
			m_ulFirstTS = pNextPacket->GetTime();
		    }
		}
		else
		{
		    m_ulFirstTS = m_ulLastTS;
		}
                // Subtract the size of the packet's buffer
                if (pPacket && !pPacket->IsLost())
                {
                    IHXBuffer* pBuffer = pPacket->GetBuffer();
                    if (pBuffer)
                    {
                        UINT32 ulBufferSize = pBuffer->GetSize();
                        m_ulQueuedBytes     = ((m_ulQueuedBytes >= ulBufferSize) ?
                                               (m_ulQueuedBytes  - ulBufferSize) : 0);
                        pBuffer->Release();
                    }
                }
	    }

	    return pPacket;
	}

	IHXPacket* Peek(void)
	{
	    IHXPacket* pPacket = NULL;

	    if (!m_Buffer.IsEmpty())
	    {
		pPacket = (IHXPacket*) m_Buffer.GetHead();
	    }

	    return pPacket;
	}

        void Reset(void)
        {
            ClearPacketQueue();
	    m_ulFirstTS     = 0;
	    m_ulLastTS      = 0;
            m_ulQueuedBytes = 0;
	    m_bTerminated   = FALSE;
	    m_bFirstTSSet   = FALSE;
        }

	void Terminate(void)	{ m_bTerminated = TRUE; }

	ULONG32 GetTime(void)	{ return m_ulFirstTS; }
	ULONG32 GetEndTime(void){ return m_ulLastTS; }
	LONG32 GetTimespan(void)   { return m_ulLastTS - m_ulFirstTS; }
	ULONG32 GetDepth(void)	{ return m_Buffer.GetCount(); }
        ULONG32 GetBytes(void)  { return m_ulQueuedBytes; }
	HXBOOL IsTerminated(void)	{ return m_bTerminated; }

    private:
	CHXSimpleList m_Buffer;
	ULONG32 m_ulFirstTS;
	ULONG32 m_ulLastTS;
        ULONG32 m_ulQueuedBytes;
	HXBOOL m_bTerminated;
	HXBOOL m_bFirstTSSet;

        void ClearPacketQueue()
        {
	    while (!m_Buffer.IsEmpty())
	    {
		IHXPacket* pPacket = (IHXPacket*) m_Buffer.RemoveHead();
                HX_RELEASE(pPacket);
	    }
        }
    };
    
    ULONG32 m_ulNumStreams;
    ULONG32 m_ulQueueDepthLimit;
    LONG32  m_lQueueTimespanLimit;
    ULONG32 m_ulQueueByteLimit;
    StreamBuffer* m_pStreamBuffers;
};

#endif  // _CSTRMSRT_H_
