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

#include "auflags.h"

#define FILE_HEADER_OFFSET  	0
#define TIME_PER_PACKET		20.0 /* ms */

#include <string.h>

#include "aufformat.ver"

#include "hxtypes.h"
#include "hxcom.h"
#include "audhead.h"
#include "hxcomm.h"
#include "netbyte.h"
#include "ihxpckts.h"
#include "hxfiles.h"
#include "hxformt.h"
#include "hxplugn.h"
#include "hxpends.h"
#include "rtptypes.h"
#include "hxengin.h"
#include "auffplin.h"
#include "hxstrutl.h"
#include "hxver.h"
#include "hxassert.h"

#include "rmfftype.h"	// for the PN_SAVE_ENABELED flag

#ifdef _DEBUG
#undef HX_THIS_FILE
static char HX_THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
 *
 *  Function:
 *
 *	HXCreateInstance()
 *
 *  Purpose:
 *
 *	Function implemented by all plugin DLL's to create an instance of
 *	any of the objects supported by the DLL. This method is similar to
 *	Window's CoCreateInstance() in its purpose, except that it only
 *	creates objects from this plugin DLL.
 *
 *	NOTE: Aggregation is never used. Therefore and outer unknown is
 *	not passed to this function, and you do not need to code for this
 *	situation.
 *
 */
HX_RESULT STDAPICALLTYPE CAUFileFormat::HXCreateInstance
(
    IUnknown**  /*OUT*/	ppIUnknown
)
{
    *ppIUnknown = (IUnknown*)(IHXPlugin*)new CAUFileFormat();
    if (*ppIUnknown)
    {
	(*ppIUnknown)->AddRef();
	return HXR_OK;
    }
    return HXR_OUTOFMEMORY;
}

HX_RESULT STDAPICALLTYPE CAUFileFormat::CanUnload2(void)
{
    return ((CHXBaseCountingObject::ObjectsActive() > 0) ? HXR_FAIL : HXR_OK);
}

const char* CAUFileFormat::zm_pDescription    = "RealNetworks AU File Format Plugin";
const char* CAUFileFormat::zm_pCopyright      = HXVER_RN_COPYRIGHT;
const char* CAUFileFormat::zm_pMoreInfoURL    = HXVER_MOREINFO;

#define DEFAULT_MIME	0
#define ULAW_MIME	1
#define L8_MIME		2
#define L16_MIME	3
#define G721_MIME	4
#define G722_MIME	5
#define G726_24_MIME	6
#define G726_40_MIME	7
#define ALAW_MIME	8

const char* CAUFileFormat::zm_pFileMimeTypes[]  = {"audio/x-pn-au",
"audio/PCMU",
"audio/L8",
"audio/L16",
"audio/G721",
"audio/G722",
"audio/G726-24",
"audio/G726-40",
"audio/PCMA",
"audio/basic",
NULL};

const char* CAUFileFormat::zm_pFileExtensions[] = {"au", NULL};
const char* CAUFileFormat::zm_pFileOpenNames[]  = {"AU Files (*.au)", NULL};
const char* CAUFileFormat::zm_pPacketFormats[] = {"rdt", "rtp", NULL};


CAUFileFormat::CAUFileFormat()
	: m_lRefCount(0)
	, m_pContext(NULL)
	, m_pFileObject(NULL)
        , m_pFileStat(NULL)
	, m_pFFResponse(NULL)
	, m_pCommonClassFactory(NULL)
	, m_bHeaderSent(FALSE)
	, m_state(Ready)
	, m_pRequest(NULL)
	, m_bSwap(FALSE)
	, m_ulDataOffset(0)
	, m_ulDataLength(0)
        , m_ulFileSize(0)
	, m_ulDataFormat(0)
	, m_ulHeaderOffset(0)
	, m_ulRate(0)
	, m_ulChannels(0)
	, m_ulEncodedBitsPerSample(0)
	, m_ulDecodedBitsPerSample(0)
	, m_bSendOpaqueData(TRUE)
	, m_packetFormat(PFMT_RDT)
	, m_bFirstGetPacket(TRUE)
	, m_fTimePerPacket(0.0)
	, m_ulPacketIdx(0)
{
};

CAUFileFormat::~CAUFileFormat()
{
};


/************************************************************************
 *  Method:
 *    IHXPlugin::InitPlugin
 *  Purpose:
 *    Initializes the plugin for use. This interface must always be
 *    called before any other method is called. This is primarily needed
 *    so that the plugin can have access to the context for creation of
 *    IHXBuffers and IMalloc.
 */
STDMETHODIMP CAUFileFormat::InitPlugin(IUnknown* /*IN*/ pContext)
{
    m_pContext = pContext;
    m_pContext->AddRef();
    m_pContext->QueryInterface(IID_IHXCommonClassFactory,(void**)&m_pCommonClassFactory);

    return HXR_OK;
}

