/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxflsrc.cpp,v 1.124 2007/04/05 21:56:15 sfu Exp $
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
			      
#include "hxcom.h"
#include "hlxclib/stdio.h"
#include "hxassert.h"
#include "hxslist.h"
#include "hxcomm.h"
#include "hxengin.h"
#include "ihxpckts.h"
#include "pckunpck.h"
#include "hxplugn.h"
#include "hxfiles.h"
#include "hxformt.h"
#include "hxmeta.h"
#include "hxcore.h"
#include "hxprefs.h"
#include "hxausvc.h"
#include "hxgroup.h"
#include "hxsmbw.h"
#include "hxstring.h"
#include "chxeven.h"
#include "chxelst.h"
#include "strminfo.h"
#include "hxflsrc.h"
#include "hxtick.h"
#include "hxplay.h"
#include "hxtypes.h"
#include "timeval.h"
#include "pq.h"
#include "hxsched.h"
#include "dbcs.h"
#include "hxcleng.h"
#include "hxauth.h"
#include "hxrquest.h"
#include "hxstrutl.h"
#include "hxupgrd.h"
#include "hxtlogutil.h"
#include "velproxy.h"
#include "srcinfo.h"
#include "hxescapeutil.h"
#include "rmfftype.h"
#include "hxxfile.h"
#include "upgrdcol.h"
#include "uri_schemes.h"
#include "stream_desc_hlpr.h"
#include "findfile.h"
#include "hxsrcbufstats.h"
#include "hxprefs.h"
#include "hxpref.h"

#if defined(HELIX_FEATURE_DRM)
#include "hxdrmcore.h"
#endif

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif

#define FILEREAD_SIZE   4096
#define LOCALSOURCE_FAST_START_BW_CAPACITY_THRESHOLD	150	// A percentage of source bitrate
#define MAX_INTERSTREAM_TIMESTAMP_JITTER		5000	// In milliseconds

HXFileSource::HXFileSource() 
    : m_lRefCount(0)
    , m_ulLastBufferingReturned(0)
    , m_ulFillEndTime(0)
    , m_ulMaxPreRoll(0)
    , m_uNumStreamsToBeFilled(0)
    , m_bInFillMode(FALSE)
    , m_bInitialPacket(TRUE)
    , m_bFastStartInProgress(FALSE)
    , m_bAddDefaultUpgrade(FALSE)
    , m_bCurrentFileFormatUnkInUse(FALSE)
    , m_bValidateMetaDone(FALSE)
    , m_bPacketlessSource(FALSE)
    , m_pDefaultUpgradeString(NULL)
    , m_pFSObject(NULL)
    , m_pFFObject(NULL)
    , m_pRAMFFObject(NULL)
    , m_pFileResponse(NULL)
    , m_pFileFormatEnumerator(NULL)
    , m_pFFClaimURLEnumerator(NULL)
    , m_pCurrentFileFormatUnk(NULL)
#if defined(HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS)
    , m_pPDSObserverList(NULL)
#endif // /HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS.
    , m_pFileObject(NULL)
    , m_pRequestHandler(NULL)
    , m_pMimeType(NULL)
    , m_pExtension(NULL)
    , m_pMimeFinderResponse(NULL)
#if defined(HELIX_FEATURE_ASM)
    , m_pSimulatedSourceBandwidth(NULL)
#endif /* HELIX_FEATURE_ASM */
    , m_pFileReader(NULL)
#if defined(HELIX_FEATURE_FILE_RECOGNIZER)
    , m_pFileRecognizer(NULL)
#endif /* #if defined(HELIX_FEATURE_FILE_RECOGNIZER) */
    , m_pHXSrcBufStats(NULL)
{
    m_bAltURL = FALSE;
    // m_bPerfectPlay = TRUE;
}


HXFileSource::~HXFileSource()
{
    DoCleanup();
}


/////////////////////////////////////////////////////////////////////////
//	Method:
//		IUnknown::QueryInterface
//	Purpose:
//		Implement this to export the interfaces supported by your 
//		object.
//
STDMETHODIMP 
HXFileSource::QueryInterface(REFIID riid, void** ppvObj)
{
    if (HXSource::QueryInterface(riid, ppvObj) == HXR_OK)
    {
	return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXFormatResponse))
    {
	AddRef();
	*ppvObj = (IHXFormatResponse*)this;
	return HXR_OK;
    } //XXXEH- IHXHTTPRedirectResponse is not here; is it needed???
#if defined(HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS)
    else if (IsEqualIID(riid, IID_IHXPDStatusMgr))
    {
	AddRef();
	*ppvObj = (IHXPDStatusMgr*)this;
	return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXPDStatusObserver))
    {
	AddRef();
	*ppvObj = (IHXPDStatusObserver*)this;
	return HXR_OK;
    }
#endif // /HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS.
    else if (IsEqualIID(riid, IID_IHXFileFormatFinder))
    {
	AddRef();
	*ppvObj = (IHXFileFormatFinder*)this;
	return HXR_OK;
    }
    // XXX HP
    // we assume the FF object doesn't QI from its m_pContext within
    // its ::QueryInterface(), otherwise we are in trouble!! - indefinite
    // loop
    // This is for IHXBackChannel, IHXASMSource
    else if (m_pFFObject && m_pFFObject->QueryInterface(riid, ppvObj) == HXR_OK)
    {
	return HXR_OK;
    }
#if defined(HELIX_FEATURE_ASM)
    else if (m_pSimulatedSourceBandwidth &&
	     m_pSimulatedSourceBandwidth->QueryInterface(riid, ppvObj) == HXR_OK)
    {
	return HXR_OK;
    }
#endif /* HELIX_FEATURE_ASM */
    else if (m_pPlayer &&
	     m_pPlayer->QueryInterface(riid, ppvObj) == HXR_OK)
    {
	return HXR_OK;
    }
    // we don't have m_pPlayer during AutoConfig    
    else if (m_pEngine &&
	     m_pEngine->QueryInterface(riid, ppvObj) == HXR_OK)
    {
	return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

/////////////////////////////////////////////////////////////////////////
//	Method:
//		IUnknown::AddRef
//	Purpose:
//		Everyone usually implements this the same... feel free to use
//		this implementation.
//
STDMETHODIMP_(ULONG32) 
HXFileSource::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

/////////////////////////////////////////////////////////////////////////
//	Method:
//		IUnknown::Release
//	Purpose:
//		Everyone usually implements this the same... feel free to use
//		this implementation.
//
STDMETHODIMP_(ULONG32) 
HXFileSource::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
	return m_lRefCount;
    }

    delete this;
    return 0;
}

void
HXFileSource::ReSetup()
{
    m_ulLastBufferingReturned = 0;;
    m_ulFirstPacketTime = 0;
    m_ulFillEndTime = 0;
    m_ulMaxPreRoll = 0;
    m_uNumStreamsToBeFilled = 0;    
    m_bInitialized = FALSE;
    m_bInFillMode = FALSE;
    m_bInitialPacket = TRUE;
    m_bFastStartInProgress = FALSE;
    m_ulStreamHeadersExpected = 0;
    m_pushDownStatus = PUSHDOWN_NONE;

#if defined(HELIX_FEATURE_DRM)
    m_bIsProtected = FALSE;
#endif

    if (m_pURL)
    {	
        CHXURL* pURL = new CHXURL(*m_pURL);

#if defined(HELIX_FEATURE_SMIL_REPEAT)
	if (m_pSourceInfo)
	{
	    CHXSimpleList* pRepeatList = m_pSourceInfo->m_bLeadingSource?m_pSourceInfo->m_pRepeatList:
									 m_pSourceInfo->m_pPeerSourceInfo->m_pRepeatList;

	    if (pRepeatList)
	    {
		RepeatInfo* pRepeatInfo = (RepeatInfo*)pRepeatList->GetAt(m_pSourceInfo->m_curPosition);
		m_ulDelay = m_pSourceInfo->m_ulRepeatDelayTimeOffset + pRepeatInfo->ulDelay;

		if (m_pSourceInfo->m_bRepeatIndefinite	&&
		    m_pSourceInfo->m_ulMaxDuration	&&
		    m_ulDelay + pRepeatInfo->ulDuration > m_ulOriginalDelay + m_pSourceInfo->m_ulMaxDuration)
		{
		    m_ulRestrictedDuration = m_ulOriginalDelay + m_pSourceInfo->m_ulMaxDuration - m_ulDelay;
		}
		else
		{
		    m_ulRestrictedDuration = pRepeatInfo->ulDuration;
		}
	    }
	}
#endif /* HELIX_FEATURE_SMIL_REPEAT */

#if defined(HELIX_FEATURE_STATS) && defined(HELIX_FEATURE_REGISTRY)
	m_pStats->Reset();
#endif /* HELIX_FEATURE_STATS && HELIX_FEATURE_REGISTRY */

	m_bReSetup = TRUE;
	Setup(pURL, FALSE);
        delete pURL;
    }

    return;
}

// pURL may be in the form of URL
// events will be opened as a separate source by HXPlayer...
HX_RESULT 
HXFileSource::Setup(const CHXURL* pURL, HXBOOL bAltURL)
{
    HX_RESULT	    theErr = HXR_OK;    
    IUnknown*	    pUnknown = NULL;
    IUnknown*	    pObject = NULL;
    IHXPlugin*	    pHXPlugin = NULL;

    HXLOGL3(HXLOG_CORP, "HXPlayer[%p]::::HXFileSource[%p]::Setup(URL=%s, bAltURL=%c) Start",
	    m_pPlayer,
	    this,
	    pURL ? pURL->GetURL() : "NULL",
	    bAltURL ? 'T' : 'F');

    mLastError = HXR_OK;
 
    if (!pURL)
    {
	return( HXR_INVALID_PATH );
    }

    const char* purl = pURL->GetURL();
    if (!purl || !*purl)
    {
	return( HXR_INVALID_PATH );
    }

    if (!m_bReSetup)
    {
	theErr = SetupRegistry();	
	m_ulOriginalDelay = m_ulDelay;
    }

    ReadPreferences();
    
    HX_VECTOR_DELETE(m_pszURL);
    HX_DELETE(m_pURL);
    
    if (m_pHXSrcBufStats)
    {
        m_pHXSrcBufStats->Close();
        HX_RELEASE(m_pHXSrcBufStats);
    }

    m_pHXSrcBufStats = new HXSourceBufferStats();
    if (m_pHXSrcBufStats)
    {
        m_pHXSrcBufStats->AddRef();

        if (HXR_OK != m_pHXSrcBufStats->Init((IUnknown*)(IHXStreamSource*)this))
        {
            m_pHXSrcBufStats->Close();
            HX_RELEASE(m_pHXSrcBufStats);
        }
    }

    if (!theErr && pURL)
    {
	m_pszURL = new char[strlen(purl) + 1];
	if (m_pszURL)
	{
	    strcpy(m_pszURL, purl); /* Flawfinder: ignore */
	    m_pURL = new CHXURL(*pURL);
	    if( !m_pURL )
	    {
	        theErr = HXR_OUTOFMEMORY;
	    }
	}
	else
	{
	    theErr = HXR_OUTOFMEMORY;
	}
    }

#if defined(HELIX_FEATURE_STATS) && defined(HELIX_FEATURE_REGISTRY)
    if (!theErr && m_pStats)
    {
	// save URL to the registry
	if (m_pStats->m_pSourceName && m_pszURL)
	{
	    m_pStats->m_pSourceName->SetStr((char*)m_pszURL);
	}

	// update transport mode(local machine)
	if (m_pStats->m_pTransportMode && m_pURL)
	{
	    UINT16	uProtocol = m_pURL->GetProtocol();

	    if (uProtocol == httpProtocol)
	    {
		m_pStats->m_pTransportMode->SetStr("HTTP");
	    }
	    else
	    {
		m_pStats->m_pTransportMode->SetStr("Local");
	    }
	}
    }
#endif /* HELIX_FEATURE_STATS && HELIX_FEATURE_REGISTRY*/

#if defined(HELIX_FEATURE_ASM) && defined(HELIX_FEATURE_REGISTRY)
    if (!theErr && m_pPlayer && !m_pSimulatedSourceBandwidth && m_pRegistry)
    {
	/* Check if the core is used for simulated network playback for
	 * local files. This feature is used by the Tools group
	 * for Preview mode at different bandwidths.
	 */
	INT32 lUseNetBandwidth = 0;
	if ((m_pRegistry->GetIntByName("UseNetBandwidthForLocalPlayback", 
					lUseNetBandwidth) == HXR_OK) && 
	    (lUseNetBandwidth == 1))
	{
	    m_pSimulatedSourceBandwidth = new SourceBandwidthInfo;
	    m_pSimulatedSourceBandwidth->AddRef();
	}
   }
#endif /* HELIX_FEATURE_ASM && HELIX_FEATURE_REGISTRY*/

    if (!theErr)
    {
        // If a fileformat claimed the scheme/extension pair, then
        // we will not attempt to load a filesystem or create a
        // file object.
        if (!m_pFFClaimURLEnumerator)
        {
	    const char* pProtocolEnd = HXFindChar(purl,':');
	    if (!pProtocolEnd)
	    {
	        goto exit;
	    }

	    int nLength = pProtocolEnd - purl;
	    CHXString strProtocol(purl,nLength);

	    IHXPlugin2Handler* pPlugin2Handler;
	    if (HXR_OK != m_pEngine->QueryInterface(IID_IHXPlugin2Handler, (void**)&pPlugin2Handler))
	    {
	        theErr = HXR_UNEXPECTED;
	        goto exit;
	    }

	    if (HXR_OK == pPlugin2Handler->FindPluginUsingStrings(PLUGIN_CLASS, PLUGIN_FILESYSTEM_TYPE, 
	        PLUGIN_FILESYSTEMPROTOCOL, (char*)(const char*)strProtocol, NULL, NULL, pUnknown))
	    {
	        pUnknown->QueryInterface(IID_IHXFileSystemObject, (void**) &m_pFSObject);
                HX_RELEASE(pUnknown);
	    }
	    else
	    {
                MergeUpgradeRequest(TRUE, (char*)(const char*) strProtocol);
	        theErr = HXR_NO_FILESYSTEM;
	    }
	    HX_RELEASE(pPlugin2Handler);

	    // Initialize the File System plugin...
	    if (!theErr)
	    {
	        if (HXR_OK != m_pFSObject->QueryInterface(IID_IHXPlugin,(void**)&pHXPlugin))
	        {
		    theErr = HXR_NOT_INITIALIZED;
		    goto exit;
	        }
    		
	        if (HXR_OK != pHXPlugin->InitPlugin((IUnknown*) (IHXStreamSource*)this))
	        {
		    theErr = HXR_NOT_INITIALIZED;
		    goto exit;
	        }
	    }

	    // Create a FileObject in order to determine the mime-type of the file
	    if (!theErr)
	    {
                // Create the file object...
	        if (HXR_OK != m_pFSObject->CreateFile(&pObject))
	        {
		    theErr = HXR_NOT_INITIALIZED;
		    goto exit;
	        }
    	    	    
	        if ((HXR_OK != pObject->QueryInterface(IID_IHXFileObject,
						    (void**)&m_pFileObject)) ||
	            (HXR_OK != pObject->QueryInterface(IID_IHXRequestHandler,
						    (void**)&m_pRequestHandler)))
	        {
		    theErr = HXR_NOT_INITIALIZED;
		    goto exit;
	        }
	        IHXHTTPRedirect* pHttpRedirect = NULL;
                m_pFileObject->QueryInterface(IID_IHXHTTPRedirect, (void**)&pHttpRedirect);
                if (pHttpRedirect)
                {
                    pHttpRedirect->SetResponseObject((IHXHTTPRedirectResponse*) this);
                    pHttpRedirect->Release();
                }
            }
        }

        if (!theErr)
        {
            // set request
            SetRequest(m_pURL, bAltURL);
        }

        if (!theErr && !m_pFFClaimURLEnumerator)
        {
	    // we want to protect against the TLC opening another URL
	    m_pPlayer->SetModal(TRUE);
	    theErr = m_pRequestHandler->SetRequest(m_pRequest);
	    m_pPlayer->SetModal(FALSE);

	    if (theErr != HXR_OK)
	    {
		theErr = HXR_NOT_INITIALIZED;
		goto exit;
	    }

	}

        if (!theErr)
        {
	    theErr = ExtendedSetup(purl);
        }
    }

exit:

    HX_RELEASE(pHXPlugin);
    HX_RELEASE(pObject);

    if (theErr)
    {
	HX_RELEASE(m_pFSObject);
    }

    HXLOGL3(HXLOG_CORP, "HXPlayer[%p]::::HXFileSource[%p]::Setup(URL=%s, bAltURL=%c) End=%ld",
	    m_pPlayer,
	    this,
	    pURL ? pURL->GetURL() : "NULL",
	    bAltURL ? 'T' : 'F',
	    theErr);
    
    return theErr;	
}

