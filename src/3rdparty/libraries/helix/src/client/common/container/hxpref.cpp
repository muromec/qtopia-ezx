/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxpref.cpp,v 1.10 2006/03/13 22:16:35 ping Exp $
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

#include "hxtypes.h"
#include "hxresult.h"
#include "hxcom.h"
#include "ihxpckts.h"
#include "hxfiles.h"
#include "hxcore.h"
#include "hxprefs.h"
#include "hxengin.h"


#include "pref.h"
#include "playpref.h"
#include "hxpref.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif


BEGIN_INTERFACE_LIST( HXPreferences )
INTERFACE_LIST_ENTRY_SIMPLE( IHXPreferences );
#ifndef _UNIX
INTERFACE_LIST_ENTRY_SIMPLE( IHXPreferences2 );
#endif
INTERFACE_LIST_ENTRY_SIMPLE( IHXPreferences3 );
INTERFACE_LIST_ENTRY_SIMPLE( IHXContextUser );
END_INTERFACE_LIST

#ifdef _UNIX

#define	    PREF_COMMIT_TIMEOUT	    0

class HXPreferencesCallback : public IHXCallback
{
public:

    /*
     * IUnknown methods
     */

    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj)
    {
	if (IsEqualIID(riid, IID_IUnknown))
	{
	    AddRef();
	    *ppvObj = (IUnknown*)this;
	    return HXR_OK;
	}
	else if (IsEqualIID(riid, IID_IHXCallback))
	{
	    AddRef();
	    *ppvObj = (IHXCallback*)this;
	    return HXR_OK;
	}

	*ppvObj = NULL;
	return HXR_NOINTERFACE;
    };

    STDMETHOD_(ULONG32,AddRef)	(THIS)
    {
	return InterlockedIncrement(&m_lRefCount);
    };

    STDMETHOD_(ULONG32,Release)	(THIS)
    {
	if (InterlockedDecrement(&m_lRefCount) > 0)
	{
	    return m_lRefCount;
	}

	delete this;
	return 0;
    };

    /*
     *  IHXCallback methods
     */
    
    STDMETHOD(Func)		(THIS)
    {
	m_CallbackHandle = 0;
	return m_pPreferences->CommitPrefs();	
    }

    void Close()
    {
	if (m_CallbackHandle && m_pScheduler)
	{
	    m_pScheduler->Remove(m_CallbackHandle);
	    m_CallbackHandle = 0;
	}

	HX_RELEASE(m_pScheduler);
    };

    HXBOOL IsPending() {return (m_CallbackHandle != 0); };

    void
    ScheduleCallback()
    {
	HX_ASSERT(m_CallbackHandle == 0);
	if (m_pScheduler && !m_CallbackHandle)
	{
	    m_CallbackHandle = m_pScheduler->RelativeEnter(this, PREF_COMMIT_TIMEOUT);
	}
    }

    HXPreferencesCallback(HXPreferences* pPreferences, IUnknown* pContext)
    {
	m_lRefCount = 0;
	m_pPreferences = pPreferences;
	m_pScheduler = 0;
	m_CallbackHandle = 0;

	if (pContext)
	{
	    HX_VERIFY(HXR_OK == 
		pContext->QueryInterface(IID_IHXScheduler, (void**) &m_pScheduler));
 	}
    };


    ~HXPreferencesCallback()
    {
	Close();
    };

    INT32	    m_lRefCount;
    CallbackHandle  m_CallbackHandle;
    IHXScheduler*  m_pScheduler;
    HXPreferences* m_pPreferences;
};
#endif

HXPreferences::HXPreferences() :
    m_pPref(0),
    m_nProdMajorVer(0),
    m_nProdMinorVer(0),
    m_bCommon(TRUE),
    m_pContext(0)
#ifdef _UNIX
    ,m_pCallback(NULL)
#endif
{
}

HXPreferences::~HXPreferences()
{
    Close();
}

void
HXPreferences::Close()
{
#ifdef _UNIX
    if (m_pCallback)
    {
	m_pCallback->Close();
	HX_RELEASE(m_pCallback);
    }
#endif

    HX_RELEASE(m_pContext);
    HX_DELETE(m_pPref);
}

STDMETHODIMP
HXPreferences::RegisterContext(IUnknown* pContext)
{
    m_pContext = pContext;
    if (m_pContext)
    {
	m_pContext->AddRef();
    }

#ifdef _UNIX
    if (m_pContext && !m_pCallback)
    {
	m_pCallback = new HXPreferencesCallback(this, m_pContext);
        if(!m_pCallback)
        {
            return HXR_OUTOFMEMORY;
        }
	m_pCallback->AddRef();
    }
#endif
    return HXR_OK;
}

