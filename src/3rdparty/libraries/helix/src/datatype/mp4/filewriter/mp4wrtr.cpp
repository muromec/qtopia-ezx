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
/****************************************************************************
 * 
 */

/****************************************************************************
 *  Defines
 */

#define MIN_RESCHED_TIME		2		// in milliseconds


/****************************************************************************
 *  Includes
 */
#include "hxver.h"
#include "mp4wrtr.h"
#include "mp4wrplin.h"
#include "mp4wrtr.ver"

#include "hxstring.h"
#include "hxstrutl.h"
#include "hxbuffer.h"
#include "chxpckts.h"
#include "pckunpck.h"

#include "rmfftype.h"
#include "netbyte.h"
#include "mimechk.h"

#include "archctx.h"
#include "hxfwrtr.h"
#include "hxiids.h"

#include "mp4arch.h"

// XXX For now, include the headers for our plugins
#include "ra10sh.h"
#include "m4ash.h"
#include "m4asm.h"

/****************************************************************************
 * Globals
 */
INT32 g_nRefCount_mp4wr = 0;


/****************************************************************************
 *  CMP4FileWriter class
 */
/****************************************************************************
 * Constants
 */

const char* CMP4FileWriter::zm_pDescription    = "RealNetworks MP4 File Writer Plugin";
const char* CMP4FileWriter::zm_pCopyright      = HXVER_COPYRIGHT;
const char* CMP4FileWriter::zm_pMoreInfoURL    = HXVER_MOREINFO;

///////////////////////////////////////////////

/****************************************************************************
 *  Constructor/Destructor
 */
CMP4FileWriter::CMP4FileWriter(IUnknown* pUnknown)
    : m_lRefCount(0)
    , m_pContext(pUnknown)
    , m_pScheduler(NULL)
    , m_pRequest(NULL)
    , m_pMonitor(NULL)
    , m_pAdviser(NULL)
    , m_pArchiver(NULL)
    , m_pProperties(NULL)
    , m_pStreamHeaderInfo(NULL)
    , m_uStreamCount(0)
    , m_uStreamHeaderCount(0)
    , m_uStreamDoneCount(0)
    , m_ppStreamHandlers(NULL)
    , m_pStreamMixer(NULL)
    , m_State(Offline)
    , m_bClosing(TRUE)
    , m_ulVolumeTag(0)
    , m_CallbackHandle(0)
    , m_ulPktCount(0)
{
    InterlockedIncrement(&g_nRefCount_mp4wr);

    if (m_pContext)
    {
	m_pContext->AddRef();
    }

}

CMP4FileWriter::~CMP4FileWriter(void)
{
    HX_RELEASE(m_pArchiver);
    HX_RELEASE(m_pMonitor);
    HX_RELEASE(m_pAdviser);
    HX_RELEASE(m_pRequest);
    HX_VECTOR_DELETE(m_pStreamHeaderInfo);
    HX_RELEASE(m_pProperties);
    HX_RELEASE(m_pContext);
    HX_RELEASE(m_pScheduler);
    
    InterlockedDecrement(&g_nRefCount_mp4wr);
}


/************************************************************************
 *  IHXPlugin methods
 */
/************************************************************************
 *  CMP4FileWriter::InitPlugin
 *  Purpose:
 *    Initializes the plugin for use. This interface must always be
 *    called before any other method is called. This is primarily needed 
 *    so that the plugin can have access to the context for creation of
 *    IHXBuffers and IMalloc.
 */
STDMETHODIMP CMP4FileWriter::InitPlugin(IUnknown* /*IN*/ pContext)
{
    HX_RESULT retVal = HXR_UNEXPECTED;

    if (m_FileHeaderInfo.m_pFileHeader == NULL)
    {
	retVal = HXR_OK;
    }

    if (SUCCEEDED(retVal))
    {
	retVal = HXR_INVALID_PARAMETER;
	if (pContext)
	{
	    HX_RELEASE(m_pContext);

	    m_pContext = pContext;
	    m_pContext->AddRef();

	    retVal = HXR_OK;
	}
	else if (m_pContext)
	{
	    retVal = HXR_OK;
	}
    }

    return retVal;
}

