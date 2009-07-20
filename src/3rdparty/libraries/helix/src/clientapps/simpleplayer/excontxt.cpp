/* ***** BEGIN LICENSE BLOCK ***** 
 * Version: RCSL 1.0/RPSL 1.0 
 *  
 * Portions Copyright (c) 1995-2002 RealNetworks, Inc. All Rights Reserved. 
 *      
 * The contents of this file, and the files included with this file, are 
 * subject to the current version of the RealNetworks Public Source License 
 * Version 1.0 (the "RPSL") available at 
 * http://www.helixcommunity.org/content/rpsl unless you have licensed 
 * the file under the RealNetworks Community Source License Version 1.0 
 * (the "RCSL") available at http://www.helixcommunity.org/content/rcsl, 
 * in which case the RCSL will apply. You may also obtain the license terms 
 * directly from RealNetworks.  You may not use this file except in 
 * compliance with the RPSL or, if you have a valid RCSL with RealNetworks 
 * applicable to this file, the RCSL.  Please see the applicable RPSL or 
 * RCSL for the rights, obligations and limitations governing use of the 
 * contents of the file.  
 *  
 * This file is part of the Helix DNA Technology. RealNetworks is the 
 * developer of the Original Code and owns the copyrights in the portions 
 * it created. 
 *  
 * This file, and the files included with this file, is distributed and made 
 * available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER 
 * EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS ALL SUCH WARRANTIES, 
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS 
 * FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT. 
 * 
 * Technology Compatibility Kit Test Suite(s) Location: 
 *    http://www.helixcommunity.org/content/tck 
 * 
 * Contributor(s): 
 *  
 * ***** END LICENSE BLOCK ***** */ 

#include "hxtypes.h"

#include "hxwintyp.h"
#include "hxcom.h"
#include "hxcomm.h"
#include "hxwin.h"
#include "fivemmap.h"

#include "hxbuffer.h"
#include "hxmangle.h"

#include "hxclsnk.h"
#include "hxgroup.h"
#include "hxerror.h"
#include "hxprefs.h"
#include "hxstrutl.h"

#include "exadvsnk.h"
#include "exerror.h"
#include "exsitsup.h"
#include "exaumgr.h"
#include "hxprdnld.h"
#include "exprdnld.h"

#include "excontxt.h"

extern HXBOOL bEnableAdviceSink;


ExampleClientContext::ExampleClientContext(LONG32 lClientIndex)
    : m_lRefCount(0)
    , m_lClientIndex(lClientIndex)
    , m_pClientSink(NULL)
    , m_pErrorSink(NULL)
    , m_pAuthMgr(NULL)
    , m_pSiteSupplier(NULL)
    , m_pDefaultPrefs(NULL)
#if defined(HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS)
    , m_pPrgDnldStatusObserver(NULL)
#endif // /HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS.
{
}


ExampleClientContext::~ExampleClientContext()
{
    Close();
};

void ExampleClientContext::Init(IUnknown*	 /*IN*/ pUnknown,
				IHXPreferences* /*IN*/ pPreferences,
				char*		 /*IN*/ pszGUID)
{
    char* pszCipher = NULL;

	
    m_pClientSink	= new ExampleClientAdviceSink(pUnknown, m_lClientIndex);
    m_pErrorSink	= new ExampleErrorSink(pUnknown);
#if defined(HELIX_FEATURE_AUTHENTICATION)
    m_pAuthMgr          = new ExampleAuthenticationManager();
    if(m_pAuthMgr)
    {
	m_pAuthMgr->AddRef();
    }

#endif /* #if defined(HELIX_FEATURE_AUTHENTICATION) */
#if defined(HELIX_FEATURE_VIDEO)
    m_pSiteSupplier	= new ExampleSiteSupplier(pUnknown);
#endif 

#if defined(HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS)
    m_pPrgDnldStatusObserver = new ExamplePDStatusObserver(pUnknown);
#endif // /HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS.

    if (m_pClientSink)
    {
	m_pClientSink->AddRef();
    }
    
    if (m_pErrorSink)
    {
	m_pErrorSink->AddRef();
    }

    if(m_pSiteSupplier)
    {
	m_pSiteSupplier->AddRef();
    }

    if (pPreferences)
    {
	m_pDefaultPrefs = pPreferences;
	m_pDefaultPrefs->AddRef();
    }

    if (pszGUID && *pszGUID)
    {
	// Encode GUID
	pszCipher = Cipher(pszGUID);
	SafeStrCpy(m_pszGUID,  pszCipher, 256);
    }
    else
    {
	m_pszGUID[0] = '\0';
    }
}

