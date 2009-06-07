/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: http.cpp,v 1.18 2005/02/25 21:30:41 darrick Exp $
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

#include <signal.h>
#include <sys/types.h>

#include "hxtypes.h"
#include "hxcom.h"
#include "hxerror.h"

#include "hxstring.h"
#include "debug.h"
#include "sio.h"
#include "client.h"
#include "hxstrutl.h"
#include "url.h"
#include "plgnhand.h"
#include "config.h"
#include "hxreg.h"

#include "chxpckts.h"
#include "ihxpckts.h"
#include "hxplugn.h"
#include "hxfiles.h"
#include "hxcomm.h" // XXXSMP not needed
#include "cbqueue.h"
#include "http_demux.h"
#include "httpprot.h"
#include "http.h"
#include "hxurl.h"
#include "servbuffer.h"


#include "hxheap.h"
#include "fsmanager.h"

#include "carray.h"

#define READSIZE 1024

#ifdef _DEBUG
#undef HX_THIS_FILE
static char HX_THIS_FILE[] = __FILE__;
#endif


STDMETHODIMP
HTTPFileResponse::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)(IHXFileResponse*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXFileResponse))
    {
        AddRef();
        *ppvObj = (IHXFileResponse*)this;
        return HXR_OK;
    }
    else if(IsEqualIID(riid, IID_IHXFileStatResponse))
    {
        AddRef();
        *ppvObj = (IHXFileStatResponse*)this;
        return HXR_OK;
    }
    else if(IsEqualIID(riid, IID_IHXThreadSafeMethods))
    {
        AddRef();
        *ppvObj = (IHXThreadSafeMethods*)this;
        return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32)
HTTPFileResponse::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32)
HTTPFileResponse::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

STDMETHODIMP_(UINT32)
HTTPFileResponse::IsThreadSafe()
{
    return HX_THREADSAFE_METHOD_FSR_READDONE;
}

HTTPFileResponse::HTTPFileResponse(HTTP* h)
{
    m_lRefCount = 0;
    m_pHTTP     = h;
    m_pFileStat = 0;
    m_file_mime = 0;
    m_bDefunct  = 0;
}

HTTPFileResponse::~HTTPFileResponse()
{
    if (m_pFileStat)
    {
        m_pFileStat->Release();
        m_pFileStat = 0;
    }

    if (m_file_mime)
    {
        m_file_mime->Release();
        m_file_mime = 0;
    }
}

STDMETHODIMP
HTTPFileResponse::InitDone (HX_RESULT status)
{
    if (m_bDefunct)
        return HXR_FAIL;

    if(status == HXR_OK)
    {
        switch(m_pHTTP->m_state)
        {
        case HS_GetFileInit:
        case HS_PostFileInit:
        {
            IHXFileObject* pFileObject = m_pHTTP->m_pFileObject;

            if(HXR_OK == pFileObject->QueryInterface(IID_IHXFileStat,
                                                     (void**)&m_pFileStat))
            {
                return m_pFileStat->Stat(this);
            }

            return StatDone(HXR_OK, 0, 0, 0, 0, 0);

        }
        default:
            return HXR_UNEXPECTED;
        }
    }
    else if(status == HXR_NOT_AUTHORIZED)
    {
        // Report "Access Denied"
        //
        if (m_pHTTP->m_pProtocol->statComplete(status, 0, 0, NULL))
        {
            m_pHTTP->m_pSock->SelectEvents(HX_SOCK_EVENT_READ|HX_SOCK_EVENT_WRITE|HX_SOCK_EVENT_CLOSE);
        }

        return HXR_OK;
    }
    else
    {
        m_pHTTP->m_pProtocol->statComplete(HXR_FILE_NOT_FOUND, 0, 0, NULL);
    }

    return status;
}