/************************************************************************
 *  CMP4FileWriter::GetPluginInfo
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
STDMETHODIMP CMP4FileWriter::GetPluginInfo
(
    REF(BOOL)		bLoadMultiple,
    REF(const char*)	pDescription,
    REF(const char*)	pCopyright,
    REF(const char*)	pMoreInfoURL,
    REF(ULONG32)	ulVersionNumber
)
{
    bLoadMultiple = TRUE;   // Must be true for file formats.

    pDescription    = zm_pDescription;
    pCopyright	    = zm_pCopyright;
    pMoreInfoURL    = zm_pMoreInfoURL;
    ulVersionNumber = TARVER_ULONG32_VERSION;

    return HXR_OK;
}


/****************************************************************************
 *  IHXFileWriter methods
 */
/************************************************************************
 *  GetFileFormatInfo
 *  Purpose:
 *    If this object is a file format object this method returns
 *    information vital to the instantiation of file format plugins.
 *    If this object is not a file format object, it should return
 *    PNR_UNEXPECTED.
 */
STDMETHODIMP CMP4FileWriter::GetFileFormatInfo
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


/****************************************************************************
 *  InitFileWriter
 */
STDMETHODIMP CMP4FileWriter::InitFileWriter
(
    IHXRequest* pRequest,
    IHXFileWriterMonitor* pMonitor,
    IHXPropertyAdviser* pAdviser
)
{
    HX_RESULT retVal = HXR_UNEXPECTED;
    IHXValues* pProperties = NULL;

    if (m_FileHeaderInfo.m_pFileHeader == NULL)
    {
	retVal = HXR_OK;
    }

    if (SUCCEEDED(retVal))
    {
	retVal = HXR_FAIL;

	if (m_pContext)
	{
	    HX_RELEASE(m_pScheduler);

	    retVal = m_pContext->QueryInterface(IID_IHXScheduler,
						(void**) &m_pScheduler);
	}
    }

    if (SUCCEEDED(retVal))
    {
	if (pRequest)
	{
	    pRequest->AddRef();
	    pRequest->GetRequestHeaders(pProperties);
	}

	if (pMonitor)
	{
	    pMonitor->AddRef();
	}

	if (pAdviser)
	{
	    pAdviser->AddRef();
	}

	HX_RELEASE(m_pRequest);
	HX_RELEASE(m_pMonitor);
	HX_RELEASE(m_pAdviser);
	HX_RELEASE(m_pProperties);

	m_pRequest = pRequest;
	m_pMonitor = pMonitor;
	m_pAdviser = pAdviser;
	m_pProperties = pProperties;

	m_ulVolumeTag = 0;

	m_bClosing = FALSE;
    }

    return retVal;
}


/****************************************************************************
 *  Close
 */
STDMETHODIMP CMP4FileWriter::Close(void)
{
    UINT16 uIdx;
    HX_RESULT retVal = HXR_OK;

    if (!m_bClosing)
    {
	m_bClosing = TRUE;
	
	// No more information will be sent
	HX_RELEASE(m_pMonitor);
	HX_RELEASE(m_pAdviser);
	
	for (uIdx = 0; uIdx < m_uStreamCount; uIdx++)
	{
	    StreamDone(uIdx);
	}
	
	m_State = Offline;
	
	HX_RELEASE(m_pArchiver);
	
	HX_RELEASE(m_pRequest);
	HX_VECTOR_DELETE(m_pStreamHeaderInfo);
	HX_RELEASE(m_pProperties);
	HX_RELEASE(m_pContext);
	
	for( int i = 0; i < m_uStreamCount; i++ )
	{
	    HX_RELEASE( m_ppStreamHandlers[i] );
	}
	HX_VECTOR_DELETE( m_ppStreamHandlers );
	HX_RELEASE(m_pStreamMixer);

	m_uStreamCount = 0;
	m_uStreamHeaderCount = 0;
	m_uStreamDoneCount = 0;
	
	m_bClosing = FALSE;
    }
    
    return retVal;
}


/****************************************************************************
 *  Abort
 */
STDMETHODIMP CMP4FileWriter::Abort(void)
{
    HX_RESULT retVal = HXR_UNEXPECTED;

    if (m_CallbackHandle && m_pScheduler)
    {
	m_pScheduler->Remove(m_CallbackHandle);
	m_CallbackHandle = 0;
    }

    if (m_pArchiver)
    {
	retVal = m_pArchiver->Abort();

	if (SUCCEEDED(retVal))
	{
	    CBaseArchiver2* pArchiver = m_pArchiver;

	    m_pArchiver = NULL;
	    retVal = OnCompletion(retVal);

	    HX_RELEASE(pArchiver);
	}
    }

    return retVal;
}


