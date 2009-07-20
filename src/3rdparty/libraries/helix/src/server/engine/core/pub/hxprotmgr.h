/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxprotmgr.h,v 1.4 2006/10/03 23:19:07 tknox Exp $
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

#ifndef _HXPROTMGR_H_
#define _HXPROTMGR_H_

/*
 * This is a simple scheme for protocol negotiation (aka "port switching").
 * It also allows the proxy to use different source files that create proxy
 * specific protocol objects.
 *
 * The protocol manager is created in the core proc on startup.  It is a
 * global object that should only be used from the core proc.
 *
 * The HXProtocolManager::Init() function "knows" the supported protocols.
 * It calls each protocol in turn to get a "sniffer" object.
 *
 * The core proc init code calls SetDefaultProtocol using the port entries in
 * the config file.
 *
 * On connection, the Connection object calls IHXSocket::Peek() and passes the
 * buffer to HXProtocolManager::FindProtocol().  Each "sniffer" is called in
 * turn to find the appropriate protocol.  If none is found, the default is
 * used.
 *
 * A few points should be noted here:
 *
 * The implementation currently ignores addresses in the IHXSockAddr; it
 * only compares port numbers.  This should be fixed but it will be a bit
 * complicated to handle the case where a socket is listening on "any".
 *
 * This scheme is meant to be a temporary measure.  Eventually we would
 * like to put the protocols into plugins.  When that happens, we can
 * create COM interfaces to replace this scheme.
 *
 * HTTP is a special case.  The HTTP protocol is required to accept both
 * normal HTTP traffic and cloaked HTTP traffic.  This is implemented with
 * a demultiplexor object.
 */

_INTERFACE IHXBuffer;

class HXProtocol;
class CHXServSocket;

enum HXProtocolType
{
    HXPROT_UNKNOWN,
    HXPROT_ALIVE,
    HXPROT_PNA, //depreciated
    HXPROT_RTSP,
    HXPROT_HTTP,
    HXPROT_MMS,
    HXPROT_CLOAK,
    HXPROT_LAST
};

class HXProtocolSniffer // Should have a better name
{
public:
    virtual BOOL         Match(IHXBuffer* pBuf) = 0;
    virtual HXProtocol*  Create(CHXServSocket* pSock) = 0;
};

struct sniffer_binding
{
    HXProtocolType      pt;
    HXProtocolSniffer*  pSniffer;
};

struct address_binding
{
    HXProtocolType      pt;
    IHXSockAddr*        pAddr;
};

//XXX: Should be dynamic...
#define MAX_PROTOCOLS 16

class HXProtocolManager
{
public:
    HXProtocolManager(void);
    ~HXProtocolManager(void);

    HX_RESULT   Init(void);

    HX_RESULT   SetDefaultProtocol(HXProtocolType pt, IHXSockAddr* pAddr);
    HX_RESULT   RegisterProtocol(HXProtocolType pt, HXProtocolSniffer* pSniffer);
    HX_RESULT   MatchProtocol(IHXSockAddr* pLocalAddr, IHXBuffer* pBuf,
                                REF(HXProtocolType) /*OUT*/ pt);
    HX_RESULT   CreateProtocol(CHXServSocket* pSock, HXProtocolType pt,
                                REF(HXProtocol*) /*OUT*/ pProtocol);

protected:
    sniffer_binding     m_sniffer_bindings[MAX_PROTOCOLS];
    address_binding     m_address_bindings[MAX_PROTOCOLS];
};

// This is created in and should only be used from the coreproc
extern HXProtocolManager* g_pProtMgr;

/*
 * This class handles newly connected sockets.  It waits for the first
 * packet and invokes the sniffer to determine the protocol.  When the
 * protocol is determined, the socket is optionally dispatched to a
 * streamer process.  The protocol is then created and takes control of
 * the connection.
 */
class HXSocketConnection : public IHXSocketResponse
{
public:
    HXSocketConnection(IHXSocket* pSock, HXProtocolType pt);
    virtual ~HXSocketConnection(void);

    STDMETHOD(QueryInterface)       (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)      (THIS);
    STDMETHOD_(ULONG32,Release)     (THIS);

    STDMETHOD(EventPending)         (THIS_ UINT32 uEvent, HX_RESULT status);

protected:
    INT32               m_nRefCount;
    CHXServSocket*      m_pSock;
    HXProtocolType      m_pt;
    HXProtocol*         m_pProtocol;
};

#endif /* _HXPROTMGR_H_ */
