/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxpreferencesenumerator.cpp,v 1.3 2006/12/06 09:43:53 gahluwalia Exp $
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

// Local
#include "hxpreferencesenumerator.h"
#include "hxpreferences.h"  // CHXPreferences member variable

// Utility
#include "hxpreferencesutils.h"

BEGIN_INTERFACE_LIST_NOCREATE(CHXPreferencesEnumerator)
    INTERFACE_LIST_ENTRY_SIMPLE(IHXPreferenceEnumerator)
END_INTERFACE_LIST

//
// Constructor / Destructor
//
CHXPreferencesEnumerator::CHXPreferencesEnumerator(CHXPreferences* pPrefs, const char* szRootKey)
    : m_pPrefs(pPrefs)
    , m_strCurrentPrefKey(szRootKey)
{
    HX_ADDREF(m_pPrefs);
}

CHXPreferencesEnumerator::~CHXPreferencesEnumerator()
{
    HX_RELEASE(m_pPrefs);
}

//
// IHXPreferenceEnumerator methods
//
STDMETHODIMP
CHXPreferencesEnumerator::BeginSubPref(THIS_ const char* szSubPref)
{
    HX_ASSERT(szSubPref && "Invalid Parameter");
    HX_ASSERT(!(CHXString(szSubPref)).IsEmpty() && "Invalid Parameter");
    REQUIRE_RETURN_QUIET(szSubPref, HXR_INVALID_PARAMETER);
    if (!m_strCurrentPrefKey.IsEmpty())
    {
        // Only if the current pref has some value, then a separator can be added
        m_strCurrentPrefKey += HXPreferencesUtils::czPrefKeySeparator;
    }
    m_strCurrentPrefKey += szSubPref;
    return HXR_OK;
}

STDMETHODIMP
CHXPreferencesEnumerator::EndSubPref(THIS)
{
    HX_ASSERT(!m_strCurrentPrefKey.IsEmpty() && "A matching valid BeginSubPref was not called");
    INT32 nLastOccurence = m_strCurrentPrefKey.ReverseFind(HXPreferencesUtils::czPrefKeySeparator);
    m_strCurrentPrefKey = (nLastOccurence >= 0) ? m_strCurrentPrefKey.Left(nLastOccurence) : CHXString("");
    return HXR_OK;
}

STDMETHODIMP
CHXPreferencesEnumerator::GetPrefKey(THIS_ UINT32 nIndex, REF(IHXBuffer*) pBuffer)
{
    HX_ASSERT(m_pPrefs && "For some reason the prefs have not been created");
    if (m_pPrefs)
    {
        return m_pPrefs->GetPrefNoPrefix(m_strCurrentPrefKey, nIndex, pBuffer);
    }
    return HXR_FAIL;
}

STDMETHODIMP
CHXPreferencesEnumerator::ReadPref(THIS_ const char* pPrefKey, IHXBuffer*& pBuffer)
{
    HX_ASSERT(m_pPrefs && "For some reason the prefs have not been created");
    if (m_pPrefs)
    {
        return m_pPrefs->ReadPrefNoPrefix(m_strCurrentPrefKey.IsEmpty() ? pPrefKey : (const char*)((m_strCurrentPrefKey + HXPreferencesUtils::czPrefKeySeparator) + pPrefKey), pBuffer);
    }
    return HXR_FAIL;
}

//Leave a CR/LF before EOF to prevent CVS from getting angry
