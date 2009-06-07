/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: httpprot.h,v 1.19 2006/07/21 23:42:19 jrmoore Exp $
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

#ifndef _HTTPPROT_H_
#define _HTTPPROT_H_

#include "client.h"
#include "player.h"
#include "hxprot.h"
#include "hxstring.h"
#include "hxslist.h"
#include "httpmsg.h"
#include "qos_tranadapt_info.h"

class HTTPParser;
class CByteQueue;
class BWCalculator;
class BaseCallback;

class HTTPProtocol : public IHXHTTPDemuxResponse,
                     public IHXClientProfileManagerResponse
{
public:
    HTTPProtocol(void);
    virtual ~HTTPProtocol(void);

    // IUnknown
    STDMETHOD(QueryInterface)   (REFIID riid, void** ppvObj);
    STDMETHOD_(UINT32,AddRef)   (THIS);
    STDMETHOD_(UINT32,Release)  (THIS);

    // IHXHTTPDemuxResponse
    STDMETHOD_(HTTP_TYPE, GetHandlerType)   (THIS);
    STDMETHOD_(BOOL, AutoDispatch)          (THIS);
    STDMETHOD_(void, Init)                  (THIS_ IHXHTTPDemux* pDemux);
    STDMETHOD_(void, OnDispatch)            (THIS);
    STDMETHOD_(void, OnRequest)             (THIS_ HTTPRequestMessage* pMsg);
    STDMETHOD_(void, OnResponse)            (THIS_ HTTPResponseMessage* pMsg);
    STDMETHOD_(void, OnData)                (THIS_ IHXBuffer* pBuf);
    STDMETHOD_(void, OnWriteReady)          (THIS);
    STDMETHOD_(void, OnClosed)              (THIS);
    STDMETHOD_(UINT32, GetFeatureFlags)     (THIS);


    // IHXClientProfileManagerResponse methods
    STDMETHOD(PSSProfileReady)          (THIS_ HX_RESULT ulStatus,
                                            IHXClientProfile* pInfo,
                                            IHXBuffer* pRequestId,
                                            IHXBuffer* pRequestURI,
                                            IHXValues* pRequestHeaders);

    BOOL                    statComplete(HX_RESULT status,
                                         UINT32 ulSize,
                                         UINT32 ulCreationTime,
                                         const char* mimetype);
    void                    handleRedirect(IHXBuffer* pURL);
    void                    handleRead(BYTE* pData, UINT32 uDataLen, int done);
    HTTPResponseMessage*    makeResponseMessage(const char* pErrNo);
    HX_RESULT               sendResponse(HTTPResponseMessage* pMsg);
    void                    Shutdown(HX_RESULT status);

    int                     sendAlert(const char* pSessionID, StreamError err);
    int                     sendAlert(const char* pSessionID, IHXBuffer* pAlert);

    void                    SetStatus(UINT32 ulCode);

private:
    class ShutdownCallback : public BaseCallback
    {
    public:
	ShutdownCallback(HTTPProtocol* pHTTPP)
	    : m_pHTTPP(pHTTPP)
	{
	    m_pHTTPP->AddRef();
	}
	~ShutdownCallback();
	STDMETHOD(Func) (THIS);

	HTTPProtocol* m_pHTTPP;
    };

    void                    sendRequest(HTTPRequestMessage* pMsg);
    void                    sendData(IHXBuffer* pBuf);
    int                     init_request(HTTPRequestMessage* pMsg);
    int                     handleMsg(HTTPGetMessage* pMsg);
    int                     handleMsg(HTTPPostMessage* pMsg);
    int                     handleMsg(HTTPHeadMessage* pMsg);
    const char*             getErrorText(const char* pErrNo);

    BOOL                    getMimeType(const char* extension,
                                        const char*& mime_type);
    int                     post(IHXBuffer* pBuf);
    void                    getRFC822Headers(HTTPMessage* pMsg,
                                             IHXValues* pRFC822Headers);

    void                    init_stats();
    void                    init_registry();
    void                    SetMimeTypeInRegistry(const char* pMimeType);
    void                    SetStatusInRegistry(const INT32 nStatus);

    HX_RESULT               GetClientProfile();

    INT32                   m_nRefCount;
    IHXHTTPDemux*           m_pDemux;
    IHXBuffer*              m_pPendingBuf;
    HTTP*                   m_pHTTP;
    CHXString               m_fileExt;
    UINT32                  m_major_version;
    UINT32                  m_minor_version;
    BOOL                    m_head_request;
    BOOL                    m_posting;
    BOOL                    m_bKeepAlive;
    IHXRequest*             m_pRequest;
    BWCalculator*           m_pbwcBandwidthReporter;
    char*                   m_pRegistryKey;
    Dict*                   mime_type_dict;
    UINT32                  m_max_post_data;
    BOOL                    m_need_posthandler;
    UINT32                  m_content_len;
    HTTPQoSTranAdaptInfo*   m_pQoSInfo;
    BOOL                    m_bNeedCountDecrement;
};

#endif  /* _HTTPPROT_H_ */
