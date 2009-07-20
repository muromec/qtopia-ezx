/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxpref.h,v 1.9 2007/07/06 21:57:57 jfinnecy Exp $
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

#ifndef _HXPREFERENCES_
#define _HXPREFERENCES_

struct IHXScheduler;
class CPref;

#ifdef _UNIX
class HXPreferencesCallback;
#endif

#include "unkimp.h"
#include "hxprefs.h"
#include "hxstring.h"
#include "hxprefutil.h"
#include "ihxcontextuser.h"

class HXPreferences :	public CUnknownIMP,
			public IHXPreferences,
			public IHXPreferences2,
			public IHXPreferences3,
			public IHXContextUser
{
protected:
    CHXString			m_CompanyName;
    CHXString			m_ProductName;
    ULONG32			m_nProdMajorVer;
    ULONG32			m_nProdMinorVer;
    HXBOOL			m_bCommon;

public:
    HXPreferences();
    ~HXPreferences();

    DECLARE_UNKNOWN( HXPreferences )

    /************************************************************************
     *	Method:
     *		HXPreferences::Open
     *	Purpose:
     *		TBD
     */
    STDMETHOD(Open)			(THIS_
					    const char* pCompanyName, const char* pProductName, 
					    ULONG32 nProdMajorVer, ULONG32 nProdMinorVer);

    /************************************************************************
     *	Method:
     *		HXPreferences::OpenShared
     *	Purpose:
     *		Have this preference object read/write from the company wide shared
     *		location for all products
     */
    STDMETHOD(OpenShared)			(THIS_
						const char* pCompanyName);

    /************************************************************************
     *	Method:
     *		HXPreferences::OpenUserPref
     *	Purpose:
     *		Opens user-specific preferences on multi-user system.
     */
    STDMETHOD(OpenUserPref)		(THIS_
					    const char* pCompanyName, const char* pProductName, 
					    ULONG32 nProdMajorVer, ULONG32 nProdMinorVer);
    void      Close(void);

    /*
     * IHXPreferences methods
     */

    /************************************************************************
     *	Method:
     *		IHXPreferences::ReadPref
     *	Purpose:
     *		TBD
     */
    STDMETHOD(ReadPref)			(THIS_
					    const char* pPrefKey, IHXBuffer*& pBuffer);

    /************************************************************************
     *	Method:
     *		IHXPreferences::WritePref
     *	Purpose:
     *		TBD
     */
    STDMETHOD(WritePref)		(THIS_
					    const char* pPrefKey, IHXBuffer* pBuffer);


    /************************************************************************
     *	Method:
     *		IHXPreferences::GetPreferenceEnumerator
     *	Purpose:
     *		TBD
     */
    STDMETHOD(GetPreferenceEnumerator)(THIS_ 
					REF(IHXPreferenceEnumerator*) /*OUT*/ pEnum);

    /************************************************************************
     *	Method:
     *		IHXPreferences2::SetRoot
     *	Purpose:
     *		Reset the root of the preferences
     */

    STDMETHOD(ResetRoot)(THIS_ const char* pCompanyName, const char* pProductName, 
    int nProdMajorVer, int nProdMinorVer);

    /************************************************************************
     *	Method:
     *		IHXPreferences::DeletePref
     *	Purpose:
     *		TBD
     */
    STDMETHOD(DeletePref)		(THIS_ const char* pPrefKey);


    /************************************************************************
     *	Method:
     *		HXPreferences::OpenSharedUser
     *	Purpose:
     *		Have this preference object read/write from the company wide shared
     *		location for all products on a multi-user system
     */
    STDMETHOD(OpenSharedUser)(THIS_ const char* pCompanyName);
   
    STDMETHOD (RegisterContext)(THIS_ IUnknown* pContext);

#if defined(_UNIX) || defined(_CARBON)
    HX_RESULT CommitPrefs();
#endif

protected:
    CPref*	    m_pPref;
    IUnknown*	    m_pContext;
#ifdef _UNIX
    HXPreferencesCallback* m_pCallback;
#endif
};

class HXPreferenceEnumerator :	public IHXPreferenceEnumerator
{
protected:
    LONG32			m_lRefCount;

public:
    HXPreferenceEnumerator(const char* pCompanyName, 
			    const char* pProductName, 
			    ULONG32 nProdMajorVer, 
			    ULONG32 nProdMinorVer,
			    HXBOOL bCommon,
			    IUnknown* pContext);
    ~HXPreferenceEnumerator();

    /*
     * IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)	(THIS);

    STDMETHOD_(ULONG32,Release)	(THIS);

    /*
     * IHXPreferenceEnumerator methods
     */

    /************************************************************************
     *	Method:
     *		IHXPreferenceEnumerator::EndSubPref
     *	Purpose:
     *		TBD
     */

    STDMETHOD(BeginSubPref) (THIS_ const char* szSubPref);

    /************************************************************************
     *	Method:
     *		IHXPreferenceEnumerator::EndSubPref
     *	Purpose:
     *		TBD
     */

   STDMETHOD(EndSubPref) (THIS);

    /************************************************************************
     *	Method:
     *		IHXPreferenceEnumerator::GetPrefKey
     *	Purpose:
     *		TBD
     */

   STDMETHOD(GetPrefKey) (THIS_ UINT32 nIndex, REF(IHXBuffer*) pBuffer);

    /************************************************************************
     *	Method:
     *		IHXPreferenceEnumerator::ReadPref
     *	Purpose:
     *		TBD
     */
    STDMETHOD(ReadPref)			(THIS_
					    const char* pPrefKey, IHXBuffer*& pBuffer);


protected:
    CPref*	    m_pPref;
};

#endif /* _HXPREFERENCES_ */
