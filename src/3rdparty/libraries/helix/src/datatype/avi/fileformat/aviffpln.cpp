/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id:$ 
 *   
 * Portions Copyright (c) 1995-2004 RealNetworks, Inc. All Rights Reserved.  
 *       
 * The contents of this file, and the files included with this file, 
 * are subject to the current version of the RealNetworks Public 
 * Source License (the "RPSL") available at 
 * http://www.helixcommunity.org/content/rpsl unless you have licensed 
 * the file under the current version of the RealNetworks Community 
 * Source License (the "RCSL") available at 
 * http://www.helixcommunity.org/content/rcsl, in which case the RCSL 
 * will apply. You may also obtain the license terms directly from 
 * RealNetworks.  You may not use this file except in compliance with 
 * the RPSL or, if you have a valid RCSL with RealNetworks applicable 
 * to this file, the RCSL.  Please see the applicable RPSL or RCSL for 
 * the rights, obligations and limitations governing use of the 
 * contents of the file. 
 *   
 * Alternatively, the contents of this file may be used under the 
 * terms of the GNU General Public License Version 2 or later (the 
 * "GPL") in which case the provisions of the GPL are applicable 
 * instead of those above. If you wish to allow use of your version of 
 * this file only under the terms of the GPL, and not to allow others  
 * to use your version of this file under the terms of either the RPSL 
 * or RCSL, indicate your decision by deleting the provisions above 
 * and replace them with the notice and other provisions required by 
 * the GPL. If you do not delete the provisions above, a recipient may 
 * use your version of this file under the terms of any one of the 
 * RPSL, the RCSL or the GPL. 
 * 
 * This file is part of the Helix DNA Technology. RealNetworks is the 
 * developer of the Original Code and owns the copyrights in the 
 * portions it created. 
 *   
 * This file, and the files included with this file, is distributed 
 * and made available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY 
 * KIND, EITHER EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS 
 * ALL SUCH WARRANTIES, INCLUDING WITHOUT LIMITATION, ANY WARRANTIES 
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET 
 * ENJOYMENT OR NON-INFRINGEMENT. 
 *  
 * Technology Compatibility Kit Test Suite(s) Location:  
 *    http://www.helixcommunity.org/content/tck  
 *  
 * Contributor(s):  
 *   
 * ***** END LICENSE BLOCK ***** */  

#include "aviffpln.ver"
#include "hxtypes.h"

#ifdef _WINDOWS
#include "hlxclib/windows.h"
#endif

#ifdef WIN32
#include "mmreg.h"
#endif // WIN32


#include <string.h>
#include <stdio.h>
#include <signal.h>


#include "aviffpln.h"
#include "avistrm.h"
#include "aviindx.h"

#include "hxtlogutil.h"

#ifdef _DEBUG
#undef HX_THIS_FILE
static char HX_THIS_FILE[] = __FILE__;
#endif // _DEBUG

#ifndef BI_RGB
// This is already defined for windows somehow.
#define BI_RGB 0
#endif // BI_RGB

#define AVI_LIST_OBJECT     0x4c495354 /* 'LIST' */
#define AVI_HDRL_OBJECT     0x6864726c /* 'hdrl' */
#define AVI_AVIH_OBJECT     0x61766968 /* 'avih' */
#define AVI_STRL_OBJECT     0x7374726c /* 'strl' */
#define AVI_STRH_CHUNK      0x73747268 /* 'strh' */
#define AVI_STRF_CHUNK      0x73747266 /* 'strf' */
#define AVI_STRD_CHUNK      0x73747264 /* 'strd' */
#define AVI_MOVI_TYPE       0x6d6f7669 /* 'movi' */
#define AVI_RECORD_TYPE     0x72656320 /* 'rec ' */
#define AVI_INFO_CHUNK      0x494e464f /* 'INFO' */
#define AVI_TITLE_CHUNK     0x494e414d /* 'INAM' */
#define AVI_AUTHOR_CHUNK    0x49415254 /* 'IART' */
#define AVI_COPYRIGHT_CHUNK 0x49434f50 /* 'ICOP' */

// TODO: Map these types explicitly:
#define AVI_H261_VIDEO      0x31363248 /* '162H' */
#define AVI_H263_VIDEO      0x3336324D /* '362M' :XXXEH- why 'M', not 'H'? */
#define AVI_MPEG4_VIDEO     0x3447504D /* '4GPM' */

// Flags for header (some unused):
#define AVI_HFLAG_HASINDEX	    0x00000010	// Index at end of file?
#define AVI_HFLAG_MUSTUSEINDEX	    0x00000020
#define AVI_HFLAG_ISINTERLEAVED	    0x00000100
#define AVI_HFLAG_TRUSTCKTYPE	    0x00000800	// Use CKType to find key frames?
#define AVI_HFLAG_WASCAPTUREFILE    0x00010000
#define AVI_HFLAG_COPYRIGHTED	    0x00020000

#define MAKE_TAG(a,b,c,d) ((a << 24) | (b << 16) | (c << 8) | (d))

// Send packets in timeline order no matter what stream is referenced in
// GetPacket()
#define FORCE_IN_ORDER_TRANSMISSION

#ifdef NET_ENDIAN
#define LE32_TO_HOST(x)  ((x << 24)              | \
                          (x << 8  & 0x00FF0000) | \
                          (x >> 8  & 0x0000FF00) | \
                          (x >> 24 & 0xFF))
#define LE16_TO_HOST(x)  ((x << 8) |
                          (x & 0xFF))
#else
#define LE32_TO_HOST(x) (x)
#define LE16_TO_HOST(x) (x)
#endif // NET_ENDIAN

INT32 g_nRefCount_avif;

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
HX_RESULT STDAPICALLTYPE CAVIFileFormat::HXCreateInstance
(
    IUnknown**  /*OUT*/ ppIUnknown
)
{
    *ppIUnknown = (IUnknown*)(IHXPlugin*) new CAVIFileFormat();
    if ( *ppIUnknown )
    {
        (*ppIUnknown)->AddRef();
        return HXR_OK;
    }
    return HXR_OUTOFMEMORY;
}


/****************************************************************************
 *
 *  Function:
 *
 *  CanUnload()
 *
 *  Purpose:
 *
 *  Function implemented by all plugin DLL's if it returns PNR_OK
 *  then the pluginhandler can unload the DLL
 *
 */
HX_RESULT CAVIFileFormat::CanUnload(void)
{
    HX_ASSERT(g_nRefCount_avif >= 0);
    return(g_nRefCount_avif > 0 ? HXR_FAIL : HXR_OK);
}

const char* CAVIFileFormat::zm_pDescription      = "RealNetworks AVI File Format Plugin";
const char* CAVIFileFormat::zm_pCopyright        = HXVER_COPYRIGHT;
const char* CAVIFileFormat::zm_pMoreInfoURL      = "http://www.real.com";

const char* CAVIFileFormat::zm_pFileMimeTypes[]  = {"application/x-pn-avi-plugin", NULL};
const char* CAVIFileFormat::zm_pFileExtensions[] = {"avi", NULL};
const char* CAVIFileFormat::zm_pFileOpenNames[]  = {"AVI Files (*.avi)",
                                                    "DivX Files (*.divx)", NULL};
const char* CAVIFileFormat::zm_pPacketFormats[]  = {"rdt", "rtp", NULL};