HX_RESULT
HXFileSource::ExtendedSetup(const char* pszURL)
{
    HX_RESULT		theErr = HXR_OK;
    char*		pszTemp = NULL;
    const char*		pMimeType = NULL;
    IHXBuffer*		pValue = NULL;
    IHXValues*		pResponseHeaders = NULL;
    IHXFileMimeMapper*  pFileMimeMapper = NULL;
    HXBOOL              bFoundMimeType = FALSE;

    // See if the caller has suggested a mime type by setting something in the response
    // headers... hxclientkit does this.
    if (m_pRequest && HXR_OK == m_pRequest->GetResponseHeaders(pResponseHeaders) && pResponseHeaders)
    {
        if (HXR_OK == pResponseHeaders->GetPropertyCString("Content-Type", pValue) &&
            pValue)
	{
	    pMimeType = (char*)pValue->GetBuffer();

            FinishSetup(HXR_OK, pMimeType);
	    theErr = mLastError;
            bFoundMimeType = TRUE;

            HX_RELEASE(pValue);
        }
       	HX_RELEASE(pResponseHeaders);
    }

    // If a fileformat plugin can claim the scheme:extension
    // pair, then we don't need to try to get the file
    // mime type from the file object, since we already
    // know of file formats who can claim this URL.
    if (!bFoundMimeType && !m_pFFClaimURLEnumerator)
    {
        if (HXR_OK == m_pFileObject->QueryInterface(IID_IHXFileMimeMapper,(void**)&pFileMimeMapper))
        {
            /* Detect the mime type from the http header */
            if (!m_pMimeFinderResponse) 
    	    {
    	        // Initialize our MimeFinder!
    	        m_pMimeFinderResponse = new CMimeFinderFileResponse(this);
	        if (!m_pMimeFinderResponse)
	        {
		    theErr = HXR_NOT_INITIALIZED;
	        }
	        else
	        {
		    m_pMimeFinderResponse->AddRef();
	        }
            }

	    // Init the file object, and ask for the mime type!
	    if (!theErr && HXR_OK == pFileMimeMapper->FindMimeType(pszURL, m_pMimeFinderResponse))
            {
                bFoundMimeType = TRUE;
            }
            else
	    {
	        theErr = HXR_DOC_MISSING;
	    }
	    HX_RELEASE(pFileMimeMapper);
        }
    }

    if (!bFoundMimeType)
    {
        if (HXXFile::IsPlusURL(pszURL))
        {
            pMimeType = "application/x-pn-plusurl";
	}
        else
        {
	    // separate options from the URL
	    pszTemp = (char*) ::HXFindChar(pszURL, '?');
	    if (pszTemp)
	    {
	        *pszTemp = '\0';
	    }
	}
	
	FinishSetup(HXR_OK, pMimeType);
	theErr = mLastError;
        bFoundMimeType = TRUE;
    }

    return theErr;
}

void
HXFileSource::SetSchemeExtensionPairClaimedEnumerator(IHXPluginSearchEnumerator* pFFClaim)
{
    if (pFFClaim)
    {
        HX_RELEASE(m_pFFClaimURLEnumerator);
        m_pFFClaimURLEnumerator = pFFClaim;
        m_pFFClaimURLEnumerator->AddRef();
    }
}

void 
HXFileSource::FinishSetup(HX_RESULT status, const char* pMimeType)
{
#if !defined(HELIX_FEATURE_META)
    m_bValidateMetaDone = TRUE;
    m_bIsMeta = FALSE;
#endif /* HELIX_FEATURE_META */

    if (status == HXR_OK)
    {
	if (pMimeType)
	{
	    HX_VECTOR_DELETE(m_pMimeType);
	    m_pMimeType = ::new_string(pMimeType);
	}

	// detect if this is a valid meta file if we haven't done so
        // If a fileformat claimed the scheme:extension pair, no
        // need to try the .ram fileformat first.
	if (!m_bValidateMetaDone && !m_pFFClaimURLEnumerator)
	{
	    AttempToLoadFilePlugin(RAM_MIMETYPE);
	}
        // retrieve the whole SDP file without SDP fileformat/renderer which
        // requires basic group&track
	else if (m_pMimeType && 0 == strcasecmp(m_pMimeType, "application/sdp"))
        {
            if (!m_pFileReader)
            {
                m_pFileReader = new CFileReader(this);
                HX_ADDREF(m_pFileReader);
            }

            if (m_pFileReader)
            {
                m_pFileReader->GetFile(m_pFileObject);
            }
            return;
        }
	// we have already validated the file and know whether
	// it is a meta or not
        else
	{
	    if (m_bIsMeta)
	    {
		AttempToLoadFilePlugin(RAM_MIMETYPE);
	    }
	    else
	    {
#if defined(HELIX_FEATURE_FILE_RECOGNIZER)
                // Call recognizer here if we do not have a definite
                // mimetype from the filesytem
                // If we already have a fileformat willing to claim
                // this scheme:extension pair, then we don't need to
                // try and get the file mime type from the recognizer.
                if ((!m_pMimeType || *m_pMimeType == '*') && !m_pFileRecognizer && !m_pFFClaimURLEnumerator)
                {
                    m_pFileRecognizer = new CHXFileRecognizer((IUnknown*)(IHXStreamSource*)this);

                    if (m_pFileRecognizer)
                    {
                        m_pFileRecognizer->AddRef();

                        if (!m_pMimeFinderResponse)
                        {
                            m_pMimeFinderResponse = new CMimeFinderFileResponse(this);                        
                            HX_ADDREF(m_pMimeFinderResponse);
                        }

                        if (m_pMimeFinderResponse)
                        {
                            m_pFileRecognizer->GetMimeType(m_pFileObject, 
                                                           (IHXFileRecognizerResponse*)m_pMimeFinderResponse);
                            return;
                        }
                        else
                        {
                            HX_DELETE(m_pFileRecognizer); 
                        }
                    }
                }
#endif /* #if defined(HELIX_FEATURE_FILE_RECOGNIZER) */
                
                AttempToLoadFilePlugin(m_pMimeType);
	    }
	}
    }
    else if (!mLastError)
    {
	mLastError = status;
	ReportError(mLastError);
    }
}

void 
HXFileSource::AttempToLoadFilePlugin(const char* pMimeType)
{
    char*   pExtension = NULL;
    char*   pPos = NULL;
    char*   pszTemp = NULL;
    char*   pszURL = NULL;
    HX_RESULT theErr = HXR_OK;

    // Don't try the .ram extension first if we already
    // know of a fileformat which can claim the scheme
    // and extension. Otherwise, if we haven't tried
    // .ram yet, then try it.
    if (!m_bValidateMetaDone && !m_pFFClaimURLEnumerator)
    {
	pExtension = "ram";
    }
    else
    {
	// This will fix up any redirected 
	GetURL();
    
	// make a copy of the URL
	pszURL = new char[strlen(m_pszURL)+1];
	memset(pszURL, 0, strlen(m_pszURL)+1);

	strcpy(pszURL, m_pszURL); /* Flawfinder: ignore */

	// separate fragment from the URL
	pszTemp = (char*) ::HXFindChar(pszURL, '#');
	if (pszTemp)
	{
	    *pszTemp = '\0';
	}

	// separate options from the URL
	pszTemp = (char*) ::HXFindChar(pszURL, '?');
	if (pszTemp)
	{
	    *pszTemp = '\0';
	}

	// load plugin

        // find the extension, stopping if we hit any path separators
        pPos = pszURL;
        while (*pPos)
        {
            pPos++;
        }

        while (pPos != pszURL)
        {
            if (*pPos == '.')
            {
                pExtension = pPos + 1;
                break;
            }

            if (strncmp(pPos, OS_PATH_DELIMITER, strlen(OS_PATH_DELIMITER)) == 0)
            {
                // Don't search past a directory separator for an extension
                break;
            }
                            
            pPos--;
        }
        	    
	if (pExtension)
	{
	    HX_VECTOR_DELETE(m_pExtension);
	    m_pExtension = ::new_string(pExtension);
	}
    }

    if (!m_pCurrentFileFormatUnk)
    {
	IHXPluginHandler3* pPlugin2Handler3 = NULL;
	HX_VERIFY(HXR_OK ==
	    m_pPlayer->m_pPlugin2Handler->QueryInterface(IID_IHXPluginHandler3, (void**) &pPlugin2Handler3));

        // If the player told us that a fileformat can claiim
        // the scheme:extension pair, then use that enumerator
        if (m_pFFClaimURLEnumerator)
        {
            HX_RELEASE(m_pFileFormatEnumerator);
            m_pFileFormatEnumerator = m_pFFClaimURLEnumerator;
            m_pFileFormatEnumerator->AddRef();
        }

        // Next try and get a fileformat enumerator
        // based on mime type
	if (pMimeType && !m_pFileFormatEnumerator)
	{
	    pPlugin2Handler3->FindGroupOfPluginsUsingStrings(PLUGIN_CLASS, PLUGIN_FILEFORMAT_TYPE, 
	    PLUGIN_FILEMIMETYPES, (char*)pMimeType, 0, 0, m_pFileFormatEnumerator);
	}

        // Last try and get an enumerator based on file extension
	if (!m_pFileFormatEnumerator)
	{
	    pPlugin2Handler3->FindGroupOfPluginsUsingStrings(PLUGIN_CLASS, PLUGIN_FILEFORMAT_TYPE, 
		PLUGIN_FILEEXTENSIONS, pExtension, 0 ,0, m_pFileFormatEnumerator);
	}

	HX_RELEASE(pPlugin2Handler3);

	if (m_pFileFormatEnumerator)
	{
	    m_pFileFormatEnumerator->GetNextPlugin(m_pCurrentFileFormatUnk, NULL);
	    HX_ASSERT(m_pCurrentFileFormatUnk != NULL);
	}
    }

    if (!m_pCurrentFileFormatUnk)
    {
	if (!m_bDefaultAltURL)
	{
	    theErr = HXR_NO_FILEFORMAT;

#if defined(HELIX_FEATURE_AUTOUPGRADE)
	    IHXUpgradeCollection* pUpgradeCollection = NULL;
	    if(m_pPlayer)
		m_pPlayer->QueryInterface(IID_IHXUpgradeCollection, (void**)&pUpgradeCollection);
	    if(pUpgradeCollection)
	    {
		IHXBuffer* pPluginID = NULL;
		CreateBufferCCF(pPluginID, (IUnknown*)(IHXPlayer*)m_pPlayer);

		if (pMimeType && !(*pMimeType == '*'))
		{
			    pPluginID->Set((const UINT8*)pMimeType, strlen(pMimeType) + 1);
		}
		else if (pExtension)
		{
			    // return file extension if mimeType is unknown
			    pPluginID->Set((const UINT8*)pExtension, strlen(pExtension) + 1);
		}
		else
		{
			    pPluginID->Set((const UINT8*)"Unknown FileFormat", strlen("Unknown FileFormat") + 1);
		}

		pUpgradeCollection->Add(eUT_Required, pPluginID, 0, 0);
		pPluginID->Release();
		pUpgradeCollection->Release();
	    }
#endif /* HELIX_FEATURE_AUTOUPGRADE */
	}
	else
	{
	    theErr = HXR_INVALID_FILE;
	}
    }

    // initialize fileformat plugins...
    if (!theErr)
    {
	theErr = InitializeFileFormat();
    }

    HX_VECTOR_DELETE(pszURL);

    // if there is an error, make sure there is no m_pCurrentFileFormatUnk...
    // if m_pCurrentFileFormatUnk is not NULL, it means we need to try 
    // the next plugin that supports this mimetype.
    // ignore error in this case.
    if (theErr && !m_pCurrentFileFormatUnk)
    {
	mLastError = theErr;

	CheckForDefaultUpgrade(theErr);

	// merge any upgrade requests for this source to the player
	MergeUpgradeRequest(m_bAddDefaultUpgrade, m_pDefaultUpgradeString);

#if defined(HELIX_FEATURE_AUTOUPGRADE)
	if (theErr != HXR_NO_FILEFORMAT)
#endif
	{
	    ReportError(theErr);
	}
    }
}

