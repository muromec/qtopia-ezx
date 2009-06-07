/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxcloakedsocket.h,v 1.4 2005/03/10 20:59:17 bobclark Exp $
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
#ifndef HXCLOAKEDSCOKET_H__
#define HXCLOAKEDSCOKET_H__

#include "hxtypes.h"
#include "hxcom.h"
#include "hxengin.h"
#include "hxnet.h"
#include "hxauthn.h"
#include "hxpreftr.h"
#include "hxstring.h"
#include "hxstring.h"
#include "hxslist.h"
#include "growingq.h"
#include "hxpnets.h"
#include "hxccf.h"

class HTTPResponseMessage;
class HXMutex;

class HXCloakedSocket : public IHXSocket,
                        public IHXResolveResponse,
                        public IHXClientAuthResponse,
                        public IHXHTTPProxy,
                        public IHXCloakedTCPSocket
{
public:
    HXCloakedSocket(IUnknown* pContext, HXBOOL bBlockResistMode);
    ~HXCloakedSocket();
    
    /* IUnknown interface */
    STDMETHOD(QueryInterface)   (THIS_ 
                                REFIID riid, void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)  (THIS);

    STDMETHOD_(ULONG32,Release) (THIS);

    STDMETHOD(SetProxy)         (THIS_
                                const char* /*IN*/  pProxyHostName,
                                UINT16      /*IN*/  nPort);

    STDMETHOD(InitCloak)        (THIS_
                                IHXValues* /*IN*/  pValues,
                                IUnknown* pUnknown);

        
    // IHXResolveResponse
    STDMETHOD(GetAddrInfoDone)      (THIS_ HX_RESULT status, UINT32 nVecLen, IHXSockAddr** ppAddrVec);
    STDMETHOD(GetNameInfoDone)      (THIS_ HX_RESULT status, const char* pszNode, const char* pszService);


    // IHXSocket
    STDMETHOD_(HXSockFamily,GetFamily)      (THIS);
    STDMETHOD_(HXSockType,GetType)          (THIS);
    STDMETHOD_(HXSockProtocol,GetProtocol)  (THIS);

    STDMETHOD(SetResponse)          (THIS_ IHXSocketResponse* pResponse);
    STDMETHOD(SetAccessControl)     (THIS_ IHXSocketAccessControl* pControl);

    STDMETHOD(Init)                 (THIS_ HXSockFamily f, HXSockType t, HXSockProtocol p);
    STDMETHOD(CreateSockAddr)       (THIS_ IHXSockAddr** ppAddr);

    STDMETHOD(Bind)                 (THIS_ IHXSockAddr* pAddr);
    STDMETHOD(ConnectToOne)         (THIS_ IHXSockAddr* pAddr);
    STDMETHOD(ConnectToAny)         (THIS_ UINT32 nVecLen, IHXSockAddr** ppAddrVec);
    STDMETHOD(GetLocalAddr)         (THIS_ IHXSockAddr** ppAddr);
    STDMETHOD(GetPeerAddr)          (THIS_ IHXSockAddr** ppAddr);

    STDMETHOD(SelectEvents)         (THIS_ UINT32 uEventMask);
    STDMETHOD(Peek)                 (THIS_ IHXBuffer** pBuf);
    STDMETHOD(Read)                 (THIS_ IHXBuffer** pBuf);
    STDMETHOD(Write)                (THIS_ IHXBuffer* pBuf);
    STDMETHOD(Close)                (THIS);

    STDMETHOD(Listen)               (THIS_ UINT32 uBackLog);
    STDMETHOD(Accept)               (THIS_ IHXSocket** ppNewSock,
                                           IHXSockAddr** ppSource);

    STDMETHOD(GetOption)            (THIS_ HXSockOpt name, UINT32* pval);
    STDMETHOD(SetOption)            (THIS_ HXSockOpt name, UINT32 val);
    
    STDMETHOD(PeekFrom)             (THIS_ IHXBuffer** ppBuf,
                                           IHXSockAddr** ppAddr);
    STDMETHOD(ReadFrom)             (THIS_ IHXBuffer** ppBuf,
                                           IHXSockAddr** ppAddr);
    STDMETHOD(WriteTo)              (THIS_ IHXBuffer* pBuf,
                                           IHXSockAddr* pAddr);


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



    // IHXClientAuthResponse
    
    STDMETHOD(ResponseReady)    (THIS_
                                HX_RESULT   HX_RESULTStatus,
                                IHXRequest* pIHXRequestResponse);
    
 
    class SockResponse : public IHXSocketResponse
    {
    public:

        SockResponse(HXCloakedSocket* pOwner, HXBOOL bIsGetResponse);
        ~SockResponse();
        
        /* IUnknown */
        STDMETHOD(QueryInterface) (THIS_
                                  REFIID riid,
                                  void** ppvObj);

        STDMETHOD_(ULONG32,AddRef)  (THIS);
        STDMETHOD_(ULONG32,Release) (THIS);
        /* IHXSocketResponse */
        STDMETHOD(EventPending)     (THIS_ UINT32 uEvent, HX_RESULT status);

    private:
        LONG32            m_lRefCount;
        HXCloakedSocket*  m_pOwner;
        HXBOOL              m_bIsGetResponse;
    };

    friend class SockResponse;
    friend class Callback;

private:

    // initial connect state
    enum ConnectState
    {
        csInitialized,
        csResolveProxy,
        csConnectBoth,
        csNGConnectPost,
        csNGWaitPostResponse,
        csNGConnectGet,
        csConnected,
        csClosed
    } m_connectState;


    // get socket connect and read state
    enum GetSockState
    {
        gsConnectPending,
        gsReconnectPending,
        gsHeaderPending,
        gsReconnectHeaderPending,
        gsHeaderResponseCode,
        gsReadOptions,
        gsReadData,
        gsClosed
    } m_getState;

    HXBOOL IsGetSockConnected()
    {
        return (m_getState >= gsHeaderPending && m_getState <= gsReadData);
    }

    // post socket connect and read state
    enum PostSockState
    {
        psConnectPending,
        psReconnectPending,
        psHeaderPending,
        psOpen,
        psClosed
    } m_postState;

    HXBOOL IsPostSockConnected()
    {
        return (m_postState == psHeaderPending || m_postState == psOpen);
    }

    // HXLOG helpers
    const char* TransConnectState(ConnectState state);
    const char* TransGetState(GetSockState state);  
    const char* TransPostState(PostSockState state);

    void SetPostState(PostSockState state);
    void SetGetState(GetSockState state);
    void SetConnectState(ConnectState state);

    void SetEventFlag(UINT32 flag);
    void ClearEventFlag(UINT32 flag);
    HXBOOL IsEventFlagSet(UINT32 flag);
    void EnsureEventSent(UINT32 event, HX_RESULT status);
    void SendReadEvent();
    void SendCloseEvent(HX_RESULT status);
    void SendConnectEvent(HX_RESULT status);


    HX_RESULT DoConnect(IHXSocket* pSocket);
    HX_RESULT ReconnectPostSocket();
    HX_RESULT CreateSocketHelper(HXSockFamily family, IHXSocketResponse* pResponse, IHXSocket*& pSock);
    HX_RESULT SetServerAddr(IHXSockAddr* pAddr);
    HX_RESULT UpdateServerAddr(const char* pszIP);
    HX_RESULT GetServerAddrString(CHXString& str, HXBOOL urlFormat = false);
    int       FormatMethodLine(char* pBuf, int cchBuf, const char* pszMethod /*get or post*/);
    void      DoCloseSockets(HX_RESULT hr);
    HX_RESULT HandleReadEventHelper(HX_RESULT status, IHXSocket* pSock, IHXBuffer*& pBufOut);
    HX_RESULT CheckAndHandleBothSocketsConnected();
    HX_RESULT ProcessGetHeaderResponseCode();
    HX_RESULT ProcessGetOptions();

    HX_RESULT ReadHTTPHeader(CByteGrowingQueue* pInboundData);
    HX_RESULT ReadHTTPHeader(CHXSimpleList& list);
    HX_RESULT WriteSocketHelper(CByteGrowingQueue* pOutBoundData, IHXSocket* pSock);
    HX_RESULT PrepareNextPostSocketWrite();
    HX_RESULT SendPost();
    HX_RESULT SendGet();


    HX_RESULT TryGetSocketWrite();
    HX_RESULT TryPostSocketWrite();
    HX_RESULT HandleGetSockReadEvent(HX_RESULT status);
    HX_RESULT HandlePostSockReadEvent(HX_RESULT status);
    HX_RESULT HandleGetSockWriteEvent(HX_RESULT status);
    HX_RESULT HandlePostSockWriteEvent(HX_RESULT status);
    void      HandleGetSockConnectEvent(HX_RESULT status);
    void      HandlePostSockConnectEvent(HX_RESULT status);
    void      HandleGetSockCloseEvent(HX_RESULT status);
    void      HandlePostSockCloseEvent(HX_RESULT status);

    HX_RESULT PreparePostMessage(const UCHAR *inData, UINT16 inLength);
    HX_RESULT EncodeBase64(const UCHAR* inData, UINT16 inLength, UCHAR* outData, UINT16& outLength);
    HX_RESULT PrepareGetMessage();
    HX_RESULT CreateGuid();
    HX_RESULT ProcessGetHTTPResponseOpcode(UCHAR response);

    void      FlushQueues();
    void      SendHTTPDone();
    HX_RESULT ConnectToOneHelper();
    void      GetServerIPFromResponse(CHXString& serverIP, const char* pszInBuffer);
    HX_RESULT StartOverAfterAuth();
    HX_RESULT ReconnectGetSocketIfNeeded();

    HXBOOL      StartAuthenticationIfNeeded(IHXBuffer* pInBuffer);
    void      GetAuthenticationInformation(CHXString& strAuth);

    HX_RESULT HandleAuthentication(IHXRequest* pRequest, HTTPResponseMessage* pMessage,
                                                const char* pHost, const char* pProxyHost);

    LONG32             m_lRefCount;
    IUnknown*          m_pContext;
    IHXSocketResponse* m_pResponse;
    IHXNetServices*    m_pNetServices;  
    IHXResolve*        m_pResolver;
    IHXSocket*         m_pGetSock;
    IHXSocket*         m_pPostSock;
    SockResponse*      m_pGetSockResponse;
    SockResponse*      m_pPostSockResponse;
    IHXBuffer*         m_pRedirectBuffer;
    CByteGrowingQueue* m_pOutboundGetData;
    CHXSimpleList      m_InboundGetData;
    CByteGrowingQueue* m_pInboundPostData;
    CByteGrowingQueue* m_pOutboundPreEncodedPostData;
    CByteGrowingQueue* m_pOutboundPostData;
    CHXSimpleList      m_PendingWriteBuffers;
    char*              m_pOutBuf;
    char*              m_pOutEncodedBuf;
    char*              m_pInBuf;
    char*              m_pGuid;
    HXBOOL               m_bDeletePadding;
    UINT16             m_uPadLength;
    HXBOOL               m_bIsHttpNg;
    HXBOOL               m_bMultiPostMode;
    HXBOOL               m_bEndStreamPending;
    IHXSockAddr*       m_pServerAddr;
    IHXSockAddr*       m_pProxyAddr;
    CHXString          m_serverName; // passed in to ConnectToOne() from user
    CHXString          m_getServerIP;
    CHXString          m_postServerIP;
    CHXString          m_proxyName;
    UINT16             m_proxyPort;
    IHXBuffer*         m_pHTTPHeaderBuffer;
    IHXValues*         m_pCloakValues;
    IUnknown*          m_pCloakContext;

    IHXPreferredTransport*        m_pPreferredTransport;
    IHXPreferredTransportManager* m_pPreferredTransportManager;

    UINT32  m_selectedEvents;   // tracks event response wants to receive
    UINT32  m_eventFlags;       // tracks event notification we've sent

    IHXClientAuthConversation* m_pClientAuthConversationAuthenticator;

    //
    // New RTSPviaHTTP1.1 cloaking fallback support
    //
    HXSockFamily m_family;
    HXBOOL         m_bBlockingResistant;
    CHXString    m_NVPairValue;
    CHXString    m_NVPairName;
    CHXString    m_ResourceName;
    
    void         _ResetResourceAndHash();
    
    IHXCommonClassFactory *m_pCCF;

  protected:
    
    HX_RESULT  _DeQueue(CHXSimpleList& list, char* pBuff, UINT16 unCount );
    HX_RESULT  EnQueue(CHXSimpleList& list, const char* pBuff, UINT16 unCount );
    HX_RESULT  DeQueue(CHXSimpleList& list, char* pBuff, UINT16 unCount );
    HX_RESULT  DiscardData(CHXSimpleList& list, UINT16 unCount);
    UINT16     GetBytesAvailable(CHXSimpleList& list );
    HX_RESULT  _BuildHTTPHeaderBuffer(UINT16 size);
    HX_RESULT _FindEndOfResponse(char* pBuff, char*& pText, char*& pEnd, UINT16 count);
    
};

#endif /* HXCLOAKEDSCOKET_H__ */
