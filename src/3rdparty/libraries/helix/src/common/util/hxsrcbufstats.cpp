/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: hxsrcbufstats.cpp,v 1.7 2007/01/11 19:41:00 milko Exp $ 
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
#include "hxsrcbufstats.h"
#include "hxslist.h"
#include "ihxpckts.h"

class HXPktInfo
{
public:
    HXPktInfo(INT64 llTimestamp,
              UINT32 uPacketSize,
              UINT32 uSeqNum);

    INT64 Timestamp() const { return m_llTimestamp;}
    UINT32 PacketSize() const { return m_uPacketSize;}
    UINT32 SequenceNum() const { return m_uSeqNum;}

private:
    INT64 m_llTimestamp;
    UINT32 m_uPacketSize;
    UINT32 m_uSeqNum;
};

HXPktInfo::HXPktInfo(INT64 llTimestamp,
                     UINT32 uPacketSize,
                     UINT32 uSeqNum) :
    m_llTimestamp(llTimestamp),
    m_uPacketSize(uPacketSize),
    m_uSeqNum(uSeqNum)
{}

class HXStreamPktBufInfo
{
public:
    enum {InvalidSeqNum = 0xffffffff};

    HXStreamPktBufInfo();
    ~HXStreamPktBufInfo();

    void Init();
    void Reset();
    void OnPacket(UINT32 uTimestamp,
                  UINT32 uPacketSize,
                  UINT16 uSeqNum);
    void OnTimeSync(UINT32 ulCurrentTime, UINT32 uFirstLivePacketTS);

    HX_RESULT GetBufferingStats(REF(INT64) llLowTS,
                                REF(INT64) llHighTS,
                                REF(UINT32) uNumBytes);
    
    HX_RESULT GetNADUInfo(UINT32 uTransportBufBytes,
                          REF(UINT16) uPlayoutDelay,
                          REF(UINT16) uNextSeqNumber,
                          REF(UINT16) uNextUnitNumber,
                          REF(UINT16) uFreeBufferSpace);

    HX_RESULT SetNADUParameters(UINT32 uFrequency,
                                UINT32 uBufferSize);

    HX_RESULT GetNADUFrequency(REF(UINT32) uFrequency);
    HX_RESULT GetNADUBufferSize(REF(UINT32) uBufferSize);
    
private:

    void UpdateBufferInfo();
    INT64 CreateINT64Timesync(UINT32 ulTime) const;
    INT64 CreateINT64Timestamp(UINT32 ulTime) const;
    UINT16 GetFreeBufferSpace(UINT32 uTransportBufBytes);

    CHXSimpleList m_pktInfo;
    UINT32 m_uBytesBuffered;
    HXBOOL   m_bCurrentTimeValid;
    INT64  m_llCurrentTime;

    UINT32 m_ulTimeSyncRollOver;
    UINT32 m_ulLastTimeSync;

    UINT32 m_ulTSRollOver;
    UINT32 m_ulLastPacketTS;

    HXBOOL m_bFirstPkt;
    INT64 m_llHighestTimestamp;
    UINT32 m_uNADUFreq;
    UINT32 m_uNADUBufferSize;
};


HXStreamPktBufInfo::HXStreamPktBufInfo() : 
    m_uBytesBuffered(0),
    m_bCurrentTimeValid(FALSE),
    m_llCurrentTime(0),
    m_ulTimeSyncRollOver(0),
    m_ulLastTimeSync(0),
    m_ulTSRollOver(0),
    m_ulLastPacketTS(0),
    m_bFirstPkt(TRUE),
    m_uNADUFreq(0),
    m_uNADUBufferSize(0)
{}

HXStreamPktBufInfo::~HXStreamPktBufInfo()
{
    Reset();
}

void HXStreamPktBufInfo::Init()
{
    Reset();
}