HX_RESULT
HXFileSource::InitializeFileFormat()
{
    HX_RESULT   theErr       = HXR_OK;
    HX_RESULT   resultInitFF = HXR_OK;
    IHXRequest* pRequest     = NULL;
    IHXPlugin*  pPlugin      = NULL;
#if defined(HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS)
    IHXPDStatusMgr* pPDStatusMgr = NULL;
#endif // /HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS.
    
    HX_ASSERT(m_pCurrentFileFormatUnk != NULL);

    if (m_pCurrentFileFormatUnk)
    {
	HX_VERIFY(HXR_OK ==
	    m_pCurrentFileFormatUnk->QueryInterface(IID_IHXFileFormatObject, (void**) &m_pFFObject));
    }
    
    HX_RELEASE(m_pCurrentFileFormatUnk);
    m_bCurrentFileFormatUnkInUse = FALSE;

    HX_ASSERT(m_pFFObject != NULL);
    if (!m_pFFObject)
    {
	return HXR_INVALID_FILE; //??
    }

    // we keep RAM FF object around by AddRef()
    // to avoid self-destruct in ::InitDone when it's not a RAM
    // It will be released in ::FinishSetup().
    
    if (!m_bValidateMetaDone && !m_pFFClaimURLEnumerator)
    {
	HX_ASSERT(!m_pRAMFFObject);
	m_pRAMFFObject = m_pFFObject;
	m_pRAMFFObject->AddRef();
    }
    
    if (HXR_OK != m_pFFObject->QueryInterface(IID_IHXPlugin,(void**)&pPlugin))
    {
	theErr = HXR_NOT_INITIALIZED;
	goto exit;
    }

    if (HXR_OK != pPlugin->InitPlugin((IUnknown*) (IHXStreamSource*)this))
    {
	theErr = HXR_NOT_INITIALIZED;
	goto exit;
    }

    if (m_pRequestHandler && m_pRequestHandler->GetRequest(pRequest) != HXR_OK)
    {
	theErr = HXR_NOT_INITIALIZED;
	goto exit;
    }
    if (!pRequest)
    {
        pRequest = m_pRequest;
        pRequest->AddRef();
    }

#if defined(HELIX_FEATURE_PLAYBACK_VELOCITY)
    // If this is not the RAM fileformat, then create
    // the playback velocity proxy
    if (m_bValidateMetaDone)
    {
        CHXPlaybackVelocityProxyFF* pProxyFF = new CHXPlaybackVelocityProxyFF((IUnknown*) (IHXStreamSource*)this,
                                                                              m_pFFObject);
        if (pProxyFF)
        {
            HX_RELEASE(m_pFFObject);
            theErr = pProxyFF->QueryInterface(IID_IHXFileFormatObject, (void**) &m_pFFObject);
            if (SUCCEEDED(theErr))
            {
                // Get the IHXPlaybackVelocity interface for convenience
                HX_RELEASE(m_pPlaybackVelocity);
                theErr = m_pFFObject->QueryInterface(IID_IHXPlaybackVelocity,
                                                     (void**) &m_pPlaybackVelocity);
                if (SUCCEEDED(theErr))
                {
                    // If InitVelocityControl was called before the
                    // source was created, then we saved m_pPlaybackVelocityResponse.
                    // Therefore, we need to now initialize m_pPlaybackVelocity.
                    if (m_pPlaybackVelocityResponse)
                    {
                        theErr = m_pPlaybackVelocity->InitVelocityControl(m_pPlaybackVelocityResponse);
                    }
                    // Now we can release the reponse interface, 
                    // since m_pPlaybackVelocity now owns it
                    HX_RELEASE(m_pPlaybackVelocityResponse);
                }
            }
        }
    }
    else
    {
        // Get the IHXPlaybackVelocity interface for convenience
        HX_RELEASE(m_pPlaybackVelocity);
        theErr = m_pFFObject->QueryInterface(IID_IHXPlaybackVelocity,
                                                (void**) &m_pPlaybackVelocity);
        if (SUCCEEDED(theErr))
        {
            // If InitVelocityControl was called before the
            // source was created, then we saved m_pPlaybackVelocityResponse.
            // Therefore, we need to now initialize m_pPlaybackVelocity.
            if (m_pPlaybackVelocityResponse)
            {
                theErr = m_pPlaybackVelocity->InitVelocityControl(m_pPlaybackVelocityResponse);
            }
            // Now we can release the reponse interface, 
            // since m_pPlaybackVelocity now owns it
            HX_RELEASE(m_pPlaybackVelocityResponse);
        }
	else
	{
	    // Playback velocity interface is optional
	    theErr = HXR_OK;
	}
    }
#endif /* #if defined(HELIX_FEATURE_PLAYBACK_VELOCITY) */

    if (HXR_OK != (resultInitFF=m_pFFObject->InitFileFormat(pRequest,
	                                      this,
	                                      m_pFileObject)) )
    {
	if(HXR_UNSUPPORTED_VIDEO == resultInitFF	||
	   HXR_UNSUPPORTED_AUDIO == resultInitFF	||
	   HXR_DOC_MISSING == resultInitFF)
	{
	    theErr = resultInitFF;
	}
	else
	{
	    theErr = HXR_INVALID_FILE; //HXR_NOT_INITIALIZED;
	}
	goto exit;
    }

#if defined(HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS)
    //HX_ASSERT(m_pFFObject);
    if (m_pFFObject)
    {
        m_pFFObject->QueryInterface(IID_IHXPDStatusMgr, (void**)&pPDStatusMgr);
    }
    if (pPDStatusMgr)
    {
        HX_RESULT hxrsltAO = pPDStatusMgr->AddObserver((IHXPDStatusObserver*)this); 
    }
    HX_RELEASE(pPDStatusMgr);
#endif // /HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS.



exit:

    HX_RELEASE(pRequest);
    HX_RELEASE(pPlugin);

    if (theErr)
    {
	/* do we need to try the next one! */
	if (!m_pCurrentFileFormatUnk && m_pFileFormatEnumerator)
	{
	    m_pFileFormatEnumerator->GetNextPlugin(m_pCurrentFileFormatUnk, NULL);
	    if (m_pCurrentFileFormatUnk && m_pSourceInfo)
	    {
		m_pSourceInfo->ScheduleProcessCallback();
	    }
	}	
    }

    return theErr;
}

HX_RESULT 
HXFileSource::DoCleanup(EndCode endCode)
{
    /* UnRegister any previously registered source */
    if (m_pSourceInfo)
    {
       	m_pSourceInfo->UnRegister();
    }

    CleanupFileObjects();

    if (m_pFileReader)
    {
        m_pFileReader->Close();
        HX_RELEASE(m_pFileReader);
    }
    HX_RELEASE(m_pRAMFFObject);

    HX_RELEASE(m_pCurrentFileFormatUnk);
    m_bCurrentFileFormatUnkInUse = FALSE;
    HX_RELEASE(m_pFileFormatEnumerator);
    HX_RELEASE(m_pFFClaimURLEnumerator);

#if defined(HELIX_FEATURE_ASM)
    HX_RELEASE(m_pSimulatedSourceBandwidth);
#endif /* HELIX_FEATURE_ASM */

    HX_VECTOR_DELETE(m_pMimeType);
    HX_VECTOR_DELETE(m_pExtension);
    HX_VECTOR_DELETE(m_pDefaultUpgradeString);    
    m_bAddDefaultUpgrade = FALSE;

#if defined(HELIX_FEATURE_FILE_RECOGNIZER)
    HX_RELEASE(m_pFileRecognizer);
#endif /* #if defined(HELIX_FEATURE_FILE_RECOGNIZER) */

#if defined(HELIX_FEATURE_STATS) && defined(HELIX_FEATURE_REGISTRY)
    /* Reason to add this code is because in FileSource 
     * we create/delete it while in NetSource
     * case, Protocol creates/deletes it
     */
    for (CHXMapLongToObj::Iterator i = mStreamInfoTable->Begin();
         i != mStreamInfoTable->End(); ++i) 
    {    
	STREAM_INFO* sInfo = (STREAM_INFO*) (*i);
	
	if (sInfo)
	{
	    HX_DELETE (sInfo->m_pStats);
	}
    }
#endif /* HELIX_FEATURE_STATS && HELIX_FEATURE_REGISTRY*/

#if defined(HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS)
    if (m_pPDSObserverList)
    {
        CHXSimpleList::Iterator i = m_pPDSObserverList->Begin();
        for (; i != m_pPDSObserverList->End(); ++i)
        {
            IHXPDStatusObserver* pObserver = (IHXPDStatusObserver*)(*i);
            HX_RELEASE(pObserver);
        }
        HX_DELETE(m_pPDSObserverList);
    }
#endif // /HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS.

    if (m_pHXSrcBufStats)
    {
        m_pHXSrcBufStats->Close();
        HX_RELEASE(m_pHXSrcBufStats);
    }
    HXSource::DoCleanup();

    return HXR_OK;
}

void
HXFileSource::ReportError(HX_RESULT theErr)
{
    m_pPlayer->ReportError( this, theErr, m_pURL->GetURL());
}

void
HXFileSource::CleanupFileObjects()
{
    HX_RELEASE (m_pFSObject);
    if (m_pFFObject)
    {
	m_pFFObject->Close();
	HX_RELEASE (m_pFFObject);
    }    
    
    HX_RELEASE (m_pFileResponse);
    if (m_pFileObject)
    {
	m_pFileObject->Close();
	HX_RELEASE (m_pFileObject);
    }

    HX_RELEASE (m_pRequestHandler);
    HX_RELEASE (m_pMimeFinderResponse);
}

HX_RESULT	
HXFileSource::DoSeek(ULONG32 seekTime)
{
    /* Add any start time to seek time */
    if (seekTime >= m_ulDelay)
    {
	seekTime    -= m_ulDelay;
	m_bDelayed   = FALSE;
    }
    else
    {
	seekTime = 0;

	/* This source has not been even started yet...
	 * Do not attempt to seek it if the start time = 0
	 */
	if (m_bDelayed && m_ulStartTime == 0 && !m_bSourceEnd)
	{
	    // will start pre-fetch again in TryResume()
	    if (!m_bIsPreBufferingDone)
	    {
		m_bIsPreBufferingStarted = FALSE;

		// will be registered again in DoResume() or TryResume()
		if (m_pSourceInfo)
		{
		    m_pSourceInfo->UnRegister();
		}
	    }

	    return HXR_OK;
	}

	m_bDelayed = TRUE;
    }

    seekTime	+= m_ulStartTime;

    //  Skip over any stream data packets that occur between delay and current
    // play time.  If delay is less than current play time, then seek past the
    // difference to avoid streaming all those unneeded (too-early) packets.
    // Note: this condition can occur if pfs:displayWhile property is used in
    // a SMIL2 element and it becomes "displayable" some time after the
    // presentation has already started; in that case its begin delay is zero
    // (relative to its syncbase time so it may be non-zero if its parent or
    // ancestor starts after 0 in the presentation), and the time of its
    // AddTrack() occurs after zero (relative to syncbase):
    if (m_pSourceInfo->m_bSeekOnLateBegin)
    {
        UINT32 ulCurPlayTime = m_pPlayer->GetInternalCurrentPlayTime();
        if (m_bFirstResume  &&  (ulCurPlayTime > m_ulDelay)  &&
                (ulCurPlayTime - m_ulDelay >
                m_pSourceInfo->m_ulSeekOnLateBeginTolerance) )
        {
            seekTime += ulCurPlayTime - m_ulDelay;
            HXLOGL4(HXLOG_CORE, "[%p]HXFileSource::DoSeek()\tseekTime(%lu) "
                    "increased for late begin by %lu (== ulCurPlayTime(%lu) -"
                    " m_ulDelay(%lu)\n", this, seekTime,
                    ulCurPlayTime - m_ulDelay, ulCurPlayTime, m_ulDelay);
        }
    }

    /* Are we seeking past the last expected packet time?
     * If so, don't bother... and mark this source as done
     */
    HX_ASSERT(m_llLastExpectedPacketTime < MAX_UINT32);
    // XXX HP make sure the source has been initialized otherwise
    // m_llLastExpectedPacketTime could be 0 and we could falsely
    // mark the source ended
    if (m_bInitialized && !mLiveStream && seekTime >= INT64_TO_UINT32(m_llLastExpectedPacketTime))
    {
	if (m_pSourceInfo && m_pSourceInfo->m_bSeekToLastFrame)
	{
	    seekTime = INT64_TO_UINT32(m_llLastExpectedPacketTime);
	}
	else
	{
	    m_bSourceEnd = TRUE;
	    m_bForcedSourceEnd = TRUE;
	    AdjustClipBandwidthStats(FALSE);
	    goto cleanup;
	}
    }

    m_bInitialBuffering = TRUE;
    m_bForcedSourceEnd	= FALSE;
    m_uActiveStreams	= m_uNumStreams; 
    m_bIsPreBufferingStarted	= FALSE;
    m_bIsPreBufferingDone	= FALSE;
    m_pushDownStatus            = PUSHDOWN_NONE;

    if (m_nSeeking == 0)
    {
	m_nSeeking++;
    }

    if (mLiveStream && m_bPlayFromRecordControl)
    {
        seekTime += m_ulFirstPacketTime;
    }

#if defined(HELIX_FEATURE_RECORDCONTROL)
    if (m_pRecordControl && m_pRecordControl->Seek(seekTime) == HXR_OK &&
	m_bPlayFromRecordControl)
    {
	m_pBufferManager->DoSeek(seekTime, TRUE);
	SeekDone(HXR_OK);
    }
    else
#endif /* HELIX_FEATURE_RECORDCONTROL */
    {
	m_bSourceEnd = FALSE;

	m_pBufferManager->DoSeek(seekTime, FALSE);

	if (HXR_OK != m_pFFObject->Seek(seekTime))
	{
	    if (m_nSeeking)
	    {
		m_nSeeking--;
	    }
	}
    }

    if (m_pHXSrcBufStats)
    {
        m_pHXSrcBufStats->Reset();
    }

cleanup:

    return HXR_OK;
}

	
HX_RESULT	
HXFileSource::DoPause(void)
{
    if (m_bPaused)
    {
	return HXR_OK;
    }

    /* Only if it is an external pause */
    if (!m_bSourceEnd && !m_bDelayed && m_pBufferManager)
    {
	m_pBufferManager->DoPause();
    }

    m_bPaused = TRUE;

    return HXR_OK;
}


HX_RESULT   
HXFileSource::StartInitialization(void)
{
    m_bFastStartInProgress = TRUE;
    m_pBufferManager->DoResume();
    if (m_pSourceInfo)
    {
	m_pSourceInfo->Resumed();
    }
    return HXR_OK;
}

HX_RESULT
HXFileSource::StopInitialization(void)
{
	 return (DoPause());
}


HX_RESULT   
HXFileSource::DoResume(UINT32 ulLoopEntryTime, UINT32 ulProcessingTimeAllowance)
{
    HX_RESULT theErr = HXR_OK;

    m_bFastStartInProgress = FALSE;
    ChangeRebufferStatus(REBUFFER_NONE);

    /* This may happen if a new source is added from SMIL renderer during 
     * initialization of exisitng sources. We will eventually call Resume 
     * on this source once it is initialized in SourceInfo::ProcessIdle
     */
    if (!m_bInitialized)
    {
	return HXR_OK;
    }

    if (m_bSourceEnd || CanBeResumed())
    {    
	m_bResumePending = FALSE;

	if (!m_bSourceEnd)
	{
	    m_pBufferManager->DoResume();

	    if (m_pSourceInfo && !m_pSourceInfo->IsRegistered() && m_pSourceInfo->IsActive())
            {
                m_pSourceInfo->Register();
                if (m_pPlayer)
                {
                    m_pPlayer->RegisterSourcesDone();
                }
            }
	}

	// resume the audio streams if the source is added
	// while the player is in play mode
	// CAUTION: this might cause rewind in audio service
	if (m_bFirstResume && m_pPlayer->IsPlaying() &&
	    m_ulDelay <= m_pPlayer->GetInternalCurrentPlayTime())
	{
	    ResumeAudioStreams();
	}

	m_bFirstResume = FALSE;
	m_bPaused = FALSE;

	if (m_pSourceInfo)
	{
	    m_pSourceInfo->Resumed();
	}

	if (!m_bSourceEnd)
	{
	    theErr = FillBuffers(ulLoopEntryTime, ulProcessingTimeAllowance);
	}
    }

    if (!theErr && !m_bIsActive && !m_bDelayed &&
	m_pPlayer->GetInternalCurrentPlayTime() >= m_ulDelay)
    {
	AdjustClipBandwidthStats(TRUE);
    }

    if (theErr == HXR_AT_END)
    {
	SetEndOfClip();
	theErr = HXR_OK;
    }

    return theErr;
}

/************************************************************************
 *	Method:
 *	    IHXPendingStatus::GetStatus
 *	Purpose:
 *	    Called by the user to get the current pending status from an object
 */

