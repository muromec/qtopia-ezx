/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: shared_udp.cpp,v 1.6 2004/07/24 23:10:19 tmarshall Exp $
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
/*
 *  Description:
 *      Allow one "shared" incoming UDP port to handle all of
 *      the packet-resend request (and other input) from all the clients
 *      on one streamer.
 *
 *      Each streamer proc will have its own shared port and port-reading
 *      callback, so threadsafe manipulations of member data shouldn't be
 *      needed.
 *
 *      The "port enum" param is used to implement listening on the RTCP
 *      port as well, for RTC connections.  That is, we're really opening
 *      two consecutive UDP ports per streamer, so all register operations
 *      and so on need to specify whether the caller is referring to the "base"
 *      UDP connection of the RTC connection.  See rtspserv.cpp for usage example.
 *              sPortEnum == 0  =>  UDP data port
 *              sPortEnum == 1  =>  RTCP port
 *      In the current implementation, the portEnum is the offset from the base
 *      port to the real port number.  This could be generalized if more shared
 *      ports need to be added.
 *
 *      Always create a shared port reader and attach it to the engine, if
 *      we're a streamer proc.  When opening a UDP data port, always try to
 *      register for shared port data.  If shared ports are in use, status will
 *      be HXR_OK and we can fetch the shared port number for the client.  If
 *      not, use the regular data backchannel.
 *
 *       Engine -->  SharedUDPPortReader
 *                          /       \
 *                      Receiver  Receiver      (one receiver per shared port)
 *
 *      m_pAddrMap is keyed on the foreign address (UINT32); the data are
 *      linked lists keyed on the foreign port number.  Why?  I was worried
 *      about performance, so I didn't want to translate every incoming
 *      datagram to an address:port string and hash on that.  I could've
 *      added a map for long:long keys, but: it probably would've had the first
 *      long as the hash result (as is currently for CHXMapLongToObj), then
 *      linear search for the full key match; that would be about what I
 *      implemented here.  I assume there are rarely lots of clients on one
 *      machine (?).
 *
 *      I hope the terminology isn't too confusing.  A receiver is an object
 *      that gets all the data coming in on one shared port, and distributes
 *      it to the registered listeners.  A listener is an object that represents
 *      one registered listener.
 *
 *
 *  Revisions:
 *      jmevissen       1/19/2001       Initial version
 */


#include "shared_udp.h"

#include "hxtypes.h"
#include "hxassert.h"
#include "debug.h"
#include "netbyte.h"

#include "hxiids.h"
#include "hxengin.h"
#include "hxmon.h"
#include "ihxpckts.h"
#include "hxmap.h"

#include "errmsg_macros.h"
#include "server_context.h"
#include "servlist.h"