/****************************************************************************
 *  SetFileHeader
 */
STDMETHODIMP CMP4FileWriter::SetFileHeader(IHXValues* pHeader)
{
    ULONG32 ulStreamCount = 0;
    HX_RESULT retVal = HXR_INVALID_PARAMETER;

    HX_ASSERT(pHeader);

    if (pHeader)
    {
	retVal = HXR_OK;
    }

    if (SUCCEEDED(retVal))
    {
	if (m_FileHeaderInfo.m_pFileHeader == NULL)
	{
	    retVal = pHeader->GetPropertyULONG32("StreamCount", ulStreamCount);
	    
	    if (SUCCEEDED(retVal))
	    {
		retVal = HXR_INVALID_PARAMETER;
		/* For now, only support single stream */
		if (ulStreamCount == 1)
		{
		    m_uStreamCount = (UINT16) ulStreamCount;

		    // Build out an array for the stream handlers; we do this knowing that we may
		    // not be able to write specific streams, i.e. certain members of the array
		    // may be null pointers.
		    retVal = HXR_OUTOFMEMORY;
		    m_ppStreamHandlers = new IMP4StreamHandler*[m_uStreamCount];
		    if( m_ppStreamHandlers )
		    {
			memset( m_ppStreamHandlers, 0, m_uStreamCount * sizeof(IMP4StreamHandler*) );
			retVal = HXR_OK;
		    }
		}
	    }

	    if (SUCCEEDED(retVal))
	    {
		IHXValues* pNewHeader = CloneHeader(pHeader, m_pContext);
		retVal = HXR_OUTOFMEMORY;
		if (pNewHeader)
		{
		    retVal = HXR_OK;
		}

		if (SUCCEEDED(retVal))
		{
		    pNewHeader->AddRef();
		    m_FileHeaderInfo.m_pFileHeader = pNewHeader;
		}

		HX_RELEASE(pNewHeader);
	    }
	    
	    HX_VECTOR_DELETE(m_pStreamHeaderInfo);
	    m_uStreamHeaderCount = 0;
	}
	else
	{
	    retVal = pHeader->GetPropertyULONG32("StreamCount", ulStreamCount);
	    
	    if (SUCCEEDED(retVal))
	    {
		retVal = HXR_INVALID_PARAMETER;
		if ((ulStreamCount != 0) && 
		    ((ulStreamCount == m_uStreamCount) ||
		     (m_pStreamHeaderInfo == NULL)))
		{
		    retVal = HXR_OK;
		}
	    }

	    // Make Sure it is not too late to modify File Header
	    if (SUCCEEDED(retVal))
	    {
		if (m_uStreamDoneCount >= m_uStreamCount)
		{
		    retVal = HXR_UNEXPECTED;
		}
	    }
	    
	    if (SUCCEEDED(retVal))
	    {
		IHXValues* pNewHeader = CloneHeader(pHeader, m_pContext);
		retVal = HXR_OUTOFMEMORY;
		if (pNewHeader)
		{
		    retVal = HXR_OK;
		}

		if (SUCCEEDED(retVal))
		{
		    m_FileHeaderInfo.m_pFileHeader->Release();
		    pNewHeader->AddRef();
		    m_FileHeaderInfo.m_pFileHeader = pNewHeader;
		    m_FileHeaderInfo.m_bFileHeaderModified = 
			(m_uStreamHeaderCount == m_uStreamCount);	

		    if (m_FileHeaderInfo.m_bFileHeaderModified &&
			m_pArchiver)
		    {
			retVal = m_pArchiver->FileHeaderReady(pHeader);
		    }
		}

		HX_RELEASE(pNewHeader);
	    }
	}
    }

    return retVal;
}
    

/****************************************************************************
 *  SetStreamHeader
 */
