/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: recordctl.cpp,v 1.25 2008/06/26 05:52:59 qluo Exp $
 * 
 * Portions Copyright (c) 1995-2004 RealNetworks, Inc. All Rights Reserved.
 * 
 * The contents of this file, and the files included with this file,
 * are subject to the current version of the RealNetworks Public
 * Source License (the "RPSL") available at
 * http://www.helixcommunity.org/content/rpsl unless you have licensed
 * the file under the current version of the RealNetworks Community
 * Source License (the "RCSL") available at
 * http://www.helixcommunity.org/content/rcsl, in which case the RCSL
 * will apply. You may also obtain the license terms directly from
 * RealNetworks.  You may not use this file except in compliance with
 * the RPSL or, if you have a valid RCSL with RealNetworks applicable
 * to this file, the RCSL.  Please see the applicable RPSL or RCSL for
 * the rights, obligations and limitations governing use of the
 * contents of the file.
 * 
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU General Public License Version 2 (the
 * "GPL") in which case the provisions of the GPL are applicable
 * instead of those above. If you wish to allow use of your version of
 * this file only under the terms of the GPL, and not to allow others
 * to use your version of this file under the terms of either the RPSL
 * or RCSL, indicate your decision by deleting the provisions above
 * and replace them with the notice and other provisions required by
 * the GPL. If you do not delete the provisions above, a recipient may
 * use your version of this file under the terms of any one of the
 * RPSL, the RCSL or the GPL.
 * 
 * This file is part of the Helix DNA Technology. RealNetworks is the
 * developer of the Original Code and owns the copyrights in the
 * portions it created.
 * 
 * This file, and the files included with this file, is distributed
 * and made available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY
 * KIND, EITHER EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS
 * ALL SUCH WARRANTIES, INCLUDING WITHOUT LIMITATION, ANY WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET
 * ENJOYMENT OR NON-INFRINGEMENT.
 * 
 * Technology Compatibility Kit Test Suite(s) Location:
 *    http://www.helixcommunity.org/content/tck
 * 
 * Contributor(s):
 * 
 * ***** END LICENSE BLOCK ***** */

#include "hxtypes.h"
#include "hxcom.h"
#include "ihxpckts.h"
#include "hxrecord.h"
#include "hxasm.h"
#include "hxplayvelocity.h"
#include "hxprefs.h"
#include "velproxy.h"
#include "hxtlogutil.h"
#include "recordctl.h"
#include "hxprefutil.h"
#if defined(HELIX_FEATURE_RECORDCONTROL_MERGESORT)
#include "cstrmsrt.h"
#endif /* #if defined(HELIX_FEATURE_RECORDCONTROL_MERGESORT) */

#include "hxheap.h"

#if defined(HELIX_FEATURE_RECORDCONTROL_MERGESORT)
#define DEFAULT_MERGESORT_DISABLED          FALSE
#else /* #if defined(HELIX_FEATURE_RECORDCONTROL_MERGESORT) */
#define DEFAULT_MERGESORT_DISABLED          TRUE
#endif /* #if defined(HELIX_FEATURE_RECORDCONTROL_MERGESORT) */
#define DEFAULT_MERGESORT_TIMESPAN_LIMIT    2000 // in millseconds
#define DEFAULT_MERGESORT_QUEUE_DEPTH_LIMIT 1000 // in number of packets

#ifdef _DEBUG
#undef HX_THIS_FILE             
static const char HX_THIS_FILE[] = __FILE__;
#endif

HXRecordControl::HXRecordControl(IUnknown* pUnkPlayer, IUnknown* pUnkSource) :
    m_lRefCount(0),
    m_pRecordSource(NULL),
    m_pRecordService(NULL),
    m_ulNumStreams(0),
    m_pStreamInfo(NULL),
    m_bFirstTimestampWritten(FALSE),
    m_ulLatestTimestampWritten(0),
    m_bFirstTimestampRead(FALSE),
    m_ulLatestTimestampRead(0),
    m_bFinishedWriting(FALSE),
    m_bFinishedReading(FALSE),
    m_bDisableRSVelocity(FALSE),
    m_bDisableRSMergeSort(DEFAULT_MERGESORT_DISABLED),
    m_ulRSTimeSpanLimit(DEFAULT_MERGESORT_TIMESPAN_LIMIT),
    m_ulRSQueueDepthLimit(DEFAULT_MERGESORT_QUEUE_DEPTH_LIMIT),
    m_bCanGetPackets(FALSE),
    m_lTimeOffset(0)