STDMETHODIMP
HTTPFileResponse::CloseDone (HX_RESULT status)
{
    if (m_bDefunct)
        return HXR_FAIL;

    if(m_pHTTP->m_pFileObject)
    {
        m_pHTTP->m_pFileObject->Release();
        m_pHTTP->m_pFileObject = NULL;
    }

    if (m_pHTTP->m_pClient)
    {
#ifdef PAULM_CLIENTAR
        REL_NOTIFY(m_pHTTP->m_pClient, 14);
#endif
        m_pHTTP->m_pClient->Release();
        m_pHTTP->m_pClient = NULL;
    }

    return HXR_OK;
}

STDMETHODIMP
HTTPFileResponse::ReadDone (HX_RESULT status, IHXBuffer* pBuffer)
{
    if (m_bDefunct)
        return HXR_FAIL;

    DPRINTF(D_ENTRY, ("HTTPFR::ReadDone(status(%lu), pBuffer(%p))\n",
            status, pBuffer));

    if(HXR_OK != status)
    {
        m_pHTTP->m_pProtocol->handleRead(NULL, 0, TRUE);
#if 0
        //XXXTDM: what is this?
        if (m_pHTTP->m_pClient->m_pCtrl->m_pCmd->write_flush_needed())
            m_pHTTP->m_bDone = 1;
#endif
    }
    else
    {
        //XXXJR This is silly, why don't we just pass the IHXBuffer in
        //      instead?
        m_pHTTP->m_pProtocol->handleRead(pBuffer->GetBuffer(), pBuffer->GetSize(),
                          FALSE);
    }

    return HXR_OK;
}

STDMETHODIMP
HTTPFileResponse::WriteDone (HX_RESULT status)
{
    if (m_bDefunct)
        return HXR_FAIL;

    m_pHTTP->m_post_buffer_write_pos++;
    if(m_pHTTP->m_post_buffer_write_pos >= m_pHTTP->m_post_buffer_pos)
    {
        // write is done, send response
        if (m_pHTTP->m_content_len > 0)
            m_pHTTP->m_pProtocol->sendResponse(m_pHTTP->m_pProtocol->makeResponseMessage("200"));
        else
            m_pHTTP->m_pProtocol->sendResponse(m_pHTTP->m_pProtocol->makeResponseMessage("204"));

        m_pHTTP->m_pClient->OnClosed(HXR_OK);
        return HXR_OK;
    }

    if(HXR_OK != m_pHTTP->m_pFileObject->Write(
        (IHXBuffer*)m_pHTTP->m_post_buffer->GetAt(m_pHTTP->m_post_buffer_write_pos)))
    {
        m_pHTTP->m_pClient->OnClosed(HXR_NET_WRITE);
        return HXR_OK;
    }

    return HXR_OK;
}

STDMETHODIMP
HTTPFileResponse::SeekDone (HX_RESULT status)
{
    if (m_bDefunct)
        return HXR_FAIL;

    return HXR_OK;
}

STDMETHODIMP
HTTPFileResponse::StatDone (HX_RESULT status,
                            UINT32 ulSize,
                            UINT32 ulCreationTime,
                            UINT32 ulAccessTime,
                            UINT32 ulModificationTime,
                            UINT32 ulFlags)
{
    if (m_bDefunct)
        return HXR_FAIL;

    IHXFileObject* pFileObject = m_pHTTP->m_pFileObject;

    m_ulSize = ulSize;
    m_ulCreationTime = ulCreationTime;

    if(HXR_OK == m_pHTTP->m_pFileObject->QueryInterface(IID_IHXFileMimeMapper,
                                                        (void**)&m_file_mime))
    {
        m_file_mime->FindMimeType(m_pHTTP->m_url,
                                 (IHXFileMimeMapperResponse*)this);
    }
    else
    {
        if(m_pHTTP->m_pProtocol->statComplete(status, ulSize, ulModificationTime, NULL))
        {
            m_pHTTP->m_pSock->SelectEvents(HX_SOCK_EVENT_READ|HX_SOCK_EVENT_WRITE|HX_SOCK_EVENT_CLOSE);
        }
    }
    if(m_pFileStat)
    {
        m_pFileStat->Release();
        m_pFileStat = 0;
    }

    return status;
}