CAVIFileFormat::CAVIFileFormat()
    : m_bSeekPriming(FALSE)
    , m_bLocalPlayback(FALSE)
    , m_usStreamTarget(0)
    , m_pIndex(NULL)
    , m_pGeneralReader(NULL)
    , m_ulMOVIOffset(0)
    , m_pFile(NULL)
    , m_pFileSystemManager(NULL)
    , m_pFFResponse(NULL)
    , m_pRequest(NULL)
    , m_pErrorMessages(NULL)
    , m_pContext(NULL)
    , m_pCommonClassFactory(NULL)
    , m_ulRefCount(0)
    , m_pszTitle(NULL)
    , m_pszAuthor(NULL)
    , m_pszCopyright(NULL)
    , m_state(AS_InitPending)
{
    g_nRefCount_avif++; // DLL Ref Counting


	// Header fields set to 0

	m_header.ulMicroSecPerFrame    = 0;
    m_header.ulMaxBytesPerSec      = 0;
    m_header.ulPaddingGranularity  = 0;
    m_header.ulFlags               = 0;
    m_header.ulTotalFrames         = 0;
    m_header.ulInitialFrames       = 0;
    m_header.ulStreams             = 0;
    m_header.ulSuggestedBufferSize = 0;
    m_header.ulWidth               = 0;
    m_header.ulHeight              = 0;
    m_header.ulScale               = 0;
    m_header.ulRate                = 0;
    m_header.ulStart               = 0;
    m_header.ulLength              = 0;


    HX_SET_HEAP_OPTION(HX_HEAP_LEAK_CHECK_ON_EXIT);
};


/************************************************************************
 *  CAVIFileFormat::InitPlugin
 *  Purpose:
 *    Initializes the plugin for use. This interface must always be
 *    called before any other method is called. This is primarily needed
 *    so that the plugin can have access to the context for creation of
 *    IRMABuffers and IMalloc.
 */
STDMETHODIMP CAVIFileFormat::InitPlugin(IUnknown* /*IN*/ pContext)
{
    m_pContext = pContext;
    HX_ASSERT_VALID_PTR(pContext);
    HX_ADDREF(m_pContext);

    if (!m_pContext)
    {
        return HXR_INVALID_PARAMETER;
    }

    HX_ENABLE_LOGGING(m_pContext);

    HX_RESULT result = HXR_OK;

    if ( HXR_OK != pContext->QueryInterface(IID_IHXErrorMessages,
                                            (void**)&m_pErrorMessages) )
    {
        m_pErrorMessages = NULL;
    }

    if (FAILED(result = m_pContext->QueryInterface(IID_IHXCommonClassFactory,
                                                   (void**) &m_pCommonClassFactory)))
    {
        m_pCommonClassFactory = NULL;
        return HXR_NOINTERFACE;
    }

    if (SUCCEEDED(result = m_pCommonClassFactory->CreateInstance(CLSID_IHXFileSystemManager,
                                                                 (void**) &m_pFileSystemManager)))
    {
        result = m_pFileSystemManager->Init(this);
    }

    return result;
}


/************************************************************************
 *  CAVIFileFormat::GetPluginInfo
 *  Purpose:
 *    Returns the basic information about this plugin. Including:
 *
 *    bLoadMultiple whether or not this plugin DLL can be loaded
 *          multiple times. All File Formats must set
 *          this value to TRUE.
 *    pDescription  which is used in about UIs (can be NULL)
 *    pCopyright    which is used in about UIs (can be NULL)
 *    pMoreInfoURL  which is used in about UIs (can be NULL)
 */
STDMETHODIMP CAVIFileFormat::GetPluginInfo
(
    REF(BOOL)        /*OUT*/ bLoadMultiple,
    REF(const char*) /*OUT*/ pDescription,
    REF(const char*) /*OUT*/ pCopyright,
    REF(const char*) /*OUT*/ pMoreInfoURL,
    REF(ULONG32)     /*OUT*/ ulVersionNumber
)
{
    bLoadMultiple = TRUE;   // Must be true for file formats.

    pDescription    = zm_pDescription;
    pCopyright      = zm_pCopyright;
    pMoreInfoURL    = zm_pMoreInfoURL;
    ulVersionNumber = TARVER_ULONG32_VERSION;

    return HXR_OK;
}


CAVIFileFormat::~CAVIFileFormat()
{
    Close();
    g_nRefCount_avif--; // DLL Ref Counting
    return;
}


/************************************************************************
 *  CAVIFileFormat::GetObjFileFormatInfo
 *  Purpose:
 *    If this object is a file format object this method returns
 *    information vital to the instantiation of file format plugins.
 *    If this object is not a file format object, it should return
 *    HXR_UNEXPECTED.
 */
