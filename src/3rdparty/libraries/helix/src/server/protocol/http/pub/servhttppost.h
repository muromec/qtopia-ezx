/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: servhttppost.h,v 1.1 2004/11/16 03:02:43 tmarshall Exp $
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

#ifndef _SERVHTTPPOST_H_
#define _SERVHTTPPOST_H_

#include "unkimp.h"
#include "smartptr.h"
#include "hxathsp.h"
#include "hxcomsp.h"
#include "hxplnsp.h"
#include "hxpktsp.h"
#include "hxtset.h"
#include "miscsp.h"
#include "httppars.h"
#include "httpmsg.h"


class CHXString;
struct IHXRegistry;
struct IHXFileResponse;

class HTTPPostTCPResponse;
class HTTPPostObjCallback;


/////////////////////////////////////////////////////////////////////////////
//
//  Class:
//
//      CHXHTTPPostObject
//
//  Purpose:
//
//      Example implementation of a basic file system file object.
//

class CHXHTTPPostObject : public IHXHTTPPostObject,
                          public IHXRequestHandler,
                          public IHXTimeoutSettings
{
//    DECLARE_UNKNOWN(CHXHTTPPostObject)

public:
    CHXHTTPPostObject();
    ~CHXHTTPPostObject();

    INT32 m_lCount;

    static CHXHTTPPostObject* CreateObject();
    STDMETHOD(QueryInterface)   (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef) (THIS);
    STDMETHOD_(ULONG32,Release) (THIS);

    void InitObject(IUnknown* pContext);

    HX_RESULT _ReOpen();

    /************************************************************************
     *  Method:
     *      Private interface::_OpenFile
     *  Purpose:
     *      This common method is used from Init() and GetFileObjectFromPool()
     */
    HX_RESULT _OpenFile( const char* pFilename,
                        ULONG32     ulFlags);
    HX_RESULT ProcessIdle(void);

    /*
     *  IHXTCPResponse methods (even though we do not implement this interface!)
     */

    STDMETHOD(ConnectDone)      (THIS_
                                 HX_RESULT              status);

    STDMETHOD(ReadDone)         (THIS_
                                HX_RESULT               status,
                                IHXBuffer*              pBuffer);

    STDMETHOD(WriteReady)       (THIS_
                                HX_RESULT               status);

    STDMETHOD(Closed)           (THIS_
                                HX_RESULT               status);

    /*
     *  IHXHTTPPostObject methods
     */

    STDMETHOD(Init)     (THIS_
                        ULONG32             /*IN*/  ulFlags,
                        IHXHTTPPostResponse*   /*IN*/  pFileResponse);

    STDMETHOD(Close)    (THIS);


    STDMETHOD(Post)     (THIS_
                        IHXBuffer* pBuffer);

    STDMETHOD(SetSize)          (THIS_
                                ULONG32         ulLength);

    STDMETHOD(GetResponse)      (THIS);

    /*
     * IHXRequestHandler methods
     */
    /************************************************************************
     *  Method:
     *      IHXRequestHandler::SetRequest
     *  Purpose:
     *      Associates an IHXRequest with an object
     */
    STDMETHOD(SetRequest)       (THIS_
                                IHXRequest*        /*IN*/  pRequest);

    /************************************************************************
     *  Method:
     *      IHXRequestHandler::GetRequest
     *  Purpose:
     *      Gets the IHXRequest object associated with an object
     */
    STDMETHOD(GetRequest)       (THIS_
                                REF(IHXRequest*)  /*OUT*/  pRequest);



    /**************************
        IHXTimeoutSettings
    ***************************/


    /************************************************************************
     *  Method:
     *      IHXTimeSettings::Get/SetConnnectionTimeout
     *  Purpose:
     *      Get/Set the connection timeout setting, in seconds
     */
    STDMETHOD(GetConnectionTimeout) (THIS_
                                      REF(UINT32)   /*OUT*/ nSeconds);
    STDMETHOD(SetConnectionTimeout) (THIS_
                                      UINT32        /*OUT*/ nSeconds);

    /************************************************************************
     *  Method:
     *      IHXTimeSettings::Get/SetServerTimeout
     *  Purpose:
     *      Get/Set the server timeout setting, in seconds
     */
    STDMETHOD(GetServerTimeout) (THIS_
                                      REF(UINT32)   /*OUT*/ nSeconds);
    STDMETHOD(SetServerTimeout) (THIS_
                                      UINT32        /*OUT*/ nSeconds);
private:

    HX_RESULT       BeginPost(void);

    //HX_RESULT     _SendAuthentication(IHXValues* pValues);

    HX_RESULT       _HandleSuccess(HTTPResponseMessage* pMessage, UINT32 ulHeaderLength);
    HX_RESULT       _HandleFail(UINT32 ulHTTPError);
    HX_RESULT       _HandleRedirect(HTTPResponseMessage* pMessage);

    void            CallPostDone(HX_RESULT status);
    void            CallResponseReady(HX_RESULT status, IHXBuffer* pContentBuffer);

    friend class HTTPPostObjCallback;
    friend class HTTPPostTCPResponse;

    HX_RESULT                   m_LastError;

    IHXCommonClassFactory*      m_pCommonClassFactory;
    IHXPreferences*             m_pPreferences;
    IHXScheduler*               m_pScheduler;
    IHXRegistry*                m_pRegistry;
    IHXErrorMessages*           m_pErrorMessages;
    IUnknown*                   m_pContext;

    IHXInterruptState*          m_pInterruptState;

#ifdef _MACINTOSH
    IHXBuffer*                  m_pReadDoneBuffer;
    HX_RESULT                   m_uReadDoneStatus;
    HX_BITFIELD                 m_bReadDoneToBeProcessed : 1;
#endif

    HX_BITFIELD                 m_bInitResponsePending : 1;
    HX_BITFIELD                 m_bTCPReadDonePending : 1;
    HX_BITFIELD                 m_bOverrideContentType : 1;
    HX_BITFIELD                 m_bOverrideLanguage : 1;
    HX_BITFIELD                 m_bInitPending : 1;
    HX_BITFIELD                 m_bInitialized : 1;
    HX_BITFIELD                 m_bInDestructor : 1;
    HX_BITFIELD                 m_bSocketReadTimeout : 1;
    HX_BITFIELD                 m_bDisableConnectionTimeOut : 1;
    HX_BITFIELD                 m_bKeepAlive : 1;
    HX_BITFIELD                 m_bConnectDone : 1;
    HX_BITFIELD                 m_bWriteDone : 1;
    HX_BITFIELD                 m_bPostDonePending : 1;
    HX_BITFIELD                 m_bReadHeaderDone : 1;
    HX_BITFIELD                 m_bReadContentsDone : 1;
    HX_BITFIELD                 m_bKnowContentSize : 1;
    HX_BITFIELD                 m_bResponseReadyPending : 1;
    HX_BITFIELD                 m_bInReadDone : 1;
    HX_BITFIELD                 m_bInPostDone : 1;
    HX_BITFIELD                 m_bInResponseReady : 1;
    HX_BITFIELD                 m_bPostResponseReady : 1;
    HX_BITFIELD                 m_bOpenFilePending : 1;

    IHXHTTPPostResponse*        m_pFileResponse;

    HTTPPostTCPResponse*        m_pTCPResponse;


    char*                       m_szBaseURL;
    char*                       m_pFilename;
    char*                       m_pPath;
    char*                       m_pHost;
    ULONG32                     m_ulFlags;
    IHXRequest*         m_pRequest;

    IHXValues*                  m_pRequestHeadersOrig;

    IHXBuffer*                  m_pLanguage; // suggested content lang

    HTTPPostObjCallback*        m_pCallback;

    UINT16                      m_uDataLength;
    char*                       m_pData;

    ULONG32                     m_ulCurrentWritePosition;

    int                         m_nPort;

    UINT32                      m_nRequestTime; // for time-outs waiting for a read to complete

    UINT32                      m_nConnTimeout;
    UINT32                      m_nServerTimeout;

    IHXTCPSocket*               m_pSocket;

    ULONG32                     m_nContentSize;
    ULONG32                     m_nContentRead;
    CHXString                   m_strResultContent;

    ULONG32                     m_nContentLength;
    UINT16                      m_nTotalRequestSize : 16;
    UINT16                      m_nRequestWritten : 16;

    ULONG32                     m_nHeaderRead;

    CHXString                   m_strHost;
    CHXString                   m_strRequest;
    CHXString                   m_strResource;
    CHXString                   m_strMimeType;
    CHXString                   m_strResultHeader;

    IHXBuffer*                  m_pContentBuffer;

    UINT16                      m_uMaxRecursionLevel : 16;
    UINT16                      m_uRecursionCount : 16;
};