STDMETHODIMP
HXFileSource::GetStatus
(
    REF(UINT16) uStatusCode, 
    REF(IHXBuffer*) pStatusDesc, 
    REF(UINT16) ulPercentDone
)
{
    HX_RESULT hResult = HXR_OK;
    IHXPendingStatus* pStatus = NULL;

    UINT16  buffer	= 100;
    UINT16  statusCode	= HX_STATUS_READY;
    UINT16  percentDone = 0;

    uStatusCode	    = HX_STATUS_READY;
    pStatusDesc	    = 0;
    ulPercentDone   = 0;
    
    if (m_bDelayed)
    {
	return HXR_OK;
    }

    if (FastStartPrerequisitesFullfilled(uStatusCode, ulPercentDone))
    {
	m_uLastStatusCode = uStatusCode;
	FastStartUpdateInfo();
        return HXR_OK;
    }

    if (m_bSourceEnd && !m_bPacketlessSource)
    {
	if (!IsRebufferDone())
	{
	    uStatusCode = HX_STATUS_BUFFERING;
	    ulPercentDone = 99;
	}
	else
	{
	    if (m_bInitialBuffering)
	    {
		    InitialBufferingDone();
	    }

	    m_ulLastBufferingReturned = 100;
	    uStatusCode = HX_STATUS_READY;
    	    if (m_rebufferStatus == REBUFFER_REQUIRED)
            {
                ChangeRebufferStatus(REBUFFER_NONE);
            }
	}

	return HXR_OK;
    }

    if (m_bInitialized && !m_bPacketlessSource)
    {
	if (m_bFirstResume)
	{
	    uStatusCode	    = HX_STATUS_INITIALIZING;
	    return HXR_OK;
	}

	m_pBufferManager->GetStatus(uStatusCode, 
				    pStatusDesc,
				    ulPercentDone);

	buffer = ulPercentDone;

	/* We only aggregate buffering from a lower level if we are in a buffering mode.
	 * Reason: Once the initial bufering is done, we go in buffering more ONLY IF the
	 * renderer tells us that it is in a panic state and we run out of packets.
	 */
	if (buffer == 100 && !m_bInitialBuffering)
	{	    
	    // Rebuffer requested by the Renderer might not be
	    // done yet
	    if (!IsRebufferDone())
	    {
		uStatusCode = HX_STATUS_BUFFERING;
		ulPercentDone = 99;
	    }
	    else
	    {
		uStatusCode = HX_STATUS_READY;
    	        if (m_rebufferStatus == REBUFFER_REQUIRED)
                {
                    ChangeRebufferStatus(REBUFFER_NONE);
                }
	    }

	    return HXR_OK;
	}
    }

    if (m_pFFObject)
    {
	if (HXR_OK != m_pFFObject->QueryInterface(IID_IHXPendingStatus, (void**)&pStatus))
	{
	    goto exit;
	}

	if (HXR_OK != pStatus->GetStatus(statusCode, pStatusDesc, percentDone))
	{
	    goto exit;
	}
    }
    else if (m_pFileObject)
    {
	if (HXR_OK != m_pFileObject->QueryInterface(IID_IHXPendingStatus, (void**)&pStatus))
	{
	    goto exit;
	}

	if (HXR_OK != pStatus->GetStatus(statusCode, pStatusDesc, percentDone))
	{
	    goto exit;
	}
    }

exit:
    
    if (HX_STATUS_CONTACTING == statusCode)
    {
	uStatusCode = HX_STATUS_CONTACTING;
	ulPercentDone = 0;
    }
    else if (!m_bInitialized)
    {
	uStatusCode	= HX_STATUS_INITIALIZING;
	ulPercentDone	= 0;
    }
    else if (HX_STATUS_READY == statusCode && 100 == buffer)
    {
	uStatusCode = HX_STATUS_READY;
	ulPercentDone = 0;
	m_ulLastBufferingReturned = 100;
    }
    else
    {
	uStatusCode = HX_STATUS_BUFFERING;
	
        // If we have a packetless source, then the "buffer"
        // local variable should be zero
        if (m_bPacketlessSource && statusCode == HX_STATUS_BUFFERING)
        {
            buffer = 0;
        }

	if (HX_STATUS_READY == statusCode)
	{
	    ulPercentDone = (UINT16)buffer;
	}
	else
	{
	    ulPercentDone = (UINT16)((buffer + percentDone) * 0.5);
	}

	// Do not go back
	if (ulPercentDone < m_ulLastBufferingReturned && m_ulLastBufferingReturned != 100)
	{
	    ulPercentDone = (UINT16) m_ulLastBufferingReturned;
	}
	else
	{
	    m_ulLastBufferingReturned = ulPercentDone;
	}
    }

    HX_RELEASE(pStatus);
  
    ulPercentDone = ulPercentDone <= 100 ? (UINT16)ulPercentDone : 100;

    if (m_bInitialBuffering && HX_STATUS_READY == uStatusCode)
    {
        InitialBufferingDone();
    }

    FastStartUpdateInfo();

    return hResult;
}

UINT16		
HXFileSource::GetNumStreams(void)
{
    HX_ASSERT(m_bInitialized);
    
    return m_uNumStreams;
}


HX_RESULT	
HXFileSource::GetStreamInfo(ULONG32	    ulStreamNumber,
			     STREAM_INFO*&  theStreamInfo)
{
    HX_RESULT theErr = HXR_OK;
    STREAM_INFO* lpStreamInfo = 0;

    if (!mStreamInfoTable->Lookup((LONG32)ulStreamNumber, (void *&)lpStreamInfo))
    {
	theErr = HXR_INVALID_PARAMETER;
    }

    theStreamInfo = lpStreamInfo;

    return theErr;
}

HX_RESULT    
HXFileSource::GetEvent(UINT16 usStreamNumber, 
		       CHXEvent*& theEvent, 
		       UINT32 ulLoopEntryTime, 
		       UINT32 ulProcessingTimeAllowance)
{
    HX_RESULT theErr = HXR_OK;

    HX_TRACE("HXFileSource::GetEvent");
    
    theEvent	= 0;
    
    if (!m_bInitialized)
    {
	return HXR_NOT_INITIALIZED;
    }

    if (mLastError != HXR_OK)
    {
	return mLastError;
    }

    if (m_bPaused && m_bDelayed)
    {
	if (TryResume())
	{
	    m_pPlayer->RegisterSourcesDone();
    	    DoResume(ulLoopEntryTime, ulProcessingTimeAllowance);
	}
	else
	{
	    return HXR_NO_DATA;
	}
    }

    HXLOGL4(HXLOG_CORP, "HXPlayer[%p]::::HXFileSource[%p]::GetEvent(Strm=%hu)",
	    m_pPlayer,
	    this,
            usStreamNumber);

    STREAM_INFO * lpStreamInfo;
    if (!mStreamInfoTable->Lookup((LONG32) usStreamNumber, (void *&) lpStreamInfo))
    {
	theErr = HXR_INVALID_PARAMETER;
	return theErr;
    }

#if defined(HELIX_FEATURE_RECORDCONTROL)
    if (m_bPlayFromRecordControl && m_pRecordControl)
    {
	IHXPacket* pPacket = NULL;

	HX_ASSERT(m_pRecordControl);

	theErr = m_pRecordControl->GetPacket(usStreamNumber, pPacket);
	if(theErr == HXR_OK)
	{
	    theEvent = new CHXEvent(pPacket,
                                    CalcEventTime(lpStreamInfo,
                                                  pPacket->GetTime(),
                                                  TRUE,
                                                  m_pPlayer->GetVelocity()));
	    if(theEvent) 
		theEvent->SetTimeOffset(m_ulStartTime - m_ulDelay); 
	    else
		theErr = HXR_OUTOFMEMORY;

	    if(m_pBufferManager)
	    {
		m_pBufferManager->UpdateCounters(pPacket, 0);
	    }
            
            if (m_pHXSrcBufStats && !pPacket->IsLost())
            {
                m_pHXSrcBufStats->OnPacket(pPacket);
            }

	    HX_RELEASE(pPacket);
	}
	else
	{
	    if(theErr == HXR_NO_DATA && (m_bSourceEnd || lpStreamInfo->m_bSrcStreamDone))
		theErr = HXR_AT_END;

	    if(theErr == HXR_NO_DATA)
	    {
                theErr = HandleOutOfPackets(lpStreamInfo, 
					    ulLoopEntryTime, 
					    ulProcessingTimeAllowance);
	    }
	}

	return theErr;
    }
#endif /* HELIX_FEATURE_RECORDCONTROL */

    // get the packet list for this stream
    CHXEventList  * lEventList = &lpStreamInfo->m_EventList;

    // do we need to fill buffers...
    if (lEventList->GetNumEvents() == 0)
    {
	theErr = FillBuffers(ulLoopEntryTime,
			     ulProcessingTimeAllowance);

	if (theErr == HXR_AT_END)
	{
	    SetEndOfClip();
	    theErr = HXR_OK;
	}

	// check if we have packets now...
	if (!theErr && lEventList->GetNumEvents() == 0)
	{
	    if (m_bSourceEnd || lpStreamInfo->m_bSrcStreamDone)
	    {
		return HXR_AT_END;	    
	    }
	    else
	    {
                return HandleOutOfPackets(lpStreamInfo, 
					  ulLoopEntryTime, 
					  ulProcessingTimeAllowance);
	    }
	}
    }

    if (!theErr)
    {
	IHXPacket* pPacket;

	// this event will be deleted by the player...
	theEvent = lEventList->RemoveHead();

	pPacket = theEvent->GetPacket();	// Not Addref-ed
        if (pPacket)
        {
	    if (m_pBufferManager)
	    {
		m_pBufferManager->UpdateCounters(pPacket, 0);
	    }

	    if (m_pHXSrcBufStats && !pPacket->IsLost())
	    {
		m_pHXSrcBufStats->OnPacket(pPacket);
	    }
	}
    }

#ifdef LOSS_HACK
    if (m_ulLossHack > 0 && ((UINT32) (rand() % 100) < m_ulLossHack) &&
	!theErr && theEvent && !(theEvent->GetPacket())->IsLost())
    {
	GenerateFakeLostPacket(theEvent);
	/* Update the stats */
	if (lpStreamInfo->m_ulReceived > 0)
	{
	    lpStreamInfo->m_ulReceived--;
	    lpStreamInfo->m_ulLost++;
	}
    }
#endif /* LOSS_HACK */

    return theErr;
}

void 
HXFileSource::ReBuffer(UINT32 ulLoopEntryTime,
		       UINT32 ulProcessingTimeAllowance)
{
    UINT32  ulRemainToBufferInMs = 0;
    UINT32  ulRemainToBuffer = 0;

    m_pBufferManager->GetRemainToBuffer(ulRemainToBufferInMs,
					ulRemainToBuffer);

    if (!ulRemainToBufferInMs && !ulRemainToBuffer)
    {
	m_pBufferManager->ReBuffer(1000);
	FillBuffers(ulLoopEntryTime, ulProcessingTimeAllowance);
    }
}



HXBOOL		
HXFileSource::IsStatisticsReady(void)
{
    return HXR_OK;
}

HX_RESULT		
HXFileSource::UpdateRegistry(UINT32 ulRegistryID)
{
    HX_RESULT	    theErr = HXR_OK;
#if defined(HELIX_FEATURE_STATS) && defined(HELIX_FEATURE_REGISTRY)
    UINT32	    ulRegId = 0;
    char	    szRegName[MAX_DISPLAY_NAME] = {0}; /* Flawfinder: ignore */
    IHXBuffer*	    pParentName = NULL;
    STREAM_INFO*    pStreamInfo = NULL;
    CHXMapLongToObj::Iterator	ndxStream;

    m_ulRegistryID = ulRegistryID;

    if (!m_pStats)
    {
	SetupRegistry();
    }
    else
    {
	if (m_pSourceInfo		    &&
	    m_pSourceInfo->m_bLeadingSource &&
	    !m_pSourceInfo->m_pRepeatList)
	{	
            //Copy over the 'next group' stats.
            SOURCE_STATS* pTmpSourceStats = new SOURCE_STATS((IUnknown*)(IHXPlayer*)m_pPlayer, m_ulRegistryID);
            *pTmpSourceStats = *m_pStats;
            
	    ndxStream = mStreamInfoTable->Begin();
	    for(; ndxStream != mStreamInfoTable->End(); ++ndxStream)
	    {
		pStreamInfo = (STREAM_INFO*) (*ndxStream);
		
		if (m_pRegistry	    && 
		    pTmpSourceStats &&
		    HXR_OK == m_pRegistry->GetPropName(pTmpSourceStats->m_ulRegistryID, pParentName))
		{
		    SafeSprintf(szRegName, MAX_DISPLAY_NAME, "%s.Stream%ld", pParentName->GetBuffer(), 
			    pStreamInfo->m_uStreamNumber);

		    ulRegId = m_pRegistry->GetId(szRegName);
		    if (!ulRegId)
		    {
			ulRegId = m_pRegistry->AddComp(szRegName);
		    }

		    STREAM_STATS* pTmpStreamStats = new STREAM_STATS((IUnknown*)(IHXPlayer*)m_pPlayer, ulRegId);

                    //Copy over the "next-group" stats
                    *pTmpStreamStats = *pStreamInfo->m_pStats;
                    HX_DELETE(pStreamInfo->m_pStats);
                    pStreamInfo->m_pStats = pTmpStreamStats;
		}
		HX_RELEASE(pParentName);
	    }

            HX_DELETE(m_pStats);
            m_pStats = pTmpSourceStats;
	}
	else if (m_pStatsManager)
	{
	    m_pStatsManager->UpdateRegistry(m_ulRegistryID);
	}
    }
#endif /* HELIX_FEATURE_STATS && HELIX_FEATURE_REGISTRY */

    return theErr;
}

HX_RESULT
HXFileSource::UpdateStatistics(void)
{
#if defined(HELIX_FEATURE_STATS) && defined(HELIX_FEATURE_REGISTRY)
    ULONG32	ulSourceLost = 0;	   
    ULONG32	ulSourceTotal = 0;	   
    ULONG32	ulSourceReceived = 0;   
    ULONG32	ulSourceNormal = 0;	      
    ULONG32	ulSourceBandwidth = 0;   
  
    CHXMapLongToObj::Iterator ndxStream = mStreamInfoTable->Begin();	
    for (; ndxStream != mStreamInfoTable->End(); ++ndxStream) 
    {
        STREAM_INFO*	pStreamInfo   = (STREAM_INFO*) (*ndxStream);

        ULONG32		ulStreamTotal = 0;
        ULONG32		ulStreamReceived = 0;
        ULONG32		ulStreamNormal = 0;
    
	// update the statistics in the registry
	ulStreamNormal = pStreamInfo->m_ulReceived;
	ulStreamReceived = ulStreamNormal;
	ulStreamTotal = ulStreamNormal + pStreamInfo->m_ulLost;

	pStreamInfo->m_pStats->m_pNormal->SetInt((INT32)ulStreamNormal);
	pStreamInfo->m_pStats->m_pReceived->SetInt((INT32)ulStreamReceived);				    
	pStreamInfo->m_pStats->m_pLost->SetInt((INT32)pStreamInfo->m_ulLost);
	pStreamInfo->m_pStats->m_pTotal->SetInt((INT32)ulStreamTotal);

	ulSourceTotal += ulStreamTotal;
	ulSourceReceived += ulStreamReceived;
	ulSourceNormal += ulStreamNormal;

	ulSourceLost	+= pStreamInfo->m_ulLost;

	ulSourceBandwidth += pStreamInfo->m_pStats->m_pClipBandwidth->GetInt();
    }

    if (m_pStats->m_pNormal)		m_pStats->m_pNormal->SetInt((INT32)ulSourceNormal);
    if (m_pStats->m_pReceived)		m_pStats->m_pReceived->SetInt((INT32)ulSourceReceived);
    if (m_pStats->m_pTotal)		m_pStats->m_pTotal->SetInt((INT32)ulSourceTotal);
    if (m_pStats->m_pClipBandwidth)	m_pStats->m_pClipBandwidth->SetInt((INT32)ulSourceBandwidth);
    if (m_pStats->m_pLost)		m_pStats->m_pLost->SetInt((INT32)ulSourceLost);    
    if (m_pStats->m_pCurBandwidth)	m_pStats->m_pCurBandwidth->SetInt((INT32)ulSourceBandwidth);
    if (m_pStats->m_pAvgBandwidth)	m_pStats->m_pAvgBandwidth->SetInt((INT32)ulSourceBandwidth);

    // update buffering mode(local machine)
    if (m_pStats->m_pBufferingMode)
    {
	m_pStats->m_pBufferingMode->SetInt(NORMAL_PLAY);
    }
#endif /* HELIX_FEATURE_STATS && HELIX_FEATURE_REGISTRY */
    
    return HXR_OK;
}

