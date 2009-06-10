/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: servhttppost.cpp,v 1.2 2006/05/12 01:48:59 atin Exp $
 *
 * Portions Copyright (c) 1995-2003 RealNetworks, Inc. All Rights Reserved.
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>

#include "hxtypes.h"
#include "hxcom.h"
#include "hxstrutl.h"
#include "hxerror.h"
#include "hxcomm.h"
#include "ihxpckts.h"
#include "hxfiles.h"
#include "hxengin.h"
#include "hxcore.h"
#include "hxmon.h"

#include "prefdefs.h"
#include "hxprefs.h"
#include "chxpckts.h"
#include "dbcs.h"

#include "hxslist.h"

#include "mimehead.h"
#include "portaddr.h"
#include "hxtick.h"
#include "hxurl.h"

#include "ihxcookies.h"

#include "timerep.h"

#include "servhttppost.h"

#if defined( _WIN32 ) || defined( _WINDOWS )
#include "win_net.h"
#endif

#ifdef _MACINTOSH
#include "HX_MoreProcesses.h"
#endif

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE
static char HX_THIS_FILE[] = __FILE__;
#endif

#define ALLOCATION_SIZE     1024

#ifdef _LINUX
#define MAX_RECURSION_LEVEL     30
#elif defined(_MACINTOSH)
#define MAX_RECURSION_LEVEL     20
#else
#define MAX_RECURSION_LEVEL     200
#endif

#undef  LOG_DATA
#define  LOG_FILE               "C:/Temp/httppost.log"
#include "http_debug.h"

// default if no timeouts in preferences.
#define DEF_HTTP_SERVER_TIMEOUT     (20 * MILLISECS_PER_SECOND)
#define DEF_HTTP_CONNECT_TIMEOUT    (30 * MILLISECS_PER_SECOND)

#define HTTPPOST_READ_TIMEOUT       "IHXHTTPPostObject: Socket timeout waiting for POST response."


CHXHTTPPostObject*
CHXHTTPPostObject::CreateObject()
{
    LOGX ((szDbgTemp, "CreateObject()"));

    CHXHTTPPostObject* pNew = new CHXHTTPPostObject;

    return pNew;
}

STDMETHODIMP_(ULONG32)
CHXHTTPPostObject::AddRef (THIS)
{
    return InterlockedIncrement(&m_lCount);
}

STDMETHODIMP_(ULONG32)
CHXHTTPPostObject::Release (THIS)
{
    HX_ASSERT(m_lCount>=0);
    if (InterlockedDecrement(&m_lCount) > 0)
    {
        return m_lCount;
    }

    delete this;
    return 0;
}