STDMETHODIMP CMP4FileWriter::SetStreamHeader(IHXValues* pHeader)
{
    UINT16 unStreamNumber;
    BOOL bStreamAdded = FALSE;
    HX_RESULT retVal = HXR_UNEXPECTED;

    // Check input parameters
    if ((m_FileHeaderInfo.m_pFileHeader != NULL) &&
	(m_uStreamCount != 0))
    {
	retVal = HXR_OK;
    }

    if (SUCCEEDED(retVal))
    {
	ULONG32 ulStreamNumber = 0;

	retVal = pHeader->GetPropertyULONG32("StreamNumber", ulStreamNumber);

	unStreamNumber = (UINT16) ulStreamNumber;
    }

    if (SUCCEEDED(retVal))
    {
	HX_ASSERT((pHeader != NULL) && (unStreamNumber < m_uStreamCount));

	retVal = HXR_INVALID_PARAMETER;
	if ((pHeader != NULL) &&
	    (unStreamNumber < m_uStreamCount))
	{
	    retVal = HXR_OK;
	}
    }

    // Make sure it is not too late to modify stream headers
    if (SUCCEEDED(retVal))
    {
	if (m_uStreamDoneCount >= m_uStreamCount)
	{
	    retVal = HXR_UNEXPECTED;
	}
    }

    // Create Stream Header Holders if needed
    if (SUCCEEDED(retVal))
    {
	if (m_pStreamHeaderInfo == NULL)
	{
	    retVal = HXR_OUTOFMEMORY;
	    m_pStreamHeaderInfo = new StreamHeaderInfo [m_uStreamCount];
	    if (m_pStreamHeaderInfo)
	    {
		retVal = HXR_OK;
	    }
	}
    }

    m_uStreamHeaderCount++;
    
    // Perform addtion or modification
    if (SUCCEEDED(retVal))
    {
	IHXValues* pNewHeader = CloneHeader(pHeader, m_pContext);
	retVal = HXR_OUTOFMEMORY;
	if (pNewHeader)
	{
	    retVal = HXR_OK;
	}
	
	if (SUCCEEDED(retVal))
	{
	    pNewHeader->AddRef();
	    HX_RELEASE(m_pStreamHeaderInfo[unStreamNumber].m_pStreamHeader);
	    m_pStreamHeaderInfo[unStreamNumber].m_pStreamHeader = pNewHeader;
	}
	
	HX_RELEASE(pNewHeader);	
    }

    // XXX this should be cleaned up. the archiver doesn't care about the
    // file or stream headers, and the streammixer assumes that it gets these
    // from the streamhandlers, which may modify the originals

    // When the final stream header arrives, create a stream mixer and archiver
    // load it with headers
    if (SUCCEEDED(retVal) && 
	(m_uStreamHeaderCount == m_uStreamCount))
    {
	if (SUCCEEDED(retVal))
	{
	    retVal = CreateAndInitArchiver();
	}
	
	if( SUCCEEDED(retVal) )
	{
	    // XXX for now, build a default stream mixer
	    retVal = HXR_OUTOFMEMORY;
	    m_pStreamMixer = new CM4AStreamMixer( m_pContext, (CMP4Archiver*)m_pArchiver );
	    if( m_pStreamMixer )
	    {
		m_pStreamMixer->AddRef();
		m_pStreamMixer->SetFileHeader( m_FileHeaderInfo.m_pFileHeader );
		
		retVal = HXR_OK;
	    }
	}
	
	// Pass the file header to the archiver object
	if (SUCCEEDED(retVal))
	{
	    HX_ASSERT(m_pArchiver);
	    
	    retVal = m_pArchiver->FileHeaderReady(
		m_FileHeaderInfo.m_pFileHeader);
	}
	
	// Pass the Stream Headers to the archiver object, build out our
	// stream handlers
	if (SUCCEEDED(retVal))
	{
	    ULONG32 ulIdx;
	    
	    for (ulIdx = 0; 
		 SUCCEEDED(retVal) && (ulIdx < m_uStreamCount); 
		 ulIdx++)
	    {
		// XXX find the appropriate streamhandler object for this stream's mime type
		// longer term we will have to figure out a way to query various plugins to
		// see what mime types are supported. for now just try the two we know about
		if( SUCCEEDED(retVal) )
		{
		    IHXBuffer* pStreamMimeTypeBuffer = NULL;
		    const char* pStreamMimeType = NULL;
		    retVal = m_pStreamHeaderInfo[unStreamNumber].m_pStreamHeader->GetPropertyCString("MimeType", pStreamMimeTypeBuffer);
		    if( SUCCEEDED( retVal ) )
		    {
			pStreamMimeType = (const char*) pStreamMimeTypeBuffer->GetBuffer();
			
			retVal = HXR_OUTOFMEMORY;
			IMP4StreamHandler* pTempHandler = new CRA10StreamHandler( m_pContext, m_pStreamMixer ) ;
			
			if( pTempHandler )
			{
			    pTempHandler->AddRef();
			    retVal = TestForMimeSupport( pTempHandler, pStreamMimeType );
			}
			if( FAILED( retVal ) )
			{
			    HX_RELEASE( pTempHandler );
			    retVal = HXR_OUTOFMEMORY;
			    pTempHandler = new CM4AStreamHandler( m_pContext, m_pStreamMixer );
			    
			    if( pTempHandler )
			    {
				pTempHandler->AddRef();
				retVal = TestForMimeSupport( pTempHandler, pStreamMimeType );
			    }
			}
			
			if( SUCCEEDED( retVal ) )
			{
			    // At this point things should be all lined up; init and set the header
			    m_ppStreamHandlers[ulIdx] = pTempHandler;
			    
			    retVal = m_ppStreamHandlers[ulIdx]->InitStreamHandler();
			    if( SUCCEEDED(retVal) )
			    {
				retVal = m_ppStreamHandlers[ulIdx]->SetFileHeader( m_FileHeaderInfo.m_pFileHeader );
				if( SUCCEEDED( retVal ) )
				{
				    retVal = m_ppStreamHandlers[ulIdx]->SetStreamHeader( m_pStreamHeaderInfo[unStreamNumber].m_pStreamHeader );
				}
			    }
			}
			else
			{
			    HX_RELEASE( pTempHandler );
			    retVal = HXR_UNEXPECTED;
			}
			HX_RELEASE( pStreamMimeTypeBuffer );
		    }
		}
	    }
	}
    }
    return retVal;
}


