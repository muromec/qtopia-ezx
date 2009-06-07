/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: chxhttp.cpp,v 1.2 2005/06/15 02:40:23 rggammon Exp $
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
 * Contributor(s): Stanislav Bobrovskiy
 * 
 * ***** END LICENSE BLOCK ***** */

#include "hxcom.h"
#include "hxstring.h"
#include "hxtset.h"
#include "ihxpckts.h"

#include "chxhttp.h"


#define NO_MAX_BUFFER_SIZE	0 // No maximum buffer size

// IUnknown interface listing
BEGIN_INTERFACE_LIST(CHXHttp)
    INTERFACE_LIST_ENTRY(IID_IHXHttp, IHXHttp)
    INTERFACE_LIST_ENTRY(IID_IHXHttp2, IHXHttp2)
    INTERFACE_LIST_ENTRY(IID_IHXHttpInitialize, IHXHttpInitialize)
END_INTERFACE_LIST


CHXHttp::CHXHttp() :
    m_pCCF(NULL),
    m_pIHttpResponse(NULL),
    m_pFileResponse(NULL),
    m_bGetDoneSent(FALSE),
    m_pRequest(NULL),
    m_pFileObject(NULL),
    m_ulBufferSize(NO_MAX_BUFFER_SIZE),
    m_ulConnTimeout(0),
    m_pHttpFS(NULL),
    m_pRedirectResponse(NULL),    
    m_bIsHttpPost(FALSE),
    m_pPostDataBuffer(NULL),
    m_bInitialized(FALSE),
    m_statResult(HXR_FAIL),
    m_nContentLength(0),
    m_nBytesWritten(0),
    m_bChunkedResponse(FALSE)
{
}

CHXHttp::~CHXHttp()
{
    if(m_bInitialized)
    {
        /* We have an Initialize()'d http connection, kill it. */
        Terminate();
    }

    if(m_pHttpFS || m_pCCF)
    {
        Destroy();
    }
}

// The Init called from the engine (as opposed IHXHttp::Initialize,
// called by the http user to set the request).
STDMETHODIMP
CHXHttp::Init(IUnknown* pContext)
{
    HX_RESULT res;
    IUnknown* pUnkHTTP = NULL;
    IHXPlugin2Handler* pPlugin2Handler = NULL;

    if(pContext == NULL)
    {
        return HXR_INVALID_PARAMETER;
    }

    if( m_bInitialized )
    {
        // Already initialized
        return HXR_FAIL;
    }
    
    res = pContext->QueryInterface( IID_IHXCommonClassFactory,
                                   (void**)&m_pCCF );
    if( SUCCEEDED( res ) )
    {
        res = pContext->QueryInterface( IID_IHXPlugin2Handler,
                                        (void**)&pPlugin2Handler );
    }
    
    if( SUCCEEDED( res ) )
    {
        res = pPlugin2Handler->FindPluginUsingStrings( "PluginType",   "PLUGIN_FILE_SYSTEM",
                                                       "FileProtocol", "http",
                                                       NULL,           NULL,
                                                       pUnkHTTP );
        HX_RELEASE( pPlugin2Handler );
    }

    if( SUCCEEDED( res ) )
    {        
        res = pUnkHTTP->QueryInterface( IID_IHXPlugin,
                                        (void**) &m_pHttpFS );

        HX_RELEASE(pUnkHTTP);
    }

    if( SUCCEEDED( res ) )
    {        
        res = m_pHttpFS->InitPlugin( pContext );
    }

    if( FAILED( res ) )
    {
        HX_RELEASE( m_pCCF );
        HX_RELEASE( m_pHttpFS );        
    }
    
    return res;
}

STDMETHODIMP
CHXHttp::Destroy(THIS)
{
    if(!m_bInitialized)
    {
        return HXR_NOT_INITIALIZED;
    }

    Terminate();
    HX_RELEASE(m_pHttpFS);
    HX_RELEASE(m_pCCF);    

    return HXR_OK;
}


STDMETHODIMP_(BOOL) 
    CHXHttp::Initialize(IUnknown* pContext)
{
    HX_RESULT res = HXR_FAIL;        

    if(!m_pCCF || !m_pHttpFS)
    {
        return HXR_NOT_INITIALIZED;
    }
    
    HX_RELEASE(m_pIHttpResponse);
    HX_RELEASE(m_pRedirectResponse);
        
    if(pContext)
    {
	res = pContext->QueryInterface(IID_IHXHttpResponse,(void**)&m_pIHttpResponse);

        /* Also query for IHXHTTPRedirectResponse -- if it doesn't exist, we'll
           handle redirects transparently. */
        pContext->QueryInterface(IID_IHXHTTPRedirectResponse, (void **)&m_pRedirectResponse);
    }

    if(SUCCEEDED(res))
    {
        m_bInitialized = TRUE;
    }
    
    return SUCCEEDED(res);
}


