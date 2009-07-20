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

#include "hxtypes.h"
#include "hxcom.h"
#include "hxccf.h"
#include "ihxpckts.h"
#include "hxslist.h"
#include "hxamrpack.h"
#include "amr_frame_hdr.h"
#include "amr_mime_info.h"

CHXAMRPayloadFormatPacketizer::CHXAMRPayloadFormatPacketizer() :
    m_lRefCount(0),
    m_pCCF(NULL),
    m_pOutQueue(NULL),
    m_ulMinPacketSize(DEFAULT_MIN_PACKET_SIZE),
    m_ulAvgPacketSize(DEFAULT_AVG_PACKET_SIZE),
    m_ulMaxPacketSize(DEFAULT_MAX_PACKET_SIZE),
    m_ulPacketBytesConsumed(0),
    m_ulDurationConsumed(0),
    m_flavor(NarrowBand),
    m_bFlush(FALSE)
{
}

CHXAMRPayloadFormatPacketizer::~CHXAMRPayloadFormatPacketizer()
{
    ClearOutputQueue();
    HX_RELEASE(m_pCCF);
    HX_DELETE(m_pOutQueue);
}

STDMETHODIMP CHXAMRPayloadFormatPacketizer::QueryInterface(REFIID riid, void** ppvObj)
{
    QInterfaceList qiList[] =
    {
	{ GET_IIDHANDLE(IID_IUnknown), this },
	{ GET_IIDHANDLE(IID_IHXPayloadFormatObject), (IHXPayloadFormatObject*) this },
    };

    HX_RESULT retVal = HXR_FAIL;

    if (ppvObj)
    {
        // Set default
        *ppvObj = NULL;
        // Clear return
        retVal = HXR_OK;
        return ::QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
    }
    return retVal;
}

STDMETHODIMP_(ULONG32) CHXAMRPayloadFormatPacketizer::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32) CHXAMRPayloadFormatPacketizer::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

STDMETHODIMP CHXAMRPayloadFormatPacketizer::Init(IUnknown* pContext, HXBOOL bPacketize)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pContext && bPacketize)
    {
        HX_RELEASE(m_pCCF);
        retVal = pContext->QueryInterface(IID_IHXCommonClassFactory,
                                          (void**) &m_pCCF);
    }

    return retVal;
}

STDMETHODIMP CHXAMRPayloadFormatPacketizer::Close()
{
    ClearOutputQueue();

    return HXR_OK;
}

STDMETHODIMP CHXAMRPayloadFormatPacketizer::Reset()
{
    return Close();
}

STDMETHODIMP CHXAMRPayloadFormatPacketizer::SetStreamHeader(IHXValues* pHeader)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pHeader)
    {
        IHXBuffer* pTmp = NULL;
        pHeader->GetPropertyCString("MimeType", pTmp);
        if (pTmp)
        {
            const char* pszTmp = (const char*) pTmp->GetBuffer();
            if (pszTmp)
            {
                if (CAMRMimeInfo::IsHXAMR(pszTmp))
                {
                    m_flavor = NarrowBand;
                    retVal   = HXR_OK;
                }
                else if (CAMRMimeInfo::IsHXWideBandAMR(pszTmp))
                {
                    m_flavor = WideBand;
                    retVal   = HXR_OK;
                }
                if (SUCCEEDED(retVal))
                {
                    // Get the max packet size if there is one
                    m_ulMaxPacketSize = GetProp(pHeader, "MaxPacketSize");
                    // Get the avg packet size if there is one
                    m_ulAvgPacketSize = GetProp(pHeader, "AvgPacketSize");
                    // Get the max packet size if there is one
                    m_ulMinPacketSize = GetProp(pHeader, "MinPacketSize");
                }
            }
        }
        HX_RELEASE(pTmp);
    }

    return retVal;
}

