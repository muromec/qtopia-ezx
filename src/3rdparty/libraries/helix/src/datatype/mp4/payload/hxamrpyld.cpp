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
#include "hxslist.h"
#include "hxamrpyld.h"

#include "amr_frame_hdr.h"
#include "amr_mime_info.h"

#include "debug.h"

#define D_HXAMR 0

CHXAMRPayloadFormat::CHXAMRPayloadFormat() :
    m_lRefCount(0),
    m_ulSampleRate(0),
    m_ulAUDuration(0),
    m_pCurrentPkt(0),
    m_pCurBufPos(0),
    m_ulBytesLeft(0),
    m_ulTimestamp(0),
    m_flavor(NarrowBand),
    m_bNeedTimestamp(FALSE),
    m_bLoss(FALSE)
{
#ifdef DEBUG
    debug_level() |= D_HXAMR;
#endif
}

CHXAMRPayloadFormat::~CHXAMRPayloadFormat()
{
    DPRINTF(D_HXAMR,("CHXAMRPayloadFormat::~CHXAMRPayloadFormat()\n"));
    HX_RELEASE(m_pCurrentPkt);
}

HX_RESULT CHXAMRPayloadFormat::Build(REF(IMP4APayloadFormat*) pFmt)
{
    pFmt = new CHXAMRPayloadFormat();
    
    HX_RESULT res = HXR_OUTOFMEMORY;
    if (pFmt)
    {
	pFmt->AddRef();
	res = HXR_OK;
    }

    return res;
}


// *** IUnknown methods ***
STDMETHODIMP CHXAMRPayloadFormat::QueryInterface (THIS_
					       REFIID riid,
					       void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
	AddRef();
	*ppvObj = this;
	return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXPayloadFormatObject))
    {
	AddRef();
	*ppvObj = (IHXPayloadFormatObject*)this;
	return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32) CHXAMRPayloadFormat::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32) CHXAMRPayloadFormat::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

/*
 *	IHXPayloadFormatObject methods
 */
STDMETHODIMP CHXAMRPayloadFormat::Init(IUnknown* pContext, HXBOOL bPacketize)
{
    DPRINTF(D_HXAMR,("CHXAMRPayloadFormat::Init()\n"));

    HX_RESULT res = HXR_UNEXPECTED;

    if (!bPacketize)
    {
	HX_RELEASE(m_pCurrentPkt);
	m_pCurBufPos = 0;
	m_ulBytesLeft = 0;
	
	m_bNeedTimestamp = TRUE;
	m_bLoss = FALSE;

	res = HXR_OK;
    }

    return res; 
}

STDMETHODIMP CHXAMRPayloadFormat::Close()
{ 
    DPRINTF(D_HXAMR,("CHXAMRPayloadFormat::Close()\n"));

    return HXR_OK;
}

STDMETHODIMP CHXAMRPayloadFormat::Reset()
{
    DPRINTF(D_HXAMR,("CHXAMRPayloadFormat::Reset()\n"));

    HX_RELEASE(m_pCurrentPkt);
    m_pCurBufPos = 0;
    m_ulBytesLeft = 0;

    m_bNeedTimestamp = TRUE;
    m_bLoss = FALSE;

    return HXR_OK; 
}

STDMETHODIMP CHXAMRPayloadFormat::SetStreamHeader(IHXValues* pHeader)
{
    DPRINTF(D_HXAMR,("CHXAMRPayloadFormat::SetStreamHeader()\n"));
    HX_RESULT res = HXR_UNEXPECTED;
    
    IHXBuffer* pBuf = 0;

    if (SUCCEEDED(pHeader->GetPropertyCString("MimeType", pBuf)))
    {
	if (CAMRMimeInfo::IsHXAMR((const char*)pBuf->GetBuffer()))
	{
	    m_flavor = NarrowBand;
	    res = HXR_OK;
	}
	else if (CAMRMimeInfo::IsHXWideBandAMR((const char*)pBuf->GetBuffer()))
	{
	    m_flavor = WideBand;
	    res = HXR_OK;
	}
    }

    HX_RELEASE(pBuf);
    
    return res; 
}

STDMETHODIMP CHXAMRPayloadFormat::GetStreamHeader(REF(IHXValues*) pHeader)
{
    DPRINTF(D_HXAMR,("CHXAMRPayloadFormat::GetStreamHeader()\n"));
    return HXR_NOTIMPL;
}

STDMETHODIMP CHXAMRPayloadFormat::SetPacket(IHXPacket* pPacket)
{
    DPRINTF(D_HXAMR,("CHXAMRPayloadFormat::SetPacket()\n"));
    HX_RESULT res = HXR_UNEXPECTED;

    if (!m_pCurrentPkt)
    {
	if (!pPacket->IsLost())
	{
	    if (m_bNeedTimestamp)
	    {
		m_ulTimestamp = m_TSConverter.Convert(pPacket->GetTime());
		m_bNeedTimestamp = FALSE;
	    }
	    m_pCurrentPkt = pPacket;
	    m_pCurrentPkt->AddRef();
	    m_pCurBufPos = 0;
	    m_ulBytesLeft = 0;
	}
	else
	{
	    m_bLoss = TRUE;
	    m_bNeedTimestamp = TRUE;
	}
        res = HXR_OK;
    }

    return res; 
}