STDMETHODIMP 
CHXHttp::Terminate()
{
    if(!m_bInitialized)
    {
        return HXR_NOT_INITIALIZED;
    }

    HX_RELEASE(m_pIHttpResponse);
    HX_RELEASE(m_pRequest);
    HX_RELEASE(m_pPostDataBuffer);
    HX_RELEASE(m_pRedirectResponse);

    if(m_pFileObject)
    {
	m_pFileObject->Close();
        HX_RELEASE(m_pFileObject);
    }
    
    HX_RELEASE(m_pFileResponse);

    m_bInitialized = FALSE;

    return HXR_OK;
}


STDMETHODIMP_(BOOL)
    CHXHttp::Get(const char* szURL)
{
    HX_RESULT nRetVal;
    m_bIsHttpPost = FALSE;

    if(!m_bInitialized)
    {
        return FALSE;
    }
    
    nRetVal = SendHTTPRequest(szURL, 0, 0, NULL);
    
    return SUCCEEDED(nRetVal);
}

STDMETHODIMP
CHXHttp::Post(const char* szURL, UINT32 nPostDataSize)
{
    if(!m_bInitialized)
    {
        return HXR_NOT_INITIALIZED;
    }

    m_bIsHttpPost = TRUE;

    // We must call InitPost on the HttpResponse interface before any call to
    // GetPostData.
    if(m_pIHttpResponse)
    {
	IHXHttpResponse2* pResponse2 = NULL;
	m_pIHttpResponse->QueryInterface(IID_IHXHttpResponse2, (void**)&pResponse2);

	if(pResponse2)
        {
	    pResponse2->InitPost();
        }

	HX_RELEASE(pResponse2);
    }

    return SendHTTPRequest(szURL, nPostDataSize, 0, NULL);
}

STDMETHODIMP
CHXHttp::GetFromPosition(const char* szURL, UINT32 nPosition, const char* pLastModified)
{
    if(!m_bInitialized)
    {
        return HXR_NOT_INITIALIZED;
    }

    return SendHTTPRequest(szURL, 0, nPosition, pLastModified);
}