/****************************************************************************
 *  SetProperties
 */
STDMETHODIMP CMP4FileWriter::SetProperties(IHXValues* pProperties)
{
    IHXValues* pNewProperties = NULL;
    HX_RESULT retVal = HXR_OK;

    pNewProperties = CloneHeader(pProperties, m_pContext);
    HX_RELEASE(m_pProperties);
    m_pProperties = pNewProperties;

    return retVal;
}

/****************************************************************************
 *  SetPacket
 */
STDMETHODIMP CMP4FileWriter::SetPacket(IHXPacket* pPacket)
{
    HX_RESULT retVal = HXR_UNEXPECTED;

    if (m_State == PacketsReady)
    {
	UINT16 uStreamNum;
	ULONG32 ulIdx = 0;

	m_ulPktCount++;

	HX_ASSERT(pPacket);

	retVal = HXR_FAIL;

	uStreamNum = pPacket->GetStreamNumber();

	HX_ASSERT(uStreamNum < m_uStreamCount);

	if (uStreamNum < m_uStreamCount)
	{
	    if( m_ppStreamHandlers[ uStreamNum ] )
	    {
		retVal = m_ppStreamHandlers[ uStreamNum ]->SetPacket( pPacket );
	    }
	}
    }
	
    return retVal;
}

/****************************************************************************
 *  StreamDone
 */
STDMETHODIMP CMP4FileWriter::StreamDone(UINT16 unStreamNumber)
{
    HX_RESULT retVal = HXR_UNEXPECTED;

    if ((m_State != Offline) &&
	(m_uStreamDoneCount < m_uStreamCount) &&
	(m_pStreamHeaderInfo) &&
	(!m_pStreamHeaderInfo[unStreamNumber].m_bStreamDone))
    {
	m_pStreamHeaderInfo[unStreamNumber].m_bStreamDone = TRUE;
	m_uStreamDoneCount++;

	if( m_ppStreamHandlers[ unStreamNumber ] )
	{
	    m_ppStreamHandlers[ unStreamNumber ]->StreamDone();
	}
	
	if (m_uStreamDoneCount >= m_uStreamCount)
	{
	    retVal = Func();
	}
    }

    return retVal;
}


/****************************************************************************
 *  IHXFileWriterMonitor
 */
/****************************************************************************
 *  OnStatus
 */
