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
#include <string.h>
#ifndef _WINCE
#include <signal.h>
#endif

#include "wvffplin.ver"

// XXX
#ifdef WIN32
#include "windows.h"
#include "mmreg.h"
#else
// from "mmreg.h"
/*
 * Careful: these constants are endian dependent
 */
#define WAVE_FORMAT_PCM         0x0001
#define WAVE_FORMAT_ALAW        0x0006
#define WAVE_FORMAT_MULAW       0x0007
#define WAVE_FORMAT_G723_ADPCM  0x0014
#define WAVE_FORMAT_GSM610      0x0031
#define WAVE_FORMAT_G721_ADPCM  0x0040
#endif

#define TIME_PER_PACKET	20 /* ms */

#include "hxwavtp.h"

#include "hxcomm.h"
#include "audhead.h"
#include "legacy.h"
#include "ihxpckts.h"
#include "hxfiles.h"
#include "hxformt.h"
#include "hxplugn.h"
#include "hxprefs.h"
#include "hxrendr.h"
#include "hxformt.h"
#include "hxpends.h"
#include "hxengin.h"
#include "hxslist.h"
#include "smppayld.h"
#include "rtptypes.h"
#include "netbyte.h"
#include "hxmarsh.h"
#include "hxstrutl.h"
#include "baseobj.h"
#include "wvffplin.h"
#include "hxver.h"

#ifdef HELIX_FEATURE_AUDIO_MPA_LAYER3
#include "mpapayld.h"
#include "mp3draft.h"
#endif

#include "rmfftype.h"	// for the HX_SAVE_ENABLED flag
#include "hxassert.h"

#define WAV_FMT_CHUNK_ID            0x666D7420              // 'fmt'
#define WAV_DATA_CHUNK_ID           0x64617461              // 'data'
#define WAV_LIST_OBJECT		    0x4c495354		    // 'LIST'
#define WAV_INFO_CHUNK		    0x494e464f		    // 'INFO' 
#define WAV_TITLE_CHUNK		    0x494e414d		    // 'INAM'
#define WAV_AUTHOR_CHUNK	    0x49415254		    // 'IART'
#define WAV_COPYRIGHT_CHUNK	    0x49434f50		    // 'ICOP'
                      
#define WAV_SUBTYPE_MAGIC_NUMBER    0x45564157              // 'WAVE'

#define PREFERED_PACKET_SIZE	500
#define RTP_PACKET_SIZE	    160
#define MIN_PREROLL_PACKETS	2

#include "hxheap.h"
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
HX_RESULT STDAPICALLTYPE CWaveFileFormat::HXCreateInstance
(
    IUnknown**  /*OUT*/	ppIUnknown
)
{
    *ppIUnknown = (IUnknown*)(IHXPlugin*)new CWaveFileFormat();
    if (*ppIUnknown)
    {
	(*ppIUnknown)->AddRef();
	return HXR_OK;
    }
    return HXR_OUTOFMEMORY;
}

HX_RESULT STDAPICALLTYPE CWaveFileFormat::CanUnload2(void)
{
    return ((CHXBaseCountingObject::ObjectsActive() > 0) ? HXR_FAIL : HXR_OK);
}

const char* CWaveFileFormat::zm_pDescription    = "Helix WAVE File Format Plugin";
const char* CWaveFileFormat::zm_pCopyright      = HXVER_COPYRIGHT;
const char* CWaveFileFormat::zm_pMoreInfoURL    = HXVER_MOREINFO;

const char* CWaveFileFormat::zm_pFileMimeTypes[]  = {"audio/x-pn-wav", "audio/x-wav", "audio/wav", "audio/wave", NULL};
const char* CWaveFileFormat::zm_pFileExtensions[] = {"wav", NULL};
const char* CWaveFileFormat::zm_pFileOpenNames[]  = {"WAVE Files (*.wav)", NULL};
const char* CWaveFileFormat::zm_pPacketFormats[]  = {"rdt", "rtp", NULL};

CWaveFileFormat::CWaveFileFormat()
                : m_lRefCount(0)
                , m_pContext(NULL)
		, m_pClassFactory(NULL)
                , m_pFileObject(NULL)
                , m_pFFResponse(NULL)
		, m_pRiffReader(NULL)
		, m_pFormatBuffer(NULL)
                , m_bHeaderSent(FALSE)
                , m_ulCurrentTime(0)
                , m_state(WS_Ready)
                , m_pRequest(NULL)
		, m_pszTitle(NULL)
		, m_pszAuthor(NULL)
		, m_pszCopyright(NULL)
		, m_packetFormat(PFMT_RTP)
		, m_pPayloadFormat(NULL)
		, m_bSwapSamples(FALSE)
{
}

CWaveFileFormat::~CWaveFileFormat()
{
    HX_VECTOR_DELETE(m_pszTitle);
    HX_VECTOR_DELETE(m_pszAuthor);
    HX_VECTOR_DELETE(m_pszCopyright);

    HX_RELEASE(m_pFormatBuffer);
    HX_RELEASE(m_pRiffReader);
    HX_RELEASE(m_pContext);
    HX_RELEASE(m_pClassFactory);

    if (m_pFileObject)
    {
	m_pFileObject->Close();
	HX_RELEASE(m_pFileObject);
    }

    HX_RELEASE(m_pFFResponse); 
    HX_RELEASE(m_pRequest); 
}

/************************************************************************
 *  Method:
 *    IHXPlugin::InitPlugin
 *  Purpose:
 *    Initializes the plugin for use. This interface must always be
 *    called before any other method is called. This is primarily needed 
 *    so that the plugin can have access to the context for creation of
 *    IHXBuffers and IMalloc.
 */
