/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hashauthbase.cpp, 2004/27/07 
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

#ifndef _HASHAUTHBASE_H_
#define _HASHAUTHBASE_H_

class CHashAuthenticatorBase : public IHXObjectConfiguration
                             , public IHXUserProperties
                             , public IHXAuthenticationDBManager
                             , public IHXAuthenticationDBManagerResponse
                             , public IHXAuthenticationDBAccess
{
public:
    CHashAuthenticatorBase();
    virtual ~CHashAuthenticatorBase();

    /*** IHXUnknown methods ***/
	STDMETHOD(QueryInterface)	(THIS_ REFIID riid,	void** ppvObj);
    STDMETHOD_(UINT32,AddRef)  (THIS) PURE;
    STDMETHOD_(UINT32,Release) (THIS) PURE;

  // IHXObjectConfiguration
    STDMETHOD(SetContext)(IUnknown* pIUnknownContext);
    STDMETHOD(SetConfiguration)(IHXValues* pConfiguration);

  // IHXUserProperties
    STDMETHOD(GetPrincipalID)(REF(IHXBuffer*) pbufPrincipalID);
    STDMETHOD(GetAuthorityName)(REF(IHXBuffer*) pbufAuthorityName);

    /************************************************************************
     *	Method:
     *	    IHXAuthenticationDBAccess::_NewEnum
     *	Purpose:
     *	    
     *	    Call this to make a new enumerator of this collection.
     *
     */
    STDMETHOD(_NewEnum)(REF(IHXAsyncEnumAuthenticationDB*) pEnumDBNew);

    /************************************************************************
     *	Method:
     *	    IHXAuthenticationDBAccess::CheckExistence
     *	Purpose:
     *	    
     *	    Call this to verify the existance of a principal.
     *
     */
    STDMETHOD(CheckExistence)(IHXAuthenticationDBAccessResponse* pDBAccessResp,
	    IHXBuffer*		pBufferPrincipalID);

    /************************************************************************
     *	Method:
     *	    IHXAuthenticationDBAccess::GetCredentials
     *	Purpose:
     *	    
     *	    Call this to access the credentials for the specified principal.
     *
     */
    STDMETHOD(GetCredentials)(IHXAuthenticationDBAccessResponse* pDBAccessResp,
	    IHXBuffer*		pBufferPrincipalID);

  // IHXAuthenticationDBManager
    STDMETHOD(AddPrincipal)(IHXAuthenticationDBManagerResponse* pDBMgrResponse,
	        IHXBuffer* pbufPrincipalID);

    STDMETHOD(RemovePrincipal)(IHXAuthenticationDBManagerResponse* pDBMgrResponse,
	        IHXBuffer* pbufPrincipalID);

    STDMETHOD(SetCredentials)(IHXAuthenticationDBManagerResponse* pDBMgrResponse,
	        IHXBuffer* pbufPrincipalID,IHXBuffer* pbufCredentials);

  // IHXAuthenticationDBManagerResponse
    STDMETHOD(AddPrincipalDone)(HX_RESULT hr, IHXBuffer* pbufPrincipalID);

    STDMETHOD(RemovePrincipalDone)(HX_RESULT hr,IHXBuffer* pbufPrincipalID);

    STDMETHOD(SetCredentialsDone)(HX_RESULT	hr, IHXBuffer* pbufPrincipalID);

protected:
    HX_RESULT  _MungeUserRealmPass(IHXBuffer*  pUserName,
	    IHXBuffer*  pRealm,	IHXBuffer*  pPassword, IHXBuffer** ppStorageKey);

    HXBOOL GetNameValuePair(const char*& instr, char* valname, char* valbuf);
    HXBOOL _GetQuotedValue(const char*& instr, char* szvalname, char* szvalbuf);
    HX_RESULT  _GetQuotedFields(char* s, IHXValues* pValues);
    void _SetPropertyFromCharArray(IHXValues* pOptions, const char* sName, 
	    const char* sValue);
    IHXValues* _GetResponseHeaders();

    // Attributes
    UINT32                               m_lRefCount;

    IUnknown*                            m_pContext; // class factory
    IHXPreferences*                      m_pPreferencesCore;

    IHXBuffer*				             m_pRealm;
    IHXBuffer*				             m_pDatabaseID;
    IHXBuffer*				             m_pPrincipalID;
    IHXAuthenticationDBManager*	         m_pAuthDBManager;
    IHXAuthenticationDBAccess*		     m_pAuthDBAccess;    
    IHXAuthenticationDBManagerResponse*  m_pAuthDBRespondee;
    IHXServerAuthResponse*		         m_pServerRespondee;
    IHXRequest*			                 m_pServerRequest;
    IHXRequestContext*			         m_pRequestContext;
    IHXValues*                           m_pCredentials;
    HXBOOL				                 m_bAuthenticated;
    HXBOOL				                 m_bIsProxyAuthentication;
};

#endif  // _HASHAUTHBASE_H_
