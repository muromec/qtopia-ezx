/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: inetwork.h,v 1.11 2007/09/04 18:20:38 seansmith Exp $
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

#ifndef _HXNETAPI_H_
#define _HXNETAPI_H_

#include "hxcom.h"
#include "hxslist.h"
#include "hxengin.h"
#include "hxerror.h"
#include "hxmon.h"
#include "sockio.h"
#include "base_callback.h"
#include "simple_callback.h"
#include "mem_cache.h"

class INetworkAcceptor;
class TCPIO;
class UDPIO;
class SIO;
class VIO;
class CByteQueue;
class ServerAccessControl;
class IHXNetworkServicesContext;
class SharedUDPPortReader;


class IHXNetworkServicesContext : public IHXNetworkServices
{
public:
    IHXNetworkServicesContext();
    ~IHXNetworkServicesContext();

    STDMETHOD(QueryInterface)   (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)  (THIS);
    STDMETHOD_(ULONG32,Release) (THIS);
    STDMETHOD(Init)             (THIS_ IUnknown* pUnknown, Engine* pEngine,
                                ServerAccessControl* pAccessCtrl);
       //IHXNetworkServices
    STDMETHOD(CreateTCPSocket)  (THIS_ IHXTCPSocket** ppTCPSocket) PURE;
    STDMETHOD(CreateUDPSocket)  (THIS_ IHXUDPSocket** ppUDPSocket);
    STDMETHOD(CreateListenSocket) (THIS_ IHXListenSocket** ppListenSocket);
    STDMETHOD(CreateResolver)   (THIS_ IHXResolver** ppResolver) PURE;

protected:
    LONG32                      m_lRefCount;
    IUnknown*                   m_pUnknown;
    Engine*                     m_pEngine;
    ServerAccessControl*        m_pAccessCtrl;
    IHXErrorMessages*           m_pMessages;
};

class INetworkServicesContext : public IHXNetworkServicesContext
{
public:
    INetworkServicesContext()   { };
    ~INetworkServicesContext()  { };

    STDMETHOD(CreateTCPSocket)  (THIS_ IHXTCPSocket** ppTCPSocket);
    STDMETHOD(CreateResolver)   (THIS_ IHXResolver** ppResolver);
};

class IHXResolverContext: public IHXResolver
{
public:
    IHXResolverContext();
    virtual ~IHXResolverContext();

    STDMETHOD(QueryInterface)   (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)  (THIS);
    STDMETHOD_(ULONG32,Release) (THIS);
    STDMETHOD(Init)             (THIS_ IHXResolverResponse* pResp);
    STDMETHOD(GetHostByName)    (THIS_ const char* pHostName) PURE;

protected:
    LONG32                      m_lRefCount;
    IHXResolverResponse*        m_pResp;
};

class INetworkResolverContext: public IHXResolverContext
{
public:
    INetworkResolverContext()   { };
    ~INetworkResolverContext()  { };

    STDMETHOD(GetHostByName)    (THIS_ const char* pHostName);
};


class ReadData
{
public:
    MEM_CACHE_MEM
    ReadData(IHXBuffer* pBuf, ULONG32 ulAddr, UINT16 nPort)
        : m_pBuffer(pBuf)
        , m_ulAddr(ulAddr)
        , m_nPort(nPort)
    {
        m_pBuffer->AddRef();
    }
    ~ReadData() {HX_RELEASE(m_pBuffer);}
    IHXBuffer* m_pBuffer;
    ULONG32     m_ulAddr;
    UINT16      m_nPort;
};

#define ARRAY_QUEUE_ARRAY_SIZE 5
#define SAMPLE_SET_SIZE 5
#define RESAMPLE_FREQ 200
#define MAX_PACKETS_PER_READ 400
#define SUCCESSIVE_MAINLOOP_ITERATIONS_TRIGGER 3
class BufArrayQueue
{
public:
    BufArrayQueue();
    ~BufArrayQueue();

    void Add(ReadData*);
    ReadData* Remove();
    CHXSimpleList m_List;

