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
#include <ctype.h>

#include "hxcom.h"
#include "hxplugn.h"
#include "hxplgns.h"
#include "hxfiles.h"
#include "hxcomm.h"
#include "hxprefs.h"
#include "hxauthn.h"
#include "hxdb.h"
#include "ihxpckts.h"
#include "pckunpck.h"

#include "hxassert.h"
#include "hxbuffer.h"

#include "md5.h"

#include "hashauthbase.h"

CHashAuthenticatorBase::CHashAuthenticatorBase():m_pContext(NULL), 
    m_pPreferencesCore(NULL), m_pRealm(NULL), m_pDatabaseID(NULL), 
    m_pPrincipalID(NULL), m_pAuthDBManager(NULL), m_pAuthDBAccess(NULL),
    m_pAuthDBRespondee(NULL), m_pServerRespondee(NULL), m_bAuthenticated(FALSE),
    m_pServerRequest(NULL), m_pRequestContext(NULL), m_pCredentials(NULL),
    m_bIsProxyAuthentication(FALSE), m_lRefCount(0)
{
}

CHashAuthenticatorBase::~CHashAuthenticatorBase()
{
    HX_RELEASE(m_pCredentials);
    HX_RELEASE(m_pServerRespondee);
    HX_RELEASE(m_pServerRequest);
    HX_RELEASE(m_pRequestContext);
    HX_RELEASE(m_pAuthDBManager);
    HX_RELEASE(m_pAuthDBAccess);
    HX_RELEASE(m_pAuthDBRespondee); 
    HX_RELEASE(m_pDatabaseID);
    HX_RELEASE(m_pPrincipalID);
    HX_RELEASE(m_pRealm);
    HX_RELEASE(m_pPreferencesCore);
    HX_RELEASE(m_pContext);    
}


// *** IUnknown methods ***

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::QueryInterface
//  Purpose:
//	Implement this to export the interfaces supported by your 
//	object.
//
STDMETHODIMP CHashAuthenticatorBase::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXObjectConfiguration))
    {
        AddRef();
        *ppvObj = (IHXObjectConfiguration*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXUserProperties))
    {
        AddRef();
        *ppvObj = (IHXUserProperties*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXAuthenticationDBManager))
    {
        AddRef();
        *ppvObj = (IHXAuthenticationDBManager*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXAuthenticationDBManagerResponse))
    {
        AddRef();
        *ppvObj = (IHXAuthenticationDBManagerResponse*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXAuthenticationDBAccess))
    {
        AddRef();
        *ppvObj = (IHXAuthenticationDBAccess*)this;
        return HXR_OK;
    }
    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

// IHXObjectConfiguration
STDMETHODIMP CHashAuthenticatorBase::SetContext(IUnknown* pContext)
{
    m_pContext = pContext;

    if (m_pContext)
    {
	m_pContext->AddRef();
    }

    HX_RELEASE(m_pPreferencesCore);
    pContext->QueryInterface(IID_IHXPreferences, (void**) &m_pPreferencesCore);

    return HXR_OK;
}

STDMETHODIMP CHashAuthenticatorBase::SetConfiguration(IHXValues* pConfig)
{
    HX_RESULT theErr = HXR_OK;

    if (!m_pContext || !pConfig)
    {
	HX_ASSERT(0);
	return HXR_UNEXPECTED;
    }

    if (!m_pRealm)
    {
	pConfig->GetPropertyCString("Realm", m_pRealm);
    }

    if (!m_pDatabaseID)
    {
	    pConfig->GetPropertyCString("DatabaseID", m_pDatabaseID);

	    if (m_pDatabaseID)
	    {
	        //   IHXDatabaseManager - decides which db to use
	        //   IHXAuthenticationDBManager - controls access to one db
	        IUnknown*                punkDbMgr = NULL;
	        IHXDatabaseManager*     pDbMgr = NULL;
	        IUnknown*                pDatabase = NULL;
	        IHXCommonClassFactory*  pFactory = NULL;

	        m_pContext->QueryInterface(IID_IHXCommonClassFactory, (void **)&pFactory);
            if (pFactory)
            {
    	        pFactory->CreateInstance(CLSID_CHXDatabaseManager, (void**)&punkDbMgr);
            }
            if (punkDbMgr)
            {
	            punkDbMgr->QueryInterface(IID_IHXDatabaseManager, (void**)&pDbMgr);
            }
            if (pDbMgr)
            {
	            pDbMgr->GetInstanceFromID(m_pDatabaseID, pDatabase);
            }
            if (pDatabase)
            {
	            pDatabase->QueryInterface(IID_IHXAuthenticationDBAccess, (void**)&m_pAuthDBAccess);
	            pDatabase->QueryInterface(IID_IHXAuthenticationDBManager, (void**)&m_pAuthDBManager);
            }
            else
            {
                theErr = HXR_FAIL;
            }

            HX_ASSERT(pFactory);
            HX_ASSERT(punkDbMgr);
            HX_ASSERT(pDbMgr);
            HX_ASSERT(pDatabase);

	        HX_RELEASE(punkDbMgr);
	        HX_RELEASE(pDbMgr);
	        HX_RELEASE(pDatabase);
	        HX_RELEASE(pFactory);
	    }
    }

    return HXR_OK;
}