STDMETHODIMP
CHXHTTPPostObject::QueryInterface(REFIID riid, void** ppvObj)
{
    LOGX ((szDbgTemp, "QueryInterface(x0%08X)", riid));
    if (!ppvObj)
        return HXR_POINTER;
    if (IsEqualIID(IID_IUnknown, riid))
    {
        AddRef();
        *ppvObj = (this);
        return HXR_OK;
    }
    if (IsEqualIID(IID_IHXRequestHandler, riid))
    {
        AddRef();
        *ppvObj = (IHXRequestHandler*)(this);
        return HXR_OK;
    }
    if (IsEqualIID(IID_IHXTimeoutSettings, riid))
    {
        AddRef();
        *ppvObj = (IHXTimeoutSettings*)(this);
        return HXR_OK;
    }
    if (IsEqualIID(IID_IHXHTTPPostObject, riid))
    {
        AddRef();
        *ppvObj = (IHXHTTPPostObject*)(this);
        return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}



CHXHTTPPostObject::CHXHTTPPostObject()
    : m_lCount(0)

    , m_LastError (HXR_OK)

    , m_pCommonClassFactory(NULL)
    , m_pPreferences(NULL)
    , m_pScheduler(NULL)
    , m_pRegistry(NULL)
    , m_pContext(NULL)
    , m_pInterruptState(NULL)
#ifdef _MACINTOSH
    , m_pReadDoneBuffer(NULL)
    , m_uReadDoneStatus(HXR_OK)
    , m_bReadDoneToBeProcessed(FALSE)
#endif
    , m_pErrorMessages(NULL)

    , m_bInitResponsePending(FALSE)
    , m_pFileResponse (NULL)

    , m_bTCPReadDonePending(FALSE)
    , m_pTCPResponse(NULL)

    , m_szBaseURL(NULL)

    , m_pFilename(NULL)
    , m_pPath(NULL)
    , m_pHost(NULL)

    , m_ulFlags(0)
    , m_pRequest(NULL)
    , m_pRequestHeadersOrig(NULL)

    , m_pLanguage(NULL)

    , m_pCallback(NULL)

    , m_bInitPending (FALSE)

    , m_bInitialized(FALSE)
    , m_bInDestructor(FALSE)

    , m_nPort(DEF_HTTP_PORT)

    , m_nRequestTime(0) // used to trigger request time-outs
    , m_bSocketReadTimeout(FALSE) // a socket read timeout has occurred
    , m_nConnTimeout(DEF_HTTP_CONNECT_TIMEOUT)
    , m_nServerTimeout(DEF_HTTP_SERVER_TIMEOUT)
    , m_bDisableConnectionTimeOut(FALSE)

    , m_pSocket(NULL)

    , m_bKeepAlive(TRUE)

    , m_bConnectDone(FALSE)
    , m_bWriteDone(TRUE)
    , m_bPostDonePending(FALSE)

    , m_bReadHeaderDone(FALSE)
    , m_bReadContentsDone(TRUE)

    , m_bKnowContentSize(FALSE)
    , m_nContentSize(0)

    , m_nContentRead(0)
    , m_nContentLength(0)
    , m_bResponseReadyPending(FALSE)

    , m_nTotalRequestSize(0)
    , m_nRequestWritten(0)

    , m_nHeaderRead(0)
    , m_bOverrideContentType(FALSE)
    , m_bOverrideLanguage(FALSE)

    , m_strHost()
    , m_strRequest()
    , m_strResource()
    , m_strMimeType()
    , m_strResultHeader()

    , m_uMaxRecursionLevel(MAX_RECURSION_LEVEL)
    , m_uRecursionCount(0)
    , m_bInReadDone(FALSE)
    , m_bInPostDone(FALSE)
    , m_bInResponseReady(FALSE)
    , m_bPostResponseReady(FALSE)
    , m_bOpenFilePending(FALSE)
    , m_pContentBuffer(NULL)
{
}

void
CHXHTTPPostObject::InitObject(IUnknown* pContext)
{
    LOGX ((szDbgTemp, "InitObject(%s)", NULLOK(szBaseURL)));

    if (pContext)
    {
        m_pContext = pContext;
        m_pContext->AddRef();

        m_pContext->QueryInterface(IID_IHXScheduler, (void**) &m_pScheduler);
        m_pContext->QueryInterface(IID_IHXCommonClassFactory, (void **)&m_pCommonClassFactory);
        m_pContext->QueryInterface(IID_IHXPreferences, (void **)&m_pPreferences);
        m_pContext->QueryInterface(IID_IHXRegistry, (void**)&m_pRegistry);
        m_pContext->QueryInterface(IID_IHXErrorMessages, (void**)&m_pErrorMessages);
        m_pContext->QueryInterface(IID_IHXInterruptState, (void**)&m_pInterruptState);
    }

    //
    // get preferences needed
    //

    IHXBuffer* pBuffer = NULL;
    if (m_pPreferences)
    {
        // Get Language preference, if available
        if (m_pLanguage==NULL)
        {
            IHXRegistry* pRegistry = NULL;

            if (m_pContext->QueryInterface(IID_IHXRegistry, (void**)&pRegistry) == HXR_OK)
            {
                CHXString strTemp;

                strTemp.Format("%s.%s",HXREGISTRY_PREFPROPNAME,"Language");

                pRegistry->GetStrByName(strTemp, m_pLanguage);
                pRegistry->Release();
            }
        }

        // if the language pref is an empty string, we're not
        // interested
        if (m_pLanguage)
        {
            if ( *(m_pLanguage->GetBuffer()) == '\0')
            {
                HX_RELEASE(m_pLanguage);
            }
        }

        // Get connection timeout, if available
        m_pPreferences->ReadPref("ConnectionTimeout", pBuffer);

        // if the connection timeout is an empty string, we're not
        // interested
        if (pBuffer && *(pBuffer->GetBuffer()) != '\0')
        {
            m_nConnTimeout = (UINT32) (atol((const char*)pBuffer->GetBuffer()) * MILLISECS_PER_SECOND);
        }
        if (m_nConnTimeout <= 0)
        {
            m_nConnTimeout = DEF_HTTP_CONNECT_TIMEOUT;
        }
        HX_RELEASE(pBuffer);

        // Get server timeout, if available
        m_pPreferences->ReadPref("ServerTimeout", pBuffer);

        // if the connection timeout is an empty string, we're not
        // interested
        if (pBuffer && *(pBuffer->GetBuffer()) != '\0')
        {
            m_nServerTimeout = (UINT32) (atol((const char*)pBuffer->GetBuffer()) * MILLISECS_PER_SECOND);
        }
        if (m_nServerTimeout <= 0)
        {
            m_nServerTimeout = DEF_HTTP_SERVER_TIMEOUT;
        }

        HX_RELEASE(pBuffer);
    }

    m_pCallback = HTTPPostObjCallback::CreateObject();
    if (m_pCallback)
    {
        m_pCallback->InitObject(this);
        m_pCallback->AddRef();
    }
}

CHXHTTPPostObject::~CHXHTTPPostObject()
{
    LOGX ((szDbgTemp, "FileObject destructor"));
    if(m_bInDestructor)
    {
        return;
    }

    // release content buffer we were holding
    if (m_pContentBuffer)
    {
        HX_RELEASE(m_pContentBuffer);
    }

    m_bInDestructor = TRUE;

    Close();
}

STDMETHODIMP
CHXHTTPPostObject::SetRequest(IHXRequest* pRequest)
{
    char*        pTemp = NULL;
    const char*  pURL;

    HX_RELEASE(m_pRequest);
    m_pRequest = pRequest;

    if (m_pRequest)
    {
        m_pRequest->AddRef();

        if (m_pRequest->GetURL(pURL) != HXR_OK)
        {
            return HXR_FAIL;
        }

        LOGX((szDbgTemp, "    GetURL() returns: '%s'", pURL));

        // if URL contains <protocol>:// then DON'T reuse basepath
        // from original request, instead just use the new URL.
        CHXString   sPath;
        if (m_szBaseURL && pURL && (strncasecmp(pURL,"http://",  7) != 0))
        {
            sPath = m_szBaseURL;
            if(sPath[sPath.GetLength()-1] != '/')
            {
                sPath += '/';
            }
        }
        else
        {
            sPath = "";
        }

        if (pURL)
        {
            sPath += pURL;
        }

        // Remove default HTTP port specification, if present
        //sPath.FindAndReplace(DEF_HTTP_PORT_STR, "/");

        delete[] m_pFilename;
        m_pFilename = new_string(sPath.GetBuffer(1));

        if (m_pRequestHeadersOrig)
        {
            HX_RELEASE(m_pRequestHeadersOrig);
        }

        // Save these headers.
        m_pRequest->GetRequestHeaders(m_pRequestHeadersOrig);
    }

    return HXR_OK;
}

STDMETHODIMP
CHXHTTPPostObject::GetRequest(REF(IHXRequest*) pRequest)
{
    pRequest = m_pRequest;

    if (pRequest)
    {
        pRequest->AddRef();
    }

    return HXR_OK;
}

/************************************************************************
 *  Method:
 *      IHXFileObject::Init
 *  Purpose:
 *      Associates a file object with the file response object it should
 *      notify of operation completness. This method should also check
 *      for validity of the object (for example by opening it if it is
 *      a local file).
 */
STDMETHODIMP CHXHTTPPostObject::Init(ULONG32 /*IN*/  ulFlags, IHXHTTPPostResponse* /*IN*/ pFileResponse)
{
    LOG ("Init()");
    HX_RESULT   theErr  = HXR_OK;
    HX_RESULT   lResult = HXR_OK;
    char*       pTemp   = NULL;
    char*       pRes    = NULL;

    if (m_pFileResponse)
    {
        HX_RELEASE(m_pFileResponse);
    }

    m_pFileResponse = pFileResponse;
    if (m_pFileResponse)
    {
        m_pFileResponse->AddRef();
    }

 /* This is temporary for Flash fileformat on Mac. We need to set a really low
 * value for recursion guard for flash fileformat since it puts everything
 * on freaki'n stack thereby blowing the system stack limit on Mac (looks like
 * there is one since it corrupts the heap) very easily.
 */
#if defined (_MACINTOSH)
    IHXGetRecursionLevel* pGet         = NULL;
    HX_RESULT              lThisResult  = HXR_OK;
    lThisResult = m_pFileResponse->QueryInterface(IID_IHXGetRecursionLevel,
                                       (void**) &pGet);
    if (lThisResult == HXR_OK)
    {
            m_uMaxRecursionLevel = pGet->GetRecursionLevel();
            pGet->Release();
    }
#endif

    m_bPostResponseReady = FALSE;

    /*
    if (m_bInitialized)
    {
        if (m_LastError == HXR_OK)
        {
            // If we have already posted part of the resource, then set back to the start
            // to zero during re-initialization
            //
            m_ulCurrentWritePosition = 0;

            m_pFileResponse->InitDone(HXR_OK);
            return HXR_OK;
        }
        else
        {
            m_pFileResponse->InitDone(HXR_FAILED);
            return HXR_FAILED;
        }
    }
    */

    theErr = _OpenFile(m_pFilename, ulFlags);

    if (!theErr)
    {
        m_bInitResponsePending = TRUE;
    }
    else
    {
        m_pFileResponse->InitDone(HXR_FAILED);
    }

    return theErr;
}

/************************************************************************
 *      Method:
 *          Private interface::OpenFile
 *      Purpose:
 *          This common method is used from Init()
 */
HX_RESULT CHXHTTPPostObject::_OpenFile(const char* url, ULONG32 ulFlags)
{
    HX_RESULT   theErr  = HXR_OK;
    HX_RESULT   lResult = HXR_OK;
    char*       pTemp   = NULL;

    LOG("_OpenFile");
    LOGX((szDbgTemp, "    URL='%s'", url));

    // Make local copy of url
    CHXString   strTemp = url;
    char*       pURL    = strTemp.GetBuffer(strTemp.GetLength());

    // HTTP requires '/' as the element delimiter.
    // So change all '\\' to '/'.
    // Also, try not to proccess any parameters.
    pTemp = pURL;
    while(*pTemp && *pTemp!='?' && *pTemp!='#')
    {
        if(*pTemp == '\\')
        {
            *pTemp = '/';
        }
        pTemp++;
    }

    m_strHost           = "";
    m_nPort             = DEF_HTTP_PORT;
    m_strResource       = "";

    // if the url's first five characters are "http:"
    // then jump past them... Otherwise, just assume
    // the URL starts with the host... This will allow
    char* pcColon = (char *) HXFindChar (pURL, ':');
    char* pcQuery = (char *) HXFindChar (pURL, '?');
    if (pcColon && (pcQuery == 0 || pcColon < pcQuery))
    {
        pURL = pcColon + 1;
    }

    // Jump past the double whack if present...
    if (HXCompareStrings(pURL, "//", 2) == 0)
    {
        pURL += 2;
    }

    pTemp = (char *)HXFindChar(pURL, '/');

    if (pTemp)
    {
        m_strResource = pTemp; //  /* Remainder is resource */

        *pTemp = '\0';
    }

    // Look for the '@' character which means there is a username and/or password.  We skip over this part to the
    // hostname.  The '@' character is reserved according to RFC 1738 show it shouldn't appear in a URL
    pTemp = (char *)HXFindChar (pURL, '@');
    if (pTemp)
        pURL += (pTemp - pURL + 1);

    /* Port (optional) */
    pTemp = (char *)HXFindChar (pURL, ':');
    if (pTemp)
    {
        *pTemp = '\0';
        m_nPort = ::atoi(pTemp+1);

        // port '0' is invalid, but we'll get that if the url had ':' but no port number
        // following. Setting to default http_port in this case will mimic the
        // behaviour of pnm: and rtsp: if no port specified after ':'
        if (m_nPort==0)
        {
            m_nPort = DEF_HTTP_PORT;
        }
    } /* if (pTemp) */

    m_strHost = pURL;

    // connect to the host and start sending the data
    theErr = BeginPost();
    if (!theErr)
    {
        m_bInitPending = TRUE;
    }

    return theErr;
}

/////////////////////////////////////////////////////////////////////////////
//
//  Method:
//
//      CHXHTTPPostObject::::BeginPost()
//
//  Purpose:
//
//      Begins retrieval of an http URL.
//
//  Parameters:
//
//      None.
//
//  Return:
//
//      HX_RESULT err
//      HXR_OK if the URL was good and setup for retrival completes
//      without error.
//
//
HX_RESULT CHXHTTPPostObject::BeginPost(void)
{

    HX_RESULT theErr = HXR_OK;
    HX_RESULT  lResult = HXR_OK;
    CHXURL*     pHXURL = NULL;
    IHXValues*  pURLProperties = NULL;
    IHXBuffer*  pHost = NULL;
    IHXBuffer*  pPath = NULL;
    IHXBuffer* pCookies = NULL;
    IHXNetworkServices* pNetworkServices = 0;

    // header labels
    const char szUserAgent[] = "\r\nUser-Agent: RMA/1.0 (compatible; HTTPPost)";
    const char szHostHeader[] = "\r\nHost: ";
    const char szContentLength[] = "\r\nContent-Length: ";

    // overridable by pre-placed request headers
    const char szContentType[] = "\r\nContent-Type: ";
    const char szAcceptLang[] = "\r\nAccept-Language: ";

    const char* pResource = m_strResource;
    ULONG32     ulPlatformData  = 0;
    IHXValues* pHeaders = 0;
    char*       pOutBuffer = 0;


#if defined (_WINDOWS) || defined (_WIN32)
    ulPlatformData  = (ULONG32) GetModuleHandle(NULL);
#endif // defined (_WINDOWS) || defined (_WIN32)

    if (!((const char*)m_strHost))
    {
        theErr = HXR_INVALID_URL_HOST;
        goto exit;
    }

    if (!*((const char*)m_strHost))
    {
        theErr = HXR_INVALID_URL_HOST;
        goto exit;
    }

    if (!pResource)
    {
        theErr = HXR_INVALID_URL_PATH;
        goto exit;
    }

    if (!*pResource)
    {
        theErr = HXR_INVALID_URL_PATH;
        goto exit;
    }

    /*
     * Get the RFC822 headers from the IHXRequest object
     */

    if (m_pRequest && m_pRequest->GetRequestHeaders(pHeaders) == HXR_OK)
    {
        if (pHeaders)
        {
            IHXValues* pValuesRequestHeaders = new CHXHeader();
            pValuesRequestHeaders->AddRef();
            CHXHeader::mergeHeaders(pValuesRequestHeaders, pHeaders);

            if (m_pRequestHeadersOrig)
            {
                CHXHeader::mergeHeaders(pValuesRequestHeaders, m_pRequestHeadersOrig);
            }

            /*
             * First spin through the headers to see how much space they
             * require (the extra 4 bytes per header is for overhead)
             */

            const char* pName;
            IHXBuffer* pValue;
            HX_RESULT result;
            UINT32 uBufferSize = 0;
            UINT32 uBufferPtr = 0;

            result = pValuesRequestHeaders->GetFirstPropertyCString(pName, pValue);

            while (result == HXR_OK)
            {
                uBufferSize += strlen(pName);
                uBufferSize += pValue->GetSize() - 1;
                uBufferSize += 4;

                pValue->Release();

                result = pValuesRequestHeaders->GetNextPropertyCString(pName, pValue);
            }

            /*
             * Allocate space for trailing '\0'
             */

            uBufferSize++;

            pOutBuffer = new char[uBufferSize];

            /*
             * Now spin through and built the outgoing string
             */

            result = pValuesRequestHeaders->GetFirstPropertyCString(pName, pValue);

            while (result == HXR_OK)
            {
                // see if any of these user-set headers should override the defaults
                if (strcasecmp(pName, "Content-Type") == 0)
                {
                    m_bOverrideContentType = TRUE;
                }
                else if (strcasecmp(pName, "Accept-Language") == 0)
                {
                    m_bOverrideLanguage = TRUE;
                }

                UINT32 uValueLength = pValue->GetSize() - 1;

                memcpy(&pOutBuffer[uBufferPtr], "\r\n", 2);
                uBufferPtr += 2;
                memcpy(&pOutBuffer[uBufferPtr], pName, strlen(pName));
                uBufferPtr += strlen(pName);
                memcpy(&pOutBuffer[uBufferPtr], ": ", 2);
                uBufferPtr += 2;
                memcpy(&pOutBuffer[uBufferPtr],
                       pValue->GetBuffer(),
                       uValueLength);
                uBufferPtr += uValueLength;

                pValue->Release();

                result = pValuesRequestHeaders->GetNextPropertyCString(pName, pValue);
            }

            HX_ASSERT(uBufferPtr == uBufferSize - 1);

            pOutBuffer[uBufferPtr] = '\0';

            HX_RELEASE(pValuesRequestHeaders);
            HX_RELEASE(pHeaders);
        }
    }


    // The request is a standard HTTP based request created from the resource...
    // since string.Format() has a max length of 512 chars, we'll build up the string manually
    m_strRequest = "POST ";
    m_strRequest += pResource;
    m_strRequest += " HTTP/1.0\r\nAccept: */*";
    m_strRequest += szUserAgent;

    if (m_nContentLength)
    {
        m_strRequest += szContentLength;
        m_strRequest.AppendULONG(m_nContentLength);
    }

    // if the caller did not set a language in the request headers, use default from prefs
    if (m_pLanguage && !m_bOverrideLanguage)
    {
        m_strRequest += szAcceptLang;
        m_strRequest += m_pLanguage->GetBuffer();
    }

    // if the caller did not set a content type in the request headers, use form-urlencoded
    if (!m_bOverrideContentType)
    {
        m_strRequest += szContentType;
        m_strRequest += "application/x-www-form-urlencoded";
    }

    m_strRequest += (pOutBuffer ? pOutBuffer : "");

    if(!m_strHost.IsEmpty())
    {
        m_strRequest += szHostHeader;
        // Use actual host, even when going through proxy..
        m_strRequest += m_strHost;
        if(m_nPort != 80)
        {
            m_strRequest += ":";
            m_strRequest.AppendULONG(m_nPort);
        }
    }


    m_strRequest += "\r\n\r\n";

    // if we have a cached content buffer from a previous POST, append it now!
    if (m_pContentBuffer)
    {
        m_strRequest += m_pContentBuffer->GetBuffer();
    }

    delete[] pOutBuffer;

    if(!m_pSocket)
    {
        if (HXR_OK != m_pContext->QueryInterface( IID_IHXNetworkServices,
                                                (void **)&pNetworkServices))
        {
            theErr = HXR_INVALID_PARAMETER;
            goto exit;
        }

        lResult = pNetworkServices->CreateTCPSocket(&m_pSocket);
        pNetworkServices->Release();
        if (lResult != HXR_OK || !m_pSocket)
        {
            theErr = HXR_INVALID_PARAMETER;
            goto exit;
        }

        if(!m_pTCPResponse)
        {
            m_pTCPResponse = HTTPPostTCPResponse::CreateObject();
            if (m_pTCPResponse)
            {
                m_pTCPResponse->InitObject(this);
                m_pTCPResponse->AddRef();
            }
            else
            {
                theErr = HXR_OUTOFMEMORY;
                goto exit;
            }
        }
        m_pSocket->Init(m_pTCPResponse);

        /* connect to the host */
        m_bSocketReadTimeout = FALSE;
        lResult = m_pSocket->Connect(m_strHost, m_nPort);
        if (lResult != HXR_OK)
        {
            theErr = HXR_INVALID_HOST;
            goto exit;
        }

        // first remove any scheduled callbacks - in case one is pending for immed.
        // callback, we want to remove that and set up for another callback in m_nCommTimeout msecs.
        if (m_pCallback && m_pCallback->m_bCallbackPending)
        {
            m_pScheduler->Remove(m_pCallback->m_ulPendingCallbackID);
        }

        // Ask the scheduler for callback to catch connection
        // timeout (in ProcessIdle)
        m_pCallback->m_bCallbackPending = TRUE;
        m_pCallback->m_ulPendingCallbackID = m_pScheduler->RelativeEnter(m_pCallback, m_nConnTimeout);
    }
    else
    {
        // Ask the scheduler to call ProcessIdle()
        // as m_pSocket!=NULL
        if (!m_pCallback->m_bCallbackPending)
        {
            m_pCallback->m_bCallbackPending = TRUE;
            m_pCallback->m_ulPendingCallbackID = m_pScheduler->RelativeEnter(m_pCallback, 0);
        }
    }

    m_nTotalRequestSize = m_strRequest.GetLength();

exit:

    return theErr;
}


/************************************************************************
 *  Method:
 *      IHXFileObject::Close
 *  Purpose:
 *      Closes the file resource and releases all resources associated
 *      with the object.
 */
STDMETHODIMP CHXHTTPPostObject::Close()
{
    LOGX ((szDbgTemp, "Close(%s)", m_pFilename));

    HX_RELEASE(m_pInterruptState);
    HX_RELEASE(m_pErrorMessages);
    HX_RELEASE(m_pRegistry);
    HX_RELEASE(m_pPreferences);
    HX_RELEASE(m_pCommonClassFactory);

    // If there is a pending callback, be sure to remove it!
    if (m_pCallback && m_pCallback->m_ulPendingCallbackID && m_pScheduler)
    {
        m_pScheduler->Remove(m_pCallback->m_ulPendingCallbackID);
    }

    HX_RELEASE(m_pCallback);
    HX_RELEASE(m_pScheduler);


    HX_RELEASE(m_pRequest);
    HX_RELEASE(m_pRequestHeadersOrig);

    HX_RELEASE(m_pContext);

    if (m_pTCPResponse)
    {
        m_pTCPResponse->m_pOwner = NULL;
        HX_RELEASE(m_pTCPResponse);
    }

    HX_RELEASE(m_pSocket);
    HX_RELEASE(m_pLanguage);

    HX_VECTOR_DELETE(m_szBaseURL);
    HX_VECTOR_DELETE(m_pFilename);
    HX_VECTOR_DELETE(m_pPath);
    HX_VECTOR_DELETE(m_pHost);

    if (m_pFileResponse && !m_bInDestructor)
    {
        m_pFileResponse->CloseDone(HXR_OK);
    }

    HX_RELEASE(m_pFileResponse);

    return HXR_OK;
}

void
CHXHTTPPostObject::CallPostDone(HX_RESULT status)
{
    m_bInPostDone = TRUE;

    // Let the file response sink know about the buffer...
    m_pFileResponse->PostDone(status);

    m_bInPostDone = FALSE;

    // release content buffer we were holding
    if (m_pContentBuffer)
    {
        HX_RELEASE(m_pContentBuffer);
    }

    /*
    if (m_uRecursionCount > 0)
    {
        m_uRecursionCount--;
    }
    */
}

void
CHXHTTPPostObject::CallResponseReady(HX_RESULT status, IHXBuffer* pContentBuffer)
{
    m_bInResponseReady = TRUE;

    // Let the file response sink know about the buffer...
    m_pFileResponse->ResponseReady(status, pContentBuffer);

    m_bInResponseReady = FALSE;

    /*
    if (m_uRecursionCount > 0)
    {
        m_uRecursionCount--;
    }
    */
}


/////////////////////////////////////////////////////////////////////////////
//
//      Method:
//              CHXHTTPPostObject::ProcessIdle()
//
//      Purpose:
//              Process various state-based actions
//
//      Parameters:
//              None.
//
//      Return:
//              HX_RESULT err
//              HXR_OK if things are continuing correctly.
//
HX_RESULT CHXHTTPPostObject::ProcessIdle()
{
    HX_RESULT theErr = HXR_OK;
    HX_RESULT  lResult = HXR_OK;
    const UINT16 bufSize = 4096;

#if 0
#ifdef _MACINTOSH
        /*
         * If we have a pending readdone callback from interrupt time
         * handle it now
         */
        if (m_bReadDoneToBeProcessed)
        {
                m_bReadDoneToBeProcessed = FALSE;
                HX_RESULT theReadStatus = m_uReadDoneStatus;
                IHXBuffer* pReadBuffer = m_pReadDoneBuffer;
                m_pReadDoneBuffer = NULL;
                m_uReadDoneStatus = HXR_OK;
                ReadDone(theReadStatus, pReadBuffer);
                HX_RELEASE(pReadBuffer);
        }
#endif
#endif


    if (m_bOpenFilePending)
    {
        m_bOpenFilePending = FALSE;
        return _OpenFile(m_pFilename, HX_FILE_READ|HX_FILE_BINARY);
    }

    // ProcessIdle doesn't get scheduled until the connection is established,
    // unless a connection timeout has been trigged.
    if (!m_bConnectDone && !m_bDisableConnectionTimeOut)
    {
        // connection failed, (ie we've hit a connect time-out)
        theErr = HXR_NET_CONNECT;
        HX_RELEASE(m_pSocket);
    }

    if (m_pSocket)
    {
        if (!m_bWriteDone)
        {
            UINT16 actual = m_nTotalRequestSize - m_nRequestWritten;
            IHXBuffer* pBuffer = new CHXBuffer;
            pBuffer->AddRef();
            pBuffer->Set((UCHAR*) (const char*) m_strRequest.Mid(m_nRequestWritten),
                         (ULONG32) actual);

            m_pSocket->WantWrite();

            lResult = m_pSocket->Write(pBuffer);
            if (lResult == HXR_OK)
            {
                m_nRequestWritten += actual;

                if (m_nRequestWritten == m_nTotalRequestSize)
                {
                    m_bWriteDone = TRUE;
                    m_bPostDonePending = TRUE;

                    if (m_pCallback && m_pCallback->m_bCallbackPending)
                    {
                        m_pScheduler->Remove(m_pCallback->m_ulPendingCallbackID);
                    }

                    HX_RELEASE(pBuffer);

                    return HXR_OK;
                }
            }
            else
            {
                theErr = HXR_SERVER_DISCONNECTED;
            }

            HX_RELEASE(pBuffer);
        }
        else if (!m_bReadContentsDone && !m_bTCPReadDonePending && !m_bSocketReadTimeout)
        {
            m_bTCPReadDonePending = TRUE;
            m_bSocketReadTimeout = FALSE;
            m_nRequestTime = HX_GET_TICKCOUNT();

            // Addref socket since it can be released in ReadDone()
            IHXTCPSocket* pTempSocket = m_pSocket;
            pTempSocket->AddRef();
            lResult = m_pSocket->Read(bufSize);
            pTempSocket->Release();
        }
        else if (m_bTCPReadDonePending &&
                 CALCULATE_ELAPSED_TICKS(m_nRequestTime,HX_GET_TICKCOUNT()) > m_nServerTimeout)
        {
            // we've timed-out waiting for a read response on the socket..
            m_bSocketReadTimeout = TRUE;
            m_bReadContentsDone = TRUE;

            if (m_pErrorMessages)
            {
                m_pErrorMessages->Report(HXLOG_ERR,
                                         HXR_SERVER_TIMEOUT,
                                         HXR_OK,
                                         HTTPPOST_READ_TIMEOUT,
                                         NULL);
            }
        }
    }

    /* We only consider the server to be disconnected if we are yet in
     * the initialization state. We need to mask this error otherwise
     */
    if (lResult != HXR_OK)
    {
        m_bReadContentsDone = TRUE;
        theErr = HXR_OK;
    }

    // defer sending post done until we've read response
    if (m_bReadContentsDone && m_bPostResponseReady)
    {
        if (m_bPostDonePending)
        {
            m_bPostDonePending = FALSE;
            CallPostDone(HXR_OK);
        }

        if (m_bResponseReadyPending)
        {
            IHXBuffer* pContentBuffer = new CHXBuffer();

            pContentBuffer->AddRef();
            pContentBuffer->Set((UCHAR*) (const char*) m_strResultContent,
                            (ULONG32) m_strResultContent.GetLength());

            m_bResponseReadyPending = FALSE;

            CallResponseReady(HXR_OK, pContentBuffer);

            HX_RELEASE(pContentBuffer);
        }
    }

    /* Do we need to write more data */
    if (!theErr && (!m_bWriteDone || !m_bReadContentsDone) &&
        m_pCallback && !m_pCallback->m_bCallbackPending)
    {
            m_pCallback->m_bCallbackPending = TRUE;
            m_pCallback->m_ulPendingCallbackID =
                                        m_pScheduler->RelativeEnter(m_pCallback, 50);
    }

    // Preserve previous errors, if any..
    if (FAILED(m_LastError))
    {
        theErr = m_LastError;
    }
    else if (theErr)            // Report this error - crucial
    {
        /* This value is checked in every function */
        m_LastError = theErr;
    }

    if (m_bInitPending)
    {
        /* report InitDone if we have any error */
        if (theErr)
        {
            m_bInitPending = FALSE;
            m_bInitialized = TRUE;

            if (m_bInitResponsePending && m_pFileResponse)
            {
                m_bInitResponsePending = FALSE;
                m_pFileResponse->InitDone(HXR_FAILED);
            }
        }
        /* report InitDone if we have written some data, or
           if bWriteContents is TRUE. m_nContentWriitten=0 & m_bWriteContentsDone=TRUE
           happens when the repsonse contains no data, only headers
         */
        else if (m_nRequestWritten > 0 || m_bWriteDone)
        {
            m_bInitPending = FALSE;
            m_bInitialized = TRUE;

            if (m_bInitResponsePending && m_pFileResponse)
            {
                        m_bInitResponsePending = FALSE;
                        m_pFileResponse->InitDone(HXR_OK);
            }
        }
    }

    return theErr;
}


STDMETHODIMP  CHXHTTPPostObject::SetSize(ULONG32 ulLength)
{
    m_nContentLength = ulLength;
    return HXR_OK;
}


STDMETHODIMP CHXHTTPPostObject::Post(IHXBuffer* pBuffer)
{

    if (m_bInPostDone)
    {
        return HXR_UNEXPECTED;
    }

    // if a buffer was passed it will form the content of the POST
    // a NULL pBuffer simply means go ahead and send the headers now alone.
    if (pBuffer)
    {
        // save the buffer - only needed if we do a re-direct
        HX_RELEASE(m_pContentBuffer);
        m_pContentBuffer = new CHXBuffer;
        m_pContentBuffer->AddRef();
        m_pContentBuffer->Set(pBuffer->GetBuffer(), strlen((const char*) pBuffer->GetBuffer())+1);

        // append the content data as provided
        m_strRequest += pBuffer->GetBuffer();

#ifdef _WRITE_POST_DATA
        FILE* fl = fopen(LOG_FILE, "w+");
        if (fl)
        {
            fprintf(fl, "%s", (const char*) m_strRequest);
            fclose(fl);
        }
#endif

        // update the request string
        m_nTotalRequestSize = m_strRequest.GetLength();
    }

    m_bWriteDone = FALSE;

    if (m_pCallback && !m_pCallback->m_bCallbackPending)
    {
        m_pCallback->m_bCallbackPending     = TRUE;
        m_pCallback->m_ulPendingCallbackID  =
                        m_pScheduler->RelativeEnter(m_pCallback, 0);
    }

    return HXR_OK;
}

STDMETHODIMP CHXHTTPPostObject::GetResponse()
{
    if (m_bInResponseReady)
    {
        return HXR_UNEXPECTED;
    }

    if (!m_bWriteDone)
    {
        return HXR_UNEXPECTED;
    }

    m_bResponseReadyPending = TRUE;

    if (m_pCallback && !m_pCallback->m_bCallbackPending)
    {
        m_pCallback->m_bCallbackPending     = TRUE;
        m_pCallback->m_ulPendingCallbackID  =
                        m_pScheduler->RelativeEnter(m_pCallback, 0);
    }

    return HXR_OK;
}


/*
 *      IHXTCPResponse methods
 */
STDMETHODIMP CHXHTTPPostObject::ConnectDone(HX_RESULT status)
{
    if (FAILED(status))
    {
        m_LastError = HXR_INVALID_HOST;
        HX_RELEASE(m_pSocket);
    }

    // If there is a pending callback, be sure to remove it!
    // (there might be the connection timeout callback pending)
    if (m_pCallback && m_pCallback->m_ulPendingCallbackID && m_pScheduler)
    {
        m_pScheduler->Remove(m_pCallback->m_ulPendingCallbackID);
    }

    m_pCallback->m_bCallbackPending = TRUE;
    m_pCallback->m_ulPendingCallbackID = m_pScheduler->RelativeEnter(m_pCallback, 50);

    // connection is made
    m_bConnectDone = TRUE;

    return HXR_OK;
}


STDMETHODIMP CHXHTTPPostObject::ReadDone(HX_RESULT   status,
                                           IHXBuffer* pBuffer)
{
    HX_RESULT   theErr  = HXR_OK;
    UINT16      size    = 0;

#ifdef _MACINTOSH
    if (m_pInterruptState && m_pInterruptState->AtInterruptTime())
    {
        if (m_pCallback && !m_pCallback->m_bCallbackPending)
        {
            m_pCallback->m_bCallbackPending = TRUE;
            m_pCallback->m_ulPendingCallbackID  =
                                        m_pScheduler->RelativeEnter(m_pCallback, 0);
        }

        HX_ASSERT(m_pReadDoneBuffer == NULL && m_bReadDoneToBeProcessed == FALSE);

        m_pReadDoneBuffer = pBuffer;
        if (m_pReadDoneBuffer)
        {
            m_pReadDoneBuffer->AddRef();
        }

        m_uReadDoneStatus = status;
        m_bReadDoneToBeProcessed = TRUE;
        return HXR_OK;
    }
#endif


    m_bTCPReadDonePending = FALSE;

    if (pBuffer)
    {
        size = (UINT16)  pBuffer->GetSize();
    }

    if (status == HXR_OK && size>0)
    {
        // If we've already read the header. then we
        // know we can just slap the contents onto the
        // end of the results buffer.
        if (m_bReadHeaderDone)
        {
            // We are using m_strResultHeader as a Binary Buffer!!!!!
            UINT32 ulLength = m_strResultContent.GetLength();
            char* pszContent = m_strResultContent.GetBuffer(ulLength + size + 1);

            // Append New Chunk
            memcpy(pszContent + ulLength, pBuffer->GetBuffer(), size);

            // Force to new length (including possible bin data at end)
            m_strResultContent.ReleaseBuffer(ulLength + size);

            // see if we are done
            if (m_bKnowContentSize && m_nContentRead >= m_nContentSize)
            {
                m_bReadContentsDone = TRUE;
                m_bPostResponseReady = TRUE;
            }
        }
        else
        {
            // We are using m_strResultHeader as a Binary Buffer!!!!!
            UINT32 ulLength = m_strResultHeader.GetLength();

            // Grow Buffer..
            char* szHeader = m_strResultHeader.GetBuffer(ulLength + size + 1);

            // Append New Chunk
            memcpy(szHeader + ulLength, pBuffer->GetBuffer(), size);

            // Force to new length (including possible bin data at end)
            m_strResultHeader.ReleaseBuffer(ulLength + size);

	    BOOL bMsgTooLarge = FALSE;
            HTTPParser Parser;
            HTTPResponseMessage* pMessage = 0;

            UINT32 ulHeaderLength = m_strResultHeader.GetLength();

            // Parse headers from message
            pMessage = (HTTPResponseMessage*)Parser.parse((const char*)m_strResultHeader, ulHeaderLength, bMsgTooLarge);
            if (pMessage && pMessage->tag() == HTTPMessage::T_RESP)
            {
                // We now have the entire header!
                //
                m_bReadHeaderDone = TRUE;

                // XXXkshoop YUCK!!! copying the headers to the response
                // var in the request object
                //
                IHXValues* pResponseHeaders = new CHXHeader();
                pResponseHeaders->AddRef();

                MIMEHeaderValue*        pHeaderValue = NULL;
                MIMEHeader*             pHeader = pMessage->getFirstHeader();

                while (pHeader)
                {
                    pHeaderValue = pHeader->getFirstHeaderValue();

                    CHXString HeaderString;

                    while (pHeaderValue)
                    {
                        CHXString TempString;

                        pHeaderValue->asString(TempString);

                        HeaderString += TempString;

                        pHeaderValue = pHeader->getNextHeaderValue();

                        if (pHeaderValue)
                        {
                            HeaderString += ", ";
                        }
                    }

                    IHXBuffer* pBuffer = new CHXBuffer();
                    pBuffer->AddRef();
                    pBuffer->Set
                    (
                        (const BYTE*)((const char*)HeaderString),
                        HeaderString.GetLength()+1
                    );
                    pResponseHeaders->SetPropertyCString
                    (
                        pHeader->name(),
                        pBuffer
                    );
                    HX_RELEASE(pBuffer);

                    pHeader = pMessage->getNextHeader();
                }

                IHXBuffer*      pCookie = NULL;

                if (m_pRequest)
                {
                    IHXValues* pOldResponseHeaders = NULL;

                    if (HXR_OK ==
                        m_pRequest->GetResponseHeaders(pOldResponseHeaders) &&
                        pOldResponseHeaders)
                    {
                        CHXHeader::mergeHeaders(pOldResponseHeaders,
                            pResponseHeaders);
                    }
                    else
                    {
                        theErr = m_pRequest->SetResponseHeaders(pResponseHeaders);
                    }
                    HX_RELEASE(pOldResponseHeaders);
                }
                HX_RELEASE(pResponseHeaders);

                UINT32 ulHTTPStatus = atoi(pMessage->errorCode());

                if(pMessage->majorVersion() > 0)
                {
                    switch(ulHTTPStatus)
                    {
                    case 200: // Success
                        {
                            theErr = _HandleSuccess(pMessage, ulHeaderLength);
                        }
                        break;
                    case 400: // Fail
                    case 404: // Not Found
                    case 401: // Not Authorized
                    case 407: // Proxy Authentication Required
                        {
                            theErr = _HandleFail(ulHTTPStatus);
                        }
                        break;
                    case 301: // Redirect
                    case 302: // Redirect
                        {
                            theErr = _HandleRedirect(pMessage);
                        }
                        break;
                    default:
                        {
                            theErr = _HandleFail(400);
                        }
                        break;
                    };
                }
            }

            delete pMessage;
        }
    }
    else        // status != HXR_OK (read failed....)
    {
        // XXXBJP : Old behaviour to support Mac TCP bug (where initial
        // read returns no data, but subsequent OK) removed since it
        // broke httpfsys. If MacTCP bug still needs to be handled, then
        // MACTCP-only implementation should handle it.
        m_bReadContentsDone = TRUE;
        m_bPostResponseReady = TRUE;

        if(m_bKnowContentSize && m_nContentRead < m_nContentSize)
        {
            theErr = HXR_SERVER_DISCONNECTED;
        }
        else
        {
            // an error was recevied in ReadDone(), so nothing more is coming
            // from the server. Have we parsed the header yet??
            if (!m_bReadHeaderDone)
            {
                // We couldn't recognize the headers, so fail.
                theErr = _HandleFail(400);
            }
            else
            {
                // either we read all the data we knew we had to, or we didn't
                // know how much data to read in the first place (!m_bKnowContentSize)
                // If we didn't know content size, we have to assume we're ok.

                theErr = HXR_OK;
            }
        }
    }

    if(SUCCEEDED(m_LastError))
    {
        m_LastError = theErr;
    }

    LOGX ((szDbgTemp, "    Read %5d of %5d bytes of '%s'",
           m_nContentRead,
           m_bKnowContentSize ? m_nContentSize : 0,
           m_pFilename));

    return HXR_OK;
}

//
//
//
//
//
//
HX_RESULT
CHXHTTPPostObject::_HandleRedirect(HTTPResponseMessage* pMessage)
{
    HX_RESULT   theErr = HXR_OK;
    CHXString   sLocation;

    sLocation = pMessage->getHeaderValue("location");
    if(!sLocation.IsEmpty())
    {
        if(m_pRequest)
        {
            m_pRequest->SetURL(sLocation);

            // Keep the request alive..
            m_pRequest->AddRef();

            // Reset this object to new request..
            SetRequest(m_pRequest);

            // Release the "keep alive" ref..
            m_pRequest->Release();

            // Cleanup for reopen..
            //

            // Redirect is absolute..
            delete [] m_szBaseURL;
            m_szBaseURL = NULL;

            // Need new connection (regardless of m_bKeepAlive)
            HX_RELEASE(m_pSocket);

            m_bKeepAlive = TRUE;
            m_bConnectDone = FALSE;

            LOG ("_HandleRedirect()");

            theErr = _ReOpen();
        }
    }

    return theErr;
}

//
//
//
//
//
//
HX_RESULT
CHXHTTPPostObject::_HandleFail(UINT32 ulHTTPError)
{
    HX_RESULT theErr = HXR_FAIL;

    if (ulHTTPError != 400)
    {
        theErr = HXR_DOC_MISSING;
    }

    if(m_bInitPending)
    {
        // since any of these callbacks could have us killed
        AddRef();

        if (m_bInitResponsePending && m_pFileResponse)
        {
            m_bInitResponsePending = FALSE;
            m_pFileResponse->InitDone(theErr);
        }

        Release();
    }

    if (m_bResponseReadyPending)
    {
        m_bResponseReadyPending = FALSE;
        CallResponseReady(theErr, NULL);
    }

    if (m_bPostDonePending)
    {
        m_bPostDonePending = FALSE;
        CallPostDone(theErr);
    }

    return theErr;
}

//
//
//
//
//
//
HX_RESULT
CHXHTTPPostObject::_HandleSuccess(HTTPResponseMessage* pMessage,
    UINT32 ulHeaderLength)
{
    HX_RESULT   theErr = HXR_OK;

    m_bDisableConnectionTimeOut = FALSE;

    if (SUCCEEDED(theErr))
    {
        // Find the content length to support percent done
        // handling...
        UINT32 ulValue = 0;
        if (pMessage->getHeaderValue("content-length", ulValue))
        {
            m_nContentSize = ulValue;
            m_bKnowContentSize = TRUE;
        }

        // Handle adding any trailing content data to the start
        // of the content buffers...
        INT32 nContentLen = m_strResultHeader.GetLength() - ulHeaderLength;
        if (nContentLen > 0)
        {
            m_strResultContent = (char*) (m_strResultHeader.GetBuffer(1)
                + ulHeaderLength);
            m_nContentRead += nContentLen;
        }

        // See if the data trailing after the header contains all
        // the content (or note the fact if their won't be any contents
        // data at all...)
        if(m_bKnowContentSize && m_nContentRead >= m_nContentSize)
        {
            m_bReadContentsDone = TRUE;
            m_bPostResponseReady = TRUE;
        }
    }

    return theErr;
}

STDMETHODIMP CHXHTTPPostObject::WriteReady(HX_RESULT status)
{
    if (HXR_OK == status)
    {
        if (m_bWriteDone && m_bPostDonePending)
        {
            m_bReadContentsDone = FALSE;

            if (m_pCallback && !m_pCallback->m_bCallbackPending)
            {
                m_pCallback->m_bCallbackPending = TRUE;
                m_pCallback->m_ulPendingCallbackID
                    = m_pScheduler->RelativeEnter(m_pCallback, 0);
            }
        }
    }

    return HXR_OK;
}

STDMETHODIMP CHXHTTPPostObject::Closed(HX_RESULT  status)
{
    return HXR_OK;
}

HX_RESULT
CHXHTTPPostObject::_ReOpen()
{
    LOG("_ReOpen");
    LOGX((szDbgTemp, "    URL='%s'", m_pFilename));

    m_bWriteDone = FALSE;
    m_nRequestWritten = 0;
    m_nTotalRequestSize = 0;
    m_bKnowContentSize = FALSE;
    m_nContentSize = 0;
    m_nContentRead = 0;
    m_nHeaderRead = 0;
    m_bReadHeaderDone = FALSE;
    m_bReadContentsDone = FALSE;
    m_strResultHeader = "";
    m_strRequest = "";
    m_bPostResponseReady = FALSE;

    m_ulCurrentWritePosition = 0;
    m_bTCPReadDonePending = FALSE;

    if (m_pCallback && m_pCallback->m_bCallbackPending && m_pScheduler)
    {
        m_pScheduler->Remove(m_pCallback->m_ulPendingCallbackID);
        m_pCallback->m_ulPendingCallbackID  = 0;
        m_pCallback->m_bCallbackPending = FALSE;
    }

    m_bOpenFilePending = TRUE;

    m_pCallback->m_bCallbackPending = TRUE;
    m_pCallback->m_ulPendingCallbackID
        = m_pScheduler->RelativeEnter(m_pCallback, 50);

    return HXR_OK;
}



/************************************************************************
 *      Method:
 *          IHXTimeSettings::Get/SetConnnectionTimeout
 *      Purpose:
 *          Get/Set the connection timeout setting, in seconds
 */
STDMETHODIMP
CHXHTTPPostObject::GetConnectionTimeout(REF(UINT32)   /*OUT*/ nSeconds)
{
    nSeconds = m_nConnTimeout;
    return HXR_OK;
}

STDMETHODIMP
CHXHTTPPostObject::SetConnectionTimeout(UINT32 /*IN*/ nSeconds)
{
    m_nConnTimeout = nSeconds;
    return HXR_OK;
}

/************************************************************************
 *      Method:
 *          IHXTimeSettings::Get/SetServerTimeout
 *      Purpose:
 *          Get/Set the server timeout setting, in seconds
 */
STDMETHODIMP
CHXHTTPPostObject::GetServerTimeout(REF(UINT32) /*OUT*/ nSeconds)
{
    nSeconds = m_nServerTimeout;
    return HXR_OK;
}

STDMETHODIMP
CHXHTTPPostObject::SetServerTimeout(UINT32 /*IN*/ nSeconds)
{
    m_nServerTimeout = nSeconds;
    return HXR_OK;
}



// HTTPPostObjCallback
BEGIN_INTERFACE_LIST(HTTPPostObjCallback)
    INTERFACE_LIST_ENTRY(IID_IHXCallback, IHXCallback)
END_INTERFACE_LIST

HTTPPostObjCallback::HTTPPostObjCallback()
    : m_pHTTPPostObject(NULL)
    , m_bCallbackPending(FALSE)
    , m_ulPendingCallbackID(0)
{
}

void
HTTPPostObjCallback::InitObject(CHXHTTPPostObject* pHTTPPostObject)
{
    m_pHTTPPostObject = pHTTPPostObject;
}

HTTPPostObjCallback::~HTTPPostObjCallback()
{
}

/*
 *      IHTTPPostObjCallback methods
 */
STDMETHODIMP HTTPPostObjCallback::Func(void)
{
    /*
     * To ensure the CHXHTTPPostObject does not get destroyed before we are
     * done with it, we AddRef()/Release() it here
     */
    if (m_pHTTPPostObject)
    {
        CHXHTTPPostObject* pPostObject = m_pHTTPPostObject;
        pPostObject->AddRef();

        m_bCallbackPending      = FALSE;
        m_ulPendingCallbackID   = 0;

        m_pHTTPPostObject->ProcessIdle();

        HX_RELEASE(pPostObject);
    }

    return HXR_OK;
}



BEGIN_INTERFACE_LIST(HTTPPostTCPResponse)
    INTERFACE_LIST_ENTRY(IID_IHXTCPResponse, IHXTCPResponse)
END_INTERFACE_LIST

HTTPPostTCPResponse::HTTPPostTCPResponse()
    : m_pOwner(NULL)
{
}

void
HTTPPostTCPResponse::InitObject(CHXHTTPPostObject* pOwner)
{
    m_pOwner = pOwner;
}

HTTPPostTCPResponse::~HTTPPostTCPResponse()
{
    m_pOwner = NULL;
}

/*
 *      IHXTCPResponse methods
 */

STDMETHODIMP HTTPPostTCPResponse::ConnectDone(HX_RESULT status)
{
    return m_pOwner ? m_pOwner->ConnectDone(status) : HXR_OK;
}

STDMETHODIMP HTTPPostTCPResponse::ReadDone( HX_RESULT   status,
                                        IHXBuffer* pBuffer)
{
    return m_pOwner ? m_pOwner->ReadDone(status, pBuffer) : HXR_OK;
}

STDMETHODIMP HTTPPostTCPResponse::WriteReady(HX_RESULT status)
{
    return m_pOwner ? m_pOwner->WriteReady(status) : HXR_OK;
}

STDMETHODIMP HTTPPostTCPResponse::Closed(HX_RESULT  status)
{
    return m_pOwner ? m_pOwner->Closed(status) : HXR_OK;
}