////////////////////////////////////////////////////////////////////////
//              SharedUDPPortReader::SharedUDPPortReader
////////////////////////////////////////////////////////////////////////
//
// Description:         Constructor.
//
// Parameters:
//
// Returns:
//
// Implementation:
//      Check whether feature is enabled in registry.
//      Open the socket pair on which we listen.
//
// Revisions:
//      jmevissen       1/19/2001       Initial version
//
////////////////////////////////////////////////////////////////////////
SharedUDPPortReader::SharedUDPPortReader(ServerContext* context)
    : m_lRefCount(0)
    , m_bSharedAvailable(FALSE)
{
    HX_RESULT hr = HXR_OK;
    IHXNetworkServices* pNetworkServices = 0;
    IHXRegistry* pRegistry = NULL;

    int nPortMin = 0;
    int nPortMax = -1;  // => no maximum constraint

    // Get registry params.
    //

    hr = context->QueryInterface(IID_IHXRegistry,
                                 (void**)&pRegistry);

    if (hr == HXR_OK)
    {
        IHXBuffer* pBuf = NULL;

        hr = pRegistry->GetStrByName(SHARED_UDP_PORT_REGISTRY_NAME, pBuf);
        if (hr == HXR_OK)
        {
            const char *pStr = (const char*) pBuf->GetBuffer();
            char* pMaxVal = (char*)strchr(pStr, '-');
            nPortMin = atoi(pStr);
            if (pMaxVal)
            {
                nPortMax = atoi(pMaxVal+1);
                if (nPortMax < 0) nPortMax = 0; // invalid value
            }
            pBuf->Release();
        }

        pRegistry->Release();
        pRegistry = 0;
    }

    // Silent, early exit if no params in registry.

    if (hr != HXR_OK) return;

    // Get logger

    IHXErrorMessages* pError = 0;
    context->QueryInterface(IID_IHXErrorMessages,
                            (void**)&pError);

    // exit on invalid params

    if (nPortMin <= 0 ||
        nPortMax == 0)
    {
        LOGMSG(pError, HXLOG_WARNING,
               "Invalid resend UDP port range %d to %d",
               nPortMin, nPortMax);
        return;
    }

    if (nPortMin < MIN_SHARED_UDP_PORT) nPortMin = MIN_SHARED_UDP_PORT;
    if (nPortMax == -1 ||
        nPortMax > MAX_SHARED_UDP_PORT) nPortMax = MAX_SHARED_UDP_PORT;

    if (nPortMin & 1) ++nPortMin;       // start on even

    // Start at port-min, and keep trying until we succeed or get
    // to port-max.

    IHXUDPSocket* pUDPSocket0 = 0;
    IHXUDPSocket* pUDPSocket1 = 0;
    UINT16 nPort = (UINT16) nPortMin;

    // instantiate receivers

    m_ppReceiver[0] = new Receiver;
    m_ppReceiver[1] = new Receiver;
    m_ppReceiver[0]->AddRef();
    m_ppReceiver[1]->AddRef();

    // get network services

    hr = context->QueryInterface(IID_IHXNetworkServices,
                                 (void**)&pNetworkServices);

    // create a pair of sockets.
    // We really only need to retry the bindings, but: if one succeeds
    // and the other fails, we'd have to release the successful socket in
    // order to retry the binding.  After an unsuccessful bind, we have
    // to re-init() anyway.  So doing it over from scratch for each socket
    // pair on every loop seems the most straightforward.

    while (hr == HXR_OK)
    {
        if((HXR_OK != pNetworkServices->CreateUDPSocket(&pUDPSocket0)) ||
           (HXR_OK != pNetworkServices->CreateUDPSocket(&pUDPSocket1)))
        {
            hr = HXR_FAIL;
        }

        if(hr == HXR_OK &&
           HXR_OK == pUDPSocket0->Init(HX_INADDR_IPBINDINGS, 0, m_ppReceiver[0]) &&
           HXR_OK == pUDPSocket1->Init(HX_INADDR_IPBINDINGS, 0, m_ppReceiver[1]))
        {
            if(HXR_OK == pUDPSocket0->Bind(HX_INADDR_IPBINDINGS, nPort) &&
               HXR_OK == pUDPSocket1->Bind(HX_INADDR_IPBINDINGS, nPort+1))
            {
                break;
            }
        }

        if (pUDPSocket0) pUDPSocket0->Release();
        if (pUDPSocket1) pUDPSocket1->Release();
        pUDPSocket0 = 0;
        pUDPSocket1 = 0;

        nPort += 2;
        if (nPort >= nPortMax)  // need room for nPort and nPort+1
        {
            hr = HXR_FAIL;

            LOGMSG(pError, HXLOG_WARNING,
                   "Cannot bind any resend UDP ports in range %d to %d",
                   nPortMin, nPortMax);
            LOGMSG(pError, HXLOG_WARNING,
                   "Range should be configured to allow two ports per CPU");
        }
    }

    // Pass the sockets off to the receivers

    if (hr == HXR_OK)
    {
        if (HXR_OK != m_ppReceiver[0]->Init(pUDPSocket0) ||
            HXR_OK != m_ppReceiver[1]->Init(pUDPSocket1))
        {
            // ???  This is unexpected.

            hr = HXR_FAIL;

            pUDPSocket0->Release();
            pUDPSocket1->Release();
        }
    }

    // Everything init'd okay

    if (hr == HXR_OK)
    {
        m_bSharedAvailable = TRUE;

        LOGMSG(pError, HXLOG_INFO, "Shared resend UDP ports %d and %d initialized",
               nPort, nPort+1);
    }
    else
    {
        LOGMSG(pError, HXLOG_WARNING, "Shared resend UDP ports NOT initialized");
    }

    // cleanup

    if (pError) pError->Release();
    if (pNetworkServices) pNetworkServices->Release();
}

