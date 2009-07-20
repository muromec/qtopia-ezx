/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxauthn.h,v 1.5 2007/07/06 20:43:41 jfinnecy Exp $
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
 * terms of the GNU General Public License Version 2 (the
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

#ifndef _HXAUTHN_H_
#define _HXAUTHN_H_

/*
 * Forward declarations of some interfaces defined or used here-in.
 */
typedef _INTERFACE  IUnknown			    IUnknown;
typedef _INTERFACE  IHXCredRequest		    IHXCredRequest;
typedef _INTERFACE  IHXCredRequestResponse	    IHXCredRequestResponse;
typedef _INTERFACE  IHXClientAuthConversation	    IHXClientAuthConversation;
typedef _INTERFACE  IHXClientAuthResponse	    IHXClientAuthResponse;
typedef _INTERFACE  IHXServerAuthConversation	    IHXServerAuthConversation;
typedef _INTERFACE  IHXServerAuthResponse	    IHXServerAuthResponse;
typedef _INTERFACE  IHXUserContext		    IHXUserContext;
typedef _INTERFACE  IHXUserProperties		    IHXUserProperties;
typedef _INTERFACE  IHXUserImpersonation	    IHXUserImpersonation;
typedef _INTERFACE  IHXChallenge		    IHXChallenge;
typedef _INTERFACE  IHXChallengeResponse	    IHXChallengeResponse;
typedef _INTERFACE  IHXRequest			    IHXRequest;
typedef _INTERFACE  IHXBuffer			    IHXBuffer;
typedef _INTERFACE  IHXValues			    IHXValues;


/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXCredRequest
 *
 *  Purpose:
 *
 *	This is queried from the response interface passed into 
 *	IHXClientAuthConversation::MakeResponse.  MakeResponse
 *	uses it to request the current user to enter their credentials. 
 *
 *  IHXCredRequest:
 *
 *	{00002801-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXCredRequest,   0x00002801, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXCredRequest

DECLARE_INTERFACE_(IHXCredRequest, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /************************************************************************
     *	Method:
     *	    IHXCredRequest::GetCredentials
     *	Purpose:
     *	    
     *	    Call this to request the credentials.  Usually presents UI to 
     *	    the user asking for username and password.
     *
     *	    While ignored at this time, pValuesCredentialRequest should
     *	    contain CString properties that describe the reason for the 
     *	    request. (like the URL, the Realm, the Auth protocol, and how 
     *	    secure it is, etc..)  In the future this data will be displayed
     *	    to the user.
     *
     */
    STDMETHOD(GetCredentials)
    (
	THIS_
	IHXCredRequestResponse* pCredRequestResponseRequester,
	IHXValues* pValuesCredentialRequest
    ) PURE;

};

/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXCredRequestResponse
 *
 *  Purpose:
 *
 *	This is implemented by a client authenticator in order to receive
 *	the credentials requested in IHXCredRequest::GetCredentials
 *
 *  IHXCredRequestResponse:
 *
 *	{00002800-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXCredRequestResponse,   0x00002800, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXCredRequestResponse

DECLARE_INTERFACE_(IHXCredRequestResponse, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /************************************************************************
     *	Method:
     *	    IHXCredRequestResponse::CredentialsReady
     *	Purpose:
     *	    
     *	    Reports the success or failure of 
     *	    IHXCredRequest::GetCredentials
     *
     *	    If successful pValuesCredentials contains the requested 
     *	    credentials.  (usually CString:Username and CString:Password)
     *
     */
    STDMETHOD(CredentialsReady)
    (
	THIS_
	HX_RESULT	ResultStatus,
	IHXValues* pValuesCredentials
    ) PURE;

};

/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXClientAuthConversation
 *
 *  Purpose:
 *
 *	This is implemented by a client authenticator in order to perform 
 *	the client side of an authentication protocol.	
 *
 *  IHXClientAuthConversation:
 *
 *	{00002803-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXClientAuthConversation,   0x00002803, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

/*
 *  The IHXCommonClassFactory supports creating an instance
 *  of this object.
 */
#define CLSID_CHXClientAuthenticator IID_IHXClientAuthConversation

#undef  INTERFACE
#define INTERFACE   IHXClientAuthConversation

DECLARE_INTERFACE_(IHXClientAuthConversation, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /************************************************************************
     *	Method:
     *	    IHXClientAuthConversation::MakeResponse
     *	Purpose:
     *	    
     *	    Call this when a challenge is received from the server.
     *	    
     *	    pRequestChallengeHeaders should contain the server challenge.
     *
     */
    STDMETHOD(MakeResponse)
    (
	THIS_
	IHXClientAuthResponse* pClientAuthResponseRequester,
	IHXRequest*	pRequestChallengeHeaders
    ) PURE;

    /************************************************************************
     *	Method:
     *	    IHXClientAuthConversation::IsDone
     *	Purpose:
     *	    
     *	    Call this to determine whether the conversation is complete.
     *	    (some protocols have more then one message exchange.)
     *
     */
    STDMETHOD_(HXBOOL,IsDone)(THIS) PURE;

    /************************************************************************
     *	Method:
     *	    IHXClientAuthConversation::Authenticated
     *	Purpose:
     *	    
     *	    Call this to signal the authenticator that the conversation 
     *	    just completed succeeded or failed.
     *
     */
    STDMETHOD(Authenticated)(THIS_ HXBOOL bAuthenticated) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXClientAuthResponse
 *
 *  Purpose:
 *
 *	This is implemented by the client core in order to receive the 
 *	response generated by IHXClientAuthConversation::MakeResponse
 *
 *  IHXClientAuthResponse:
 *
 *	{00002802-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXClientAuthResponse,   0x00002802, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXClientAuthResponse

DECLARE_INTERFACE_(IHXClientAuthResponse, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /************************************************************************
     *	Method:
     *	    IHXClientAuthResponse::ResponseReady
     *	Purpose:
     *	    
     *	    Reports the success or failure of 
     *	    IHXClientAuthConversation::MakeResponse
     *
     *	    pRequestResponseHeaders should be the same Request object 
     *	    that was passed into MakeResponse, it should contain
     *	    CString values for each MimeHeader it wishes to send to 
     *	    the Server.
     *
     */
    STDMETHOD(ResponseReady)
    (
	THIS_
	HX_RESULT	ResultStatus,
	IHXRequest*	pRequestResponseHeaders
    ) PURE;

};


/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXServerAuthConversation
 *
 *  Purpose:
 *
 *	This is implemented by a server authenticator in order to perform 
 *	the server side of an authentication protocol.	
 *
 *  IHXServerAuthConversation:
 *
 *	{00002805-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXServerAuthConversation,   0x00002805, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

/*
 *  The IHXCommonClassFactory supports creating an instance
 *  of this object.
 */
#define CLSID_CHXServerAuthenticator IID_IHXServerAuthResponse

#undef  INTERFACE
#define INTERFACE   IHXServerAuthConversation

DECLARE_INTERFACE_(IHXServerAuthConversation, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /************************************************************************
     *	Method:
     *	    IHXServerAuthConversation::MakeChallenge
     *	Purpose:
     *	    
     *	    Call this to create a challenge for a client.  If the request 
     *	    passed in does not contain a respose from the client, then it 
     *	    will generate the initial challenge.
     *
     *	    pRequestResponseHeaders is the request for a secured URL.  If
     *	    this is the initial request for the URL it probably does not
     *	    have any credentials from the client.
     *
     */
    STDMETHOD(MakeChallenge)
    (
	THIS_
	IHXServerAuthResponse*	pServerAuthResponseRequester,
	IHXRequest*		pRequestResponseHeaders
    ) PURE;

    /************************************************************************
     *	Method:
     *	    IHXServerAuthConversation::IsAuthenticated
     *	Purpose:
     *	    
     *	    Call this to determine whether the last response from the 
     *	    client completed the authentication successfully.
     *
     */
    STDMETHOD_(HXBOOL,IsAuthenticated)(THIS) PURE;

    /************************************************************************
     *	Method:
     *	    IHXServerAuthConversation::GetUserContext
     *	Purpose:
     *	    
     *	    Call this to retrieve the Context of the user that completed
     *	    authentication successfully.
     *
     *	    If successful pUnknownUser is a valid context
     *
     */
    STDMETHOD(GetUserContext)(THIS_ REF(IUnknown*) pUnknownUser) PURE;

};

/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXServerAuthResponse
 *
 *  Purpose:
 *
 *	This is implemented by various server plugins in order to receive the 
 *	challenge generated by IHXServerAuthConversation::MakeChallenge
 *
 *  IHXServerAuthResponse:
 *
 *	{00002804-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXServerAuthResponse,   0x00002804, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXServerAuthResponse

DECLARE_INTERFACE_(IHXServerAuthResponse, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /************************************************************************
     *	Method:
     *	    IHXServerAuthResponse::ChallengeReady
     *	Purpose:
     *	    
     *	    Reports the success or failure of 
     *	    IHXServerAuthConversation::MakeChallenge
     *
     *	    pRequestChallengeHeaders should be the same Request object 
     *	    that was passed into MakeChallenge, it should contain
     *	    CString values for each MimeHeader it wishes to send to 
     *	    the client.
     *
     */
    STDMETHOD(ChallengeReady)
    (
	THIS_
	HX_RESULT	ResultStatus,
	IHXRequest*	pRequestChallengeHeaders
    ) PURE;

};


/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXUserContext
 *
 *  Purpose:
 *
 *	This is implemented by a user context in order to provide 
 *	access to information about the currently authenticated user.
 *
 *  IHXUserContext:
 *
 *	{00002806-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXUserContext,   0x00002806, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXUserContext

DECLARE_INTERFACE_(IHXUserContext, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /************************************************************************
     *	Method:
     *	    IHXUserContext::IsMemberOf
     *	Purpose:
     *	    
     *	    Call this to determine whether the authenticated user
     *	    is a member of the specified group.
     *
     */
    STDMETHOD(IsMemberOf)(THIS_ IHXBuffer* pBufferGroupID) PURE;
};


/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXUserProperties
 *
 *  Purpose:
 *
 *	This is implemented by a user context in order to provide 
 *	access to properties of the currently authenticated user.
 *
 *  IHXUserProperties:
 *
 *	{00002807-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXUserProperties,   0x00002807, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXUserProperties

DECLARE_INTERFACE_(IHXUserProperties, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;


    /************************************************************************
     *	Method:
     *	    IHXUserProperties::GetPrincipalID
     *	Purpose:
     *	    
     *	    Call this to determine the principalID of the authenticated user.
     *
     */
    STDMETHOD(GetPrincipalID)(THIS_ REF(IHXBuffer*) pBufferPrincipalID) PURE;

    /************************************************************************
     *	Method:
     *	    IHXUserProperties::GetAuthorityName
     *	Purpose:
     *	    
     *	    Call this to determine the authority name that authorized the 
     *	    authenticated user. (realm or domain name)
     *
     */
    STDMETHOD(GetAuthorityName)(THIS_ REF(IHXBuffer*) pBufferAuthorityName) PURE;

};


/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXUserImpersonation
 *
 *  Purpose:
 *
 *	This can be implemented by a user context in order to provide 
 *	the ability to have the server impersonate the currently authenticated
 *	user.
 *
 *  IHXUserImpersonation:
 *
 *	{00002808-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXUserImpersonation,   0x00002808, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXUserImpersonation

DECLARE_INTERFACE_(IHXUserImpersonation, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /************************************************************************
     *	Method:
     *	    IHXUserImpersonation::Start
     *	Purpose:
     *	    
     *	    Call this to impersonate the authenticated user.
     *
     */
    STDMETHOD(Start)(THIS) PURE;

    /************************************************************************
     *	Method:
     *	    IHXUserImpersonation::Stop
     *	Purpose:
     *	    
     *	    Call this to stop impersonating the authenticated user.
     *
     */
    STDMETHOD(Stop)(THIS) PURE;

};

/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXChallenge
 *
 *  Purpose:
 *
 *	This is implemented by the server core in order to allow 
 *	additional exchanges of information with the client without
 *	creating a new request. (It is stored in the IHXRequest object
 *	and can be retrieved by calling IHXRequestContext::GetRequester()
 *	if it is absent then the protocol that this request was made on 
 *	does not support multi-message authentication (PNA doesn't) )
 *
 *  IHXChallenge:
 *
 *	{0000280A-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXChallenge,   0x0000280A, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXChallenge

DECLARE_INTERFACE_(IHXChallenge, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /************************************************************************
     *	Method:
     *	    IHXChallenge::SendChallenge
     *	Purpose:
     *	    
     *	    Call this to request additional information from the client.
     *
     *	    pRequestChallenge should be the same Request object 
     *	    that was passed into MakeChallenge, it should contain
     *	    CString values for each MimeHeader it wishes to send to 
     *	    the client.
     *
     */
    STDMETHOD(SendChallenge)
    (
	THIS_
	IHXChallengeResponse* pChallengeResponseSender,
	IHXRequest* pRequestChallenge
    ) PURE;

};

/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXChallengeResponse
 *
 *  Purpose:
 *
 *	This is implemented by a server authenticator in order to 
 *	receive the Response returned by the client in response to 
 *	IHXChallenge::SendChallenge.
 *
 *  IHXChallengeResponse:
 *
 *	{00002809-0901-11d1-8B06-00A024406D59}
 *
 */
DEFINE_GUID(IID_IHXChallengeResponse,   0x00002809, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXChallengeResponse

DECLARE_INTERFACE_(IHXChallengeResponse, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /************************************************************************
     *	Method:
     *	    IHXChallengeResponse::ResponseReady
     *	Purpose:
     *	    
     *	    Called this to return the additional information requested 
     *	    from IHXChallenge::SendChallenge.
     *
     *	    pRequestResponse should be the same Request object 
     *	    that was passed into MakeChallenge and SendChallenge.
     *
     */
    STDMETHOD(ResponseReady)
    (
	THIS_
	IHXRequest* pRequestResponse
    ) PURE;

};

#endif //!_HXAUTHN_H_