/************************************************************************
 *  Method:
 *    IHXPlugin::GetPluginInfo
 *  Purpose:
 *    Returns the basic information about this plugin. Including:
 *
 *    bLoadMultiple	whether or not this plugin DLL can be loaded
 *			multiple times. All File Formats must set
 *			this value to TRUE.
 *    pDescription	which is used in about UIs (can be NULL)
 *    pCopyright	which is used in about UIs (can be NULL)
 *    pMoreInfoURL	which is used in about UIs (can be NULL)
 */
STDMETHODIMP CAUFileFormat::GetPluginInfo
(
    REF(HXBOOL)	    /*OUT*/ bLoadMultiple,
    REF(const char*)/*OUT*/ pDescription,
    REF(const char*)/*OUT*/ pCopyright,
    REF(const char*)/*OUT*/ pMoreInfoURL,
    REF(ULONG32)    /*OUT*/ ulVersionNumber
)
{
    bLoadMultiple = TRUE;   // Must be true for file formats.

    pDescription    = zm_pDescription;
    pCopyright	    = zm_pCopyright;
    pMoreInfoURL    = zm_pMoreInfoURL;
    ulVersionNumber = TARVER_ULONG32_VERSION;

    return HXR_OK;
}

/************************************************************************
 *  Method:
 *    IHXPlugin::GetObjFileFormatInfo
 *  Purpose:
 *    If this object is a file format object this method returns
 *    information vital to the instantiation of file format plugins.
 *    If this object is not a file format object, it should return
 *    HXR_UNEXPECTED.
 */
STDMETHODIMP CAUFileFormat::GetFileFormatInfo
(
    REF(const char**) /*OUT*/ pFileMimeTypes,
    REF(const char**) /*OUT*/ pFileExtensions,
    REF(const char**) /*OUT*/ pFileOpenNames
)
{
    pFileMimeTypes  = zm_pFileMimeTypes;
    pFileExtensions = zm_pFileExtensions;
    pFileOpenNames  = zm_pFileOpenNames;

    return HXR_OK;
}