STDMETHODIMP
CHXHttp::SendHTTPRequest(const char* szURL,
                         UINT32 nPostDataSize, 
			 UINT32 nPosition,
                         const char* pLastModified)
{
    HX_RESULT res;
    IHXValues* pHeaders = NULL;

    m_bGetDoneSent = FALSE;

    // Step 1: Create request and header (IHXValues) objects
    res = m_pCCF->CreateInstance( IID_IHXRequest,
                                  (void**)&m_pRequest );

    if(SUCCEEDED(res))
    {
        res = m_pCCF->CreateInstance( IID_IHXValues,
                                      (void**)&pHeaders );
    }
       
    // Step 2: Set headers   
    if(SUCCEEDED(res))
    {
        m_pRequest->SetURL(szURL);

        if(nPostDataSize)
        {
            IHXBuffer* pSizeBuffer = NULL;
            res = m_pCCF->CreateInstance( IID_IHXBuffer,
                                          (void**)&pSizeBuffer );

            if(SUCCEEDED(res))
            {
                CHXString strDataSize;
                strDataSize.Format("%d", nPostDataSize);
                pSizeBuffer->Set((Byte*)(const char*)strDataSize, strDataSize.GetLength() + 1);

                pHeaders->SetPropertyCString("Content-length", pSizeBuffer);
                HX_RELEASE(pSizeBuffer);
            }
        }

        if(SUCCEEDED(res) && nPosition != 0)
        {
            IHXBuffer* pRangeBuffer = NULL;
            res = m_pCCF->CreateInstance( IID_IHXBuffer,
                                          (void**)&pRangeBuffer );
            if(SUCCEEDED(res))
            {
                CHXString strRange;
                strRange.Format("bytes=%d-", nPosition);
                pRangeBuffer->Set((Byte*)(const char*)strRange, strRange.GetLength() + 1);

                pHeaders->SetPropertyCString("Range", pRangeBuffer);
                HX_RELEASE(pRangeBuffer);

                if(pLastModified && pLastModified[0])
                {
                    // Only makes sence if nPosition != 0.
                    res = m_pCCF->CreateInstance( IID_IHXBuffer,
                                                  (void**)&pRangeBuffer );
                    if(SUCCEEDED(res))
                    {
                        pRangeBuffer->Set((Byte*)pLastModified, strlen(pLastModified) + 1);

                        pHeaders->SetPropertyCString("If-Range", pRangeBuffer);
                        HX_RELEASE(pRangeBuffer);
                    }
                }
            }
        }

        if(SUCCEEDED(res))
        {
            // Allow the client to set additional HTTP headers
            IHXHttpResponse2* pResponse2 = NULL;

            res = m_pIHttpResponse->QueryInterface(IID_IHXHttpResponse2, (void**)&pResponse2);

            if(SUCCEEDED(res))
            {
                pResponse2->SetHeaders(pHeaders);
                HX_RELEASE(pResponse2);
            }            
        }

        if(SUCCEEDED(res))
        {
            m_pRequest->SetRequestHeaders(pHeaders);
        }
        
        HX_RELEASE(pHeaders);
    }

    // Step 3: CreateFile a m_pFileObject (prepare http connection)
    if(SUCCEEDED(res))
    {
	HX_RELEASE(m_pFileObject);

	IHXFileSystemObject* pFileSystem = NULL;

        HX_ASSERT(m_pHttpFS != NULL); 

        res = m_pHttpFS->QueryInterface(IID_IHXFileSystemObject, (void**)&pFileSystem);
	if(SUCCEEDED(res))
	{
	    IUnknown* pUnknown = NULL;
	    res = pFileSystem->CreateFile(&pUnknown);
	    if(SUCCEEDED(res))
            {
		res = pUnknown->QueryInterface(IID_IHXFileObject, (void**)&m_pFileObject);
                HX_RELEASE(pUnknown);
            }
            HX_RELEASE(pFileSystem);
	}   
    }

    // Step 4: Set the request (created in step 1)
    if(SUCCEEDED(res))
    {
	IHXRequestHandler* pRequestHandler = NULL;
    	res = m_pFileObject->QueryInterface(IID_IHXRequestHandler, (void**)&pRequestHandler);

	if(SUCCEEDED(res))
        {
	    pRequestHandler->SetRequest(m_pRequest);
            HX_RELEASE(pRequestHandler);
        }
    }

    // Step 5: Set a connection timeout if applicable
    if(SUCCEEDED(res))
    {
	// see if we should set a connection timeout
	if(m_ulConnTimeout != 0)
	{
	    IHXTimeoutSettings* pITimeout = NULL;
	    
	    if(m_pFileObject->QueryInterface(IID_IHXTimeoutSettings, (void**)&pITimeout) == HXR_OK)
    	    {
		pITimeout->SetConnectionTimeout(m_ulConnTimeout);
		HX_RELEASE(pITimeout);
    	    }
	}
    }

    // Step 6: Create our m_pFileResponse (if it doesn't exist)
    if(SUCCEEDED(res))
    {
        if (!m_pFileResponse)
        {
            m_pFileResponse = new CHXHttpFileResponse(this);
            HX_ADDREF(m_pFileResponse);
        }
    }

    // Step 7: Set up our redirect handler
    if(SUCCEEDED(res))
    {
	// Setup the redirect response that will be used to reinitialize the
	// http post data provider in case we have a redirect.
        IHXHTTPRedirect* pRedirect = NULL;
        IHXHTTPRedirectResponse* pRedirectResponse = NULL;

        res = m_pFileObject->QueryInterface(IID_IHXHTTPRedirect, (void**)&pRedirect);
        if(SUCCEEDED(res))
        {
            res = m_pFileResponse->QueryInterface(IID_IHXHTTPRedirectResponse, (void**)&pRedirectResponse);
        }        
        if(SUCCEEDED(res))
        {
            pRedirect->SetResponseObject(pRedirectResponse);
        }
        HX_RELEASE(pRedirect);
        HX_RELEASE(pRedirectResponse);
    }

    // Step 8: Connect
    if(SUCCEEDED(res))
    {
        res = m_pFileObject->Init(HX_FILE_READ, m_pFileResponse);
    }

    // Step 9: Report any failure              
    if(FAILED(res))
    {
	SendOnGetDone(FALSE);
    }

    return res;
}


STDMETHODIMP_(UINT32) 
    CHXHttp::GetBufferSize()
{
    return m_ulBufferSize;
}

STDMETHODIMP 
CHXHttp::SetBufferSize(UINT32 nBufferSize)
{
    m_ulBufferSize = nBufferSize;

    return HXR_OK;
}

STDMETHODIMP 
CHXHttp::SetConnectionTimeout(UINT32 nSeconds)
{
    m_ulConnTimeout = nSeconds;

    return HXR_OK;
}