#if defined(HELIX_FEATURE_RECORDCONTROL_MERGESORT)
    , m_pMergeSorter(NULL)
#endif /* #if defined(HELIX_FEATURE_RECORDCONTROL_MERGESORT) */
{
    ReadRecordSourcePrefs(pUnkPlayer);
    SPIHXRecordManager spRecordManager = pUnkPlayer;
    if(spRecordManager.IsValid())
	spRecordManager->GetRecordService(m_pRecordService);
    if(m_pRecordService)
	m_pRecordService->CreateRecordSource(pUnkSource, m_pRecordSource);

    if(m_pRecordSource)
    {
#if defined(HELIX_FEATURE_PLAYBACK_VELOCITY)
        if (!m_bDisableRSVelocity)
        {
            CHXPlaybackVelocityProxyRS* pProxyRS = new CHXPlaybackVelocityProxyRS(pUnkPlayer, m_pRecordSource);
            if (pProxyRS)
            {
                IHXRecordSource* pRecordSource = NULL;
                HX_RESULT rv = pProxyRS->QueryInterface(IID_IHXRecordSource, (void**) &pRecordSource);
                if (SUCCEEDED(rv))
                {
                    HX_RELEASE(m_pRecordSource);
                    m_pRecordSource = pRecordSource;
                    m_pRecordSource->AddRef();
                }
                HX_RELEASE(pRecordSource);
                // Tell the proxy whether or not to expect
                // the packets coming in OnPacket() to be
                // sorted by timestamp or not
                pProxyRS->SetExpectSortedOnPacketFlag(!m_bDisableRSMergeSort);
            }
        }
#endif /* #if defined(HELIX_FEATURE_PLAYBACK_VELOCITY) */
	if(m_pRecordSource->SetFormatResponse(this) == HXR_OK)
	    m_bCanGetPackets = TRUE;
    }
}

HXRecordControl::~HXRecordControl()
{
    Cleanup();
}

STDMETHODIMP 
HXRecordControl::QueryInterface(REFIID riid, void** ppvObj)
{
    QInterfaceList qiList[] =
        {
            { GET_IIDHANDLE(IID_IHXFormatResponse), (IHXFormatResponse*)this },
            { GET_IIDHANDLE(IID_IUnknown), (IUnknown*)(IHXFormatResponse*)this },
        };

    return ::QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
}

STDMETHODIMP_(ULONG32) 
HXRecordControl::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32) 
HXRecordControl::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
	return m_lRefCount;
    }

    delete this;
    return 0;
}


void
HXRecordControl::Cleanup()
{
    OnEndOfPackets();

    for(UINT16 nStream = 0; nStream < m_PendingGetPackets.GetSize(); nStream++)
    {
	IHXPacket* pPacket = (IHXPacket*)m_PendingGetPackets.GetAt(nStream);
	m_PendingGetPackets.SetAt(nStream, NULL);
	HX_RELEASE(pPacket);
    }

    if(m_pRecordService)
	m_pRecordService->CloseRecordSource(m_pRecordSource);

    HX_RELEASE(m_pRecordService);
    HX_RELEASE(m_pRecordSource);
    HX_VECTOR_DELETE(m_pStreamInfo);
#if defined(HELIX_FEATURE_RECORDCONTROL_MERGESORT)
    HX_DELETE(m_pMergeSorter);
#endif /* #if defined(HELIX_FEATURE_RECORDCONTROL_MERGESORT) */

    while(!m_PendingPutPackets.IsEmpty())
    {
	PendingPutPacket* pPutPacket = (PendingPutPacket*)m_PendingPutPackets.RemoveHead();
	HX_ASSERT(pPutPacket);
	HX_RELEASE(pPutPacket->pPacket);
	HX_DELETE(pPutPacket);
    }
}

