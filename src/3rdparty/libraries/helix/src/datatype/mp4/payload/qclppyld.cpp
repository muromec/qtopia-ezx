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

#include <string.h>
#include "hxtypes.h"
#include "hxcom.h"
#include "qclppyld.h"
#include "hxccf.h"
#include "mp4desc.h"

#include "debug.h"

#define QCELP_FRAME_DURATION 160
#define D_QCELP              0

CQCELPPayloadFormat::CQCELPPayloadFormat() :
    m_lRefCount(0),
    m_uObjectProfileIndication(0),
    m_bitstreamHdr(NULL),
    m_ulBitstreamHdrSize(0),
    m_ulSampleRate(0),
    m_ulAUDuration(0),
    m_pCCF(0)
{
#ifdef DEBUG
    debug_level() |= D_QCELP;
#endif
}

CQCELPPayloadFormat::~CQCELPPayloadFormat()
{
    DPRINTF(D_QCELP,("CQCELPPayloadFormat::~CQCELPPayloadFormat()\n"));
    FlushOutput();
    HX_VECTOR_DELETE(m_bitstreamHdr);
    HX_RELEASE(m_pCCF);
}

HX_RESULT CQCELPPayloadFormat::Build(REF(IMP4APayloadFormat*) pFmt)
{
    pFmt = new CQCELPPayloadFormat();

    HX_RESULT res = HXR_OUTOFMEMORY;
    if (pFmt)
    {
	pFmt->AddRef();
	res = HXR_OK;
    }

    return res;
}

// *** IUnknown methods ***
STDMETHODIMP CQCELPPayloadFormat::QueryInterface (THIS_
					       REFIID riid,
					       void** ppvObj)
{
    QInterfaceList qiList[] =
    {
	{ GET_IIDHANDLE(IID_IUnknown), this },
	{ GET_IIDHANDLE(IID_IHXPayloadFormatObject), (IHXPayloadFormatObject*) this },
    };
    return ::QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
}

STDMETHODIMP_(ULONG32) CQCELPPayloadFormat::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32) CQCELPPayloadFormat::Release()
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
STDMETHODIMP CQCELPPayloadFormat::Init(IUnknown* pContext, HXBOOL bPacketize)
{
    DPRINTF(D_QCELP,("CQCELPPayloadFormat::Init()\n"));

    HX_RESULT res = HXR_UNEXPECTED;
    if (!bPacketize)
    {
	res = pContext->QueryInterface(IID_IHXCommonClassFactory,
				       (void**)&m_pCCF);
	res = HXR_OK;
    }
    return res;
}

STDMETHODIMP CQCELPPayloadFormat::Close()
{
    DPRINTF(D_QCELP,("CQCELPPayloadFormat::Close()\n"));
    return HXR_NOTIMPL;
}

STDMETHODIMP CQCELPPayloadFormat::Reset()
{
    HX_RESULT retVal = HXR_OK;
    DPRINTF(D_QCELP,("CQCELPPayloadFormat::Reset()\n"));

    retVal = m_depack.Reset();
    FlushOutput();

    return retVal;
}