// IHXUserProperties
STDMETHODIMP CHashAuthenticatorBase::GetPrincipalID(REF(IHXBuffer*) pPrincipalID)
{
    pPrincipalID = m_pPrincipalID;

    if (pPrincipalID)
    {
	    pPrincipalID->AddRef();
	    return HXR_OK;
    }

    return HXR_FAIL;
}

STDMETHODIMP CHashAuthenticatorBase::GetAuthorityName(REF(IHXBuffer*) pRealm)
{
    pRealm = m_pRealm;

    if (pRealm)
    {
	    pRealm->AddRef();
	    return HXR_OK;
    }

    return HXR_FAIL;
}

// IHXAuthenticationDBManager
STDMETHODIMP CHashAuthenticatorBase::AddPrincipal(IHXAuthenticationDBManagerResponse* pDBRespondee,
        IHXBuffer* pPrincipalID)
{
    if(!pDBRespondee)
    {
	    return HXR_UNEXPECTED;
    }

    m_pAuthDBRespondee = pDBRespondee;
    m_pAuthDBRespondee->AddRef();

    if (!m_pAuthDBManager)
    {
	    m_pAuthDBRespondee->AddPrincipalDone(HXR_NOINTERFACE, pPrincipalID);
	    HX_RELEASE(m_pAuthDBRespondee);
	    return HXR_NOINTERFACE;
    }

    m_pAuthDBManager->AddPrincipal(this, pPrincipalID);

    return HXR_OK;
}

STDMETHODIMP CHashAuthenticatorBase::RemovePrincipal(IHXAuthenticationDBManagerResponse* pDBRespondee,
        IHXBuffer* pPrincipalID)
{
    if(!pDBRespondee)
    {
    	return HXR_UNEXPECTED;
    }

    m_pAuthDBRespondee = pDBRespondee;
    m_pAuthDBRespondee->AddRef();

    if (!m_pAuthDBManager)
    {
	    m_pAuthDBRespondee->RemovePrincipalDone(HXR_NOINTERFACE, pPrincipalID);
	    HX_RELEASE(m_pAuthDBRespondee);
	    return HXR_NOINTERFACE;
    }

    m_pAuthDBManager->RemovePrincipal(this, pPrincipalID);

    return HXR_OK;
}

STDMETHODIMP CHashAuthenticatorBase::SetCredentials(IHXAuthenticationDBManagerResponse* pDBRespondee,
        IHXBuffer* pPrincipalID, IHXBuffer* pPassword)
{
    if(!pDBRespondee)
    {
	    return HXR_UNEXPECTED;
    }

    m_pAuthDBRespondee = pDBRespondee;
    m_pAuthDBRespondee->AddRef();

    if (!m_pAuthDBManager)
    {
	    m_pAuthDBRespondee->SetCredentialsDone(HXR_NOINTERFACE, pPrincipalID);
	    HX_RELEASE(m_pAuthDBRespondee);
	    return HXR_NOINTERFACE;
    }

    IHXBuffer* pStorageKey = NULL;
    _MungeUserRealmPass(pPrincipalID, m_pRealm, pPassword, &pStorageKey);
    m_pAuthDBManager->SetCredentials(this, pPrincipalID, pStorageKey);

    HX_RELEASE(pStorageKey);

    return HXR_OK;
}