    UINT32 m_ulTotal;
    UINT32 m_ulLastRemove;
    ReadData** m_pLast;
    LISTPOSITION m_LastPosition;
};


class IHXUDPSocketContext :  public IHXUDPSocket,
                             public IHXUDPMulticastInit,
                             public IHXSetSocketOption,
                             public IHXFastPathNetWrite,
                             public IHXSharedUDPServices,
                             public IHXUDPConnectedSocket,
                             public IHXBufferedSocket
{
public:
    IHXUDPSocketContext(Engine* pEngine);
    IHXUDPSocketContext(Engine* pEngine, UDPIO*  pUDPIO);
    ~IHXUDPSocketContext();

    STDMETHOD(QueryInterface)   (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)  (THIS);
    STDMETHOD_(ULONG32,Release) (THIS);

    /*
     * IHXUDPSocket methods
     */

    STDMETHOD(Init)             (THIS_
                                ULONG32                 ulAddr,
                                UINT16                  nPort,
                                IHXUDPResponse* pUDPResponse);

    STDMETHOD(Bind)             (THIS_
                                UINT32                      ulLocalAddr,
                                UINT16                      nPort);

    STDMETHOD(Read)             (THIS_
                                UINT16                  Size);

    STDMETHOD(Write)            (THIS_
                                IHXBuffer*              pBuffer);

    STDMETHOD(WriteTo)          (THIS_
                                ULONG32                 ulAddr,
                                UINT16                  nPort,
                                IHXBuffer*              pBuffer);

    STDMETHOD(GetLocalPort)     (THIS_
                                UINT16&                 nPort);

    STDMETHOD(JoinMulticastGroup)       (THIS_
                                        ULONG32     ulMulticastAddr,
                                        ULONG32     ulInterfaceAddr);

    STDMETHOD(LeaveMulticastGroup)      (THIS_
                                        ULONG32     ulMulticastAddr,
                                        ULONG32     ulInterfaceAddr);

    /*
     * IHXBufferedSocket methods
     */
    STDMETHOD(BufferedWrite)            (THIS_
                                        IHXBuffer*);

    STDMETHOD(FlushWrite)               (THIS);

    STDMETHOD(SetDesiredPacketSize)     (THIS_
                                        UINT32);


    /*
     * IHXUDPMulticastInit methods
     */

    STDMETHOD(InitMulticast)            (THIS_
                                        UINT8       ulTTL);

    /*
     * IHXSetSocketOption methods
     */
    STDMETHOD(SetOption)                (THIS_
                                        HX_SOCKET_OPTION option,
                                        UINT32 ulValue);

    STDMETHOD(FastWrite)        (THIS_
                                const UINT8* pBuffer, UINT32 ulLen);

    /*
     * IHXSharedUDPServices methods
     */
    STDMETHOD(RegisterSharedResponse)   (THIS_
                                         IHXUDPResponse* response,
                                         UINT16 sPortEnum);

    STDMETHOD(UnregisterSharedResponse) (THIS);

    STDMETHOD_(UINT16, GetSharedPort)   (THIS);

    /*
     * IHXUDPConnectedSocket methods
     */
    STDMETHOD(UDPConnect)       (THIS);

    STDMETHOD(UDPConnect)       (THIS_  ULONG32 ulAddr, UINT16 nPort);

    STDMETHOD(UDPDisconnect) (THIS);

    STDMETHOD_(BOOL, IsUDPConnected) (THIS);

    STDMETHOD_(BOOL, IsUDPConnected) (THIS_ REF(ULONG32) ulAddr,
                                            REF(UINT16) nPort);


    class UDPSocketReadCallback: public BaseCallback
    {
    public:
        STDMETHOD(Func) (THIS);
        IHXUDPSocketContext* m_pContext;
    };
    friend class UDPSocketReadCallback;
    UDPSocketReadCallback*      m_pReadCallback;

private:
    HX_RESULT                   DoRead();
    inline HX_RESULT            FinishBind(INT32 ret);
    int                         UseIPBinding(UINT32 ulLocalAddr, UINT16 nPort);
    ReadData*                   GetReadData();
    void                        init();
    void                        tini();
    HX_RESULT                   ConnectSockets();
    HX_RESULT                   DisconnectSockets();

    LONG32                      m_lRefCount;
    IHXUDPResponse*             m_pUDPResponse;
    UDPIO**                     m_ppUDPIO;
    UINT32                      m_ulNumUDPIO;
    VIO*                        m_pVio;
    IHXBuffer**                 m_pVectorBuffers;
    HX_IOVEC*                   m_pWriteVectors;
    UINT16                      m_unNumVectors;
    UINT32                      m_ulRoundRobinReads;
    Engine*                     m_pEngine;
    struct sockaddr_in          m_sockAddr;
    UINT16                      m_initPort;
    UINT16                      m_localPort;
    BOOL                        m_bReadCallbackPending;
    ULONG32                     m_nRequired;
    INT32                       m_nRead;
    UINT32                      m_ulDestAddr;
    UINT16                      m_nDestPort;
    enum UDPSocketState
    {
        INIT, ALIVE, DEAD
    };
    UDPSocketState             m_UDPIOState;
    BOOL                        m_bReuseAddr;
    BOOL                        m_bReusePort;
    BufArrayQueue*              m_pReadStore;
    UINT32                      m_pSamples[SAMPLE_SET_SIZE];
    UINT32                      m_ulNextSample;
    UINT32                      m_ulCurrentReadIterations;
    BOOL                        m_bSampleOn;
    BOOL                        m_bSendingReadDones;
    UINT32                      m_ulLastMainloopIteration;
    UINT32                      m_ulSuccessiveMainloopCount;
    BOOL                        m_bSocketIsConnected;
    BOOL                        m_bSocketShouldBeConnected;
    SharedUDPPortReader*        m_pSharedUDPReader;
    UINT16                      m_sPortEnum;
    IHXRegistry*             m_pReg;
};