STDMETHODIMP	
HXRecordControl::PacketReady(HX_RESULT status, IHXPacket* pPacket)
{
    HX_RESULT retVal = HXR_UNEXPECTED;

    if(pPacket && m_pStreamInfo)
    {
        UINT16 usStreamNum = pPacket->GetStreamNumber();
	if(usStreamNum < m_PendingGetPackets.GetSize())
        {
            HX_ASSERT(!m_PendingGetPackets.GetAt(usStreamNum));
            pPacket->AddRef();
            m_PendingGetPackets.SetAt(pPacket->GetStreamNumber(), pPacket);
            retVal = HXR_OK;
        }
    }

    return retVal;
}

HX_RESULT
HXRecordControl::Seek(ULONG32 seekTime)
{
    HXLOGL4(HXLOG_TRIK, "HXRecordControl Seek(%lu)", seekTime);
    if (!m_pRecordSource)
    {
	return HXR_NOT_INITIALIZED;
    }
	
    m_bFirstTimestampRead = FALSE;
    m_ulLatestTimestampRead = 0;

    HX_RESULT nResult = HXR_FAILED;
    if(m_bCanGetPackets)
    {
	nResult = m_pRecordSource->Seek(seekTime);
	if(nResult == HXR_OK)
	{
	    for(UINT16 nStream = 0; nStream < m_PendingGetPackets.GetSize(); nStream++)
	    {
		IHXPacket* pPacket = (IHXPacket*)m_PendingGetPackets.GetAt(nStream);
		m_PendingGetPackets.SetAt(nStream, NULL);
		HX_RELEASE(pPacket);
		
		// If seek succeeded the first GetPacket() for each stream should not fail. 
		// In case of metafiles though no packets should be send to renderers again.
		// Therefore GetPacket() fails in case of metafiles. SB.
		m_pRecordSource->GetPacket(nStream);
	    }
	}
    }

    if(nResult != HXR_OK)
    {
        // reset internal states when record source is flushed(actual seek is issued)
        m_bFinishedReading = FALSE;
        m_bFinishedWriting = FALSE;
        for (UINT32 i = 0; i < m_ulNumStreams; i++)
        {
            m_pStreamInfo[i].m_bStreamDone = FALSE;
        }

	m_pRecordSource->Flush();

	for(UINT16 nStream = 0; nStream < m_PendingGetPackets.GetSize(); nStream++)
	{
	    IHXPacket* pPacket = (IHXPacket*)m_PendingGetPackets.GetAt(nStream);
	    m_PendingGetPackets.SetAt(nStream, NULL);
	    HX_RELEASE(pPacket);
	}
    }

    return nResult;
}

HX_RESULT	
HXRecordControl::GetPacket(UINT16 usStreamNumber, IHXPacket*& pPacket)
{
    HXLOGL4(HXLOG_TRIK, "HXRecordControl GetPacket(%u,)", usStreamNumber);
    pPacket = NULL;

    if(!m_pRecordSource || !m_bCanGetPackets)
	return HXR_FAILED;

    if (m_bFinishedReading || m_PendingGetPackets.IsEmpty())
        return HXR_NO_DATA;
    
    HX_RESULT nResult = HXR_OK;
    pPacket = (IHXPacket*)m_PendingGetPackets.GetAt(usStreamNumber);

    if (!pPacket)
    {
	nResult = m_pRecordSource->GetPacket(usStreamNumber);
	if (nResult == HXR_OK)
	{
	    pPacket = (IHXPacket*)m_PendingGetPackets.GetAt(usStreamNumber);
	    if (!pPacket)
	    {
		nResult = HXR_RETRY;
	    }
	}
	
	if ((nResult != HXR_OK) &&	    // OK means we got packet
	    (nResult != HXR_RETRY) &&	    // RETRY means no packet but are encouraged to try again immediately
	    m_bFinishedWriting &&
	    (nResult != HXR_WOULD_BLOCK))   // HXR_WOULD_BLOCK means no packet and are encouraged to try a bit later
	{
	    // We did not get a packet and no more packet will be written into the
	    // record control.
	    // It is time to check if we have any packets left in record source.
	    UINT16 uStrmIdx = 0;
	    HXBOOL bRecordSourceDone = TRUE;
	    
	    for (uStrmIdx = 0; uStrmIdx < m_PendingGetPackets.GetSize(); uStrmIdx++)
	    {
		if (uStrmIdx != usStreamNumber)
		{
		    if (m_PendingGetPackets.GetAt(uStrmIdx))
		    {
			// Found a packet - not done yet
			bRecordSourceDone = FALSE;
			break;
		    }
		
		    if (m_pRecordSource->GetPacket(uStrmIdx) == HXR_OK)
		    {
			// Stream still accepting packet requests - not done yet
			bRecordSourceDone = FALSE;
			break;
		    }
		}
	    }
	    
	    if (bRecordSourceDone)
	    {
		m_bFinishedReading = TRUE;
		nResult = HXR_NO_DATA;
	    }	    
	}
    }

    m_PendingGetPackets.SetAt(usStreamNumber, NULL);
    
    // we only track the last timestamp of non-Sparse stream
    if (pPacket && !m_pStreamInfo[usStreamNumber].m_bSparseStream)
    { 
	// This is not quite right but best we can do given that offset is not being 
	// passed through the record source.  It works as long as the packet offsets 
	// are not changing within the source.  Currently, there are no such cases.
	UINT32 ulTime = pPacket->GetTime() - m_lTimeOffset; 

        if (m_bFirstTimestampRead)
        {
            // Save the latest timestamp and handle 32-bit rollover
	    if (((LONG32) (ulTime - m_ulLatestTimestampRead)) > 0)
	    {
                m_ulLatestTimestampRead = ulTime;
            }
        }
        else
        {
            m_ulLatestTimestampRead = ulTime;
            m_bFirstTimestampRead = TRUE;
        }
    }

    return nResult;
}

