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

#include "hxcom.h"
#include "hxtypes.h"
#include "hxplugn.h"
#include "hxformt.h"
#include "ihxpckts.h"
#include "hxcomm.h"
#include "hxpends.h"
#include "riff.h"
#include "riffres.h"
#include "netbyte.h"
#include "hxmarsh.h"
#include "legacy.h"
#include "hxengin.h"
#include "baseobj.h"
#include "aiffplin.h"
#include "aiffplin.ver"
#include "hxver.h"

#include "rmfftype.h"	// for the HX_SAVE_ENABLED flag

#include <hlxclib/math.h>

const char* const AIFFFileFormat::zm_pDescription    = "Helix AIFF File Format Plugin";
const char* const AIFFFileFormat::zm_pCopyright      = HXVER_COPYRIGHT;
const char* const AIFFFileFormat::zm_pMoreInfoURL    = HXVER_MOREINFO;

const char* const AIFFFileFormat::zm_pFileMimeTypes[]  = {"audio/x-pn-aiff", NULL};
const char* const AIFFFileFormat::zm_pFileExtensions[] = {"aiff", "aif", NULL};
const char* const AIFFFileFormat::zm_pFileOpenNames[]  = {"AIFF Files (*.aif)", NULL};

#define COMM_CHUNK_ID 0x434f4d4d /* 'COMM' */
#define SSND_CHUNK_ID 0x53534e44 /* 'SSND' */

#define PACKETSIZE 500

HX_RESULT STDAPICALLTYPE AIFFFileFormat::HXCreateInstance
(
    IUnknown**  /*OUT*/	ppIUnknown
)
{
   *ppIUnknown = (IUnknown*)(IHXPlugin*)new AIFFFileFormat();
    if (*ppIUnknown)
    {
	(*ppIUnknown)->AddRef();
	return HXR_OK;
    }
    return HXR_OUTOFMEMORY;
}

HX_RESULT STDAPICALLTYPE AIFFFileFormat::CanUnload2(void)
{
    return ((CHXBaseCountingObject::ObjectsActive() > 0) ? HXR_FAIL : HXR_OK);
}

UINT32 IEEE754ToUINT32(UINT8* bytes)
{
    UINT32 ulRet  = (bytes[2] << 24) | (bytes[3] << 16) | (bytes[4] << 8) | bytes[5];
    UINT32 ulExp  = (bytes[1] <= 30 ? 30 - bytes[1] : 0);
    UINT32 ulLast = 0;
    while (ulExp--)
    {
        ulLast   = ulRet;
        ulRet  >>= 1;
    }
    if (ulLast & 0x00000001) ulRet++;

    return ulRet;
}

AIFFFileFormat::AIFFFileFormat() :
    m_lRefCount(0),
    m_pContext(NULL),
    m_pCommonClassFactory(NULL),
    m_pRequest(NULL),
    m_pFFResponse(NULL),
    m_pFileObject(NULL),
    m_pRiffReader(NULL),
    m_bHeaderSent(FALSE),
    m_ulPacketSize(PACKETSIZE),
    m_ulAvgBitRate(0),
    m_ulDuration(0),
    m_ulBytesSent(0),
    m_ulLastPacketEndTime(0),
    m_state(AS_Ready)
{
}

AIFFFileFormat::~AIFFFileFormat()
{
    HX_RELEASE(m_pContext);
    HX_RELEASE(m_pCommonClassFactory);
    HX_RELEASE(m_pRequest);
    HX_RELEASE(m_pFFResponse);
    if (m_pFileObject)
    {
	m_pFileObject->Close();
	HX_RELEASE(m_pFileObject);
    }
    HX_RELEASE(m_pRiffReader);
}