STDMETHODIMP
HTTPFileResponse::MimeTypeFound(HX_RESULT status,
                                const char* mimeType)
{
    if (m_bDefunct)
        return HXR_FAIL;

    if(m_pHTTP->m_pProtocol->statComplete(status, m_ulSize, m_ulCreationTime, mimeType))
    {
        m_pHTTP->m_pSock->SelectEvents(HX_SOCK_EVENT_READ|HX_SOCK_EVENT_WRITE|HX_SOCK_EVENT_CLOSE);
    }

    if (m_file_mime)
    {
        m_file_mime->Release();
        m_file_mime = 0;
    }



//    m_pHTTP->m_pFileObject->Read(READSIZE);
    return HXR_OK;
}

HTTP::HTTP(IHXSocket* pSock, HTTPProtocol* pProtocol, Client* pClient) :
    m_url(NULL),
    m_pRequestContextCurrent(NULL),
    m_pChallengeResponseRequester(NULL),
    m_bNeedHandler(FALSE),
    m_pPostHandler(NULL),
    m_pPostDataBuffer(NULL)
{
    m_pSock = pSock;
    pSock->AddRef();
    m_pProtocol = pProtocol;
    m_pProtocol->AddRef();
    m_pClient = pClient;
    pClient->AddRef();
    m_bDone = 0;
    m_pFileObject = NULL;
    m_FSManager = NULL;
    m_pFileResponse = NULL;
    m_pFSMR = NULL;
    m_post_received = FALSE;
    m_post_buffer = NULL;
    m_post_buffer_pos = 0;
    m_post_buffer_write_pos = 0;
    m_content_len = 0;

    m_pRequest = NULL;

    m_state = HS_GetInit;
}

HTTP::~HTTP()
{
    Done();

    if(m_FSManager)
        m_FSManager->Release();

    if(m_pFileResponse)
    {
        m_pFileResponse->m_bDefunct = TRUE;
        m_pFileResponse->Release();
    }

    if(m_pFSMR)
    {
        m_pFSMR->HTTPDone();
        m_pFSMR->Release();
    }

    if(m_pRequest)
        m_pRequest->Release();

    // clean up the post buffer
    for (UINT32 ulIndex = m_post_buffer_write_pos; ulIndex < m_post_buffer_pos; ulIndex++)
    {
        ((IHXBuffer*)m_post_buffer->GetAt(ulIndex))->Release();
    }
    HX_DELETE(m_post_buffer);
    HX_RELEASE(m_pPostDataBuffer);

    HX_VECTOR_DELETE(m_url);

    HX_RELEASE(m_pProtocol);
    HX_RELEASE(m_pRequestContextCurrent);
    HX_RELEASE(m_pChallengeResponseRequester);
    HX_RELEASE(m_pPostHandler);
    HX_RELEASE(m_pSock);
}

HX_RESULT
HTTP::WriteReady(void)
{
    if (m_pFileObject)
    {
        return m_pFileObject->Read(READSIZE);
    }	
    return HXR_OK;
}