////////////////////////////////////////////////////////////////////////
//              SharedUDPPortReader::~SharedUDPPortReader
////////////////////////////////////////////////////////////////////////
//
// Description:         Destructor.  Should never be invoked, since we
//                      want to live as long as the streamer we're
//                      associated with.
//
// Parameters:
//
// Returns:
//
// Implementation:
//      If we really could be invoked, it would be necessary to clean
//      up the references to the receivers.
//
//
// Revisions:
//      jmevissen       1/19/2001       Initial version
//
////////////////////////////////////////////////////////////////////////
SharedUDPPortReader::~SharedUDPPortReader()
{
    ASSERT(0);
}

////////////////////////////////////////////////////////////////////////
//              SharedUDPPortReader::RegisterForeignAddress
////////////////////////////////////////////////////////////////////////
//
// Description:         Register to get incoming datagrams.
//                      Callback must implement IHXUDPResponse.
//
// Parameters:
//              ulAddr          Foreign address caller wants data from
//              sPort           Foreign port caller wants data from
//              pUDPResponse    Callback object
//              sPortEnum       0=local UDP port, 1=local RTSP/RTC port
//
// Returns:     HXR_OK
//              HXR_FAIL        shared ports not available
//              HXR_INVALID_PARAMETER
//                              sPortEnum out-of-range
//
// Implementation:
//
//
// Revisions:
//      jmevissen       1/22/2001       Initial version
//
////////////////////////////////////////////////////////////////////////
HX_RESULT
SharedUDPPortReader::RegisterForeignAddress(UINT32 ulAddr,
                                            UINT16 sPort,
                                            IHXUDPResponse* pUDPResponse,
                                            UINT16 sPortEnum)
{
    if (!m_bSharedAvailable) return HXR_FAIL;

    if (sPortEnum != 0 &&
        sPortEnum != 1)
    {
        return HXR_INVALID_PARAMETER;
    }
    return
        m_ppReceiver[sPortEnum]->RegisterForeignAddress(ulAddr, sPort, pUDPResponse);
}

HX_RESULT
SharedUDPPortReader::Receiver::RegisterForeignAddress(UINT32 ulAddr,
                                                      UINT16 sPort,
                                                      IHXUDPResponse* pUDPResponse)
{
    // Remove any pre-existing listener for this address (needed to remove
    // references to the ReadDone callback object)

    UnregisterForeignAddress(ulAddr, sPort);

    // Add this listener

    HXList* portList = 0;
    Listener* listener = new Listener;

    listener->SetAddr(ulAddr);
    listener->SetPort(sPort);
    listener->SetObj(pUDPResponse);

    // m_pAddrMap is a hash on the addr to a list of listeners keyed by port number.
    // See note in header.

    if (m_pAddrMap->Lookup(ulAddr, (void*&) portList))
    {
        portList->insert(listener);
    }
    else
    {
        portList = new HXList;
        portList->insert(listener);
        (*m_pAddrMap)[ulAddr] = portList;
    }

    return HXR_OK;
}

////////////////////////////////////////////////////////////////////////
//              SharedUDPPortReader::UnregisterForeignAddress
////////////////////////////////////////////////////////////////////////
//
// Description:         Unregister for incoming datagrams.
//
// Parameters:
//              ulAddr          Foreign address caller wanted data from
//              sPort           Foreign port caller wanted data from
//              sPortEnum       0=local UDP port, 1=local RTSP/RTC port
//
// Returns:     HXR_OK
//              HXR_FAIL        shared ports not available
//              HXR_INVALID_PARAMETER
//                              sPortEnum out-of-range
//
// Implementation:
//
//
// Revisions:
//      jmevissen       1/22/2001       Initial version
//
////////////////////////////////////////////////////////////////////////
HX_RESULT
SharedUDPPortReader::UnregisterForeignAddress(UINT32 ulAddr,
                                              UINT16 sPort,
                                              UINT16 sPortEnum)
{
    if (!m_bSharedAvailable) return HXR_FAIL;

    if (sPortEnum != 0 &&
        sPortEnum != 1)
    {
        return HXR_INVALID_PARAMETER;
    }
    return
        m_ppReceiver[sPortEnum]->UnregisterForeignAddress(ulAddr, sPort);
}