#ifdef PAULM_IHXTCPSCAR
#include "../server/odbg.h"
#endif


class IHXTCPSocketContext : public IHXTCPSocket,
                             public IHXResolverResponse,
                             public IHXSetSocketOption,
                             public IHXWouldBlock,
                             public IHXBufferedSocket
#ifdef PAULM_IHXTCPSCAR
                                ,public ObjDebugger
#endif
{
public:
    IHXTCPSocketContext(Engine* pEngine, IHXResolver* pResolver);
    IHXTCPSocketContext(Engine* pEngine, IHXResolver* pResolver,
                         TCPIO* sock, UINT32 lForeignAddr,
                         UINT16 sForeignPort);
    virtual ~IHXTCPSocketContext();

    /*
     * IUnknown methods
     */

    STDMETHOD(QueryInterface)   (THIS_ REFIID riid, void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)  (THIS);
    STDMETHOD_(ULONG32,Release) (THIS);

    /*
     *  IHXTCPSocket methods
     */

    STDMETHOD(Init)             (THIS_
                                IHXTCPResponse*    /*IN*/  pTCPResponse);
    STDMETHOD(SetResponse)      (THIS_
                                IHXTCPResponse*    /*IN*/  pTCPResponse);
    STDMETHOD(Bind)             (THIS_
                                UINT32                      ulLocalAddr,
                                UINT16                      nPort);

    STDMETHOD(Connect)          (THIS_
                                const char*                 pDestination,
                                UINT16                      nPort);
    STDMETHOD(Read)             (THIS_
                                UINT16                      Size) PURE;
    STDMETHOD(Write)            (THIS_
                                IHXBuffer*                  pBuffer) PURE;
    STDMETHOD(WantWrite)        (THIS) PURE;

    STDMETHOD(GetForeignAddress)(THIS_
                                UINT32&                     lAddress);
    STDMETHOD(GetForeignPort)   (THIS_
                                UINT16&                     sPort);
    STDMETHOD(GetLocalAddress)  (THIS_
                                UINT32&                     lAddress);
    STDMETHOD(GetLocalPort)     (THIS_
                                UINT16&                     sPort);

    /*
     * IHXResolverResponse methods
     */
    STDMETHOD(GetHostByNameDone)        (THIS_
                                        HX_RESULT status,
                                        ULONG32 ulAddr);

    /*
     * IHXSetSocketOption methods
     */
    STDMETHOD(SetOption)                (THIS_
                                        HX_SOCKET_OPTION option,
                                        UINT32 ulValue);
    /*
     * IHXWouldBlock methods
     */
    STDMETHOD(WantWouldBlock)   (THIS_
                                IHXWouldBlockResponse*, UINT32);

    /*
     * IHXBufferedSocket methods
     */
    STDMETHOD(BufferedWrite)            (THIS_
                                        IHXBuffer*);

    STDMETHOD(FlushWrite)               (THIS);

    STDMETHOD(SetDesiredPacketSize)     (THIS_
                                        UINT32);

    BOOL SupportsBufferedSocket() { return m_bSupportsBufferedSocket; }
    class TCPSocketReadCallback: public BaseCallback
    {
    public:
        STDMETHOD(Func) (THIS);
        IHXTCPSocketContext* m_pContext;
    };
    friend class TCPSocketReadCallback;
    TCPSocketReadCallback*      m_pReadCallback;

    class TCPSocketWriteCallback: public BaseCallback
    {
    public:
        STDMETHOD(Func) (THIS);
        IHXTCPSocketContext* m_pContext;
    };
    friend class TCPSocketWriteCallback;
    TCPSocketWriteCallback*     m_pWriteCallback;

    class TCPSocketConnectCallback: public BaseCallback
    {
    public:
        STDMETHOD(Func) (THIS);
        IHXTCPSocketContext* m_pContext;
    };
    friend class TCPSocketConnectCallback;
    TCPSocketConnectCallback*   m_pConnectCallback;

    class TCPSocketConnectTimeoutCallback: public BaseCallback
    {
    public:
        STDMETHOD(Func) (THIS);
        IHXTCPSocketContext* m_pContext;
    };
    friend class TCPSocketConnectTimeoutCallback;

    class TCPSocketEnableReadCallback: public BaseCallback
    {
    public:
        STDMETHOD(Func) (THIS);
        IHXTCPSocketContext* m_pContext;
    };
    friend class TCPSocketEnableReadCallback;

    class TCPSocketWriteFlushTimeoutCallback: public BaseCallback
    {
    public:
        STDMETHOD(Func) (THIS);
        IHXTCPSocketContext* m_pContext;
    };
    friend class TCPSocketWriteFlushTimeoutCallback;

    virtual void                disableRead() = 0;
    virtual void                enableRead() = 0;
    virtual int                 readUndo(BYTE* pMsg, UINT32 nBytes) = 0;
    virtual void                disconnect() = 0;
    virtual void                reconnect(Engine* pEngine) = 0;
    virtual int                 write_flush() = 0;

    virtual TCPIO*              getReadTCPIO() = 0;
    virtual TCPIO*              getWriteTCPIO() = 0;
    virtual SIO*                getReadSIO() = 0;
    virtual SIO*                getWriteSIO() = 0;

    //XXXBAB make them public until the server/protocol code gets cleaned up
    TCPIO*                      m_pCtrl;
    SIO*                        m_pCmd;
    VIO*                        m_pVio;

    virtual HX_RESULT           DoRead() = 0;
    virtual void                DoWrite() = 0;

protected:
    void                        disableConnectCallback();
    void                        init();

    LONG32                      m_lRefCount;
#ifdef PAULM_IHXTCPSCAR
public:
#endif
    IHXTCPResponse*             m_pTCPResponse;
#ifdef PAULM_IHXTCPSCAR
protected:
#endif
    IHXResolver*                m_pResolver;
    Engine*                     m_pEngine;
    int                         m_bReadCallbackPending;
    int                         m_bReadCallbackEnabled;
    int                         m_bWantWriteCallbackPending;
    int                         m_bWriteFlushCallbackPending;
    int                         m_bConnectCallbackPending;
    UINT16                      m_nRequired;
    UINT32                      m_nConnectTimeoutID;
    UINT32                      m_nEnableReadID;
    UINT32                      m_nWriteFlushTimeoutID;
    UINT32                      m_lForeignAddress;
    UINT16                      m_sForeignPort;
    BOOL                        m_bConnected;
    BOOL                        m_bTCPIOInitialized;
    BOOL                        m_bInitCalled;
    CByteQueue*                 m_pUndoBuf;
    BOOL                        m_bReuseAddr;
    BOOL                        m_bReusePort;
    IHXWouldBlockResponse*      m_pWouldBlockResponse;
    UINT32                      m_ulWouldBlockResponseID;
    BOOL                        m_bWantWriteFromWB;
    virtual IHXTCPSocketContext* GetNextTCPSocketContext(){return NULL;}

    // For IHXBufferedSocket
    BOOL                        m_bBufferedSocketClosed;
    BOOL                        m_bWantWriteVFromWB;
    BOOL                        m_bWantWriteVCallbackPending;
    IHXBuffer**                 m_pVectorBuffers;
    HX_IOVEC*                   m_pWriteVectors;
    int                         m_nIOVectorSize;
    UINT32                      m_ulBufferedDataSize;
    UINT32                      m_ulBufferCount;
    UINT32                      m_ulPacketSize;
    UINT32                      m_ulFlushCount;
    BOOL                        m_bSupportsBufferedSocket;
    IHXRegistry*                m_pReg;

    void DoWriteV(BOOL bOnCleared = FALSE);
};

