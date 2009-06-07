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
#include "hlxclib/memory.h"
#include "ihxpckts.h"
#include "hxformt.h"
#include "mp4apyif.h"
#include "mdpkt.h"
#include "netbyte.h"
#include "mp3draft.h"

CMP3DraftPayloadFormat::CMP3DraftPayloadFormat()
{
    m_lRefCount    = 0;
    m_pCurPacket   = NULL;
    m_ulHeaderSize = 0;
    m_pHeader      = NULL;
    m_ulSampleRate = 0;
    m_bClearMainDataBegin = FALSE;
}

CMP3DraftPayloadFormat::~CMP3DraftPayloadFormat()
{
    HX_RELEASE(m_pCurPacket);
    HX_VECTOR_DELETE(m_pHeader);
}

HX_RESULT CMP3DraftPayloadFormat::Build(REF(IMP4APayloadFormat*) pFmt)
{
    HX_RESULT retVal = HXR_FAIL;

    pFmt = new CMP3DraftPayloadFormat();
    if (pFmt)
    {
        pFmt->AddRef();
        retVal = HXR_OK;
    }

    return retVal;
}

STDMETHODIMP CMP3DraftPayloadFormat::QueryInterface(THIS_ REFIID riid, void** ppvObj)
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

STDMETHODIMP_(ULONG32) CMP3DraftPayloadFormat::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32) CMP3DraftPayloadFormat::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

STDMETHODIMP CMP3DraftPayloadFormat::Init(IUnknown* pContext, HXBOOL bPacketize)
{
    HX_RESULT retVal = HXR_FAIL;

    if (!bPacketize)
    {
        retVal = HXR_OK;
    }

    return retVal;
}

STDMETHODIMP CMP3DraftPayloadFormat::Close()
{
    HX_RELEASE(m_pCurPacket);
    return HXR_OK;
}

STDMETHODIMP CMP3DraftPayloadFormat::Reset()
{
    HX_RELEASE(m_pCurPacket);
    return HXR_OK;
}

STDMETHODIMP CMP3DraftPayloadFormat::SetStreamHeader(IHXValues* pHeader)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pHeader)
    {
        IHXBuffer* pBuffer = NULL;
        retVal = pHeader->GetPropertyCString("MimeType", pBuffer);
        if (SUCCEEDED(retVal))
        {
            const char* pszMimeType = (const char*) pBuffer->GetBuffer();
            if (pszMimeType)
	    {
                if (!strcmp(pszMimeType, "audio/X-MP3-draft-00") ||
		    !strcmp(pszMimeType, "audio/X-MP3-draft-00-RN"))
		{
		    m_bClearMainDataBegin = TRUE;
		    retVal = HXR_OK;
		}
		else if (!strcmp(pszMimeType, "audio/MPEG-ELEMENTARY"))
		{
		    retVal = HXR_OK;
		}
	    }
        }
        HX_RELEASE(pBuffer);
        if (SUCCEEDED(retVal))
        {
            // Get the number of channels if specified in the stream header
            // Check both "NumChannels" and "Channels" in that order
            UINT32 ulTmp         = 0;
            UINT32 ulNumChannels = 0;
            if (SUCCEEDED(pHeader->GetPropertyULONG32("NumChannels", ulTmp)))
            {
                ulNumChannels = ulTmp;
            }
            else if (SUCCEEDED(pHeader->GetPropertyULONG32("Channels", ulTmp)))
            {
                ulNumChannels = ulTmp;
            }
            // Get the sample rate if specified in stream header
            // Check both "SampleRate" and "SamplesPerSecond" in that order
            if (SUCCEEDED(pHeader->GetPropertyULONG32("SampleRate", ulTmp)))
            {
                m_ulSampleRate = ulTmp;
            }
            else if (SUCCEEDED(pHeader->GetPropertyULONG32("SamplesPerSecond", ulTmp)))
            {
                m_ulSampleRate = ulTmp;
            }
            // Get the delay if specified in the stream header
            UINT32 ulDelay = 0;
            if (SUCCEEDED(pHeader->GetPropertyULONG32("Delay", ulTmp)))
            {
                ulDelay = ulTmp;
            }
            // Set the version of this bitstream header
            UINT32 ulVersion = 0;
            // Create our bitstream header
            m_pHeader = new BYTE [20];
            if (m_pHeader)
            {
                // Set the size
                m_ulHeaderSize = 20;
                // Pack the information into the bitstream header
                *((UINT32*) &m_pHeader[0])  = ulVersion;
                *((UINT32*) &m_pHeader[4])  = ulNumChannels;
                *((UINT32*) &m_pHeader[8])  = m_ulSampleRate;
                *((UINT32*) &m_pHeader[12]) = ulDelay;
		*((UINT32*) &m_pHeader[16]) = m_bClearMainDataBegin;
                // Byteswap if we are little-endian
                if (!TestBigEndian())
                {
                    SwapDWordBytes((HostDWord*) &m_pHeader[0], 5);
                }
            }
        }
    }

    return retVal;
}

