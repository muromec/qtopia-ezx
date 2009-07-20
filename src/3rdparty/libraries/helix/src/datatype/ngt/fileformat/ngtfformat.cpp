/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: ngtfformat.cpp,v 1.7 2008/07/02 14:47:22 gwright Exp $
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
 * terms of the GNU General Public License Version 2 (the
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

/****************************************************************************
 * Defines
 */
#define NGTFF_LOCAL_STREAMS	FALSE
#define NGTFF_REMOTE_STREAMS	TRUE

#define NGTFF_LOCAL_SEEK_DURATION_PERCENT_THRESHOLD 0.2


/****************************************************************************
 * Includes
 */
#include "ngtfformat.ver"

#include "ngtfformat.h"
#include "hxtlogutil.h"
#include "pckunpck.h"
#include "hxtlogutil.h"
#include "hxver.h"

#include "hlxclib/string.h"
#include "hlxclib/memory.h"
#include "pckunpck.h"

#include "ngtfileobject.h"


/****************************************************************************
 * Constants
 */
const char* const CNGTFileFormat::zm_pDescription    = "Helix Media Nugget File Format Plugin";
const char* const CNGTFileFormat::zm_pCopyright      = HXVER_COPYRIGHT;
const char* const CNGTFileFormat::zm_pMoreInfoURL    = HXVER_MOREINFO;

const char* const CNGTFileFormat::zm_pFileMimeTypes[]  = {"application/x-hx-nugt", NULL};
const char* const CNGTFileFormat::zm_pFileExtensions[] = {"ngt", NULL};
const char* const CNGTFileFormat::zm_pFileOpenNames[] = {"Media Nugget Files (*.ngt)", NULL};

const char* const CNGTFileFormat::m_zpNGTFileFormatStateToStringMap[] = 
{
    "Offline",
    "Initializing",
    "Ready",
    "LocalSeeking",
    "RemoteSeeking"
};

const char* const CNGTFileFormat::m_zpNGTRemoteSourceStateToStringMap[] = 
{
    "Offline",
    "Connecting",
    "Connected",
    "Expired",
    "Incomaptible",
    "Unavailable",
    "Error"
};

const char* const CNGTFileFormat::m_zpNGTFileFormatStreamStateToStringMap[] = 
{
    "LocalSelected",
    "RemoteSelectedPendingAlignment",
    "RemoteSelected"
};

const char* const CNGTFileFormat::m_zpNGTFFSourceTypeToStringMap[] = 
{
    "Local",
    "Remote"
};


/****************************************************************************
 *  CNGTFileFormat
 */
/****************************************************************************
 *  Interface
 */
BEGIN_INTERFACE_LIST_NOCREATE(CNGTFileFormat)
INTERFACE_LIST_ENTRY_SIMPLE(IHXPlugin)
INTERFACE_LIST_ENTRY_SIMPLE(IHXFileFormatObject)
INTERFACE_LIST_ENTRY_SIMPLE(IHXMetaFileFormatResponse)
INTERFACE_LIST_ENTRY_SIMPLE(IHXAdvise)
END_INTERFACE_LIST


/****************************************************************************
 *  Constructor/Destructor
 */
CNGTFileFormat::CNGTFileFormat()
    : m_pContext(NULL)
    , m_pClassFactory(NULL)
    , m_pFFResponse(NULL)
    , m_pRequest(NULL)
    , m_pErrorMessages(NULL)
    , m_pLocalFileFormat(NULL)
    , m_pRemoteFileFormat(NULL)
    , m_pNGTFileObject(NULL)
    , m_pLocalFileHeader(NULL)
    , m_pRemoteFileHeader(NULL)
    , m_uNumStreams(0)
    , m_pStreamStatus(NULL)
    , m_eState(NGTFF_Offline)
    , m_eRemoteSourceState(NGTRS_Offline)
    , m_ulRemoteFFSeekPending(NGTFF_INVALID_TIMESTAMP)
    , m_bExcludeRemoteSource(FALSE)
{
    ;
}

CNGTFileFormat::~CNGTFileFormat()
{
    Close();

    m_uNumStreams = 0;
    HX_VECTOR_DELETE(m_pStreamStatus);
}


/************************************************************************
 *  IHXPlugin methods
 */
/************************************************************************
 *  IHXPlugin::InitPlugin
 *  Purpose:
 *    Initializes the plugin for use. This interface must always be
 *    called before any other method is called. This is primarily needed 
 *    so that the plugin can have access to the context for creation of
 *    IHXBuffers and IMalloc.
 */
STDMETHODIMP CNGTFileFormat::InitPlugin(IUnknown* /*IN*/ pContext)
{
    HX_RESULT retVal = HXR_OK;

    if (pContext)
    {
	m_pContext = pContext;
	m_pContext->AddRef();

	HX_ENABLE_LOGGING(pContext);
    }
    else
    {
	retVal = HXR_INVALID_PARAMETER;
    }

    if (SUCCEEDED(retVal))
    {
	HX_ASSERT(!m_pErrorMessages);
	pContext->QueryInterface(IID_IHXErrorMessages, 
		    (void**) &m_pErrorMessages);

	retVal = pContext->QueryInterface(IID_IHXCommonClassFactory,
		    (void**)&m_pClassFactory);
    }

    return retVal;
}

/************************************************************************
 *  IHXPlugin::GetPluginInfo
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
STDMETHODIMP CNGTFileFormat::GetPluginInfo
(
    REF(HXBOOL)		bLoadMultiple,
    REF(const char*)	pDescription,
    REF(const char*)	pCopyright,
    REF(const char*)	pMoreInfoURL,
    REF(ULONG32)	ulVersionNumber
)
{
    bLoadMultiple = TRUE;   // Must be true for file formats.

    pDescription    = (const char*) zm_pDescription;
    pCopyright	    = (const char*) zm_pCopyright;
    pMoreInfoURL    = (const char*) zm_pMoreInfoURL;
    ulVersionNumber = TARVER_ULONG32_VERSION;

    return HXR_OK;
}


/************************************************************************
 *  IHXFileFormatObject methods
 */
/************************************************************************
 *  GetFileFormatInfo
 *  Purpose:
 *    If this object is a file format object this method returns
 *    information vital to the instantiation of file format plugins.
 *    If this object is not a file format object, it should return
 *    HXR_UNEXPECTED.
 */
STDMETHODIMP CNGTFileFormat::GetFileFormatInfo
(
    REF(const char**) /*OUT*/ pFileMimeTypes,
    REF(const char**) /*OUT*/ pFileExtensions,
    REF(const char**) /*OUT*/ pFileOpenNames
)
{
    pFileMimeTypes  = (const char**) zm_pFileMimeTypes;
    pFileExtensions = (const char**) zm_pFileExtensions;
    pFileOpenNames  = (const char**) zm_pFileOpenNames;

    return HXR_OK;
}

/************************************************************************
 *  InitFileFormat
 */
STDMETHODIMP CNGTFileFormat::InitFileFormat
(
    IHXRequest*		/*IN*/	pRequest, 
    IHXFormatResponse*	/*IN*/	pFileFormatResponse,
    IHXFileObject*	/*IN*/  pFileObject
)
{
    HX_RESULT retVal = HXR_OK;

    HXLOGL1(HXLOG_NGTF, "CNGTFileFormat::InitFileFormat() State=%s RemoteState=%s",
	    StateToString(m_eState),
	    StateToString(m_eRemoteSourceState));

    if ((m_eState != NGTFF_Offline) || (!m_pContext))
    {
	return HXR_UNEXPECTED;
    }

    if ((pFileFormatResponse == NULL) || (pFileObject == NULL))
    {
	retVal = HXR_INVALID_PARAMETER;
    }

    if (SUCCEEDED(retVal))
    {
	m_pRequest = pRequest;
	HX_ADDREF(m_pRequest);

	m_pFFResponse = pFileFormatResponse;
	m_pFFResponse->AddRef();

	m_pNGTFileObject = new CNGTFileObject();
	retVal = HXR_OUTOFMEMORY;
	if (m_pNGTFileObject)
	{
	    m_pNGTFileObject->AddRef();
	    retVal = HXR_OK;
	}
    }

    if (SUCCEEDED(retVal))
    {
	m_eState = NGTFF_Initializing;
	retVal = m_pNGTFileObject->InitNugget(m_pContext,
					      pFileObject,
					      HX_FILE_READ | HX_FILE_BINARY,					      
					      this);	// IHXMetaFileFormatResponse
    }

    return retVal;
}

/************************************************************************
 *  Close
 */
STDMETHODIMP CNGTFileFormat::Close()
{
    HXLOGL1(HXLOG_NGTF, "CNGTFileFormat::Close() State=%s RemoteState=%s",
	    StateToString(m_eState),
	    StateToString(m_eRemoteSourceState));

    m_eState = NGTFF_Offline;
    m_eRemoteSourceState = NGTRS_Offline;
    m_ulRemoteFFSeekPending = NGTFF_INVALID_TIMESTAMP;

    m_uNumStreams = 0;
    HX_VECTOR_DELETE(m_pStreamStatus);

    HX_RELEASE(m_pLocalFileHeader);
    HX_RELEASE(m_pRemoteFileHeader);

    HX_RELEASE(m_pRequest);
    HX_RELEASE(m_pFFResponse);

    if (m_pNGTFileObject)
    {
	m_pNGTFileObject->CloseNugget();
	HX_RELEASE(m_pNGTFileObject);
    }

    if (m_pRemoteFileFormat)
    {
	m_pRemoteFileFormat->Close();
	HX_RELEASE(m_pRemoteFileFormat);
    }

    if (m_pLocalFileFormat)
    {
	m_pLocalFileFormat->Close();
	HX_RELEASE(m_pLocalFileFormat);
    }

    HX_RELEASE(m_pClassFactory);
    HX_RELEASE(m_pErrorMessages);
    HX_RELEASE(m_pContext);

    return HXR_OK;
}

/************************************************************************
 *  GetFileHeader
 */
STDMETHODIMP CNGTFileFormat::GetFileHeader()
{
    HX_RESULT retVal = HXR_UNEXPECTED;

    HXLOGL1(HXLOG_NGTF, "CNGTFileFormat::GetFileHeader() State=%s RemoteState=%s",
	    StateToString(m_eState),
	    StateToString(m_eRemoteSourceState));

    if (m_pFFResponse && m_pNGTFileObject)
    {
	retVal = HXR_OK;
	if (m_pNGTFileObject->IsKnownVersion())
	{
	    if (m_pLocalFileHeader)
	    {
		retVal = m_pFFResponse->FileHeaderReady(HXR_OK, m_pLocalFileHeader);
	    }
	    else if (m_pLocalFileFormat)
	    {
		retVal = m_pLocalFileFormat->GetFileHeader();
	    }
	}
	else
	{
	    retVal = m_pFFResponse->FileHeaderReady(HXR_REQUEST_UPGRADE, NULL);
	}
    }

    return retVal;
}

/************************************************************************
 *  GetStreamHeader
 */
STDMETHODIMP CNGTFileFormat::GetStreamHeader(UINT16 unStreamNumber)
{
    HX_RESULT retVal = HXR_UNEXPECTED;

    HXLOGL1(HXLOG_NGTF, "CNGTFileFormat::GetStreamHeader(Strm=%hu) State=%s RemoteState=%s",
	    unStreamNumber,
	    StateToString(m_eState),
	    StateToString(m_eRemoteSourceState));

    if (m_pFFResponse && m_pStreamStatus && (unStreamNumber < m_uNumStreams))
    {
	if (m_pStreamStatus[unStreamNumber].pLocalHeader)
	{
	    retVal = m_pFFResponse->StreamHeaderReady(HXR_OK, 
						      m_pStreamStatus[unStreamNumber].pLocalHeader);
	}
	else if (m_pLocalFileFormat)
	{
	    retVal = m_pLocalFileFormat->GetStreamHeader(unStreamNumber);
	}
    }

    return retVal;
}

/************************************************************************
 *  GetPacket
 *  Method:
 *	IHXFileFormatObject::GetPacket
 *  Purpose:
 *	Called by controller to ask the file format for the next packet
 *	for a particular stream in the file. The file format should call
 *	IHXFileFormatResponse::Data*() for the IHXFileFormatResponse
 *	object that was passed in during initialization, when the packet
 *	is available.
 */
STDMETHODIMP CNGTFileFormat::GetPacket(UINT16 unStreamNumber)
{
    HX_RESULT retVal = HXR_UNEXPECTED;

    HXLOGL4(HXLOG_NGTF, "CNGTFileFormat::GetPacket(Strm=%hu) StrmState=%s State=%s RemoteState=%s",
	    unStreamNumber,
	    StateToString(m_pStreamStatus[unStreamNumber].eState),
	    StateToString(m_eState),
	    StateToString(m_eRemoteSourceState));

    if ((m_eState == NGTFF_Ready) && 
	(unStreamNumber < m_uNumStreams) &&
	(!m_pStreamStatus[unStreamNumber].bStreamDone) &&
	(!m_pStreamStatus[unStreamNumber].bStreamPending))
    {
	if (m_pStreamStatus[unStreamNumber].eState == NGTFFSTRM_LocalSelected)
	{
	    m_pStreamStatus[unStreamNumber].bStreamPending = TRUE;
	    m_pStreamStatus[unStreamNumber].bLocalPending = TRUE;
	    
	    retVal = m_pLocalFileFormat->GetPacket(unStreamNumber);

	    // If local packet is still panding after request, we need
	    // to pull packets from local file format that have been switched
	    // to the remote source since local file format may hold back
	    // packets for a stream and enforcing sorted order of packets
	    // accross streams.
	    if (SUCCEEDED(retVal))
	    { 
		if ((m_pStreamStatus[unStreamNumber].eState == NGTFFSTRM_LocalSelected) &&
		    m_pStreamStatus[unStreamNumber].bLocalPending)
		{
		    RequestRemoteStreamPacketsFromLocalSource();
		}
	    }
	    else
	    {
		m_pStreamStatus[unStreamNumber].bLocalPending = FALSE;
	    }
	}
	else if (m_pStreamStatus[unStreamNumber].eState >= 
		 NGTFFSTRM_RemoteSelectedPendingAlignment)
	{
	    // Remote source is selected - possibly still aligning
	    m_pStreamStatus[unStreamNumber].bStreamPending = TRUE;

	    if (m_eRemoteSourceState == NGTRS_Connected)
	    {
		retVal = m_pRemoteFileFormat->GetPacket(unStreamNumber);
	    }
	    else if (m_eRemoteSourceState == NGTRS_Connecting)
	    {
		HX_ASSERT(m_pStreamStatus[unStreamNumber].eState == NGTFFSTRM_RemoteSelectedPendingAlignment);
		retVal = HXR_OK;
	    }
	    else
	    {
		// remote source is in error or offline state
		m_pStreamStatus[unStreamNumber].bStreamPending = FALSE;
		m_pStreamStatus[unStreamNumber].bStreamDone = TRUE;
		retVal = m_pFFResponse->StreamDone(unStreamNumber);
	    }
	}
    }

    return retVal;
}

/************************************************************************
 *  Seek
 */
STDMETHODIMP CNGTFileFormat::Seek(UINT32 ulOffset)
{
    HX_RESULT retVal = HXR_UNEXPECTED;

    HXLOGL1(HXLOG_NGTF, "CNGTFileFormat::Seek(Offset=%lu) State=%s RemoteState=%s",
	    ulOffset,
	    StateToString(m_eState),
	    StateToString(m_eRemoteSourceState));

    if ((m_eState == NGTFF_Ready) ||
	(m_eState == NGTFF_LocalSeeking) ||
	(m_eState == NGTFF_RemoteSeeking))
    {
	UINT16 uStrmIdx;
	UINT32 ulRemoteOffset = 0;
	UINT32 ulLocalSeekThreashold = (UINT32) (m_pNGTFileObject->GetNuggetLocalDuration() * 
						 NGTFF_LOCAL_SEEK_DURATION_PERCENT_THRESHOLD);

	retVal = HXR_OK;

	if ((ulOffset > ulLocalSeekThreashold) &&
	    m_pRemoteFileFormat &&
	    IsRemoteSourceHalthy())
	{
	    m_eState = NGTFF_RemoteSeeking;
	    ulRemoteOffset = ulOffset;
	}
	else
	{
	    m_eState = NGTFF_LocalSeeking;
	    ulRemoteOffset = m_pNGTFileObject->GetNuggetLocalDuration();
	    if (ulRemoteOffset < ulOffset)
	    {
		ulRemoteOffset = ulOffset;
	    }
	}

	for (uStrmIdx = 0; uStrmIdx < m_uNumStreams; uStrmIdx++)
	{
	    m_pStreamStatus[uStrmIdx].Reset();
	    if (m_eState == NGTFF_RemoteSeeking)
	    {
		m_pStreamStatus[uStrmIdx].eState = NGTFFSTRM_RemoteSelected;
	    }
	}

	if (m_pRemoteFileFormat &&
	    (m_eRemoteSourceState == NGTRS_Connected))
	{
	    retVal = m_pRemoteFileFormat->Seek(ulRemoteOffset);
	}
	else
	{
	    m_ulRemoteFFSeekPending = ulOffset;
	}

	if (m_eState == NGTFF_LocalSeeking)
	{
	    HX_ASSERT(m_pLocalFileFormat);

	    retVal = m_pLocalFileFormat->Seek(ulOffset);
	}
	else if (m_eState == NGTFF_RemoteSeeking)
	{
	    if ((!IsRemoteSourceHalthy()) &&
		SUCCEEDED(retVal))
	    {
		retVal = m_pFFResponse->SeekDone(HXR_FAIL);
	    }
	}
    }

    return retVal;
}


/************************************************************************
 *  IHXMetaFileFormatResponse
 */
/************************************************************************
 *  InitDone
 */
STDMETHODIMP CNGTFileFormat::InitDone(HX_RESULT status)
{
    HX_RESULT retVal = HXR_UNEXPECTED;

    HXLOGL1(HXLOG_NGTF, "CNGTFileFormat::InitDone(status=%ld) State=%s RemoteState=%s",
	    status,
	    StateToString(m_eState),
	    StateToString(m_eRemoteSourceState));

    if (m_eState == NGTFF_Initializing)
    {
	HX_ASSERT(m_pNGTFileObject);

	retVal = HXR_OK;

	if (SUCCEEDED(status) &&
	    ((m_pNGTFileObject->IsInitializedForCreation()) ||
	     (!m_pNGTFileObject->GetNuggetLocalMimeType())))
	{
	    status = HXR_FAIL;
	}

	if (SUCCEEDED(status))
	{
	    IUnknown* pUnknown = NULL;
	    IHXPluginHandler3* pPluginHandler3 = NULL;

	    // Set where remote source should start streaming from.
	    m_ulRemoteFFSeekPending = m_pNGTFileObject->GetNuggetLocalDuration();

	    status = m_pContext->QueryInterface(IID_IHXPluginHandler3, (void**) &pPluginHandler3);

	    // Find and initialize the local file format to handle the 
	    // local, in-nugget media data.
	    if (SUCCEEDED(status))
	    {
		status = pPluginHandler3->FindPluginUsingStrings(PLUGIN_CLASS,
								 PLUGIN_FILEFORMAT_TYPE,
								 PLUGIN_FILEMIMETYPES,
								 (char*) m_pNGTFileObject->GetNuggetLocalMimeType(),
								 NULL,	// propName3
								 NULL,	// propVal3
								 pUnknown,
								 NULL);
	    }

	    if (SUCCEEDED(status))
	    {
		status = pUnknown->QueryInterface(IID_IHXFileFormatObject, 
						  (void**) &m_pLocalFileFormat);
	    }

	    HX_RELEASE(pUnknown);

	    if (SUCCEEDED(status))
	    {
		CNGTFormatResponseReceiver* pResponse = new CNGTFormatResponseReceiver(this,
										       NUGTFFSourceType_Local);

		status = HXR_OUTOFMEMORY;
		if (pResponse)
		{
		    pResponse->AddRef();
		    status = InitChildFileFormat(m_pLocalFileFormat,
						 m_pRequest,
						 pResponse,
						 m_pNGTFileObject);
		}

		HX_RELEASE(pResponse);
	    }

	    // Find and initialize the remote source object to handle the remote stream
	    // referenced via URL in nugget file meta-data portion.
	    // If there is no remote URL in nugget meta-file, simply skip remote
	    // source initialization completely.  The playback will be handled
	    // without any remote source considertaions in such case.
	    if (SUCCEEDED(status) &&
		(!m_bExcludeRemoteSource) &&
		m_pNGTFileObject->GetNuggetRemoteSourceURL())
	    {
		if (m_pNGTFileObject->IsExpired())
		{
		    HandleRemoteSourceFailure(NGTRS_Expired);
		    status = HXR_FAIL;
		}

		if (SUCCEEDED(status))
		{
		    status = pPluginHandler3->FindPluginUsingStrings(PLUGIN_CLASS,
								     PLUGIN_FILEFORMAT_TYPE,
								     PLUGIN_FILEMIMETYPES,
								     "application/x-hx-source",
								     NULL,	// propName3
								     NULL,	// propVal3
								     pUnknown,
								     NULL);
		}

		if (SUCCEEDED(status))
		{
		    status = pUnknown->QueryInterface(IID_IHXFileFormatObject, 
						      (void**) &m_pRemoteFileFormat);
		}

		HX_RELEASE(pUnknown);

		if (SUCCEEDED(status))
		{
		    IHXRequest* pRemoteRequest = NULL;
		    CNGTFormatResponseReceiver* pResponse = new CNGTFormatResponseReceiver(this,
											   NUGTFFSourceType_Remote);

		    status = HXR_OUTOFMEMORY;
		    if (pResponse)
		    {
			pResponse->AddRef();
			status = InitRequest(pRemoteRequest, 
					     m_pNGTFileObject->GetNuggetRemoteSourceURL(),
					     NULL); // request headers
		    }

		    if (SUCCEEDED(status))
		    {
			pResponse->AddRef();
			m_eRemoteSourceState = NGTRS_Connecting;
			status = InitChildFileFormat(m_pRemoteFileFormat,
						     pRemoteRequest,
						     pResponse,
						     NULL); // pFileObject

			if (FAILED(status))
			{
			    HandleRemoteSourceFailure(NGTRS_Unavailable);
			}
		    }

		    HX_RELEASE(pRemoteRequest);
		    HX_RELEASE(pResponse);
		}

		if (FAILED(status) && (m_eRemoteSourceState == NGTRS_Offline))
		{
		    HandleRemoteSourceFailure(NGTRS_Error);
		    // If wecannot get to remote source, we'll proceed with local
		    // content as far as we can.
		    status = HXR_OK;
		}
	    }
	}

	if (FAILED(status))
	{
	    retVal = m_pFFResponse->InitDone(status);
	}
    }

    return retVal;
}


/************************************************************************
 *  IHXAdvise method
 */
/************************************************************************
 *  Method:
 *      IHXAdvise::Advise
 *  Purpose:
 *      Allows IHXFileObject or Core to query its IHXFileResponse about
 *      usage heuristics. It should pass HX_FILERESPONSEADVISE_xxx
 *      flags into this method.
 */
STDMETHODIMP CNGTFileFormat::Advise(ULONG32 ulInfo)
{
    HX_RESULT retVal = HXR_FAILED;

    if (ulInfo == HX_FILERESPONSEADVISE_LOCALACCESSONLY)
    {
	if (!m_pRemoteFileFormat)
	{
	    m_bExcludeRemoteSource = TRUE;
	    retVal = HXR_OK;
	}
    }

    return retVal;
}


/************************************************************************
 *  Public methods
 */
HX_RESULT CNGTFileFormat::InitDone(NGTFFSourceType eSourceType, 
				   HX_RESULT status)
{
    HX_RESULT retVal = HXR_UNEXPECTED;

    HXLOGL2(HXLOG_NGTF, "CNGTFileFormat::ChildInitDone(Type=%s, status=%ld) State=%s RemoteState=%s",
	    StateToString(eSourceType),
	    status,
	    StateToString(m_eState),
	    StateToString(m_eRemoteSourceState));

    if (eSourceType == NUGTFFSourceType_Local)
    {
	HX_ASSERT(m_eState == NGTFF_Initializing);

	if (m_pFFResponse && (m_eState == NGTFF_Initializing))
	{
	    m_eState = NGTFF_Ready;
	    retVal = m_pFFResponse->InitDone(status);
	}
    }
    else if (eSourceType == NUGTFFSourceType_Remote)
    {
	HX_ASSERT(m_eRemoteSourceState == NGTRS_Connecting);

	if (m_pRemoteFileFormat && (m_eRemoteSourceState == NGTRS_Connecting))
	{
	    if (SUCCEEDED(status))
	    {
		retVal = m_pRemoteFileFormat->GetFileHeader();
	    }
	    else
	    {
		HandleRemoteSourceFailure(NGTRS_Unavailable);
	    }
	}
    }

    return retVal;
}

HX_RESULT CNGTFileFormat::FileHeaderReady(NGTFFSourceType eSourceType, 
					  HX_RESULT status,
					  IHXValues* pHeader)
{
    HX_RESULT retVal = HXR_UNEXPECTED;

    HXLOGL2(HXLOG_NGTF, "CNGTFileFormat::ChildFileHeaderReady(Type=%s, status=%ld) State=%s RemoteState=%s",
	    StateToString(eSourceType),
	    status,
	    StateToString(m_eState),
	    StateToString(m_eRemoteSourceState));

    if (eSourceType == NUGTFFSourceType_Local)
    {
	if (!m_pLocalFileHeader)
	{
	    if (SUCCEEDED(status))
	    {
		m_pLocalFileHeader = pHeader;
		m_pLocalFileHeader->AddRef();

		HX_ASSERT(m_pNGTFileObject);

		if (m_pNGTFileObject)
		{
		    m_pLocalFileHeader->SetPropertyULONG32("NuggetExpiration", m_pNGTFileObject->GetNuggetExpiration());
		    m_pLocalFileHeader->SetPropertyULONG32("NuggetConnectTime", m_pNGTFileObject->GetNuggetConnectTime());
		    m_pLocalFileHeader->SetPropertyULONG32("NuggetLocalDuration", m_pNGTFileObject->GetNuggetLocalDuration());
		    if (m_pNGTFileObject->GetNuggetRemoteSourceURL())
		    {
			SetCStringPropertyCCF(m_pLocalFileHeader,
					      "NuggetRemoteSourceURL",
					      m_pNGTFileObject->GetNuggetRemoteSourceURL(),
					      m_pContext);
		    }

		    if (m_pNGTFileObject->GetNuggetOverallDuration() != NGTFF_INVALID_TIMESTAMP)
		    {
			UINT32 ulVal;

			m_pLocalFileHeader->SetPropertyULONG32("NuggetOverallDuration", m_pNGTFileObject->GetNuggetOverallDuration());

			if (SUCCEEDED(m_pLocalFileHeader->GetPropertyULONG32("Duration", ulVal)) &&
			    (ulVal != m_pNGTFileObject->GetNuggetOverallDuration()))
			{
			    status = m_pLocalFileHeader->SetPropertyULONG32("Duration", 
									    m_pNGTFileObject->GetNuggetOverallDuration());
			} 
		    }
		}

		if (SUCCEEDED(status))
		{
		    UINT32 ulNumStreams = 0;
		    status = m_pLocalFileHeader->GetPropertyULONG32("StreamCount", ulNumStreams);

		    m_uNumStreams = (UINT16) ulNumStreams;

		    if ((m_uNumStreams == 0) && SUCCEEDED(status))
		    {
			status = HXR_FAIL;
		    }
		}

		if (SUCCEEDED(status))
		{
		    HX_ASSERT(!m_pStreamStatus);

		    m_pStreamStatus = new CStreamStatus[m_uNumStreams];
		    status = HXR_OUTOFMEMORY;
		    if (m_pStreamStatus)
		    {
			status = HXR_OK;
		    }
		}
		else
		{
		    status = HXR_FAIL;
		}

		// In case remote file header is already here, kick of 
		// fetching of remote stream headers now that we know the 
		// number of streams.
		// The number of streams is always driven by the local
		// nugget media and remote content must be compatbile with it.
		// In case of incompatibility, local media is played
		// only.
		if (SUCCEEDED(status) && 
		    m_pRemoteFileHeader && 
		    (m_eRemoteSourceState == NGTRS_Connecting))
		{
		    FetchRemoteStreamHeaders();
		}
	    }

	    if (m_pFFResponse)
	    {
		retVal = m_pFFResponse->FileHeaderReady(status, m_pLocalFileHeader);
	    }
	}
    }
    else if (eSourceType == NUGTFFSourceType_Remote)
    {
	if (!m_pRemoteFileHeader)
	{
	    retVal = HXR_OK;

	    if (SUCCEEDED(status) && (m_eRemoteSourceState == NGTRS_Connecting))
	    {
		m_pRemoteFileHeader = pHeader;
		m_pRemoteFileHeader->AddRef();

		if (m_pLocalFileHeader)
		{
		    FetchRemoteStreamHeaders();
		}
	    }	
	}
    }

    return retVal;
}

HX_RESULT CNGTFileFormat::StreamHeaderReady(NGTFFSourceType eSourceType, 
					    HX_RESULT status,
					    IHXValues* pHeader)
{
    HX_RESULT retVal = HXR_UNEXPECTED;

    // If stream status is initialized, we are prepared to receive 
    // stream headers.
    if (m_pStreamStatus)
    {
	UINT32 ulStreamNumber = 0;
    
	if (SUCCEEDED(status) && pHeader)
	{
	    status = pHeader->GetPropertyULONG32("StreamNumber", ulStreamNumber);
	}

	HXLOGL2(HXLOG_NGTF, "CNGTFileFormat::ChildStreamHeaderReady(Type=%s, status=%ld, Strm=%lu) State=%s RemoteState=%s",
	    StateToString(eSourceType),
	    status,
	    ulStreamNumber,
	    StateToString(m_eState),
	    StateToString(m_eRemoteSourceState));

	if ((eSourceType == NUGTFFSourceType_Local) &&
	    (!m_pStreamStatus[ulStreamNumber].pLocalHeader))
	{
	    if (SUCCEEDED(status))
	    {
		m_pStreamStatus[ulStreamNumber].pLocalHeader = pHeader;
		pHeader->AddRef();

		HX_ASSERT(m_pNGTFileObject);

		if (m_pNGTFileObject && (m_pNGTFileObject->GetNuggetOverallDuration() != NGTFF_INVALID_TIMESTAMP))
		{
		    UINT32 ulVal;

		    if (SUCCEEDED(pHeader->GetPropertyULONG32("Duration", ulVal)))
		    {
			status = pHeader->SetPropertyULONG32("Duration", 
							     m_pNGTFileObject->GetNuggetOverallDuration());
		    }
		}

		if (SUCCEEDED(status) &&
		    (m_eRemoteSourceState == NGTRS_Connecting) &&
		    AreAllStreamHeadersReceived(NGTFF_LOCAL_STREAMS) &&
		    AreAllStreamHeadersReceived(NGTFF_REMOTE_STREAMS))
		{
		    StartRemoteSource();
		}
	    }

	    if (m_pFFResponse)
	    {
		retVal = m_pFFResponse->StreamHeaderReady(status, pHeader);
	    }
	}
	else if ((eSourceType == NUGTFFSourceType_Remote) &&
		 (!m_pStreamStatus[ulStreamNumber].pRemoteHeader))
	{
	    if (SUCCEEDED(status) && (m_eRemoteSourceState == NGTRS_Connecting))
	    {
		m_pStreamStatus[ulStreamNumber].pRemoteHeader = pHeader;
		pHeader->AddRef();

		if (AreAllStreamHeadersReceived(NGTFF_LOCAL_STREAMS) &&
		    AreAllStreamHeadersReceived(NGTFF_REMOTE_STREAMS))
		{
		    StartRemoteSource();
		}
	    }
	    else if (m_eRemoteSourceState == NGTRS_Connecting)
	    {
		HandleRemoteSourceFailure(NGTRS_Error);
	    }

	    retVal = HXR_OK;
	}
    }

    return retVal;
}

HX_RESULT CNGTFileFormat::PacketReady(NGTFFSourceType eSourceType,
				      HX_RESULT status, 
				      IHXPacket* pPacket)
{
    HX_RESULT retVal = HXR_UNEXPECTED;
    UINT16 uStreamNumber = 0;
    
    if (!pPacket || FAILED(status))
    {
	return HXR_OK;
    }
    
    uStreamNumber = pPacket->GetStreamNumber();

    HXLOGL4(HXLOG_NGTF, "CNGTFileFormat::ChildPacketReady(Type=%s, Strm=%hu, Time=%lu, Lost=%c) StrmState=%s State=%s RemoteState=%s",
	    StateToString(eSourceType),
	    uStreamNumber,
	    pPacket->GetTime(),
	    pPacket->IsLost() ? 'T' : 'F',
	    StateToString(m_pStreamStatus[uStreamNumber].eState),
	    StateToString(m_eState),
	    StateToString(m_eRemoteSourceState));

    if (uStreamNumber < m_uNumStreams)
    {
	if (eSourceType == NUGTFFSourceType_Local)
	{
	    HX_ASSERT(m_pStreamStatus[uStreamNumber].bLocalPending);

	    if (m_pStreamStatus[uStreamNumber].bLocalPending)
	    {
		retVal = HXR_OK;

		m_pStreamStatus[uStreamNumber].bLocalPending = FALSE;

		if (m_pStreamStatus[uStreamNumber].eState == NGTFFSTRM_LocalSelected)
		{
		    HX_ASSERT(m_pStreamStatus[uStreamNumber].bStreamPending);

		    pPacket->AddRef();

		    if (!pPacket->IsLost())
		    {
			m_pStreamStatus[uStreamNumber].ulLastLocalTS = pPacket->GetTime();

			if (m_pStreamStatus[uStreamNumber].pRemoteSwitchToPacket &&
			    (m_pStreamStatus[uStreamNumber].pRemoteSwitchToPacket->GetTime() <= 
			     m_pStreamStatus[uStreamNumber].ulLastLocalTS))
			{
			    // Perform the switch to the remote stream
			    m_pStreamStatus[uStreamNumber].eState = NGTFFSTRM_RemoteSelected;
			    pPacket = m_pStreamStatus[uStreamNumber].pRemoteSwitchToPacket;
			    m_pStreamStatus[uStreamNumber].pRemoteSwitchToPacket = NULL;

			    HXLOGL2(HXLOG_NGTF, "CNGTFileFormat::SwitchToRemote(Strm=%hu, Time=%lu) LastLocalTime=%lu StrmState=%s State=%s RemoteState=%s",
				uStreamNumber,
				pPacket->GetTime(),
				m_pStreamStatus[uStreamNumber].ulLastLocalTS,
				StateToString(m_pStreamStatus[uStreamNumber].eState),
				StateToString(m_eState),
				StateToString(m_eRemoteSourceState));
			}
		    }

		    m_pStreamStatus[uStreamNumber].bStreamPending = FALSE;

		    retVal = m_pFFResponse->PacketReady(status, pPacket);

		    pPacket->Release();
		    pPacket = NULL;
		}

		if (m_pStreamStatus[uStreamNumber].eState != NGTFFSTRM_LocalSelected)
		{
		    // We either received a local packet for a stream that is in remote 
		    // or we just switched to the remote mode upon receiving a local
		    // packet that met the switch point.
		    // We request for packets from the local file format for any stream 
		    // in remote mode in case we still have locally pending packets in 
		    // order to avoid any holding back of packets by the file-format 
		    // due to the time-stamp order enforcement.
		    if (AreAnyStreamsLocal())
		    {
			RequestRemoteStreamPacketsFromLocalSource();
		    }
		}
	    }
	}
	else if (eSourceType == NUGTFFSourceType_Remote)
	{
	    retVal = HXR_OK;

	    if (m_pStreamStatus[uStreamNumber].eState == NGTFFSTRM_RemoteSelected)
	    {
		HX_ASSERT(m_pStreamStatus[uStreamNumber].bStreamPending);

		m_pStreamStatus[uStreamNumber].bStreamPending = FALSE;
		retVal = m_pFFResponse->PacketReady(status, pPacket);
	    }
	    else if ((m_pStreamStatus[uStreamNumber].eState == NGTFFSTRM_RemoteSelectedPendingAlignment) ||
		     (m_pStreamStatus[uStreamNumber].eState == NGTFFSTRM_LocalSelected))
	    {
		// look for a remote stream packet we can switch to
		if ((!pPacket->IsLost()) &&
		    ((m_pStreamStatus[uStreamNumber].ulLastLocalTS == NGTFF_INVALID_TIMESTAMP) ||
		     (pPacket->GetTime() > m_pStreamStatus[uStreamNumber].ulLastLocalTS)))
		{
		    // We found remote stream packet we can switch to
		    if (m_pStreamStatus[uStreamNumber].eState == NGTFFSTRM_RemoteSelectedPendingAlignment)
		    {
			// Switch to remote stream
			HX_ASSERT(m_pStreamStatus[uStreamNumber].bStreamPending);

			m_pStreamStatus[uStreamNumber].eState = NGTFFSTRM_RemoteSelected;
			m_pStreamStatus[uStreamNumber].bStreamPending = FALSE;

			HXLOGL2(HXLOG_NGTF, "CNGTFileFormat::SwitchToRemoteOnAlignment(Strm=%hu, Time=%lu) LastLocalTime=%lu StrmState=%s State=%s RemoteState=%s",
				uStreamNumber,
				pPacket->GetTime(),
				m_pStreamStatus[uStreamNumber].ulLastLocalTS,
				StateToString(m_pStreamStatus[uStreamNumber].eState),
				StateToString(m_eState),
				StateToString(m_eRemoteSourceState));
			
			retVal = m_pFFResponse->PacketReady(status, pPacket);
		    }
		    else
		    {
			// Note the packet we can switch to

			HXLOGL2(HXLOG_NGTF, "CNGTFileFormat::QueueSwitchToRemote(Strm=%hu, Time=%lu) StrmState=%s State=%s RemoteState=%s",
				uStreamNumber,
				pPacket->GetTime(),
				StateToString(m_pStreamStatus[uStreamNumber].eState),
				StateToString(m_eState),
				StateToString(m_eRemoteSourceState));

			HX_ASSERT(!m_pStreamStatus[uStreamNumber].pRemoteSwitchToPacket);
			m_pStreamStatus[uStreamNumber].pRemoteSwitchToPacket = pPacket;
			pPacket->AddRef();
		    }
		}
		else
		{
		    // We did not find a remote stream packet we can switch to yet.
		    // Request another remote stream packet
		    retVal = m_pRemoteFileFormat->GetPacket(uStreamNumber);
		}
	    }
	}
    }

    return retVal;
}

HX_RESULT CNGTFileFormat::SeekDone(NGTFFSourceType eSourceType, HX_RESULT status)
{
    HX_RESULT retVal = HXR_UNEXPECTED;

    HXLOGL2(HXLOG_NGTF, "CNGTFileFormat::ChildSeekDone(Type=%s, status=%ld) State=%s RemoteState=%s",
				StateToString(eSourceType),
				status,
				StateToString(m_eState),
				StateToString(m_eRemoteSourceState));
    
    if (eSourceType == NUGTFFSourceType_Local)
    {
	if (m_eState == NGTFF_LocalSeeking)
	{
	    HX_ASSERT(m_pFFResponse);

	    m_eState = NGTFF_Ready;

	    if (m_pFFResponse)
	    {
		retVal = m_pFFResponse->SeekDone(status);
	    }
	}
    }
    else if (eSourceType == NUGTFFSourceType_Remote)
    {
	if (m_eState == NGTFF_RemoteSeeking)
	{
	    HX_ASSERT(m_pFFResponse);

	    m_eState = NGTFF_Ready;

	    if (m_pFFResponse)
	    {
		retVal = m_pFFResponse->SeekDone(status);
	    }
	}
	else if ((m_eState == NGTFF_LocalSeeking) ||
	         (m_eState == NGTFF_Ready))
	{
	    retVal = StartRemotePackets();
	}
    }

    return retVal;
}

HX_RESULT CNGTFileFormat::StreamDone(NGTFFSourceType eSourceType,
				     UINT16 uStreamNumber)
{
    HX_RESULT retVal = HXR_UNEXPECTED;

    HXLOGL2(HXLOG_NGTF, "CNGTFileFormat::ChildStreamDone(Type=%s, Strm=%hu) State=%s RemoteState=%s",
				StateToString(eSourceType),
				uStreamNumber,
				StateToString(m_eState),
				StateToString(m_eRemoteSourceState));
    
    if (uStreamNumber < m_uNumStreams)
    {
	if (eSourceType == NUGTFFSourceType_Local)
	{
	    HX_ASSERT(m_pStreamStatus[uStreamNumber].bLocalPending);

	    if (m_pStreamStatus[uStreamNumber].bLocalPending)
	    {
		retVal = HXR_OK;

		m_pStreamStatus[uStreamNumber].bLocalPending = FALSE;
		m_pStreamStatus[uStreamNumber].bLocalDone = TRUE;
		
		if (m_pStreamStatus[uStreamNumber].eState == NGTFFSTRM_LocalSelected)
		{
		    HX_ASSERT(m_pStreamStatus[uStreamNumber].bStreamPending);

		    // Local end of stream while local stream is selected.
		    if (m_pStreamStatus[uStreamNumber].pRemoteSwitchToPacket)
		    {
			IHXPacket* pPacket;

			// remote stream is ready with packet to switch to
			HX_ASSERT((m_pStreamStatus[uStreamNumber].ulLastLocalTS == NGTFF_INVALID_TIMESTAMP) ||
				  (m_pStreamStatus[uStreamNumber].pRemoteSwitchToPacket->GetTime() >= m_pStreamStatus[uStreamNumber].ulLastLocalTS));

			// Perform the switch to the remote stream
			m_pStreamStatus[uStreamNumber].eState = NGTFFSTRM_RemoteSelected;
			pPacket = m_pStreamStatus[uStreamNumber].pRemoteSwitchToPacket;
			m_pStreamStatus[uStreamNumber].pRemoteSwitchToPacket = NULL;

			m_pStreamStatus[uStreamNumber].bStreamPending = FALSE;

			HXLOGL2(HXLOG_NGTF, "CNGTFileFormat::SwitchToRemote(Strm=%hu, Time=%lu) LastLocalTime=%lu StrmState=%s State=%s RemoteState=%s",
				uStreamNumber,
				pPacket->GetTime(),
				m_pStreamStatus[uStreamNumber].ulLastLocalTS,
				StateToString(m_pStreamStatus[uStreamNumber].eState),
				StateToString(m_eState),
				StateToString(m_eRemoteSourceState));

			retVal = m_pFFResponse->PacketReady(HXR_OK, pPacket);

			pPacket->Release();
		    }
		    else
		    {
			// Remote stream does not have packet ready to switch to.
			// We need to look for alignment before switching to remote stream
			m_pStreamStatus[uStreamNumber].eState = NGTFFSTRM_RemoteSelectedPendingAlignment;

			HXLOGL2(HXLOG_NGTF, "CNGTFileFormat::SwitchToRemotePendingAlignment(Strm=%hu) LastLocalTime=%lu StrmState=%s State=%s RemoteState=%s",
				uStreamNumber,
				m_pStreamStatus[uStreamNumber].ulLastLocalTS,
				StateToString(m_pStreamStatus[uStreamNumber].eState),
				StateToString(m_eState),
				StateToString(m_eRemoteSourceState));

			if ((!IsRemoteSourceHalthy()) ||
			    m_pStreamStatus[uStreamNumber].bRemoteDone)
			{
			    // Remote source is out of comission or this particular remote stream
			    // has already ended, there is no point in pending for alignment since 
			    // it cannot happen
			    m_pStreamStatus[uStreamNumber].bStreamPending = FALSE;
			    m_pStreamStatus[uStreamNumber].bStreamDone = TRUE;

			    HXLOGL2(HXLOG_NGTF, "CNGTFileFormat::ReportStreamDone(Strm=%hu) StrmState=%s State=%s RemoteState=%s",
				uStreamNumber,
				StateToString(m_pStreamStatus[uStreamNumber].eState),
				StateToString(m_eState),
				StateToString(m_eRemoteSourceState));

			    retVal = m_pFFResponse->StreamDone(uStreamNumber);
			}
		    }
		}
		else
		{
		    // We received a stream done for a stream that is in remote mode.
		    // This is possible when other streams are still in local mode and
		    // thus all local streams need to be serviced in order to avoid
		    // any holding back of packets by the file-format due to time-stamp
		    // order enforcement.
		    // We'll keep this stream as pending locally since to prevent attempts
		    // to request packets from it again.
		    if (AreAnyStreamsLocal())
		    {
			RequestRemoteStreamPacketsFromLocalSource();
		    }
		}
	    }
	}
	else if (eSourceType == NUGTFFSourceType_Remote)
	{
	    if ((m_pStreamStatus[uStreamNumber].eState == NGTFFSTRM_RemoteSelected) ||
		(m_pStreamStatus[uStreamNumber].eState == NGTFFSTRM_RemoteSelectedPendingAlignment))
	    {
		HX_ASSERT(m_pStreamStatus[uStreamNumber].bStreamPending);

		m_pStreamStatus[uStreamNumber].bRemoteDone = TRUE;
		m_pStreamStatus[uStreamNumber].bStreamPending = FALSE;
		m_pStreamStatus[uStreamNumber].bStreamDone = TRUE;

		HXLOGL2(HXLOG_NGTF, "CNGTFileFormat::ReportStreamDone(Strm=%hu) StrmState=%s State=%s RemoteState=%s",
				uStreamNumber,
				StateToString(m_pStreamStatus[uStreamNumber].eState),
				StateToString(m_eState),
				StateToString(m_eRemoteSourceState));

		retVal = m_pFFResponse->StreamDone(uStreamNumber);
	    }
	    else if (m_pStreamStatus[uStreamNumber].eState == NGTFFSTRM_LocalSelected)
	    {
		// Remote stream ended while local stream is still selected.
		// Simply mark remote stream is done.
		m_pStreamStatus[uStreamNumber].bRemoteDone = TRUE;
	    }
	}
    }

    return retVal;
}


/************************************************************************
 *  Protected methods
 */
HX_RESULT CNGTFileFormat::InitChildFileFormat(IHXFileFormatObject *pFileFormat,
					      IHXRequest* pRequest,
					      IHXFormatResponse* pFormatResponse,
					      IHXFileObject *pFileObject)
{
    IHXPlugin* pPlugin = NULL;
    HX_RESULT res = HXR_OK;

    /* init the file format plugin */
    res = pFileFormat->QueryInterface(IID_IHXPlugin, (void**) &pPlugin);

    if (SUCCEEDED(res))
    {
	res = pPlugin->InitPlugin(m_pContext);
    }

    HX_RELEASE(pPlugin);

    if (SUCCEEDED(res))
    {
	res = pFileFormat->InitFileFormat(pRequest, pFormatResponse, pFileObject);
    }

    return res;
}

HX_RESULT CNGTFileFormat::InitRequest(IHXRequest* &pRequest, 
				      const char *pszFileName,
				      IHXValues* pRequestHeader)
{
    HX_RESULT retVal = HXR_OK;

    if (pszFileName == NULL)
    {
	retVal = HXR_INVALID_PARAMETER;
    }

    /* create and init the file system request object */
    if (SUCCEEDED(retVal))
    {
	retVal = CreateInstanceCCF(CLSID_IHXRequest, 
				   (void**) &pRequest, 
				   m_pContext);
    }

    if (SUCCEEDED(retVal) && pRequestHeader)
    {
	retVal = pRequest->SetRequestHeaders(pRequestHeader);
    }

    if (SUCCEEDED(retVal))
    {
	retVal = pRequest->SetURL(pszFileName);
    }

    return retVal;
}

HX_RESULT CNGTFileFormat::CheckFileHeaderCompatibility(void)
{
    HX_RESULT retVal = HXR_NO_DATA;

    if (m_pLocalFileHeader && m_pRemoteFileHeader)
    {
	UINT32 ulVal1 = 0;
	UINT32 ulVal2 = 0;

	retVal = HXR_FAIL;

	m_pLocalFileHeader->GetPropertyULONG32("StreamCount", ulVal1);
	m_pRemoteFileHeader->GetPropertyULONG32("StreamCount", ulVal2);

	if (ulVal1 == ulVal2)
	{
	    retVal = HXR_OK;
	}
    }

    return retVal;
}

HX_RESULT CNGTFileFormat::CheckStreamHeaderCompatibility(void)
{
    UINT16 uStrmIdx;

    for (uStrmIdx = 0; uStrmIdx < m_uNumStreams; uStrmIdx++)
    {
	if (!TestStreamHeaderItemMatchBuffer(uStrmIdx, "MimeType", TRUE))
	{
	    return HXR_FAIL;
	}
    }

    return HXR_OK;
}

HXBOOL CNGTFileFormat::TestStreamHeaderItemMatchBuffer(UINT16 uStrmNumber,
						       const char* pItemName, 
						       HXBOOL bAsString)
{
    HXBOOL bRetVal = FALSE;

    if (m_pStreamStatus[uStrmNumber].pLocalHeader &&
	m_pStreamStatus[uStrmNumber].pRemoteHeader)
    {
	IHXBuffer* pBuffer1 = NULL;
	IHXBuffer* pBuffer2 = NULL;
	
	if (bAsString)
	{
	    m_pStreamStatus[uStrmNumber].pLocalHeader->GetPropertyCString(pItemName, pBuffer1);
	    m_pStreamStatus[uStrmNumber].pRemoteHeader->GetPropertyCString(pItemName, pBuffer2);
	}
	else
	{
	    m_pStreamStatus[uStrmNumber].pLocalHeader->GetPropertyBuffer(pItemName, pBuffer1);
	    m_pStreamStatus[uStrmNumber].pRemoteHeader->GetPropertyBuffer(pItemName, pBuffer2);
	}

	if (pBuffer1 && 
	    pBuffer2)
	{
	    UINT32 ulSize; 

	    if (bAsString)
	    {
		ulSize = HX_MIN(pBuffer1->GetSize(), pBuffer2->GetSize());
		if (!strncmp((const char*) pBuffer1->GetBuffer(), 
			     (const char*) pBuffer2->GetBuffer(), 
			     ulSize))
		{
		    bRetVal = TRUE;
		}
	    }
	    else if (pBuffer1->GetSize() == pBuffer2->GetSize())
	    {
		if (!memcmp(pBuffer1->GetBuffer(), pBuffer2->GetBuffer(), pBuffer1->GetSize()))
		{
		    bRetVal = TRUE;
		}
	    }
	}
	else if ((!pBuffer1) && (!pBuffer2))
	{
	    bRetVal = TRUE;
	}

	HX_RELEASE(pBuffer1);
	HX_RELEASE(pBuffer2);
    }

    return bRetVal;
}

HXBOOL CNGTFileFormat::TestStreamHeaderItemMatchUINT32(UINT16 uStrmNumber,
						       const char* pItemName)
{
    HXBOOL bRetVal = FALSE;

    if (m_pStreamStatus[uStrmNumber].pLocalHeader &&
	m_pStreamStatus[uStrmNumber].pRemoteHeader)
    {
	UINT32 ulVal1 = 0;
	UINT32 ulVal2 = 0;
	HX_RESULT status1;
	HX_RESULT status2;
	
	status1 = m_pStreamStatus[uStrmNumber].pLocalHeader->GetPropertyULONG32(pItemName, ulVal1);
	status2 = m_pStreamStatus[uStrmNumber].pRemoteHeader->GetPropertyULONG32(pItemName, ulVal2);

	if ((status1 == status2) && ((status1 != HXR_OK) || (ulVal1 == ulVal2)))
	{
	    bRetVal = TRUE;
	}
    }

    return bRetVal;
}

HX_RESULT CNGTFileFormat::FetchRemoteStreamHeaders(void)
{
    HX_RESULT retVal = CheckFileHeaderCompatibility();

    if (retVal == HXR_OK)
    {
	UINT16 uStrmIdx;

	for (uStrmIdx = 0; 
	     SUCCEEDED(retVal) && (uStrmIdx < m_uNumStreams); 
	     uStrmIdx++)
	{
	    retVal = m_pRemoteFileFormat->GetStreamHeader(uStrmIdx);
	}

	if (FAILED(retVal))
	{
	    HandleRemoteSourceFailure(NGTRS_Error);
	}
    }
    else
    {
	HandleRemoteSourceFailure(NGTRS_Incomaptible);
    }

    return retVal;
}

HX_RESULT CNGTFileFormat::StartRemotePackets(void)
{
    UINT16 uStrmIdx;
    HX_RESULT retVal = HXR_OK;

    for (uStrmIdx = 0; 
	 SUCCEEDED(retVal) && (uStrmIdx < m_uNumStreams); 
	 uStrmIdx++)
    {
	retVal = m_pRemoteFileFormat->GetPacket(uStrmIdx);
    }

    if (FAILED(retVal))
    {
	HandleRemoteSourceFailure(NGTRS_Error);
    }

    return retVal;
}

HX_RESULT CNGTFileFormat::StartRemoteSource(void)
{
    HX_RESULT retVal = CheckStreamHeaderCompatibility();

    if (retVal == HXR_OK)
    {
	HX_ASSERT(m_pRemoteFileFormat);

	retVal = HXR_OK;
	m_eRemoteSourceState = NGTRS_Connected;

	if (m_ulRemoteFFSeekPending != NGTFF_INVALID_TIMESTAMP)
	{
	    retVal = m_pRemoteFileFormat->Seek(m_ulRemoteFFSeekPending);
	    if (FAILED(retVal))
	    {
		HandleRemoteSourceFailure(NGTRS_Error);
	    }
	}
	else
	{
	    StartRemotePackets();
	}
    }
    else
    {
	HandleRemoteSourceFailure(NGTRS_Incomaptible);
    }

    return retVal;
}

HXBOOL CNGTFileFormat::AreAllStreamHeadersReceived(HXBOOL bRemote)
{
    HXBOOL bRetVal = FALSE;

    if (m_pStreamStatus)
    {
	UINT16 uStrmIdx;

	bRetVal = TRUE;

	for (uStrmIdx = 0; 
	     bRetVal && (uStrmIdx < m_uNumStreams); 
	     uStrmIdx++)
	{
	    bRetVal = bRemote ? (m_pStreamStatus[uStrmIdx].pRemoteHeader != NULL) :
				(m_pStreamStatus[uStrmIdx].pLocalHeader != NULL);
	}
    }

    return bRetVal;
}

HXBOOL CNGTFileFormat::AreAnyStreamsLocal(void)
{
    UINT16 uStrmIdx;

    for (uStrmIdx = 0; 
	 uStrmIdx < m_uNumStreams;
	 uStrmIdx++)
    {
	if ((m_pStreamStatus[uStrmIdx].eState == NGTFFSTRM_LocalSelected) &&
	    (!m_pStreamStatus[uStrmIdx].bLocalDone))
	{
	    return TRUE;
	}
    }

    return FALSE;
}

void CNGTFileFormat::RequestRemoteStreamPacketsFromLocalSource(void)
{
    UINT16 uStrmIdx;

    for (uStrmIdx = 0; uStrmIdx < m_uNumStreams; uStrmIdx++)
    {
	if ((m_pStreamStatus[uStrmIdx].eState != NGTFFSTRM_LocalSelected) &&
	    (!m_pStreamStatus[uStrmIdx].bLocalPending) &&
	    (!m_pStreamStatus[uStrmIdx].bLocalDone))
	{
	    m_pStreamStatus[uStrmIdx].bLocalPending = TRUE;
	    if (FAILED(m_pLocalFileFormat->GetPacket(uStrmIdx)))
	    {
		m_pStreamStatus[uStrmIdx].bLocalPending = FALSE;
	    }
	}
    }
}

void CNGTFileFormat::HandleRemoteSourceFailure(NGTRemoteSourceState eNewRemoteSourceState)
{
    UINT16 uStrmIdx;
    HX_RESULT ulHXCode = HXR_FAILED;
    const char* pUserString = NULL;

    m_eRemoteSourceState = eNewRemoteSourceState;

    HX_ASSERT(!IsRemoteSourceHalthy());

    AddRef();

    switch (eNewRemoteSourceState)
    {
    case NGTRS_Expired:
	ulHXCode = HXR_EXPIRED;
	pUserString = "Content Expired!";
	break;
    case NGTRS_Incomaptible:
	ulHXCode = HXR_INVALID_STREAM;
	pUserString = "Remote content incomaptible with the nugget!";
	break;
    case NGTRS_Unavailable:
	ulHXCode = HXR_PE_SERVICE_UNAVAILABLE;
	pUserString = "Remote content not reachable!";
	break;
    case NGTRS_Error:
	ulHXCode = HXR_FAILED;
	pUserString = "Connection to remote content failed!";
	break;
    default:
	HX_ASSERT(FALSE);
	break;
    }

    m_pErrorMessages->Report(HXLOG_ERR, 
			     ulHXCode,
			     0,
			     pUserString,
			     NULL);

    if (m_pStreamStatus)
    {
	for (uStrmIdx = 0; uStrmIdx < m_uNumStreams; uStrmIdx++)
	{
	    if ((m_pStreamStatus[uStrmIdx].eState != NGTFFSTRM_LocalSelected) &&
		m_pStreamStatus[uStrmIdx].bStreamPending)
	    {
		HX_ASSERT(!m_pStreamStatus[uStrmIdx].bStreamDone);
		m_pStreamStatus[uStrmIdx].bStreamPending = FALSE;
		m_pStreamStatus[uStrmIdx].bStreamDone = TRUE;
		if (m_pFFResponse)
		{
		    HXLOGL2(HXLOG_NGTF, "CNGTFileFormat::ReportStreamDone(Strm=%hu) StrmState=%s State=%s RemoteState=%s",
			uStrmIdx,
			StateToString(m_pStreamStatus[uStrmIdx].eState),
			StateToString(m_eState),
			StateToString(m_eRemoteSourceState));

		    m_pFFResponse->StreamDone(uStrmIdx);
		}
	    }
	}
    }

    Release();
}

const char* CNGTFileFormat::StateToString(NGTFileFormatState eState)
{
    return (((eState >= NGTFF_Offline) && (eState <= NGTFF_RemoteSeeking)) ?
	    m_zpNGTFileFormatStateToStringMap[eState] :
	    "????");
}

const char* CNGTFileFormat::StateToString(NGTRemoteSourceState eState)
{
    return (((eState >= NGTRS_Offline) && (eState <= NGTRS_Error)) ?
	    m_zpNGTRemoteSourceStateToStringMap[eState] :
	    "????");
}

const char* CNGTFileFormat::StateToString(NGTFileFormatStreamState eState)
{
    return (((eState >= NGTFFSTRM_LocalSelected) && (eState <= NGTFFSTRM_RemoteSelected)) ?
	    m_zpNGTFileFormatStreamStateToStringMap[eState] :
	    "????");
}

const char* CNGTFileFormat::StateToString(NGTFFSourceType eState)
{
    return (((eState >= NUGTFFSourceType_Local) && (eState <= NUGTFFSourceType_Remote)) ?
	    m_zpNGTFFSourceTypeToStringMap[eState] :
	    "????");
}


/****************************************************************************
 *  CNGTFormatResponseReceiver
 */
/****************************************************************************
 *  Interface
 */
BEGIN_INTERFACE_LIST_NOCREATE(CNGTFormatResponseReceiver)
INTERFACE_LIST_ENTRY_SIMPLE(IHXFormatResponse)
END_INTERFACE_LIST
