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
#include "amrpyld.h"

#include <string.h>
#include "hxccf.h"

#include "amr_frame_hdr.h"
#include "amr_mime_info.h"

#include "debug.h"

#define D_AMR 0

// fmtp fields
static const char z_octetAlign[]     = "FMTPoctet-align";
static const char z_modeSet[]        = "FMTPmode-set";
static const char z_changePeriod[]   = "FMTPmode-change-period";
static const char z_changeNeighbor[] = "FMTPmode-change-neighbor";
static const char z_maxPTime[]       = "FMTPmaxptime";
static const char z_crc[]            = "FMTPcrc";
static const char z_robustSort[]     = "FMTProbust-sorting";
static const char z_interleaving[]   = "FMTPinterleaving";
static const char z_ptime[]          = "FMTPptime";
static const char z_channels[]       = "FMTPchannels";


CAMRPayloadFormat::CAMRPayloadFormat() :
    m_lRefCount(0),
    m_pCCF(0),
    m_ulSampleRate(0),
    m_ulAUDuration(0)
{
#ifdef DEBUG
    debug_level() |= D_AMR;
#endif
}

CAMRPayloadFormat::~CAMRPayloadFormat()
{
    DPRINTF(D_AMR,("CAMRPayloadFormat::~CAMRPayloadFormat()\n"));

    FlushOutput();
    HX_RELEASE(m_pCCF);
}

HX_RESULT CAMRPayloadFormat::Build(REF(IMP4APayloadFormat*) pFmt)
{
    pFmt = new CAMRPayloadFormat();

    HX_RESULT res = HXR_OUTOFMEMORY;
    if (pFmt)
    {
	pFmt->AddRef();
	res = HXR_OK;
    }

    return res;
}

// *** IUnknown methods ***
STDMETHODIMP CAMRPayloadFormat::QueryInterface (THIS_
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

STDMETHODIMP_(ULONG32) CAMRPayloadFormat::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32) CAMRPayloadFormat::Release()
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
STDMETHODIMP CAMRPayloadFormat::Init(IUnknown* pContext, HXBOOL bPacketize)
{
    DPRINTF(D_AMR,("CAMRPayloadFormat::Init()\n"));

    HX_RESULT res = HXR_UNEXPECTED;

    if (!bPacketize)
    {
	res = pContext->QueryInterface(IID_IHXCommonClassFactory,
				       (void**)&m_pCCF);

	res = HXR_OK;
    }

    return res;
}

STDMETHODIMP CAMRPayloadFormat::Close()
{
    DPRINTF(D_AMR,("CAMRPayloadFormat::Close()\n"));
    return HXR_NOTIMPL;
}

STDMETHODIMP CAMRPayloadFormat::Reset()
{
    DPRINTF(D_AMR,("CAMRPayloadFormat::Reset()\n"));

    m_depack.Reset();
    FlushOutput();

    return HXR_OK;
}

