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


#if !defined(CHXRTCPTRANSMAPSOCKET_H__)
#define CHXRTCPTRANSMAPSOCKET_H__


#include "chxrtsptransocket.h"
#include "hxtypes.h"
#include "rtspserv.h"
#include "hxnet.h"

class CHXRTCPTransMapSocket : public CHXRTSPTranSocket
{
public:
    CHXRTCPTransMapSocket();
    CHXRTCPTransMapSocket(IHXSocket* pSocket);
    virtual ~CHXRTCPTransMapSocket(void);

    /* IHXSocket */
    STDMETHOD(QueryInterface)       (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)      (THIS);
    STDMETHOD_(ULONG32,Release)     (THIS);

    STDMETHOD(ConnectToOne)         (THIS_ IHXSockAddr* pAddr);
    STDMETHOD(ConnectToAny)         (THIS_ UINT32 nVecLen, IHXSockAddr** ppAddrVec);

    STDMETHOD(SelectEvents)         (THIS_ UINT32 uEventMask);
    STDMETHOD(Read)                 (THIS_ IHXBuffer** pBuf);

    STDMETHOD(PeekFrom)             (THIS_ IHXBuffer** ppBuf,
                                           IHXSockAddr** ppAddr);
    STDMETHOD(ReadFrom)             (THIS_ IHXBuffer** pBuf, IHXSockAddr** pAddr);
       
    STDMETHOD(Write)                (THIS_ IHXBuffer* pBuf);
    STDMETHOD(Close)                (THIS);

    /* IHXSocketResponse */
    STDMETHOD(EventPending)         (THIS_ UINT32 uEvent, HX_RESULT status);
     
       
    void SetSocket(IHXSocket* pSock);
    IHXSocket* GetSocket(void);
    void SetIsSockResponse(BOOL b );
    void SetSocketPort(IHXSocket *pSocket, UINT16 fport);
    void SetPeerAddr(IHXSockAddr* pPeerAddr);
    void SetMap(CHXMapLongToObj* m_pFportTransResponseMap);
   
protected:
    UINT16      m_uForeignPort;
    BOOL        m_bIsSockResponse;
    BOOL        m_bReadPending;
    BOOL        m_bEventReceived;
    IHXSockAddr* m_pPeerAddr;
    IHXSocket* m_pSockNew; //this one is for reading, base class m_pSock is for writing.
    CHXMapLongToObj* m_pFportTransResponseMap;
    
};

#endif /* CHXRTCPTRANSMAPSOCKET_H__ */