void HXStreamPktBufInfo::OnPacket(UINT32 uTimestamp,
                                  UINT32 uPacketSize,
                                  UINT16 uSeqNum)
{
    //  0xFA .. 0xFF [roll over] (0x01)
    if (m_ulLastPacketTS > uTimestamp &&
	((m_ulLastPacketTS - uTimestamp) > MAX_TIMESTAMP_GAP))
    {
	m_ulTSRollOver++;
    }
    
    INT64 llActualTS = CreateINT64Timestamp(uTimestamp);
    m_ulLastPacketTS = uTimestamp;

    if (m_bFirstPkt)
    {
        m_llHighestTimestamp = llActualTS;        
        m_bFirstPkt = FALSE;
    }

    if (m_llHighestTimestamp < llActualTS)
    {
        m_llHighestTimestamp = llActualTS;
    }

    HXPktInfo* pPktInfo = new HXPktInfo(llActualTS,
                                        uPacketSize,
                                        uSeqNum);

    if (pPktInfo)
    {
        if (m_pktInfo.AddTail(pPktInfo))
        {
            m_uBytesBuffered += uPacketSize;
        }
        else
        {
            HX_DELETE(pPktInfo);
        }
    }

    // We update the buffer info here to
    // keep the list as short as possible
    UpdateBufferInfo();
}

void HXStreamPktBufInfo::OnTimeSync(UINT32 uCurrentTime, UINT32 uFirstLivePacketTS)
{
    //  0xFA .. 0xFF [roll over] (0x01)
    if (m_ulLastTimeSync > uCurrentTime &&
       ((m_ulLastTimeSync - uCurrentTime) > MAX_TIMESTAMP_GAP))
    {
       m_ulTimeSyncRollOver++;
    }
    m_ulLastTimeSync = uCurrentTime;

    if (!m_bFirstPkt)
    {
        m_llCurrentTime = CreateINT64Timesync(uCurrentTime);

        m_llCurrentTime += uFirstLivePacketTS; // Add back the live packet TS
        m_bCurrentTimeValid = TRUE;
    }
}

void HXStreamPktBufInfo::Reset()
{
    while(!m_pktInfo.IsEmpty())
    {
        HXPktInfo* pPktInfo = (HXPktInfo*)m_pktInfo.RemoveHead();
        HX_DELETE(pPktInfo);
    }
    
    m_uBytesBuffered = 0;
    m_bCurrentTimeValid = FALSE;
    m_llCurrentTime = 0;
    m_ulTimeSyncRollOver = 0;
    m_ulLastTimeSync = 0;
    m_ulTSRollOver = 0;
    m_ulLastPacketTS = 0;
    m_bFirstPkt = TRUE;
    m_llHighestTimestamp = 0;
}

HX_RESULT 
HXStreamPktBufInfo::GetBufferingStats(REF(INT64) llLowTS,
                                      REF(INT64) llHighTS,
                                      REF(UINT32) uNumBytes)
{
    HX_RESULT res = HXR_NO_DATA;

    if (!m_bFirstPkt)
    {
        UpdateBufferInfo();

        if (!m_pktInfo.IsEmpty())
        {
            HXPktInfo* pPktInfo = (HXPktInfo*)m_pktInfo.GetHead();
            llLowTS = pPktInfo->Timestamp();
        }
        else
        {
            llLowTS = m_llHighestTimestamp;
        }

        llHighTS = m_llHighestTimestamp;
        uNumBytes = m_uBytesBuffered;

        res = HXR_OK;
    }

    return res;
}

