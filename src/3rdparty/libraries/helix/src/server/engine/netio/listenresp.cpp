/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: listenresp.cpp,v 1.5 2005/01/25 22:55:21 jzeng Exp $
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

#include "hxtypes.h"
#include "hxcom.h"
#include "hxerror.h"

#include "debug.h"
#include "netbyte.h"

#include "proc.h"

#include "servsockimp.h"

#include "listenresp.h"

#include "engine.h"
#include "servreg.h"
#include "acceptor.h"
#include "base_errmsg.h"
#include "servbuffer.h"

#ifdef _UNIX
#if defined _AIX
#define SETEUID(uid) setreuid(-1, (uid))
#define SETEGID(gid) setregid(-1, (gid))
#elif defined _HPUX
#define SETEUID(uid) setresuid(-1, (uid), -1)
#define SETEGID(gid) setresgid(-1, (gid), -1)
#else
#define SETEUID(uid) seteuid(uid)
#define SETEGID(gid) setegid(gid)
#endif
#endif // _UNIX

ListenResponse::ListenResponse(Process* pProc) :
    m_nRefCount(0),
    m_pProc(pProc),
    m_pMessages(pProc->pc->error_handler),
    m_pRegistry(pProc->pc->registry),
    m_pNetSvc(m_pProc->pc->net_services),
    m_pAddr(NULL),
    m_pSock(NULL)
{
    HX_ADDREF(m_pMessages);
    HX_ADDREF(m_pRegistry);
    HX_ADDREF(m_pNetSvc);
}

ListenResponse::~ListenResponse(void)
{
    Close();
    HX_RELEASE(m_pSock);
    HX_RELEASE(m_pAddr);
    HX_RELEASE(m_pNetSvc);
    HX_RELEASE(m_pRegistry);
    HX_RELEASE(m_pMessages);
}

STDMETHODIMP
ListenResponse::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)(IHXActivePropUser*)this;
        return HXR_OK;
    }
    if (IsEqualIID(riid, IID_IHXActivePropUser))
    {
        AddRef();
        *ppvObj = (IHXActivePropUser*)this;
        return HXR_OK;
    }
    if (IsEqualIID(riid, IID_IHXListeningSocketResponse))
    {
        AddRef();
        *ppvObj = (IHXListeningSocketResponse*)this;
        return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32)
ListenResponse::AddRef(void)
{
    return InterlockedIncrement(&m_nRefCount);
}

STDMETHODIMP_(ULONG32)
ListenResponse::Release(void)
{
    if (InterlockedDecrement(&m_nRefCount) > 0)
    {
        return m_nRefCount;
    }

    delete this;
    return 0;
}

STDMETHODIMP
ListenResponse::SetActiveInt(const char* pName, UINT32 ul,
                IHXActivePropUserResponse* pResponse)
{
    /*
     * Since we are only active for ports we know that someone is
     * trying to change our port.  We don't event have to know which
     * port we are (ex. rtsp, pna ...)
     */

    /*
     * If they are just trying to set it to what it was, tell them
     * that they succeeded.
     */
    if(m_pAddr != NULL && m_pAddr->GetPort() == ul)
    {
        /*
         * Set this so we can verify the commit.
         */
        pResponse->SetActiveIntDone(HXR_OK, pName, ul, 0, 0);
        return HXR_OK;
    }
    HX_RESULT res;
    res = InitSocket((UINT16)ul);
    IHXBuffer* pBuf = NULL;
    UINT32 uResCount = 0;
    if (res != HXR_OK)
    {
        pBuf = new ServerBuffer(TRUE);
        char errstr[256];
        sprintf(errstr, "Cannot bind to port %lu", ul);
        pBuf->Set((const unsigned char*)errstr, strlen(errstr) + 1);
        m_ppActiveRes[uResCount++] = pBuf;
    }

    pResponse->SetActiveIntDone(res, pName, ul, m_ppActiveRes, uResCount);
    HX_RELEASE(pBuf);
    return HXR_OK;
}

STDMETHODIMP
ListenResponse::SetActiveStr(const char* pName, IHXBuffer* pBuffer,
                IHXActivePropUserResponse* pResponse)
{
    pResponse->SetActiveStrDone(HXR_FAIL, pName, pBuffer, NULL, 0);
    return HXR_OK;
}

/************************************************************************
* IHXActivePropUser::SetActiveBuf
*
*    Async request to set buffer pName to buffer in pBuffer.
*/
STDMETHODIMP
ListenResponse::SetActiveBuf(const char* pName, IHXBuffer* pBuffer,
                IHXActivePropUserResponse* pResponse)
{
    pResponse->SetActiveBufDone(HXR_FAIL, pName, pBuffer, NULL, 0);
    return HXR_OK;
}

STDMETHODIMP
ListenResponse::DeleteActiveProp(const char* pName,
                IHXActivePropUserResponse* pResponse)
{
    UINT32 uResCount = 0;
    IHXBuffer* pBuf = new ServerBuffer(TRUE);
    char errstr[256];
    strcpy(errstr, "Cannot delete property: is a listening port");
    pBuf->Set((const unsigned char*)errstr, strlen(errstr) + 1);
    m_ppActiveRes[uResCount++] = pBuf;
    pResponse->DeleteActivePropDone(HXR_FAIL, pName, m_ppActiveRes, uResCount);
    return HXR_OK;
}

HX_RESULT
ListenResponse::InitSocket(UINT16 uPort)
{
    HX_RESULT rc;

    IHXSockAddr* pNewAddr = NULL;
    IHXListeningSocket* pNewSock = NULL;

    if (m_pAddr != NULL)
    {
        m_pAddr->Clone(&pNewAddr);
    }
    else
    {
        rc = m_pNetSvc->CreateSockAddr(HX_SOCK_FAMILY_INANY, &pNewAddr);
    }

    if (SUCCEEDED(rc))
    {
        rc = pNewAddr->SetPort(uPort);
    }
    if (SUCCEEDED(rc))
    {
        m_pProc->pc->net_services->CreateListeningSocket(&pNewSock);

        rc = pNewSock->Init(pNewAddr->GetFamily(),
                            HX_SOCK_TYPE_TCP,
                            HX_SOCK_PROTO_ANY,
                            this);
    }
    if (SUCCEEDED(rc))
    {
        rc = pNewSock->Listen(pNewAddr);
    }
    if (FAILED(rc))
    {
        HX_RELEASE(pNewSock);
        HX_RELEASE(pNewAddr);
        return HXR_FAIL;
    }

    // Succeess.  Replace existing address and socket, if present
    HX_RELEASE(m_pAddr);
    m_pAddr = pNewAddr;
    m_pAddr->AddRef();

    if (m_pSock != NULL)
    {
        m_pSock->Close();
    }
    HX_RELEASE(m_pSock);
    m_pSock = pNewSock;
    m_pSock->AddRef();

    HX_RELEASE(pNewSock);
    HX_RELEASE(pNewAddr);

    return HXR_OK;
}

void
ListenResponse::Close(void)
{
    if (m_pSock != NULL)
    {
        m_pSock->Close();
        HX_RELEASE(m_pSock);
    }
}