HX_RESULT
HXRecordControl::OnFileHeader(IHXValues* pValues)
{
    HX_RESULT nResult = HXR_FAILED;
    UINT32 nStreamCount = 0;			
    if(pValues)
	pValues->GetPropertyULONG32("StreamCount", nStreamCount);

    if(nStreamCount)
    {
        // Save the number of streams
        m_ulNumStreams = nStreamCount;
        // Allocate array of HXRecordControlStreamInfo's
        HX_VECTOR_DELETE(m_pStreamInfo);
        m_pStreamInfo = new HXRecordControlStreamInfo [nStreamCount];
        if (m_pStreamInfo)
        {
            // Clear the return value
            nResult = HXR_OK;
            // NULL out the memory
            memset((void*) m_pStreamInfo, 0, nStreamCount * sizeof(HXRecordControlStreamInfo));
            // Clear the flag that says all streams are done
            m_bFinishedWriting = FALSE;
	    m_bFinishedReading = FALSE;
        }
        else
        {
            nResult = HXR_OUTOFMEMORY;
        }
        if (SUCCEEDED(nResult))
        {
            // Allocate the pending packet ptr array
	    m_PendingGetPackets.SetSize(nStreamCount);
	    for(UINT16 nStream = 0; nStream < nStreamCount; nStream++)
	        m_PendingGetPackets.SetAt(nStream, NULL);
#if defined(HELIX_FEATURE_RECORDCONTROL_MERGESORT)
            // If we are merge sorting, then create the
            // merge sorter
            if (!m_bDisableRSMergeSort)
            {
                HX_DELETE(m_pMergeSorter);
                m_pMergeSorter = new CStreamMergeSorter();
                if (m_pMergeSorter)
                {
                    nResult = m_pMergeSorter->Init(nStreamCount,
                                                    m_ulRSQueueDepthLimit,
                                                    (INT32) m_ulRSTimeSpanLimit);
                }
                else
                {
                    nResult = HXR_OUTOFMEMORY;
                }
            }
#endif /* #if defined(HELIX_FEATURE_RECORDCONTROL_MERGESORT) */
        }
    }

    if (SUCCEEDED(nResult))
    {
        if(m_pRecordSource)
	    nResult = m_pRecordSource->OnFileHeader(pValues);

        if(nResult != HXR_OK && 
	   nResult != HXR_RECORD &&
	   nResult != HXR_RECORD_NORENDER)
	    Cleanup();
    }

    return nResult;
}