STDMETHODIMP CAVIFileFormat::GetFileFormatInfo
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
//  IUnknown::QueryInterface
//  Purpose:
//  Implement this to export the interfaces supported by your
//  object.
//
STDMETHODIMP CAVIFileFormat::QueryInterface(REFIID riid, void** ppvObj)
{
    if ( IsEqualIID(riid, IID_IUnknown) )
    {
        AddRef();
        *ppvObj = this;
        return HXR_OK;
    }
    else if ( IsEqualIID(riid, IID_IHXPlugin) )
    {
        AddRef();
        *ppvObj = (IHXPlugin*) this;
        return HXR_OK;
    }
    else if ( IsEqualIID(riid, IID_IHXFileFormatObject) )
    {
        AddRef();
        *ppvObj = (IHXFileFormatObject*) this;
        return HXR_OK;
    }
    else if ( IsEqualIID(riid, IID_IHXFileResponse) )
    {
        AddRef();
        *ppvObj = (IHXFileResponse*) this;
        return HXR_OK;
    }
    else if ( IsEqualIID(riid, IID_IHXPendingStatus) )
    {
        AddRef();
        *ppvObj = (IHXPendingStatus*) this;
        return HXR_OK;
    }
    else if ( IsEqualIID(riid, IID_IHXPacketFormat) )
    {
        AddRef();
        *ppvObj = (IHXPacketFormat*) this;
        return HXR_OK;
    }
    else if ( IsEqualIID(riid, IID_IHXInterruptSafe) )
    {
        AddRef();
        *ppvObj = (IHXInterruptSafe*) this;
        return HXR_OK;
    }
    else if ( IsEqualIID(riid, IID_IHXFileSystemManagerResponse) )
    {
        AddRef();
        *ppvObj = (IHXFileSystemManagerResponse*) this;
        return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}


////////////////////////////////////////////////////////////////////////////////
//  IUnknown::AddRef
//
STDMETHODIMP_(ULONG32) CAVIFileFormat::AddRef()
{
    return InterlockedIncrement(&m_ulRefCount);
}


////////////////////////////////////////////////////////////////////////////////
//  IUnknown::Release
//
STDMETHODIMP_(ULONG32) CAVIFileFormat::Release()
{
#if defined(DEBUG)
    ULONG32 ulRefCount = (ULONG32) InterlockedDecrement((LONG32*) &m_ulRefCount);
    HX_ASSERT((ulRefCount != 0xFFFFFFFF) && "Max refcount reached or unbalanced Release()");
    if (ulRefCount != 0)
#else
    if ( InterlockedDecrement(&m_ulRefCount) > 0 )
#endif
    {
        return m_ulRefCount;
    }

    delete this;
    return 0;
}


/*******************************************************************************
 *  InitFileFormat
 */
STDMETHODIMP CAVIFileFormat::InitFileFormat
(
    IHXRequest*        /*IN*/  pRequest,
    IHXFormatResponse* /*IN*/  pFormatResponse,
    IHXFileObject*     /*IN*/  pFile
)
{
    m_pRequest    = pRequest;
    m_pFFResponse = pFormatResponse;
    m_pFile       = pFile;

    HX_ASSERT_VALID_PTR(m_pRequest);
    HX_ADDREF(m_pRequest);
    HX_ASSERT_VALID_PTR(m_pFFResponse);
    HX_ADDREF(m_pFFResponse);
    HX_ASSERT_VALID_PTR(m_pFile);
    HX_ADDREF(m_pFile);

    if ( !(m_pRequest && m_pFFResponse && m_pFile)
         || m_pRequest->GetURL(m_pURL) != HXR_OK )
    {
        return HXR_FAILED;
    }

    m_pGeneralReader = new CRIFFReader(m_pContext, this, m_pFile);
    HX_ADDREF(m_pGeneralReader);
    if (!m_pGeneralReader)
    {
        return HXR_OUTOFMEMORY;
    }

    IUnknown* pUnk;
    if (SUCCEEDED(m_pContext->QueryInterface(IID_IHXClientEngine, (void**) &pUnk)))
    {
        m_bLocalPlayback = TRUE;
        HX_RELEASE(pUnk);
    }

    HX_ASSERT(m_state == AS_InitPending);
    m_state = AS_OpenPending;
    return m_pGeneralReader->Open((char*)m_pURL);
}


STDMETHODIMP CAVIFileFormat::Close()
{
    m_state = AS_Closed;

    HX_VECTOR_DELETE(m_pszTitle);
    HX_VECTOR_DELETE(m_pszAuthor);
    HX_VECTOR_DELETE(m_pszCopyright);

    HX_RELEASE(m_pRequest);
    HX_RELEASE(m_pErrorMessages);
    HX_RELEASE(m_pFFResponse);

    HX_RELEASE(m_pFileSystemManager);

	if(m_pIndex)
	{
		m_pIndex->Close();
		HX_RELEASE(m_pIndex);
	}


    if(!m_streamArray.IsEmpty())
	{
		for (UINT32 i = 0; i < m_header.ulStreams; ++i)
		{
			CAVIStream* pStream = (CAVIStream*) m_streamArray[i];
			if(pStream)
			{
				pStream->Close();
				HX_RELEASE(pStream);
			}
		}
		m_streamArray.RemoveAll();
	}


    if (m_pFile)
    {
        m_pFile->Close();
        HX_RELEASE(m_pFile);
    }

	HX_RELEASE(m_pGeneralReader);
    HX_RELEASE(m_pFile);

    HX_RELEASE(m_pCommonClassFactory);
    HX_RELEASE(m_pContext);

    return HXR_OK;
}


/*******************************************************************************
 *  GetFileHeader
 */
STDMETHODIMP CAVIFileFormat::GetFileHeader()
{
    if ( m_state != AS_OpenPending )
    {
        HX_ASSERT(!"Unexpected call order: m_state != AS_OpenPending");
        return HXR_UNEXPECTED;
    }

    m_state = AS_FileDescend;
    m_pGeneralReader->Descend();

    return HXR_OK;
}


/*******************************************************************************
 *  GetStreamHeader
 *  Purpose:
 *	Called by controller to ask the file format for the header for
 *	a particular stream in the file. The file format should call
 *	IHXFileFormatResponse::HeaderReady() for the IHXFileFormatResponse
 *	object that was passed in during initialization, when the header
 *	is available.
 */
STDMETHODIMP CAVIFileFormat::GetStreamHeader(UINT16 usStream)
{

    HX_ASSERT(usStream <= m_header.ulStreams);

    // Note request:
    ((CAVIStream*) m_streamArray[usStream])->SetPendingHeaderRequest();

    if (m_state == AS_INFOAscend)
    {
        // Stream headers have been read while parsing hdrl, but we still
        // need to initialize the index here to obtain max and average
        // bitrates--otherwise we'd delay until the first GetPacket()
        m_state = AS_GetIndexFilePending;
        if (!m_pFileSystemManager)
        {
            m_pFFResponse->StreamHeaderReady(HXR_FAILED, NULL);
        }
        else
        {
            const char* szFilename;
            m_pFile->GetFilename(szFilename);
            m_pFileSystemManager->GetRelativeFileObject(m_pFile, szFilename);
        }

        return HXR_OK;
    }

    if (m_state < AS_GetStreamFilePending)
    {
        return HXR_OK;
    }

    IHXValues* pHeader;
    if (SUCCEEDED(m_pCommonClassFactory->CreateInstance(CLSID_IHXValues,
                                                        (void**) &pHeader)))
    {
        CAVIStream* pStream = (CAVIStream*) m_streamArray[usStream];
        HX_ASSERT_VALID_PTR(pStream);
        pStream->SetIndex(m_pIndex);

        if (SUCCEEDED(pStream->GetHeader(pHeader)))
        {
            m_pFFResponse->StreamHeaderReady(HXR_OK, pHeader);
        }
        else
        {
            HX_ASSERT(!"Bad stream header!");
            m_pFFResponse->StreamHeaderReady(HXR_FAIL, NULL);
        }

        HX_RELEASE(pHeader);
    }
    else
    {
        m_pFFResponse->StreamHeaderReady(HXR_OUTOFMEMORY, NULL);
    }

    return HXR_OK;
}


/*******************************************************************************
 *  GetPacket
 *  Method:
 *	IHXFileFormatObject::GetPacket
 *  Purpose:
 *	Called by controller to ask the file format for the next packet
 *	for a particular stream in the file. The file format should call
 *	IHXFileFormatResponse::PacketReady() for the IHXFileFormatResponse
 *	object that was passed in during initialization, when the packet
 *	is available.
 */

STDMETHODIMP CAVIFileFormat::GetPacket(UINT16 unStreamNumber)
{
    HX_TRACE("CAVIFileFormat::GetPacket() on stream %i\n", unStreamNumber);
    HX_ASSERT(m_state >= AS_GetIndexFilePending);
    //HX_ASSERT(!m_bSeekPriming);
    if (m_state <= AS_IndexFileInit)
    {
        HX_ASSERT(!"Unexpected call order");
        return HXR_UNEXPECTED;
    }

    CAVIStream* pStream = (CAVIStream*) m_streamArray[unStreamNumber];
    HX_ASSERT_VALID_PTR(pStream);
    if (!pStream)
    {
        return HXR_UNEXPECTED;
    }

    // Any pending packets should have been dispatched from the reader response:
    HX_ASSERT(pStream->GetPendingPacketCount() ? !pStream->HasPackets() : TRUE);

    pStream->IncrementPendingPacketCount();

    // GetPacket() behavior:
    // - Check for stream completion
    // - Choose the stream with the earliest unsent data; all things being
    //   equal, audio streams have top priority.  The core should already do
    //   this for us; this is an extra check.  // To do: add check
    // - Send a stream packet if available (pending packets will be sent in
    //   the reader response)
    // - Kickstart a stream prefetch scan if one is not already in progress--this
    //   plugin double buffers file reads for maximum performance in local
    //   playback.

    if (!pStream->HasPackets() && pStream->AtEndOfStream())
    {
        m_pFFResponse->StreamDone(unStreamNumber);
    }

    ScanState();

    return HXR_OK;
}


/************************************************************************
 *  Seek
 *  Purpose:
 *	Called by controller to tell the file format to seek to the
 *	nearest packet to the requested offset. The file format should
 *	call IHXFileFormatResponse::SeekDone() for the IHXFileFormat-
 *	Session object that was passed in during initialization, when
 *	the seek has completed.
 */
STDMETHODIMP CAVIFileFormat::Seek(ULONG32 ulOffset)
{
    HX_TRACE("CAVIFileFormat::Seek()\t%lu\n", ulOffset);

    // We should handle seeks past end of stream by streaming all data
    // past and including the last keyframe

    // Reset packet pending counts, flush old data;
    // Seek readers to correct time if necessary
    for (UINT16 i = 0; i < m_streamArray.GetSize(); ++i)
    {
        CAVIStream* pStream = (CAVIStream*) m_streamArray[i];
        pStream->Seek(ulOffset);
    }

    if (m_bLocalPlayback)
    {
        m_bSeekPriming = TRUE;
    }
    else
    {
        m_pFFResponse->SeekDone(HXR_OK);
    }

    ScanState();

    return HXR_OK;
}

/*******************************************************************************
 * CAVIFileFormat::GetStatus
 */
STDMETHODIMP
CAVIFileFormat::GetStatus
(
    REF(UINT16) uStatusCode,
    REF(IHXBuffer*) pStatusDesc,
    REF(UINT16) ulPercentDone
)
{
    //HX_TRACE("CAVIFileFormat::GetStatus()\n");
    HX_RESULT hResult = HXR_OK;

#if 0
    IHXPendingStatus* pFileSystemStatus = NULL;

    // asking status information from the file system object
    if ( m_pFile )
    {
        if ( HXR_OK == m_pFile->QueryInterface(IID_IHXPendingStatus,(void**)&pFileSystemStatus) )
        {
            hResult = pFileSystemStatus->GetStatus(uStatusCode, pStatusDesc, ulPercentDone);

            pFileSystemStatus->Release();
            return hResult;
        }
    }

#endif

    if (m_state == AS_Ready)
    {
        // by default
        uStatusCode = HX_STATUS_READY;
        pStatusDesc = NULL;
        ulPercentDone = 0;

    }
    else
    {
        // by default
        uStatusCode = HX_STATUS_BUFFERING;
        pStatusDesc = NULL;
        ulPercentDone = 50;
    }
    return hResult;
}


/************************************************************************
 *      Method:
 *          IHXPacketFormat::GetSupportedPacketFormats
 *      Purpose:
 *          Obtains a list of packet formats supported by this file format
 */
STDMETHODIMP
CAVIFileFormat::GetSupportedPacketFormats
(
    REF(const char**) /*OUT*/ pPacketFormats
)
{
    //HX_TRACE("CAVIFileFormat::GetSupportedPacketFormats()\n");
    pPacketFormats = zm_pPacketFormats;
    return HXR_OK;
}


/************************************************************************
 *          IRMAPacketFormat::SetPacketFormat
 *      Purpose:
 *          Sets the packet type for this file format
 */
STDMETHODIMP
CAVIFileFormat::SetPacketFormat
(
    const char* pPacketFormat
)
{
    #if 0
    //HX_TRACE("CAVIFileFormat::SetPacketFormat()\n");
    if ( strcasecmp(pPacketFormat, "rtp") == 0 )
    {
        m_packetFormat = PFMT_RTP;
    }
    else
    {
        m_packetFormat = PFMT_RDT;
    }
    #endif 0
    return HXR_OK;
}


STDMETHODIMP
CAVIFileFormat::RIFFOpenDone(HX_RESULT status)
{
    //PN_TRACE("CAVIFileFormat::RIFFOpenDone(%lx)\n", status);
    HX_ASSERT(SUCCEEDED(status));

    if ( m_state != AS_OpenPending )
    {
        HX_ASSERT(FALSE);
        return HXR_UNEXPECTED;
    }

    if ( status != HXR_OK )
    {
        m_state = AS_InitPending;
        m_pFFResponse->InitDone(status);
        return HXR_OK;
    }

    // Verify this is indeed an AVI file:
    if (m_pGeneralReader->FileSubtype() != HX_MAKE4CC('A', 'V', 'I', ' '))
    {
        m_pErrorMessages->Report(HXLOG_ERR, HXR_INVALID_FILE,
                                 0, (const char*) "The stream URL is not a valid AVI",
                                 NULL);
        m_pFFResponse->InitDone(HXR_BAD_FORMAT);
    }

    // We descend on GetFileHeader
    m_pFFResponse->InitDone(HXR_OK);
    return HXR_OK;
}

STDMETHODIMP
CAVIFileFormat::RIFFCloseDone(HX_RESULT status)
{
    //HX_TRACE("CAVIFileFormat::RIFFCloseDone(%lx)\n", status);
    HX_ASSERT(SUCCEEDED(status));
    HX_ASSERT(m_state >= AS_GetIndexFilePending);

    return HXR_OK;
}

STDMETHODIMP
CAVIFileFormat::RIFFFindChunkDone(HX_RESULT status, UINT32 len)
{
    //HX_TRACE("CAVIFileFormat::RIFFFindChunkDone(""%lx, %lu)\n\tstate=%lu\n",
    //         status, len, m_state);

    switch (m_state)
    {
        case AS_HDRLFind:
            if (FAILED(status))
            {
                m_state = AS_InitPending;
                m_pFFResponse->FileHeaderReady(status, NULL);
            }
            else
            {
                if (m_pGeneralReader->GetChunkType() == AVI_LIST_OBJECT &&
                    m_pGeneralReader->GetListType() == AVI_HDRL_OBJECT )
                {
                    m_state = AS_HDRLDescend;
                    m_pGeneralReader->Descend();
                }
                else
                {
                    m_state = AS_HDRLFind;
                    m_pGeneralReader->FindChunk(AVI_LIST_OBJECT, TRUE);
                }
            }

            break;

        case AS_AVIHFind:
            if (FAILED(status) ||
                len < sizeof(m_header))
            {
                m_state = AS_InitPending;
                m_pFFResponse->FileHeaderReady(status, NULL);
            }
            else
            {
                m_state = AS_AVIHRead;
                m_pGeneralReader->Read(len);
            }

            break;

        case AS_STRLFind:
            if (FAILED(status))
            {
                m_state = AS_HDRLAscend;
                m_pGeneralReader->Ascend();
            }
            else
            {
                if (m_pGeneralReader->GetChunkType() == AVI_LIST_OBJECT &&
                    m_pGeneralReader->GetListType() == AVI_STRL_OBJECT)
                {
                    m_state = AS_STRLDescend;
                    m_pGeneralReader->Descend();
                }
                else
                {
                    m_state = AS_STRLFind;
                    m_pGeneralReader->FindChunk(AVI_LIST_OBJECT, TRUE);
                }
            }

            break;

        case AS_STRLScan:
            if (FAILED(status))
            {
                m_state = AS_STRLAscend;
                m_pGeneralReader->Ascend();
            }
            else
            {
                switch (m_pGeneralReader->GetChunkType())
                {
                    case AVI_STRF_CHUNK:
                    case AVI_STRH_CHUNK:
                    case AVI_STRD_CHUNK:
                        m_state = AS_STRLRead;
                        m_pGeneralReader->Read(len);
                        break;
                    default:
                        if (m_pGeneralReader->GetChunkType() == AVI_LIST_OBJECT
                            && m_pGeneralReader->GetListType() == AVI_STRL_OBJECT)
                        {
                            m_state = AS_STRLDescend;
                            m_pGeneralReader->Descend();
                        }
                        else
                        {
//							m_state = AS_STRLScanDone;
//                          m_pGeneralReader->FindNextChunk();

							m_state = AS_STRLAscend;
							m_pGeneralReader->Ascend();
                        }
                }
            }
            break;

        case AS_INFOFind:
            if (FAILED(status))
            {
                m_state = AS_INFOAscend;
                m_pGeneralReader->Ascend();
            }
            else
            {
                switch (m_pGeneralReader->GetListType())
                {
                    case AVI_MOVI_TYPE:
                        // We note the MOVI offset:
                        m_state = AS_INFOAscend;
                        m_ulMOVIOffset = m_pGeneralReader->GetOffset() - 4;
                        //HX_TRACE("movi offset:%lx\n", m_ulMOVIOffset);
                        m_pGeneralReader->Ascend();
                        break;
                    case AVI_INFO_CHUNK:
                        m_state = AS_INFODescend;
                        m_pGeneralReader->Descend();
                        break;
                    default:
                        m_pGeneralReader->FindChunk(AVI_LIST_OBJECT, TRUE);
                }
            }
            break;

        case AS_INFOScan:
            if (FAILED(status))
            {
                m_state = AS_INFOAscend;
                m_pGeneralReader->Ascend();
            }

            switch (m_pGeneralReader->GetChunkType())
            {
                case AVI_INFO_CHUNK:
                case AVI_TITLE_CHUNK:
                case AVI_AUTHOR_CHUNK:
                    m_state = AS_INFORead;
                    m_pGeneralReader->Read(len);
                    break;

                case AVI_LIST_OBJECT:
                    m_state = AS_INFOFind;
                    m_pGeneralReader->FindChunk(AVI_LIST_OBJECT, TRUE);
                    break;

                default:
                    m_state = AS_INFORead;
                    m_pGeneralReader->Read(len);
            }
            break;
        default:
            HX_ASSERT(!"Unexpected AVI state.");
            return HXR_UNEXPECTED;
    }

    return HXR_OK;
}

STDMETHODIMP
CAVIFileFormat::RIFFDescendDone(HX_RESULT status)
{
    //HX_TRACE("CAVIFileFormat::RIFFDescendDone(%lx)\n\tstate=%lu\n", status, m_state);
    HX_ASSERT(SUCCEEDED(status));

    switch ( m_state )
    {
        case AS_FileDescend:
            if ( status != HXR_OK )
            {
                m_state = AS_InitPending;
                m_pFFResponse->FileHeaderReady(status, NULL);
            }
            else
            {
                m_state = AS_HDRLFind;
                m_pGeneralReader->FindChunk(AVI_LIST_OBJECT, TRUE);
            }
            break;
        case AS_HDRLDescend:
            if ( status != HXR_OK )
            {
                m_state = AS_InitPending;
                m_pFFResponse->FileHeaderReady(status, NULL);
            }
            else
            {
                m_state = AS_AVIHFind;
                m_pGeneralReader->FindChunk(AVI_AVIH_OBJECT, TRUE);
            }
            break;
        case AS_STRLDescend:
            if ( status != HXR_OK )
            {
                m_state = AS_InitPending;
                m_pFFResponse->FileHeaderReady(status, NULL);
            }
            else
            {
                m_state = AS_STRLScan;
                m_pGeneralReader->FindNextChunk();
            }
            break;
        case AS_INFODescend:
            if ( status != HXR_OK)
            {
                m_state = AS_InitPending;
                m_pFFResponse->FileHeaderReady(status, NULL);
            }
            else
            {
                m_state = AS_INFOScan;
                m_pGeneralReader->FindNextChunk();
            }

            break;
        default:
            HX_ASSERT(!"Unexpected AVI state.");
            return HXR_UNEXPECTED;
    }

    return HXR_OK;
}

STDMETHODIMP
CAVIFileFormat::RIFFAscendDone(HX_RESULT status)
{
    //HX_TRACE("CAVIFileFormat::RIFFAscendDone(%lx)\n\tstate=%lu\n", status, m_state);
    HX_ASSERT(SUCCEEDED(status));

    switch ( m_state )
    {
        case AS_STRLAscend:
			if(m_usStreamTarget <  m_header.ulStreams)
			{
				// There are more streams
				// Find Stream header for
				// next stream
				m_state = AS_STRLFind;
				m_pGeneralReader->FindChunk(AVI_LIST_OBJECT, TRUE);
			}
			else
			{
				m_state = AS_HDRLAscend;
				m_pGeneralReader->Ascend();
			}
			break;

        case AS_HDRLAscend:
            m_state = AS_INFOFind;
            m_pGeneralReader->FindChunk(AVI_LIST_OBJECT, FALSE);
            break;

        case AS_INFOAscend:
            {
                IHXValues* pFileHeader = GetHeader();
                if (pFileHeader)
                {
                    m_pFFResponse->FileHeaderReady(HXR_OK, pFileHeader);
                    HX_RELEASE(pFileHeader);
                }
                else
                {
                    // TODO: improve error reporting
                    m_pFFResponse->FileHeaderReady(HXR_FAIL, NULL);
                }
            }
            break;

        default:
            m_state = AS_Ready;
            return HXR_UNEXPECTED;
    }

    return HXR_OK;
}

STDMETHODIMP
CAVIFileFormat::RIFFReadDone(HX_RESULT status, IHXBuffer* pBuffer)
{
    //HX_TRACE("CAVIFileFormat::RIFFReadDone(%lx)\n\tstate=%lu\n", status, m_state);
    HX_ASSERT(SUCCEEDED(status));

    if (m_state < AS_AVIHRead ||
        m_state > AS_INFORead)
    {
        HX_ASSERT(FALSE);
        return HXR_UNEXPECTED;
    }

    switch (m_state)
    {
        case AS_AVIHRead:
        {
            if (FAILED(SetHeader(pBuffer)))
            {
                m_pFFResponse->FileHeaderReady(HXR_BAD_FORMAT, NULL);
            }

            // Initialize stream array:
            HX_ASSERT(m_header.ulStreams > 0);
            m_streamArray.SetSize(m_header.ulStreams);

            HX_ASSERT(m_usStreamTarget == 0);
            m_usStreamTarget = 0;

            HX_ASSERT( m_header.ulStreams < MAX_INT16 );
            for (UINT32 i = 0; i < m_header.ulStreams; ++i)
            {
                m_streamArray[i] = new CAVIStream(this, (UINT16) i, m_header, m_bLocalPlayback,
                                                  m_pContext);
                if (!m_streamArray[i])
                {
                    m_pFFResponse->FileHeaderReady(HXR_OUTOFMEMORY, NULL);
                }
				else
				{
					CAVIStream* pStream = (CAVIStream*) m_streamArray[i];
					pStream->AddRef();
				}
            }

            m_state = AS_STRLFind;
            m_pGeneralReader->FindChunk(AVI_LIST_OBJECT, TRUE);
            break;
        }

        case AS_STRLRead:
            switch (m_pGeneralReader->GetChunkType())
            {
                case AVI_STRH_CHUNK:
                {
                    ++m_usStreamTarget;
                    CAVIStream* pStream = (CAVIStream*) m_streamArray[m_usStreamTarget -1];
                    HX_ASSERT(pStream);
                    pStream->SetHeader(m_usStreamTarget, pBuffer);
                }
                break;
                case AVI_STRF_CHUNK:
                {
                    CAVIStream* pStream = (CAVIStream*) m_streamArray[m_usStreamTarget -1];
                    HX_ASSERT(pStream);
                    pStream->SetFormat(pBuffer);
                }
                break;
                case AVI_STRD_CHUNK:
                {
                    CAVIStream* pStream = (CAVIStream*) m_streamArray[m_usStreamTarget -1];
                    HX_ASSERT(pStream);
                    pStream->SetOpaque(pBuffer);
                }
                break;
            }

            m_state = AS_STRLScan;
            m_pGeneralReader->FindNextChunk();
            break;

        case AS_INFORead:
            SetInfo(pBuffer, m_pGeneralReader->GetChunkType());

            m_state = AS_INFOScan;
            m_pGeneralReader->FindNextChunk();
            break;

        default:
            HX_ASSERT(FALSE);
            return HXR_UNEXPECTED;
    }

    return HXR_OK;
}

void CAVIFileFormat::SetInfo(IHXBuffer* pBuffer, UINT32 ulChunkType)
{
    //HX_TRACE("CAVIFileFormat::SetInfo()\n", m_state);
    if (!pBuffer)
    {
        HX_ASSERT(FALSE);
        return;
    }

    UINT32 len = pBuffer->GetSize();
    UCHAR* buf = pBuffer->GetBuffer();

    HX_ASSERT(len && buf);

    switch (ulChunkType)
    {
        case AVI_TITLE_CHUNK:
            m_pszTitle = new CHAR[len+1];
            memset(m_pszTitle, len+1, 0);
            strcpy(m_pszTitle, (const char*)buf);
            break;

        case AVI_AUTHOR_CHUNK:
            m_pszAuthor = new CHAR[len+1];
            memset(m_pszAuthor, len+1, 0);
            strcpy(m_pszAuthor, (const char*)buf);
            break;

        case AVI_COPYRIGHT_CHUNK:
            m_pszCopyright = new CHAR[len+1];
            memset(m_pszCopyright, len+1, 0);
            strcpy(m_pszCopyright, (const char*)buf);
            break;

    }
}


HX_RESULT CAVIFileFormat::SetHeader(IHXBuffer* pBuffer)
{
    //HX_TRACE("CAVIFileFormat::SetHeader\n\tstate=%lu\n", m_state);
    HX_ASSERT_VALID_PTR(pBuffer);

    UINT32 len = pBuffer->GetSize();
    UCHAR* buf = pBuffer->GetBuffer();

    if (len < sizeof(m_header))
    {
        HX_ASSERT(FALSE);
        return HXR_FAIL;
    }

    m_header.ulMicroSecPerFrame    = LE32_TO_HOST( *(UINT32*) &buf[0]);
    m_header.ulMaxBytesPerSec      = LE32_TO_HOST( *(UINT32*) &buf[4]);
    m_header.ulPaddingGranularity  = LE32_TO_HOST( *(UINT32*) &buf[8]);
    m_header.ulFlags               = LE32_TO_HOST( *(UINT32*) &buf[12]);
    m_header.ulTotalFrames         = LE32_TO_HOST( *(UINT32*) &buf[16]);
    m_header.ulInitialFrames       = LE32_TO_HOST( *(UINT32*) &buf[20]);
    m_header.ulStreams             = LE32_TO_HOST( *(UINT32*) &buf[24]);
    m_header.ulSuggestedBufferSize = LE32_TO_HOST( *(UINT32*) &buf[28]);

    m_header.ulWidth               = LE32_TO_HOST( *(UINT32*) &buf[32]);
    m_header.ulHeight              = LE32_TO_HOST( *(UINT32*) &buf[36]);

    m_header.ulScale               = LE32_TO_HOST( *(UINT32*) &buf[40]);
    m_header.ulRate                = LE32_TO_HOST( *(UINT32*) &buf[44]);
    m_header.ulStart               = LE32_TO_HOST( *(UINT32*) &buf[48]);
    m_header.ulLength              = LE32_TO_HOST( *(UINT32*) &buf[52]);

    return HXR_OK;
}

STDMETHODIMP
CAVIFileFormat::RIFFSeekDone(HX_RESULT status)
{
    //HX_TRACE("CAVIFileFormat::RIFFSeekDone(%lx)\n\tstate=%lu\n", status, m_state);
    HX_ASSERT(FALSE);

    return HXR_UNEXPECTED;
}

STDMETHODIMP
CAVIFileFormat::RIFFGetChunkDone(HX_RESULT status,
                                 UINT32 ulChunkType,
                                 IHXBuffer* pBuffer)
{
//    HX_TRACE("CAVIFileFormat::RIFFGetChunkDone(%lx, %lx)\n\tstate=%lu\n",
//       status, ulChunkType, m_state);
//    HX_ASSERT(FALSE);
//    return HXR_OK;

	HX_ASSERT(SUCCEEDED(status));
//	HX_ASSERT_VALID_PTR(pBuffer);

    switch (m_state)
    {
	case AS_STRLScanDone:
		{
			m_state = AS_STRLAscend;
			m_pGeneralReader->Ascend();
			break;
		}

	default:
		HX_ASSERT(FALSE);
		break;
	}
	return HXR_OK;
}


/////////////////////////////////////////////////////////////////////////
//  CAVIFileFormat::IRMAFileSystemManagerResponse::FileObjectReady
//
STDMETHODIMP
CAVIFileFormat::FileObjectReady(HX_RESULT status,
                                IUnknown* pObject)
{
    //HX_TRACE("CAVIFileFormat::FileObjectReady\n\tstate=%lu\n", m_state);
    HX_ASSERT(SUCCEEDED(status));
    HX_ASSERT_VALID_PTR(pObject);
    HX_ASSERT(m_state == AS_GetIndexFilePending || m_state == AS_GetStreamFilePending);

    HX_RESULT pnr = status;

    switch ( m_state )
    {
        case AS_GetStreamFilePending:

            // we are going to give the file object to the sub stream,
            // it is responsible for closing the file when we tell it to,
            // we don't keep a reference to it.
            IHXFileObject* pStreamFile;
            if (SUCCEEDED(status) && (HXR_OK == (pnr = pObject->QueryInterface(IID_IHXFileObject,
                                                 (void**)&pStreamFile))))
            {
                CAVIStream* pStream = (CAVIStream*) m_streamArray[m_usStreamTarget];
                HX_ASSERT(pStream);
                HX_ASSERT( !((CAVIStream*) m_streamArray[m_usStreamTarget])->ReadInitialized());

                /* CRIFFReader* pReader = new CRIFFReader(m_pContext,
                                                       pStream,
                                                       pStreamFile);
				*/
                m_state = AS_IOEvent;
                ((CAVIStream*) m_streamArray[m_usStreamTarget])->InitForReading(m_pContext, pStreamFile);
                HX_RELEASE(pStreamFile);
                // Wait for IOEvent
            }
            else
            {
                m_pFFResponse->PacketReady(pnr, NULL);
            }

            break;
        case AS_GetIndexFilePending:

            m_state = AS_IndexFileInit;
            HX_ASSERT(m_ulMOVIOffset);
            IHXFileObject* pIndexFile;
            if (SUCCEEDED(status) && (HXR_OK == (pnr =
                           pObject->QueryInterface(IID_IHXFileObject,
                                                   (void**) &pIndexFile))))
            {
                m_pIndex = new CAVIIndex();
				// after new, addref m_pIndex
				m_pIndex->AddRef();
                m_state = AS_IndexFileInit;
                m_pIndex->Init(this, pIndexFile, m_pContext,
                               m_ulMOVIOffset,
                               (UINT16) m_header.ulStreams);
                // IOEvent sent on completion
                HX_RELEASE(pIndexFile);
            }
            else
            {
                // This is odd, but the index can still work sans file:
                m_pIndex = new CAVIIndex();
				// after new, addref m_pIndex
				m_pIndex->AddRef();
                m_state = AS_IndexFileInit;
                m_pIndex->Init(this, NULL, m_pContext,
                               m_ulMOVIOffset,
                               (UINT16) m_header.ulStreams);
                // IOEvent sent on completion
            }

			// after new, addref m_pIndex
			// m_pIndex->AddRef();


            break;
        default:
             m_state = AS_Ready;
             pnr = HXR_UNEXPECTED;
    }

    return pnr;
}

/////////////////////////////////////////////////////////////////////////
//  CAVIFileFormat::IRMAFileSystemManagerResponse::CloseDone
//
STDMETHODIMP
CAVIFileFormat::CloseDone(HX_RESULT)
{
    return HXR_NOTIMPL;
}

/////////////////////////////////////////////////////////////////////////
//  CAVIFileFormat::IRMAFileSystemManagerResponse::ReadDone
//
STDMETHODIMP
CAVIFileFormat::ReadDone(HX_RESULT status,
                         IHXBuffer* pBuffer)
{
    return HXR_NOTIMPL;
}

/////////////////////////////////////////////////////////////////////////
//  CAVIFileFormat::IRMAFileSystemManagerResponse::SeekDone
//
STDMETHODIMP
CAVIFileFormat::SeekDone(HX_RESULT)
{
    return HXR_NOTIMPL;
}

/////////////////////////////////////////////////////////////////////////
//  CAVIFileFormat::IRMAFileSystemManagerResponse::WriteDone
//
STDMETHODIMP
CAVIFileFormat::WriteDone(HX_RESULT status)
{
    return HXR_NOTIMPL;
}

/////////////////////////////////////////////////////////////////////////
//  CAVIFileFormat::IRMAFileSystemManagerResponse::InitDone
//
STDMETHODIMP
CAVIFileFormat::InitDone(HX_RESULT status)
{
    if (FAILED(status))
    {
        return m_pFFResponse->InitDone(status);
    }

    return HXR_OK;
}

/////////////////////////////////////////////////////////////////////////
//  CAVIFileFormat::IRMAFileSystemManagerResponse::DirObjectReady
//
STDMETHODIMP
CAVIFileFormat::DirObjectReady(HX_RESULT status,
                               IUnknown* pDirObject)
{
    return HXR_NOTIMPL;
}

void CAVIFileFormat::ScanState()
{
    //HX_TRACE("CAVIFileFormat::ScanState\n\tstate=%lu\n", m_state);

    if (m_state != AS_Ready)
    {
        return;
    }

    INT32 lEarliestStream;

    do
    {
        lEarliestStream = -1;
        UINT32 ulEarliestPacketTime = MAX_UINT32;

        for (UINT16 i = 0; i < m_streamArray.GetSize(); ++i)
        {
            CAVIStream* pStream = (CAVIStream*) m_streamArray[i];
            HX_ASSERT(pStream);

            if (!pStream->ReadInitialized())
            {
                // We defer stream initialization until GetPacket() to avoid delays when the
                // core only wants headers - for example, when importing clips into the
                // media library. or for SMIL prefetch
                m_usStreamTarget = i;
                m_state = AS_GetStreamFilePending;
                const char* szFilename;
                m_pFile->GetFilename(szFilename);
                m_pFileSystemManager->GetRelativeFileObject(m_pFile, szFilename);
                return;
            }

            if (!pStream->HasPackets() && pStream->AtEndOfStream())
            {
                pStream->ClearPendingPacketCount();
                m_pFFResponse->StreamDone(i);
                continue;
            }

            if (pStream->HasPackets() &&
                pStream->PeekPacketTime() <= ulEarliestPacketTime)
            {
                // Audio streams get priority:
                if (!pStream->IsAudio() &&
                    pStream->PeekPacketTime() == ulEarliestPacketTime)
                {
                    continue;
                }

                ulEarliestPacketTime = pStream->PeekPacketTime();
                lEarliestStream = i;
            }
        }

        if (lEarliestStream >= 0)
        {
            CAVIStream* pStream = (CAVIStream*) m_streamArray[lEarliestStream];

            if (pStream->GetPendingPacketCount() > 0)
			{
                IHXPacket* pPendingPacket = ((CAVIStream*) m_streamArray[lEarliestStream])->GetNextPacket();
                HX_ASSERT(pPendingPacket);

                if (m_bSeekPriming)
                {
                    m_bSeekPriming = FALSE;
                    HX_ASSERT(!pPendingPacket->GetASMRuleNumber());
                }

                HX_TRACE("CAVIFileFormat::ScanState\tPacketReady, stream: %lu\ttimestamp: %lu\n", pPendingPacket->GetStreamNumber(), pPendingPacket->GetTime());
                // Warning: core call
                m_pFFResponse->PacketReady(HXR_OK, pPendingPacket);
                HX_RELEASE(pPendingPacket);

                // Reentrancy ch eck; if we're closed, return:
                if (m_state == AS_Closed)
                {
                    return;
                }
			}
			else
			{
				break;
			}
        }
    } while (lEarliestStream >= 0);

    // assert pending packet counts are zero

    // Fill index:
    if (m_pIndex->CanLoadSlice())
    {
        m_state = AS_IOEvent;
        m_pIndex->GetNextSlice();
        return;
    }

    // Load chunks until all streams are double buffered
    // Many slight optimations could be made in this method.
    do
    {
        UINT32 ulEarliestChunkBeginTime = MAX_UINT32;
        lEarliestStream = -1;

        for (UINT16 i = 0; i < m_streamArray.GetSize(); ++i)
        {
            CAVIStream* pStream = (CAVIStream*) m_streamArray[i];
            HX_ASSERT(pStream);

            if (pStream->ReadInitialized() &&
                pStream->PeekPacketTime() <= ulEarliestChunkBeginTime)
            {
                // Audio streams get priority:
                if (pStream->AtEndOfStream() || pStream->HasPackets() ||
                    (!pStream->IsAudio() &&
                    pStream->PeekPacketTime() == ulEarliestChunkBeginTime))
                {
                    continue;
                }

                ulEarliestChunkBeginTime = pStream->PeekPacketTime();
                lEarliestStream = i;
            }
        }

        if (lEarliestStream >= 0)
        {
			CAVIStream* pStream = (CAVIStream*) m_streamArray[lEarliestStream];
			if(!pStream->AtEndOfStream())
			{
				m_state = AS_IOEvent;
				((CAVIStream*) m_streamArray[lEarliestStream])->GetNextSlice();
			}
			// We serialize all file operations for good performance on low end drives;
			// We return after every file operation and wait to be chained again.
			return;
        }
    }
    while (lEarliestStream >= 0);

    // Fill index:
    if (m_pIndex->CanLoadSlice())
    {
        m_state = AS_IOEvent;
        m_pIndex->GetNextSlice();
        return;
    }

    do
    {
        UINT32 ulEarliestChunkBeginTime = MAX_UINT32;
        lEarliestStream = -1;

        for (UINT16 i = 0; i < m_streamArray.GetSize(); ++i)
        {
            CAVIStream* pStream = (CAVIStream*) m_streamArray[i];
            HX_ASSERT(pStream);

            if (pStream->ReadInitialized() &&
                pStream->PeekPrefetchTime() <= ulEarliestChunkBeginTime)
            {
                // Audio streams get priority:
                // If a stream doesn't want any more data, skip it:
                if (pStream->AtEndOfStream() || !pStream->CanPrefetchSlice() ||
                    (!pStream->IsAudio() &&
                    pStream->PeekPrefetchTime() == ulEarliestChunkBeginTime))
                {
                    continue;
                }

                ulEarliestChunkBeginTime = pStream->PeekPrefetchTime();
                lEarliestStream = i;
            }
        }

        if (lEarliestStream >= 0)
        {
			CAVIStream* pStream = (CAVIStream*) m_streamArray[lEarliestStream];
			if(!pStream->AtEndOfStream())
			{
			    m_state = AS_IOEvent;
				((CAVIStream*) m_streamArray[lEarliestStream])->GetNextSlice();
			}
			// We serialize all file operations for good performance on low end drives;
			// We return after every file operation and wait to be chained again.
			return;
        }
    }
    while (lEarliestStream >= 0);

    // Wait until all streams are primed before InitDone() or SeekDone():
    if (m_bSeekPriming)
    {
        m_pFFResponse->SeekDone(HXR_OK);
        return;
    }

    // Prefetch index:
    if (m_pIndex->CanPreloadSlice())
    {
        m_state = AS_IOEvent;
        m_pIndex->GetNextSlice();
    }
}

IHXValues* CAVIFileFormat::GetHeader()
{
    //HX_TRACE("CAVIFileFormat::GetHeader()\n", m_state);
    HX_ASSERT(m_streamArray.GetSize() > 0);
    HX_ASSERT(m_header.ulStreams == m_streamArray.GetSize());

    IHXBuffer* pTitle = NULL;
    IHXBuffer* pAuthor = NULL;
    IHXBuffer* pCopyright = NULL;

    IHXValues* pHeader;
    if (FAILED(m_pCommonClassFactory->CreateInstance(CLSID_IHXValues,
                                                     (void**)&pHeader)) ||
        FAILED(pHeader->SetPropertyULONG32("StreamCount", m_header.ulStreams)))
    {
        m_state = AS_InitPending;
        m_pFFResponse->FileHeaderReady(HXR_OUTOFMEMORY, pHeader);
        return NULL;
    }

    if (SUCCEEDED(m_pCommonClassFactory->CreateInstance(CLSID_IHXBuffer,
                                                        (void**) &pTitle)) &&
        SUCCEEDED(m_pCommonClassFactory->CreateInstance(CLSID_IHXBuffer,
                                                        (void**) &pAuthor)) &&
        SUCCEEDED(m_pCommonClassFactory->CreateInstance(CLSID_IHXBuffer,
                                                        (void**) &pCopyright)))
    {
        if ( m_pszTitle )
        {
            pTitle->Set((const UCHAR*)m_pszTitle, strlen(m_pszTitle)+1);
            pHeader->SetPropertyBuffer("Title", pTitle);
            //HX_TRACE("\tTitle:\t%s\n", m_pszTitle);
        }

        if ( m_pszAuthor )
        {
            pAuthor->Set((const UCHAR*)m_pszAuthor, strlen(m_pszAuthor)+1);
            pHeader->SetPropertyBuffer("Author", pAuthor);
            //HX_TRACE("\tAuthor:\t%s\n", m_pszAuthor);
        }

        if ( m_pszCopyright )
        {
            pCopyright->Set((const UCHAR*)m_pszCopyright, strlen(m_pszCopyright)+1);
            pHeader->SetPropertyBuffer("Copyright", pCopyright);
            //HX_TRACE("\tCopyright:\t%s\n", m_pszCopyright);
        }
    }

    // XXXKB Unconditionally enable recording?  Can this be improved??
    pHeader->SetPropertyULONG32("Flags", HX_SAVE_ENABLED);
    //HX_TRACE("\tFlags:\t%lu\n", HX_SAVE_ENABLED);

    // XXXKB Disable seeking if we have no index?

    HX_RELEASE(pTitle);
    HX_RELEASE(pAuthor);
    HX_RELEASE(pCopyright);

    return pHeader;
}


void CAVIFileFormat::IOEvent()
{
    //HX_TRACE("CAVIFileFormat::IOEvent\n\tstate=%lu\n", m_state);
    HX_ASSERT(m_state == AS_IndexFileInit || m_state == AS_IOEvent ||
              m_state == AS_Closed);

    if (m_state == AS_Closed)
    {
        return;
    }

    if (m_state == AS_IndexFileInit)
    {
        m_state = AS_Ready;
        // Index has finished all necessary file operations
        for (UINT16 i = 0; i < m_header.ulStreams; ++i)
        {
            HX_ASSERT(m_streamArray[i]);
            CAVIStream* pStream = (CAVIStream*) m_streamArray[i];

            if (pStream->PendingHeaderRequest())
            {
                pStream->SetIndex(m_pIndex);
                IHXValues* pHeader = NULL;
                if (SUCCEEDED(m_pCommonClassFactory->CreateInstance(CLSID_IHXValues,
                                                                    (void**)&pHeader)))
                {
                    HX_ASSERT_VALID_PTR(pStream);

                    if (SUCCEEDED(pStream->GetHeader(pHeader)))
                    {
                        m_pFFResponse->StreamHeaderReady(HXR_OK, pHeader);
                    }
                    else
                    {
                        HX_ASSERT(!"Bad stream header!\n");
                        m_pFFResponse->StreamHeaderReady(HXR_FAIL, NULL);
                    }

                    HX_RELEASE(pHeader);
                }
                else
                {
                    m_pFFResponse->StreamHeaderReady(HXR_OUTOFMEMORY, NULL);
                }
            }
        }
    }
    else
    {
        m_state = AS_Ready;
        ScanState();
    }
}

BOOL CAVIFileFormat::IsAVChunk(UINT32 ulChunkId)
{
    return ((char) ulChunkId - '0') < 10 && ((char) (ulChunkId >> 8) - '0') < 10;
}

UINT16 CAVIFileFormat::GetStream(UINT32 ulChunkId)
{
    return ((char) ulChunkId - '0')*10 + ((char) (ulChunkId >> 8) - '0');
}
