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
#include "hxwintyp.h"
#include "hxcom.h"
#include "hxcomm.h"

#include "hxassert.h"
#include "hxslist.h"
#include "hxmarsh.h"
#include "hxcomm.h"
#include "ihxpckts.h"
#include "hxformt.h"
#include "hxrendr.h"
#include "hxformt.h"
#include "hxengin.h"
#include "ringbuf.h"
#include "mp3format.h"  // CMp3Format

#include "mp3draft.h"


MP3DraftPayloadFormat::MP3DraftPayloadFormat()
    : m_lRefCount	    (0)
    , m_pContext	    (NULL)
    , m_pClassFactory	    (NULL)
    , m_pStreamHeader	    (NULL)
    , m_ulStreamNum	    (0)
    , m_bPacketizing	    (FALSE)
    , m_bFlushed	    (FALSE)
    , m_pFmtBuf		    (NULL)
    , m_pMp3Fmt		    (NULL)
    , m_bPacketReceived	    (FALSE)
    , m_bHeaderParsed	    (FALSE)
    , m_fTimePerFrame	    (0.0)
    , m_fNextTimestamp	    (0.0)
{
}

MP3DraftPayloadFormat::~MP3DraftPayloadFormat()
{
    HX_RELEASE(m_pContext);
    HX_RELEASE(m_pClassFactory);
    HX_RELEASE(m_pStreamHeader);
    HX_DELETE(m_pFmtBuf);
    HX_DELETE(m_pMp3Fmt);
}

// *** IUnknown methods ***

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::QueryInterface
//  Purpose:
//	Implement this to export the interfaces supported by your
//	object.
//
STDMETHODIMP
MP3DraftPayloadFormat::QueryInterface(REFIID riid, void** ppvObj)
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

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::AddRef
//  Purpose:
//	Everyone usually implements this the same... feel free to use
//	this implementation.
//
STDMETHODIMP_(ULONG32)
MP3DraftPayloadFormat::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::Release
//  Purpose:
//	Everyone usually implements this the same... feel free to use
//	this implementation.
//
STDMETHODIMP_(ULONG32)
MP3DraftPayloadFormat::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}


STDMETHODIMP
MP3DraftPayloadFormat::Init(IUnknown* pContext,
			    HXBOOL bPacketize)
{
    HX_ASSERT(pContext);

    HX_RELEASE(m_pContext);
    m_pContext = pContext;
    m_pContext->AddRef();

    HX_RELEASE(m_pClassFactory);
    m_pContext->QueryInterface(IID_IHXCommonClassFactory,
	(void**)&m_pClassFactory);
    HX_ASSERT(m_pClassFactory);

    // Remember if we are packetizing or depacketizing
    m_bPacketizing = bPacketize;
    if (!m_bPacketizing)
    {
	// For now, we don't support depacketizing
	return HXR_NOTIMPL;
    }

    // Create a ring buffer to store all incoming data
    HX_DELETE(m_pFmtBuf);
    m_pFmtBuf = new CIHXRingBuffer(m_pClassFactory, 8192, (1024<<1)+512);

    // Create an MP3 format object to handle parsing
    HX_DELETE(m_pMp3Fmt);
    m_pMp3Fmt = new CMp3Format();

    return HXR_OK;
}

STDMETHODIMP
MP3DraftPayloadFormat::Reset()
{
    // Discard all data from the ring buffer
    m_pFmtBuf->Reset();

    // Create a new MP3 format object
    HX_DELETE(m_pMp3Fmt);
    m_pMp3Fmt = new CMp3Format();
    m_bHeaderParsed = FALSE;

    m_bPacketReceived = FALSE;
    m_bFlushed = FALSE;

    return HXR_OK;
}

STDMETHODIMP
MP3DraftPayloadFormat::SetStreamHeader(IHXValues* pHeader)
{
    HX_ASSERT(pHeader);
    m_pStreamHeader = pHeader;
    m_pStreamHeader->AddRef();

    m_pStreamHeader->GetPropertyULONG32("StreamNumber", m_ulStreamNum);

    return HXR_OK;
}