HX_RESULT
HTTP::http_start(IHXRequest* pRequest, IHXValues* pAuthValues)
{
    const char* url;
    BOOL bAutoRedirect = TRUE;

    HX_RELEASE(m_pRequest);
    HX_RELEASE(m_pRequestContextCurrent);

    m_pRequest = pRequest;
    m_pRequest->AddRef();

    m_pRequest->QueryInterface
    (
        IID_IHXRequestContext,
        (void**)&m_pRequestContextCurrent
    );

    if(m_pRequestContextCurrent)
    {
        IUnknown* pUnknownChallenge = new HTTPChallenge(this);

        pUnknownChallenge->AddRef();

        m_pRequestContextCurrent->SetRequester(pUnknownChallenge);

        HX_RELEASE(pUnknownChallenge);
    }

    if(m_pRequest->GetURL(url) == HXR_OK && url)
    {
        m_url = new_string(url);

        // In order to parse the URL with the CHXURL class, we must ensure
        // that the URL is fully qualified. Add a bogus protocol and domain
        char* pTemp = NULL;
        pTemp = new char[strlen(url) + strlen("http://x/") + 1];
        sprintf(pTemp, "http://x/%s", url);

        CHXURL urlParser(pTemp);
        if (SUCCEEDED(urlParser.GetLastError()))
        {
            IHXValues* pParams = NULL;
            IHXBuffer* pBuf = NULL;

            // Find out if there is an "AutoRedirect=False" parameter. If so,
            // we should disable the file system's auto-redirect feature
            pParams = (IHXValues*)urlParser.GetOptions();
            if (HXR_OK == pParams->GetPropertyBuffer("AutoRedirect", pBuf))
            {
                if (!strcasecmp((char*)pBuf->GetBuffer(), "False"))
                {
                    bAutoRedirect = FALSE;
                }
            }
            HX_RELEASE(pBuf);
            HX_RELEASE(pParams);
        }
        HX_VECTOR_DELETE(pTemp);
    }
    else
    {
        m_url = new_string("");
    }

    if(m_state != HS_KeepAliveInit)
    {
        if (m_state != HS_PostInit)
        {
            m_state = HS_GetInit;
        }

        // If we're posting we defer this initialization until we can fill in
        // the PostData request header.  If m_bNeedHandler is set, though,
        // that means the data is too big for a header and needs the
        // IHXPostHandler interface.

        if (m_state == HS_PostInit && !m_bNeedHandler)
        {
            return HXR_OK;
        }

        return InitFileSystem(bAutoRedirect);

    }
    else if (m_pChallengeResponseRequester && m_state == HS_KeepAliveInit)
    {
        m_state = HS_GetFileInit;

        // Tell the Challenge Sender that the Challenge Response
        // was received.
        m_pChallengeResponseRequester->ResponseReady(m_pRequest);

        // Only respond once per Challenge
        // and prevent circular references.
        HX_RELEASE(m_pChallengeResponseRequester);

        return HXR_OK;
    }

    return HXR_UNEXPECTED;
}

HX_RESULT
HTTP::InitFileSystem(BOOL bAutoRedirect)
{
    HX_RELEASE(m_pFSMR);
    HX_RELEASE(m_FSManager);

    m_pFSMR = new HTTPFileSystemManagerResponse(this);
    m_pFSMR->AddRef();

    m_FSManager = new FSManager(m_pClient->proc);
    m_FSManager->AddRef();

    if (!bAutoRedirect)
    {
        // Disable the auto redirect feature of httpfsys by finding
        // and initializing the IHXHTTPRedirect interface. This
        // tells the file system manager that we want to be notified
        // of any redirects instead of letting the file system try
        // to handle them.
        IHXHTTPRedirect* pHTTPRedirect = NULL;

        m_FSManager->QueryInterface(IID_IHXHTTPRedirect,
            (void**)&pHTTPRedirect);

        if (pHTTPRedirect)
        {
            pHTTPRedirect->Init((IHXHTTPRedirectResponse*)m_pFSMR);
        }
        HX_RELEASE(pHTTPRedirect);
    }

    return m_FSManager->Init((IHXFileSystemManagerResponse*)m_pFSMR);
}

HX_RESULT
HTTP::http_post(char* url, UINT32 content_len, BOOL bNeedHandler)
{
    m_content_len = content_len;
    m_wrote_len = 0;

    m_post_buffer = new CHXPtrArray();
    m_post_buffer->SetSize(20);
    m_post_buffer_pos = 0;

    m_state = HS_PostInit;
    m_bNeedHandler = bNeedHandler;
    return HXR_OK;
}