HX_RESULT	
HXFileSource::_ProcessIdle(HXBOOL atInterrupt, 
			   UINT32 ulLoopEntryTime, 
			   UINT32 ulProcessingTimeAllowance)
{
    HX_RESULT	theErr = HXR_OK;

    if (m_bLocked)
    {
	return HXR_OK;
    }
    
    if (HandleAudioRebuffer())
    {
        return HXR_OK;
    }

    theErr = _ProcessIdleExt(atInterrupt);
    if (HXR_ABORT == theErr)
    {
	return HXR_OK;
    }

    // need to try the next fileformat for same mimetype...
    // cannot do it at interrupt time!
    if (m_pCurrentFileFormatUnk && 
	(m_bCurrentFileFormatUnkInUse || atInterrupt))
    {
	return HXR_OK;
    }

    m_bLocked	= TRUE;

    if (m_pCurrentFileFormatUnk)
    {
	m_bCurrentFileFormatUnkInUse = TRUE;
	CleanupFileObjects();
	ReSetup();
	m_bLocked = FALSE;
	return HXR_OK;
    }

    if (!m_bInitialized)
    {
        m_bLocked = FALSE;
        return theErr;
    }

    if (m_bRedirectPending && !m_bPartOfNextGroup)
    {
        m_bRedirectPending = FALSE;

        HX_ASSERT(m_pRedirectURL);

	theErr = m_pSourceInfo->HandleRedirectRequest();
        HX_DELETE(m_pRedirectURL);

        m_bLocked = FALSE;
        return theErr;
    }

    // we keep RAM FF object around by AddRef() in ::InitFileFormat
    // to avoid self-destruct in ::InitDone when it's not a RAM 
    HX_RELEASE(m_pRAMFFObject);

    if (!theErr)
    {
	UINT32 ulCurrentTime = m_pPlayer->GetInternalCurrentPlayTime();

	if (m_bPaused && m_bDelayed)
	{
	    if (TryResume())
	    {
		m_pPlayer->RegisterSourcesDone();
		DoResume(ulLoopEntryTime, ulProcessingTimeAllowance);
	    }
	}

	// set m_bDelayed to FALSE as soon as it's due to begin playback
	// even it's still in pre-fetch mode
	if (!m_bPaused && m_bDelayed)
	{
	    if (IsPassedResumeTime(NETWORK_FUDGE_FACTOR))
	    {
		m_bDelayed = FALSE;
	    }
	}
	
	if  (!m_bSourceEnd && m_bIsPreBufferingStarted && !m_bIsPreBufferingDone)
	{
	    /* Get Current buffering status every 1 sec. */
	    UINT32 ulCurrentSystemTime = HX_GET_TICKCOUNT();

	    if (CALCULATE_ELAPSED_TICKS(m_ulLastBufferingCalcTime, 
					ulCurrentSystemTime) > 1000)
	    {
		m_ulLastBufferingCalcTime = ulCurrentSystemTime;
		CalculateCurrentBuffering();
	    }
	}

	// XXX HP TBD HTTP prefetch
	// no prefetch support for now
	if (m_bPrefetch)
	{
	    LeavePrefetch();
	}

        CheckForInitialPrerollRebuffer(ulCurrentTime);
    }

    m_bLocked = FALSE;

    if (!theErr && !m_bIsActive && !m_bDelayed &&
	m_pPlayer->GetInternalCurrentPlayTime() >= m_ulDelay)
    {
	AdjustClipBandwidthStats(TRUE);
    }

    /* tell the player about the error...
     * This is crucial...
     */
    if (theErr)
    {
	ReportError(theErr);
    }

    return theErr;
}

HX_RESULT
HXFileSource::_ProcessIdleExt(HXBOOL atInterrupt)
{
    return HXR_OK;
}

STDMETHODIMP
HXFileSource::InitDone
(
    HX_RESULT		status
)
{
    if (!m_bValidateMetaDone && !m_pFFClaimURLEnumerator)
    {
	m_bValidateMetaDone = TRUE;

	if (HXR_OK == status)
	{
	    m_bIsMeta = TRUE;
	    // reset all the timing attributes since meta file(RAM)
	    // doesn't support any
	    SetPlayTimes(0, 0, 0, 0);
	}
	else
	{
	    m_bIsMeta = FALSE;
	    HX_RELEASE(m_pFFObject);
	    HX_RELEASE(m_pCurrentFileFormatUnk);
	    HX_RELEASE(m_pFileFormatEnumerator);
	    
	    // not a meta file, since the file object has been initialized,
	    // calling ExtendedSetup() again to find the actual mimetype if
	    // the file object supports findings of mimetypes.
	    return ExtendedSetup(m_pszURL);
	}
    }

    if (!mLastError && status != HXR_OK)
    {
	CheckForDefaultUpgrade(status);

	/* do we need to try the next one! */
	if (!m_pCurrentFileFormatUnk && m_pFileFormatEnumerator)
	{
            m_pFileFormatEnumerator->GetNextPlugin(m_pCurrentFileFormatUnk, NULL);
	    if (m_pCurrentFileFormatUnk && m_pSourceInfo)
	    {
		m_pSourceInfo->ScheduleProcessCallback();
	    }
	}	

	// early exit to try next fileformat for the same mimetype!
	if (m_pCurrentFileFormatUnk)
	{
	    return HXR_OK;
	}

	mLastError = status;
	// merge any upgrade requests for this source to the player
	MergeUpgradeRequest(m_bAddDefaultUpgrade, m_pDefaultUpgradeString);
    }

    if (HXR_OK == status && m_ulStreamHeadersExpected == 0)
    {
	HX_RELEASE(m_pBackChannel);
	HX_RELEASE(m_pASMSource);
	m_pFFObject->QueryInterface(IID_IHXBackChannel, (void**) &m_pBackChannel);
	m_pFFObject->QueryInterface(IID_IHXASMSource, (void**) &m_pASMSource);

	HX_RESULT result = m_pFFObject->GetFileHeader();
    	return result;
    }
    else 
    {
	ReportError(status);
    }

    return HXR_OK;
}

STDMETHODIMP
HXFileSource::FileHeaderReady
(
    HX_RESULT	    status,
    IHXValues*	    pHeader
)
{
    HX_RELEASE(m_pFileHeader);

    if (HXR_OK == status)
    {
	status = HandleSDPData(pHeader);
    }

#if defined(HELIX_FEATURE_DRM)
    m_bIsProtected = IsHelixDRMProtected(pHeader);
    if (IsHelixDRMProtected())
    {
        status = InitHelixDRM(pHeader);

        if (SUCCEEDED(status) && m_pDRM)
        {
            m_pDRM->FileHeaderHook(pHeader);
        }
    }
#endif /*HELIX_FEATURE_DRM*/

#if defined(HELIX_FEATURE_RECORDCONTROL)
    SendHeaderToRecordControl(TRUE, pHeader);
#endif /* HELIX_FEATURE_RECORDCONTROL */

    return ContinueWithFileHeader(status, pHeader);
}

HX_RESULT
HXFileSource::ContinueWithFileHeader(HX_RESULT status, IHXValues* pHeader)
{
    HX_RESULT 	    result = HXR_OK;

    HX_ASSERT(!m_pFileHeader);

    if(!m_pFileHeader && pHeader)
    {
	m_pFileHeader = pHeader;
	m_pFileHeader->AddRef();
    }

    result = ContinueWithFileHeaderExt(status, m_pFileHeader);
    if (result == HXR_REQUEST_UPGRADE)
    {
	mLastError = result;
	return HXR_OK;
    }
    else if (result == HXR_WOULD_BLOCK)
    {
	return HXR_OK;
    }

    m_bContinueWithHeaders = FALSE;

    if (status != HXR_OK)
    {
	CheckForDefaultUpgrade(status);
	/* do we need to try the next one! */
	if (!m_pCurrentFileFormatUnk && m_pFileFormatEnumerator)
	{
	    m_pFileFormatEnumerator->GetNextPlugin(m_pCurrentFileFormatUnk, NULL);
	    // only proceed source info(renderer) initialization in the 
	    // CURRENT group so that sources in the NEXT group won't have
	    // effect(i.e. layout) in the current playback
	    if (m_pCurrentFileFormatUnk && 
		m_pPlayer		&&
		m_pSourceInfo		&&
		m_pPlayer->GetCurrentGroupID() == m_pSourceInfo->m_uGroupID)
	    {
		m_pSourceInfo->ScheduleProcessCallback();
	    }
	}	

	// early exit to try next fileformat for the same mimetype!
	if (m_pCurrentFileFormatUnk)
	{
	    return HXR_OK;
	}

    	mLastError = status;
	// merge any upgrade requests for this source to the player
	MergeUpgradeRequest(m_bAddDefaultUpgrade, m_pDefaultUpgradeString);
	
	ReportError(mLastError);
	return HXR_FAILED;
    }

#if defined(HELIX_FEATURE_DRM)
    if (IsHelixDRMProtected() && m_pDRM)
    {
        //DRM OnFileHeader callback will call ProcessFileHeader later
	return m_pDRM->ProcessFileHeader(pHeader);
    }
#endif /*HELIX_FEATURE_DRM*/

    return ProcessFileHeader();
}


HX_RESULT
HXFileSource::ContinueWithFileHeaderExt(HX_RESULT status, IHXValues* pHeader)
{
    return HXR_OK;
}

HX_RESULT
HXFileSource::ProcessFileHeader(void)
{
    HX_RESULT result;

    result = HXSource::ProcessFileHeader();

    if (SUCCEEDED(result))
    {
        if(m_pFileHeader)
        {
	    UINT32 ulLiveStream = 0;
	    m_pFileHeader->GetPropertyULONG32("LiveStream", ulLiveStream);
	    mLiveStream = ulLiveStream ? TRUE : FALSE;

	    ULONG32 ulFlags = 0;
	    m_pFileHeader->GetPropertyULONG32("Flags", ulFlags);
	    mSaveAsAllowed = ulFlags & HX_SAVE_ENABLED ? TRUE : FALSE;

            // Check the "PacketlessSource" property
            UINT32 ulTmp = 0;
            m_pFileHeader->GetPropertyULONG32("PacketlessSource", ulTmp);
            m_bPacketlessSource = (ulTmp ? TRUE : FALSE);

	    for (UINT16 i=0; (result == HXR_OK) && i < m_ulStreamHeadersExpected; i++)
	    {
	        result = m_pFFObject->GetStreamHeader(i);
	    }
        }
        else
        {
	    result = HXR_FAILED;
        }
    }
    return result;
}


STDMETHODIMP	
HXFileSource::StreamHeaderReady(HX_RESULT status, IHXValues* pHeader)
{
    HX_RESULT	    theErr = HXR_OK;
    STREAM_INFO*    pStreamInfo = NULL;

    // we do not support receiving headers once we have started getting data...
    if (m_bReceivedData)
	return HXR_FAILED;	// define some more appropriate error code..

    if (HXR_OK == status)
    {
	status = HandleSDPData(pHeader);
    }

    if (HXR_OK != status)
    {
	mLastError = status;
    	ReportError(mLastError);
	return HXR_OK;
    }

#if defined(HELIX_FEATURE_DRM)
    if (SUCCEEDED(status) && IsHelixDRMProtected() && m_pDRM)
    {
        status = m_pDRM->StreamHeaderHook(pHeader);
    }
#endif /*HELIX_FEATURE_DRM*/

#if defined(HELIX_FEATURE_RECORDCONTROL)
    SendHeaderToRecordControl(FALSE, pHeader);
#endif /* HELIX_FEATURE_RECORDCONTROL */

    StreamHeaderReadyExt(pHeader);

    // fileformat initialized...clear any pending upgrade requests for this source!
    ClearUpgradeRequest();

    // we have already received enough headers...
    if (m_uNumStreams >= m_ulStreamHeadersExpected)
	return HXR_FAILED;


#if defined(HELIX_FEATURE_DRM)
    if (IsHelixDRMProtected() && m_pDRM)
    {
        //DRM OnStreamHeader callback will call ProcessStreamHeaders later
        return m_pDRM->ProcessStreamHeader(pHeader);
    }
#endif /*HELIX_FEATURE_DRM*/

    return ProcessStreamHeaders(pHeader, pStreamInfo);
}

HX_RESULT
HXFileSource::ProcessStreamHeaders(IHXValues* pHeader, STREAM_INFO*& pStreamInfo)
{
    HX_RESULT	    theErr = HXR_OK;

    theErr = HXSource::ProcessStreamHeaders(pHeader, pStreamInfo);

    if (!theErr)
    {
#if defined(HELIX_FEATURE_STATS) && defined(HELIX_FEATURE_REGISTRY)
    char	    szRegKeyName[MAX_DISPLAY_NAME] = {0}; /* Flawfinder: ignore */
	IHXBuffer*	    pszParentName = NULL;
        HX_ASSERT(pStreamInfo);

	// create all the statistic registry keys
	if (m_pRegistry && m_pStats &&
	    HXR_OK == m_pRegistry->GetPropName(m_pStats->m_ulRegistryID, pszParentName))
	{
	    SafeSprintf(szRegKeyName, MAX_DISPLAY_NAME, "%s.Stream%ld", pszParentName->GetBuffer(), 
		m_ulStreamIndex);

	    /* does this ID already exists ? */
	    UINT32 ulRegistryID = m_pRegistry->GetId(szRegKeyName);
	    if (!ulRegistryID)
	    {
		ulRegistryID = m_pRegistry->AddComp(szRegKeyName);
	    }

	    pStreamInfo->m_pStats = new STREAM_STATS((IUnknown*)(IHXPlayer*)m_pPlayer, ulRegistryID);

            if(pStreamInfo->m_pStats)
            {
	        // set stream bandwidth
	        pStreamInfo->m_pStats->m_pClipBandwidth->SetInt((INT32)pStreamInfo->BufferingState().AvgBandwidth());
            }
            else
            {
                // NOTE: It may be that we can still function without the 
                // m_pStats object, but can we still function if we are 
                // running out of memory at this point? Assuming no.
                theErr = HXR_OUTOFMEMORY;
            }

	}
	HX_RELEASE(pszParentName);
#endif /* HELIX_FEATURE_STATS && HELIX_FEATURE_REGISTRY */

        if (pStreamInfo && m_pHXSrcBufStats)
        {
            m_pHXSrcBufStats->InitStream(pStreamInfo->m_uStreamNumber,
                                         mLiveStream);
        }

        m_ulStreamIndex++;
        m_uNumStreams++;
    }

    if (!theErr && m_uNumStreams == m_ulStreamHeadersExpected)
    {
        // We successfully found a file format and successfully
        // retrieved a file header and all the stream headers.
        // Therefore, we no longer need the enumerators
        if (m_pFFClaimURLEnumerator)
        {
            HX_RELEASE(m_pFileFormatEnumerator);
        }
        HX_RELEASE(m_pFFClaimURLEnumerator);

        m_uActiveStreams = m_uNumStreams;
	m_ulOriginalDuration = m_ulDuration;
        m_bRARVSource = IsRARVSource();
        m_bInitialized = TRUE;	

	theErr = AdjustClipTime();
	m_pBufferManager->Init();

        if (m_pHXSrcBufStats)
        {
            SetSrcBufStats(m_pHXSrcBufStats);
        }
    }

    return theErr;
}

HX_RESULT
HXFileSource::StreamHeaderReadyExt(IHXValues* pHeader)
{
    return HXR_OK;
}