STDMETHODIMP
MP3DraftPayloadFormat::GetStreamHeader(REF(IHXValues*) pHeader)
{
    HX_ASSERT(m_pStreamHeader);
    pHeader = m_pStreamHeader;
    pHeader->AddRef();

    return HXR_OK;
}

STDMETHODIMP
MP3DraftPayloadFormat::SetPacket(IHXPacket* pPacket)
{
    HX_RESULT hResult = HXR_OK;

    HX_ASSERT(pPacket);

    // Add the data from this buffer to the ring buffer
    IHXBuffer* pBuffer = pPacket->GetBuffer();
    m_pFmtBuf->CopyData(pBuffer->GetBuffer(), pBuffer->GetSize());
    HX_RELEASE(pBuffer);

    // Test for buffer overflow, and wrap the
    // ring buffer if necessary
    WrapRingBuffer();

    if (!m_bPacketReceived)
    {
	m_bPacketReceived = TRUE;

	m_fNextTimestamp = pPacket->GetTime();
    }

    if (!m_bHeaderParsed)
    {
	UCHAR* pBuf = NULL;
	UINT32 ulBytes = 0;

	// See if we have enough data to initialize our formatter
	pBuf = m_pFmtBuf->GetReadPointer(ulBytes);

	int     nFrame;
	INT32    lScan = 0;
	lScan = m_pMp3Fmt->ScanForSyncWord(pBuf, ulBytes, nFrame);
	if (lScan == -1)
	{
	    // We probably just don't have enough data yet to
	    // be able to initialize our header information
	    hResult = HXR_OK;
	}
	else
	{
	    // We have enough data to initialize, but we
	    // need to advance past any initial garbage to
	    // get to the beginning of the first MP3 frame
	    m_pFmtBuf->AdvanceRead(lScan);
	    pBuf = m_pFmtBuf->GetReadPointer(ulBytes);

	    // Init our reformatter
	    if (!m_pMp3Fmt->Init(pBuf, ulBytes))
	    {
		hResult = HXR_FAIL;
	    }
	    else
	    {
		UINT32 ulBitRate = 0;
		UINT32 ulMaxSampleRate = 0;
		int nChannels = 0;
		int nLayer = 0;
		int nSamplesPerFrame = 0;
		int nPadding = 0;

		m_pMp3Fmt->GetEncodeInfo(pBuf, ulBytes, ulBitRate,
					   ulMaxSampleRate, nChannels,
					   nLayer, nSamplesPerFrame, nPadding);

		// Compute frame duration in ms
		m_fTimePerFrame = nSamplesPerFrame * 1000.0 / ulMaxSampleRate;

		hResult = HXR_OK;
		m_bHeaderParsed = TRUE;
	    }
	}
    }

    return hResult;
}

STDMETHODIMP
MP3DraftPayloadFormat::GetPacket(REF(IHXPacket*) pPacket)
{
    HX_RESULT hResult = HXR_OK;
    pPacket = NULL;

    if (!m_bHeaderParsed)
    {
	if (m_bFlushed)
	{
	    // We have used up all available input
	    hResult = HXR_STREAM_DONE;
	}
	else
	{
	    // We don't have enough input
	    // data to produce a packet
	    hResult = HXR_INCOMPLETE;
	}
    }
    else
    {
	hResult = CreateNormalPacket(pPacket);
    }

    return hResult;
}

STDMETHODIMP
MP3DraftPayloadFormat::Flush()
{
    m_bFlushed = TRUE;

    return HXR_OK;
}