HX_RESULT
HTTP::http_post_data(char* data, int len)
{
    IHXBuffer* writeBuffer;

    if ((m_post_received && !m_pPostHandler) || !m_content_len)
    {
        return HXR_UNEXPECTED;
    }

    if (data && len)
    {
        writeBuffer = new CHXBuffer();
        writeBuffer->AddRef();
        writeBuffer->Set((const UCHAR*)data, (UINT32)len);

        if(m_post_buffer_pos >= (UINT32)m_post_buffer->GetSize())
        {
           m_post_buffer->SetSize(m_post_buffer->GetSize() + 20);
                   // 20 is arbitrary
        }
        m_post_buffer->SetAt(m_post_buffer_pos++, writeBuffer);
    }

    if (m_pPostHandler)
    {
        while (m_post_buffer_write_pos < m_post_buffer_pos)
        {
            IHXBuffer* pBuf = (IHXBuffer*)
                    m_post_buffer->GetAt(m_post_buffer_write_pos++);

            m_pPostHandler->PostData(pBuf);
            HX_RELEASE(pBuf);
        }
    }

    if(len + m_wrote_len >= m_content_len || m_post_received)
    {
        m_post_received = TRUE;
        return http_post_done();
    }

    m_wrote_len += len;
    return HXR_OK;
}

HX_RESULT
HTTP::http_post_done()
{
    if (!m_post_received && !m_pPostHandler)
    {
        return HXR_OK;
    }

    if (m_pPostHandler)
    {
        HX_ASSERT(m_post_buffer_write_pos == m_post_buffer_pos);
        if (m_pPostDataBuffer)
        {
            m_pPostHandler->PostData(m_pPostDataBuffer);
            HX_RELEASE(m_pPostDataBuffer);
        }
        // Signal handle - no more data
        m_pPostHandler->PostData(NULL);
        HX_RELEASE(m_pPostHandler);
    }
    else
    {
        // We have what we need - add PostData to request header
        IHXBuffer* pBuf = NULL;

        if (m_post_buffer_pos > 1)
        {
            UINT32 ulPos = 0;
            pBuf = new CHXBuffer();
            pBuf->AddRef();

            while (m_post_buffer_write_pos < m_post_buffer_pos)
            {
                IHXBuffer* pPartial = (IHXBuffer*)
                        m_post_buffer->GetAt(m_post_buffer_write_pos++);
                pBuf->SetSize(ulPos + pPartial->GetSize());
                memcpy(pBuf->GetBuffer()+ulPos, pPartial->GetBuffer(),
                        pPartial->GetSize());
                ulPos += pPartial->GetSize();
                HX_RELEASE(pPartial);
            }
        }
        else
        {
            pBuf = (IHXBuffer*)m_post_buffer->GetAt(m_post_buffer_write_pos++);
        }

        // Null Terminate
        if (pBuf)
        {
            UINT32 size = pBuf->GetSize();

            //respect the Content-Length header
            if (m_content_len && (m_content_len < size))
            {
                size = m_content_len;
            }

            pBuf->SetSize(size+1);
            *(pBuf->GetBuffer()+size) = 0;
        }

        IHXValues* pReq = NULL;

        if (HXR_OK != m_pRequest->GetRequestHeaders(pReq))
        {
            pReq = new CHXHeader();
            pReq->AddRef();
        }

        pReq->SetPropertyBuffer("PostData", pBuf);
        m_pRequest->SetRequestHeaders(pReq);
        HX_RELEASE(pReq);

        // Save this in case IHXPostDataHandler is supported
        m_pPostDataBuffer = pBuf;

        InitFileSystem(FALSE);

    }

    return HXR_OK;
}


STDMETHODIMP
HTTPFileSystemManagerResponse::QueryInterface(REFIID riid, void** ppvObj)
{
    if(IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)(IHXFileSystemManagerResponse*)this;
        return HXR_OK;
    }
    else if(IsEqualIID(riid, IID_IHXFileSystemManagerResponse))
    {
        AddRef();
        *ppvObj = (IHXFileSystemManagerResponse*)this;
        return HXR_OK;
    }
    else if(IsEqualIID(riid, IID_IHXHTTPRedirectResponse))
    {
        AddRef();
        *ppvObj = (IHXHTTPRedirectResponse*)this;
        return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32)
HTTPFileSystemManagerResponse::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32)
HTTPFileSystemManagerResponse::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