STDMETHODIMP CMP3DraftPayloadFormat::GetStreamHeader(REF(IHXValues*) pHeader)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP CMP3DraftPayloadFormat::SetPacket(IHXPacket* pPacket)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pPacket)
    {
        HX_RELEASE(m_pCurPacket);
        m_pCurPacket = pPacket;
        m_pCurPacket->AddRef();
        retVal = HXR_OK;
    }

    return retVal;
}

STDMETHODIMP CMP3DraftPayloadFormat::GetPacket(REF(IHXPacket*) pOutPacket)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP CMP3DraftPayloadFormat::Flush()
{
    return HXR_OK;
}

ULONG32 CMP3DraftPayloadFormat::GetBitstreamHeaderSize(void)
{
    return m_ulHeaderSize;
}

const UINT8* CMP3DraftPayloadFormat::GetBitstreamHeader(void)
{
    return (const UINT8*) m_pHeader;
}

HX_RESULT CMP3DraftPayloadFormat::CreateMediaPacket(CMediaPacket*& pOutMediaPacket)
{
    HX_RESULT retVal = HXR_FAIL;

    if (m_pCurPacket)
    {
        IHXBuffer* pBuffer = m_pCurPacket->GetBuffer();
        if (pBuffer)
        {
	    /* Clear packet main data begin */
	    if (m_bClearMainDataBegin && pBuffer->GetSize() > 7)
	    {
		unsigned char* data = pBuffer->GetBuffer();
		int nOffset = 4;
		
		// Check protection bit
		if ((data[1] & 1) == 0)
		    nOffset += 2;
	    
		// MPEG1 main_data_begin is 9 bits in MPEG2 it is 8
		data[nOffset] = 0;

		if(m_ulSampleRate >= 32000)
		{
		    // This is MPEG1
		    data[nOffset + 1] &= 0x7F;
		}
	    }

            UINT32 ulTime          = m_pCurPacket->GetTime();
            UINT32 ulFlags         = MDPCKT_USES_IHXBUFFER_FLAG;
            pOutMediaPacket = new CMediaPacket(pBuffer,
                                               (UINT8*) pBuffer->GetBuffer(),
                                               pBuffer->GetSize(),
                                               pBuffer->GetSize(),
                                               ulTime,
                                               ulFlags,
                                               NULL);
            if (pOutMediaPacket)
            {
                HX_RELEASE(m_pCurPacket);
                retVal = HXR_OK;
            }
        }
        HX_RELEASE(pBuffer);
    }

    return retVal;
}

HX_RESULT CMP3DraftPayloadFormat::SetSamplingRate(ULONG32 ulSamplesPerSecond)
{
    m_ulSampleRate = ulSamplesPerSecond;
    return HXR_OK;
}

ULONG32 CMP3DraftPayloadFormat::GetTimeBase(void)
{
    ULONG32 ulTimeBase = 1000;

    return ulTimeBase;
}

HX_RESULT CMP3DraftPayloadFormat::SetAUDuration(ULONG32 ulAUDuration)
{
    return HXR_OK;
}

HX_RESULT CMP3DraftPayloadFormat::SetTimeAnchor(ULONG32 ulTimeMs)
{
    return HXR_OK;
}

