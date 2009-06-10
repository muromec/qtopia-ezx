/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: rn5auth.cpp, 2004/27/07 
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
#ifndef __CRN5Authenticator__
#define __CRN5Authenticator__

class CRN5Authenticator
    : public CHashAuthenticatorBase
    , public IHXPlugin
    , public IHXPluginProperties
    , public IHXClientAuthConversation
    , public IHXCredRequestResponse
    , public IHXServerAuthConversation
    , public IHXAuthenticationDBAccessResponse
{
public:
    CRN5Authenticator();
    ~CRN5Authenticator();

/*** IUnknown methods ***/
	STDMETHOD(QueryInterface)	(THIS_
				    	REFIID riid,
				    	void** ppvObj);

	STDMETHOD_(ULONG32,AddRef)	(THIS);

	STDMETHOD_(ULONG32,Release)	(THIS);

    /*** IHXPlugin methods ***/

    /************************************************************************
     *	Method:
     *	    IHXPlugin::GetPluginInfo
     *	Purpose:
     *	    Returns the RN5 information about this plugin. Including:
     *
     *	    bLoadMultiple	whether or not this plugin DLL can be loaded
     *				multiple times. All File Formats must set
     *				this value to TRUE.
     *	    pDescription	which is used in about UIs (can be NULL)
     *	    pCopyright		which is used in about UIs (can be NULL)
     *	    pMoreInfoURL	which is used in about UIs (can be NULL)
     */
    STDMETHOD(GetPluginInfo)	(THIS_
				REF(HXBOOL)        /*OUT*/ bLoadMultiple,
				REF(const char*) /*OUT*/ pDescription,
				REF(const char*) /*OUT*/ pCopyright,
				REF(const char*) /*OUT*/ pMoreInfoURL,
				REF(ULONG32)	 /*OUT*/ ulVersionNumber
				);

    /************************************************************************
     *	Method:
     *	    IHXPlugin::InitPlugin
     *	Purpose:
     *	    Initializes the plugin for use. This interface must always be
     *	    called before any other method is called. This is primarily needed 
     *	    so that the plugin can have access to the context for creation of
     *	    IHXBuffers and IMalloc.
     */
    STDMETHOD(InitPlugin)   (THIS_
			    IUnknown*   /*IN*/  pContext);


    // IHXPluginProperties
    STDMETHOD(GetProperties)(REF(IHXValues*) pIHXValuesOptions);

    // IHXClientAuthConversation
    STDMETHOD(MakeResponse)
    (
	IHXClientAuthResponse* pIHXClientAuthResponseRequester,
	IHXRequest* pIHXRequestChallenge
    );
    STDMETHOD_(HXBOOL,IsDone)();
    STDMETHOD(Authenticated)(HXBOOL bAuthenticated);

    // IHXServerAuthConversation
    STDMETHOD(MakeChallenge)
    (
	IHXServerAuthResponse* pIHXServerAuthResponseRequester,
	IHXRequest* pIHXRequestResponse
    );
    STDMETHOD_(HXBOOL,IsAuthenticated)();
    STDMETHOD(GetUserContext)(REF(IUnknown*) pIUnknownUser);

    // IHXCredRequestResponse
    STDMETHOD(CredentialsReady)
    (
	HX_RESULT	HX_RESULTStatus,
	IHXValues* pIHXValuesCredentials
    );

    // IHXAuthenticationDBAccessResponse
    STDMETHOD(ExistenceCheckDone)
    (
	HX_RESULT		ResultStatus,
	IHXBuffer*		pBufferPrincipalID
    );

    STDMETHOD(GetCredentialsDone)
    (
	HX_RESULT		ResultStatus,
	IHXBuffer*		pBufferPrincipalID,
	IHXBuffer*		pBufferCredentials
    );

    static HX_RESULT STDAPICALLTYPE HXCreateInstance(IUnknown** ppIUnknown);

private:
    HX_RESULT _DescribeCredentials
    (
	IHXValues* pIHXValuesChallengeHeaders,
	IHXValues** ppIHXValuesParameterDescriptions
    );

    HX_RESULT _ChallengeToCredentials
    (
	IHXValues* pIHXValuesChallengeHeaders, 
	IHXValues** ppIHXValuesCredentials
    );

    HX_RESULT
    _MungeUserRealmPassFromValues
    (
	IHXValues* pCredentials,
	IHXBuffer** ppStorageKey
    );

    HX_RESULT
    _HeaderToCredentials
    (
	IHXBuffer* pHeader,
	IHXValues** ppCredentials
    );

    HX_RESULT _CreateQuotedHeader
    ( 
	IHXValues* pIHXValuesCredentials,
	IHXBuffer* pIHXBufferStoredPassword,
	IHXValues** ppIHXValuesResponseHeaders
    );

    HX_RESULT
    _StorageToToken
    (
	IHXValues* pCredentials,
	IHXBuffer* pStoredPassword,
	IHXBuffer** ppToken
    );

    HXBOOL 
    _GetQuotedValue
    (
	const char*& instr, 
	char* valname, 
	char* valbuf
    );
    
    HX_RESULT 
    _SendChallengeResponse();
    
    // Client Members
    IHXRequest*			 m_pClientRequest;
    IHXClientAuthResponse*		 m_pClientRespondee;
    HXBOOL				 m_bFinished;

    // Plugin Members
    IHXRequestContext*			 m_pRequestContext;

    static const char*			 zm_pDescription;
    static const char*			 zm_pCopyright;
    static const char*			 zm_pMoreInfoURL;
};


#endif // !__CRN5Authenticator__
