/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: basecloak.h,v 1.4 2005/09/01 19:08:34 srobinson Exp $
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

#ifndef _BASECLOAK_H_
#define _BASECLOAK_H_

#include "hxtypes.h"


class BaseCallback;
class CByteQueue;
class HTTPParser;
class CloakedHTTPResponse;
class CloakedHTTPRVResponse;
class CHXString;
class Dict;
class HTTPMessage;
class HTTPGetMessage;
class HTTPPostMessage;
class HTTPRequestMessage;
class HTTPResponseMessage;
class Player;
class Process;
class HTTPProtocolMgr;
class CloakedGUIDDict;
class CloakConn;

struct IHXBuffer;

#ifndef MAX_POST_MESSAGE_LENGTH
#define MAX_POST_MESSAGE_LENGTH 3000
#endif

class CBaseCloakGETHandler : public IHXHTTPDemuxResponse
{
public:
    CBaseCloakGETHandler(void);
    virtual ~CBaseCloakGETHandler(void);

    // IUnknown
    STDMETHOD(QueryInterface)       (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)      (THIS);
    STDMETHOD_(ULONG32,Release)     (THIS);

    // IHXHTTPDemuxResponse
    STDMETHOD_(HTTP_TYPE, GetHandlerType)   (THIS) = 0;
    STDMETHOD_(BOOL, AutoDispatch)          (THIS);
    STDMETHOD_(void, Init)                  (THIS_ IHXHTTPDemux* pDemux);
    STDMETHOD_(void, OnDispatch)            (THIS);
    STDMETHOD_(void, OnRequest)             (THIS_ HTTPRequestMessage* pMsg);
    STDMETHOD_(void, OnResponse)            (THIS_ HTTPResponseMessage* pMsg);
    STDMETHOD_(void, OnData)                (THIS_ IHXBuffer* pBuf);
    STDMETHOD_(void, OnWriteReady)          (THIS);
    STDMETHOD_(void, OnClosed)              (THIS);
    STDMETHOD_(UINT32, GetFeatureFlags)     (THIS);

    virtual void        PostReceived(void) {}

    HX_RESULT           SendData(IHXBuffer* pBuf);

    void                PostTimeout(void);
    HX_RESULT           Write(IHXBuffer* pBuf);
    void                Shutdown(void);
    void                CleanupConn(void);

    void                sendPostStatus(Byte status);
    void                sendResponseHeader(const char* szContentType);

    virtual BOOL        IsMultiPOST(void) { return FALSE; }

    class CloakGETShutdownCallback : public BaseCallback
    {
    public:
        CloakGETShutdownCallback(CBaseCloakGETHandler* pCGH)
        : m_pCGH(pCGH)
        {
            m_pCGH->AddRef();
        }
        ~CloakGETShutdownCallback();
        STDMETHOD(Func) (THIS);

        CBaseCloakGETHandler* m_pCGH;
    };


protected:

    INT32                       m_nRefCount;
    IHXHTTPDemux*               m_pDemux;
    Process*                    m_pProc;
    BOOL                        m_bGotRequest;
    CloakConn*                  m_pConn;

    UINT32                      m_ulCGSCHandle;
    BOOL                        m_bOnClosedCalled;
    CloakedGUIDDict*            m_pCloakedGUIDDict;
    char*                       m_szCloakSessionId;
};

class CBaseCloakPOSTHandler : public IHXHTTPDemuxResponse
{

public:

    CBaseCloakPOSTHandler(void);
    virtual ~CBaseCloakPOSTHandler(void);

    // IUnknown
    STDMETHOD(QueryInterface)       (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)      (THIS);
    STDMETHOD_(ULONG32,Release)     (THIS);

    // IHXHTTPDemuxResponse
    STDMETHOD_(HTTP_TYPE, GetHandlerType)   (THIS) = 0;
    STDMETHOD_(BOOL, AutoDispatch)          (THIS);
    STDMETHOD_(void, Init)                  (THIS_ IHXHTTPDemux* pDemux);
    STDMETHOD_(void, OnDispatch)            (THIS);
    STDMETHOD_(void, OnRequest)             (THIS_ HTTPRequestMessage* pMsg) = 0;
    STDMETHOD_(void, OnResponse)            (THIS_ HTTPResponseMessage* pMsg);
    STDMETHOD_(void, OnData)                (THIS_ IHXBuffer* pBuf) = 0;
    STDMETHOD_(void, OnWriteReady)          (THIS);
    STDMETHOD_(void, OnClosed)              (THIS);
    STDMETHOD_(UINT32, GetFeatureFlags)     (THIS);

    void        BeginClose(void);
    void        ProcessPendingData(void);
    void        Shutdown(void);
    void        CleanupConn(HX_RESULT status);

    class CloakPOSTShutdownCallback : public BaseCallback
    {
    public:
        CloakPOSTShutdownCallback(CBaseCloakPOSTHandler* pCPH)
        : m_pCPH(pCPH)
        {
            m_pCPH->AddRef();
        }
        ~CloakPOSTShutdownCallback();
        STDMETHOD(Func) (THIS);

        CBaseCloakPOSTHandler* m_pCPH;
    };

protected:

    void                        QueuePendingData(BYTE* pData, UINT32 ulDataLen);

    INT32                       m_nRefCount;
    IHXHTTPDemux*               m_pDemux;
    Process*                    m_pProc;
    BOOL                        m_bGotRequest;
    CloakConn*                  m_pConn;

    BOOL                        m_bProcessingPendingData;
    UINT32                      m_uContentLen;
    IHXBuffer*                  m_pPendingData;
    UINT32                      m_ulCPSCHandle;
    BOOL                        m_bOnClosedCalled;
    CloakedGUIDDict*            m_pCloakedGUIDDict;
    char*                       m_szCloakSessionId;
};

#endif  /* _BaseCLOAK_H_ */