class INetworkTCPSocketContext : public IHXTCPSocketContext
{
public:
    INetworkTCPSocketContext(Engine* pEngine, IHXResolver* pResolver);
    INetworkTCPSocketContext(Engine* pEngine, IHXResolver* pResolver,
                             TCPIO* sock, UINT32 lForeignAddr,
                             UINT16 sForeignPort);
    ~INetworkTCPSocketContext();

    /*
     * IUnknown methods
     */


    STDMETHOD(Read)             (THIS_
                                UINT16                      Size);
    STDMETHOD(Write)            (THIS_
                                IHXBuffer*                  pBuffer);
    STDMETHOD(WantWrite)        (THIS);

    void                        disableRead();
    void                        enableRead();
    int                         readUndo(BYTE* pMsg, UINT32 nBytes);
    void                        disconnect();
    void                        reconnect(Engine* pEngine);
    int                         write_flush();

    TCPIO*                      getReadTCPIO();
    TCPIO*                      getWriteTCPIO();
    SIO*                        getReadSIO();
    SIO*                        getWriteSIO();

    HX_RESULT                   DoRead();
    void                        DoWrite();
    void                        WriteFlushTimeout();
};

inline TCPIO*
INetworkTCPSocketContext::getReadTCPIO()
{
    return m_pCtrl;
}

