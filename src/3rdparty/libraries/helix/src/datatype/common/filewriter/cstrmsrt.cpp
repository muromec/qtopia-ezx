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
/****************************************************************************
 *  Defines
 */
#define NO_STREAM_SET	0xFFFFFFFF


/****************************************************************************
 *  Includes
 */
#include "cstrmsrt.h"


/****************************************************************************
 *  Class CStreamMergeSorter
 */
/****************************************************************************
 *  Constructor/Destructor
 */
CStreamMergeSorter::CStreamMergeSorter(void)
    : m_pStreamBuffers(NULL)
    , m_ulNumStreams(0)
    , m_ulQueueDepthLimit(0)
    , m_lQueueTimespanLimit(0)
{
    ;
}


CStreamMergeSorter::~CStreamMergeSorter()
{
    HX_VECTOR_DELETE(m_pStreamBuffers);
}


/****************************************************************************
 *  Main Interface 
 */
/****************************************************************************
 *  Init
 */
HX_RESULT CStreamMergeSorter::Init(ULONG32 ulNumStreams,
				   ULONG32 ulQueueDepthLimit,
				   LONG32 lQueueTimespanLimit)
{
    HX_RESULT retVal = HXR_INVALID_PARAMETER;

    HX_ASSERT(ulNumStreams > 0);

    if (ulNumStreams > 0)
    {
	retVal = HXR_OK;
    }

    if (SUCCEEDED(retVal))
    {
	HX_VECTOR_DELETE(m_pStreamBuffers);

	m_ulNumStreams = ulNumStreams;
	m_ulQueueDepthLimit = ulQueueDepthLimit;
	m_lQueueTimespanLimit = lQueueTimespanLimit;

	retVal = HXR_OUTOFMEMORY;
	m_pStreamBuffers = new StreamBuffer [ulNumStreams];
	if (m_pStreamBuffers)
	{
	    retVal = HXR_OK;
	}
    }

    return retVal;
}


/****************************************************************************
 *  SetPacket
 */
HX_RESULT CStreamMergeSorter::SetPacket(IHXPacket* pPacket)
{
    HX_RESULT retVal = HXR_UNEXPECTED;
    ULONG32 ulStreamNum = pPacket->GetStreamNumber();

    if (m_pStreamBuffers)
    {
	retVal = HXR_INVALID_PARAMETER;

	HX_ASSERT(ulStreamNum < m_ulNumStreams);

	if (ulStreamNum < m_ulNumStreams)
	{
	    retVal = m_pStreamBuffers[ulStreamNum].Enqueue(pPacket);
	}
    }

    return retVal;
}
    

/****************************************************************************
 *  GetPacket
 */
HX_RESULT CStreamMergeSorter::GetPacket(IHXPacket* &pPacket, HXBOOL bRemove)
{
    ULONG32 ulQueueDepth;
    ULONG32 ulTime;
    ULONG32 ulEndTime;
    ULONG32 ulMinTS = 0;
    ULONG32 ulMaxTS = 0;
    ULONG32 ulNextStreamIdx = NO_STREAM_SET;
    ULONG32 ulTotQueueDepth = 0;
    LONG32 lTotQueueTimespan = 0;
    ULONG32 ulEmptyQueueCount = 0;
    ULONG32 ulIdx = 0;
    HX_RESULT retVal = HXR_UNEXPECTED;

    if (m_pStreamBuffers)
    {
	retVal = HXR_NO_DATA;

	do
	{
	    ulQueueDepth = m_pStreamBuffers[ulIdx].GetDepth();

	    if (ulQueueDepth > 0)
	    {
		ulTotQueueDepth += ulQueueDepth;

		if (ulTotQueueDepth == ulQueueDepth)
		{
		    ulMinTS = m_pStreamBuffers[ulIdx].GetTime();
		    ulMaxTS = m_pStreamBuffers[ulIdx].GetEndTime();
		}
		else
		{
		    ulTime = m_pStreamBuffers[ulIdx].GetTime();
		    ulEndTime = m_pStreamBuffers[ulIdx].GetEndTime();

		    if (((LONG32) (ulMinTS - ulTime)) > 0)
		    {
			ulMinTS = ulTime;
		    }

		    if (((LONG32) (ulEndTime - ulMaxTS)) > 0)
		    {
			ulMaxTS = ulEndTime;
		    }
		}
		
		if ((ulNextStreamIdx == NO_STREAM_SET) ||
		    (((LONG32) (m_pStreamBuffers[ulNextStreamIdx].GetTime() -
				m_pStreamBuffers[ulIdx].GetTime())) > 0))
		{
		    ulNextStreamIdx = ulIdx;
		}
	    }
	    else if (!m_pStreamBuffers[ulIdx].IsTerminated())
	    {
		ulEmptyQueueCount++;
	    }

	    ulIdx++;
	} while (ulIdx < m_ulNumStreams);
	
	lTotQueueTimespan = (LONG32) (ulMaxTS - ulMinTS);

	if (ulNextStreamIdx != NO_STREAM_SET)
	{
	    if ((ulEmptyQueueCount == 0) ||
		(lTotQueueTimespan > m_lQueueTimespanLimit) ||
		(ulTotQueueDepth > m_ulQueueDepthLimit))
	    {
		if (bRemove)
		{
		    pPacket = m_pStreamBuffers[ulNextStreamIdx].Dequeue();
		}
		else
		{
		    pPacket = m_pStreamBuffers[ulNextStreamIdx].Peek();
		    if (pPacket)
		    {
			pPacket->AddRef();
		    }
		}

		retVal = HXR_OK;
	    }
	}
	else if (ulEmptyQueueCount == 0)
	{
	    retVal = HXR_STREAM_DONE;
	}
    }

    return retVal;
}

/****************************************************************************
 *  Flush
 */
HX_RESULT CStreamMergeSorter::Terminate(ULONG32 ulStreamNum)
{
    HX_RESULT retVal = HXR_UNEXPECTED;

    if (m_pStreamBuffers)
    {
	retVal = HXR_INVALID_PARAMETER;

	HX_ASSERT(ulStreamNum < m_ulNumStreams);

	if (ulStreamNum < m_ulNumStreams)
	{
	    m_pStreamBuffers[ulStreamNum].Terminate();
	    retVal = HXR_OK;
	}
    }

    return retVal;
}