STDMETHODIMP CMP4FileWriter::OnStatus(HX_RESULT status, IHXValues* pInfoList)
{
    HX_RESULT retVal = HXR_OK;

    if (m_pMonitor)
    {
	retVal = m_pMonitor->OnStatus(status, pInfoList);
    }

    return retVal;
}

/****************************************************************************
 *  OnVolumeInitiation
 */
STDMETHODIMP CMP4FileWriter::OnVolumeInitiation(HX_RESULT status,
					       const char* pName,
					       REF(ULONG32) ulTag)
{
    HX_RESULT retVal = HXR_OK;

    m_ulVolumeTag++;

    if (m_pMonitor)
    {
	ulTag = m_ulVolumeTag;

	AddRef();

	retVal = m_pMonitor->OnVolumeInitiation(status, pName, ulTag);

	if (retVal == HXR_OK)
	{
	    m_ulVolumeTag = ulTag;
	}

	Release();
    }
    else
    {
	ulTag = m_ulVolumeTag;
    }

    return retVal;
}

/****************************************************************************
 *  OnPacketsReady
 */
STDMETHODIMP CMP4FileWriter::OnPacketsReady(HX_RESULT status)
{
    HX_RESULT retVal = HXR_OK;

    m_State = PacketsReady;
    
    if (m_pMonitor)
    {
	retVal = m_pMonitor->OnPacketsReady(status);
    }

    return retVal;
}


/****************************************************************************
 *  OnVolumeCompletion
 */
STDMETHODIMP CMP4FileWriter::OnVolumeCompletion(HX_RESULT status,
					       ULONG32 ulTag)
{
    HX_RESULT retVal = HXR_OK;

    if (m_pMonitor)
    {
	retVal = m_pMonitor->OnVolumeCompletion(status, ulTag);
    }

    return retVal;
}


/****************************************************************************
 *  OnCompletion
 */
STDMETHODIMP CMP4FileWriter::OnCompletion(HX_RESULT status)
{
    HX_RESULT retVal = HXR_OK;

    m_State = Offline;

    if (m_pMonitor)
    {
	retVal = m_pMonitor->OnCompletion(status);
    }

    return retVal;
}


/****************************************************************************
 *  IHXPropertyAdviser Merhods
 */
/****************************************************************************
 *  GetPropertyULONG32
 */
STDMETHODIMP CMP4FileWriter::GetPropertyULONG32(const char*      pPropertyName,
					       REF(ULONG32)     ulPropertyValue)
{
    HX_RESULT retVal = HXR_FAIL;

    if (m_pAdviser)
    {
	retVal = m_pAdviser->GetPropertyULONG32(pPropertyName, 
	    ulPropertyValue);
    }	
    
    if (FAILED(retVal))
    {
	if (m_pProperties)
	{
	    retVal = m_pProperties->GetPropertyULONG32(pPropertyName, 
		ulPropertyValue);
	}
    }
    
    return retVal;
}
    
/****************************************************************************
 *  GetPropertyBuffer
 */
STDMETHODIMP CMP4FileWriter::GetPropertyBuffer(const char*      pPropertyName,
					      REF(IHXBuffer*) pPropertyValue)
{
    HX_RESULT retVal = HXR_FAIL;

    if (m_pAdviser)
    {
	retVal = m_pAdviser->GetPropertyBuffer(pPropertyName, 
					       pPropertyValue);
    }

    if (FAILED(retVal))
    {
	if (m_pProperties)
	{
	    retVal = m_pProperties->GetPropertyBuffer(pPropertyName, 
						      pPropertyValue);
	}
    }

    return retVal;
}
    
/****************************************************************************
 *  GetPropertyCString
 */
STDMETHODIMP CMP4FileWriter::GetPropertyCString(const char*      pPropertyName,
					       REF(IHXBuffer*) pPropertyValue)
{
    HX_RESULT retVal = HXR_FAIL;

    if (m_pAdviser)
    {
	retVal = m_pAdviser->GetPropertyCString(pPropertyName, 
					        pPropertyValue);
    }

    if (FAILED(retVal))
    {
	if (m_pProperties)
	{
	    retVal = m_pProperties->GetPropertyCString(pPropertyName, 
						       pPropertyValue);
	}
    }
    
    return retVal;
}
    
/****************************************************************************
 *  GetPropertySet
 */