HX_RESULT
SharedUDPPortReader::Receiver::UnregisterForeignAddress(UINT32 ulAddr,
                                                        UINT16 sPort)
{
    Listener* listener = 0;
    HXList* portList = 0;

    // m_pAddrMap is a hash on the addr to a list of listeners keyed by port number.
    // See note in header.

    if (m_pAddrMap->Lookup(ulAddr, (void*&) portList))
    {
        HXList_iterator i(portList);
        for (; *i != 0; ++i)
        {
            listener = (Listener*) (*i);
            if (listener->GetPort() == sPort)
            {
                break;
            }
            listener = 0;
        }
    }
    if (listener)
    {
        portList->remove(listener);
        if (portList->empty())
        {
            m_pAddrMap->RemoveKey(ulAddr);
            delete portList;
        }
        delete listener;

        // clear simple cache

        if (ulAddr == m_lastAddr &&
            sPort == m_lastPort)
        {
            m_lastAddr = m_lastPort = 0;
            m_lastListener = 0;
        }
    }
    return HXR_OK;
}

////////////////////////////////////////////////////////////////////////
//              SharedUDPPortReader::GetPort
////////////////////////////////////////////////////////////////////////
//
// Description:         Return the shared port number.
//
// Parameters:
//
// Returns:
//
// Implementation:
//
//
// Revisions:
//      jmevissen       1/22/2001       Initial version
//
////////////////////////////////////////////////////////////////////////
UINT16
SharedUDPPortReader::GetPort(UINT16 sPortEnum)
{
    if (!m_bSharedAvailable) return 0;

    if (sPortEnum != 0 &&
        sPortEnum != 1)
    {
        return 0;
    }

    return m_ppReceiver[sPortEnum]->GetPort();
}

////////////////////////////////////////////////////////////////////////
//              SharedUDPPortReader::Receiver::Receiver
////////////////////////////////////////////////////////////////////////
//
//
// Revisions:
//      jmevissen       1/25/2001       Initial version
//
////////////////////////////////////////////////////////////////////////
SharedUDPPortReader::Receiver::Receiver()
    : m_sPort(0)
    , m_lRefCount(0)
    , m_pUDPSocket(0)
{
}

////////////////////////////////////////////////////////////////////////
//              SharedUDPPortReader::Receiver::Receiver
////////////////////////////////////////////////////////////////////////
//
// Description:         Init the receiver with the socket and port it's
//                      listening on.
//
//
//
// Revisions:
//      jmevissen       1/25/2001       Initial version
//
////////////////////////////////////////////////////////////////////////
HX_RESULT
SharedUDPPortReader::Receiver::Init(IHXUDPSocket* pSocket)
{
    m_pUDPSocket = pSocket;
    m_pUDPSocket->GetLocalPort(m_sPort);

    // init the address map

    m_pAddrMap = new CHXMapLongToObj;

    /*
     * Try to increase the UDP receive buffer size to max, since
     * all clients for one streamer will be funneled through one port.
     */
    IHXSetSocketOption* pSocketOption = 0;
    if (m_pUDPSocket->QueryInterface(IID_IHXSetSocketOption,
                                     (void**)&pSocketOption)
        == HXR_OK)
    {
        pSocketOption->SetOption(HX_SOCKOPT_SET_RECVBUF_SIZE,
                                 1000000); // XXXJMEV
        pSocketOption->Release();
    }

    // Start the read

    return m_pUDPSocket->Read(HX_SAFEUINT(4096)); // XXXJMEV - what constant?
}

////////////////////////////////////////////////////////////////////////
//              SharedUDPPortReader::Receiver::~Receiver
////////////////////////////////////////////////////////////////////////
//
// Description:         The destructor is never invoked.
//
//
// Implementation:
//      If we really could be invoked, it would be necessary to clean
//      up the references to the transport objects in m_pAddrMap, as
//      well as the map itself.  Also release the socket objects we
//      created (for the UDP sockets we opened).
//
// Revisions:
//      jmevissen       1/25/2001       Initial version
//
////////////////////////////////////////////////////////////////////////
SharedUDPPortReader::Receiver::~Receiver()
{
    ASSERT(0);
}

