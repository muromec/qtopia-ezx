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

#include "concatpyld.h"

const UINT32 MinPacketDuration = 200;

HXConcatenatePayloadFormat::HXConcatenatePayloadFormat() :
    m_lRefCount(0),
    m_pCCF(NULL),
    m_pHeader(NULL),
    m_pFinishedPkt(NULL),
    m_bHaveFirstPkt(FALSE),
    m_ulTimestamp(0),
    m_uStream(0),
    m_uASMFlags(0),
    m_uASMRule(0),
    m_ulByteCount(0),
    m_bPendingFlush(FALSE)
{}

HXConcatenatePayloadFormat::~HXConcatenatePayloadFormat()
{
    Close();
}

HX_RESULT 
HXConcatenatePayloadFormat::CreateInstance(REF(IHXPayloadFormatObject*) pPyld)
{
    HX_RESULT res = HXR_OUTOFMEMORY;

    pPyld = new HXConcatenatePayloadFormat();

    if (pPyld)
    {
        pPyld->AddRef();
        res = HXR_OK;
    }

    return res;
}

/*
 *      IUnknown methods
 */
STDMETHODIMP HXConcatenatePayloadFormat::QueryInterface(THIS_
                                                        REFIID riid,
                                                        void** ppvObj)
{
    HX_RESULT retVal = HXR_NOINTERFACE;
    QInterfaceList qiList[] =
    {
        { GET_IIDHANDLE(IID_IUnknown), this },
        { GET_IIDHANDLE(IID_IHXPayloadFormatObject), (IHXPayloadFormatObject*) this },
    };

    if (ppvObj)
    {
        *ppvObj = NULL;
        return ::QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
    }
    return retVal;
}

STDMETHODIMP_(ULONG32) HXConcatenatePayloadFormat::AddRef(THIS)
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32) HXConcatenatePayloadFormat::Release(THIS)
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }
    
    delete this;
    return 0;
}

/*
 *      IHXPayloadFormatObject methods
 */
STDMETHODIMP HXConcatenatePayloadFormat::Init(THIS_
                                              IUnknown* pContext,
                                              HXBOOL bPacketize)
{
    HX_RESULT res = HXR_UNEXPECTED;

    if (bPacketize && pContext)
    {
        res = pContext->QueryInterface(IID_IHXCommonClassFactory,
                                       (void**)&m_pCCF);
    }

    return res;
}

STDMETHODIMP HXConcatenatePayloadFormat::Close(THIS)
{
    Reset();

    HX_RELEASE(m_pCCF);

    return HXR_OK;
}

STDMETHODIMP HXConcatenatePayloadFormat::Reset(THIS)
{
    HX_RELEASE(m_pFinishedPkt);

    m_bHaveFirstPkt = FALSE;
	m_bPendingFlush = FALSE;
    ClearBufferList();

    return HXR_OK;
}

STDMETHODIMP HXConcatenatePayloadFormat::SetStreamHeader(THIS_
                                                         IHXValues* pHeader)
{
    HX_RESULT res = HXR_FAILED;
    
    HX_RELEASE(m_pHeader);

    if (pHeader)
    {   
        m_pHeader = pHeader;
        m_pHeader->AddRef();

        res = HXR_OK;
    }

    return res;
}

STDMETHODIMP HXConcatenatePayloadFormat::GetStreamHeader(THIS_
                                                       REF(IHXValues*) pHeader)
{
    pHeader = m_pHeader;

    return (m_pHeader) ? HXR_OK : HXR_FAILED;
}

STDMETHODIMP HXConcatenatePayloadFormat::SetPacket(THIS_
                                                   IHXPacket* pPacket)
{
    HX_RESULT res = HXR_OK;

    if (!pPacket)
    {
        res = HXR_UNEXPECTED;
    }
    else if (m_bHaveFirstPkt)
    {
        UINT32 ulTimeDelta = pPacket->GetTime() - m_ulTimestamp;
        if (!MatchesFirstPacket(pPacket) || (ulTimeDelta >= MinPacketDuration))
        {
            HX_RESULT tmpRes = CreateFinishedPkt();

            if (HXR_OK == tmpRes)
            {
		OnFirstPacket(pPacket);
            }
            else if (HXR_OUTOFMEMORY == tmpRes)
            {
                res = HXR_OUTOFMEMORY;
            }
	    else
	    {
		/* Any other errors will just
		 * cause this packet to get
		 * queued in the buffer list.
		 */
	    }
        }
    }
    else
    {
	OnFirstPacket(pPacket);
    }

    if (HXR_OK == res)
    {
	res = AddToBufferList(pPacket);
    }

    return res;
}

