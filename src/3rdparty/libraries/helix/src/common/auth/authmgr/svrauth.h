/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: svrauth.h,v 1.5 2005/03/14 19:30:05 bobclark Exp $
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

#ifndef __CServerAuthenticator__
#define __CServerAuthenticator__

#include "hxplgns.h"

#include "unkimp.h"
#include "smartptr.h"
#include "hxpktsp.h"
#include "hxfilsp.h"
#include "hxmonsp.h"
#include "hxathsp.h"
#include "hxplnsp.h"
#include "hxdbsp.h"
#include "miscsp.h"

#include "cliauth.h"

class CServerAuthenticator
    : public CUnknownIMP
    , public IHXObjectConfiguration
    , public IHXServerAuthConversation
    , public IHXServerAuthResponse
    , public IHXAuthenticationDBAccess
    , public IHXAuthenticationDBManager
    , public IHXAuthenticationDBManagerResponse
{
    DECLARE_UNKNOWN(CServerAuthenticator)

public:
    CServerAuthenticator();
    virtual ~CServerAuthenticator();

    STDMETHOD(SetContext)(IUnknown* pIUnknownContext);
    STDMETHOD(SetConfiguration)
    (
	IHXValues* pIHXValuesConfiguration
    );

    // IHXServerAuthConversation
    STDMETHOD(MakeChallenge)
    (
	IHXServerAuthResponse* pIHXServerAuthResponseRequester,
	IHXRequest* pIHXRequestResponse
    );
    STDMETHOD_(HXBOOL,IsAuthenticated)();
    STDMETHOD(GetUserContext)(REF(IUnknown*) pIUnknownUser);

    // IHXServerAuthResponse
    STDMETHOD(ChallengeReady)
    (
	HX_RESULT   HX_RESULTStatus,
	IHXRequest* pIHXRequestChallenge
    );

    /************************************************************************
     *	Method:
     *	    IHXAuthenticationDBAccess::_NewEnum
     *	Purpose:
     *	    
     *	    Call this to make a new enumerator of this collection.
     *
     */
    STDMETHOD(_NewEnum)
    (
	REF(IHXAsyncEnumAuthenticationDB*) pAsyncEnumAuthenticationDBNew
    );

    /************************************************************************
     *	Method:
     *	    IHXAuthenticationDBAccess::CheckExistence
     *	Purpose:
     *	    
     *	    Call this to verify the existance of a principal.
     *
     */
    STDMETHOD(CheckExistence)
    (
	IHXAuthenticationDBAccessResponse* pAuthenticationDBAccessResponseNew,
	IHXBuffer*		pBufferPrincipalID
    );

    /************************************************************************
     *	Method:
     *	    IHXAuthenticationDBAccess::GetCredentials
     *	Purpose:
     *	    
     *	    Call this to access the credentials for the specified principal.
     *
     */
    STDMETHOD(GetCredentials)
    (
	IHXAuthenticationDBAccessResponse* pAuthenticationDBAccessResponseNew,
	IHXBuffer*		pBufferPrincipalID
    );

    // IHXAuthenticationDBManager
    STDMETHOD(AddPrincipal)
    (
	IHXAuthenticationDBManagerResponse* pAuthenticationDBManagerResponseNew,
	IHXBuffer*		pBufferPrincipalID
    );

    STDMETHOD(RemovePrincipal)
    (
	IHXAuthenticationDBManagerResponse* pAuthenticationDBManagerResponseNew,
	IHXBuffer*		pBufferPrincipalID
    );

    STDMETHOD(SetCredentials)
    (
	IHXAuthenticationDBManagerResponse* pAuthenticationDBManagerResponseNew,
	IHXBuffer*		pBufferPrincipalID, 
	IHXBuffer*		pBufferCredentials
    );

    // IHXAuthenticationDBManagerResponse
    STDMETHOD(AddPrincipalDone)
    (
	HX_RESULT		ResultStatus,
	IHXBuffer*		pBufferPrincipalID
    );

    STDMETHOD(RemovePrincipalDone)
    (
	HX_RESULT		ResultStatus,
	IHXBuffer*		pBufferPrincipalID
    );

    STDMETHOD(SetCredentialsDone)
    (
	HX_RESULT		ResultStatus,
	IHXBuffer*		pBufferPrincipalID
    );

private:
    HX_RESULT _TryToLoadPlugins();
    HX_RESULT _GetRealmSettings
    (
	char*	    pcharRealm,
	IHXValues**    ppIHXValuesRealmProperties
    );
    HX_RESULT _GetFirstPlugin
    (
	IHXValues*	pIHXValuesPluginList,
	IHXBuffer**    ppIHXBufferPluginName, 
	IHXValues**	ppIHXValuesPluginProperties
    );
    HX_RESULT _GetNextPlugin
    (
	IHXValues*	pIHXValuesPluginList,
	IHXBuffer**    ppIHXBufferPluginName, 
	IHXValues**	ppIHXValuesPluginProperties
    );
    HX_RESULT _GetPluginDataFromID
    (
	const char*	pcharPropertyPath,
	UINT32		ulProperty_id,
	const char*	pcharRealPropertyNameLoc,
	IHXBuffer**	ppIHXBufferPluginName, 
	IHXValues**	ppIHXValuesPluginProperties
    );
    HX_RESULT _CreatePlugin
    (
	IHXBuffer* pIHXBufferPluginID,
	IHXValues* pIHXValuesPluginProperties,
	IUnknown** ppIUnknownPlugin
    );
    HX_RESULT _ValuesFromHXRegList
    (
	IHXValues* pIHXValuesPNRegList,
	IHXValues** ppIHXValuesValues
    );

    HX_RESULT m_HX_RESULTStatus;

    DECLARE_SMART_POINTER_UNKNOWN m_spIUnknownContext;
    DECLARE_SMART_POINTER(IHXValues) m_spIHXValuesConfig;
    DECLARE_SMART_POINTER(IHXRequest) m_spIHXRequestResponse;
    DECLARE_SMART_POINTER(IHXValues) m_spIHXValuesChallengeHeaders;
    DECLARE_SMART_POINTER(IHXBuffer) m_spBufferCredentials;
    _CListOfWrapped_IUnknown_ m_ListOfIUnknownPlugins;
    _CListOfWrapped_IUnknown_::iterator m_ListOfIUnknownIteratorCurrent;
    DECLARE_SMART_POINTER(IHXServerAuthResponse) 
        m_spIHXServerAuthResponseRequester;
    DECLARE_SMART_POINTER(IHXAuthenticationDBManagerResponse)
        m_spAuthenticationDBManagerResponseRequester;

    IHXCommonClassFactory* m_pClassFactory;
};


#endif // !__CServerAuthenticator__