inline TCPIO*
INetworkTCPSocketContext::getWriteTCPIO()
{
    return m_pCtrl;
}

inline SIO*
INetworkTCPSocketContext::getReadSIO()
{
    return m_pCmd;
}

inline SIO*
INetworkTCPSocketContext::getWriteSIO()
{
    return m_pCmd;
}

class IHXListenSocketContext : public IHXListenSocket
{
public:
    IHXListenSocketContext(Engine* pEngine, IHXErrorMessages* pMessages,
                            ServerAccessControl* pAccessCtrl);
    ~IHXListenSocketContext();

    STDMETHOD(QueryInterface)   (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)  (THIS);
    STDMETHOD_(ULONG32,Release) (THIS);

    /*
     *  IHXListenSocket methods
     */

    STDMETHOD(Init)             (THIS_
                                UINT32              ulLocalAddr,
                                UINT16              port,
                                IHXListenResponse* pListenResponse);

protected:
    HX_RESULT                   _Init(UINT32 ulLocalAddr,
                                        UINT32 port,
                                        IHXListenResponse* pListenResponse);
    LONG32                      m_lRefCount;
    Engine*                     m_pEngine;
    IHXListenResponse*          m_pListenResponse;
    INetworkAcceptor**          m_ppAcceptors;
    UINT32                      m_ulNumAcceptors;
    IHXErrorMessages*           m_pMessages;
    ServerAccessControl*        m_pAccessCtrl;
};

#endif /*_HXNETAPI_H_*/
