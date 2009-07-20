/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: server_request.h,v 1.4 2003/09/04 22:39:09 dcollins Exp $ 
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

#ifndef _SERVER_REQUEST_H_
#define _SERVER_REQUEST_H_

#include "hxtypes.h"
#include "hxcom.h"
#include "hxfiles.h"

typedef enum
{
    FS_HEADERS,
    FF_HEADERS
} REQUEST_HEADER_TYPE;

class ServerRequest
{
public:
    ServerRequest();
    virtual ~ServerRequest();

    /*
     * The IHXRequest interface is implemented by the ServerRequestWrapper
     */

    STDMETHOD_(ULONG32,AddRef)		(THIS);

    STDMETHOD_(ULONG32,Release)		(THIS);

    STDMETHOD(SetRequestHeaders)	(THIS_
					REQUEST_HEADER_TYPE HeaderType,
					IHXValues* pRequestHeaders);
    
    STDMETHOD(GetRequestHeaders)	(THIS_
					REQUEST_HEADER_TYPE HeaderType,
					REF(IHXValues*) pRequestHeaders);

    STDMETHOD(SetResponseHeaders)	(THIS_
					REQUEST_HEADER_TYPE HeaderType,
					IHXValues* pResponseHeaders);
    
    STDMETHOD(GetResponseHeaders)	(THIS_
					REQUEST_HEADER_TYPE HeaderType,
					REF(IHXValues*) pResponseHeaders);

    STDMETHOD(SetURL)			(THIS_
					const char* pURL);

    STDMETHOD(GetURL)			(THIS_
					REF(const char*) pURL);

    /************************************************************************
     *	Method:
     *	    IHXRequest::SetUserContext
     *	Purpose:
     *	    Sets the Authenticated users Context.
     */
    STDMETHOD(SetUserContext)
    (
	IUnknown* pIUnknownNewContext
    );

    /************************************************************************
     *	Method:
     *	    IHXRequest::GetUserContext
     *	Purpose:
     *	    Gets the Authenticated users Context.
     */
    STDMETHOD(GetUserContext)
    (
	REF(IUnknown*) pIUnknownCurrentContext
    );

    /************************************************************************
     *	Method:
     *	    IHXRequest::SetRequester
     *	Purpose:
     *	    Sets the Object that made the request.
     */
    STDMETHOD(SetRequester)
    (
	IUnknown* pIUnknownNewRequester
    );

    /************************************************************************
     *	Method:
     *	    IHXRequest::GetRequester
     *	Purpose:
     *	    Gets the Object that made the request.
     */
    STDMETHOD(GetRequester)
    (
	REF(IUnknown*) pIUnknownCurrentRequester
    );

private:
    IHXValues*	_GetRequestHeaders	(REQUEST_HEADER_TYPE HeaderType);
    IHXValues* _GetResponseHeaders	(REQUEST_HEADER_TYPE HeaderType);
    LONG32			m_lRefCount;
    char*			m_pURL;
    IHXValues*			m_pFSRequestHeaders;
    IHXValues*			m_pFSResponseHeaders;
    IHXValues*			m_pFFRequestHeaders;
    IHXValues*			m_pFFResponseHeaders;
    IUnknown*			m_pIUnknownUserContext;
    IUnknown*			m_pIUnknownRequester;
};

class ServerRequestWrapper 
    : public IHXRequest
    , public IHXRequestContext
{
public:
    ServerRequestWrapper(REQUEST_HEADER_TYPE pHeaderType,
                         ServerRequest *pServerRequest);
    ~ServerRequestWrapper();

    HX_RESULT RemoveRequest(void);
    HX_RESULT ReplaceRequest(ServerRequest* pRequest);

    STDMETHOD(QueryInterface)		(THIS_
					REFIID riid,
					void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)		(THIS);

    STDMETHOD_(ULONG32,Release)		(THIS);

    STDMETHOD(SetRequestHeaders)	(THIS_
					IHXValues* pRequestHeaders);
    
    STDMETHOD(GetRequestHeaders)	(THIS_
					REF(IHXValues*) pRequestHeaders);

    STDMETHOD(SetResponseHeaders)	(THIS_
					IHXValues* pResponseHeaders);
    
    STDMETHOD(GetResponseHeaders)	(THIS_
					REF(IHXValues*) pResponseHeaders);

    STDMETHOD(SetURL)			(THIS_
					const char* pURL);

    STDMETHOD(GetURL)			(THIS_
					REF(const char*) pURL);

    /************************************************************************
     *	Method:
     *	    IHXRequest::SetUserContext
     *	Purpose:
     *	    Sets the Authenticated users Context.
     */
    STDMETHOD(SetUserContext)
    (
	IUnknown* pIUnknownNewContext
    );

    /************************************************************************
     *	Method:
     *	    IHXRequest::GetUserContext
     *	Purpose:
     *	    Gets the Authenticated users Context.
     */
    STDMETHOD(GetUserContext)
    (
	REF(IUnknown*) pIUnknownCurrentContext
    );

    /************************************************************************
     *	Method:
     *	    IHXRequest::SetRequester
     *	Purpose:
     *	    Sets the Object that made the request.
     */
    STDMETHOD(SetRequester)
    (
	IUnknown* pIUnknownNewRequester
    );

    /************************************************************************
     *	Method:
     *	    IHXRequest::GetRequester
     *	Purpose:
     *	    Gets the Object that made the request.
     */
    STDMETHOD(GetRequester)
    (
	REF(IUnknown*) pIUnknownCurrentRequester
    );

private:
    LONG32			m_lRefCount;
    REQUEST_HEADER_TYPE		m_HeaderType;
    ServerRequest*		m_pServerRequest;
};

inline IHXValues*
ServerRequest::_GetRequestHeaders(REQUEST_HEADER_TYPE HeaderType)
{
    return (HeaderType == FS_HEADERS) ? m_pFSRequestHeaders :
                                        m_pFFRequestHeaders;
}

inline IHXValues*
ServerRequest::_GetResponseHeaders(REQUEST_HEADER_TYPE HeaderType)
{
    return (HeaderType == FS_HEADERS) ? m_pFSResponseHeaders :
					m_pFFResponseHeaders;
}



#endif // _SERVER_REQUEST_H_