STDMETHODIMP CMP4FileWriter::GetPropertySet(const char*	     pPropertySetName,
					   REF(IHXValues*)  pPropertySet)
{
    IHXValues* pSet = NULL;
    HX_RESULT retVal = HXR_FAIL;

    if (m_pAdviser)
    {
	retVal = m_pAdviser->GetPropertySet(pPropertySetName, 
					    pPropertySet);
    }

    if (FAILED(retVal))
    {
	if (strcmp(pPropertySetName, SUPLEMENTED_STRM_HEADER_SET) == 0)
	{
	    pSet = pPropertySet;
	    retVal = SuplementStreamHeader(pSet);
	}
	else if (strcmp(pPropertySetName, STRM_HEADER_SET) == 0)
	{
	    ULONG32 ulStreamNumber;

	    retVal = HXR_INVALID_PARAMETER;
	    if (pPropertySet)
	    {
		retVal = pPropertySet->GetPropertyULONG32("StreamNumber",
							  ulStreamNumber);
	    }

	    if (SUCCEEDED(retVal))
	    {
		retVal = HXR_INVALID_PARAMETER;
		if ((ulStreamNumber < m_uStreamCount) &&
		    (m_pStreamHeaderInfo != NULL))
		{
		    retVal = HXR_OK;
		    pSet = m_pStreamHeaderInfo[ulStreamNumber].m_pStreamHeader;
		}
	    }
	}
	else if (strcmp(pPropertySetName, FILE_HEADER_SET) == 0)
	{
	    retVal = HXR_OK;
	    pSet = m_FileHeaderInfo.m_pFileHeader;
	}
	else if (strcmp(pPropertySetName, FWRT_PROPERTY_SET) == 0)
	{
	    retVal = HXR_OK;
	    pSet = m_pProperties;
	}

	if (SUCCEEDED(retVal))
	{
	    if (pSet)
	    {
		pSet->AddRef();
		if (pPropertySet)
		{
		    pPropertySet->Release();
		}
		pPropertySet = pSet;
	    }
	}
    }

    return retVal;
}


/****************************************************************************
 *  Private Merhods
 */

HX_RESULT CMP4FileWriter::TestForMimeSupport( IMP4StreamHandler* pStreamHandler, const char* pszMimeType )
{
    HX_RESULT retVal = HXR_FAIL;

    const char** ppSupportedMimeTypes = NULL;
    retVal = pStreamHandler->GetFormatInfo( &ppSupportedMimeTypes, NULL, NULL );
    
    if( SUCCEEDED( retVal ) )
    {
	retVal = HXR_FAIL;
	// Step through the list of supported mime types looking for a match
	while( *ppSupportedMimeTypes && **ppSupportedMimeTypes )
	{
	    if( stricmp( *ppSupportedMimeTypes, pszMimeType ) == 0 )
	    {
		// Supported
		retVal = HXR_OK;
		break;
	    }
	    ppSupportedMimeTypes++;
	}
    }
    
    return retVal;
}

/****************************************************************************
 *  CreateArchiver
 */
HX_RESULT CMP4FileWriter::CreateAndInitArchiver(void)
{
    HX_RESULT retVal = HXR_OK;
    BOOL bUseMultiStreamArchiver = FALSE;
    BOOL bFullyNative = TRUE;
    BOOL bIsEncrypted = FALSE;
    BOOL bVBR = FALSE;
    IUnknown* pArchiverContext = NULL;
    
    HX_ASSERT(m_uStreamCount == m_uStreamHeaderCount);
    HX_ASSERT(m_pStreamHeaderInfo);

    retVal = HXR_FAIL;
    if (m_uStreamCount > 0)
    {
	retVal = HXR_OK;
    }

    // Create the context to initialize the archiver with
    if (SUCCEEDED(retVal))
    {
	CArchiverContext* pArchCtxObj = NULL;

	retVal = HXR_OUTOFMEMORY;
	pArchCtxObj = new CArchiverContext(m_pContext,
					   &g_nRefCount_mp4wr);

	if (pArchCtxObj)
	{
	    pArchCtxObj->AddRef();

	    retVal = pArchCtxObj->QueryInterface(IID_IUnknown, 
						 (void**) &pArchiverContext);

	    pArchCtxObj->Release();
	}
    }

    // Create appropriate archiver
    if (SUCCEEDED(retVal))
    {
	m_pArchiver = new CMP4Archiver(pArchiverContext, 
					  (IHXFileWriterMonitor*) this, 
					  (IHXPropertyAdviser*) this); 
	
	retVal = HXR_OUTOFMEMORY;
	if (m_pArchiver)
	{
	    retVal = HXR_OK;
	    m_pArchiver->AddRef();
	}
    }
    
    // Initialize archiver
    if (SUCCEEDED(retVal))
    {
	retVal = m_pArchiver->Init(m_pRequest);
    }

    HX_RELEASE(pArchiverContext);

    return retVal;
}


