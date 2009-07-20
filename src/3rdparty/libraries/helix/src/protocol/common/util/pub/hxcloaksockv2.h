/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxcloaksockv2.h,v 1.4 2005/10/03 21:17:32 gashish Exp $
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
 * Contributor(s):
 * 
 * ***** END LICENSE BLOCK ***** */

#ifndef HXCLOAKV2TCP_H
#define HXCLOAKV2TCP_H

#include "hxtypes.h"
#include "hxcom.h"
#include "hxengin.h"
#include "hxnet.h"
#include "hxpnets.h"
#include "growingq.h"
#include "httpmsg.h"
#include "hxauthn.h"

class HXCloakedV2TCPSocket: public IHXSocket, 
                            public IHXCloakedTCPSocket,
                            public IHXHTTPProxy, 
                            public IHXSocketResponse,
                            public IHXInterruptSafe,
                            public IHXResolveResponse,
                            public IHXClientAuthResponse
{
public:

    HXCloakedV2TCPSocket(IUnknown* pContext);
    ~HXCloakedV2TCPSocket();
    
    //IUnknown 
    STDMETHOD(QueryInterface)  (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef) (THIS);
    STDMETHOD_(ULONG32,Release)(THIS);

    //IHXSocket methods
    STDMETHOD_(HXSockFamily,GetFamily)      (THIS);
    STDMETHOD_(HXSockType,GetType)          (THIS);
    STDMETHOD_(HXSockProtocol,GetProtocol)  (THIS);

    STDMETHOD(SetResponse)          (THIS_ IHXSocketResponse* pResponse);
    STDMETHOD(SetAccessControl)     (THIS_ IHXSocketAccessControl* pControl);
    STDMETHOD(Init)                 (THIS_ HXSockFamily f,
                                     HXSockType t,
                                     HXSockProtocol p);
    STDMETHOD(CreateSockAddr)       (THIS_ IHXSockAddr** ppAddr);
    STDMETHOD(Bind)                 (THIS_ IHXSockAddr* pAddr);
    STDMETHOD(ConnectToOne)         (THIS_ IHXSockAddr* pAddr);
    STDMETHOD(ConnectToAny)         (THIS_ UINT32 nVecLen, IHXSockAddr** ppAddrVec);
    STDMETHOD(GetLocalAddr)         (THIS_ IHXSockAddr** ppAddr);
    STDMETHOD(GetPeerAddr)          (THIS_ IHXSockAddr** ppAddr);
    STDMETHOD(SelectEvents)         (THIS_ UINT32 uEventMask);
    STDMETHOD(Peek)                 (THIS_ IHXBuffer** ppBuf);
    STDMETHOD(Read)                 (THIS_ IHXBuffer** ppBuf);
    STDMETHOD(Write)                (THIS_ IHXBuffer* pBuf);
    STDMETHOD(Close)                (THIS);
    STDMETHOD(Listen)               (THIS_ UINT32 uBackLog);
    STDMETHOD(Accept)               (THIS_ IHXSocket** ppNewSock, IHXSockAddr** ppSource);
    STDMETHOD(GetOption)            (THIS_ HXSockOpt name, UINT32* pval);
    STDMETHOD(SetOption)            (THIS_ HXSockOpt name, UINT32 val);
    STDMETHOD(PeekFrom)             (THIS_ IHXBuffer** ppBuf, IHXSockAddr** ppAddr);
    STDMETHOD(ReadFrom)             (THIS_ IHXBuffer** ppBuf, IHXSockAddr** ppAddr);
    STDMETHOD(WriteTo)              (THIS_ IHXBuffer* pBuf, IHXSockAddr* pAddr);
    STDMETHOD(ReadV)                (THIS_ UINT32 nVecLen, UINT32* puLenVec,
                                     IHXBuffer** ppBufVec);
    STDMETHOD(ReadFromV)            (THIS_ UINT32 nVecLen, UINT32* puLenVec,
                                     IHXBuffer** ppBufVec,
                                     IHXSockAddr** ppAddr);
    STDMETHOD(WriteV)               (THIS_ UINT32 nVecLen,
                                     IHXBuffer** ppBufVec);
    STDMETHOD(WriteToV)             (THIS_ UINT32 nVecLen,
                                     IHXBuffer** ppBufVec,
                                     IHXSockAddr* pAddr);
    
    //IHXInterruptSafe methods
    STDMETHOD_(HXBOOL,IsInterruptSafe) (THIS) {return TRUE;};

    //IHXHTTPProxy methods
    STDMETHOD(SetProxy) (THIS_ const char* pProxyName, UINT16 nPort);

    //IHXCloakv2tcpSocket::InitCloak
    STDMETHOD(InitCloak) (THIS_ IHXValues* pValues, IUnknown* pUnknown);

    //IHXSocketResponse methods
    STDMETHOD(EventPending) (THIS_ UINT32 uEvent, HX_RESULT status);

    //IHXResolverResponse methods
    STDMETHOD(GetAddrInfoDone) (THIS_ HX_RESULT status, UINT32 nVecLen, IHXSockAddr** ppAddrVec);
    STDMETHOD(GetNameInfoDone) (THIS_ HX_RESULT status, const char* pNode, const char* pService);

    //IHXClientAuthResponse methods.
    STDMETHOD(ResponseReady) (THIS_ HX_RESULT status, IHXRequest* pRequestResponse);

    //This is the string we append to the end of the URL to
    //generate the MD5 hash.
    static const char* const zm_HashName;
    
    //The list of names we use for the name value pair that we append
    //to the end of the URL. The name changes but the value is always
    //the MD5 hash of the filename with
    static const char* const zm_NameList[];
    static const int   zm_nNameCount;

protected:

    //The overall state of the cloaked connection.
/*     typedef enum */
/*     { */
/*         csDisconnected, //No setup has happend, or we closed the socket. */
/*         csConnected,    //TCP connection but no GET/POST setup has been */
/*                         //done. */
/*         csPreping,      //We have sent the GET and POST requests but */
/*                         //have not received the GET response yet. */
/*         csReady,        //We have recieved the GET response with an */
/*                         //200 OK We can now let the RTSP layer know */
/*                         //that it can send/receive RTSP traffic. */
/*         csTearingDown,  //We have sent a zero chunk through the POST */
/*                         //channel to signal we are shutting down. */
/*     } cloakState; */

    typedef enum
    {
        csDisconnected, 
        csConnected,      
        csPreping,        
        csReady,
        csPostWait,
        csGetWait,
        csTearingDown,  
                        
    } cloakState;

    static const int   zm_QueueSize;
    
    HXBOOL _GetInterruptSafe(IHXSocketResponse* pResponseObject);
    
    HX_RESULT _EstablishGetAndPost();
    void      _CreateRandomString(CHXString& str );
    void      _ResetResourceAndHash();
    CHXString _GetHTTPResourcePath();
    CHXString _GetHTTPHost();
    CHXString _GenerateGetRequest();
    HX_RESULT _GeneratePostRequest(IHXBuffer* pBuf, IHXBuffer*& ppNew);
    HX_RESULT _WriteString( const CHXString& str );
    HX_RESULT _ParseGetResponse();
    HX_RESULT _ReadFromSocket();
    HX_RESULT _HandleReadEvent();
//    HX_RESULT _ChunkBuffer(IHXBuffer*, IHXBuffer*& pNew);
    HX_RESULT _UnChunkBuffer(IHXBuffer*, IHXBuffer*& pNew, ULONG32& ulBytesUsed);
    HX_RESULT _ReadChunk(ULONG32& ulSizeRead );
    HX_RESULT _GetReadQueue(IHXBuffer*& pNew );
    HX_RESULT _DoProxyAuthentication(HTTPResponseMessage* pMess);
    CHXString _GetProxyAuthString();

private:
    
    IUnknown*           m_pContext;
    IUnknown*           m_pCloakContext;
    IHXScheduler*       m_pScheduler;
    IHXNetServices*     m_pNetworkServices;
    IHXInterruptSafe*   m_pSchedulerState;
    LONG32              m_lRefCount;
    IHXSocketResponse*  m_pTCPResponse;
    HXBOOL                m_bResponseIsInterruptSafe;
    IHXSocket*          m_pTCPSocket;
    UINT16              m_nProxyPort;
    char*               m_pszProxyName;
    IHXSockAddr*        m_pDestAddr;
    IHXSockAddr*        m_pProxyAddr;
    IHXResolve*         m_pResolver;
    cloakState          m_CloakingState;
    CHXString           m_ResourceName;      
    CHXString           m_NVPairValue;
    CHXString           m_NVPairName;
    
    CBigByteGrowingQueue   m_ReadQueue;
    CBigByteGrowingQueue   m_RTSPQueue;

    HXSockFamily   m_family;
    HXSockType     m_type;
    HXSockProtocol m_protocol;

};

#endif /* HXCLOAKV2TCP_H */
