/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: http_demux.h,v 1.10 2006/10/03 23:19:07 tknox Exp $
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

#ifndef _HTTP_DEMUX_H_
#define _HTTP_DEMUX_H_

#include "hxnet.h"
#include "hxprot.h"

_INTERFACE IHXBuffer;
_INTERFACE IHXHTTPDemux;
_INTERFACE IHXHTTPDemuxResponse;

class CHXServSocket;
class Client;
class HTTPMessage;
class HTTPParser;
class HTTPRequestMessage;
class HTTPResponseMessage;
class MIMEHeader;
class Process;

#define HTTP_FEATURE_NONE                      0
#define HTTP_FEATURE_V11_SUPPORT              (1 << 0)
#define HTTP_FEATURE_CHUNKED_ENCODING_SUPPORT (1 << 1)
#define HTTP_FEATURE_IGNORE_CONTENT_LENGTH    (1 << 2)

class HTTPParser;

enum HTTP_TYPE
{
    HTTP_TYPE_NONE,
    HTTP_TYPE_NORMAL,
    HTTP_TYPE_CLOAK_RN1,
    HTTP_TYPE_CLOAK_RN2,
    HTTP_TYPE_CLOAK_QT,
    HTTP_TYPE_CLOAK_WMT,
    HTTP_TYPE_LAST
};

enum DEMUX_READ_STATE
{
    DEMUX_READ_INIT,
    DEMUX_READ_MSG,
    DEMUX_READ_DATA,
    DEMUX_READ_LAST
};

typedef _INTERFACE IHXHTTPDemuxResponse IHXHTTPDemuxResponse;
typedef _INTERFACE IHXHTTPDemux         IHXHTTPDemux;

// fab8b184-fc7b-11d8-8829-0002b3658720
DEFINE_GUID(IID_IHXHTTPDemuxResponse, 0xfab8b184, 0xfc7b, 0x11d8,
                0x88, 0x29, 0x00, 0x02, 0xb3, 0x65, 0x87, 0x20);

// 43481048-fc7c-11d8-a5f7-0002b3658720
DEFINE_GUID(IID_IHXHTTPDemux, 0x43481048, 0xfc7c, 0x11d8,
                0xa5, 0xf7, 0x00, 0x02, 0xb3, 0x65, 0x87, 0x20);

#undef  INTERFACE
#define INTERFACE IHXHTTPDemuxResponse
DECLARE_INTERFACE_(IHXHTTPDemuxResponse, IUnknown)
{
    // IUnknown
    STDMETHOD(QueryInterface)   (REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(UINT32,AddRef)   (THIS) PURE;
    STDMETHOD_(UINT32,Release)  (THIS) PURE;

    // IHXHTTPDemuxResponse
    STDMETHOD_(HTTP_TYPE, GetHandlerType)   (THIS) PURE;
    STDMETHOD_(BOOL, AutoDispatch)          (THIS) PURE;
    STDMETHOD_(void, Init)                  (THIS_ IHXHTTPDemux* pDemux) PURE;
    STDMETHOD_(void, OnDispatch)            (THIS) PURE;
    STDMETHOD_(void, OnRequest)             (THIS_ HTTPRequestMessage* pMsg) PURE;
    STDMETHOD_(void, OnResponse)            (THIS_ HTTPResponseMessage* pMsg) PURE;
    STDMETHOD_(void, OnData)                (THIS_ IHXBuffer* pBuf) PURE;
    STDMETHOD_(void, OnWriteReady)          (THIS) PURE;
    STDMETHOD_(void, OnClosed)              (THIS) PURE;
    STDMETHOD_(UINT32, GetFeatureFlags)     (THIS) PURE;
};

#undef  INTERFACE
#define INTERFACE IHXHTTPDemux
DECLARE_INTERFACE_(IHXHTTPDemux, IUnknown)
{
    // IUnknown
    STDMETHOD(QueryInterface)   (REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(UINT32,AddRef)   (THIS) PURE;
    STDMETHOD_(UINT32,Release)  (THIS) PURE;

    // IHXHTTPDemux
    STDMETHOD_(Process*, GetProc)       (THIS) PURE;
    STDMETHOD_(void, SetProc)           (THIS_ Process* proc) PURE;
    STDMETHOD_(CHXServSocket*, GetSock) (THIS) PURE;
    STDMETHOD(CreateClient)             (THIS) PURE;
    STDMETHOD_(Client*, GetClient)      (THIS) PURE;
    STDMETHOD(SendMessage)              (THIS_ HTTPMessage* pMsg) PURE;
    STDMETHOD(SendData)                 (THIS_ IHXBuffer* pBuf) PURE;
    STDMETHOD_(void, Close)             (THIS_ HX_RESULT status) PURE;
};

class CHTTPDemux : public IHXSocketResponse,
                   public IHXHTTPDemux,
                   public HXProtocol
{
public:
    CHTTPDemux(void);
    virtual ~CHTTPDemux(void);

    // IUnknown
    STDMETHOD(QueryInterface)   (REFIID riid, void** ppvObj);
    STDMETHOD_(UINT32,AddRef)   (THIS);
    STDMETHOD_(UINT32,Release)  (THIS);

    // IHXSocketResponse
    STDMETHOD(EventPending)             (UINT32 uEvent, HX_RESULT status);

    // IHXHTTPDemux
    STDMETHOD_(Process*, GetProc)       (THIS);
    STDMETHOD_(void, SetProc)           (THIS_ Process* proc);
    STDMETHOD_(CHXServSocket*, GetSock) (THIS);
    STDMETHOD(CreateClient)             (THIS);
    STDMETHOD_(Client*, GetClient)      (THIS);
    STDMETHOD(SendMessage)              (THIS_ HTTPMessage* pMsg);
    STDMETHOD(SendData)                 (THIS_ IHXBuffer* pBuf);
    STDMETHOD_(void, Close)             (THIS_ HX_RESULT status);

    // HXProtocol
    virtual void                init(Process* proc, IHXSocket* pSock);
    virtual const char*         versionStr(void) { return "HTTP/1.0"; }

private:
    void                        handleInput(IHXBuffer* pBuf);
    void                        DispatchMessage(HTTPMessage* pMsg);
    void                        DetectHandler(HTTPMessage* pMsg);
    bool                        IsWMTHTTP(const char* pUrl, MIMEHeader* pUserAgentHdr);
    bool                        IsASXGen(const char* pUrl);

protected:
    INT32                       m_nRefCount;
    IHXBuffer*                  m_pFragBuf;
    HTTPParser*                 m_pParser;
    UINT32                      m_uContentRemain;
    IHXHTTPDemuxResponse*       m_pResponse;
    HTTPMessage*                m_pSavedMessage;
    DEMUX_READ_STATE            m_ReadState;
    BOOL			m_bClosed;
    UINT32			m_ulMsgLen;
};

#endif  /* _HTTP_DEMUX_H_ */