// *** IUnknown methods ***

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::QueryInterface
//  Purpose:
//	Implement this to export the interfaces supported by your
//	object.
//
STDMETHODIMP CAUFileFormat::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
	AddRef();
	*ppvObj = this;
	return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXPlugin))
    {
	AddRef();
	*ppvObj = (IHXPlugin*)this;
	return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXFileFormatObject))
    {
	AddRef();
	*ppvObj = (IHXFileFormatObject*)this;
	return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXFileResponse))
    {
	AddRef();
	*ppvObj = (IHXFileResponse*)this;
	return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXFileStatResponse))
    {
	AddRef();
	*ppvObj = (IHXFileStatResponse*)this;
	return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXPendingStatus))
    {
	AddRef();
	*ppvObj = (IHXPendingStatus*)this;
	return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXPacketFormat))
    {
	AddRef();
	*ppvObj = (IHXPacketFormat*)this;
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

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::AddRef
//  Purpose:
//	Everyone usually implements this the same... feel free to use
//	this implementation.
//
STDMETHODIMP_(ULONG32) CAUFileFormat::AddRef()
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
STDMETHODIMP_(ULONG32) CAUFileFormat::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

// *** IHXFileFormatObject methods ***

STDMETHODIMP CAUFileFormat::InitFileFormat
(
    IHXRequest*	    /*IN*/  pRequest,
    IHXFormatResponse*	    /*IN*/  pFormatResponse,
    IHXFileObject*	    /*IN*/  pFileObject
)
{
    HX_RESULT resultInit = HXR_OK;
    m_pRequest    = pRequest;
    m_pFFResponse = pFormatResponse;
    m_pFileObject = pFileObject;

    if (m_pRequest)    m_pRequest->AddRef();
    if (m_pFFResponse) m_pFFResponse->AddRef();
    if (m_pFileObject) m_pFileObject->AddRef();

    // we want to make sure the file object is initialized, we can't
    // actually return the header count until the file init is done... (See InitDone).
    m_state = InitPending;
    m_ulPacketIdx = 0;

    // Note, we need to pass ourself to the FileObject, because this is its
    // first oppurtunity to know that we implement the IHXFileResponse
    // interface it will call for completed pending operations
    resultInit = m_pFileObject->Init(HX_FILE_READ | HX_FILE_BINARY, this);

    return resultInit;
}

STDMETHODIMP CAUFileFormat::Close()
{
    if (m_pContext) m_pContext->Release(); 	 m_pContext = 0;
    HX_RELEASE(m_pFileStat);
    if (m_pFileObject)
    {
	m_pFileObject->Close();
	m_pFileObject->Release();
	m_pFileObject = 0;
    }
    if (m_pFFResponse) m_pFFResponse->Release(); m_pFFResponse = 0;
    if (m_pCommonClassFactory)
	m_pCommonClassFactory->Release(); 	 m_pCommonClassFactory = 0;
    if (m_pRequest) m_pRequest->Release();	 m_pRequest = 0;
    m_bFirstGetPacket = TRUE;
    return HXR_OK;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IHXFileFormatObject::GetFileHeader
//  Purpose:
//	Called by controller to ask the file format for the number of
//	headers in the file. The file format should call the
//	IHXFormatResponse::StreamCountReady() for the IHXFileFormat-
//	Session object that was passed in during initialization, when the
//	header count is available.
//
STDMETHODIMP CAUFileFormat::GetFileHeader()
{
    HX_RESULT result = HXR_OK;

    // If we are not ready then something has gone wrong
    if (m_state != Ready)
	return HXR_UNEXPECTED;

    // Since au file format has not file header, we will set the
    // the state to the stream header seek pending and call FileHeaderReady()
    // with the stream count.

    // Create header object here, notice that we call the
    // CreateInstance method of the controller, but we could
    // have implemented our own object that exposed the IRMAHeader
    // interface.
    IHXValues* pHeader;
    if (HXR_OK == m_pCommonClassFactory->CreateInstance(CLSID_IHXValues,
                                                (void**)&pHeader))
    {
	// Only need to set the stream count.
	pHeader->SetPropertyULONG32("StreamCount",  1);

	//XXXTRH: The new record implementation can record all audio
	// content on it's way out to the speaker. By enabling this
	// flag, the record feature will be enabled on the new record
	// implementation in the player. If using the old record
	// implementation, setting this flag will NOT enable the record
	// feature unless the plugin supports the
	// IID_IHXPacketHookHelper interface.
	// see if there are any audio streams, if so set the record flag
	pHeader->SetPropertyULONG32("Flags",HX_SAVE_ENABLED);

	// Tell the FormatResponse of our success in
	// getting the header.
	result = m_pFFResponse->FileHeaderReady(HXR_OK, pHeader);
	pHeader->Release();
    }

    // See GetStreamHeader() for next "step".

    return result;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IHXFileFormatObject::GetStreamHeader
//  Purpose:
//	Called by controller to ask the file format for the header for
//	a particular stream in the file. The file format should call
//	IHXFormatResponse::StreamHeaderReady() for the IHXFormatResponse
//	object that was passed in during initialization, when the header
//	is available.
//
STDMETHODIMP CAUFileFormat::GetStreamHeader(UINT16 unStreamNumber)
{
    HX_RESULT retVal = HXR_OK;

    // If we are not ready then something has gone wrong
    if (m_state != Ready) return HXR_UNEXPECTED;

    // Get the IHXFileStat interface
    HX_RELEASE(m_pFileStat);
    retVal = m_pFileObject->QueryInterface(IID_IHXFileStat,
                                           (void**) &m_pFileStat);
    if (SUCCEEDED(retVal))
    {
        // Get our own stat response interface
        IHXFileStatResponse* pResponse = NULL;
        retVal = QueryInterface(IID_IHXFileStatResponse, (void**) &pResponse);
        if (SUCCEEDED(retVal))
        {
            // Set state
            m_state = GetStreamHeaderStatDonePending;
            // Stat the file
            retVal = m_pFileStat->Stat(pResponse);
        }
        HX_RELEASE(pResponse);
    }

    if (FAILED(retVal))
    {
        // Reset the state
        m_state = Ready;
	// Tell the FormatResponse of our complete and utter failure
	m_pFFResponse->StreamHeaderReady(HXR_FAIL, NULL);
    }

    return retVal;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IHXFileFormatObject::GetPacket
//  Purpose:
//	Called by controller to ask the file format for the next packet
//	for a particular stream in the file. The file format should call
//	IHXFormatResponse::PacketReady() for the IHXFormatResponse
//	object that was passed in during initialization, when the packet
//	is available.
//
STDMETHODIMP CAUFileFormat::GetPacket(UINT16 unStreamNumber)
{
    HX_RESULT result = HXR_OK;

    // If we are not ready then something has gone wrong
    if (m_state != Ready) return HXR_UNEXPECTED;

    if (!m_bHeaderSent)
    {
        result = HXR_UNEXPECTED;
    }
    else if (m_bFirstGetPacket)
    {
	m_state = GetPacketSeekPending;
	m_bFirstGetPacket = FALSE;
	m_pFileObject->Seek(m_ulDataOffset, FALSE);
    }
    else
    {
	m_state = GetPacketReadPending;

	// Actually read...
	m_pFileObject->Read(m_ulPacketSize);

	// See ReadDone() for next "step" of GetPacket()
    }

    return result;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IHXFileFormatObject::Seek
//  Purpose:
//	Called by controller to tell the file format to seek to the
//	nearest packet to the requested offset. The file format should
//	call IHXFormatResponse::SeekDone() for the IHXFileFormat-
//	Session object that was passed in during initialization, when
//	the seek has completed.
//
STDMETHODIMP CAUFileFormat::Seek(ULONG32 ulOffset)
{
    // If we are not ready then something has gone wrong
    // if (m_state != Ready) return HXR_UNEXPECTED;
    /* This was wrong.  We can be waiting for a Read request to
     * complete when we get a Seek request.  In which case, we should
     * drop whatever we were waiting for, and assume the filesystem
     * will do the same when it gets our seek request.
     */

    // Notice that the seek is passed as time in milliseconds, we
    // need to convert this to our packet number and its offset
    // in the file...
    ULONG32 ulPacket = (ULONG32) (ulOffset / m_fTimePerPacket + 0.5);
    ULONG32 ulPacketOffset = (ulPacket * m_ulPacketSize) + m_ulHeaderOffset;

    // To actually seek to the correct place in the time line,
    // we will seek in the file. We need to call the file object's
    // seek method.

    // Since this is asyncronous we need to note our state so we can
    // correctly respond to the seek complete response from the file
    // object.
    m_state = SeekSeekPending;

    // Note that the real time we are seeked to is not exactly what
    // was requested...
    m_ulPacketIdx = ulPacket;

    // Actually seek...
    m_pFileObject->Seek(ulPacketOffset,FALSE);

    // See SeekDone() for next "step" of the Seek() process.

    return HXR_OK;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//    IHXFileResponse::InitDone
//  Purpose:
//    Notification interface provided by users of the IHXFileObject
//    interface. This method is called by the IHXFileObject when the
//    initialization of the file is complete, and the Mime type is
//    available for the request file. If the URL is not valid for the
//    file system, the status HXR_FAILED should be returned,
//    with a mime type of NULL. If the URL is valid but the mime type
//    is unknown, then the status HXR_OK should be returned with
//    a mime type of NULL.
//
STDMETHODIMP CAUFileFormat::InitDone
(
    HX_RESULT	status
)
{
    // If we are not ready then something has gone wrong
    if (m_state != InitPending) return HXR_UNEXPECTED;

    // This simple file format is not a container type, so it only supports one
    // stream and therefore one header, since we now know the file is initialized
    // we can return the header count to the controller...
    m_state = Ready;

    HX_RESULT result = m_pFFResponse->InitDone(status);

    return result;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IHXFileResponse::CloseDone
//  Purpose:
//	Notification interface provided by users of the IHXFileObject
//	interface. This method is called by the IHXFileObject when the
//	close of the file is complete.
//
STDMETHODIMP CAUFileFormat::CloseDone(HX_RESULT status)
{
    return HXR_OK;
}

void
CAUFileFormat::doPacketCalculations()
{
    // check to see if there is a requested max size.
    UINT32 ulSize = 0;
    IHXValues* pValues = NULL;
    if (m_pRequest && m_pRequest->GetRequestHeaders(pValues) == HXR_OK)
    {
	if (pValues)
	{
	    IHXBuffer* pBuf = NULL;
	    if (SUCCEEDED(pValues->GetPropertyCString("blocksize", pBuf)))
	    {
		ulSize = atol((char*)pBuf->GetBuffer());
	    }
	    HX_RELEASE(pBuf);
	}
    }
    HX_RELEASE(pValues);

    m_fTimePerPacket = TIME_PER_PACKET;
    m_ulPacketSize = (ULONG32) (
			((double) m_ulRate) *
			((double) m_ulEncodedBitsPerSample) *
			((double) m_fTimePerPacket) *
			((double) m_ulChannels) /
			8000.0);

    if (ulSize && (ulSize < m_ulPacketSize))
    {
	// decrease the packet size to the maximum packet size...
	m_ulPacketSize = ulSize;
    }

    // Compute Per Packet Time based on integral sample size
    m_fTimePerPacket =	((double) m_ulPacketSize) *
			8000.0 /
			((double) m_ulRate) /
			((double) m_ulEncodedBitsPerSample) /
			((double) m_ulChannels);
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IHXFileResponse::ReadDone
//  Purpose:
//	Notification interface provided by users of the IHXFileObject
//	interface. This method is called by the IHXFileObject when the
//	last read from the file is complete and a buffer is available.
//
STDMETHODIMP CAUFileFormat::ReadDone
(
    HX_RESULT		status,
    IHXBuffer*		pBuffer
)
{
    HX_RESULT result = HXR_OK;
    UCHAR*  pBuf;
    UINT32  nBufLen;

    // Note, the read may be done because we needed to read to produce
    // a header or a packet. We need to remember why we tried to read and
    // respond accordingly...

    switch (m_state)
    {
	// There is no file header in the au file format.
	case GetStreamHeaderReadPending:
	{
            if (SUCCEEDED(status) && pBuffer != NULL)
	    {
		// assume dynamic payload unless determined differently
		// later on
		INT32 nRTPPayloadType = RTP_PAYLOAD_RTSP;

		// We are in the process of responding to a GetStreamHeader()
		// request and the read has completed, which means we
		// are done and can go back to the ready state...
		m_state = Ready;

		pBuffer->Get(pBuf, nBufLen);
		if(nBufLen < SUN_HDRSIZE )
		{
		    return HXR_UNEXPECTED;
		}

		// Read the .snd header info from the IHXBuffer
		UINT32 nMagic = 0;
		UINT32 nOffset = 0;
		memcpy(&nMagic, pBuf, 4); /* Flawfinder: ignore */
		nOffset += 4;
		if(nMagic == SUN_MAGIC || nMagic == DEC_MAGIC)
		{
		    if(TestBigEndian())
			m_bSwap = FALSE;
		    else
			m_bSwap = TRUE;
		}
		else if(nMagic == SUN_INV_MAGIC || nMagic == DEC_INV_MAGIC)
		{
		    if(TestBigEndian())
			m_bSwap = FALSE;
		    else
			m_bSwap = TRUE;
		}
		else
		{
		    return HXR_UNSUPPORTED_AUDIO;
		}
		memcpy(&m_ulDataOffset, &pBuf[nOffset], 4); /* Flawfinder: ignore */
		nOffset += 4;
		memcpy(&m_ulDataLength, &pBuf[nOffset], 4); /* Flawfinder: ignore */
		nOffset += 4;
		memcpy(&m_ulDataFormat, &pBuf[nOffset], 4); /* Flawfinder: ignore */
		nOffset += 4;
		memcpy(&m_ulRate, &pBuf[nOffset], 4); /* Flawfinder: ignore */
		nOffset += 4;
		memcpy(&m_ulChannels, &pBuf[nOffset], 4); /* Flawfinder: ignore */
		nOffset += 4;
		if(m_bSwap)
		{
		    SwapDWordBytes(&m_ulDataOffset, 1);
		    SwapDWordBytes(&m_ulDataLength, 1);
		    SwapDWordBytes(&m_ulDataFormat, 1);
		    SwapDWordBytes(&m_ulRate, 1);
		    SwapDWordBytes(&m_ulChannels, 1);
		}

		const char* pMimeType = NULL;

		switch(m_ulDataFormat)
		{
		    case SUN_ULAW:
		    {
			pMimeType = zm_pFileMimeTypes[ULAW_MIME];
			m_ulEncodedBitsPerSample = 8;
			m_ulDecodedBitsPerSample = 16;
			// XXXJHUG why only PCMU payload if it is 8khz
			///the spec says the payload has a variable sampling rate
			if(m_ulRate == 8000)
			{
			    nRTPPayloadType = RTP_PAYLOAD_PCMU;
			}

			// Never send opaque data
			m_bSendOpaqueData = FALSE;
		    }
		    break;

		    case SUN_LIN_8:
		    {
			pMimeType = zm_pFileMimeTypes[L8_MIME];
			m_ulEncodedBitsPerSample = 8;
			m_ulDecodedBitsPerSample = 8;
			// payload is dynamic - no change

			// Never send opaque data
			m_bSendOpaqueData = FALSE;
		    }
		    break;

		    case SUN_LIN_16:
		    {
			pMimeType = zm_pFileMimeTypes[L16_MIME];
			m_ulEncodedBitsPerSample = 16;
			m_ulDecodedBitsPerSample = 16;
			// XXXJHUG the spec says the payload has a variable
			// sampling rate, can't this be supported here?
			if(m_ulRate == 16000)
			{
			    if(m_ulChannels == 2)
				nRTPPayloadType = RTP_PAYLOAD_L16_2CH;
			    else if(m_ulChannels == 1)
				nRTPPayloadType = RTP_PAYLOAD_L16_1CH;
			}

			// Never send opaque data
			m_bSendOpaqueData = FALSE;
		    }
		    break;

		    case SUN_G721: // G726-32
			pMimeType = zm_pFileMimeTypes[G721_MIME];
			m_ulEncodedBitsPerSample = 4;
			m_ulDecodedBitsPerSample = 16;
			// XXXJHUG the spec says the payload has a variable
			// sampling rate, can't this be supported here?
			if (m_ulRate == 8000)
			{
			    nRTPPayloadType = RTP_PAYLOAD_G721;
			}
			break;
		    case SUN_G722:
			pMimeType = zm_pFileMimeTypes[G722_MIME];
			m_ulEncodedBitsPerSample = 4;
			m_ulDecodedBitsPerSample = 16;
			HX_ASSERT(m_ulRate == 16000);
			if (m_ulRate == 16000)
			{
			    nRTPPayloadType = RTP_PAYLOAD_G722;
			}
			break;
		    case SUN_G726_24:
			pMimeType = zm_pFileMimeTypes[G726_24_MIME];
			m_ulEncodedBitsPerSample = 3;
			m_ulDecodedBitsPerSample = 16;
			// dynamic rtp payload.
			break;
		    case SUN_G726_40:
			pMimeType = zm_pFileMimeTypes[G726_40_MIME];
			m_ulEncodedBitsPerSample = 5;
			m_ulDecodedBitsPerSample = 16;
			// dynamic rtp payload
			break;
		    case SUN_ALAW_8:
			pMimeType = zm_pFileMimeTypes[ALAW_MIME];
			m_ulEncodedBitsPerSample = 8;
			m_ulDecodedBitsPerSample = 16;
			// XXXJHUG the spec says the payload has a variable
			// sampling rate, can't this be supported here?
			if (m_ulRate== 8000)
			{
			    nRTPPayloadType = RTP_PAYLOAD_PCMA;
			}

			// Never send opaque data
			m_bSendOpaqueData = FALSE;
			break;
		    default:
		    {
			return HXR_UNSUPPORTED_AUDIO;
		    }
		}

		// figure out packet size
		doPacketCalculations();

		// We now need to form a "header" object and pass it
		// off to our controller. Notice since our header data
		// comes straight out of the file as is, we don't need
		// to copy data at all, the IHXBuffer returned by read
		// can be placed into the header object and passed off.

		// Create header object here, notice that we call the
		// CreateInstance method of the controller, but we could
		// have implemented our own object that exposed the IRMAHeader
		// interface.
		IHXValues* pHeader;
		IHXBuffer* pAuHeaderInfo = 0;
		IHXBuffer* pMTBuf = 0;
		IHXBuffer* pSNBuf = 0;
		if ((HXR_OK == m_pCommonClassFactory->CreateInstance(
			CLSID_IHXValues, (void**)&pHeader))  &&
		    (HXR_OK == m_pCommonClassFactory->CreateInstance(
			CLSID_IHXBuffer, (void**)&pAuHeaderInfo)) &&
		    (HXR_OK == m_pCommonClassFactory->CreateInstance(
			CLSID_IHXBuffer, (void**)&pSNBuf)) &&
		    (HXR_OK == m_pCommonClassFactory->CreateInstance(
			CLSID_IHXBuffer, (void**)&pMTBuf)))
		{
		    UINT16	uStreamNumber = 0;
		    ULONG32	ulMaxBitRate =  (ULONG32) (
						8000.0 *
						((double) m_ulPacketSize) /
						m_fTimePerPacket);
		    ULONG32	ulAvgBitRate = ulMaxBitRate;
		    ULONG32	ulMaxPacketSize = m_ulPacketSize;
		    ULONG32	ulAvgPacketSize = m_ulPacketSize;
		    ULONG32	ulStartTime = 0;
		    ULONG32	ulPreroll = 0;

		    /* Sometimes the data length in the AU header seems
		     * to be 0xFFFFFFFF, so in that case we will estimate
		     * the data length with filesize - offset */
		    UINT32 ulDataLength    = m_ulDataLength;
		    UINT32 ulMaxDataLength = m_ulFileSize - m_ulDataOffset;
		    if (ulDataLength > ulMaxDataLength)
		    {
			ulDataLength = ulMaxDataLength;
		    }
		    ULONG32 ulNumSamples = (ULONG32) (
						8.0 *
						(double) ulDataLength /
						(double) m_ulEncodedBitsPerSample);
		    ULONG32 ulDuration = (ULONG32) (
					    1000.0 *
					    (double) ulNumSamples /
					    (double) m_ulRate /
					    (double) m_ulChannels);

		    IHXBuffer*	pStreamName = NULL;

		    // Fill in the au header information
		    AuHeader auHeader;
		    auHeader.usFormatTag     = (UINT16) m_ulDataFormat;
		    auHeader.usChannels      = (UINT16) m_ulChannels;
		    auHeader.ulSamplesPerSec = m_ulRate;

		    if (TestBigEndian())
		    {
			// Set header info to be in little endian order
			SwapWordBytes( (UINT16*)&auHeader.usFormatTag, 1);
			SwapWordBytes( (UINT16*)&auHeader.usChannels, 1);
			SwapDWordBytes((UINT32*)&auHeader.ulSamplesPerSec, 1);
		    }
		    pAuHeaderInfo->Set((UCHAR*)&auHeader, (UINT32) sizeof(AuHeader));

		    // Fill in the Header with the relevant data...
		    if (m_bSendOpaqueData)
		    {
			pHeader->SetPropertyBuffer ("OpaqueData",    pAuHeaderInfo);
		    }
		    pHeader->SetPropertyULONG32("StreamNumber",  uStreamNumber);
		    pHeader->SetPropertyULONG32("MaxBitRate",    ulMaxBitRate);
		    pHeader->SetPropertyULONG32("AvgBitRate",    ulAvgBitRate);
		    pHeader->SetPropertyULONG32("MaxPacketSize", ulMaxPacketSize);
		    pHeader->SetPropertyULONG32("AvgPacketSize", ulAvgPacketSize);
		    pHeader->SetPropertyULONG32("StartTime",     ulStartTime);
		    pHeader->SetPropertyULONG32("Preroll",       ulPreroll);
		    pHeader->SetPropertyULONG32("Duration",      ulDuration);


		    if(m_packetFormat == PFMT_RTP)
		    {
			pHeader->SetPropertyULONG32("RTPPayloadType",
			    nRTPPayloadType);
		    }

		    if (!pMimeType)
		    {
			// Default mime type
			pMimeType = zm_pFileMimeTypes[DEFAULT_MIME];
		    }

		    UINT32 nBitsPerSample = m_ulDecodedBitsPerSample;
		    pHeader->SetPropertyULONG32("BitsPerSample", nBitsPerSample);
		    pHeader->SetPropertyULONG32("SamplesPerSecond", m_ulRate);
		    pHeader->SetPropertyULONG32("Channels", m_ulChannels);

		    pMTBuf->Set((const BYTE*)pMimeType, strlen(pMimeType)+1);
		    pSNBuf->Set((const BYTE*)pMimeType, strlen(pMimeType)+1);
		    pHeader->SetPropertyCString("MimeType",  pMTBuf);
		    pHeader->SetPropertyCString("StreamName", pSNBuf);

		    // Tell the FormatResponse of our success in
		    // getting the header.
		    m_bHeaderSent = TRUE;
		    m_bFirstGetPacket = TRUE;
		    m_pFFResponse->StreamHeaderReady(status, pHeader);

		    // Release our reference on the header!
		    pHeader->Release();
		    pAuHeaderInfo->Release();
		    pSNBuf->Release();
		    pMTBuf->Release();
		}
	    }
	    else
	    {
		// Reset the state
		m_state = Ready;
		// Tell the response interface
		status = m_pFFResponse->StreamHeaderReady(status, NULL);
	    }
	}
	break;

	case GetPacketReadPending:
	{
	    // We are in the process of responding to a GetPacket()
	    // request and the read has completed, which means we
	    // are done and can go back to the ready state...
	    m_state = Ready;

	    if (status == HXR_OK && pBuffer != NULL)
	    {
		// We now need to form a "packet" object and pass it
		// off to our controller. Notice since our packet data
		// comes straight out of the file as is, we don't need
		// to copy data at all, the IHXBuffer returned by read
		// can be placed into the packet object and passed off.

		// Create packet object here, notice that we call the
		// CreateInstance method of the controller, but we could
		// have implemented our own object that exposed the IHXPacket
		// interface.

		IHXRTPPacket* pPacket;
		double fCurrentTime = (m_ulPacketIdx++) *
				      m_fTimePerPacket;
		ULONG32 ulCurrentTime = (ULONG32) (fCurrentTime + 0.5);
		ULONG32 ulCurrentRTPTime = (ULONG32) (fCurrentTime *
						      m_ulRate /
						      1000.0 +
						      0.5);

		if (HXR_OK == m_pCommonClassFactory->CreateInstance(
		    CLSID_IHXRTPPacket, (void**)&pPacket))
		{
		    // Transform data if needed
		    switch(m_ulDataFormat)
		    {
		    case SUN_LIN_8:
			{
			    // Bring data into spec. for L8 via RTP as
			    // specified by RFC1890, section 4.4.8
			    IHXBuffer *pNewBuffer;

			    status = m_pCommonClassFactory->CreateInstance(
					    CLSID_IHXBuffer,
					    (void**) &pNewBuffer);

			    if (status == HXR_OK)
			    {
				UCHAR* pData;
				ULONG32 ulDataLen;
				UCHAR* pNewData;

				pBuffer->Get(pData, ulDataLen);

				status = pNewBuffer->SetSize(ulDataLen);
				if (status == HXR_OK)
				{
				    pNewData = pNewBuffer->GetBuffer();

				    while ((ulDataLen--) != 0)
				    {
					*(pNewData++) = (UCHAR)
							((*(pData++)) + 0x80);
				    }

				    pPacket->SetRTP(pNewBuffer,
						    ulCurrentTime,
						    ulCurrentRTPTime,
						    0,
						    HX_ASM_SWITCH_ON,
						    0);
				}

				pNewBuffer->Release();
			    }
			}
			break;

		    default:
			// Just pass on the read in buffer
			pPacket->SetRTP(pBuffer,
					ulCurrentTime,
					ulCurrentRTPTime,
					0,
					HX_ASM_SWITCH_ON,
					0);
			break;
		    }

		    // Tell the FormatResponse of our success in
		    // getting the packet.
		    m_pFFResponse->PacketReady(status, pPacket);

		    // Release our reference on the packet!
		    pPacket->Release();
		}
	    }
	    // If the read failed then we will call PacketReady() with
	    // a NULL packet!
	    else
	    {
		m_pFFResponse->StreamDone(0);
	    }
	}
	break;

	default:
	{
	    result = HXR_UNEXPECTED;
	}
	break;
    }

    return result;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IHXFileResponse::WriteDone
//  Purpose:
//	Notification interface provided by users of the IHXFileObject
//	interface. This method is called by the IHXFileObject when the
//	last write to the file is complete.
//
STDMETHODIMP CAUFileFormat::WriteDone(HX_RESULT status)
{
    // We don't ever write, so we don't expect to get this...
    return HXR_UNEXPECTED;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IHXFileResponse::SeekDone
//  Purpose:
//	Notification interface provided by users of the IHXFileObject
//	interface. This method is called by the IHXFileObject when the
//	last seek in the file is complete.
//
STDMETHODIMP CAUFileFormat::SeekDone(HX_RESULT status)
{
    /* This may happen in HTTP streaming when the file system
     * is in still a seeking mode when the next seek is issued.
     * The file system will then call SeekDone with a status of
     * HXR_CANCELLED for the pending seek.
     */
    if(status == HXR_CANCELLED)
    {
	return HXR_OK;
    }

    HX_RESULT result = HXR_OK;

    // Note, the seek may be done because we needed to seek to produce
    // a packet or because as a file format we were asked to seek to
    // a time position. We need to remember why we tried to seek and
    // respond accordingly...

    switch (m_state)
    {
	// There is no GetFileHeaderSeekPending since there is no file header.
	case GetStreamHeaderSeekPending:
	{
	    m_state = GetStreamHeaderReadPending;

	    // Actually read the stream header...
	    //m_pFileObject->Read(0);
	    result = m_pFileObject->Read(SUN_HDRSIZE);
	    // See ReadDone() for next "step" of GetStreamHeader()
	}
	break;

	case SeekSeekPending:
	{
	    // We are in the process of responding to a Seek()
	    // request and the seek has completed, so we are really
	    // done with the seek, so let's inform the FormatResponse.
	    m_state = Ready;

	    // Tell the FormatResponse of our success in seeking.
	    m_pFFResponse->SeekDone(status);
	}
	break;
	case GetPacketSeekPending:
	{
	    // We are responding to a GetPacket, we had to seek to the
	    // data offset.
	    m_state = GetPacketReadPending;

	    // Actually read...
	    m_pFileObject->Read(m_ulPacketSize);

	    // See ReadDone() for next "step" of GetPacket()
	}

	default:
	{
	    result = HXR_UNEXPECTED;
	}
	break;
    }

    return result;
}

/************************************************************************
 *	Method:
 *	    IHXFileResponse::FileObjectReady
 *	Purpose:
 *	    Notification interface provided by users of the IHXFileObject
 *	    interface. This method is called by the IHXFileObject when the
 *	    requested FileObject is ready. It may return NULL with
 *	    HXR_FAIL if the requested filename did not exist in the
 *	    same pool.
*/
STDMETHODIMP
CAUFileFormat::FileObjectReady
(
    HX_RESULT	    status,
    IHXFileObject* pFileObject
)
{
    return HXR_OK;
}

/************************************************************************
 *	Method:
 *	    IHXPendingStatus::GetStatus
 *	Purpose:
 *	    Called by the user to get the current pending status from an object
 */
STDMETHODIMP
CAUFileFormat::GetStatus
(
    REF(UINT16) uStatusCode,
    REF(IHXBuffer*) pStatusDesc,
    REF(UINT16) ulPercentDone
)
{
    HX_RESULT hResult = HXR_OK;
    IHXPendingStatus* pFileSystemStatus = NULL;

    // asking status information from the file system object
    if (m_pFileObject)
    {
	if (HXR_OK == m_pFileObject->QueryInterface(IID_IHXPendingStatus,(void**)&pFileSystemStatus))
	{
	    hResult = pFileSystemStatus->GetStatus(uStatusCode, pStatusDesc, ulPercentDone);

	    pFileSystemStatus->Release();
	    return hResult;
	}
    }

    // by default
    uStatusCode = HX_STATUS_READY;
    pStatusDesc = NULL;
    ulPercentDone = 0;

    return hResult;
}

STDMETHODIMP CAUFileFormat::StatDone(HX_RESULT status,
                                     UINT32    ulSize,
                                     UINT32    ulCreationTime,
                                     UINT32    ulAccessTime,
                                     UINT32    ulModificationTime,
                                     UINT32    ulMode)
{
    HX_RESULT retVal = HXR_UNEXPECTED;

    if (m_state == GetStreamHeaderStatDonePending)
    {
        // We don't need the stat interface any more
        HX_RELEASE(m_pFileStat);
        // Save the file size
        if (SUCCEEDED(status))
        {
            m_ulFileSize = ulSize;
        }
        // To give the header we need to be make sure the file is positioned
        // at the start of the file. We need to call the file object's
        // seek method.

        // Since this is asyncronous we need to note our state so we can
        // correctly respond to the seek complete response from the file
        // object.
        m_state = GetStreamHeaderSeekPending;

        // Actually seek...
//    m_pFileObject->Seek(m_ulDataOffset,FALSE);

        retVal = m_pFileObject->Seek(FILE_HEADER_OFFSET,FALSE);
        // See SeekDone() for next "step" of the GetStreamHeader process.
    }

    return retVal;
}

/************************************************************************
 *      Method:
 *          IHXPacketFormat::GetSupportedPacketFormats
 *      Purpose:
 *          Obtains a list of packet formats supported by this file format
 */
STDMETHODIMP
CAUFileFormat::GetSupportedPacketFormats
(
    REF(const char**) /*OUT*/ pPacketFormats
)
{
    pPacketFormats = zm_pPacketFormats;
    return HXR_OK;
}

/************************************************************************
 *      Method:
 *          IHXPacketFormat::SetPacketFormat
 *      Purpose:
 *          Sets the packet type for this file format
 */
STDMETHODIMP
CAUFileFormat::SetPacketFormat
(
    const char* pPacketFormat
)
{
    if(strcasecmp(pPacketFormat, "rtp") == 0)
    {
        m_packetFormat = PFMT_RTP;
    }
    else
    {
        m_packetFormat = PFMT_RDT;
    }
    return HXR_OK;
}