#if defined(_UNIX) || defined(_CARBON)
HX_RESULT
HXPreferences::CommitPrefs()
{
    HX_RESULT theErr = HXR_OK;
    if (m_pPref)
    {
	theErr = m_pPref->commit_prefs();
    }

    return theErr;
}
#endif

/*
 * HXPreferences methods
 */
/************************************************************************
 *	Method:
 *		IHXPreferences::Init
 *	Purpose:
 *		TBD
 */
STDMETHODIMP HXPreferences::Open(const char* pCompanyName, const char* pProductName, 
					ULONG32 nProdMajorVer, ULONG32 nProdMinorVer)
{
    m_CompanyName = pCompanyName;
    m_ProductName = pProductName;
    m_nProdMajorVer = nProdMajorVer;
    m_nProdMinorVer = nProdMinorVer;
    
    m_pPref = CPlayerPref::open_pref(pCompanyName, pProductName, nProdMajorVer, nProdMinorVer, FALSE, m_pContext);
    
    if (!m_pPref)
    {
	return HXR_UNEXPECTED;
    }

    m_bCommon = m_pPref->IsCommonPref();

    return HXR_OK;
}

STDMETHODIMP HXPreferences::OpenShared(const char* pCompanyName)
{
    m_CompanyName = pCompanyName;
    m_ProductName = HX_PRODUCTNAME_SHARED;
    m_nProdMajorVer = 0;
    m_nProdMinorVer = 0;
    
    m_pPref = CPlayerPref::open_shared_pref(pCompanyName, m_pContext);

    if (!m_pPref)
    {
	return HXR_UNEXPECTED;
    }

    m_bCommon = m_pPref->IsCommonPref();

    return HXR_OK;
}

STDMETHODIMP HXPreferences::OpenSharedUser(const char* pCompanyName)
{
    m_CompanyName = pCompanyName;
    m_ProductName = HX_PRODUCTNAME_SHARED;
    m_nProdMajorVer = 0;
    m_nProdMinorVer = 0;
    
    m_pPref = CPlayerPref::open_shared_user_pref(pCompanyName, m_pContext);

    if (!m_pPref)
    {
	return HXR_UNEXPECTED;
    }

    m_bCommon = m_pPref->IsCommonPref();

    return HXR_OK;
}



STDMETHODIMP HXPreferences::OpenUserPref(const char* pCompanyName, const char* pProductName, 
					ULONG32 nProdMajorVer, ULONG32 nProdMinorVer)
{
    m_CompanyName = pCompanyName;
    m_ProductName = pProductName;
    m_nProdMajorVer = nProdMajorVer;
    m_nProdMinorVer = nProdMinorVer;
    
    m_pPref = CPlayerPref::open_pref(pCompanyName, pProductName, nProdMajorVer, nProdMinorVer, FALSE, m_pContext);

    if (!m_pPref)
    {
	return HXR_UNEXPECTED;
    }

    m_bCommon = m_pPref->IsCommonPref();

    return HXR_OK;
}



/************************************************************************
 *	Method:
 *		IHXPreferences::ReadPref
 *	Purpose:
 *		TBD
 */
STDMETHODIMP HXPreferences::ReadPref(const char* pPrefKey, IHXBuffer*& pBuffer)
{
    return m_pPref->read_pref(pPrefKey, pBuffer);
}


/************************************************************************
 *	Method:
 *		IHXPreferences::WritePref
 *	Purpose:
 *		TBD
 */
STDMETHODIMP HXPreferences::WritePref(const char* pPrefKey, IHXBuffer* pBuffer)
{
    HX_RESULT theErr = HXR_FAIL;
    if( m_pPref )
    {
        theErr = m_pPref->write_pref(pPrefKey, pBuffer);
    }

#ifdef _UNIX
    if (m_pCallback && !m_pCallback->IsPending())
    {
	m_pCallback->ScheduleCallback();
    }
#endif

    return theErr;
}

/************************************************************************
 *	Method:
 *		IHXPreferences::DeletePref
 *	Purpose:
 *		TBD
 */
STDMETHODIMP HXPreferences::DeletePref(const char* pPrefKey)
{
    HX_RESULT theErr = m_pPref->delete_pref(pPrefKey);

#ifdef _UNIX
    if (m_pCallback && !m_pCallback->IsPending())
    {
	m_pCallback->ScheduleCallback();
    }
#endif

    return theErr;
}