HX_RESULT
HXRecordControl::OnStreamHeader(IHXValues* pValues)
{
    HX_RESULT   nResult = HXR_FAILED;
    UINT32      ulStreamNumber = 0;
    IHXBuffer*  pMimeTypeBuffer = NULL;

    if(m_pRecordSource)
	nResult = m_pRecordSource->OnStreamHeader(pValues);

    if(nResult != HXR_OK && 
       nResult != HXR_RECORD &&
       nResult != HXR_RECORD_NORENDER)
    {
	Cleanup();
    }
    else
    {
        if (SUCCEEDED(pValues->GetPropertyULONG32("StreamNumber", ulStreamNumber)) &&
            SUCCEEDED(pValues->GetPropertyCString("MimeType", pMimeTypeBuffer)))
        {
            m_pStreamInfo[ulStreamNumber].m_bSparseStream = IsSparseStream((const char*) pMimeTypeBuffer->GetBuffer());
        }
        HX_RELEASE(pMimeTypeBuffer);
    }

    return nResult;
}

HX_RESULT
HXRecordControl::OnPacket(IHXPacket* pPacket, INT32 nTimeOffset)
{
    HXLOGL4(HXLOG_TRIK, "RecordControl OnPacket(0x%08x,%ld) strm=%u pts=%lu rule=%u flags=0x%02x lost=%lu",
            pPacket, nTimeOffset,
            (pPacket ? pPacket->GetStreamNumber() : 0),
            (pPacket ? pPacket->GetTime() : 0),
            (pPacket ? pPacket->GetASMRuleNumber() : 0),
            (pPacket ? pPacket->GetASMFlags() : 0),
            (pPacket ? pPacket->IsLost() : 0));
    HX_RESULT nResult = HXR_UNEXPECTED;

    // Save the time offset
    m_lTimeOffset = nTimeOffset;

#if defined(HELIX_FEATURE_RECORDCONTROL_MERGESORT)
    if (m_bDisableRSMergeSort)
    {
        nResult = WritePacket(pPacket, nTimeOffset);
    }
    else if (m_pMergeSorter)
    {
        // Get the stream number
        UINT32 ulStreamNum = (UINT32) pPacket->GetStreamNumber();
        // Is the packet lost?
        if (pPacket->IsLost())
        {
            // Change the timestamp of the lost
            // packet to the last timestamp seen
            // for that stream
            ChangeLostPacketTimestamp(pPacket,
                                      m_pStreamInfo[ulStreamNum].m_ulLastTimestamp);
        }
        // Save the last timestamp written
        if (!pPacket->IsLost() && m_pStreamInfo && ulStreamNum < m_ulNumStreams)
        {
            m_pStreamInfo[ulStreamNum].m_ulLastTimestamp = pPacket->GetTime();
        }
        // First add the packet to the merge sorter
        nResult = m_pMergeSorter->SetPacket(pPacket);
        if (SUCCEEDED(nResult))
        {
            // Now we try to get packets out until
            // we don't get any more
            nResult = WriteAllAvailablePackets();
        }
    }
#else /* #if defined(HELIX_FEATURE_RECORDCONTROL_MERGESORT) */
    nResult = WritePacket(pPacket, nTimeOffset);
#endif /* #if defined(HELIX_FEATURE_RECORDCONTROL_MERGESORT) */

    if(nResult != HXR_OK)
	Cleanup();

    return nResult;
}

HXBOOL		
HXRecordControl::CanAcceptPackets()
{
    if(!m_pRecordSource)
	return FALSE;

    while(!m_PendingPutPackets.IsEmpty())
    {
	PendingPutPacket* pPutPacket = (PendingPutPacket*)m_PendingPutPackets.GetHead();
	HX_ASSERT(pPutPacket);

	if(SendPacket(pPutPacket->pPacket, pPutPacket->nTimeOffset) == HXR_RETRY)
	    return FALSE;

	HX_RELEASE(pPutPacket->pPacket);
	HX_DELETE(pPutPacket);
	m_PendingPutPackets.RemoveHead();
    }

    return TRUE;
}

void		
HXRecordControl::SetSource(IUnknown* pUnkSource)
{
    if (m_pRecordSource)
    {
	m_pRecordSource->SetSource(pUnkSource);
    }
}