STDMETHODIMP CAMRPayloadFormat::SetStreamHeader(IHXValues* pHeader)
{
    DPRINTF(D_AMR,("CAMRPayloadFormat::SetStreamHeader()\n"));
    HX_RESULT res = HXR_UNEXPECTED;

    IHXBuffer* pMimeType = 0;
    IHXBuffer* pPayloadParams = 0;

    if (SUCCEEDED(pHeader->GetPropertyCString("MimeType", pMimeType)) &&
	SUCCEEDED(pHeader->GetPropertyCString("PayloadParameters",
					      pPayloadParams)) &&
	(CAMRMimeInfo::IsNarrowBandAMR((const char*)pMimeType->GetBuffer()) ||
	 CAMRMimeInfo::IsWideBandAMR((const char*)pMimeType->GetBuffer())))
    {
	AMRFlavor flavor = NarrowBand;
	ULONG32 ulOctetAlign = 0;
	ULONG32 ulModeSet = 0;
	ULONG32 ulChangePeriod = 0;
	ULONG32 ulChangeNeighbor = 0;
	ULONG32 ulMaxPtime = 0;
	ULONG32 ulHasCRC = 0;
	ULONG32 ulRobustSort = 0;
	ULONG32 ulMaxInterleave = 0;
	ULONG32 ulPtime = 0;
	ULONG32 ulChannels = 0;

	if (CAMRMimeInfo::IsNarrowBandAMR((const char*)pMimeType->GetBuffer()))
	{
	    flavor = NarrowBand;
	}
	else if (CAMRMimeInfo::IsWideBandAMR((const char*)pMimeType->GetBuffer()))
	{
	    flavor = WideBand;
	}

	pHeader->GetPropertyULONG32(z_octetAlign, ulOctetAlign);
	pHeader->GetPropertyULONG32(z_modeSet, ulModeSet);
	pHeader->GetPropertyULONG32(z_changePeriod, ulChangePeriod);
	pHeader->GetPropertyULONG32(z_changeNeighbor, ulChangeNeighbor);
	pHeader->GetPropertyULONG32(z_maxPTime, ulMaxPtime);
	pHeader->GetPropertyULONG32(z_crc, ulHasCRC);
	pHeader->GetPropertyULONG32(z_robustSort, ulRobustSort);
	pHeader->GetPropertyULONG32(z_interleaving, ulMaxInterleave);
	pHeader->GetPropertyULONG32(z_ptime, ulPtime);
	pHeader->GetPropertyULONG32(z_channels, ulChannels);
        
	if (m_depack.Init(flavor,
			  (ulOctetAlign == 1),
			  ulModeSet,
			  ulChangePeriod,
			  (ulChangeNeighbor = 1),
			  ulMaxPtime,
			  (ulHasCRC == 1),
			  (ulRobustSort == 1),
			  ulMaxInterleave,
			  ulPtime,
			  ulChannels,
			  &CAMRPayloadFormat::static_OnFrame,
			  this))
        {
            ULONG32 ulMinFrameCount = 5; // Tunable
            ULONG32 ulPtimeMultiplier = 2; // Tunable
            ULONG32 ulAMRFrameDuration = 20; // milliseconds
            ULONG32 ulPreroll = ulMinFrameCount * ulAMRFrameDuration;

            if (ulMaxInterleave)
            {
                // Set preroll time to the duration of the largest
                // allowable interleave block
                ulPreroll = ulMaxInterleave * ulAMRFrameDuration;
            }
            else if (ulMaxPtime)
            {
                // Set the preroll to ulPtimeMultiplier times the
                // max packet duration
                ulPreroll = ulMaxPtime * ulPtimeMultiplier;
            }
            else if (ulPtime)
            {
                // Set the preroll to ulPtimeMultiplier times the
                // packet duration
                ulPreroll = ulPtime * ulPtimeMultiplier;
            }

            pHeader->SetPropertyULONG32("Preroll", ulPreroll);

	    res = HXR_OK;
        }
    }

    HX_RELEASE(pMimeType);
    HX_RELEASE(pPayloadParams);

    return res;
}

STDMETHODIMP CAMRPayloadFormat::GetStreamHeader(REF(IHXValues*) pHeader)
{
    DPRINTF(D_AMR,("CAMRPayloadFormat::GetStreamHeader()\n"));
    return HXR_NOTIMPL;
}

STDMETHODIMP CAMRPayloadFormat::SetPacket(IHXPacket* pPacket)
{
    DPRINTF(D_AMR,("CAMRPayloadFormat::SetPacket()\n"));
    HX_RESULT res = HXR_UNEXPECTED;

    if (pPacket->IsLost())
    {
	if (m_depack.OnLoss(1))
	    res = HXR_OK;
    }
    else
    {
	IHXBuffer* pBuf = pPacket->GetBuffer();

	DPRINTF(D_AMR,("CAMRPayloadFormat::SetPacket(%lu, %lu, %u)\n",
		       pPacket->GetTime(),
		       pBuf->GetSize(),
		       pPacket->GetASMRuleNumber()));

	ULONG32 ulTime =  m_TSConverter.Convert(pPacket->GetTime());
	
	if (m_depack.OnPacket(ulTime,
			      pBuf->GetBuffer(),
			      pBuf->GetSize(),
			      (pPacket->GetASMRuleNumber() == 1)))
	    res = HXR_OK;

	HX_RELEASE(pBuf);
    }

    return res;
}