// IHXAuthenticationDBManagerResponse
STDMETHODIMP CHashAuthenticatorBase::AddPrincipalDone(HX_RESULT	hr,
                                                IHXBuffer* pBufferPrincipalID)
{
    m_pAuthDBRespondee->AddPrincipalDone(hr, pBufferPrincipalID);
    HX_RELEASE(m_pAuthDBRespondee);
    return HXR_OK;
}

STDMETHODIMP CHashAuthenticatorBase::RemovePrincipalDone(HX_RESULT hr,
                                            IHXBuffer* pBufferPrincipalID)
{
    m_pAuthDBRespondee->RemovePrincipalDone(hr, pBufferPrincipalID);
    HX_RELEASE(m_pAuthDBRespondee);
    return HXR_OK;
}

STDMETHODIMP CHashAuthenticatorBase::SetCredentialsDone(HX_RESULT hr, 
                                           IHXBuffer* pBufferPrincipalID)
{
    m_pAuthDBRespondee->SetCredentialsDone(hr, pBufferPrincipalID);
    HX_RELEASE(m_pAuthDBRespondee);
    return HXR_OK;
}

/************************************************************************
 *	Method:
 *	    IHXAuthenticationDBAccess::_NewEnum
 *	Purpose:
 *	    
 *	    Call this to make a new enumerator of this collection.
 *
 */
STDMETHODIMP CHashAuthenticatorBase::_NewEnum(REF(IHXAsyncEnumAuthenticationDB*) pEnumDBNew)
{
    if (m_pAuthDBAccess)
    {
    	return m_pAuthDBAccess->_NewEnum(pEnumDBNew);
    }
    else
    {
	    pEnumDBNew = NULL;
	    return HXR_NOINTERFACE;
    }
}

/************************************************************************
 *	Method:
 *	    IRMAAuthenticationDBAccess::CheckExistence
 *	Purpose:
 *	    
 *	    Call this to verify the existance of a principal.
 *
 */
STDMETHODIMP CHashAuthenticatorBase::CheckExistence(IHXAuthenticationDBAccessResponse* pDBAccessResp,
    IHXBuffer*		pBufferPrincipalID)
{
    if(!pDBAccessResp)
    {
    	return HXR_UNEXPECTED;
    }

    // XXXSSH - why not implemented?
    pDBAccessResp->ExistenceCheckDone(HXR_NOTIMPL, pBufferPrincipalID);

    return HXR_OK;
}

/************************************************************************
 *	Method:
 *	    IHXAuthenticationDBAccess::GetCredentials
 *	Purpose:
 *	    
 *	    Call this to access the credentials for the specified principal.
 *
 */
STDMETHODIMP CHashAuthenticatorBase::GetCredentials(IHXAuthenticationDBAccessResponse* pDBAccessResp,
    IHXBuffer*		pBufferPrincipalID)
{
    if(!pDBAccessResp)
    {
	return HXR_UNEXPECTED;
    }

    // XXXSSH - why not implemented?
    pDBAccessResp->GetCredentialsDone(HXR_NOTIMPL, pBufferPrincipalID, NULL);

    return HXR_OK;
}

HX_RESULT CHashAuthenticatorBase::_MungeUserRealmPass(IHXBuffer*  pUserName,
         IHXBuffer*  pRealm, IHXBuffer*  pPassword, IHXBuffer** ppStorageKey)
{
    HX_ASSERT(pUserName && pPassword && pRealm);

    char resbuf[1024]; /* Flawfinder: ignore */

    if (HXR_OK == CreateBufferCCF(*ppStorageKey, m_pContext))
    {
	(*ppStorageKey)->SetSize(64);

	char* sMD5 = (char*)(*ppStorageKey)->GetBuffer();

	sprintf(resbuf, "%-.200s:%-.200s:%-.200s", /* Flawfinder: ignore */
	    pUserName->GetBuffer(),
	    pRealm->GetBuffer(),
	    pPassword->GetBuffer());

	//The result of this MD5 is what needs to be stored in
	//a file somewhere in the server, associated with the username.
	MD5Data(sMD5, (BYTE*)resbuf, strlen(resbuf));
    }

    return HXR_OK;
}