void		
HXRecordControl::OnEndOfPackets()
{
    if (m_pStreamInfo)
    {
	UINT16 uStrmIdx;

	for (uStrmIdx = 0; uStrmIdx < m_ulNumStreams; uStrmIdx++)
	{
	    if (!m_pStreamInfo[uStrmIdx].m_bStreamDone)
	    {
		HandleStreamDone(HXR_OK, uStrmIdx);
	    }
	}
    }
}

HX_RESULT HXRecordControl::GetRecordSource(REF(IHXRecordSource*) rpSource)
{
    HX_RESULT retVal = HXR_FAIL;

    if (m_pRecordSource)
    {
        HX_RELEASE(rpSource);
        rpSource = m_pRecordSource;
        rpSource->AddRef();
        retVal = HXR_OK;
    }

    return retVal;
}

HX_RESULT HXRecordControl::HandleStreamDone(HX_RESULT status, UINT16 usStreamNumber)
{
    HXLOGL3(HXLOG_TRIK, "RecordControl HandleStreamDone(0x%08x,%u)", status, usStreamNumber);
    HX_RESULT retVal = HXR_OK;

    if (!m_pStreamInfo[usStreamNumber].m_bStreamDone)
    {
#if defined(HELIX_FEATURE_RECORDCONTROL_MERGESORT)
	if (!m_bDisableRSMergeSort && m_pMergeSorter)
	{
	    // Tell the merge sorter we are done with this stream
	    m_pMergeSorter->Terminate(usStreamNumber);
	    // Try and write all available packets
	    retVal = WriteAllAvailablePackets();
	}
#endif /* #if defined(HELIX_FEATURE_RECORDCONTROL_MERGESORT) */
	// Set the stream done flag for this stream
	m_pStreamInfo[usStreamNumber].m_bStreamDone = TRUE;
    }

    // See if all the streams are done
    UINT32 i = 0;
    for (i = 0; i < m_ulNumStreams; i++)
    {
        if (!m_pStreamInfo[i].m_bStreamDone)
        {
            break;
        }
    }

    if (i == m_ulNumStreams)
    {
        // We're gotten a stream done from all streams
        // so we're finished writing.
	if (!m_bFinishedWriting)
	{
	    m_bFinishedWriting = TRUE;
	    if (m_pRecordSource)
	    {
		m_pRecordSource->OnEndOfPackets();
	    }
	}
    }

    return retVal;
}

void HXRecordControl::ReadRecordSourcePrefs(IUnknown* pUnkPlayer)
{
    if (pUnkPlayer)
    {
        // Get the IHXPreferences interface
        IHXPreferences* pPrefs = NULL;
        pUnkPlayer->QueryInterface(IID_IHXPreferences, (void**) &pPrefs);
        if (pPrefs)
        {
            // See if we need to disable velocity for record source
            ReadPrefBOOL(pPrefs,
                         "PlaybackVelocity\\DisableRecordSourceVelocity",
                         m_bDisableRSVelocity);
#if defined(HELIX_FEATURE_RECORDCONTROL_MERGESORT)
            // See if we need to disable the merge sort
            ReadPrefBOOL(pPrefs,
                         "DisableRecordSourceMergeSort",
                         m_bDisableRSMergeSort);
#endif /* #if defined(HELIX_FEATURE_RECORDCONTROL_MERGESORT) */
            // Check the merge sort time span limit
            ReadPrefUINT32(pPrefs,
                           "RecordSourceMergeSortTimeSpanLimit",
                           m_ulRSTimeSpanLimit);
            // Check the merge sort queue depth limit
            ReadPrefUINT32(pPrefs,
                           "RecordSourceMergeSortQueueDepthLimit",
                           m_ulRSQueueDepthLimit);
        }
        HX_RELEASE(pPrefs);
    }
}

HX_RESULT HXRecordControl::WritePacket(IHXPacket* pPacket, INT32 lTimeOffset)
{
    HX_RESULT retVal = HXR_UNEXPECTED;

    if (pPacket && m_pRecordSource)
    {
	if(m_PendingPutPackets.GetCount())
	    retVal = HXR_RETRY;
	else
	    retVal = SendPacket(pPacket, lTimeOffset);
	if(retVal == HXR_RETRY)
	{
	    PendingPutPacket* pPutPacket = new PendingPutPacket;
	    if(!pPutPacket)
		return HXR_OUTOFMEMORY;

	    pPutPacket->pPacket = pPacket;
	    pPutPacket->pPacket->AddRef();
	    pPutPacket->nTimeOffset = lTimeOffset;

	    m_PendingPutPackets.AddTail(pPutPacket);

	    retVal = HXR_OK;
	}
    }

    return retVal;
}