STDMETHODIMP	
HXFileSource::PacketReady(HX_RESULT status, IHXPacket* pPacket)
{
    HXLOGL4(HXLOG_TRIK, "FS PacketReady(0x%08x,) strm=%u pts=%lu rule=%u flags=0x%02x playtime=%lu",
            status,
            (pPacket ? pPacket->GetStreamNumber() : 0),
            (pPacket ? pPacket->GetTime() : 0),
            (pPacket ? pPacket->GetASMRuleNumber() : 0),
            (pPacket ? pPacket->GetASMFlags() : 0),
            m_pPlayer->GetInternalCurrentPlayTime());
    HXLOGL4(HXLOG_CORP, "HXPlayer[%p]::::HXFileSource[%p]::PacketReady(0x%08x,) strm=%u pts=%lu rule=%u flags=0x%02x playtime=%lu",
	    m_pPlayer,
	    this,
            status,
            (pPacket ? pPacket->GetStreamNumber() : 0),
            (pPacket ? pPacket->GetTime() : 0),
            (pPacket ? pPacket->GetASMRuleNumber() : 0),
            (pPacket ? pPacket->GetASMFlags() : 0),
            m_pPlayer->GetInternalCurrentPlayTime());
    HX_RESULT theErr = HXR_OK;

    // We should have been initialized by now
    HX_ASSERT(m_bInitialized);
    if (!m_bInitialized)
    {
	return HXR_NOT_INITIALIZED;
    }
    
    // Report a non-HXR_OK error code only if no packet. If a packet
    // accompanies the error code we assume end of stream and handle
    // later on (see below).
    if (!pPacket)
    {
        if (HXR_OK != status)
        {
	    mLastError = status;
	    ReportError(mLastError);
            return HXR_OK;
        }
        else
        {
	    // HXR_OK with a NULL packet makes no sense
	    return HXR_INVALID_PARAMETER;
        }
    }
    
    IHXBuffer*	    pBuffer	    = 0;
    UINT32	    ulPacketTime    = 0;	    // packet time encoded
    UINT32	    ulPacketFilledDuration = 0;	    // packets' time have been filled 
    INT64	    llActualPacketTime = 0;	    // packet time with timestamp rollover
    UINT16	    uStreamNumber   = 0;
    UINT8	    unASMFlags	    = 0;
    UINT16	    unASMRuleNumber = 0;

    if (HXR_OK != pPacket->Get(pBuffer, ulPacketTime, uStreamNumber, unASMFlags, unASMRuleNumber))
    {
	theErr = HXR_FAILED;
	return theErr;
    }

    HX_RELEASE(pBuffer);

    CHXEvent*	    theEvent	    = NULL;
    STREAM_INFO*    lpStreamInfo    = NULL;
    CHXEventList*  lEventList	    = NULL;

    if (!mStreamInfoTable->Lookup((LONG32) uStreamNumber, (void *&) lpStreamInfo))
    {
	return HXR_INVALID_PARAMETER;
    }

    if (status != HXR_OK)
    {
	if (!lpStreamInfo->m_bSrcStreamDone)
	{
	    // if the status is not OK, it is probably the end of the stream..
	    // we should mark this stream as DONE...
	    lpStreamInfo->m_bPacketRequested	    = FALSE;
	    lpStreamInfo->m_bSrcStreamDone	    = TRUE;
	    
	    if (m_uActiveStreams > 0)
	    {
		m_uActiveStreams--;
	    }
	    
	    if (m_uActiveStreams == 0)
	    {
		SetEndOfClip();
	    }
	}

	return HXR_OK;
    }

    lpStreamInfo->m_ulReceived++;

    if (!theErr)
    {
	// reset
	lpStreamInfo->m_bPacketRequested    = FALSE;
	lEventList = &lpStreamInfo->m_EventList;
    }

    /*
     * Save off the initial timestamp for live streams so we can buffer
     * relative to the first timstamp
     */
    if (m_bInitialPacket)
    {
	m_bInitialPacket = FALSE;
	m_ulFirstPacketTime = ulPacketTime;
    }

    lpStreamInfo->m_ulLastPacketTime = ulPacketTime - m_ulStartTime;

#if defined(HELIX_FEATURE_RECORDCONTROL)
    if (m_pRecordControl)
    {
	m_pRecordControl->OnPacket(pPacket, m_ulStartTime - m_ulDelay);
    }
#endif /* HELIX_FEATURE_RECORDCONTROL */

    if (!m_bPlayFromRecordControl)
    {
	theEvent = new CHXEvent(pPacket,
                                CalcEventTime(lpStreamInfo,
                                              ulPacketTime,
                                              TRUE,
                                              m_pPlayer->GetVelocity()));
	if(!theEvent) 
	{
	    theErr = HXR_OUTOFMEMORY;
	}

	// enqueue the event into event queue
	if(!theErr) 
	{
    	    /* offset used by Player::ProcessCurrentEvents to send to renderer::OnPacket */
    	    theEvent->SetTimeOffset(m_ulStartTime - m_ulDelay); 

	    HXLOGL4(HXLOG_CORP, "HXPlayer[%p]::::HXFileSource[%p]::PacketReady: InsertEvent: EventTime=%lu EventOffset=%lu Packet=(strm=%u pts=%lu rule=%u flags=0x%02x) PlayTime=%lu",
		    m_pPlayer,
		    this,
		    theEvent->GetTimeStartPos(),
		    theEvent->GetTimeOffset(),
		    (pPacket ? pPacket->GetStreamNumber() : 0),
		    (pPacket ? pPacket->GetTime() : 0),
		    (pPacket ? pPacket->GetASMRuleNumber() : 0),
		    (pPacket ? pPacket->GetASMFlags() : 0),
		    m_pPlayer->GetInternalCurrentPlayTime());

	    theErr = lEventList->InsertEvent(theEvent);
	}
    }

    if (!theErr)
    {
	m_bReceivedData = TRUE;
    }
    else if (theEvent)
    {
        HX_DELETE(theEvent);
    }

    return theErr;
}


STDMETHODIMP	
HXFileSource::StreamDone(UINT16   uStreamNumber)
{
    STREAM_INFO	   *lpStreamInfo    = NULL;

    if (!mStreamInfoTable->Lookup((LONG32) uStreamNumber, (void *&) lpStreamInfo))
    {
	return HXR_INVALID_PARAMETER;
    }

    if (!lpStreamInfo->m_bSrcStreamDone)
    {
	lpStreamInfo->m_bSrcStreamDone		= TRUE;
	lpStreamInfo->m_bSrcStreamFillingDone	= TRUE;
	lpStreamInfo->m_bPacketRequested	= FALSE;

	if (m_uNumStreamsToBeFilled > 0)
	{
	    m_uNumStreamsToBeFilled--;
	}

	if (m_uActiveStreams > 0)
	{
	    m_uActiveStreams--;
	}
    
	if (m_uActiveStreams == 0)
	{
	    SetEndOfClip();
	}
    }

    return HXR_OK;
}

STDMETHODIMP	
HXFileSource::SeekDone(HX_RESULT status)
{
    HX_RESULT lResult = HXR_OK;
    
    if (m_nSeeking > 0)
    {
	m_nSeeking--;
    }

    _ProcessIdle(FALSE);

    return lResult;
}

STDMETHODIMP HXFileSource::RedirectDone(IHXBuffer* pURL)
{
    HX_RESULT	hr = HXR_NOTIMPL;

    // handle redirect to diff. protocol from HTTP file system
    if (m_pszURL && pURL)
    {
	if (strncasecmp(m_pszURL, "http://", 7) == 0 &&
	    strncasecmp((const char*)pURL->GetBuffer(), "http://", 7) != 0)
	{
            HX_DELETE(m_pRedirectURL);
            m_pRedirectURL = new CHXURL((const char*)pURL->GetBuffer(),
                                        (IHXClientEngine*)m_pEngine);

            if (m_bPartOfNextGroup)
            {
                m_bRedirectPending = TRUE;
            }
            else
            {
 	        hr = m_pSourceInfo->HandleRedirectRequest();
            }
	}
    }
    
    return hr;
}



#if defined(HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS)
/*
 *  Helper methods that are used by IHXPDStatusMgr methods, below
 */        
HX_RESULT
HXFileSource::EstablishPDSObserverList()
{
    HX_RESULT  hxrslt = HXR_OK;

    if (!m_pPDSObserverList)
    {
        m_pPDSObserverList = new CHXSimpleList();

        if (!m_pPDSObserverList)
        {
            hxrslt = HXR_OUTOFMEMORY;
        }
    }

    return hxrslt;
}

/*
 *  IHXPDStatusMgr methods
 */        

/************************************************************************
 *  Method:
 *      IHXPDStatusMgr::AddObserver
 *  Purpose:
 *      Lets an observer register so it can be notified of file changes
 */
STDMETHODIMP
HXFileSource::AddObserver(IHXPDStatusObserver* /*IN*/ pObserver)
{
    HX_RESULT hxrslt = HXR_INVALID_PARAMETER;
    if (pObserver)
    {
        if (!m_pPDSObserverList)
        {
            hxrslt = EstablishPDSObserverList();
        }
        if (SUCCEEDED(hxrslt)  &&  m_pPDSObserverList)
        {
            pObserver->AddRef();
            m_pPDSObserverList->AddTail(pObserver);
        }
    }

    return hxrslt;
}

/************************************************************************
 *  Method:
 *      IHXPDStatusMgr::RemoveObserver
 *  Purpose:
 *      Lets an observer unregister so it can stop being notified of
 *      file changes
 */
STDMETHODIMP
HXFileSource::RemoveObserver(IHXPDStatusObserver* /*IN*/ pObserver)
{
    HX_RESULT hxrslt = HXR_INVALID_PARAMETER;
    if (pObserver)
    {
        hxrslt = HXR_FAIL;
        if (m_pPDSObserverList)
        {
            LISTPOSITION lPosition = m_pPDSObserverList->Find(pObserver);
            if (lPosition)
            {
                m_pPDSObserverList->RemoveAt(lPosition);
                HX_RELEASE(pObserver);
            }

            hxrslt = HXR_OK;
        }
    }
    return hxrslt;
}

/************************************************************************
 *  Method:
 *      IHXPDStatusMgr::SetStatusUpdateGranularityMsec
 *  Purpose:
 *      Lets an observer set the interval that the reporter (fsys) takes
 *      between status updates.  Value is in milliseconds.
 */
STDMETHODIMP
HXFileSource::SetStatusUpdateGranularityMsec(
        UINT32 /*IN*/ ulStatusUpdateGranularityInMsec)
{
    HX_RESULT hxrslt = HXR_FAIL;
    if (m_pFFObject)
    {

        IHXPDStatusMgr* pPDStatusMgr = NULL;
        m_pFFObject->QueryInterface(IID_IHXPDStatusMgr, (void**)&pPDStatusMgr);
        if (pPDStatusMgr)
        {
            hxrslt = pPDStatusMgr->SetStatusUpdateGranularityMsec(
                    ulStatusUpdateGranularityInMsec); 
        }
        HX_RELEASE(pPDStatusMgr);
    }

    return hxrslt;
}

/*
 *  IHXPDStatusObserver methods
 */

/************************************************************************
 *  Method:
 *      IHXPDStatusObserver::OnDownloadProgress
 *
 *  Purpose:
 *      Notification from IHXPDStatusMgr of download progress when
 *      file size changes.
 *
 *      lTimeSurplus is calculated farther up the chain (HXPlayer) and is
 *      thus passed as an invalid value HX_PROGDOWNLD_UNKNOWN_TIME_SURPLUS
 *      from here.
 *
 *      Note: ulNewDurSoFar can be HX_PROGDOWNLD_UNKNOWN_DURATION if the
 *      IHXMediaBytesToMediaDur was not available to, or was unable to
 *      convert the bytes to a duration for the IHXPDStatusMgr calling this:
 */
STDMETHODIMP
HXFileSource::OnDownloadProgress(
             IHXStreamSource* /*IN*/ /*NULL is valid value*/ pStreamSource,
             UINT32 /*IN*/ ulNewDurSoFar,
             UINT32 /*IN*/ ulNewBytesSoFar,
             INT32  /*IN*/ lTimeSurplus)
{
    if (m_pPDSObserverList)
    {
        LISTPOSITION lPos = m_pPDSObserverList->GetHeadPosition();
        while (lPos)
        {
            HX_ASSERT(m_pPDSObserverList->GetCount() > 0);
            IHXPDStatusObserver* pObserver = (IHXPDStatusObserver*)
                    m_pPDSObserverList->GetNext(lPos);
            if (pObserver)
            {
                // /Pass what we observed along to our observers:
                pObserver->OnDownloadProgress(pStreamSource,
                        ulNewDurSoFar, ulNewBytesSoFar,
                        lTimeSurplus);
            }
        }
    }
    return HXR_OK;
}

/************************************************************************
 *  Method:
 *      IHXPDStatusObserver::OnTotalDurChanged
 *  Purpose:
 *      This is a notification if the total file duration becomes known
 *      or becomes better-known during download/playback
 *      
 *      Note: pStreamSource can be NULL.  This will be true when
 *      IHXPDStatusMgr calling this is upstream of the IHXStreamSource
 *      object.
 */
STDMETHODIMP
HXFileSource::OnTotalDurChanged(
            IHXStreamSource* /*IN*/ /*NULL is valid value*/ pStreamSource,
            UINT32 ulNewDur)
{
    if (m_pPDSObserverList)
    {
        LISTPOSITION lPos = m_pPDSObserverList->GetHeadPosition();
        while (lPos)
        {
            HX_ASSERT(m_pPDSObserverList->GetCount() > 0);
            IHXPDStatusObserver* pObserver = (IHXPDStatusObserver*)
                    m_pPDSObserverList->GetNext(lPos);
            if (pObserver)
            {
                HXBOOL bReleaseSSWhenDone = FALSE;
                if (NULL == pStreamSource)
                {
                    // /Use ourself (which we should probably always do):
                    QueryInterface(IID_IHXStreamSource, (void**)&pStreamSource);
                    bReleaseSSWhenDone = TRUE;
                }
                // /Pass what we observed along to our observers:
                pObserver->OnTotalDurChanged(pStreamSource, ulNewDur);
                if (bReleaseSSWhenDone)
                {
                    HX_RELEASE(pStreamSource);
                }
            }
        }
    }
    return HXR_OK;
}

/************************************************************************
 *  Method:
 *      IHXPDStatusObserver::OnDownloadComplete
 *
 *  Purpose:
 *      Notification that the entire file has been downloaded.
 *
 *      Note: pStreamSource can be NULL.  This will be true when
 *      IHXPDStatusMgr calling this is upstream of the IHXStreamSource
 *      object.
 *      
 */
STDMETHODIMP
HXFileSource::OnDownloadComplete(
            IHXStreamSource* /*IN*/ /*NULL is valid value*/ pStreamSource)
{
    if (m_pPDSObserverList)
    {
        LISTPOSITION lPos = m_pPDSObserverList->GetHeadPosition();
        while (lPos)
        {
            HX_ASSERT(m_pPDSObserverList->GetCount() > 0);
            IHXPDStatusObserver* pObserver = (IHXPDStatusObserver*)
                    m_pPDSObserverList->GetNext(lPos);
            if (pObserver)
            {
                HXBOOL bReleaseSSWhenDone = FALSE;
                if (NULL == pStreamSource)
                {
                    // /Use ourself (which we should probably always do):
                    QueryInterface(IID_IHXStreamSource, (void**)&pStreamSource);
                    bReleaseSSWhenDone = TRUE;
                }
                // /Pass what we observed along to our observers:
                pObserver->OnDownloadComplete(pStreamSource);
                if (bReleaseSSWhenDone)
                {
                    HX_RELEASE(pStreamSource);
                }
            }
        }
    }
    return HXR_OK;
}


/************************************************************************
 *  Method:
 *      IHXPDStatusObserver::SrcClaimsSeekSupport         ref: hxprdnld.h
 *  Purpose:
 *      Passes along notification from file sys that seek support
 *      is or is not claimed to be available (although sometimes HTTP
 *      server claims this when it doesn't actually support it).
 */
STDMETHODIMP
HXFileSource::SrcClaimsSeekSupport(IHXStreamSource* pStreamSource,
                                   HXBOOL bClaimsSupport)
{
    if (m_pPDSObserverList)
    {
        LISTPOSITION lPos = m_pPDSObserverList->GetHeadPosition();
        while (lPos)
        {
            HX_ASSERT(m_pPDSObserverList->GetCount() > 0);
            IHXPDStatusObserver* pObserver = (IHXPDStatusObserver*)
                    m_pPDSObserverList->GetNext(lPos);
            if (pObserver)
            {
                // /Pass what we observed along to our observers:
                pObserver->SrcClaimsSeekSupport(pStreamSource,
                        bClaimsSupport);
            }
        }
    }
    return HXR_OK;
}


/************************************************************************
 *  Method:
 *      IHXPDStatusObserver::OnDownloadPause
 *  Purpose:
 *      Notification that the file-download process has purposefully
 *      and temporarily halted downloading of the file
 *      
 *      Note: pStreamSource can be NULL.  This will be true when
 *      IHXPDStatusMgr calling this is upstream of the IHXStreamSource
 *      object.
 */
STDMETHODIMP
HXFileSource::OnDownloadPause(
            IHXStreamSource* /*IN*/ /*NULL is valid value*/ pStreamSource)
{
    if (m_pPDSObserverList)
    {
        /* Send Begins to all the scheduled renderers */
        LISTPOSITION lPos = m_pPDSObserverList->GetHeadPosition();

        while (lPos)
        {
            HX_ASSERT(m_pPDSObserverList->GetCount() > 0);
            IHXPDStatusObserver* pObserver = (IHXPDStatusObserver*)
                    m_pPDSObserverList->GetNext(lPos);
            if (pObserver)
            {
                HXBOOL bReleaseSSWhenDone = FALSE;
                if (NULL == pStreamSource)
                {
                    // /Use ourself (which we should probably always do):
                    QueryInterface(IID_IHXStreamSource, (void**)&pStreamSource);
                    bReleaseSSWhenDone = TRUE;
                }
                // /Pass what we observed along to our observers:
                pObserver->OnDownloadPause(pStreamSource);
                if (bReleaseSSWhenDone)
                {
                    HX_RELEASE(pStreamSource);
                }
            }
        }
    }
    return HXR_OK;
}