/****************************************************************************
 *  SuplementStreamHeader
 */
 HX_RESULT CMP4FileWriter::SuplementStreamHeader(IHXValues* pHeader)
{
    const char* pMimeTypeChars;
    IHXBuffer* pMimeType = NULL;
    HX_RESULT retVal = HXR_INVALID_PARAMETER;
    UINT8* pSuplement = NULL;
    ULONG32 ulSuplementSize = 0;
    const char* pMimeTypeExtension = NULL;

    if (pHeader)
    {
	retVal = HXR_OK;
    }

    if (SUCCEEDED(retVal))
    {
	retVal = pHeader->GetPropertyCString("MimeType", pMimeType);
    }

    if (SUCCEEDED(retVal))
    {
	retVal = HXR_FAIL;

	pMimeTypeChars = (const char*) pMimeType->GetBuffer();
    }

    // Add Mime Type Extension
    if (SUCCEEDED(retVal) &&
	pMimeTypeExtension)
    {
	BOOL bEncryptedMimeType;
	CHXString strNewMimeType = pMimeTypeChars;
	IHXBuffer* pNewMimeType = NULL;

	bEncryptedMimeType = decryptMimeType(strNewMimeType);

	strNewMimeType += pMimeTypeExtension;

	if (bEncryptedMimeType)
	{
	    encryptMimeType(strNewMimeType);
	}
	
	retVal = CreateBufferCCF(pNewMimeType, m_pContext);
	if (SUCCEEDED(retVal))
	{
	    retVal = pNewMimeType->SetSize(strNewMimeType.GetLength() + 1);
	}
	
	if (SUCCEEDED(retVal))
	{
	    const char* pNewMimeTypeChars = (const char*) strNewMimeType;

	    strcpy((char*) pNewMimeType->GetBuffer(), pNewMimeTypeChars); /* Flawfinder: ignore */
	}

	if (SUCCEEDED(retVal))
	{
	    pHeader->SetPropertyCString("MimeType", pNewMimeType);
	}

	HX_RELEASE(pNewMimeType);
    }

    // Add the Suplement (OpaqueData)
    if (SUCCEEDED(retVal) &&
	pSuplement &&
	(ulSuplementSize > 0))
    {
	IHXBuffer* pOpaqueData = NULL;
	retVal = CreateBufferCCF(pOpaqueData, m_pContext);
	if (SUCCEEDED(retVal))
	{
	    retVal = pOpaqueData->Set(pSuplement, ulSuplementSize);
	}

	if (SUCCEEDED(retVal))
	{
	    pHeader->SetPropertyBuffer("OpaqueData", pOpaqueData);
	}

	HX_RELEASE(pOpaqueData);
    }

    HX_RELEASE(pMimeType);
    HX_VECTOR_DELETE(pSuplement);

    return retVal;
}

/************************************************************************
 *  IHXCallback methods
 */
STDMETHODIMP CMP4FileWriter::Func()
{
    HX_RESULT retVal = HXR_OK;

    m_CallbackHandle = 0;

    if (m_pArchiver)
    {
	retVal = m_pArchiver->Done();
    }
    retVal = HXR_OK;

    return retVal;
}


/************************************************************************
 *  IUnknown methods
 */
STDMETHODIMP CMP4FileWriter::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
	AddRef();
	*ppvObj = (IUnknown*) (IHXPlugin*) this;
	return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXPlugin))
    {
	AddRef();
	*ppvObj = (IHXPlugin*) this;
	return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXFileWriter))
    {
	AddRef();
	*ppvObj = (IHXFileWriter*) this;
	return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXPropertyAdviser))
    {
	AddRef();
	*ppvObj = (IHXPropertyAdviser*) this;
	return HXR_OK;
    }
    
    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32) CMP4FileWriter::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32) CMP4FileWriter::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}
