/* ***** BEGIN LICENSE BLOCK *****
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
#include "hxnet.h"
#include "hxprefs.h"
#include "hxprefutil.h"
#include "chxresolver.h"
#include "chxclientsocket.h"

#include "hxtlogutil.h"
#include "debug.h"
#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif


CHXClientSocket::CHXClientSocket(CHXNetServices* pNetSvc,
                                 IUnknown* pContext,
                                 HXSockFamily f, 
                                 HXSockType t, 
                                 HXSockProtocol p, 
                                 HX_SOCK s) 
: CHXSocket(pNetSvc, pContext, f, t, p, s)
, m_pSelector(0)
{
    HX_RESULT hr = DoClientSockInit();
    HX_ASSERT(HXR_OK == hr); // XXXLCM
}


CHXClientSocket::CHXClientSocket(CHXNetServices* pNetSvc, 
                                 IUnknown* pContext)
: CHXSocket(pNetSvc, pContext)
, m_pSelector(0)
{
}

CHXClientSocket::~CHXClientSocket()
{
    Close();
}

void CHXClientSocket::CloseSelector()
{
    if(m_pSelector)
    {
        m_pSelector->RemoveHandler(this);
        HX_RELEASE(m_pSelector);
    }
}

STDMETHODIMP
CHXClientSocket::Close()
{
    CloseSelector(); // selector must be closed first
    HX_RESULT hr = CHXSocket::Close();
    return hr;
}

HX_RESULT
CHXClientSocket::InitSelector()
{
    HX_RESULT hr = HXR_OK;
    HX_ASSERT(!m_pSelector);
    if (!m_pSelector)
    {
        hr = HXSocketSelector::GetThreadInstance(m_pSelector, m_punkContext);
    }
    return hr;
}

HX_RESULT 
CHXClientSocket::InitRcvBufSize()
{
    //
    // set system receive buffer size for this socket (critical
    // for high-bitrate UDP streaming)
    //
    HX_ASSERT(m_type != HX_SOCK_TYPE_NONE);
    HX_RESULT hr = HXR_OK;

    const char* pKey = 0;
    UINT32 cbDefault = 0;
    if (HX_SOCK_TYPE_TCP == m_type)
    {
        // XXXLCM receive buffer probably not useful for TCP (reliable/windowed)
        pKey = "SockTCPReceiveBufSize";
        cbDefault = 1; // use socket default
    }
    else
    {
        HX_ASSERT(HX_SOCK_TYPE_UDP == m_type);
        pKey = "SockUDPReceiveBufSize";
        // set arbitrary default value reasonably large enough
        // to prevent lost packets under normal load 
        cbDefault = 256 * 1024; 
    }

    UINT32 cbWanted = cbDefault;
    ReadPrefUINT32(m_punkContext, pKey, cbWanted);
    switch (cbWanted)
    {
    case 0:
        if (cbDefault == 1)
        {
            // app-specified default is to use socket default
            cbWanted = 0;
        }
        else
        {
            // use app-specified default
            cbWanted = cbDefault;
        }
        break;
    case 1:
        // use socket default
        cbWanted = 0;
        break;
    default:
        break;
    }

    if (0 != cbWanted )
    {
        // override socket default
        HXLOGL4(HXLOG_NETW, "CHXClientSocket[%p]::SetRcvBufSize(): PREF: sock system receive buffer size = %lu bytes", this, cbWanted);
        
        // only size if the requested value is larger than the socket's default value
        UINT32 cbSocketDefault = 0;
        GetOption(HX_SOCKOPT_RCVBUF, &cbSocketDefault);
        HX_ASSERT(cbWanted > cbSocketDefault);
        while (cbWanted > cbSocketDefault)
        {
            hr = SetOption(HX_SOCKOPT_RCVBUF, cbWanted);
            if (SUCCEEDED(hr))
            {
                HXLOGL4(HXLOG_NETW, "CHXClientSocket[%p]::SetRcvBufSize(): size set = %lu bytes; old size = %lu bytes", this, cbWanted, cbSocketDefault);
                break;
            }

            // this may be an issue
            HX_ASSERT(false);

            // reduce size by half and try again...
            cbWanted /= 2;
        }
    }
    return hr;
}

HX_RESULT CHXClientSocket::InitPerReadBufSize()
{
    HX_ASSERT(m_type != HX_SOCK_TYPE_NONE);

    //
    // set allocation size for each recv destination 
    // buffer; 0 = default based on MSS (TCP) or ~64K (UDP)
    //

    HX_RESULT hr = HXR_OK;
    if (HX_SOCK_TYPE_UDP == m_type)
    {
        UINT32 cbPerReadBuf = 0;
        ReadPrefUINT32(m_punkContext, "SockUDPPerReadBufSize", cbPerReadBuf);
        if (cbPerReadBuf != 0)
        {
            HXLOGL3(HXLOG_NETW, "CHXClientSocket[%p]::Init(): PREF: setting sock UDP per-read buf size = %lu bytes", this, cbPerReadBuf);
            hr = SetOption(HX_SOCKOPT_APP_READBUF_MAX, cbPerReadBuf);
        }
    }
    return hr;
}

HX_RESULT CHXClientSocket::DoClientSockInit()
{
    // Init() or full ctor must be called first
    HX_ASSERT(m_family != HX_SOCK_FAMILY_NONE);
    HX_ASSERT(m_type != HX_SOCK_TYPE_NONE);

    HX_RESULT hr = InitSelector();
    if (FAILED(hr))
    {
        return hr;
    }

    // Configure default client-specific socket options... 
    
    // XXXLCM These defaults may be more appropriately configured in higher-level code
    //        where the actual needs of the socket are better known. For example, will
    //        all UDP sockets on the client need large receive buffers?

    //
    // By default the socket will allocate a default-sized buffer to read into, then set 
    // the buffer size to the amount actually read in after the underlying socket read is
    // completed. In that case the extra buffer space is not freed--the size associated with
    // the buffer is merely adjusted. This option forces a copy so we conserve memory at the
    // expense of an extra copy for each read.
    //
    HXBOOL bCopySocketReadBuffers = TRUE;
    ReadPrefBOOL( m_punkContext, "CopySocketReadBuffers", bCopySocketReadBuffers );
    if( bCopySocketReadBuffers )
    {
        SetOption(HX_SOCKOPT_APP_READBUF_ALLOC, HX_SOCK_READBUF_SIZE_COPY);
    }
    else
    {
        SetOption(HX_SOCKOPT_APP_READBUF_ALLOC, HX_SOCK_READBUF_SIZE_DEFAULT);
    }
    
    //SetOption(HX_SOCKOPT_APP_BUFFER_TYPE, HX_SOCKBUF_TIMESTAMPED);
    InitPerReadBufSize();
    InitRcvBufSize();

    return HXR_OK;
}

STDMETHODIMP
CHXClientSocket::Init(HXSockFamily f, HXSockType t, HXSockProtocol p)
{
    HX_RESULT hr = CHXSocket::Init(f, t, p);
    if (SUCCEEDED(hr))
    {
        hr = DoClientSockInit();
    }
    return hr;
}
    
void 
CHXClientSocket::OnSelectEvent(sockobj_t fd, UINT32 event, HX_RESULT err)
{    
    HX_ASSERT(fd == m_sock.sock);

    // selector must be configured to provide basic select semantics only
    HX_ASSERT(event == HX_SOCK_EVENT_READ || event == HX_SOCK_EVENT_WRITE);

    //
    // XXXLCM 
    //
    // Since the handler does not add-ref us we do this in case the response
    // responds to the event by closing and releasing this socket.
    //
    AddRef();
    OnEvent(event); //XXXLCM pass err
    Release();
}

HX_RESULT 
CHXClientSocket::Select(UINT32 uEventMask, HXBOOL bImplicit)
{
    if (!HX_SOCK_VALID(m_sock))
    {
        HX_ASSERT(0 == uEventMask);
        return HXR_FAIL;
    }

    if (0 == uEventMask)
    {
        if (m_pSelector && m_pSelector->HasSocket(m_sock.sock))
        {
            HXLOGL3(HXLOG_NETW, "CHXClientSocket[%p]::Select(): de-selecting socket",this);
            m_pSelector->RemoveSocket(m_sock.sock);
        }
    }
    else if (bImplicit)
    {
        // This is an implicit call made by the base class. That happens
        // when a re-enabling class member function is called. For example,
        // if you call IHXSocket::Read() it implicitly re-selects a read event.
        HX_ASSERT(uEventMask != 0);
        HX_ASSERT(m_pSelector);
        if(m_pSelector)
        {
            HX_ASSERT(m_pSelector->HasSocket(m_sock.sock));
            m_pSelector->ReEnableSocket(m_sock.sock, uEventMask);
        }
    }
    else
    {
        // This is an explicit call. We are here because the socket user
        // has called IHXSocket::SelectEvents().
        HXLOGL3(HXLOG_NETW, "CHXClientSocket[%p]::Select(): mask = 0x%08x",this, uEventMask);
        HX_ASSERT(uEventMask != 0);
        HX_ASSERT(m_pSelector);
        if(!m_pSelector->HasSocket(m_sock.sock))
        {
            m_pSelector->AddSocket(m_sock.sock, uEventMask, this); 
        }
        else
        {
            HXLOGL3(HXLOG_NETW, "CHXClientSocket[%p]::SelectEvents(): warning: re-selecting socket (performance)", this);
            m_pSelector->UpdateSocket(m_sock.sock, uEventMask);
        }
    }

    return HXR_OK;
}