STDMETHODIMP CAMRPayloadFormat::GetPacket(REF(IHXPacket*) pOutPacket)
{
    DPRINTF(D_AMR,("CAMRPayloadFormat::GetPacket()\n"));
    return HXR_NOTIMPL;
}

STDMETHODIMP CAMRPayloadFormat::Flush()
{
    DPRINTF(D_AMR,("CAMRPayloadFormat::Flush()\n"));

    m_depack.Flush();

    return HXR_OK;
}

/*
 *	IMP4APayloadFormat methods
 */
ULONG32 CAMRPayloadFormat::GetBitstreamHeaderSize(void)
{
    DPRINTF(D_AMR,("CAMRPayloadFormat::GetBitstreamHeaderSize()\n"));
    return 1;
}

const UINT8* CAMRPayloadFormat::GetBitstreamHeader(void)
{
    DPRINTF(D_AMR,("CAMRPayloadFormat::GetBitstreamHeader()\n"));
    return m_bitstreamHdr;
}

HX_RESULT CAMRPayloadFormat::CreateMediaPacket(CMediaPacket* &pOutMediaPacket)
{
    DPRINTF(D_AMR,("CAMRPayloadFormat::CreateMediaPacket()\n"));
    HX_RESULT res = HXR_NO_DATA;

    if (!m_outputQueue.IsEmpty())
    {
	pOutMediaPacket = (CMediaPacket*)m_outputQueue.RemoveHead();
	res = HXR_OK;
    }

    return res;
}

HX_RESULT CAMRPayloadFormat::SetSamplingRate(ULONG32 ulSamplesPerSecond)
{
    DPRINTF(D_AMR,("CAMRPayloadFormat::SetSamplingRate(%lu)\n",
		    ulSamplesPerSecond));
    m_ulSampleRate = ulSamplesPerSecond;

    m_TSConverter.SetBase(1000, m_ulSampleRate);

    m_depack.SetTSSampleRate(m_ulSampleRate);

    return HXR_OK;
}

ULONG32 CAMRPayloadFormat::GetTimeBase(void)
{
    DPRINTF(D_AMR,("CAMRPayloadFormat::GetTimeBase() = %ul\n",
		    m_ulSampleRate));

    return m_ulSampleRate;
}

HX_RESULT CAMRPayloadFormat::SetAUDuration(ULONG32 ulAUDuration)
{
    DPRINTF(D_AMR,("CAMRPayloadFormat::SetAUDuration(%lu)\n",
		    ulAUDuration));
    m_ulAUDuration = ulAUDuration;
    return HXR_OK;
}

HX_RESULT CAMRPayloadFormat::SetTimeAnchor(ULONG32 ulTimeMs)
{
    DPRINTF(D_AMR,("CAMRPayloadFormat::SetTimeAnchor(%lu)\n",
		    ulTimeMs));
    m_TSConverter.SetOffset(ulTimeMs, TSCONVRT_INPUT);
    return HXR_OK;
}

void CAMRPayloadFormat::OnFrame(ULONG32 ulTime,
				const UINT8* pData, ULONG32 ulSize,
				HXBOOL bPreviousLoss)
{
    DPRINTF(D_AMR,("CAMRPayloadFormat::OnFrame(%lu, %lu, %d)\n",
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

void CAMRPayloadFormat::static_OnFrame(void* pUserData,
				       ULONG32 ulTime,
				       const UINT8* pData, ULONG32 ulSize,
				       HXBOOL bPreviousLoss)
{
    CAMRPayloadFormat* pObj = (CAMRPayloadFormat*)pUserData;

    pObj->OnFrame(ulTime, pData, ulSize, bPreviousLoss);
}

void CAMRPayloadFormat::FlushOutput()
{
    while(!m_outputQueue.IsEmpty())
    {
	CMediaPacket* pPkt = (CMediaPacket*)m_outputQueue.RemoveHead();
	delete pPkt;
    }
}