STDMETHODIMP CQCELPPayloadFormat::SetStreamHeader(IHXValues* pHeader)
{
    DPRINTF(D_QCELP,("CQCELPPayloadFormat::SetStreamHeader()\n"));
    HX_RESULT                retVal        = HXR_UNEXPECTED;
    HX_RESULT                res           = HXR_UNEXPECTED;
    IHXBuffer*               pMimeType     = NULL;
    DecoderConfigDescriptor* pDCDesc       = NULL;
    DecoderSpecifcInfo*      pDSIDesc      = NULL;
    IHXBuffer*               pESDescriptor = NULL;
    UINT8*                   pESDescData   = NULL;
    ULONG32                  ulESDescSize  = 0;
    ES_Descriptor            ESDesc;

    if (SUCCEEDED(pHeader->GetPropertyCString("MimeType", pMimeType)) &&
        stricmp((const char *)pMimeType->GetBuffer(), "audio/QCELP") == 0)
    {
        retVal = HXR_OK;

        res = pHeader->GetPropertyBuffer("OpaqueData",
					  pESDescriptor);

        if (SUCCEEDED(res))
        {
	    retVal = HXR_INVALID_PARAMETER;
	    if (pESDescriptor)
	    {
	        retVal = HXR_OK;
	    }
        }

        if (SUCCEEDED(res))
        {
	    pESDescData = pESDescriptor->GetBuffer();
	    ulESDescSize = pESDescriptor->GetSize();

	    retVal = ESDesc.Unpack(pESDescData, ulESDescSize);
        }

        HX_RELEASE(pESDescriptor);

        if (SUCCEEDED(res))
        {
	    retVal = HXR_FAIL;
	    pDCDesc = ESDesc.m_pDecConfigDescr;

	    if (pDCDesc)
	    {
	        pDSIDesc = pDCDesc->m_pDecSpecificInfo;
	        retVal = HXR_OK;
	    }
        }

        if (SUCCEEDED(res))
        {
            m_uObjectProfileIndication = ESDesc.m_pDecConfigDescr->m_uObjectProfileIndication;
            m_ulBitstreamHdrSize = pDSIDesc->m_ulLength;
            m_bitstreamHdr = new UINT8[m_ulBitstreamHdrSize];
            if (!m_bitstreamHdr)
            {
                retVal = HXR_OUTOFMEMORY;
            }
        }

        if (SUCCEEDED(res))
        {
            memcpy(m_bitstreamHdr, pDSIDesc->m_pData, m_ulBitstreamHdrSize);
        }
        retVal = m_depack.Init(&CQCELPPayloadFormat::static_OnFrame, this, m_pCCF);
        if (SUCCEEDED(retVal))
        {
            retVal = m_depack.SetFrameDuration(QCELP_FRAME_DURATION);
        }
    }
    HX_RELEASE(pMimeType);
    return retVal;
}

STDMETHODIMP CQCELPPayloadFormat::GetStreamHeader(REF(IHXValues*) pHeader)
{
    DPRINTF(D_QCELP,("CQCELPPayloadFormat::GetStreamHeader()\n"));
    return HXR_NOTIMPL;
}

STDMETHODIMP CQCELPPayloadFormat::SetPacket(IHXPacket* pPacket)
{
    DPRINTF(D_QCELP,("CQCELPPayloadFormat::SetPacket()\n"));
    HX_RESULT     res             = HXR_UNEXPECTED;

    if (pPacket->IsLost())
    {
	res = m_depack.OnLoss(1);
    }
    else
    {
	IHXBuffer* pBuf = pPacket->GetBuffer();

	DPRINTF(D_QCELP,("CQCELPPayloadFormat::SetPacket(%lu, %lu, %u)\n",
		       pPacket->GetTime(),
		       pBuf->GetSize(),
		       pPacket->GetASMRuleNumber()));

	ULONG32 ulTime =  m_TSConverter.Convert(pPacket->GetTime());
	
	res = m_depack.OnPacket(ulTime,
			        pBuf->GetBuffer(),
			        pBuf->GetSize(),
			        (pPacket->GetASMRuleNumber() == 1));
	HX_RELEASE(pBuf);
    }

    return res;
}

STDMETHODIMP CQCELPPayloadFormat::GetPacket(REF(IHXPacket*) pOutPacket)
{
    DPRINTF(D_QCELP,("CQCELPPayloadFormat::GetPacket()\n"));
    return HXR_NOTIMPL;
}

STDMETHODIMP CQCELPPayloadFormat::Flush()
{
    HX_RESULT retVal = HXR_OK;
    DPRINTF(D_QCELP,("CQCELPPayloadFormat::Flush()\n"));

    retVal = m_depack.Flush();

    return retVal;
}

/*
 *	IMP4APayloadFormat methods
 */