STDMETHODIMP CHXAMRPayloadFormatPacketizer::GetStreamHeader(REF(IHXValues*) pHeader)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP CHXAMRPayloadFormatPacketizer::SetPacket(IHXPacket* pPacket)
{
    // Process this input packet into output queue packets.
    // We could have done this either in SetPacket() or GetPacket(),
    // but we choose to do it here. We don't force flushing yet,
    // since we may get more data. Once we get a call to flush,
    // then we process all the input, regardless of min size.
    ProcessInputPacket(pPacket, m_bFlush, m_ulMinPacketSize,
                       m_ulPacketBytesConsumed, m_ulDurationConsumed);

    return HXR_OK;
}

STDMETHODIMP CHXAMRPayloadFormatPacketizer::GetPacket(REF(IHXPacket*) pOutPacket)
{
    HX_RESULT retVal = HXR_FAIL;

    if (m_pOutQueue && m_pOutQueue->GetCount() > 0)
    {
        IHXPacket* pPacket = (IHXPacket*) m_pOutQueue->RemoveHead();
        if (pPacket)
        {
            HX_RELEASE(pOutPacket);
            pOutPacket = pPacket;
            pOutPacket->AddRef();
            // Clear the return value
            retVal = HXR_OK;
        }
        // Release the queue's ref
        HX_RELEASE(pPacket);
    }

    return retVal;
}

STDMETHODIMP CHXAMRPayloadFormatPacketizer::Flush()
{
    m_bFlush = TRUE;

    return HXR_OK;
}

void CHXAMRPayloadFormatPacketizer::ClearOutputQueue()
{
    if (m_pOutQueue)
    {
        LISTPOSITION pos = m_pOutQueue->GetHeadPosition();
        while (pos)
        {
            IHXPacket* pPacket = (IHXPacket*) m_pOutQueue->GetNext(pos);
            HX_RELEASE(pPacket);
        }
        m_pOutQueue->RemoveAll();
    }
}

HXBOOL CHXAMRPayloadFormatPacketizer::ProcessInputPacket(IHXPacket*  pPacket,
                                                       HXBOOL        bFlush,
                                                       UINT32      ulMinSize,
                                                       REF(UINT32) rulBytesConsumed,
                                                       REF(UINT32) rulTimeConsumed)
{
    HXBOOL bRet = FALSE;

    if (pPacket)
    {
        IHXBuffer* pBuffer = pPacket->GetBuffer();
        if (pBuffer)
        {
            BYTE*  pBufStart = pBuffer->GetBuffer();
            BYTE*  pBuf      = pBufStart;
            BYTE*  pBufLimit = pBuffer->GetBuffer() + pBuffer->GetSize();
            UINT32 ulLen     = 0;
            UINT32 ulDur     = 0;
            UINT32 ulDurSum  = 0;
            while (pBuf < pBufLimit &&
                   FindAllAMRFramesLength(pBuf, pBufLimit - pBuf,
                                          ulMinSize,
                                          ulLen, ulDur))
            {
                // Create the packet
                CreateAndQueuePacket(pBuf, ulLen,
                                     pPacket->GetTime() + ulDurSum,
                                     pPacket->GetStreamNumber(),
                                     pPacket->GetASMFlags(),
                                     pPacket->GetASMRuleNumber());
                // Advance the pointer
                pBuf += ulLen;
                // Add to duration sum
                ulDurSum += ulDur;
            }
            // Now if bFlush is TRUE and we have some bytes left,
            // then we need to go ahead and consume everything.
            if (bFlush && pBuf < pBufLimit)
            {
                // Setting the minimum to 0 will cause everything
                // to be consumed.
                if (FindAllAMRFramesLength(pBuf, pBufLimit - pBuf, 0, ulLen, ulDur))
                {
                    // Create the packet
                    CreateAndQueuePacket(pBuf, ulLen,
                                         pPacket->GetTime() + ulDurSum,
                                         pPacket->GetStreamNumber(),
                                         pPacket->GetASMFlags(),
                                         pPacket->GetASMRuleNumber());
                    // Advance the pointer
                    pBuf += ulLen;
                    // Add to duration sum.
                    ulDurSum += ulDur;
                }
            }
            // Set the number of packet bytes consumed
            rulBytesConsumed = pBuf - pBufStart;
            // Set the duration consumed
            rulTimeConsumed  = ulDurSum;
            // If we produced any packets, then return TRUE
            bRet = (pBuf > pBufStart ? TRUE : FALSE);
        }
        HX_RELEASE(pBuffer);
    }

    return bRet;
}