HX_RESULT HXRecordControl::SendPacket(IHXPacket* pPacket, INT32 lTimeOffset)
{
    HXLOGL4(HXLOG_TRIK, "RecordControl SendPacket(0x%08x,%ld) strm=%u pts=%lu rule=%u flags=0x%02x",
            pPacket, lTimeOffset,
            (pPacket ? pPacket->GetStreamNumber() : 0),
            (pPacket ? pPacket->GetTime() : 0),
            (pPacket ? pPacket->GetASMRuleNumber() : 0),
            (pPacket ? pPacket->GetASMFlags() : 0));
    HX_RESULT retVal = HXR_UNEXPECTED;

    if (m_pRecordSource)
    {
        UINT32 ulTime = pPacket->GetTime() - lTimeOffset;
        if (m_bFirstTimestampWritten)
        {
            // Save the latest timestamp and handle 32-bit rollover
	    if (((LONG32) (ulTime - m_ulLatestTimestampWritten)) > 0)
	    {
                m_ulLatestTimestampWritten = ulTime;
            }
        }
        else
        {
            m_ulLatestTimestampWritten = ulTime;
            m_bFirstTimestampWritten   = TRUE;
        }
        retVal = m_pRecordSource->OnPacket(pPacket, lTimeOffset);
    }

    return retVal;
}

HX_RESULT HXRecordControl::WriteAllAvailablePackets()
{
    HX_RESULT retVal = HXR_UNEXPECTED;

#if defined(HELIX_FEATURE_RECORDCONTROL_MERGESORT)
    if (m_pMergeSorter)
    {
        // Clear the return value
        retVal = HXR_OK;
        // Now we try to get packets out until
        // we don't get any more
        while (retVal == HXR_OK)
        {
            IHXPacket* pOutPacket = NULL;
            retVal = m_pMergeSorter->GetPacket(pOutPacket, TRUE);
            if (retVal == HXR_OK)
            {
                // Write the packet to the record source
                retVal = WritePacket(pOutPacket, m_lTimeOffset);
            }
            HX_RELEASE(pOutPacket);
        }
        // If we got a HXR_NO_DATA, that just means that
        // the merge sorter needs more data before it can write,
        // so mask that error
        if (retVal == HXR_NO_DATA)
        {
            retVal = HXR_OK;
        }
    }
#endif /* #if defined(HELIX_FEATURE_RECORDCONTROL_MERGESORT) */

    return retVal;
}

void HXRecordControl::ChangeLostPacketTimestamp(IHXPacket* pPacket, UINT32 ulNewTime)
{
    if (pPacket && pPacket->IsLost())
    {
        IHXBuffer* pBuffer      = NULL;
        UINT32     ulOldTime    = 0;
        UINT16     usStreamNum  = 0;
        UINT8      ucASMFlags   = 0;
        UINT16     usASMRuleNum = 0;
        HX_RESULT rv = pPacket->Get(pBuffer, ulOldTime, usStreamNum, ucASMFlags, usASMRuleNum);
        if (SUCCEEDED(rv))
        {
            rv = pPacket->Set(pBuffer, ulNewTime, usStreamNum, ucASMFlags, usASMRuleNum);
            if (SUCCEEDED(rv))
            {
                pPacket->SetAsLost();
            }
        }
        HX_RELEASE(pBuffer);
    }
}

HXBOOL
HXRecordControl::IsSparseStream(const char* pszMimeType)
{
    if (0 == ::strcasecmp(pszMimeType, "application/x-pn-realevent") ||
	0 == ::strcasecmp(pszMimeType, "syncMM/x-pn-realvideo")      ||
        0 == ::strcasecmp(pszMimeType, "application/x-pn-realad")    ||
	0 == ::strcasecmp(pszMimeType, "application/x-pn-imagemap")  ||
        0 == ::strcasecmp(pszMimeType, "image_map/x-pn-realvideo"))
    {
        return TRUE;
    }

    return FALSE;
}