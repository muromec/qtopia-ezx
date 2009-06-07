/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: cloak_common.h,v 1.4 2005/07/19 00:41:10 darrick Exp $
 *
 * Portions Copyright (c) 1995-2005 RealNetworks, Inc. All Rights Reserved.
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

#ifndef _CLOAK_COMMON_H_
#define _CLOAK_COMMON_H_

#include "hxtypes.h"
#include "hxnet.h"
#include "servsockimp.h"
#include "servlbsock.h"
#include "hxslist.h"

class CHXList;
class RTSPProtocol;
class CBaseCloakPOSTHandler;
class CBaseCloakGETHandler;


#define CLOAK_MODE_MULTIPOST                (1 << 0)


class CloakConn : public IHXSocket
{
public:
    CloakConn(void);
    virtual ~CloakConn(void);

    // IUnknown

    STDMETHOD(QueryInterface)   (REFIID riid, void** ppvObj);
    STDMETHOD_(UINT32,AddRef)   (THIS);
    STDMETHOD_(UINT32,Release)  (THIS);


    // IHXSocket

    STDMETHOD_(HXSockFamily,GetFamily)      (THIS);
    STDMETHOD_(HXSockType,GetType)          (THIS);
    STDMETHOD_(HXSockProtocol,GetProtocol)  (THIS);

    STDMETHOD(SetResponse)      (THIS_ IHXSocketResponse* pResponse);
    STDMETHOD(SetAccessControl) (THIS_ IHXSocketAccessControl* pControl);
    STDMETHOD(Init)             (THIS_ HXSockFamily f, HXSockType t, HXSockProtocol p);
    STDMETHOD(CreateSockAddr)   (THIS_ IHXSockAddr** ppAddr);

    STDMETHOD(Bind)             (THIS_ IHXSockAddr* pAddr);
    STDMETHOD(ConnectToOne)     (THIS_ IHXSockAddr* pAddr);
    STDMETHOD(ConnectToAny)     (THIS_ UINT32 nVecLen, IHXSockAddr** ppAddrVec);
    STDMETHOD(GetLocalAddr)     (THIS_ IHXSockAddr** ppAddr);
    STDMETHOD(GetPeerAddr)      (THIS_ IHXSockAddr** ppAddr);

    STDMETHOD(SelectEvents)     (THIS_ UINT32 uEventMask);
    STDMETHOD(Peek)             (THIS_ IHXBuffer** ppBuf);
    STDMETHOD(Read)             (THIS_ IHXBuffer** ppBuf);
    STDMETHOD(Write)            (THIS_ IHXBuffer* pBuf);
    STDMETHOD(Close)            (THIS);

    STDMETHOD(Listen)           (THIS_ UINT32 uBackLog);
    STDMETHOD(Accept)           (THIS_ IHXSocket** ppNewSock,
                                       IHXSockAddr** ppSource);

    STDMETHOD(GetOption)        (THIS_ HXSockOpt name, UINT32* pval);
    STDMETHOD(SetOption)        (THIS_ HXSockOpt name, UINT32 val);

    STDMETHOD(PeekFrom)         (THIS_ IHXBuffer** ppBuf,
                                       IHXSockAddr** ppAddr);
    STDMETHOD(ReadFrom)         (THIS_ IHXBuffer** ppBuf,
                                           IHXSockAddr** ppAddr);
    STDMETHOD(WriteTo)          (THIS_ IHXBuffer* pBuf,
                                       IHXSockAddr* pAddr);

    STDMETHOD(ReadV)            (THIS_ UINT32 nVecLen, UINT32* puLenVec,
                                       IHXBuffer** ppBufVec);
    STDMETHOD(ReadFromV)        (THIS_ UINT32 nVecLen, UINT32* puLenVec,
                                       IHXBuffer** ppBufVec,
                                       IHXSockAddr** ppAddr);

    STDMETHOD(WriteV)           (THIS_ UINT32 nVecLen,
                                       IHXBuffer** ppBufVec);
    STDMETHOD(WriteToV)         (THIS_ UINT32 nVecLen,
                                       IHXBuffer** ppBufVec,
                                       IHXSockAddr* pAddr);


    HX_RESULT                   GetGETHandler(CBaseCloakGETHandler*&);
    HX_RESULT                   SetGETHandler(CBaseCloakGETHandler*);

    UINT32                      GetPOSTHandlerCount(void);
    HX_RESULT                   GetFirstPOSTHandler(CBaseCloakPOSTHandler *&);
    HX_RESULT                   AddPOSTHandler(CBaseCloakPOSTHandler *);
    HX_RESULT                   RemovePOSTHandler(CBaseCloakPOSTHandler *);
    HX_RESULT                   CloseAllPOSTHandlers(void);

    void                        SetRefSocket(IHXSocket* pSock);

    void                        OnGETRequest(CBaseCloakGETHandler* pGETHandler);
    void                        GETChannelReady(Process* pProc);
    void                        OnGETWriteReady();

    void                        POSTChannelReady();
    HX_RESULT                   OnPOSTData(IHXBuffer* pBuf);

    void                        OnClosed(HX_RESULT status);

    UINT32                      GetCloakMode();
    void                        SetCloakMode(UINT32 ulMode);


private:

    INT32                       m_nRefCount;

    CHXSimpleList               m_ReadQueue;

    RTSPProtocol*               m_pRTSPProt;    // for initializing RTSP stack
    IHXSocketResponse*          m_pRTSPSvrProt; // for communication w/ RTSP stack

    IHXSocket*                  m_pRefSock;     // reference socket, for addr info, etc.
    CBaseCloakGETHandler*       m_pGETHandler;  // the primary GET connection

    UINT32                      m_ulCloakMode;  // Cloaking mode.

public:

    int                         m_iProcNum;     // Streamer for the connection
    CHXSimpleList               m_POSTHandlers; // Associated POST connections
    HX_MUTEX                    m_pMutex;       // functional mutex protection
};


#endif  /* _CLOAK_COMMON_H_ */