STDMETHODIMP_(BOOL)
HTTPFileSystemManagerResponse::IsAccessAllowed(char* url, char*** HTTP_paths)
{
    BOOL bOK = TRUE;

    char** ppPath;

    // Max size: '/' + url + '\0'
    NEW_FAST_TEMP_STR(normurl, 1024, 1+strlen(url)+1);
    char* p = normurl;

    BOOL bNewSegment = FALSE; // TRUE iff on first char of new segment
    if (url[0] != '/' && url[0] != '\\')
    {
        *p++ = '/';
        url++;
        bNewSegment = TRUE;
    }

    // do not examine the query string portion of the URL
    while (*url && (*url != '?'))
    {
        if (bNewSegment)
        {
            if (url[0] == '/' || url[0] == '\\')
            {
                // Found "//", skip this one
                url += 1;
                continue;
            }
            if (url[0] == '.')
            {
                if (url[1] == '/' || url[1] == '\\')
                {
                    // Found "/./", skip it
                    url += 2;
                    continue;
                }
                if (url[1] == '.' && (url[2] == '/' || url[2] == '\\'))
                {
                    // Found "/../", skip it and back up to previous pathsep
                    p--;
                    if (p == normurl)
                    {
                        // Already at top level, cannot back up
                        bOK = FALSE;
                        break;
                    }
                    while (*(p-1) != '/')
                    {
                        p--;
                    }
                    url += 3;
                    continue;
                }
            }
            bNewSegment = FALSE;
        }
        if (url[0] == '/' || url[0] == '\\')
        {
            *p++ = '/';
            bNewSegment = TRUE;
        }
        else
        {
            *p++ = *url;
        }
        url++;
    }
    *p = '\0';

    // Don't allow directories
    if (p == normurl || *(p-1) == '/')
    {
        bOK = FALSE;
    }

    // No paths means no access
    if (!HTTP_paths)
    {
        bOK = FALSE;
    }

    if (bOK)
    {
        // Default to deny
        bOK = FALSE;

        for (ppPath = *HTTP_paths; *ppPath != NULL; ppPath++)
        {
            char* pCurPath = *ppPath;
            UINT32 uCurLen = strlen(pCurPath);
            if (uCurLen == 0 || (uCurLen == 1 && pCurPath[0] == '/'))
            {
                // Empty path matches all (same as "/")
                bOK = TRUE;
                break;
            }

#if defined(_WIN32)
            if (!strnicmp(pCurPath, normurl, uCurLen) &&
#else
            if (!strncmp(pCurPath, normurl, uCurLen) &&
#endif
                (normurl[uCurLen] == '\0' || normurl[uCurLen] == '/'))
            {
                bOK = TRUE;
                break;
            }
        }
    }
    DELETE_FAST_TEMP_STR(normurl);

    return bOK;
}

STDMETHODIMP
HTTPFileSystemManagerResponse::InitDone(HX_RESULT status)
{
    DPRINTF(D_INFO, ("HTTPFSMR::InitDone\n"));
    if(HXR_OK == status)
    {
        HX_RESULT result = HXR_OK;
        FSManager* pFSManager = m_pHTTP->m_FSManager;

        if (m_pHTTP->m_state == HS_GetInit)
        {
            if (!IsAccessAllowed(m_pHTTP->m_url, m_pHTTP->m_pClient->proc->pc->HTTP_deliver_paths))
            {
                if (!m_pHTTP || !m_pHTTP->m_pFSMR)
                {
                    return HXR_FAIL;
                }
                m_pHTTP->m_pFSMR->FileObjectReady(HXR_FAIL, NULL);
                return HXR_OK;
            }
        }
        else if (m_pHTTP->m_state == HS_PostInit)
        {
            if (!IsAccessAllowed(m_pHTTP->m_url, m_pHTTP->m_pClient->proc->pc->HTTP_postable_paths))
            {
                if (!m_pHTTP || !m_pHTTP->m_pFSMR)
                {
                    return HXR_FAIL;
                }
                m_pHTTP->m_pFSMR->FileObjectReady(HXR_FAIL, NULL);
                return HXR_OK;
            }
        }

        switch(m_pHTTP->m_state)
        {
        case HS_PostInit:
        {
            m_pHTTP->m_state = HS_PostFile;
            result = pFSManager->GetFileObject(m_pHTTP->m_pRequest,
                                               NULL);
            break;
        }
        case HS_KeepAliveInit:
        {
            m_pHTTP->m_state = HS_GetFile;
            if(m_pHTTP->m_pFileObject)
            {
                m_pHTTP->m_pFileObject->AddRef();
            }
            result = FileObjectReady
            (
                HXR_OK,
                m_pHTTP->m_pFileObject
            );
            if(m_pHTTP->m_pFileObject)
            {
                m_pHTTP->m_pFileObject->Release();
            }
            break;
        }
        case HS_GetInit:
        {
            m_pHTTP->m_state = HS_GetFile;
            result = pFSManager->GetFileObject(m_pHTTP->m_pRequest,
                                               NULL);
            break;
        }
        default:
            result = HXR_UNEXPECTED;
        }

        return result;
    }
    else
        return HXR_OK;
}

STDMETHODIMP
HTTPFileSystemManagerResponse::FileObjectReady(HX_RESULT status,
                                               IUnknown* pObject)
{
    if(m_bHTTPDone)
    {
        return HXR_UNEXPECTED;
    }

    if(HXR_OK != status)
    {
        m_pHTTP->m_pProtocol->statComplete(HXR_FILE_NOT_FOUND, 0, 0, NULL);
        return HXR_OK;
    }

    if(m_pHTTP->m_pFileObject)
    {
        m_pHTTP->m_pFileObject->Release();
        m_pHTTP->m_pFileObject = NULL;
    }

    if(HXR_OK == pObject->QueryInterface(IID_IHXFileObject,
                                       (void**)&m_pHTTP->m_pFileObject))
    {
        if(!m_pHTTP->m_pFileResponse)
        {
            m_pHTTP->m_pFileResponse = new HTTPFileResponse(m_pHTTP);
            m_pHTTP->m_pFileResponse->AddRef();
        }

        switch(m_pHTTP->m_state)
        {
        case HS_GetFile:
            m_pHTTP->m_state = HS_GetFileInit;
            return m_pHTTP->m_pFileObject->Init(
                HX_FILE_READ | HX_FILE_BINARY,
                (IHXFileResponse*)m_pHTTP->m_pFileResponse);
            break;
        case HS_PostFile:
            m_pHTTP->m_state = HS_PostFileInit;
            if (HXR_OK != pObject->QueryInterface(IID_IHXPostDataHandler,
                                           (void**)&m_pHTTP->m_pPostHandler))
            {
                m_pHTTP->m_pPostHandler = NULL;

                if (m_pHTTP->m_bNeedHandler)
                {
                    // Bad news...The POST data is too long for PostData header
                    // but this file system doesn't support IHXPostDataHandler
                    // Fail out...
                    m_pHTTP->m_pProtocol->statComplete(HXR_INVALID_OPERATION, 0, 0, NULL);
                    return HXR_OK;
                }
            }

            if (m_pHTTP->m_pPostHandler)
            {
                HX_RESULT res = HXR_OK;
                res = m_pHTTP->m_pFileObject->Init(HX_FILE_READ|HX_FILE_BINARY,
                                (IHXFileResponse*)m_pHTTP->m_pFileResponse);
                m_pHTTP->http_post_data(NULL, 0);
                return res;
            }
            else if (m_pHTTP->m_post_received)
            {
                return m_pHTTP->m_pFileObject->Init(HX_FILE_READ|HX_FILE_BINARY,
                                (IHXFileResponse*)m_pHTTP->m_pFileResponse);
            }

            HX_ASSERT(0 && "No POST data\n");
            // Shouldn't get here!
            return HXR_OK;

        default:
            return HXR_UNEXPECTED;
        }
    }
    DPRINTF(D_INFO, ("Mount point for %s is not a regular file system\n",
                     m_pHTTP->m_url));
    m_pHTTP->m_pProtocol->statComplete(HXR_FAILED, 0, 0, NULL);
    return HXR_FAIL;
}

STDMETHODIMP
HTTPFileSystemManagerResponse::DirObjectReady(HX_RESULT, IUnknown*)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP
HTTPFileSystemManagerResponse::RedirectDone(IHXBuffer* pURL)
{
    if (!m_bHTTPDone)
    {
        m_pHTTP->m_pProtocol->handleRedirect(pURL);
    }
    return HXR_OK;
}

void
HTTPFileSystemManagerResponse::HTTPDone()
{
    m_bHTTPDone = TRUE;
}

int
HTTP::flushed_tcp(int flush_count)
{
#if 0
    //XXXTDM: what is this?
    if (m_pClient->m_pCtrl->m_pCmd->write_flush_count() < 16384)
    {
        if (!m_bDone)
            m_pFileObject->Read(READSIZE);
        else
        {
            if(m_pClient)
            {
#ifdef PAULM_CLIENTAR
                REL_NOTIFY(m_pClient, 13);
#endif
                m_pClient->Release();
                m_pClient = NULL;
            }
            return -1;
        }
    }
#endif
    return 0;
}

void
HTTP::Done()
{
    if(m_pFileObject)
    {
        m_pFileObject->Close();
        HX_RELEASE(m_pPostHandler);
    }
    else
    {
        if (m_pClient)
        {
#ifdef PAULM_CLIENTAR
            REL_NOTIFY(m_pClient, 14);
#endif
            m_pClient->Release();
            m_pClient = NULL;
        }
    }
}

HTTPChallenge::HTTPChallenge(HTTP* pHTTP) :
    m_lRefCount(0),
    m_pHTTP(pHTTP)
{
}

HTTPChallenge::~HTTPChallenge()
{
}

STDMETHODIMP
HTTPChallenge::QueryInterface(REFIID riid, void** ppvObj)
{
    if(IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)this;
        return HXR_OK;
    }
    else if(IsEqualIID(riid, IID_IHXChallenge))
    {
        AddRef();
        *ppvObj = (IHXChallenge*)this;
        return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32)
HTTPChallenge::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32)
HTTPChallenge::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