HXBOOL CHXAMRPayloadFormatPacketizer::FindAMRFrameLength(BYTE* pBuf, UINT32 ulLen,
                                                       REF(UINT32) rulLen, REF(UINT32) rulDur)
{
    HXBOOL bRet = FALSE;

    if (pBuf && ulLen)
    {
        CAMRFrameHdr cHdr(m_flavor);
        cHdr.Unpack(pBuf);
        if (cHdr.HdrBytes() + cHdr.DataBytes() <= ulLen)
        {
            rulLen = cHdr.HdrBytes() + cHdr.DataBytes();
            rulDur = CAMRFrameInfo::FrameDuration();
            bRet   = TRUE;
        }
    }

    return bRet;
}

HXBOOL CHXAMRPayloadFormatPacketizer::FindAllAMRFramesLength(BYTE* pBuf, UINT32 ulLen,
                                                           UINT32 ulMinSize, REF(UINT32) rulLen,
                                                           REF(UINT32) rulDur)
{
    HXBOOL bRet = FALSE;

    if (pBuf && ulLen)
    {
        BYTE*  pBufStart  = pBuf;
        BYTE*  pBufLimit  = pBuf + ulLen;
        UINT32 ulDurSum   = 0;
        UINT32 ulFrameLen = 0;
        UINT32 ulFrameDur = 0;
        while (pBuf < pBufLimit &&
               (ulMinSize ? (UINT32)(pBuf - pBufStart) < ulMinSize : TRUE) &&
               FindAMRFrameLength(pBuf, pBufLimit - pBuf, ulFrameLen, ulFrameDur))
        {
            pBuf     += ulFrameLen;
            ulDurSum += ulFrameDur;
        }
        if (pBuf > pBufStart && (UINT32)(pBuf - pBufStart) >= ulMinSize)
        {
            rulLen = pBuf - pBufStart;
            rulDur = ulDurSum;
            bRet   = TRUE;
        }
    }

    return bRet;
}

HXBOOL CHXAMRPayloadFormatPacketizer::CreateAndQueuePacket(BYTE* pBuf, UINT32 ulLen, UINT32 ulTimeStamp,
                                                         UINT16 usStream, UINT8 ucASMFlags, UINT16 usRuleNum)
{
    HXBOOL bRet = FALSE;

    if (pBuf && ulLen && m_pCCF)
    {
        IHXBuffer* pBuffer = NULL;
        m_pCCF->CreateInstance(CLSID_IHXBuffer, (void**) &pBuffer);
        if (pBuffer)
        {
            HX_RESULT rv = pBuffer->Set(pBuf, ulLen);
            if (SUCCEEDED(rv))
            {
                IHXPacket* pPacket = NULL;
                m_pCCF->CreateInstance(CLSID_IHXPacket, (void**) &pPacket);
                if (pPacket)
                {
                    rv = pPacket->Set(pBuffer, ulTimeStamp, usStream, ucASMFlags, usRuleNum);
                    if (SUCCEEDED(rv))
                    {
                        if (!m_pOutQueue)
                        {
                            m_pOutQueue = new CHXSimpleList();
                        }
                        if (m_pOutQueue)
                        {
                            pPacket->AddRef();
                            m_pOutQueue->AddTail((void*) pPacket);
                            bRet = TRUE;
                        }
                    }
                }
                HX_RELEASE(pPacket);
            }
        }
        HX_RELEASE(pBuffer);
    }

    return bRet;
}

UINT32 CHXAMRPayloadFormatPacketizer::GetProp(IHXValues* pValues, const char* pszName)
{
    UINT32 ulRet = 0;

    if (pValues && pszName)
    {
        UINT32 ulTmp = 0;
        if (SUCCEEDED(pValues->GetPropertyULONG32(pszName, ulTmp)) && ulTmp)
        {
            ulRet = ulTmp;
        }
    }

    return ulRet;
}