HX_RESULT
HXStreamPktBufInfo::GetNADUInfo(UINT32 uTransportBufBytes,
                                REF(UINT16) uPlayoutDelay,
                                REF(UINT16) uNextSeqNumber,
                                REF(UINT16) uNextUnitNumber,
                                REF(UINT16) uFreeBufferSpace)
{
    HX_RESULT res = HXR_FAILED;

    UpdateBufferInfo();

    if (m_pktInfo.IsEmpty())
    {
        // Our buffer is empty
        uPlayoutDelay = 0xffff;
        uNextSeqNumber = 0;
        uNextUnitNumber = 0;
        uFreeBufferSpace = GetFreeBufferSpace(uTransportBufBytes);

        // We return HXR_NO_DATA so the caller knows that we didn't
        // fill in uPlayoutDelay, uNextSeqNumber, uNextUnitNumber with
        // valid values, but uFreeBufferSpace is valid.
        res = HXR_NO_DATA;
    }
    else
    {
        // We have packets buffered
        HXPktInfo* pPktInfo = (HXPktInfo*)m_pktInfo.GetHead();
        
        if (pPktInfo->SequenceNum() != InvalidSeqNum)
        {
            // This packet info has a valid sequence number

            uNextSeqNumber = (UINT16)pPktInfo->SequenceNum();

            // We always specify unit 0 for now because we have no
            // way to determine which application data unit is 
            // actually next.
            uNextUnitNumber = 0;

            if (m_bCurrentTimeValid)
            {
                
                UINT32 uDelay = 
                    INT64_TO_UINT32(pPktInfo->Timestamp() - m_llCurrentTime);

                
                if (uDelay > 0xfffe)
                {
                    // The delay is too large. Use the reserved value
                    uPlayoutDelay = 0xffff;
                }
                else
                {
                    uPlayoutDelay = (UINT16)uDelay;
                }
            }
            else
            {
                // We don't know what the delay is
                // so just report the reserved value.
                uPlayoutDelay = 0xffff;
            }

            uFreeBufferSpace = GetFreeBufferSpace(uTransportBufBytes);
            
            res = HXR_OK;
        }
    }
    
    return res;
}

HX_RESULT 
HXStreamPktBufInfo::SetNADUParameters(UINT32 uFrequency,
                                      UINT32 uBufferSize)
{
    m_uNADUFreq = uFrequency;
    m_uNADUBufferSize = uBufferSize;

    return HXR_OK;
}

HX_RESULT
HXStreamPktBufInfo::GetNADUFrequency(REF(UINT32) uFrequency)
{
    uFrequency = m_uNADUFreq;

    return HXR_OK;
}

HX_RESULT 
HXStreamPktBufInfo::GetNADUBufferSize(REF(UINT32) uBufferSize)
{
    uBufferSize = m_uNADUBufferSize;
    return HXR_OK;
}

void HXStreamPktBufInfo::UpdateBufferInfo()
{
    HXBOOL bDone = m_pktInfo.IsEmpty() || !m_bCurrentTimeValid;
    while(!bDone)
    {
        HXPktInfo* pPktInfo = (HXPktInfo*)m_pktInfo.GetHead();

        if (pPktInfo->Timestamp() <= m_llCurrentTime)
        {
            m_pktInfo.RemoveHead();
            m_uBytesBuffered -= pPktInfo->PacketSize();

            HX_DELETE(pPktInfo);

            bDone = m_pktInfo.IsEmpty();
        }
        else
        {
            bDone = TRUE;
        }
    }
}

INT64 
HXStreamPktBufInfo::CreateINT64Timestamp(UINT32 ulTime) const
{
    return ((CAST_TO_INT64(m_ulTSRollOver) * CAST_TO_INT64 MAX_UINT32) + 
            CAST_TO_INT64(ulTime));
}

INT64 
HXStreamPktBufInfo::CreateINT64Timesync(UINT32 ulTime) const
{
    return ((CAST_TO_INT64(m_ulTimeSyncRollOver) * CAST_TO_INT64(MAX_UINT32)) +
            CAST_TO_INT64(ulTime));
}