/************************************************************************
 *  Method:
 *      IHXPDStatusObserver::OnDownloadResume
 *  Purpose:
 *      Notification that the file-download process has resumed
 *      the process of downloading the remainder of the file
 *      
 *      Note: pStreamSource can be NULL.  This will be true when
 *      IHXPDStatusMgr calling this is upstream of the IHXStreamSource
 *      object.
 */
STDMETHODIMP
HXFileSource::OnDownloadResume(
            IHXStreamSource* /*IN*/ /*NULL is valid value*/ pStreamSource)
{
    if (m_pPDSObserverList)
    {
        LISTPOSITION lPos = m_pPDSObserverList->GetHeadPosition();
        while (lPos)
        {
            HX_ASSERT(m_pPDSObserverList->GetCount() > 0);
            IHXPDStatusObserver* pObserver = (IHXPDStatusObserver*)
                    m_pPDSObserverList->GetNext(lPos);
            if (pObserver)
            {
                HXBOOL bReleaseSSWhenDone = FALSE;
                if (NULL == pStreamSource)
                {
                    // /Use ourself (which we should probably always do):
                    QueryInterface(IID_IHXStreamSource, (void**)&pStreamSource);
                    bReleaseSSWhenDone = TRUE;
                }
                // /Pass what we observed along to our observers:
                pObserver->OnDownloadResume(pStreamSource);
                if (bReleaseSSWhenDone)
                {
                    HX_RELEASE(pStreamSource);
                }
            }
        }
    }
    return HXR_OK;
}
#endif // /HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS.

STDMETHODIMP HXFileSource::FindFileFormat(REF(IHXFileFormatObject*) rpFileFormat)
{
    HX_RESULT retVal = HXR_FAIL;

    HX_ASSERT(rpFileFormat == NULL);

    if (m_pFFObject)
    {
        rpFileFormat = m_pFFObject;
        rpFileFormat->AddRef();
        retVal = HXR_OK;
    }

    return retVal;
}

// Fill buffer takes action of the file format to obtain a media packet
// for every stream in the source.  When all streams have at least one
// packet buffered, the buffers are considered filled at this level.
HX_RESULT    
HXFileSource::FillBuffers(UINT32 ulLoopEntryTime, UINT32 ulProcessingTimeAllowance)
{
    HX_RESULT status;
    STREAM_INFO* lpStreamInfo = NULL;
    HX_RESULT retVal = HXR_OK;

    HXLOGL4(HXLOG_TRIK, "FS FillBuffers()");
    HXLOGL4(HXLOG_CORP, "HXPlayer[%p]::::HXFileSource[%p]::FillBuffers Start",
			m_pPlayer,
			this);

    /* 
     * Do not call GetPacket if we have not yet received SeekDone from
     * the file format plugin.
     *	    || 
     * We are in a paused state and no rebuffering and no fast start 
     * is required.
     *	    ||
     * We are in force end mode such as seeking to the end of the source duration
     */
    if (m_nSeeking > 0 ||
	(m_bPaused && !m_bFastStartInProgress && 
        (REBUFFER_REQUIRED != m_rebufferStatus) && !m_pRecordControl) ||
	m_bForcedSourceEnd
#if defined(HELIX_FEATURE_RECORDCONTROL)
	|| (m_pRecordControl && (!m_pRecordControl->CanAcceptPackets()))
#endif	// HELIX_FEATURE_RECORDCONTROL
       )
    {
	return HXR_OK;
    }

    // If we need to limit this processing and processing time allowance is not set,
    // acquire the processing time allowance from the player.
    if ((ulLoopEntryTime != 0) && (ulProcessingTimeAllowance == 0))
    {
	// Use the most generous - non-interrupt type allowance.
	ulProcessingTimeAllowance = m_pPlayer->GetPlayerProcessingInterval(FALSE);
    }

    while (SUCCEEDED(retVal) && SelectNextStreamToFill(lpStreamInfo))
    {
	HX_ASSERT(!lpStreamInfo->m_bPacketRequested);

	lpStreamInfo->m_bPacketRequested = TRUE;

	HXLOGL4(HXLOG_CORP, "HXPlayer[%p]::::HXFileSource[%p]::FillBuffers::GetPacket Strm=%hu",
			m_pPlayer,
			this,
			lpStreamInfo->m_uStreamNumber);

	status = m_pFFObject->GetPacket((UINT16) lpStreamInfo->m_uStreamNumber);
	if (status != HXR_OK)
	{
	    StreamDone(lpStreamInfo->m_uStreamNumber);
	    // Don't lose OOM errors.
	    if (status == HXR_OUTOFMEMORY)
	    {
		retVal = status;
	    }
#ifdef HELIX_FEATURE_STOP_STREAM_BY_TEMP_FILE_ERROR
	    if (status == HXR_TEMP_FILE)
	    {
		retVal = status;
		m_pBufferManager->Stop();
		SetEndOfClip();
		mLastError = retVal;
		ReportError(mLastError);
	    }
#endif //HELIX_FEATURE_STOP_STREAM_BY_TEMP_FILE_ERROR
	}

	// For now, we'll assume processing limit at non-interrupt time which is less restrictive.
	// If needed, this could be constrained further for interrupt time by passing appropriate
	// input parameter to indicate execution from interrupt event chain or from an interrupt
	// thread.
	if ((ulLoopEntryTime != 0) &&
	    ((HX_GET_BETTERTICKCOUNT() - ulLoopEntryTime) > ulProcessingTimeAllowance))
	{
	    HXLOGL3(HXLOG_CORP, "HXPlayer[%p]::HXFileSource[%p]::FillBuffers() CPU use timeout: Time=%lu Allowed=%lu", 
		    m_pPlayer, 
		    this, 
		    HX_GET_BETTERTICKCOUNT() - ulLoopEntryTime,
		    ulProcessingTimeAllowance);
	    break;
	}
    }

    HXLOGL4(HXLOG_CORP, "HXPlayer[%p]::::HXFileSource[%p]::FillBuffers End",
			m_pPlayer,
			this);
		
    return retVal;
}

// Select an empty (prfererred) or lowest time-stamp stream that is
// not completed or already requested.
HXBOOL HXFileSource::SelectNextStreamToFill(STREAM_INFO* &lpStreamInfoOut)
{
    STREAM_INFO* lpStreamInfo = NULL;
    UINT32 ulMinTS;
    INT32 lVelocity = m_pPlayer->GetVelocity();
    UINT32 ulEmptyStreamsMaxBufferingInMs = 0;
    HXBOOL bMinTSSet = FALSE;
    HXBOOL bSelectedStream = FALSE;
    HXBOOL bSelectedEmptyStream = FALSE;
    HXBOOL bHasUnfilledStreams = FALSE;

    CHXMapLongToObj::Iterator ndxStream	= mStreamInfoTable->Begin();

    for(; ndxStream != mStreamInfoTable->End(); ++ndxStream)
    {
	lpStreamInfo = (STREAM_INFO*) (*ndxStream);
	
	if (!lpStreamInfo->m_bSrcStreamDone)
	{
	    if (lpStreamInfo->m_EventList.GetNumEvents() == 0)
	    {
		bHasUnfilledStreams = TRUE;

		if (ulEmptyStreamsMaxBufferingInMs < lpStreamInfo->BufferingState().GetMinBufferingInMs())
		{
		    ulEmptyStreamsMaxBufferingInMs = lpStreamInfo->BufferingState().GetMinBufferingInMs();
		}
	    }

	    if (!lpStreamInfo->m_bPacketRequested)
	    {
		if (lpStreamInfo->m_EventList.GetNumEvents() == 0)
		{
		    lpStreamInfoOut = lpStreamInfo;
		    bSelectedStream = TRUE;
		    bSelectedEmptyStream = TRUE;
		    break;
		}
		else
		{
		    UINT32 ulTS = lpStreamInfo->m_EventList.GetTail()->GetPacket()->GetTime();

		    if (bMinTSSet)
		    {
			LONG32 lTSDelta = (LONG32) ((lVelocity >= 0) ? (ulMinTS - ulTS) : (ulTS - ulMinTS));
			if (lTSDelta > 0)
			{
			    lpStreamInfoOut = lpStreamInfo;
			    ulMinTS = ulTS;
			}
		    }
		    else
		    {
			lpStreamInfoOut = lpStreamInfo;
			ulMinTS = ulTS;
			bMinTSSet = TRUE;
		    }
		}

		bSelectedStream = TRUE;
	    }
	}
    }

    if (bHasUnfilledStreams && bSelectedStream && (!bSelectedEmptyStream) &&
	(ulEmptyStreamsMaxBufferingInMs == 0))
    {
	// We need to make sure we do not select streams to be filled too far
	// ahead of time and thus loading too much data and into RAM and
	// spinning for longer than neeed.  
	// This can occur when one of the streams is very sparse
	// (e.g. event stream with events possibly hours apart) and we are
	// unable to fill the buffer for it without filling the buffers
	// of all other streams excessively.
	// Thus, when we select a non-empty stream over an empty one,
	// we check that the minimum renderer dispatch time of the empty stream is
	// ahead of where the play-time currently is and if so we do
	// not select the stream to fill.
	// We skip this check when empty streams
	// have a buffering requirement (all very sparse streams should have no
	// buffering requirement).

	UINT32 ulMinEmptyStreamsTime = 
		lpStreamInfoOut->m_EventList.GetTail()->GetTimeStartPos();

	ulMinEmptyStreamsTime += lpStreamInfoOut->BufferingState().GetMinBufferingInMs();
		
	UINT32 ulEmptyStreamsMaxRendererDispatchTime = m_pPlayer->ComputeFillEndTime(
		    m_pPlayer->GetInternalCurrentPlayTime(),
		    m_pPlayer->GetGranularity(),
		    ulEmptyStreamsMaxBufferingInMs + MAX_INTERSTREAM_TIMESTAMP_JITTER);

	if (IsAcceleratingEventDelivery())
        {
            ulEmptyStreamsMaxRendererDispatchTime = CalcAccelDeliveryTime(ulEmptyStreamsMaxRendererDispatchTime);            
        }

	INT32 lMinEmptyStreamsAheadOfDispatchTime = 
	    ((INT32) (ulMinEmptyStreamsTime - ulEmptyStreamsMaxRendererDispatchTime));

	if (lVelocity >= 0)
	{
	    if (lMinEmptyStreamsAheadOfDispatchTime > 0)
	    {
		bSelectedStream = FALSE;
	    }
	}
	else
	{
	    if (lMinEmptyStreamsAheadOfDispatchTime < 0)
	    {
		bSelectedStream = FALSE;
	    }
	}
    }

    return (bSelectedStream && bHasUnfilledStreams);
}

void		
HXFileSource::SetEndOfClip(HXBOOL bForcedEndofClip) 
{
    m_bForcedSourceEnd = bForcedEndofClip;

    if (!m_bSourceEnd)
    {
	m_bSourceEnd = TRUE;				  
	m_pBufferManager->Stop();

	m_pPlayer->EndOfSource(this);

#if defined(HELIX_FEATURE_RECORDCONTROL)
	if (m_pRecordControl)
	{
	    m_pRecordControl->OnEndOfPackets();
	}
#endif /* HELIX_FEATURE_RECORDCONTROL*/
    }
}



// IUnknown methods
/////////////////////////////////////////////////////////////////////////
//	Method:
//		IUnknown::QueryInterface
//	Purpose:
//		Implement this to export the interfaces supported by your 
//		object.
//
STDMETHODIMP 
HXFileSource::CMimeFinderFileResponse::QueryInterface(REFIID riid, void** ppvObj)
{
    QInterfaceList qiList[] =
        {
            { GET_IIDHANDLE(IID_IHXFileMimeMapperResponse), (IHXFileMimeMapperResponse*)this },
            { GET_IIDHANDLE(IID_IHXFileRecognizerResponse), (IHXFileRecognizerResponse*)this },
            { GET_IIDHANDLE(IID_IUnknown), (IUnknown*)(IHXFileMimeMapperResponse*)this },
        };
    
    return ::QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
}

/////////////////////////////////////////////////////////////////////////
//	Method:
//		IUnknown::AddRef
//	Purpose:
//		Everyone usually implements this the same... feel free to use
//		this implementation.
//
STDMETHODIMP_(ULONG32) 
HXFileSource::CMimeFinderFileResponse::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

/////////////////////////////////////////////////////////////////////////
//	Method:
//		IUnknown::Release
//	Purpose:
//		Everyone usually implements this the same... feel free to use
//		this implementation.
//
STDMETHODIMP_(ULONG32) 
HXFileSource::CMimeFinderFileResponse::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
	return m_lRefCount;
    }

    delete this;
    return 0;
}

	// IHXFileMimeMapperResponse methods

STDMETHODIMP 
HXFileSource::CMimeFinderFileResponse::MimeTypeFound(HX_RESULT	status,
		                                     const char* pMimeType)
{
    m_pSource->FinishSetup(status,pMimeType); 
    return HXR_OK; 
}

STDMETHODIMP 
HXFileSource::CMimeFinderFileResponse::GetMimeTypeDone(HX_RESULT status,
                                                       IHXBuffer* pMimeType)
{
    char* szMimeType = NULL;

    if (SUCCEEDED(status) && pMimeType)
    {
        pMimeType->AddRef();
        szMimeType = (char*)pMimeType->GetBuffer();
    }

#if !defined(_SYMBIAN)
    //XXXLCM temporary fix until recognizer return codes are better defined and handled; this
    //       forces fallback to load-by-file-extension logic in FinishSetup()
    if (status == HXR_FAIL)
    {
        status = HXR_OK;
    }
#endif
    m_pSource->FinishSetup(status, szMimeType);
    HX_RELEASE(pMimeType);
    
    return status;
}

/*
 *	IHXRegistryID methods
 */

/************************************************************************
 *	Method:
 *	    IHXRegistryID::GetID
 *	Purpose:
 *	    Get registry ID(hash_key) of the objects(player, source and stream)
 *
 */
STDMETHODIMP 
HXFileSource::GetID(REF(UINT32) /*OUT*/ ulRegistryID)
{
#if defined(HELIX_FEATURE_STATS) && defined(HELIX_FEATURE_REGISTRY)
    (m_pStats)?(ulRegistryID = m_pStats->m_ulRegistryID):(ulRegistryID = 0);
   
    return HXR_OK;
#else
    return HXR_NOTIMPL;
#endif /* HELIX_FEATURE_STATS && HELIX_FEATURE_REGISTRY */
}

/************************************************************************
 *	Method:
 *	    IHXInfoLogger::LogInformation
 *	Purpose:
 *	    Logs any user defined information in form of action and 
 *	    associated data.
 */
STDMETHODIMP
HXFileSource::LogInformation(const char* /*IN*/ pAction,
			      const char* /*IN*/ pData)
{
    return HXR_OK;
}

HXBOOL		
HXFileSource::IsSourceDone(void)
{
    return m_bSourceEnd;
}