HX_RESULT
MP3DraftPayloadFormat::CreateNormalPacket(REF(IHXPacket*) pPacket)
{
    HX_RESULT hResult = HXR_OK;
    UCHAR* pModFrameStart = NULL;
    int nModFrameSize = 0;

    // Make sure the current read position in the
    // ring buffer is the beginning of an MP3 frame
    hResult = FindMP3Frame();
    if (SUCCEEDED(hResult))
    {
	UINT32 nSync = 0;

	for (;;)
	{
	    // We need 1+ audio frames
	    UINT32 ulReadSize = 0;
	    pModFrameStart = m_pFmtBuf->GetReadPointer(ulReadSize);

	    nSync = m_pMp3Fmt->CheckValidFrame(pModFrameStart, ulReadSize);

	    if (ulReadSize >= nSync + 6 ||
		m_bFlushed)
	    {
		nModFrameSize = m_pMp3Fmt->ReformatMP3Frame(&pModFrameStart,
							      ulReadSize,
							      m_pFmtBuf->GetPrevBytes());

		// If we did not have enough back data to reformat,
		// skip this frame in the format buffer
		if (!nModFrameSize && nSync)
		{
		    m_pFmtBuf->AdvanceRead(nSync);
		}
		else
		{
		    break;
		}
	    }
	    else
	    {
		nModFrameSize = 0;
		break;
	    }
	}

        if (nModFrameSize)
	{
            m_pFmtBuf->AdvanceRead(nSync);
	}
        else
        {
	    hResult = HXR_INCOMPLETE;
        }
    }

    if (SUCCEEDED(hResult))
    {
	IHXBuffer* pBuffer = NULL;
	UCHAR* pBuf = NULL;

	// Create a packet with this new formatted frame
	m_pClassFactory->CreateInstance(CLSID_IHXPacket, (void**)&pPacket);
	HX_ASSERT(pPacket);

	m_pClassFactory->CreateInstance(CLSID_IHXBuffer, (void**)&pBuffer);
	HX_ASSERT(pBuffer);

	pBuffer->SetSize(nModFrameSize + 4);
	pBuf = pBuffer->GetBuffer();

	// Set the 4-Byte audio header for this packet
	// (as dictated by RFC 2250)
	putshort(pBuf, 0);
	pBuf += 2;
	putshort(pBuf, 0);
	pBuf += 2;

	// Copy in the actual frame data
        memcpy(pBuf, pModFrameStart, nModFrameSize); /* Flawfinder: ignore */

        pPacket->Set(pBuffer, (UINT32)m_fNextTimestamp, (UINT16)m_ulStreamNum,
	    HX_ASM_SWITCH_ON | HX_ASM_SWITCH_OFF, 0);
	HX_RELEASE(pBuffer);

	m_fNextTimestamp += m_fTimePerFrame;
    }
    else if (m_bFlushed)
    {
	// We have used up all available input
	hResult = HXR_STREAM_DONE;
    }

    return hResult;
}

HX_RESULT
MP3DraftPayloadFormat::FindMP3Frame()
{
    INT32   lScan = 0;
    UCHAR* pFrame = NULL;
    UINT32 ulBytes = 0;

    pFrame = m_pFmtBuf->GetReadPointer(ulBytes);

    // We should be at an mp3 frame, make sure we are
    int nSync = m_pMp3Fmt->CheckValidFrame(pFrame, ulBytes);

    // If we were not at a frame, scan for the next one
    if (!nSync)
    {
        lScan = m_pMp3Fmt->ScanForSyncWord(pFrame,
                                           ulBytes,
                                           nSync);
	if (lScan == -1)
	{
	    return HXR_INCOMPLETE;
	}

	// Skip leading garbage
	m_pFmtBuf->AdvanceRead(lScan);

	// Try again...
	pFrame = m_pFmtBuf->GetReadPointer(ulBytes);
        nSync = m_pMp3Fmt->CheckValidFrame(pFrame, ulBytes);
    }

    if (nSync)
    {
	return HXR_OK;
    }

    return HXR_INCOMPLETE;
}

HX_RESULT
MP3DraftPayloadFormat::WrapRingBuffer()
{
    UCHAR  *pTemp = NULL;
    UINT32 ulBytes = 0;

    pTemp = m_pFmtBuf->GetReadPointer(ulBytes);

    UINT32  nSync = m_pMp3Fmt->CheckValidFrame(pTemp, ulBytes);

    // Make sure we have 1+ frame in the buffer
    if (!nSync ||
        (m_pFmtBuf->GetBytesInBuffer() > (UINT32)nSync+6 &&
         ulBytes < (UINT32)nSync + 6))
    {
        // Preserve enough data for main_data_begin
        m_pFmtBuf->Wrap(512);
    }

    return HXR_OK;
}
