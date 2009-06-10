/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: crdcache.cpp,v 1.7 2007/07/06 20:39:16 jfinnecy Exp $
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

#include "hxcom.h"
#include "hxtypes.h"
#include "hxstring.h"
#include "hxmap.h"
#include "hxbuffer.h"
#include "chxpckts.h"
#include "pckunpck.h"
#include "crdcache.h"

#define DEFAULTCREDENTIALENTRY	"defaultCredentialEntry"

CHXCredentialsCache::CHXCredentialsCache(IUnknown* pContext)
{
    m_pContext = pContext;
    HX_ADDREF(m_pContext);
    m_lRefCount = 0;
}

CHXCredentialsCache::~CHXCredentialsCache()
{
    Close();
}

STDMETHODIMP
CHXCredentialsCache::QueryInterface(REFIID riid, void**ppvObj)
{
	QInterfaceList qiList[] =
	{
		{ GET_IIDHANDLE(IID_IUnknown), this },
		{ GET_IIDHANDLE(IID_IHXCredentialsCache), (IHXCredentialsCache*) this },
	};	
    return QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::AddRef
//  Purpose:
//	Everyone usually implements this the same... feel free to use
//	this implementation.
//
STDMETHODIMP_(ULONG32) CHXCredentialsCache::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::Release
//  Purpose:
//	Everyone usually implements this the same... feel free to use
//	this implementation.
//
STDMETHODIMP_(ULONG32) CHXCredentialsCache::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

STDMETHODIMP_(HXBOOL)
CHXCredentialsCache::IsEmpty(IHXBuffer* pBuffer)
{
    const char*		pKey = NULL;
    CredentialEntry*	pCredentialEntry = NULL;

    if (!pBuffer)
    {
	pKey = DEFAULTCREDENTIALENTRY;
    }
    else
    {
	pKey = (const char*)pBuffer->GetBuffer();
    }

    if (m_credentialMap.Lookup(pKey, (void*&)pCredentialEntry) &&
	pCredentialEntry)
    {
	return FALSE;
    }
    else
    {
	return TRUE;
    }
}

STDMETHODIMP 
CHXCredentialsCache::Empty(IHXBuffer* pBuffer)
{   
    HX_RESULT		hr = HXR_OK;
    const char*		pKey = NULL;
    CredentialEntry*	pCredentialEntry = NULL;

    if (!pBuffer)
    {
	pKey = DEFAULTCREDENTIALENTRY;
    }
    else
    {
	pKey = (const char*)pBuffer->GetBuffer();
    }

    if (m_credentialMap.Lookup(pKey, (void*&)pCredentialEntry) &&
	pCredentialEntry)
    {
	HX_DELETE(pCredentialEntry);
    
        m_credentialMap.RemoveKey(pKey);
    }

    return hr;
}

STDMETHODIMP
CHXCredentialsCache::FillCredentials(REF(IHXValues*) pValues)
{
    HX_RESULT		hr = HXR_FAILED;
    IHXBuffer*		pRealm = NULL;
    CredentialEntry*	pCredentialEntry = NULL;

    if (!pValues)
    {
	goto cleanup;
    }
    
    if (HXR_OK != pValues->GetPropertyCString("Realm", pRealm))
    {
	hr = CreateAndSetBufferCCF(pRealm, (UCHAR*)DEFAULTCREDENTIALENTRY, 
			           strlen(DEFAULTCREDENTIALENTRY)+1, m_pContext);
    }
        
    if (m_credentialMap.Lookup((const char*)pRealm->GetBuffer(), (void*&)pCredentialEntry) &&
	pCredentialEntry)
    {
	pValues->SetPropertyCString("Username", pCredentialEntry->m_pUserName);
	pValues->SetPropertyCString("Password", pCredentialEntry->m_pPassword);
	hr = HXR_OK;
    }

cleanup:

    HX_RELEASE(pRealm);

    return hr;
}

STDMETHODIMP
CHXCredentialsCache::SetCredentials(IHXValues* pValues)
{
    HX_RESULT		hr = HXR_OK;
    IHXBuffer*		pRealm = NULL;
    IHXBuffer*		pUserName = NULL;
    IHXBuffer*		pPassword = NULL;
    CredentialEntry*	pCredentialEntry = NULL;

    if (!pValues)
    {
	goto cleanup;
    }
    
    if (HXR_OK != pValues->GetPropertyCString("Realm", pRealm))
    {
	hr = CreateAndSetBufferCCF(pRealm, (UCHAR*)DEFAULTCREDENTIALENTRY, 
				   strlen(DEFAULTCREDENTIALENTRY)+1, m_pContext);
    }
    
    Empty(pRealm);

    if (HXR_OK == pValues->GetPropertyCString("Username", pUserName) && pUserName &&
	HXR_OK == pValues->GetPropertyCString("Password", pPassword) && pPassword)
    {
	pCredentialEntry = new CredentialEntry((char*)pUserName->GetBuffer(),
					       (char*)pPassword->GetBuffer(),
					       m_pContext);

	m_credentialMap.SetAt((const char*)pRealm->GetBuffer(), (void*)pCredentialEntry);
    }

cleanup:

    HX_RELEASE(pRealm);
    HX_RELEASE(pUserName);
    HX_RELEASE(pPassword);

    return hr;
}

void
CHXCredentialsCache::Close()
{
    CHXString		key;
    POSITION		p;
    CredentialEntry*	pCredentialEntry = NULL;
    
    p = m_credentialMap.GetStartPosition();
    while (p)
    {
	m_credentialMap.GetNextAssoc(p, key, (void*&)pCredentialEntry);
	HX_DELETE(pCredentialEntry);
    }
    m_credentialMap.RemoveAll();
    HX_RELEASE(m_pContext);
}