STDMETHODIMP
HTTPChallenge::SendChallenge
(
    IHXChallengeResponse* pChallengeResponseSender,
    IHXRequest* pRequestChallenge
)
{
    HX_RELEASE(m_pHTTP->m_pChallengeResponseRequester);
    m_pHTTP->m_pChallengeResponseRequester = pChallengeResponseSender;
    m_pHTTP->m_pChallengeResponseRequester->AddRef();

    IHXBuffer* pBufferAction = NULL;
    IHXValues* pValuesResponseHeaders = NULL;
    IHXValues* pValuesRequestHeaders = NULL;

    pRequestChallenge->GetRequestHeaders(pValuesRequestHeaders);
    pRequestChallenge->GetResponseHeaders(pValuesResponseHeaders);
    m_pHTTP->m_pRequest->SetResponseHeaders(pValuesResponseHeaders);

    pValuesRequestHeaders->GetPropertyCString("Connection", pBufferAction);

    if (pBufferAction)
    {
        if (!strncasecmp((const char*)pBufferAction->GetBuffer(),
                         "Keep-Alive", 10))
        {
            HX_RELEASE(pBufferAction);
            ServerBuffer::FromCharArray("keep-alive", &pBufferAction);
            pValuesResponseHeaders->SetPropertyCString("Connection", pBufferAction);
        }
    }
    HX_RELEASE(pBufferAction);
    HX_RELEASE(pValuesResponseHeaders);
    HX_RELEASE(pValuesRequestHeaders);

    // Report "Access Denied"
    //
    if (m_pHTTP->m_pProtocol->statComplete(HXR_NOT_AUTHORIZED, 0, 0, NULL))
    {
        m_pHTTP->m_pSock->SelectEvents(HX_SOCK_EVENT_READ|HX_SOCK_EVENT_WRITE|HX_SOCK_EVENT_CLOSE);
    }

    return HXR_OK;
}