void
HXFileSource::AdjustClipBandwidthStats(HXBOOL bActivate /* = FALSE */)
{
    m_bIsActive = bActivate;

    for (CHXMapLongToObj::Iterator i = mStreamInfoTable->Begin();
         i != mStreamInfoTable->End(); ++i) 
    {    

#if defined(HELIX_FEATURE_STATS) && defined(HELIX_FEATURE_REGISTRY)	
	STREAM_INFO* pStreamInfo = (STREAM_INFO*) (*i);
	HX_ASSERT(pStreamInfo->m_pStats);

	if (!pStreamInfo->m_pStats)
	{
	    continue;
	}
#endif /* HELIX_FEATURE_STATS && HELIX_FEATURE_REGISTRY */

	if (bActivate)
	{
#if defined(HELIX_FEATURE_STATS) && defined(HELIX_FEATURE_REGISTRY)
	    pStreamInfo->m_pStats->m_pClipBandwidth->SetInt((INT32)pStreamInfo->BufferingState().AvgBandwidth());
#endif /* HELIX_FEATURE_STATS && HELIX_FEATURE_REGISTRY */
	}
#if defined(HELIX_FEATURE_STATS) && defined(HELIX_FEATURE_REGISTRY)
	else
	{
	    pStreamInfo->m_pStats->m_pClipBandwidth->SetInt(0);
	}
#endif /* HELIX_FEATURE_STATS && HELIX_FEATURE_REGISTRY */
    }
}

HXBOOL
HXFileSource::CanBeResumed()
{
    HXBOOL    bResult = TRUE;
    UINT32  ulCurrentTime = 0;

    if (!m_bInitialized		    ||
	!m_pPlayer->IsInitialized() ||
	m_bSourceEnd		    ||
	(m_pSourceInfo && !m_pSourceInfo->AreStreamsSetup()))
    {
	bResult = FALSE;
    }
    else if (m_bPrefetch)
    {
	HX_ASSERT(m_pSourceInfo?(m_pPlayer->GetCurrentGroupID() == m_pSourceInfo->m_uGroupID):TRUE);

	ulCurrentTime = m_pPlayer->GetInternalCurrentPlayTime();
	if (ulCurrentTime < m_ulPrefetchDelay)
	{
	    bResult = FALSE;	
	}
    }
    else if ((!m_bIsPreBufferingStarted && m_bDelayed) ||
	     (m_bIsPreBufferingDone && ((!m_bPaused && !m_bFirstResume) || m_bDelayed)))
    {
	bResult = FALSE;
    }

    return bResult;
}

#if defined(HELIX_FEATURE_ASM)
// SourceBandwidthInfo
STDMETHODIMP 
HXFileSource::SourceBandwidthInfo::QueryInterface(REFIID riid, void** ppvObj)
{
    QInterfaceList qiList[] =
        {
            { GET_IIDHANDLE(IID_IHXSourceBandwidthInfo), (IHXSourceBandwidthInfo*)this },
            { GET_IIDHANDLE(IID_IUnknown), (IUnknown*)(IHXSourceBandwidthInfo*)this },
        };
    
    return ::QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
}

STDMETHODIMP_(ULONG32) 
HXFileSource::SourceBandwidthInfo::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32) 
HXFileSource::SourceBandwidthInfo::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
	return m_lRefCount;
    }

    delete this;
    return 0;
}

STDMETHODIMP
HXFileSource::SourceBandwidthInfo::InitBw(IHXBandwidthManagerInput* pBwMgr)
{
    return HXR_OK;
}

STDMETHODIMP
HXFileSource::SourceBandwidthInfo::SetTransmitRate(UINT32 ulBitRate)
{
    return HXR_OK;
}
#endif /* HELIX_FEATURE_ASM */

void 
HXFileSource::CalculateCurrentBuffering(void)
{
    UINT32	ulRemainToBufferInMs	= 0;
    UINT32	ulRemainToBuffer	= 0;
    UINT32	ulExcessBufferInMs	= 0;
    UINT32	ulExcessBuffer		= 0;
    HXBOOL	bValidInfo		= FALSE;
    UINT32	ulActualExcessBufferInMs= 0;
    UINT32	ulActualExcessBuffer	= 0;

    m_pBufferManager->GetExcessBufferInfo(ulRemainToBufferInMs,
					  ulRemainToBuffer,
					  ulExcessBufferInMs,
					  ulExcessBuffer,
					  bValidInfo,
				  	  ulActualExcessBufferInMs,
				  	  ulActualExcessBuffer);

    /* Be conservative in marking pre-buffering done */
    if (bValidInfo		    &&
	!m_bIsPreBufferingDone	    &&
	ulRemainToBufferInMs == 0   && 
	ulRemainToBuffer == 0	    && 
	(ulExcessBuffer > m_ulAvgBandwidth/8 || 
	 ulExcessBufferInMs > m_ulPreRollInMs))
    {
	if (m_bDelayed && m_pPlayer)
	{
	    // pause the src if it's not time to play but it's done
	    // with the pre-fetch
	    if (m_pSourceInfo)
	    {
		m_pSourceInfo->UnRegister();
	    }

	    DoPause();
	}

	m_bIsPreBufferingDone = TRUE;
    }
}

void
HXFileSource::GetFileDone(HX_RESULT rc, IHXBuffer* pFile)
{
    HX_RESULT   ret = rc;
    UINT32      ulSDPBufferSize = 0;
    char*       pszSDPBuffer = NULL;
    CHXString   escapedSDP;
    CHXString   url;

    if (HXR_OK != ret || !pFile)
    {
        goto cleanup;
    }

    ulSDPBufferSize = pFile->GetSize() + 1;

    pszSDPBuffer = new char[ulSDPBufferSize];
    if (NULL == pszSDPBuffer)
    {
        ret = HXR_OUTOFMEMORY;
        goto cleanup;
    }

    memset(pszSDPBuffer, 0, ulSDPBufferSize);
    strncpy(pszSDPBuffer, (const char*)pFile->GetBuffer(), pFile->GetSize());

    escapedSDP = HXEscapeUtil::EscapeGeneric(pszSDPBuffer);

    url = HELIX_SDP_SCHEME;
    url += ":";
    url += escapedSDP;

    HX_DELETE(m_pSDPURL);
    m_pSDPURL = new CHXURL((const char*)url,
                           (IHXClientEngine*)m_pEngine);
    
    ret = m_pSourceInfo->HandleSDPRequest();

cleanup:

    HX_VECTOR_DELETE(pszSDPBuffer);

    if (HXR_OK != ret)
    {
	mLastError = ret;
	ReportError(mLastError);
    }

    return;
}

void
HXFileSource::CheckForDefaultUpgrade(HX_RESULT status)
{
#if defined(HELIX_FEATURE_AUTOUPGRADE)
    if (status == HXR_REQUEST_UPGRADE && !m_bAddDefaultUpgrade)
    {
	m_bAddDefaultUpgrade = TRUE;
	if (m_pMimeType && !(*m_pMimeType == '*'))
	{
	    m_pDefaultUpgradeString = ::new_string(m_pMimeType);
	}
	else if (m_pExtension)
	{
	    m_pDefaultUpgradeString = ::new_string(m_pExtension);
	}
    }
#endif /* HELIX_FEATURE_AUTOUPGRADE */ 
}

HX_RESULT	
HXFileSource::FillRecordControl(UINT32 ulLoopEntryTime)
{
#if defined(HELIX_FEATURE_RECORDCONTROL)
    if(!m_pRecordControl)
	return HXR_FAILED; 

    FillBuffers(ulLoopEntryTime);

    return HXR_OK;
#else
    return HXR_NOTIMPL;
#endif /* HELIX_FEATURE_RECORDCONTROL */
}

HXBOOL	
HXFileSource::ShouldDisableFastStart(void)
{
    HXBOOL bCheckBandwidth = FALSE;
    HXBOOL bCheckDeterminationFinalized = FALSE;
    HXBOOL bRetVal = FALSE;

    if (m_pFFObject)
    {
	IHXAdvise* pFFAdvise = NULL;

	if (SUCCEEDED(m_pFFObject->QueryInterface(IID_IHXAdvise, (void**) &pFFAdvise)))
	{
	    HX_RESULT adviseStatus = pFFAdvise->Advise(HX_FILERESPONSEADVISE_FASTSTART);

	    if (adviseStatus == HXR_ADVISE_NO_FASTSTART)
	    {
		// Disable FastStart
		bRetVal = TRUE;
		bCheckBandwidth = FALSE;
		bCheckDeterminationFinalized = TRUE;
	    }
	    else
	    {
		adviseStatus = pFFAdvise->Advise(HX_FILERESPONSEADVISE_NETWORKACCESS);
		if (adviseStatus == HXR_ADVISE_LOCAL_ACCESS)
		{
		    bCheckBandwidth = FALSE;
		    bCheckDeterminationFinalized = TRUE;
		}
		else if (SUCCEEDED(adviseStatus))
		{
		    bCheckBandwidth = TRUE;
		}
	    }
	}

	HX_RELEASE(pFFAdvise);
    }

    if (m_pFileObject && 
	(!bRetVal) && 
	(!bCheckBandwidth) && 
	(!bCheckDeterminationFinalized))
    {
	HX_RESULT adviseStatus = m_pFileObject->Advise(HX_FILERESPONSEADVISE_FASTSTART);

	if (adviseStatus == HXR_ADVISE_NO_FASTSTART)
	{
	    // Disable FastStart
	    bRetVal = TRUE;
	}
	else
	{
	    adviseStatus = m_pFileObject->Advise(HX_FILEADVISE_NETWORKACCESS);
	    if (adviseStatus == HXR_ADVISE_LOCAL_ACCESS)
	    {
		bCheckBandwidth = FALSE;
	    }
	    else if (SUCCEEDED(adviseStatus))
	    {
		bCheckBandwidth = TRUE;
	    }
	}
    }

    if (bCheckBandwidth && (!bRetVal))
    {
	UINT32 ulBandwidth = 0;
	if (((HXR_OK == ReadPrefUINT32(m_pPreferences, "ConnectionBandwidth", ulBandwidth)) && (ulBandwidth != 0)) ||
	    (HXR_OK == ReadPrefUINT32(m_pPreferences, "Bandwidth", ulBandwidth)))
	{
	    UINT32 ulPercentageOfBandwidthNeededForFastStart = LOCALSOURCE_FAST_START_BW_CAPACITY_THRESHOLD;
	    if (HXR_OK != ReadPrefUINT32(m_pPreferences, "PercentageBW4FS", ulPercentageOfBandwidthNeededForFastStart))
	    {
		// default to 150%
		ulPercentageOfBandwidthNeededForFastStart = LOCALSOURCE_FAST_START_BW_CAPACITY_THRESHOLD;
	    }

	    if ((((float) m_ulAvgBandwidth) * 
		 ((float) ulPercentageOfBandwidthNeededForFastStart) / 
		 100.0) > ((float) ulBandwidth))
	    {
		// We're too close to the boundary. We should disable fast start.
		bRetVal = TRUE;
	    }
	}
    }

    return bRetVal;
}

HX_RESULT HXFileSource::HandleSDPData(IHXValues* pHeader)
{
    return HXStreamDescriptionHelper::ParseAndMergeSDPData(
        (IHXClientEngine*)m_pEngine, pHeader);
}

HX_RESULT 
HXFileSource::HandleOutOfPackets(STREAM_INFO* pStreamInfo, 
				 UINT32 ulLoopEntryTime,
				 UINT32 ulProcessingTimeAllowance)
{
    HX_RESULT res =  HXR_NO_DATA;

    if (ShouldRebufferOnOOP(pStreamInfo))
    {
        DoRebuffer(ulLoopEntryTime, ulProcessingTimeAllowance);
        FillBuffers(ulLoopEntryTime, ulProcessingTimeAllowance);
        res = HXR_BUFFERING;
    }
    
    return res;
}

HXFileSource::CFileReader::CFileReader(HXFileSource* pOwner)
    : m_bGetFilePending(FALSE),
      m_pBuffer(NULL),
      m_pOwner(pOwner),
      m_pFile(NULL),
      m_lRefCount(0)
{
}

HXFileSource::CFileReader::~CFileReader()
{
    Close();
}

STDMETHODIMP HXFileSource::CFileReader::QueryInterface(REFIID riid, void** ppvObj)
{
    QInterfaceList qiList[] =
        {
            { GET_IIDHANDLE(IID_IHXFileResponse), (IHXFileResponse*)this },
            { GET_IIDHANDLE(IID_IUnknown), (IUnknown*)(IHXFileResponse*)this },
        };
    
    return ::QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
}

STDMETHODIMP_(ULONG32) HXFileSource::CFileReader::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32) HXFileSource::CFileReader::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

STDMETHODIMP HXFileSource::CFileReader::InitDone(HX_RESULT status)
{
    HX_RESULT ret = HXR_FAIL;

    if (FAILED(status))
    {
        m_bGetFilePending = FALSE;
        m_pOwner->GetFileDone(HXR_FAILED, NULL);
    }
    else
    {
	ret = m_pFile->Read(FILEREAD_SIZE);
    }
    return ret;
}

STDMETHODIMP HXFileSource::CFileReader::SeekDone(HX_RESULT /* status */)
{
    return HXR_OK;
}

STDMETHODIMP HXFileSource::CFileReader::ReadDone(HX_RESULT status,
					         IHXBuffer* pBuffer)
{
    HX_RESULT   ret = HXR_OK;
    IHXBuffer*  pMergedBuffer = NULL;

    if (SUCCEEDED(status))
    {
	CreateBufferCCF(pMergedBuffer, (IUnknown*)(IHXPlayer*)(m_pOwner->m_pPlayer));
        if (!pMergedBuffer)
        {
            ret = HXR_OUTOFMEMORY;
            goto cleanup;
        }

        if (!m_pBuffer)
        {
            pMergedBuffer->SetSize(pBuffer->GetSize());
            pMergedBuffer->Set(pBuffer->GetBuffer(), pBuffer->GetSize());
        }
        else
        {
            pMergedBuffer->SetSize(m_pBuffer->GetSize() + pBuffer->GetSize());
	    memcpy(pMergedBuffer->GetBuffer(), m_pBuffer->GetBuffer(), m_pBuffer->GetSize()); /* Flawfinder: ignore */
	    memcpy(pMergedBuffer->GetBuffer()+m_pBuffer->GetSize(), pBuffer->GetBuffer(), pBuffer->GetSize()); /* Flawfinder: ignore */
	}

	HX_RELEASE(m_pBuffer);
	m_pBuffer = pMergedBuffer;

	m_pFile->Read(FILEREAD_SIZE);
    }
    else
    {
        m_bGetFilePending = FALSE;
        if (m_pBuffer && m_pBuffer->GetSize())
        {	   
            // normal-case eof
            m_pOwner->GetFileDone(HXR_OK, m_pBuffer);
        }
        else
        {	    
            // read error
            m_pFile->Seek(0, FALSE);
            m_pOwner->GetFileDone(HXR_FAIL, NULL);
        }
    }

cleanup:

    return ret;
}

STDMETHODIMP HXFileSource::CFileReader::WriteDone(HX_RESULT /* status */)
{
    return HXR_OK;
}

STDMETHODIMP HXFileSource::CFileReader::CloseDone(HX_RESULT /* status */)
{
    return HXR_OK;
}

HX_RESULT
HXFileSource::CFileReader::GetFile(IHXFileObject* /*IN*/ pFile)
{
    HX_RESULT   ret = HXR_OK;

    if (m_bGetFilePending || !m_pOwner)
    {
        return HXR_FAILED;
    }

    Close();

    // get our own IHXFileResponse interface
    IHXFileResponse* pFileResponse = NULL;
    ret = QueryInterface(IID_IHXFileResponse, (void**) &pFileResponse);

    if (pFile != NULL)
    {
	m_pFile = pFile;
	m_pFile->AddRef();
    
	if (pFileResponse)
	{
	    ret = m_pFile->Init(HX_FILE_READ | HX_FILE_BINARY,
				pFileResponse);
	}
    }

    if (FAILED(ret))
    {
        m_pOwner->GetFileDone(HXR_FAIL, NULL); // XXXLCM bad (we return fail; caller should handle)
    }
    else
    {
        m_bGetFilePending = TRUE;
    }
    
    HX_RELEASE(pFileResponse);

    return ret;
}

void
HXFileSource::CFileReader::Close(void)
{
    m_bGetFilePending = FALSE;
    HX_RELEASE(m_pBuffer);
    HX_RELEASE(m_pFile);
}