UINT16 
HXStreamPktBufInfo::GetFreeBufferSpace(UINT32 uTransportBufBytes)
{
    UINT16 uRet = 0;

    UINT32 uTotalBytesBuffered = m_uBytesBuffered + uTransportBufBytes;
    
    if (uTotalBytesBuffered < m_uNADUBufferSize)
    {
        UINT32 uBytesFree = m_uNADUBufferSize - uTotalBytesBuffered;
        
        // Enforce the upper limit
        const UINT32 NADUMaxBytesFree = 0xffff * 64;
        if (uBytesFree < NADUMaxBytesFree)
        {
            // The 3GPP spec says to report the free buffer space 
            // as the number of 64 byte blocks available.
            uRet = (UINT16)(uBytesFree / 64);
        }
        else
        {
            // The amount of free space is over the
            // highest value that can be reported. 
            // Just report the max.
            uRet = 0xffff;
        }
    }

    return uRet;
}


HXSourceBufferStats::HXSourceBufferStats() :
    m_lRefCount(0),
    m_pTimeMan(NULL),
    m_bIsLive(FALSE),
    m_bFirstPacket(TRUE),
    m_uFirstLivePacketTS(0)
{}

HXSourceBufferStats::~HXSourceBufferStats()
{
    DoClose();
}

HX_RESULT 
HXSourceBufferStats::Init(IUnknown* pContext)
{
    HX_RESULT res = HXR_FAILED;

    if (pContext)
    {
        if (HXR_OK == pContext->QueryInterface(IID_IHXTransportTimeManager,
                                               (void**)&m_pTimeMan))
        {
            if (HXR_OK == m_pTimeMan->AddSink(this))
            {
                res = HXR_OK;
            }
            else
            {
                HX_RELEASE(m_pTimeMan);
            }
        }
    }

    return res;
}

void HXSourceBufferStats::Close()
{
    DoClose();
}

HX_RESULT 
HXSourceBufferStats::InitStream(UINT16  uStreamNumber, HXBOOL bIsLive)
{
    HX_RESULT res = HXR_FAILED;

    if (Initialized())
    {
        HXStreamPktBufInfo* pBufInfo = GetPktBufInfo(uStreamNumber);
        
        if (!pBufInfo)
        {
            pBufInfo = new HXStreamPktBufInfo();
            if (pBufInfo)
            {
                m_streamPktInfo[uStreamNumber] = pBufInfo;
            }
        }
        
        if (pBufInfo)
        {
            pBufInfo->Init();
        }

        m_bIsLive = bIsLive;
    }

    return res;
}

void HXSourceBufferStats::OnPacket(IHXPacket* pPacket)
{
    OnPacket(pPacket, HXStreamPktBufInfo::InvalidSeqNum);
}

void HXSourceBufferStats::OnPacket(IHXPacket* pPacket,
                                   UINT16 uSeqNum)
{    
    if (pPacket)
    {
        if (m_bFirstPacket)
        {
            if (m_bIsLive)
            {
                m_uFirstLivePacketTS = pPacket->GetTime();
            }
            m_bFirstPacket = FALSE;
        }

        HXStreamPktBufInfo* pBufInfo = 
            GetPktBufInfo(pPacket->GetStreamNumber());

        if (pBufInfo)
        {
            IHXBuffer* pBuf = pPacket->GetBuffer();
            pBufInfo->OnPacket(pPacket->GetTime(), 
                               pBuf->GetSize(), 
                               uSeqNum);
            HX_RELEASE(pBuf);
        }
    }
}

/*
 * IHXTransportTimeSink methods
 */
STDMETHODIMP
HXSourceBufferStats::OnTransportTime(THIS_ UINT32 ulCurrentTime)
{
    CHXMapLongToObj::Iterator itr = m_streamPktInfo.Begin();

    for(;itr != m_streamPktInfo.End(); ++itr)
    {
        HXStreamPktBufInfo* pPktInfo = (HXStreamPktBufInfo*)(*itr);
        if (pPktInfo)
        {
            pPktInfo->OnTimeSync(ulCurrentTime, m_uFirstLivePacketTS);
        }
    }

    return HXR_OK;
}