HXBOOL CHashAuthenticatorBase::_GetQuotedValue(const char*& instr, char* valname, char* valbuf)
{
    char* equals = (char*)strchr(instr, '=');
    if(!equals)
	return FALSE;

    while(isspace(*(equals - 1)) && equals > instr) equals--;
    if(equals <= instr || (equals - instr > 200))
	return FALSE;

    strncpy(valname, instr, equals - instr); /* Flawfinder: ignore */
    valname[equals -instr] = 0;

    char* firstquote = strchr(equals, '"');
    if(!firstquote)
	return FALSE;

    char* secondquote = strchr(firstquote + 1, '"');
    if(!secondquote || (secondquote - firstquote > 200))
	return FALSE;

    strncpy(valbuf, firstquote + 1, secondquote - firstquote - 1); /* Flawfinder: ignore */
    valbuf[secondquote - firstquote - 1] = 0;
    instr = secondquote + 1;
    return TRUE;
}

HXBOOL CHashAuthenticatorBase::GetNameValuePair(const char*& instr, char* valname, char* valbuf)
{
    char* equals = (char*)strchr(instr, '=');
    if(!equals)
    	return FALSE;
    
    while(isspace(*(equals - 1)) && equals > instr) equals--;
    if(equals <= instr || (equals - instr > 200))
	    return FALSE;

    strncpy(valname, instr, equals - instr); /* Flawfinder: ignore */
    valname[equals -instr] = 0;

    equals++;

    while (isspace(*equals)) equals++;
    if (*equals==',')
    {
        return FALSE;
    }
    else
    {
        if (*equals=='"')
        {
            char* firstquote = equals;
            char* secondquote = strchr(firstquote + 1, '"');
            if(!secondquote || (secondquote - firstquote > 200))
	            return FALSE;

            strncpy(valbuf, firstquote + 1, secondquote - firstquote - 1); /* Flawfinder: ignore */
            valbuf[secondquote - firstquote - 1] = 0;
            instr = secondquote + 1;
            return TRUE;
        }
        else
        {
            // Non Quoted string
            char* szvalstart = equals;
            equals++;
            while (!isspace(*equals) && *equals && (*equals!=',')) equals++;
            char* szvalend = equals;
            strncpy(valbuf, szvalstart, szvalend-szvalstart);
            valbuf[szvalend-szvalstart] = 0;
            instr = szvalend;
            return TRUE;
        }
    } 
}

HX_RESULT CHashAuthenticatorBase::_GetQuotedFields(char* s, IHXValues* pValues)
{
    char valbuf[256]; /* Flawfinder: ignore */
    char valname[256]; /* Flawfinder: ignore */
    HXBOOL done = FALSE;

    while(!done)
    {
	    while ((isspace(*s) || *s == ',') && *s)
	    {
	        ++s;
	    }

	    if(!(*s))
	        break;

	    if (GetNameValuePair((const char*&)s, valname, valbuf))
	    {
	        _SetPropertyFromCharArray(pValues, valname, valbuf);
	    }
	    else
	    {
	        done = TRUE;
	    }
    }

    return HXR_OK;
}

void 
CHashAuthenticatorBase::_SetPropertyFromCharArray(IHXValues* pOptions, 
			  const char* sName, const char* sValue)
{
    IHXBuffer* pVal = NULL;
    CHXBuffer::FromCharArray(sValue, &pVal);
    pOptions->SetPropertyCString(sName, pVal);
    HX_RELEASE(pVal);
}

/* 
    Returns the repsonse headers from m_pServerRequest, creating them if they 
    don't exist. Caller must release the returned pointer 
*/
IHXValues* CHashAuthenticatorBase::_GetResponseHeaders()
{
    HX_ASSERT(m_pServerRequest);
    if (!m_pServerRequest)
	    return NULL;    // uh-oh

    IHXValues* pResHeaders = NULL;

    m_pServerRequest->GetResponseHeaders(pResHeaders);
    if (!pResHeaders)
    {
	    IUnknown* pUnknown = NULL;
        IHXCommonClassFactory*  pFactory = NULL;

	    m_pContext->QueryInterface(IID_IHXCommonClassFactory,
	        (void **)&pFactory);

	    if (pFactory)
	    {
	        pFactory->CreateInstance(CLSID_IHXKeyValueList, (void**)&pUnknown);
	        HX_RELEASE(pFactory);
	    }
	    if (pUnknown)
	    {
	        pUnknown->QueryInterface(IID_IHXValues, (void**)&pResHeaders);
	        
	        if (pResHeaders)
	        {
    		    m_pServerRequest->SetResponseHeaders(pResHeaders);
	        }
            HX_RELEASE(pUnknown);
	    }
    }

    return pResHeaders;
}