STDMETHODIMP CHXAMRPayloadFormat::GetPacket(REF(IHXPacket*) pOutPacket)
{
    DPRINTF(D_HXAMR,("CHXAMRPayloadFormat::GetPacket()\n"));

    return HXR_OK;
}

STDMETHODIMP CHXAMRPayloadFormat::Flush()
{
    DPRINTF(D_HXAMR,("CHXAMRPayloadFormat::Flush()\n"));

    return HXR_OK;
}

/*
 *	IMP4APayloadFormat methods
 */
ULONG32 CHXAMRPayloadFormat::GetBitstreamHeaderSize(void)
{
    DPRINTF(D_HXAMR,("CHXAMRPayloadFormat::GetBitstreamHeaderSize()\n"));
    return 1;
}

const UINT8* CHXAMRPayloadFormat::GetBitstreamHeader(void)
{
    DPRINTF(D_HXAMR,("CHXAMRPayloadFormat::GetBitstreamHeader()\n"));
    return m_bitstreamHdr;
}

HX_RESULT CHXAMRPayloadFormat::CreateMediaPacket(CMediaPacket* &pOutMediaPacket)
{
    DPRINTF(D_HXAMR,("CHXAMRPayloadFormat::CreateMediaPacket()\n"));
    HX_RESULT res = HXR_NO_DATA;

    if (m_pCurrentPkt)
    {
	IHXBuffer* pBuf = m_pCurrentPkt->GetBuffer();

	if (!m_pCurBufPos)
	{
	    m_pCurBufPos = pBuf->GetBuffer();
	    m_ulBytesLeft = pBuf->GetSize();
	}

	if (m_ulBytesLeft)
	{
	    ULONG32 ulFlags = MDPCKT_USES_IHXBUFFER_FLAG;

	    if (m_bLoss)
		ulFlags |= MDPCKT_FOLLOWS_LOSS_FLAG;

	    CAMRFrameHdr hdr(m_flavor);
	    UINT8* pTmp = m_pCurBufPos;
	    hdr.Unpack(pTmp);

	    ULONG32 ulFrameSize = hdr.DataBytes() + CAMRFrameHdr::HdrBytes();

            // per PR 109022, it's possible for a badly encoded clip to cause
            // things to go nuts because the frame sizes appear to be messed up.
            // The code below allows us to survive these and still play to
            // completion. xxxjhhb
            if( ulFrameSize > m_ulBytesLeft )
            {
		m_pCurBufPos = 0;
		HX_RELEASE(m_pCurrentPkt);
	        HX_RELEASE(pBuf);
                return HXR_FAIL;
            }

	    pOutMediaPacket = new CMediaPacket(pBuf,
					       m_pCurBufPos,
					       pBuf->GetSize(),
					       ulFrameSize,
					       m_ulTimestamp,
					       ulFlags,
					       0);
	    m_pCurBufPos += ulFrameSize;
	    m_ulBytesLeft -= ulFrameSize;
	    m_ulTimestamp += m_ulAUDuration;

	    if (pOutMediaPacket)
	    {
		if (m_bLoss)
		    m_bLoss = FALSE;
		
		res = HXR_OK;
	    }
	    else
		res = HXR_OUTOFMEMORY;

	    if (!m_ulBytesLeft)
	    {
		m_pCurBufPos = 0;
		HX_RELEASE(m_pCurrentPkt);
	    }
	}
	
	HX_RELEASE(pBuf);
    }

    return res; 
}

HX_RESULT CHXAMRPayloadFormat::SetSamplingRate(ULONG32 ulSamplesPerSecond)
{
    DPRINTF(D_HXAMR,("CHXAMRPayloadFormat::SetSamplingRate(%lu)\n",
		    ulSamplesPerSecond));
    m_ulSampleRate = ulSamplesPerSecond;

    m_TSConverter.SetBase(1000, m_ulSampleRate);

    return HXR_OK;
}

ULONG32 CHXAMRPayloadFormat::GetTimeBase(void)
{
    DPRINTF(D_HXAMR,("CHXAMRPayloadFormat::GetTimeBase() = %ul\n",
		     m_ulSampleRate));

    return m_ulSampleRate;
}

HX_RESULT CHXAMRPayloadFormat::SetAUDuration(ULONG32 ulAUDuration)
{
    DPRINTF(D_HXAMR,("CHXAMRPayloadFormat::SetAUDuration(%lu)\n",
		    ulAUDuration));
    m_ulAUDuration = ulAUDuration;
    return HXR_OK; 
}

HX_RESULT CHXAMRPayloadFormat::SetTimeAnchor(ULONG32 ulTimeMs)
{
    DPRINTF(D_HXAMR,("CHXAMRPayloadFormat::SetTimeAnchor(%lu)\n",
		    ulTimeMs));
    m_TSConverter.SetOffset(ulTimeMs, TSCONVRT_INPUT);
    return HXR_OK; 
}

