/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: rtspmcast.h,v 1.11 2007/03/08 00:17:43 tknox Exp $
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

#ifndef _RTSPMCAST_H_
#define _RTSPMCAST_H_

#include "hxtypes.h"
#include "hxcom.h"

#include "hxmutexlock.h"

#include "transport.h"
#include "rtspserv.h"
#include "base_callback.h"

class Process;
class Transport;
class RDTUDPTransport;
class HXProtocol;
class RTSPTransportHack;
struct IHXValues;
struct IHXPacketResend;
class MulticastManager;

struct RTSPMcastInfo
{
    Transport*          m_pTransport;
    UINT32              m_ulPlayerCount;
    UINT32              m_ulAddress;
    BYTE                m_bRuleOn[RULE_TABLE_WIDTH * RULE_TABLE_HEIGHT];
    UINT32              m_ulSequence[RULE_TABLE_HEIGHT];
    BOOL                m_bAddrAllocated;
};

class RTSPServerMulticastTransport : public Transport
{
public:

    RTSPServerMulticastTransport(IUnknown* pContext, Transport*
        pTrans, UINT32 ulTTL, UINT32 ulAddress, MulticastManager* pMgr);
    virtual ~RTSPServerMulticastTransport();

    /*
     *  IUnknown methods
     */

    STDMETHOD(QueryInterface)           (THIS_
                                        REFIID riid,
                                        void** ppvObj);

    /*
     *  IHXStatistics methods
     */

    STDMETHOD (InitializeStatistics)    (THIS_
                                        UINT32  /*IN*/ ulRegistryID);

    STDMETHOD (UpdateStatistics)        (THIS);

    /* Other Methods */

    STDMETHOD_(BOOL,isNullSetup)        (THIS);

    STDMETHOD_(BOOL,isRTSPMulticast)    (THIS);

    /* Transport Methods */

    virtual HX_RESULT           sendPacket(BasePacket* packet);
    virtual HX_RESULT           streamDone(UINT16 uStreamNumber,
                                   UINT32 uReasonCode = 0,
                                   const char* pReasonText = NULL);
    virtual void                setSequenceNumber(UINT16 uStreamID,
                                                  UINT16 uSequenceNumber);
    UINT32                      wrapSequenceNumber();
    STDMETHOD_(HXBOOL,IsInitialized)    (THIS);
    virtual BOOL                IsUpdated();
    virtual HX_RESULT           SubscriptionDone(BYTE* bRuleOn,
                                                 REF(UINT32)ulSourcePort,
                                                 REF(UINT32)ulPort,
                                                 REF(UINT32)ulAddr,
                                                 REF(TransportStreamHandler*)pHandler);

private:
    void SetupMulticastSocket(UINT32 ulTTL);

    void SetAddressOnTransport(RDTUDPTransport* pTransport, UINT32 ulAddress, UINT32 ulPort);

    // Only for unit tests (hence private).
    friend class CUTRTSPServerMulticastTransportTestDriver;
    RTSPServerMulticastTransport(IUnknown* pContext,
                                CHXMapStringToOb* pTransports);

    class SockCloseCallback : public BaseCallback
    {
    public:
        SockCloseCallback(IUnknown* pContext, IHXSocket* pMcastSocket)
        {
	    m_pContext = pContext;
            m_pContext->AddRef();

	    m_pUDPMcastSocket = pMcastSocket;
	}

        STDMETHOD(Func)             (THIS);

    private:
	IHXSocket*   m_pUDPMcastSocket;
	IUnknown*    m_pContext;
    };

    TransportStreamHandler*  m_pStreamHandler;
    CHXMapStringToOb*   m_pTransports;
    HX_MUTEX            m_pTransMutex;
    UINT32              m_ulRegistryID;
    UINT16              m_unSourcePort;
    MulticastManager*   m_pMgr;
    IHXSocket*		m_pUDPMcastSocket;
    IUnknown* 		m_pContext;
};

#endif // _RTSPMCAST_H_