void 
CHXHttp::SendOnGetDone(BOOL bSuccess)
{
    if (m_pIHttpResponse && !m_bGetDoneSent)
    {
	m_pIHttpResponse->OnGetDone(bSuccess);
    }
    m_bGetDoneSent = TRUE;
}

void 
CHXHttp::SendOnDataReceived(IHXBuffer* pBuffer)
{
    if (m_pIHttpResponse)
    {
	m_pIHttpResponse->OnDataReceived(pBuffer);
    }
}

void 
CHXHttp::SendOnHeaders(IHXValues* pHeaders)
{
    if (m_pIHttpResponse)
    {
	m_pIHttpResponse->OnHeaders(pHeaders);
    }
}

HX_RESULT 
CHXHttp::GetPostData()
{
    HX_RESULT nRetVal = HXR_OK;

    if (m_pPostDataBuffer == NULL)
    {
	IHXHttpResponse2* pResponse2 = NULL;
	if (m_pIHttpResponse)
        {
	    m_pIHttpResponse->QueryInterface(IID_IHXHttpResponse2,
                                             (void**) &pResponse2);
        }

	if(pResponse2)
        {
	    nRetVal = pResponse2->GetPostData(m_pPostDataBuffer);
        }

	HX_RELEASE(pResponse2);
    }

    if(nRetVal == HXR_OK && m_pFileObject)
    {
	HX_ASSERT(m_pPostDataBuffer != NULL);
	nRetVal = m_pFileObject->Write(m_pPostDataBuffer);
	if (SUCCEEDED(nRetVal))
	{
	    // We don't need to keep the buffer around, the write succeeded
	    HX_RELEASE(m_pPostDataBuffer);
	}
    }

    return nRetVal;
}


// IUnknown interface listing
BEGIN_INTERFACE_LIST_NOCREATE(CHXHttpFileResponse)
    INTERFACE_LIST_ENTRY(IID_IHXFileResponse, IHXFileResponse)
    INTERFACE_LIST_ENTRY(IID_IHXFileStatResponse, IHXFileStatResponse)
    INTERFACE_LIST_ENTRY(IID_IHXHTTPRedirectResponse, IHXHTTPRedirectResponse)    
END_INTERFACE_LIST

CHXHttpFileResponse::CHXHttpFileResponse(CHXHttp* pHttp) :
    m_pHttp(NULL),
    m_bReadSucceeded(FALSE)
{
    m_pHttp = pHttp;
    if (m_pHttp)
    {
    	HX_ADDREF(m_pHttp);
    }    
}

CHXHttpFileResponse::~CHXHttpFileResponse()
{
    if(m_pHttp)
    {
        HX_RELEASE(m_pHttp);
    }
}

STDMETHODIMP
CHXHttpFileResponse::InitDone(HX_RESULT status)
{
    if(status == HXR_OK || status == HXR_RESOURCE_PARTIALCOPY)
    {
	IHXHttpResponse2* pResponse2 = NULL;
	if (m_pHttp && m_pHttp->m_pIHttpResponse)
        {
	    m_pHttp->m_pIHttpResponse->QueryInterface(IID_IHXHttpResponse2, (void**)&pResponse2);
        }

	if(pResponse2)
        {
	    pResponse2->SetDataType(status == HXR_OK ? DATA_COMPLETE : DATA_PARTIAL);
        }

	HX_RELEASE(pResponse2);
	status = HXR_OK;
    }

    HX_RESULT result = status;
    this->AddRef();

    /*
     * Print out the headers
     */

    IHXValues* pHeaders = 0;
    m_pHttp->m_nContentLength = 0;
    m_pHttp->m_statResult = HXR_FAIL;
    m_pHttp->m_bChunkedResponse = FALSE;

    if (HXR_OK == result)
    {
        result = m_pHttp->m_pRequest->GetResponseHeaders(pHeaders);
    }

    if (pHeaders && result == HXR_OK)
    {
        // use stat to get content length (file size).
        IHXFileStat* pIStat=NULL;
        m_pHttp->m_pFileObject->QueryInterface(IID_IHXFileStat,(void**)&pIStat);
        if (pIStat)
        {
            // since the headers have been received, stat will callback
            // immediately
            pIStat->Stat((IHXFileStatResponse*)this);

            pIStat->Release();
        }

        // response callback with the headers.
        m_pHttp->SendOnHeaders(pHeaders);

        pHeaders->Release();
    }

    // Possible cases are:
    //  - http response contained a non-zero content length (HXR_OK, the length)
    //  - http response contained a zero content length     (HXR_OK, 0)
    //  - http response contained an unknown content length (HXR_FAIL, HX_UNKNOWN_FILE_SIZE)
    //  - failure                                           (other error)
    
    if ( SUCCEEDED(m_pHttp->m_statResult) && m_pHttp->m_nContentLength > 0 )
    {
        UINT32 ulBufferSize = m_pHttp->GetBufferSize();

        if(ulBufferSize == NO_MAX_BUFFER_SIZE)
        {
            m_pHttp->m_nTotalBytesToRead = m_pHttp->m_nContentLength;
        }
        else
        {
            m_pHttp->m_nTotalBytesToRead = HX_MIN(m_pHttp->m_nContentLength, ulBufferSize);
        }

        // Start reading
        result = m_pHttp->m_pFileObject->Read(m_pHttp->m_nTotalBytesToRead);
    }
    else if ( SUCCEEDED(result) && m_pHttp->m_nContentLength == 0 )
    {
        // this case is just headers and no data
	m_pHttp->SendOnGetDone(TRUE);
	if (m_pHttp->m_pFileObject)
	{
	    result = m_pHttp->m_pFileObject->Close();
	    m_pHttp->m_pFileObject->Release();
	    m_pHttp->m_pFileObject = NULL;
	}
    }
    else if ( m_pHttp->m_statResult == HXR_FAIL &&
              m_pHttp->m_nContentLength == HX_UNKNOWN_FILE_SIZE )
    {
        m_pHttp->m_bChunkedResponse = TRUE;

        // Start reading
        result = m_pHttp->m_pFileObject->Read(m_pHttp->m_nTotalBytesToRead);        
    }
    else // other failure
    {
	m_pHttp->SendOnGetDone(FALSE);        
    }    

    this->Release();
    return result;
}

