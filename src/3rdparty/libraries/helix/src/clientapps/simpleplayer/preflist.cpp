/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: preflist.cpp,v 1.1 2004/11/12 02:21:55 acolwell Exp $
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
#include "preflist.h"

#include "hxstring.h"
#include "hxccf.h"
#include "hxprefs.h"
#include "ihxpckts.h"

class CHXPrefInfo
{
public:
    CHXPrefInfo(const char* pKey, const char* pValue);
    ~CHXPrefInfo();
    
    const char* Key() const { return m_key;}
    const char* Value() const { return m_value;}

private:
    CHXString m_key;
    CHXString m_value;
};

CHXPrefInfo::CHXPrefInfo(const char* pKey, const char* pValue) :
    m_key(pKey),
    m_value(pValue)
{}

CHXPrefInfo::~CHXPrefInfo()
{}

CHXPrefList::CHXPrefList()
{}

CHXPrefList::~CHXPrefList()
{
    Clear();
}

void CHXPrefList::Add(const char* pKey, const char* pValue)
{
    CHXPrefInfo* pInfo = new CHXPrefInfo(pKey, pValue);

    if (pInfo)
    {
        if (!m_prefInfo.AddTail(pInfo))
        {
            // We failed to insert the preference.
            HX_DELETE(pInfo);
        }
    }
}

void CHXPrefList::Clear()
{
    while(!m_prefInfo.IsEmpty())
    {
        CHXPrefInfo* pInfo = (CHXPrefInfo*)m_prefInfo.RemoveHead();
        HX_DELETE(pInfo);
    }
}

void CHXPrefList::SetPreferences(IUnknown* pContext)
{
    IHXPreferences* pPrefs = NULL;
    IHXCommonClassFactory* pCCF = NULL;

    if (pContext &&
        (HXR_OK == pContext->QueryInterface(IID_IHXPreferences,
                                            (void**)&pPrefs)) &&
        (HXR_OK == pContext->QueryInterface(IID_IHXCommonClassFactory,
                                            (void**)&pCCF)))
    {
        CHXSimpleList::Iterator itr = m_prefInfo.Begin();
        
        for(; itr != m_prefInfo.End(); ++itr)
        {
            CHXPrefInfo* pInfo = (CHXPrefInfo*)(*itr);
            
            IHXBuffer* pBuf = NULL;
            
            if ((HXR_OK == pCCF->CreateInstance(CLSID_IHXBuffer,
                                                (void**)&pBuf)) &&
                (HXR_OK == pBuf->Set((const unsigned char*)pInfo->Value(),
                                     strlen(pInfo->Value()))))
            {
                pPrefs->WritePref(pInfo->Key(), pBuf);
            }

            HX_RELEASE(pBuf);
        }
    }

    HX_RELEASE(pPrefs);
    HX_RELEASE(pCCF);
}