void HXSourceBufferStats::Reset()
{
    CHXMapLongToObj::Iterator itr = m_streamPktInfo.Begin();

    m_bFirstPacket = TRUE;
    m_uFirstLivePacketTS = 0;

    for(;itr != m_streamPktInfo.End(); ++itr)
    {
        HXStreamPktBufInfo* pPktInfo = (HXStreamPktBufInfo*)(*itr);
        if (pPktInfo)
        {
            pPktInfo->Reset();
        }
    }
}

/*
 * IUnknown methods
 */
STDMETHODIMP
HXSourceBufferStats::QueryInterface(THIS_
                                    REFIID riid,
                                    void** ppvObj)
{
    QInterfaceList qiList[] =
    {
        { GET_IIDHANDLE(IID_IUnknown), this },
        { GET_IIDHANDLE(IID_IHXSourceBufferingStats3), (IHXSourceBufferingStats3*) this },
        { GET_IIDHANDLE(IID_IHXTransportTimeSink), (IHXTransportTimeSink*) this },
        { GET_IIDHANDLE(IID_IHX3gppNADU), (IHX3gppNADU*)this}
    };
    return QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
}

STDMETHODIMP_(ULONG32) HXSourceBufferStats::AddRef(THIS)
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32) HXSourceBufferStats::Release(THIS)
{
    ULONG32 ulRet = DecRefCount();

    if (!ulRet)
    {
        delete this;
    }

    return ulRet;
}

ULONG32 HXSourceBufferStats::DecRefCount()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    return 0;
}

/*
 * IHXSourceBufferingStats3 methods
 */
STDMETHODIMP 
HXSourceBufferStats::GetCurrentBuffering (THIS_ UINT16 uStreamNumber,
                                          REF(UINT32) ulLowestTimestamp, 
                                          REF(UINT32) ulHighestTimestamp,
                                          REF(UINT32) ulNumBytes,
                                          REF(HXBOOL) bDone)
{
    HX_RESULT res = HXR_INVALID_PARAMETER;

    if (GetPktBufInfo(uStreamNumber))
    {
        ulLowestTimestamp = 0;
        ulHighestTimestamp = 0;
        ulNumBytes = 0;
        bDone = FALSE;
        res = HXR_OK;
    }

    return res;
}

STDMETHODIMP
HXSourceBufferStats::GetTotalBuffering(THIS_ UINT16 uStreamNumber,
                                       REF(UINT32) ulLowestTimestamp, 
                                       REF(UINT32) ulHighestTimestamp,
                                       REF(UINT32) ulNumBytes,
                                       REF(HXBOOL) bDone)
{
    HX_RESULT res = HXR_INVALID_PARAMETER;

    HXStreamPktBufInfo* pBufInfo = GetPktBufInfo(uStreamNumber);
    
    if (pBufInfo)
    {
	INT64 llLowestTimestamp;
	INT64 llHighestTimestamp;
        UINT32 ulTransportLow = 0;
        UINT32 ulTransportHigh = 0;
        UINT32 ulTransportBytes = 0;
        HXBOOL bTransportDone = FALSE;
        
        res = pBufInfo->GetBufferingStats(llLowestTimestamp,
                                          llHighestTimestamp,
                                          ulNumBytes);

        bDone = FALSE;

        if (HXR_OK == res)
	{
	    ulLowestTimestamp = INT64_TO_UINT32(llLowestTimestamp);
	    ulHighestTimestamp = INT64_TO_UINT32(llHighestTimestamp);

            if (HXR_OK == GetCurrentBuffering(uStreamNumber,
					      ulTransportLow, 
                                              ulTransportHigh, 
                                              ulTransportBytes,
                                              bTransportDone))
	    {
		if (((LONG32) (ulTransportHigh - ulHighestTimestamp)) > 0)
		{
		    ulHighestTimestamp = ulTransportHigh;
		}
            
		ulNumBytes += ulTransportBytes;
		bDone = bTransportDone;
	    }
	}
    }

    return res;
}