STDMETHODIMP
CHXHttpFileResponse::CloseDone(HX_RESULT status)
{
    m_pHttp->SendOnGetDone(SUCCEEDED(status));

    return HXR_OK;
}

STDMETHODIMP
CHXHttpFileResponse::ReadDone(HX_RESULT status, IHXBuffer* pBuffer)
{
    HX_RESULT result = status;
    this->AddRef();

    // XXXRGG: Need to detect end of a chunked response
    if (SUCCEEDED(result))
    {
	m_pHttp->SendOnDataReceived(pBuffer);

        m_pHttp->m_nBytesWritten += pBuffer->GetSize();
        
	if (!m_pHttp->m_bChunkedResponse &&
            (m_pHttp->m_nBytesWritten >= m_pHttp->m_nTotalBytesToRead))
	{
	    m_pHttp->m_pFileObject->Close();
	}
        else
        {
            m_pHttp->m_pFileObject->Read(m_pHttp->m_nTotalBytesToRead - m_pHttp->m_nBytesWritten);
        }
    }
    else
    {
        m_pHttp->m_pFileObject->Close();
    }

    this->Release();
    return result;
}

STDMETHODIMP
CHXHttpFileResponse::WriteDone(HX_RESULT status)
{
    HX_RESULT nRetVal = HXR_FAILED;

    if(m_pHttp)
    {
	nRetVal = m_pHttp->GetPostData();
    }

    return nRetVal;
}

STDMETHODIMP
CHXHttpFileResponse::SeekDone(HX_RESULT status)
{
    return HXR_NOTIMPL;
}


STDMETHODIMP
CHXHttpFileResponse::StatDone(
    HX_RESULT status,
    UINT32 ulSize,
    UINT32 ulCreationTime,
    UINT32 ulAccessTime,
    UINT32 ulModificationTime,
    UINT32 ulMode)
{
    m_pHttp->m_statResult = status;
    m_pHttp->m_nContentLength = ulSize;

    return HXR_OK;
}


STDMETHODIMP
CHXHttpFileResponse::RedirectDone(IHXBuffer* pURL)
{
    // Returning HXR_NOTIMPL indicates that the http file object should continue
    // doing the redirect.
    HX_RESULT res = HXR_NOTIMPL;

    // Since we will have to restart sending of the HTTP Post data, we need
    // to inform the client so that the next call to GetPostData will be from
    // the start of the data again.
    if(m_pHttp && m_pHttp->m_bIsHttpPost && m_pHttp->m_pIHttpResponse)
    {
	IHXHttpResponse2* pResponse2 = NULL;
	m_pHttp->m_pIHttpResponse->QueryInterface(IID_IHXHttpResponse2, (void**)&pResponse2);

	if(pResponse2)
        {
	    pResponse2->InitPost();
        }

	HX_RELEASE(pResponse2);
    }

    if(m_pHttp && m_pHttp->m_pRedirectResponse)
    {
        res = m_pHttp->m_pRedirectResponse->RedirectDone(pURL);
    }
    
    return res;
}