void ExampleClientContext::Close()
{
    HX_RELEASE(m_pClientSink);
    HX_RELEASE(m_pErrorSink);
#if defined(HELIX_FEATURE_AUTHENTICATION)
    HX_RELEASE(m_pAuthMgr);
#endif /* #if defined(HELIX_FEATURE_AUTHENTICATION) */
    HX_RELEASE(m_pSiteSupplier);
    HX_RELEASE(m_pDefaultPrefs);
#if defined(HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS)
    HX_RELEASE(m_pPrgDnldStatusObserver);
#endif // /HELIX_FEATURE_PROGRESSIVE_DOWNLD_STATUS.
}



// *** IUnknown methods ***

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::QueryInterface
//  Purpose:
//	Implement this to export the interfaces supported by your 
//	object.
//
STDMETHODIMP ExampleClientContext::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
	AddRef();
	*ppvObj = this;
	return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXPreferences))
    {
	AddRef();
	*ppvObj = (IHXPreferences*)this;
	return HXR_OK;
    }
    else if (m_pClientSink && 
	     m_pClientSink->QueryInterface(riid, ppvObj) == HXR_OK)
    {
	return HXR_OK;
    }
    else if (m_pErrorSink && 
	     m_pErrorSink->QueryInterface(riid, ppvObj) == HXR_OK)
    {
	return HXR_OK;
    }
#if defined(HELIX_FEATURE_AUTHENTICATION)
    else if(m_pAuthMgr &&
	    m_pAuthMgr->QueryInterface(riid, ppvObj) == HXR_OK)
    {
	return HXR_OK;
    }
#endif /* #if defined(HELIX_FEATURE_AUTHENTICATION) */
    else if(m_pSiteSupplier &&
	    m_pSiteSupplier->QueryInterface(riid, ppvObj) == HXR_OK)
    {
	return HXR_OK;
    }
    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::AddRef
//  Purpose:
//	Everyone usually implements this the same... feel free to use
//	this implementation.
//
STDMETHODIMP_(ULONG32) ExampleClientContext::AddRef()
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
STDMETHODIMP_(ULONG32) ExampleClientContext::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}


// *** IUnknown methods ***

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IHXPreferences::ReadPref
//  Purpose:
//	Read a Preference from the registry.
//
STDMETHODIMP
ExampleClientContext::ReadPref(const char* pref_key, IHXBuffer*& buffer)
{
    HX_RESULT hResult	= HXR_OK;
    char*     pszCipher = NULL;
    
    if ((stricmp(pref_key, CLIENT_GUID_REGNAME) == 0) &&
	(*m_pszGUID))
    {
	// Create a Buffer 
	buffer = new CHXBuffer();
	buffer->AddRef();

	// Copy the encoded GUID into the buffer
	buffer->Set((UCHAR*)m_pszGUID, strlen(m_pszGUID) + 1);
    }
    else if (m_pDefaultPrefs)
    {
	hResult = m_pDefaultPrefs->ReadPref(pref_key, buffer);
    }
    else
    {
	hResult = HXR_NOTIMPL;
    }

    return hResult;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IHXPreferences::WritePref
//  Purpose:
//	Write a Preference to the registry.
//
STDMETHODIMP
ExampleClientContext::WritePref(const char* pref_key, IHXBuffer* buffer)
{
    if (m_pDefaultPrefs)
    {
	return m_pDefaultPrefs->WritePref(pref_key, buffer);
    }
    else	
    {
	return HXR_OK;
    }
}