/*
 * IHX3gppNADU Methods
 */

STDMETHODIMP
HXSourceBufferStats::GetNADUInfo(THIS_ UINT16 uStreamNumber,
                                 REF(UINT16) uPlayoutDelay,
                                 REF(UINT16) uNextSeqNumber,
                                 REF(UINT16) uNextUnitNumber,
                                 REF(UINT16) uFreeBufferSpace)
{
    HX_RESULT res = HXR_INVALID_PARAMETER;

    HXStreamPktBufInfo* pBufInfo = GetPktBufInfo(uStreamNumber);

    if (pBufInfo)
    {
        UINT32 ulLowestTimestamp;
        UINT32 ulHighestTimestamp;
        UINT32 uTransportBufBytes;
        HXBOOL bDone;
        
        // Get the number of bytes in the transport buffer
        res = GetCurrentBuffering(uStreamNumber,
                                  ulLowestTimestamp,
                                  ulHighestTimestamp,
                                  uTransportBufBytes,
                                  bDone);

        if (HXR_OK == res)
        {
            res = pBufInfo->GetNADUInfo(uTransportBufBytes,
                                        uPlayoutDelay, uNextSeqNumber,
                                        uNextUnitNumber, uFreeBufferSpace);
        }
    }
    
    return res;
}

STDMETHODIMP
HXSourceBufferStats::SetNADUParameters(THIS_ UINT16 uStreamNumber,
                                       UINT32 uFrequency,
                                       UINT32 uBufferSize)
{
    HX_RESULT res = HXR_INVALID_PARAMETER;

    HXStreamPktBufInfo* pBufInfo = GetPktBufInfo(uStreamNumber);
    
    if (pBufInfo)
    {
        res = pBufInfo->SetNADUParameters(uFrequency, uBufferSize);
    }
    
    return res;
}

STDMETHODIMP
HXSourceBufferStats::GetNADUFrequency(THIS_ UINT16 uStreamNumber,
                                      REF(UINT32) uFrequency)
{
    HX_RESULT res = HXR_INVALID_PARAMETER;

    HXStreamPktBufInfo* pBufInfo = GetPktBufInfo(uStreamNumber);

    if (pBufInfo)
    {
        res = pBufInfo->GetNADUFrequency(uFrequency);
    }
    
    return res;
}

STDMETHODIMP
HXSourceBufferStats::GetNADUBufferSize(THIS_ UINT16 uStreamNumber,
                                       REF(UINT32) uBufferSize)
{
    HX_RESULT res = HXR_INVALID_PARAMETER;

    HXStreamPktBufInfo* pBufInfo = GetPktBufInfo(uStreamNumber);

    if (pBufInfo)
    {
        res = pBufInfo->GetNADUBufferSize(uBufferSize);
    }
    
    return res;
}

void HXSourceBufferStats::DoClose()
{
    CHXMapLongToObj::Iterator itr = m_streamPktInfo.Begin();
    for(;itr != m_streamPktInfo.End(); ++itr)
    {
        HXStreamPktBufInfo* pPktInfo = (HXStreamPktBufInfo*)(*itr);
        HX_DELETE(pPktInfo);
    }

    m_streamPktInfo.RemoveAll();

    if (m_pTimeMan)
    {
        m_pTimeMan->RemoveSink(this);
        HX_RELEASE(m_pTimeMan);
    }
}

HXStreamPktBufInfo* 
HXSourceBufferStats::GetPktBufInfo(UINT16  uStreamNumber)
{
    HXStreamPktBufInfo* pRet;

    if (!m_streamPktInfo.Lookup(uStreamNumber,(void*&) pRet))
    {
        pRet = NULL;
    }

    return pRet;
}