/************************************************************************
 *	Method:
 *		IHXPreferences::GetPreferenceEnumerator
 *	Purpose:
 *		TBD
 */
STDMETHODIMP HXPreferences::GetPreferenceEnumerator(
					REF(IHXPreferenceEnumerator*) /*OUT*/ pEnum)
{
    pEnum = new HXPreferenceEnumerator(
							m_CompanyName, 
							m_ProductName, 
							m_nProdMajorVer, 
							m_nProdMinorVer,
							m_bCommon,
							m_pContext);
    if (!pEnum)
	return HXR_FAIL;

    pEnum->AddRef();
    return HXR_OK;
}


STDMETHODIMP HXPreferences::ResetRoot(const char* pCompanyName, const char* pProductName, 
int nProdMajorVer, int nProdMinorVer)
{
    if (m_pPref)
    {
	delete m_pPref;
    }

    if(m_bCommon)
	return Open( pCompanyName, pProductName, nProdMajorVer, nProdMinorVer );

    return OpenUserPref( pCompanyName, pProductName, nProdMajorVer, nProdMinorVer );
}



/***********************************************************************
****		    HXPreferenceEnumerator			    ****
***********************************************************************/


HXPreferenceEnumerator::HXPreferenceEnumerator(
				    const char* pCompanyName, 
				    const char* pProductName, 
				    ULONG32 nProdMajorVer, 
				    ULONG32 nProdMinorVer,
				    HXBOOL bCommon,
				    IUnknown* pContext)
	:	m_lRefCount (0)
	,	m_pPref(0)
{
    m_pPref = CPlayerPref::open_pref(pCompanyName, pProductName, nProdMajorVer, nProdMinorVer, bCommon, pContext);
}

HXPreferenceEnumerator::~HXPreferenceEnumerator()
{
    if (m_pPref)
    {
	delete m_pPref;
	m_pPref = 0;
    }
}

/*
 * IUnknown methods
 */


/////////////////////////////////////////////////////////////////////////
//	Method:
//		IUnknown::QueryInterface
//	Purpose:
//		Implement this to export the interfaces supported by your 
//		object.
//
STDMETHODIMP HXPreferenceEnumerator::QueryInterface(REFIID riid, void** ppvObj)
{
    QInterfaceList qiList[] =
        {
            { GET_IIDHANDLE(IID_IHXPreferenceEnumerator), (IHXPreferenceEnumerator*)this },
            { GET_IIDHANDLE(IID_IUnknown), (IUnknown*)(IHXPreferenceEnumerator*)this },
        };
    
    return ::QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
}

/////////////////////////////////////////////////////////////////////////
//	Method:
//		IUnknown::AddRef
//	Purpose:
//		Everyone usually implements this the same... feel free to use
//		this implementation.
//
STDMETHODIMP_(ULONG32) HXPreferenceEnumerator::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

/////////////////////////////////////////////////////////////////////////
//	Method:
//		IUnknown::Release
//	Purpose:
//		Everyone usually implements this the same... feel free to use
//		this implementation.
//
STDMETHODIMP_(ULONG32) HXPreferenceEnumerator::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
	return m_lRefCount;
    }

    delete this;
    return 0;
}


/************************************************************************
 *	Method:
 *		IHXPreferenceEnumerator::EndSubPref
 *	Purpose:
 *		TBD
 */

STDMETHODIMP HXPreferenceEnumerator::BeginSubPref (const char* szSubPref)
{
    return m_pPref->BeginSubPref(szSubPref);
}


/************************************************************************
 *	Method:
 *		IHXPreferenceEnumerator::EndSubPref
 *	Purpose:
 *		TBD
 */

STDMETHODIMP HXPreferenceEnumerator::EndSubPref()
{
    return m_pPref->EndSubPref();
}

/************************************************************************
 *	Method:
 *		IHXPreferenceEnumerator::GetPrefKey
 *	Purpose:
 *		TBD
 */

STDMETHODIMP HXPreferenceEnumerator::GetPrefKey (UINT32 nIndex, REF(IHXBuffer*) pBuffer)
{
    return m_pPref->GetPrefKey(nIndex, pBuffer);
}


/************************************************************************
 *	Method:
 *		IHXPreferenceEnumerator::ReadPref
 *	Purpose:
 *		TBD
 */

STDMETHODIMP HXPreferenceEnumerator::ReadPref(const char* pPrefKey, IHXBuffer*& pBuffer)
{
    return m_pPref->read_pref(pPrefKey, pBuffer);
}