HX_RESULT CQCELPPayloadFormat::CreateMediaPacket(CMediaPacket* &pOutMediaPacket)
{
    DPRINTF(D_QCELP,("CQCELPPayloadFormat::CreateMediaPacket()\n"));
    HX_RESULT res = HXR_NO_DATA;

    if (!m_outputQueue.IsEmpty())
    {
	pOutMediaPacket = (CMediaPacket*)m_outputQueue.RemoveHead();
	res = HXR_OK;
    }

    return res;
}

HX_RESULT CQCELPPayloadFormat::SetSamplingRate(ULONG32 ulSamplesPerSecond)
{
    DPRINTF(D_QCELP,("CQCELPPayloadFormat::SetSamplingRate(%lu)\n",
		    ulSamplesPerSecond));
    m_ulSampleRate = ulSamplesPerSecond;

    m_TSConverter.SetBase(1000, m_ulSampleRate);

    m_depack.SetSampleRate(m_ulSampleRate);

    return HXR_OK;
}

ULONG32 CQCELPPayloadFormat::GetTimeBase(void)
{
    DPRINTF(D_QCELP,("CQCELPPayloadFormat::GetTimeBase() = %ul\n",
		    m_ulSampleRate));

    return m_ulSampleRate;
}

HX_RESULT CQCELPPayloadFormat::SetAUDuration(ULONG32 ulAUDuration)
{
    DPRINTF(D_QCELP,("CQCELPPayloadFormat::SetAUDuration(%lu)\n",
		    ulAUDuration));
    m_ulAUDuration = ulAUDuration;
    return HXR_OK;
}

HX_RESULT CQCELPPayloadFormat::SetTimeAnchor(ULONG32 ulTimeMs)
{
    DPRINTF(D_QCELP,("CQCELPPayloadFormat::SetTimeAnchor(%lu)\n",
		    ulTimeMs));
    m_TSConverter.SetOffset(ulTimeMs, TSCONVRT_INPUT);
    return HXR_OK;
}

void CQCELPPayloadFormat::OnFrame(ULONG32 ulTime,
				const UINT8* pData, ULONG32 ulSize,
				HXBOOL bPreviousLoss)
{
    DPRINTF(D_QCELP,("CQCELPPayloadFormat::OnFrame(%lu, %lu, %d)\n",
		   ulTime,
		   ulSize,
		   bPreviousLoss));

    IHXBuffer* pBuf = 0;

    if (SUCCEEDED(m_pCCF->CreateInstance(IID_IHXBuffer, (void**)&pBuf)) &&
	SUCCEEDED(pBuf->SetSize(ulSize)))
    {
	::memcpy(pBuf->GetBuffer(), pData, ulSize); /* Flawfinder: ignore */

	ULONG32 ulFlags = MDPCKT_USES_IHXBUFFER_FLAG;

	if (bPreviousLoss)
	    ulFlags |= MDPCKT_FOLLOWS_LOSS_FLAG;

	CMediaPacket* pMediaPacket = new CMediaPacket(pBuf,
						      pBuf->GetBuffer(),
						      pBuf->GetSize(),
						      pBuf->GetSize(),
						      ulTime,
						      ulFlags,
						      0);
	if (pMediaPacket)
	    m_outputQueue.AddTail(pMediaPacket);
    }

    HX_RELEASE(pBuf);

}

void CQCELPPayloadFormat::static_OnFrame(void* pUserData,
				       ULONG32 ulTime,
				       const UINT8* pData, ULONG32 ulSize,
				       HXBOOL bPreviousLoss)
{
    CQCELPPayloadFormat* pObj = (CQCELPPayloadFormat*)pUserData;

    pObj->OnFrame(ulTime, pData, ulSize, bPreviousLoss);
}

void CQCELPPayloadFormat::FlushOutput()
{
    while(!m_outputQueue.IsEmpty())
    {
	CMediaPacket* pPkt = (CMediaPacket*)m_outputQueue.RemoveHead();
	delete pPkt;
    }
}