////////////////////////////////////////////////////////////////////////
//              SharedUDPPortReader::Receiver::ReadDone
////////////////////////////////////////////////////////////////////////
//
// Description:         Distribute an incoming datagram to the correct
//                      registered recipient.
//
// Parameters:          see IHXUDPResponse
//
// Returns:             HXR_OK
//
// Implementation:
//
//
// Revisions:
//      jmevissen       1/22/2001       Initial version
//
////////////////////////////////////////////////////////////////////////
STDMETHODIMP
SharedUDPPortReader::Receiver::ReadDone(HX_RESULT status, IHXBuffer* pBuffer,
        UINT32 ulAddr, UINT16 sPort)
{
    // The caller (DoRead) often reads the same socket to exhaustion
    // before moving on.  There may be a performance boost to caching the
    // last datagram's address.  (Conversely, these replies only come
    // once per second under normal conditions, so perhaps this cache is
    // NOT justified.)

    Listener* listener = 0;
    HXList* portList = 0;

    // Same as last time?

    if (ulAddr == m_lastAddr &&
        sPort  == m_lastPort)
    {
        listener = m_lastListener;
    }
    // No, look it up in the map.
    else if(m_pAddrMap->Lookup(ulAddr, (void*&) portList))
    {
        HXList_iterator i(portList);
        for (; *i != 0; ++i)
        {
            listener = (Listener*) (*i);
            if (listener->GetPort() == sPort)
            {
                break;
            }
            listener = 0;
        }
    }

    if (listener)
    {
        IHXUDPResponse* obj = listener->GetObj();
        if (obj)
        {
            if (HXR_OK != obj->ReadDone(status, pBuffer, ulAddr, sPort))
            {
                // DoRead() releases the callback if ReadDone is not OK.
                // Shall we do the same?  (We can't simply pass up the
                // result of ReadDone() to the caller.)

                obj->ReadDone(HXR_FAIL, 0, 0, 0);
                UnregisterForeignAddress(ulAddr, sPort);
                listener = 0;
            }
        }
    }

    m_lastAddr = ulAddr;
    m_lastPort = sPort;
    m_lastListener = listener;

    // always return OK; we want the shared socket to stay open regardless
    // of whether this datagram was successfully distributed.

    return HXR_OK;
}

void
SharedUDPPortReader::Listener::SetObj(IHXUDPResponse *p)
{
    // clean up any old reference

    UnsetObj();

    // set new reference

    m_pObj = p;
    if (p) p->AddRef();
}

SharedUDPPortReader::Listener::~Listener()
{
    UnsetObj();
}


/////////////////////////////////////////////////////////////////////////
//  Method:
//      IUnknown::QueryInterface
//  Purpose:
//      Implement this to export the interfaces supported by your
//      object.
//
STDMETHODIMP
SharedUDPPortReader::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)this;
        return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//      IUnknown::AddRef
//  Purpose:
//      Everyone usually implements this the same... feel free to use
//      this implementation.
//
STDMETHODIMP_(ULONG32)
SharedUDPPortReader::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//      IUnknown::Release
//  Purpose:
//      Everyone usually implements this the same... feel free to use
//      this implementation.
//
STDMETHODIMP_(ULONG32)
SharedUDPPortReader::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//      IUnknown::QueryInterface
//  Purpose:
//      Implement this to export the interfaces supported by your
//      object.
//
STDMETHODIMP
SharedUDPPortReader::Receiver::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)(IHXUDPResponse*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXUDPResponse))
    {
        AddRef();
        *ppvObj = (IHXUDPResponse*)this;
        return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//      IUnknown::AddRef
//  Purpose:
//      Everyone usually implements this the same... feel free to use
//      this implementation.
//
STDMETHODIMP_(ULONG32)
SharedUDPPortReader::Receiver::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//      IUnknown::Release
//  Purpose:
//      Everyone usually implements this the same... feel free to use
//      this implementation.
//
STDMETHODIMP_(ULONG32)
SharedUDPPortReader::Receiver::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}