STDMETHODIMP HXConcatenatePayloadFormat::GetPacket(THIS_
                                                   REF(IHXPacket*) pPacket)
{
    HX_RESULT res = HXR_OK;

    if (m_bPendingFlush && !m_pFinishedPkt)
    {
        /* Handle the pending flush*/
        res = CreateFinishedPkt();
    }

    if (m_pFinishedPkt)
    {
        /* Transfering ownership here */
        pPacket = m_pFinishedPkt;
        m_pFinishedPkt = NULL;
    }
    else if (HXR_OK == res)
    {
        /* We don't have a finished
         * packet here. We need more
         * packets.
         */
        res = HXR_INCOMPLETE;
    }

    return res;
}

STDMETHODIMP HXConcatenatePayloadFormat::Flush(THIS)
{
    HX_RESULT res = HXR_OK;

    if (m_pFinishedPkt)
    {
        m_bPendingFlush = TRUE;
    }
    else if (m_bHaveFirstPkt)
	{
        res = CreateFinishedPkt();
    }

    return res;
}

HX_RESULT HXConcatenatePayloadFormat::CreateFinishedPkt()
{
    HX_RESULT res = HXR_UNEXPECTED;

    if (!m_pFinishedPkt && m_pCCF && m_bHaveFirstPkt)
    {
        IHXPacket* pNewPkt = NULL;
        IHXBuffer* pBuf = NULL;

        if ((HXR_OK == m_pCCF->CreateInstance(CLSID_IHXPacket,
                                          (void**)&pNewPkt)) &&
            (HXR_OK == m_pCCF->CreateInstance(CLSID_IHXBuffer, 
                                              (void**)&pBuf)) &&
            (HXR_OK == pBuf->SetSize(m_ulByteCount)))
        {
            UCHAR* pCurPos = pBuf->GetBuffer();
            
            /* Copy the buffers in the buffer list */
            LISTPOSITION pos = m_bufferList.GetHeadPosition();
            while(pos)
            {
                IHXBuffer* pTmpBuf = (IHXBuffer*)m_bufferList.GetNext(pos);

                memcpy(pCurPos, pTmpBuf->GetBuffer(), pTmpBuf->GetSize());
                pCurPos += pTmpBuf->GetSize();
            }

            res = pNewPkt->Set(pBuf, m_ulTimestamp, m_uStream, m_uASMFlags,
                               m_uASMRule);

            if (HXR_OK == res)
            {
                m_bPendingFlush = FALSE;

                m_pFinishedPkt = pNewPkt;
                m_pFinishedPkt->AddRef();

		m_bHaveFirstPkt = FALSE;
                ClearBufferList();
            }
        }

        HX_RELEASE(pNewPkt);
        HX_RELEASE(pBuf);
    }

    return res;
}

HX_RESULT HXConcatenatePayloadFormat::AddToBufferList(IHXPacket* pPacket)
{
    IHXBuffer* pBuf = pPacket->GetBuffer();

    if (pBuf)
    {
        m_ulByteCount += pBuf->GetSize();
        
        /* Transfering ownership here.
         * We don't need to call HX_RELEASE()
         * on pBuf
         */
        m_bufferList.AddTail(pBuf);
    }

    return HXR_OK;
}

void HXConcatenatePayloadFormat::ClearBufferList()
{
    while(!m_bufferList.IsEmpty())
    {
        IHXBuffer* pBuf = (IHXBuffer*)m_bufferList.RemoveHead();
        HX_RELEASE(pBuf);
    }

    m_ulByteCount = 0;
}

HXBOOL HXConcatenatePayloadFormat::MatchesFirstPacket(IHXPacket* pPacket)
{
    HXBOOL bRet = FALSE;

    if (m_bHaveFirstPkt && pPacket &&
        (pPacket->GetStreamNumber() == m_uStream) &&
        (pPacket->GetASMRuleNumber() == m_uASMRule) &&
        (pPacket->GetASMFlags() == m_uASMFlags))
    {
        bRet = TRUE;
    }

    return bRet;
}

void HXConcatenatePayloadFormat::OnFirstPacket(IHXPacket* pPacket)
{
    m_bHaveFirstPkt = TRUE;
    m_ulTimestamp = pPacket->GetTime();
    m_uStream = pPacket->GetStreamNumber();
    m_uASMFlags = pPacket->GetASMFlags();
    m_uASMRule = pPacket->GetASMRuleNumber();
}