STDMETHODIMP CWaveFileFormat::InitPlugin(IUnknown* /*IN*/ pContext)
{
    m_pContext = pContext;
    m_pContext->AddRef();

    m_pContext->QueryInterface(IID_IHXCommonClassFactory, 
	(void**)&m_pClassFactory);

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
STDMETHODIMP CWaveFileFormat::GetPluginInfo
(
    REF(HXBOOL)        /*OUT*/ bLoadMultiple,
    REF(const char*) /*OUT*/ pDescription,
    REF(const char*) /*OUT*/ pCopyright,
    REF(const char*) /*OUT*/ pMoreInfoURL,
    REF(ULONG32)     /*OUT*/ ulVersionNumber
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
STDMETHODIMP CWaveFileFormat::GetFileFormatInfo
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
STDMETHODIMP CWaveFileFormat::QueryInterface(REFIID riid, void** ppvObj)
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
STDMETHODIMP_(ULONG32) CWaveFileFormat::AddRef()
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
STDMETHODIMP_(ULONG32) CWaveFileFormat::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

// *** IHXFileFormatObject methods ***

STDMETHODIMP CWaveFileFormat::InitFileFormat
(
    IHXRequest*	    /*IN*/  pRequest, 
    IHXFormatResponse*	    /*IN*/  pFormatResponse,
    IHXFileObject*	    /*IN*/  pFileObject
)
{
    m_pRequest    = pRequest;
    m_pFFResponse = pFormatResponse;
    m_pFileObject = pFileObject;

    if (m_pRequest)	m_pRequest->AddRef();
    if (m_pFFResponse)	m_pFFResponse->AddRef();
    if (m_pFileObject)	m_pFileObject->AddRef();

    m_pRiffReader = new CRIFFReader(m_pContext,
				    this,
				    m_pFileObject);

    if (m_pRiffReader) m_pRiffReader->AddRef();

    m_ulBytesSent = 0;

    // This simple file format is not a container type, so it only supports one
    // stream and therefore one header, but before, we do that, we want to
    // make sure the file object is initialized, we can't actually return
    // the header count until the file init is done... (See InitDone).
    m_state = WS_InitPending;

    // Note, we need to pass ourself to the FileObject, because this is its
    // first oppurtunity to know that we implement the IHXFileResponse
    // interface it will call for completed pending operations.  We're going
    // through the RIFF reader as an intermediary.

    const char* pURL;

    if (!m_pRequest || m_pRequest->GetURL(pURL) != HXR_OK)
    {
	return HXR_FAILED;
    }

    return m_pRiffReader->Open((char*)pURL);
}

STDMETHODIMP CWaveFileFormat::Close()
{
    HX_VECTOR_DELETE(m_pszTitle);
    HX_VECTOR_DELETE(m_pszAuthor);
    HX_VECTOR_DELETE(m_pszCopyright);

    if (m_pRiffReader)
    {
	m_pRiffReader->Close();
    }

    HX_RELEASE(m_pContext);

    if (m_pFileObject)
    {
	m_pFileObject->Close();
	HX_RELEASE(m_pFileObject);
    }

    HX_RELEASE(m_pFFResponse);
    HX_RELEASE(m_pRequest);
    HX_RELEASE(m_pPayloadFormat);

    return HXR_OK;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IHXFileFormatObject::GetFileHeader
//  Purpose:
//	Called by controller to ask the file format for the number of
//	headers in the file. The file format should call the 
//	IHXFileFormatSession::HeaderCountReady() for the IHXFileFormat-
//	Session object that was passed in during initialization, when the
//	header count is available.
//
STDMETHODIMP CWaveFileFormat::GetFileHeader()
{
    IHXBuffer* pTitle = NULL;
    IHXBuffer* pAuthor = NULL;
    IHXBuffer* pCopyright = NULL;
    
    // If we are not ready then something has gone wrong
    if (m_state != WS_Ready) return HXR_UNEXPECTED;


    IHXValues* pHeader = 0;
    if (HXR_OK != m_pClassFactory->CreateInstance(CLSID_IHXValues, 
	(void**)&pHeader))
    {
	return HXR_UNEXPECTED;
    }

    pHeader->SetPropertyULONG32("StreamCount", 1);

    if (HXR_OK == m_pClassFactory->CreateInstance(CLSID_IHXBuffer,
	    (void**)&pTitle)   &&
	HXR_OK == m_pClassFactory->CreateInstance(CLSID_IHXBuffer,
	    (void**)&pAuthor)  &&
	HXR_OK == m_pClassFactory->CreateInstance(CLSID_IHXBuffer,
	    (void**)&pCopyright))
    {    
	// set TAC properties to the file header
	if (m_pszTitle)	    
	{
	    pTitle->Set((const UCHAR*)m_pszTitle, strlen(m_pszTitle)+1);
	    pHeader->SetPropertyBuffer("Title", pTitle);
	}
	    
	if (m_pszAuthor)	    
	{
	    pAuthor->Set((const UCHAR*)m_pszAuthor, strlen(m_pszAuthor)+1);
	    pHeader->SetPropertyBuffer("Author", pAuthor);
	}

	if (m_pszCopyright)  
	{
	    pCopyright->Set((const UCHAR*)m_pszCopyright, strlen(m_pszCopyright)+1);
	    pHeader->SetPropertyBuffer("Copyright", pCopyright);
	}
    }
	    
	//XXXTRH: The new record implementation can record all audio
	// content on it's way out to the speaker. By enabling this 
	// flag, the record feature will be enabled on the new record
	// implementation in the player. If using the old record 
	// implementation, setting this flag will NOT enable the record
	// feature unless the plugin supports the 
	// IID_IHXPacketHookHelper interface.
	pHeader->SetPropertyULONG32("Flags",HX_SAVE_ENABLED);

    m_pFFResponse->FileHeaderReady(HXR_OK, pHeader);

    if (pTitle)	    pTitle->Release();
    if (pAuthor)    pAuthor->Release();
    if (pCopyright) pCopyright->Release();

    if (pHeader)	pHeader->Release();

    return HXR_OK;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IHXFileFormatObject::GetStreamHeader
//  Purpose:
//	Called by controller to ask the file format for the header for
//	a particular stream in the file. The file format should call 
//	IHXFileFormatSession::StreamHeaderReady() for the IHXFileFormatSession
//	object that was passed in during initialization, when the header
//	is available.
//
STDMETHODIMP CWaveFileFormat::GetStreamHeader(UINT16 unStreamNumber)
{
    // If we are not ready then something has gone wrong
    if (m_state != WS_Ready) return HXR_UNEXPECTED;

    m_state = WS_FindFmtChunkPending;
    return m_pRiffReader->FindChunk(WAV_FMT_CHUNK_ID, FALSE);
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IHXFileFormatObject::GetPacket
//  Purpose:
//	Called by controller to ask the file format for the next packet
//	for a particular stream in the file. The file format should call 
//	IHXFileFormatSession::PacketReady() for the IHXFileFormatSession
//	object that was passed in during initialization, when the packet
//	is available.
//
STDMETHODIMP CWaveFileFormat::GetPacket(UINT16 unStreamNumber)
{
    HX_RESULT hResult = HXR_OK;
    IHXPacket* pPacket = NULL;

    // If we are not ready then something has gone wrong
    if (m_state != WS_Ready) 
    {
  	return HXR_UNEXPECTED;
    }
    else if (!m_bHeaderSent)
    {
  	return HXR_UNEXPECTED;
    }
    HX_ASSERT(m_pPayloadFormat);

    hResult = m_pPayloadFormat->GetPacket(pPacket);
    if (HXR_OK == hResult)
    {
	m_pFFResponse->PacketReady(HXR_OK, pPacket);
	HX_RELEASE(pPacket);
	return HXR_OK;
    }
    else if (HXR_STREAM_DONE == hResult)
    {
	m_pFFResponse->StreamDone(0);
	return HXR_OK;
    }
    else
    {
	// The payload formatter needs more data,
	// so proceed with the normal reading state
	HX_ASSERT(HXR_INCOMPLETE == hResult);
    }

    if (m_ulBytesSent >= m_ulDataSizeInBytes)
    {
	m_pFFResponse->StreamDone(0);
	return HXR_OK;
    }

    if (m_ulDataSizeInBytes - m_ulBytesSent < m_ulPacketSize)
    {
	m_ulPacketSize = m_ulDataSizeInBytes - m_ulBytesSent;
    }

    // We are being asked to get the next packet and we have sent
    // the header, so basically we know the next m_ulPacketSize bytes
    // should be the next packet... again, since read is asynchronous, 
    // we need to set up our state...
    m_state = WS_GetPacketReadPending;

    // Actually read...
    // See RIFFReadDone() for next "step" of GetPacket()
    m_pRiffReader->Read(m_ulPacketSize); 

    return HXR_OK;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IHXFileFormatObject::Seek
//  Purpose:
//	Called by controller to tell the file format to seek to the 
//	nearest packet to the requested offset. The file format should 
//	call IHXFileFormatSession::SeekDone() for the IHXFileFormat-
//	Session object that was passed in during initialization, when 
//	the seek has completed.
//
STDMETHODIMP CWaveFileFormat::Seek(ULONG32 ulOffset)
{
    // Notice that the seek is passed as time in milliseconds, we
    // need to convert this to our packet number and its offset
    // in the file...
    UINT32 ulPacketOffset = (UINT32) (((double)ulOffset*(double)m_ulAvgBytesPerSec)/1000.);
    UINT32 ulSeekBlockOffset = (ulPacketOffset/m_ulBlockAlign)*m_ulBlockAlign;

    // Since this is asyncronous we need to note our state so we can
    // correctly respond to the seek complete response from the file
    // object.
    m_state = WS_SeekSeekPending;

    // Note that the real byte offset we will seek to.
    m_ulBytesSent = ulSeekBlockOffset;

    // Discard all buffered data
    // from the payload formatter
    m_pPayloadFormat->Reset();

    // Actually seek...
    // See RIFFSeekDone() for next "step" of the Seek() process.
    m_pRiffReader->Seek(ulSeekBlockOffset, FALSE);

    return HXR_OK;
}

/************************************************************************
 *	Method:
 *	    IHXPendingStatus::GetStatus
 *	Purpose:
 *	    Called by the user to get the current pending status from an object
 */
STDMETHODIMP
CWaveFileFormat::GetStatus
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

/************************************************************************
 *	Method:
 *	    IHXPacketFormat::GetSupportedPacketFormats
 *	Purpose:
 *	    Obtains a list of packet formats supported by this file format
 */
STDMETHODIMP CWaveFileFormat::GetSupportedPacketFormats
(
    REF(const char**) /*OUT*/ pPacketFormats
)
{
    pPacketFormats = zm_pPacketFormats;
    return HXR_OK;
}

/************************************************************************
 *	Method:
 *	    IHXPacketFormat::SetPacketFormat
 *	Purpose:
 *	    Sets the packet type for this file format
 */
STDMETHODIMP CWaveFileFormat::SetPacketFormat
(
    const char* pPacketFormat
)
{
    return HXR_OK;
}

STDMETHODIMP
CWaveFileFormat::RIFFOpenDone(HX_RESULT status)
{
    // If we are not ready then something has gone wrong
    if (m_state != WS_InitPending) return HXR_UNEXPECTED;
    
    if (m_pRiffReader->FileType() != RIFF_FILE_MAGIC_NUMBER ||
        m_pRiffReader->FileSubtype() != WAV_SUBTYPE_MAGIC_NUMBER ||
        status != HXR_OK)
    {
        
        m_pFFResponse->InitDone(status == HXR_OK ? HXR_INVALID_FILE : status);
        return HXR_OK;
    }

    // To give the header we need to be make sure the file is positioned
    // at the start of the file. We need to call the file object's
    // seek method (or, since we're dealing with a RIFF file we can use
    // our built-in RIFF reading library).

    // Since this is asyncronous we need to note our state so we can
    // correctly respond to the seek complete response from the file
    // object.
    m_state = WS_DescendPending;

    // Actually seek...
    // See RIFFDescendDone() for next "step" of the GetStreamHeader process.
    return m_pRiffReader->Descend();
}

STDMETHODIMP
CWaveFileFormat::RIFFCloseDone(HX_RESULT)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP
CWaveFileFormat::RIFFFindChunkDone(HX_RESULT status, UINT32 len)
{
    HX_RESULT retVal = HXR_OK;
    
    // we've found either the 'fmt' or 'data' chunk of the wave file.
    switch(m_state)
    {
    case WS_FindFmtChunkPending:
	m_state = WS_ReadFmtChunkPending;
	m_ulFormatChunkLen = len;

	return m_pRiffReader->Read(len);

    case WS_FindINFOChunkPending:
	if (status != HXR_OK)
	{
	    m_state = WS_Ready;
	    return m_pFFResponse->InitDone(HXR_OK);
	}
	
	if (m_pRiffReader->GetListType() == WAV_INFO_CHUNK)
	{
	    m_state = WS_INFODescendPending;
	    m_pRiffReader->Descend();
	    return HXR_OK;
	}

	m_pRiffReader->FindChunk(WAV_LIST_OBJECT, TRUE);
	return HXR_OK;
    case WS_FindINAMChunkPending:
	if (status != HXR_OK)
	{
	    m_state = WS_FindIARTChunkPending; 
	    return m_pRiffReader->FindChunk(WAV_AUTHOR_CHUNK, FALSE);
	}

	m_ulThisTACLen = len;
	m_state = WS_ReadINAMChunkPending;
	m_pRiffReader->Read(m_ulThisTACLen);

	return HXR_OK;
     case WS_FindIARTChunkPending:
	if (status != HXR_OK)
	{
	    m_state = WS_FindICOPChunkPending; 
	    return m_pRiffReader->FindChunk(WAV_COPYRIGHT_CHUNK, FALSE);
	}

	m_ulThisTACLen = len;
	m_state = WS_ReadIARTChunkPending;
	m_pRiffReader->Read(m_ulThisTACLen);

	return HXR_OK;
    case WS_FindICOPChunkPending:
	if (status != HXR_OK)
	{
	    m_state = WS_INFOAscendPending;
	    return m_pRiffReader->Ascend();
	}
	m_ulThisTACLen = len;
	m_state = WS_ReadICOPChunkPending;
	m_pRiffReader->Read(m_ulThisTACLen);

	return HXR_OK;
    case WS_FindDataChunkPending:
    {
	m_state = WS_Ready;

	IHXValues* pHeader;
	IHXBuffer* pHeaderBuf;
	IHXBuffer* pMTBuf;
	int nRTPPayloadType = 101;

	if ((HXR_OK == m_pClassFactory->CreateInstance(CLSID_IHXValues, 
		(void**)&pHeader)) &&
	    (HXR_OK == m_pClassFactory->CreateInstance(CLSID_IHXBuffer, 
		(void**)&pHeaderBuf)) &&
	    (HXR_OK == m_pClassFactory->CreateInstance(CLSID_IHXBuffer, 
		(void**)&pMTBuf)))
	{
	    UCHAR* rawbuf = m_pFormatBuffer->GetBuffer();

	    //TRUE_SIZEOF_WAVEHEADER is used instead of sizeof(WAVEHEADER)
	    // because, on some os's like Solaris, byte-packing causes
	    // the sizeof(WAVEHEADER) to return 20 (or 24) instead of 18:
	    UINT16 usCbSize = 0;
	    if(!rawbuf  ||
		    //Some wav files have WAVEHEADERs that are missing
		    // the usCbSize element and we should handle that
		    // gracefully but return error if we got less than
		    // the size expected minus the usCbSize element:
		    m_pFormatBuffer->GetSize() <
		    (TRUE_SIZEOF_WAVEHEADER-sizeof(usCbSize)) )
	    {
		pMTBuf->Release();
		pHeader->Release();
		pHeaderBuf->Release();
		retVal = HXR_UNEXPECTED;
		m_pFFResponse->InitDone(retVal);
		break;
	    }

	    HXBOOL bCbSizeElementExists =
		    m_pFormatBuffer->GetSize() >= TRUE_SIZEOF_WAVEHEADER;

  	    // wav files are in intel-endian format.
  	    HXBOOL bBigEndian = TestBigEndian(); //False if Intel
 	    UINT16 uWord = 0;
 	    UINT32 ulDWord = 0;
 	    UCHAR* ptr = rawbuf;
 	    UINT16 usFormatTag  = 0;
 	    UINT16 nChannels      = 0;
 	    UINT32 nSamplesPerSec = 0;
 	    UINT16 nBitsPerSample = 0;
 
  	    // Note: We will send out the data in packets of about 500 bytes
  	    // based on the blockAlign required by the file format. Note,
  	    // this works for uncompressed (PCM) style wav files and, compress
  	    // (ACM) style wave files as well.
  
 	    // Create a packet size that represents about 50 ms of audio
 	    usFormatTag = getshort(ptr); ptr += 2;
 	    SwapWordBytes(&usFormatTag, 1);
 	    nChannels = getshort(ptr); ptr += 2;
 	    SwapWordBytes(&nChannels, 1);
 	    nSamplesPerSec = getlong(ptr); ptr += 4;
 	    SwapDWordBytes(&nSamplesPerSec, 1);
 	    m_ulAvgBytesPerSec = getlong(ptr); ptr += 4;
 	    SwapDWordBytes(&m_ulAvgBytesPerSec, 1);
 	    // The only thing we care about here is the block align. This
 	    // will be the sample size for any PCM wave files, or some crazy
 	    // number for compressed wave files.
 	    uWord = getshort(ptr); ptr += 2;
 	    SwapWordBytes(&uWord, 1);
 	    m_ulBlockAlign = (UINT32)uWord;
 	    nBitsPerSample = getshort(ptr); ptr += 2;
 	    SwapWordBytes(&nBitsPerSample, 1);
  	    //Make sure enough buffer arrived for this element of the
  	    // WAVEHEADER struct (and, if not, usCbSize was already
  	    // initialized to 0):
  	    if(bCbSizeElementExists)
  	    {
 		usCbSize = getshort(ptr); ptr += 2;
 		SwapWordBytes(&usCbSize, 1);
  	    }

	    // Some basic sanity checking. This will populate absent fields for avgbytespersec and nchannels,
	    // correct potentially wrong values for nbitspersample, and fix the sample rate if it is incorrect.
	    // These calculations require 3 of the 4 above fields to be available

	    if( m_ulAvgBytesPerSec == 0 )
	    {
		// Try to calculate this to avoid a divide by zero
		m_ulAvgBytesPerSec = (nChannels * nSamplesPerSec * nBitsPerSample) / 8;

		if( m_ulAvgBytesPerSec == 0 )
		{
		    // Corrupt header
		    pMTBuf->Release();
		    pHeader->Release();
		    pHeaderBuf->Release();
		    retVal = HXR_INVALID_FILE;
		    m_pFFResponse->InitDone(retVal);
		    break;
		}
	    }

		HXBOOL bIsBitsPerSampleCalculated = FALSE;

	    // We assume that 0 or values not divisible by 8 are probably incorrect
	    if( nBitsPerSample == 0 || (nBitsPerSample % 8) )
	    {
		UINT32 nDiv = nSamplesPerSec * nChannels;

		if( nDiv == 0 )
		{
		    // Corrupt header
		    pMTBuf->Release();
		    pHeader->Release();
		    pHeaderBuf->Release();
		    retVal = HXR_INVALID_FILE;
		    m_pFFResponse->InitDone(retVal);
		    break;
		}
		nBitsPerSample = (UINT16) ((8 * m_ulAvgBytesPerSec) / nDiv);
		bIsBitsPerSampleCalculated = TRUE;
	    }

	    if( nChannels == 0 )
	    {
		UINT32 nDiv = (nSamplesPerSec * nBitsPerSample) / 8;

		// nDiv cannot be zero due to above checks
		nChannels = (UINT16) (m_ulAvgBytesPerSec / nDiv);
	    }

		// there is no point in calculating the samplerPerSec deviation
		// if the bitsPerSample was calculated earlier using 
		// m_ulAvgBytesPerSec, nChannels, nSamplesPerSec since the
		// resulting value should come out to be the same as nSamplesPerSec.
		// In certain cases where bitsPerSample gets rounded off to the nearest integer value
		// this calculation was resulting in erroneous calculation of nExpectedSamplesPerSec 
		// and re-setting of nSamplesPerSec to a wrong value.
		// Fixes issue with .wav file with MP3 content 
		if (!bIsBitsPerSampleCalculated)
		{
	    // Attempt to correct bad sample rates. The logic here is that if our calculated
	    // sample rate is more than 10% off from the provided one, the provided one is probably
	    // incorrect. The calculated one uses the average bitrate, meaning that it may not be
	    // exact, but this is part of a "best effort"
	    UINT32 nExpectedSamplesPerSec = ((m_ulAvgBytesPerSec / nChannels)*8) / nBitsPerSample;
	    UINT32 nDeviation = (nExpectedSamplesPerSec > nSamplesPerSec ?
		nExpectedSamplesPerSec-nSamplesPerSec :
		nSamplesPerSec-nExpectedSamplesPerSec);

	    if( nDeviation > (nSamplesPerSec / 10) )
	    {
		nSamplesPerSec = nExpectedSamplesPerSec;
	    }
		}
	    
	    m_ulDataSizeInBytes = len;
	    m_ulAvgBitRate      = 8 * m_ulAvgBytesPerSec;

	    // The duration is of course calculatable from the size of
	    // the data chunk in the same manner.
	    ULONG32	ulDuration = (UINT32)(1000.0 *
		    ((double)m_ulDataSizeInBytes / 
		     (double)m_ulAvgBytesPerSec));

	    const char* pMimeType = NULL;
	    char	szMimeTypePCM[] = "audio/x-pn-wav";
	    char	szMimeTypeACM[] = "audio/x-pn-windows-acm";

	    doPacketSizeCalculations(nSamplesPerSec, nChannels, nBitsPerSample);

	    if (usFormatTag != WAVE_FORMAT_PCM)
	    {
		/////////////////////////////////////////////////////
		// This wave file is not PCM and must be converted //
		// by the ACM renderer                             //
		/////////////////////////////////////////////////////
		switch(usFormatTag)
		{
		    case WAVE_FORMAT_ALAW:
		    {
			pMimeType = "audio/PCMA";
			if(m_packetFormat == PFMT_RTP)
			{
			    pMimeType = "audio/pcma";
			    nRTPPayloadType = RTP_PAYLOAD_PCMA;
			}

			// the sample output from a law is 16bits
			nBitsPerSample = 16;
		    }
		    break;

		    case WAVE_FORMAT_MULAW:
		    {
			pMimeType = "audio/PCMU";
			if(m_packetFormat == PFMT_RTP)
			{
			    nRTPPayloadType = RTP_PAYLOAD_PCMU;
			}
			// the sample output from mu law is 16bits
			nBitsPerSample = 16;
		    }
		    break;

		    case WAVE_FORMAT_G723_ADPCM:
		    {
			pMimeType = "audio/x-pn-g723";
			if(m_packetFormat == PFMT_RTP)
			{
			    nRTPPayloadType = RTP_PAYLOAD_G723;
			}
		    }
		    break;

		    case WAVE_FORMAT_G721_ADPCM:
		    {
			pMimeType = "audio/x-pn-g721";
			if(m_packetFormat == PFMT_RTP)
			{
			    nRTPPayloadType = RTP_PAYLOAD_G721;
			}
		    }
		    break;

		    case WAVE_FORMAT_GSM610:
		    {
			pMimeType = "audio/x-pn-gsm610";
			if(m_packetFormat == PFMT_RTP)
			{
			    nRTPPayloadType = RTP_PAYLOAD_GSM;
			}
		    }
		    break;

#ifdef HELIX_FEATURE_AUDIO_MPA_LAYER3
		    case WAVE_FORMAT_MP3:
		    {
			// Format MP3 data according to the live.com
			// internet draft payload specification
			/*
			pMimeType = "audio/X-MP3";
			HX_RELEASE(m_pPayloadFormat);
			m_pPayloadFormat = new MP3DraftPayloadFormat();
			m_pPayloadFormat->AddRef();
			m_pPayloadFormat->Init(m_pContext, TRUE);

			nRTPPayloadType = 101; // Dynamic payload type
			*/

			// Format MP3 data according to the RFC 2250
			// MPEG audio payload specification
			pMimeType = "audio/MPA";
			HX_RELEASE(m_pPayloadFormat);
			m_pPayloadFormat = new MPAPayloadFormat();
			m_pPayloadFormat->AddRef();
			m_pPayloadFormat->Init(m_pContext, TRUE);

			nRTPPayloadType = RTP_PAYLOAD_MPA;

			if (m_packetFormat == PFMT_RTP)
			{
			    m_ulPacketSize = RTP_PACKET_SIZE;
			}
		    }
		    break;
#endif
                    
		    default:
		    {
			pMimeType = szMimeTypeACM;
		    }
		    break;
		}

		// Set the mime type
		pMTBuf->Set((const BYTE*)pMimeType, strlen(pMimeType)+1);

		if (pHeaderBuf)
		{
		    AudioWAVEHEADER tmpAudioWAVHEADER, audioWAVHEADER;
		    tmpAudioWAVHEADER.usVersion = 0;
		    tmpAudioWAVHEADER.usMagicNumberTag =
			    AUDIO_WAVHEADER_MAGIC_NUMBER;
		    tmpAudioWAVHEADER.usFormatTag = usFormatTag;
		    tmpAudioWAVHEADER.usChannels = nChannels;
		    tmpAudioWAVHEADER.ulSamplesPerSec = nSamplesPerSec;
		    tmpAudioWAVHEADER.ulAvgBytesPerSec = m_ulAvgBytesPerSec;
    		    tmpAudioWAVHEADER.usBlockAlign = (UINT16)m_ulBlockAlign;
		    tmpAudioWAVHEADER.usBitsPerSample = nBitsPerSample;
		    tmpAudioWAVHEADER.usCbSize = usCbSize;

    		    //ACM headers are now always in net-endian order, so
		    // pack the header to make sure it's net-endian:
		    UINT32 ulLen = 0L;
    		    tmpAudioWAVHEADER.pack((UINT8*)&audioWAVHEADER, ulLen);

		    //In case byte alignment is not 1, use this instead of
		    // sizeof(WAVEHEADER) or you might get diff results;
		    // (4 is size of usVersion + size of usMagicNumberTag):
		    UINT32 ulSizeOfWAVEHEADER =
			    tmpAudioWAVHEADER.static_size() - 4;

		    IHXBuffer* pNewBuf = NULL;
		    if(usCbSize  &&
			    m_pFormatBuffer->GetSize() > ulSizeOfWAVEHEADER)
		    {
			pHeaderBuf->SetSize(ulLen + (ULONG32)usCbSize);
			UCHAR* pTmp = (UCHAR*)pHeaderBuf->GetBuffer();
			memcpy(pTmp, &audioWAVHEADER, ulLen); /* Flawfinder: ignore */
			ULONG32 ulLenRemainderOfBuf =
				m_pFormatBuffer->GetSize();
			if(ulLenRemainderOfBuf > usCbSize)
			{
			    ulLenRemainderOfBuf = usCbSize;
			}
			memcpy(pTmp + ulLen, rawbuf + ulSizeOfWAVEHEADER, /* Flawfinder: ignore */
				ulLenRemainderOfBuf);
		    }
		    else
		    {
			pHeaderBuf->Set((UCHAR*)&audioWAVHEADER,
				ulLen + usCbSize);
		    }
		}
	    }
	    else
	    {   
		///////////////////////////////
		// This wave file is raw PCM //
		///////////////////////////////

		// Don't set any opaque data
		HX_RELEASE(pHeaderBuf);

		// Set appropriate mime type
		if (nBitsPerSample == 8)
		{
		    pMimeType = "audio/L8";
		}
		else if (nBitsPerSample == 16)
		{
		    pMimeType = "audio/L16";

		    // Remember to make all data net-endian
		    m_bSwapSamples = TRUE;
		}
		else
		{
		    // Default mime type
		    pMimeType = szMimeTypePCM;
		}
		pMTBuf->Set((const BYTE*)pMimeType, strlen(pMimeType)+1);

		// Offer an RTP payload type if compatible
		if (nBitsPerSample == 16 &&
		    nSamplesPerSec == 44100 &&
		    m_packetFormat == PFMT_RTP)
		{
		    if (nChannels == 1)
		    {
			nRTPPayloadType = RTP_PAYLOAD_L16_1CH;
		    }
		    else if (nChannels == 2)
		    {
			nRTPPayloadType = RTP_PAYLOAD_L16_2CH;
		    }
		}
	    }
	    // For Pre-roll, we will assume we need at least 2 packets
	    // Notice that we don't ever store time per packet because
	    // of round off rammifications, but we do sometimes calculate
	    // it at times like this...
	    ULONG32	ulPreroll = (UINT32)(1000.0 *
		    ((double)(MIN_PREROLL_PACKETS * m_ulPacketSize) / 
		     (double)m_ulAvgBytesPerSec));

	    // Make sure preroll is at least 2 seconds
	    if (ulPreroll < 2000)
	    {
		ulPreroll = 2000;
	    }

	    // Fill in the Header with the relevant data...
	    if (pHeaderBuf)
	    {
		pHeader->SetPropertyBuffer ("OpaqueData",    pHeaderBuf);
	    }
	    pHeader->SetPropertyULONG32("StreamNumber",  0);
	    pHeader->SetPropertyULONG32("MaxBitRate",    m_ulAvgBitRate);
	    pHeader->SetPropertyULONG32("AvgBitRate",    m_ulAvgBitRate);
	    pHeader->SetPropertyULONG32("MaxPacketSize", m_ulPacketSize);
	    pHeader->SetPropertyULONG32("AvgPacketSize", m_ulPacketSize);
	    pHeader->SetPropertyULONG32("StartTime",     0);
	    pHeader->SetPropertyULONG32("Preroll",       ulPreroll);
	    pHeader->SetPropertyULONG32("Duration",      ulDuration);
	    pHeader->SetPropertyCString("MimeType",      pMTBuf);
	    pHeader->SetPropertyULONG32("BitsPerSample", (UINT32)nBitsPerSample);
	    pHeader->SetPropertyULONG32("SamplesPerSecond", (UINT32)nSamplesPerSec);
	    pHeader->SetPropertyULONG32("Channels",	 (UINT32)nChannels);

	    if(m_packetFormat == PFMT_RTP)
	    {
		pHeader->SetPropertyULONG32("RTPPayloadType",
		    (UINT32)nRTPPayloadType);
	    }

	    m_bHeaderSent = TRUE;
	    m_ulHeaderOffset = m_pRiffReader->GetOffset();
	    
	    // Make sure we have a payload formatter by now.
	    // If not, create a simple payload formatter that
	    // will leave all packets as they are
	    if (!m_pPayloadFormat)
	    {
		m_pPayloadFormat = new SimplePayloadFormat();
		m_pPayloadFormat->AddRef();
		m_pPayloadFormat->Init(m_pContext, TRUE);
	    }

	    if (SUCCEEDED(status))
	    {
		// Allow the payload formatter to adjust
		// the stream header however it wants
		m_pPayloadFormat->SetStreamHeader(pHeader);
		HX_RELEASE(pHeader);

		status = m_pPayloadFormat->GetStreamHeader(pHeader);
	    }
	    m_pFFResponse->StreamHeaderReady(status, pHeader);

	    HX_RELEASE(pMTBuf);
	    HX_RELEASE(pHeader);
	    HX_RELEASE(pHeaderBuf);
	}
	else
	{
	    return HXR_OUTOFMEMORY;
	}
	return HXR_OK;
    }
    default:
	m_state = WS_Ready;
	return HXR_UNEXPECTED;
    }
	
    return retVal;
}

void
CWaveFileFormat::doPacketSizeCalculations(UINT32 nSamplesPerSec, 
					  UINT16 nChannels,
					  UINT16 nBitsPerSample)
{
    IHXValues* pValues = NULL;
    UINT32 ulSize = (ULONG32) (((double) nSamplesPerSec) * 
			       ((double) nBitsPerSample) *
			       ((double) TIME_PER_PACKET) *
			       ((double) nChannels) / 
			       1000.0 / 
			       8.0);
    
#ifdef XXXJHUG_USE_MAX_PACKET_SIZE
    // make sure the set size isn't too big...
    if (ulSize > PREFERED_PACKET_SIZE)
    {
	ulSize = PREFERED_PACKET_SIZE;
    }
#endif	// XXXJHUG_USE_MAX_PACKET_SIZE

    if (m_pRequest && m_pRequest->GetRequestHeaders(pValues) == HXR_OK)
    {
	if (pValues)
	{
	    pValues->GetPropertyULONG32("blocksize", ulSize);
	}
    }

    if (ulSize)
    {
	// override the prefered packet size if there is a block size.
	ULONG32 ulRoundUp = 0;
	if((m_ulBlockAlign > 0) &&
	   ((ulSize / m_ulBlockAlign) < (((float) ulSize) / ((float) m_ulBlockAlign))))
	{
	    //Only add 1 if there was a remainder in the division:
	    ulRoundUp = 1;
	}
	m_ulPacketSize = ((ulSize / m_ulBlockAlign) + ulRoundUp) * m_ulBlockAlign;
    }

    HX_RELEASE(pValues);
}

STDMETHODIMP
CWaveFileFormat::RIFFDescendDone(HX_RESULT status)
{
    switch (m_state)
    {
    case WS_DescendPending:
	m_state = WS_FindINFOChunkPending;
	return (m_pRiffReader->FindChunk(WAV_LIST_OBJECT, FALSE));
    case WS_INFODescendPending:
	if(status != HXR_OK)
	{
	    m_state = WS_Ready;
	    m_pFFResponse->InitDone(HXR_FAILED);
	    return HXR_OK;
	}

	m_state = WS_FindINAMChunkPending;
	return (m_pRiffReader->FindChunk(WAV_TITLE_CHUNK, FALSE));
    default:
	m_state = WS_Ready;
	return HXR_UNEXPECTED;
    }
}

STDMETHODIMP
CWaveFileFormat::RIFFAscendDone(HX_RESULT status)
{
   switch(m_state)
    {
    case WS_INFOAscendPending:
	m_state = WS_Ready;
	m_pFFResponse->InitDone(HXR_OK);
	return HXR_OK;
    default:
	m_state = WS_Ready;
	return HXR_UNEXPECTED;
    }
}

STDMETHODIMP
CWaveFileFormat::RIFFReadDone(HX_RESULT status, IHXBuffer* pBuffer)
{
    HX_RESULT hResult = HXR_OK;
    IHXPacket* pPacket = NULL;
    UCHAR* buf;
    UINT32 len;

    if (HXR_OK != status)
    {
	m_state = WS_Ready;
    
        if (!m_pPayloadFormat)
        {
           m_pFFResponse->InitDone(HXR_FAILED);
            return HXR_OK;
        }

        if (!m_pPayloadFormat)
        {
           m_pFFResponse->InitDone(HXR_FAILED);
           return HXR_OK;
        }

	// We've read the whole file, so tell the payload
	// formatter that it's not getting any more data
	m_pPayloadFormat->Flush();

	hResult = m_pPayloadFormat->GetPacket(pPacket);
	if (HXR_OK == hResult)
	{
	    m_pFFResponse->PacketReady(HXR_OK, pPacket);
	}
	else
	{
	    m_pFFResponse->StreamDone(0);
	}

	HX_RELEASE(pPacket);
	return HXR_OK;
    }

    pBuffer->Get(buf, len);

    // we've either read the 'fmt' chunk or a portion of the data, our
    // state tells us which is the case
    switch(m_state)
    {
	case WS_ReadFmtChunkPending:
	{
	    if(len != m_ulFormatChunkLen)
	    {
		m_state = WS_Ready;
		return HXR_UNEXPECTED;
	    }

	    m_pFormatBuffer = pBuffer;
	    m_pFormatBuffer->AddRef();

	    m_state = WS_FindDataChunkPending;
	    return m_pRiffReader->FindChunk(WAV_DATA_CHUNK_ID, FALSE);
	}
	case WS_ReadINAMChunkPending:
	{
    	    if((status != HXR_OK) ||
	       (len != m_ulThisTACLen))
	    {
		m_state = WS_Ready;
		m_pFFResponse->InitDone(HXR_FAILED);
		return HXR_OK;
	    }
	    
	    m_pszTitle = new CHAR[len+1];
	    if (m_pszTitle)
	    {
		memcpy(m_pszTitle, (const char*)buf, len); /* Flawfinder: ignore */
		m_pszTitle[len] = '\0';
	    }

	    m_state = WS_FindIARTChunkPending;
	    return m_pRiffReader->FindChunk(WAV_AUTHOR_CHUNK, FALSE);
	}
	case WS_ReadIARTChunkPending:
	{
    	    if((status != HXR_OK) ||
	       (len != m_ulThisTACLen))
	    {
		m_state = WS_Ready;
		m_pFFResponse->InitDone(HXR_FAILED);
		return HXR_OK;
	    }
	    
	    m_pszAuthor = new CHAR[len+1];
	    if (m_pszAuthor)
	    {
		memcpy(m_pszAuthor, (const char*)buf, len); /* Flawfinder: ignore */
		m_pszAuthor[len] = '\0';
	    }

	    m_state = WS_FindICOPChunkPending;
	    return m_pRiffReader->FindChunk(WAV_COPYRIGHT_CHUNK, FALSE);
	}
	case WS_ReadICOPChunkPending:
	{
    	    if((status != HXR_OK) ||
	       (len != m_ulThisTACLen))
	    {
		m_state = WS_Ready;
		m_pFFResponse->InitDone(HXR_FAILED);
		return HXR_OK;
	    }
	    
	    m_pszCopyright = new CHAR[len+1];
	    if (m_pszCopyright)
	    {
		memcpy(m_pszCopyright, (const char*)buf, len); /* Flawfinder: ignore */
		m_pszCopyright[len] = '\0';
	    }

	    m_state = WS_INFOAscendPending;
	    m_pRiffReader->Ascend();

	    return HXR_OK;
	}
	case WS_GetPacketReadPending:
	{
	    if(len != m_ulPacketSize)
	    {
		m_state = WS_Ready;

		// We've read the whole file, so tell the payload
		// formatter that it's not getting any more data
		m_pPayloadFormat->Flush();

		hResult = m_pPayloadFormat->GetPacket(pPacket);
		if (HXR_OK == hResult)
		{
		    m_pFFResponse->PacketReady(HXR_OK, pPacket);
		}
		else
		{
		    m_pFFResponse->StreamDone(0);
		}
		HX_RELEASE(pPacket);

		return HXR_OK;
	    }

	    m_state = WS_Ready;

	    if (HXR_OK == m_pClassFactory->CreateInstance(CLSID_IHXPacket, 
		    (void**)&pPacket))
	    {
		// Packet time is always recalced based on bytes per second
		// information from the header. This prevents errors in time
		// drift.
		UINT32 ulPacketTime = (UINT32)
		    (1000. * (double) m_ulBytesSent/(double)m_ulAvgBytesPerSec);

		// AddRef() this just in case we replace it below
		pBuffer->AddRef();

		// If this is L16 PCM data, always convert 
		// it to net endian (big endian) format
		if (m_bSwapSamples)
		{
		    // Replace the buffer with one that we can change
		    IHXBuffer* pNewBuffer = NULL;
		    if (HXR_OK == m_pClassFactory->CreateInstance(CLSID_IHXBuffer, 
			(void**)&pNewBuffer))
		    {
			pNewBuffer->Set(pBuffer->GetBuffer(), pBuffer->GetSize());
			HX_RELEASE(pBuffer);
			pBuffer = pNewBuffer;

			UCHAR* pSampleBuf = pBuffer->GetBuffer();
    			SwapWordBytes((UINT16 *)pSampleBuf, pBuffer->GetSize() / 2);
		    }
		}

		if (m_ulPacketSize % m_ulBlockAlign)
		{
		    // Align the packet with block align field.
		    IHXBuffer* pNewBuffer = NULL;
		    if (HXR_OK == m_pClassFactory->CreateInstance(CLSID_IHXBuffer, 
			(void**)&pNewBuffer))
		    {
			if (HXR_OK == pNewBuffer->SetSize (m_ulPacketSize + (m_ulBlockAlign -
				(m_ulPacketSize % m_ulBlockAlign))))
			{
			    memcpy (pNewBuffer->GetBuffer (),
				    pBuffer->GetBuffer (),
				    m_ulPacketSize);

			    memset (pNewBuffer->GetBuffer () + m_ulPacketSize,
				    0,
				    (m_ulBlockAlign - (m_ulPacketSize % m_ulBlockAlign)));

			    HX_RELEASE(pBuffer);
			    pBuffer = pNewBuffer;
			}
			else
			{
			    HX_RELEASE(pNewBuffer);
			}
		    }
		}

		// Fill in the Packet with the relevant data...
		pPacket->Set(pBuffer,ulPacketTime,0,HX_ASM_SWITCH_ON,0);
		HX_RELEASE(pBuffer);

		// How often do we deliver packets?
		m_ulBytesSent += m_ulPacketSize;

		// Hand the packet to the payload formatter
		m_pPayloadFormat->SetPacket(pPacket);
		HX_RELEASE(pPacket);

		hResult = m_pPayloadFormat->GetPacket(pPacket);
		if (HXR_OK == hResult)
		{
		    // Tell the FormatResponse of our success in 
		    // getting the packet.
		    m_pFFResponse->PacketReady(status, pPacket);
		}
		else if (HXR_INCOMPLETE == hResult)
		{
		    // The payload formatter needs more input before it can give
		    // us a packet so we need to read again
		    m_state = WS_GetPacketReadPending;

		    // Actually read...
		    // See RIFFReadDone() for next "step" of GetPacket()
		    m_pRiffReader->Read(m_ulPacketSize); 
		}
		else
		{
		    HX_ASSERT(HXR_STREAM_DONE == hResult);
		    m_pFFResponse->StreamDone(0);
		}
		
		// Release our reference on the packet!
		HX_RELEASE(pPacket);  	
	    }
	    return HXR_OK;
	}

	default:
	{
	    m_state = WS_Ready;
	    return HXR_UNEXPECTED;
	}
    }
}

STDMETHODIMP
CWaveFileFormat::RIFFSeekDone(HX_RESULT status)
{
    if(m_state != WS_SeekSeekPending || status != HXR_OK)
    {
	return HXR_UNEXPECTED;
    }

    m_state = WS_Ready;
    return m_pFFResponse->SeekDone(status);
}

STDMETHODIMP
CWaveFileFormat::RIFFGetChunkDone(HX_RESULT status,
				  UINT32 chunkType,
				  IHXBuffer* pBuffer)
{   
    return HXR_NOTIMPL;
}