class HTTPPostObjCallback : public CUnknownIMP,
                            public IHXCallback
{
    DECLARE_UNKNOWN(HTTPPostObjCallback)
public:
    HTTPPostObjCallback();

    void InitObject(CHXHTTPPostObject* pHTTPPostObject);

    /*
     *  IHXCallback methods
     */
    STDMETHOD(Func) (THIS);

private:
    ~HTTPPostObjCallback();


    friend class CHXHTTPPostObject;

    CHXHTTPPostObject*      m_pHTTPPostObject;
    BOOL                    m_bCallbackPending;
    ULONG32                 m_ulPendingCallbackID;
};

class HTTPPostTCPResponse : public CUnknownIMP,
                            public IHXTCPResponse
{
    DECLARE_UNKNOWN(HTTPPostTCPResponse)
public:

    HTTPPostTCPResponse();
    ~HTTPPostTCPResponse();

    void InitObject(CHXHTTPPostObject* pOwner);

    /*
     *  IHXTCPResponse methods
     */

    STDMETHOD(ConnectDone)      (THIS_
                                 HX_RESULT              status);

    STDMETHOD(ReadDone)         (THIS_
                                HX_RESULT               status,
                                IHXBuffer*              pBuffer);

    STDMETHOD(WriteReady)       (THIS_
                                HX_RESULT               status);

    STDMETHOD(Closed)           (THIS_
                                HX_RESULT               status);
protected:
    CHXHTTPPostObject*          m_pOwner;

    friend class CHXHTTPPostObject;
};

#endif // _SERVHTTPPOST_H_