STDMETHODIMP
AIFFFileFormat::QueryInterface(REFIID riid, void**ppvObj)
{
    if(IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)(IHXPlugin*)this;
        return HXR_OK;
    }
    else if(IsEqualIID(riid, IID_IHXPlugin))
    {
        AddRef();
        *ppvObj = (IHXPlugin*)this;
        return HXR_OK;
    }
    else if(IsEqualIID(riid, IID_IHXFileFormatObject))
    {
        AddRef();
        *ppvObj = (IHXFileFormatObject*)this;
        return HXR_OK;
    }
    else if(IsEqualIID(riid, IID_IHXPendingStatus))
    {
        AddRef();
        *ppvObj = (IHXPendingStatus*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXInterruptSafe))
    {
	AddRef();
	*ppvObj = (IHXInterruptSafe*)this;
	return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(UINT32)
AIFFFileFormat::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(UINT32)
AIFFFileFormat::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

STDMETHODIMP
AIFFFileFormat::GetPluginInfo(REF(HXBOOL) bLoadMultiple,
			      REF(const char*) pDescription,
			      REF(const char*)pCopyright,
			      REF(const char*)pMoreInfoURL,
			      REF(ULONG32) ulVersionNumber)
{
    bLoadMultiple = TRUE;

    pDescription = zm_pDescription;
    pCopyright = zm_pCopyright;
    pMoreInfoURL = zm_pMoreInfoURL;
    ulVersionNumber = TARVER_ULONG32_VERSION;

    return HXR_OK;
}

STDMETHODIMP
AIFFFileFormat::InitPlugin(IUnknown* pContext)
{
    m_pContext = pContext;
    m_pContext->AddRef();

    if(m_pContext->QueryInterface(IID_IHXCommonClassFactory,
				  (void**)&m_pCommonClassFactory) != HXR_OK)
    {
	return HXR_UNEXPECTED;
    }

    return HXR_OK;
}

STDMETHODIMP
AIFFFileFormat::GetFileFormatInfo(REF(const char**)pFileMimeTypes,
				  REF(const char**) pFileExtensions,
				  REF(const char**)pFileOpenNames)
{
    pFileMimeTypes = (const char**) zm_pFileMimeTypes;
    pFileExtensions = (const char**) zm_pFileExtensions;
    pFileOpenNames = (const char**) zm_pFileOpenNames;
    return HXR_OK;
}

STDMETHODIMP
AIFFFileFormat::InitFileFormat(IHXRequest* pRequest,
			       IHXFormatResponse* pFormatResponse,
			       IHXFileObject* pFileObject)
{
    m_pRequest = pRequest;
    m_pFFResponse = pFormatResponse;
    m_pFileObject = pFileObject;

    if(m_pRequest) m_pRequest->AddRef();
    if(m_pFFResponse) m_pFFResponse->AddRef();
    if(m_pFileObject) m_pFileObject->AddRef();

    m_pRiffReader = new CRIFFReader(m_pContext,
				    this,
				    m_pFileObject);
    if(m_pRiffReader) m_pRiffReader->AddRef();
    m_state = AS_InitPending;
    const char* pURL;
    if(!m_pRequest || m_pRequest->GetURL(pURL) != HXR_OK)
    {
	return HXR_FAILED;
    }

    return m_pRiffReader->Open((char*)pURL);
}

STDMETHODIMP
AIFFFileFormat::Close()
{
    if(m_pRiffReader)
	m_pRiffReader->Close();

    if (m_pFileObject)
    {
	m_pFileObject->Close();
	HX_RELEASE(m_pFileObject);
    }

    return HXR_OK;
}

STDMETHODIMP
AIFFFileFormat::GetFileHeader()
{
    IHXValues* pHeader = 0;
    if(m_pCommonClassFactory->CreateInstance(CLSID_IHXValues,
					     (void**)&pHeader) != HXR_OK)
    {
	return HXR_UNEXPECTED;
    }

    pHeader->SetPropertyULONG32("StreamCount", 1);

	//XXXTRH: The new record implementation can record all audio
	// content on it's way out to the speaker. By enabling this
	// flag, the record feature will be enabled on the new record
	// implementation in the player. If using the old record
	// implementation, setting this flag will NOT enable the record
	// feature unless the plugin supports the
	// IID_IHXPacketHookHelper interface.
	// see if there are any audio streams, if so set the record flag
	pHeader->SetPropertyULONG32("Flags",HX_SAVE_ENABLED);

	m_pFFResponse->FileHeaderReady(HXR_OK, pHeader);
    pHeader->Release();
    return HXR_OK;
}

STDMETHODIMP
AIFFFileFormat::GetStreamHeader(UINT16 unStreamNumber)
{
    if(m_state != AS_Ready) return HXR_UNEXPECTED;
    m_state = AS_FindCommChunkPending;
    m_pRiffReader->FindChunk(COMM_CHUNK_ID, FALSE);
    return HXR_OK;
}

STDMETHODIMP
AIFFFileFormat::GetPacket(UINT16 unStreamNumber)
{
    if(m_state != AS_Ready) return HXR_UNEXPECTED;

    if(!m_bHeaderSent) return HXR_UNEXPECTED;

    if (m_ulLastPacketEndTime > m_ulDuration)
    {
        m_state = AS_Ready;
        m_pFFResponse->StreamDone(0);
        return HXR_OK;
    }

    m_state = AS_GetPacketReadPending;
    m_pRiffReader->Read(m_ulPacketSize);
    return HXR_OK;
}

STDMETHODIMP
AIFFFileFormat::Seek(UINT32 ulOffset)
{
    m_ulLastPacketEndTime = 0;
    m_ulSeekOffset = ulOffset;
    m_state = AS_SeekFindChunkPending;
    m_pRiffReader->FindChunk(SSND_CHUNK_ID, FALSE);
    return HXR_OK;
}

STDMETHODIMP
AIFFFileFormat::GetStatus(REF(UINT16) uStatusCode,
			  REF(IHXBuffer*)pStatusDesc,
			  REF(UINT16) ulPercentDone)
{
    HX_RESULT hResult = HXR_OK;
    IHXPendingStatus* pFileSystemStatus = NULL;

    if(m_pFileObject)
    {
	if(m_pFileObject->QueryInterface(IID_IHXPendingStatus,
					 (void**)&pFileSystemStatus) == HXR_OK)
	{
	    hResult = pFileSystemStatus->GetStatus(uStatusCode, pStatusDesc,
						   ulPercentDone);
	    pFileSystemStatus->Release();
	    return hResult;
	}
    }

    uStatusCode = HX_STATUS_READY;
    pStatusDesc = NULL;
    ulPercentDone = 0;

    return hResult;
}

// CRIFFResponse methods
STDMETHODIMP
AIFFFileFormat::RIFFOpenDone(HX_RESULT status)
{
    if(m_state != AS_InitPending) return HXR_UNEXPECTED;

    m_state = AS_Ready;
    m_pFFResponse->InitDone(HXR_OK);
    return HXR_OK;
}

STDMETHODIMP
AIFFFileFormat::RIFFCloseDone(HX_RESULT status)
{
    return HXR_OK;
}

HX_RESULT
AIFFFileFormat::RIFFFindChunkDone(HX_RESULT status, UINT32 len)
{
    UINT32 ulByteRate = 0;
    switch(m_state)
    {
	case AS_FindCommChunkPending:
	    if(status != HXR_OK)
	    {
		m_state = AS_Ready;
		m_pFFResponse->StreamHeaderReady(HXR_FAIL, NULL);
		return HXR_OK;
	    }

	    m_CommChunkLen = len;

	    m_state = AS_ReadCommChunkPending;
	    m_pRiffReader->Read(m_CommChunkLen);
	    break;
	case AS_FindSSNDChunkPending:
	    if(status != HXR_OK)
	    {
		m_state = AS_Ready;
		m_pFFResponse->StreamHeaderReady(HXR_FAIL, NULL);
	    }

	    IHXValues* pHeader;
	    IHXBuffer* pOpaque;
	    IHXBuffer* pMimeType;

	    if(m_pCommonClassFactory->CreateInstance(CLSID_IHXValues,
						     (void**)&pHeader)
	       != HXR_OK)
	    {
		m_state = AS_Ready;
		return HXR_UNEXPECTED;
	    }

	    if(m_pCommonClassFactory->CreateInstance(CLSID_IHXBuffer,
						     (void**)&pOpaque)
	       != HXR_OK)
	    {
		m_state = AS_Ready;
		return HXR_UNEXPECTED;
	    }

	    if(m_pCommonClassFactory->CreateInstance(CLSID_IHXBuffer,
						     (void**)&pMimeType)
	       != HXR_OK)
	    {
		m_state = AS_Ready;
		return HXR_UNEXPECTED;
	    }

	    AudioPCMHEADER audioPCMHEADER;
	    ULONG32 ulLenPacked;
	    m_AudioPCMHEADER.pack((UINT8*)&audioPCMHEADER, ulLenPacked);

	    pMimeType->Set((const UINT8*)"audio/x-pn-wav",
			   strlen("audio/x-pn-wav") + 1);
	    pOpaque->Set((UINT8*)&audioPCMHEADER, ulLenPacked);
	    pHeader->SetPropertyBuffer("OpaqueData", pOpaque);
	    pHeader->SetPropertyULONG32("StreamNumber", 0);
	    pHeader->SetPropertyULONG32("MaxBitRate", m_ulAvgBitRate);
	    pHeader->SetPropertyULONG32("AvgBitRate", m_ulAvgBitRate);
	    pHeader->SetPropertyULONG32("AvgPacketSize", m_ulPacketSize);
	    pHeader->SetPropertyULONG32("MaxPacketSize", m_ulPacketSize);
	    pHeader->SetPropertyULONG32("StartTime", 0);
	    pHeader->SetPropertyULONG32("Preroll", 1000);
	    pHeader->SetPropertyULONG32("Duration", m_ulDuration);
	    pHeader->SetPropertyCString("MimeType", pMimeType);

	    m_bHeaderSent = TRUE;
	    m_state = AS_Ready;
	    m_pFFResponse->StreamHeaderReady(HXR_OK, pHeader);
	    pHeader->Release();
	    pMimeType->Release();
	    pOpaque->Release();
	    break;
	case AS_SeekFindChunkPending:
	    m_state = AS_SeekPending;
        // Bitrate is always divisible by 8
        ulByteRate     = m_ulAvgBitRate >> 3;
        m_ulBytesSent  = (ulByteRate / 1000) * m_ulSeekOffset;
        m_ulBytesSent += (ulByteRate % 1000) * m_ulSeekOffset / 1000;
	    //We don't want to start on an odd byte or we end up with
	    // distorted sound:
	    if(m_ulBytesSent & 0x1)
	    {
		m_ulBytesSent++;
	    }
	    m_pRiffReader->Seek(m_ulBytesSent, TRUE);
	    break;
	default:
	    m_state = AS_Ready;
	    return HXR_UNEXPECTED;
    }
    return HXR_OK;
}

STDMETHODIMP
AIFFFileFormat::RIFFDescendDone(HX_RESULT status)
{
    return HXR_UNEXPECTED;
}

STDMETHODIMP
AIFFFileFormat::RIFFAscendDone(HX_RESULT status)
{
    return HXR_UNEXPECTED;
}

STDMETHODIMP
AIFFFileFormat::RIFFReadDone(HX_RESULT status, IHXBuffer* pBuffer)
{
    UCHAR* buf;
    UINT32 len;

    if(status == HXR_OK)
	pBuffer->Get(buf, len);

    switch(m_state)
    {
	case AS_ReadCommChunkPending:
	{
	    if(len != m_CommChunkLen || status != HXR_OK)
	    {
		m_state = AS_Ready;
		m_pFFResponse->StreamHeaderReady(HXR_FAIL, NULL);
		return HXR_UNEXPECTED;
	    }

	    m_AudioPCMHEADER.usVersion = 0;
	    m_AudioPCMHEADER.usMagicNumberTag = AUDIO_PCMHEADER_MAGIC_NUMBER;
	    m_AudioPCMHEADER.usChannels = getshort(&buf[0]);
	    m_AudioPCMHEADER.usBitsPerSample = getshort(&buf[6]);
	    m_AudioPCMHEADER.ulSamplesPerSec = IEEE754ToUINT32(&buf[8]);
	    m_AudioPCMHEADER.usSampleEndianness = 1;  // AIFF is big endian
	    m_AudioPCMHEADER.usFormatTag = 2; // 2's complement

            UINT32 ulNumberOfSamples = getlong(&buf[2]);
            // To compute duration, we must modularlize the
            // computation to avoid 32-bit overflow
            m_ulDuration = 5000;
            if (m_AudioPCMHEADER.ulSamplesPerSec)
            {
                m_ulDuration  = (ulNumberOfSamples / m_AudioPCMHEADER.ulSamplesPerSec) * 1000;
                m_ulDuration += (ulNumberOfSamples % m_AudioPCMHEADER.ulSamplesPerSec) * 1000 / m_AudioPCMHEADER.ulSamplesPerSec;
            }
	    m_ulAvgBitRate = m_AudioPCMHEADER.ulSamplesPerSec *
		             m_AudioPCMHEADER.usBitsPerSample *
			     m_AudioPCMHEADER.usChannels;
	    m_ulPacketSize = PACKETSIZE;

	    m_state = AS_FindSSNDChunkPending;
	    m_pRiffReader->FindChunk(SSND_CHUNK_ID, FALSE);
	    break;
	}
	case AS_GetPacketReadPending:
	{
	    if(status != HXR_OK)
	    {
		m_state = AS_Ready;
		m_pFFResponse->StreamDone(0);
		return HXR_OK;
	    }

	    m_state = AS_Ready;
	    IHXPacket* pPacket;
	    if(m_pCommonClassFactory->CreateInstance(CLSID_IHXPacket,
						     (void**)&pPacket)
	       != HXR_OK)
	    {
		m_state = AS_Ready;
		m_pFFResponse->StreamDone(0);
		return HXR_OK;
	    }
	    //We need to make sure not to exceed max UINT32 in the
	    // following calculation (or the time will be unexpectedly low),
	    // so we cast to float to make sure before casting back to
	    // UINT32:
	    UINT32 ulPacketTime = (UINT32) (float)(
		    ((double)m_ulBytesSent * 1000.0) /
		    ((double)m_ulAvgBitRate / 8.0));
	    pPacket->Set(pBuffer,
			 ulPacketTime,
			 0,
			 HX_ASM_SWITCH_ON,
			 0);
	    m_ulBytesSent += pBuffer->GetSize();
        m_ulLastPacketEndTime = (UINT32) (float)(
                                ((double)m_ulBytesSent * 1000.0) /
                                ((double)m_ulAvgBitRate / 8.0));
	    m_pFFResponse->PacketReady(HXR_OK, pPacket);
	    pPacket->Release();
	    break;
	}
	default:
	    m_state = AS_Ready;
	    return HXR_UNEXPECTED;
    }
    return HXR_OK;
}

STDMETHODIMP
AIFFFileFormat::RIFFSeekDone(HX_RESULT status)
{
    if(m_state != AS_SeekPending)
	return HXR_UNEXPECTED;

    m_state = AS_Ready;
    m_pFFResponse->SeekDone(HXR_OK);
    return HXR_OK;
}

STDMETHODIMP
AIFFFileFormat::RIFFGetChunkDone(HX_RESULT status,
				 UINT32 chunkType,
				 IHXBuffer* pBuffer)
{
    return HXR_NOTIMPL;
}
