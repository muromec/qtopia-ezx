/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: testplgnhdlr.cpp,v 1.2 2007/07/06 20:51:33 jfinnecy Exp $
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

#include "testplgnhdlr.h"

CHXTestPluginHdlr::CHXTestPluginHdlr() :
    m_lRefCount(0)
{}

CHXTestPluginHdlr::~CHXTestPluginHdlr()
{
    clearList();
}

HX_RESULT 
CHXTestPluginHdlr::SetMimetypes(const char* pMimetypes)
{
    HX_RESULT res = HXR_INVALID_PARAMETER;
    
    if (pMimetypes && *pMimetypes)
    {
        clearList();

        const char* pCur = pMimetypes;
        const char* pStart = pCur;

        res = HXR_OK;
        while((HXR_OK == res) && *pCur)
        {
            pStart = pCur;

            for (;*pCur && (*pCur != ','); pCur++)
                ;

            if (*pCur == ',')
            {
                res = addString(pStart, pCur - pStart);

                pCur++; // skip ','

                if (!*pCur)
                {
                    res = HXR_UNEXPECTED;
                }
            }
        }
        
        if (HXR_OK == res)
        {
            res = addString(pStart, pCur - pStart);
        }
        else
        {
            clearList();
        }
    }

    return res;
}

/*
 *  IUnknown methods
 */
STDMETHODIMP
CHXTestPluginHdlr::QueryInterface(THIS_
                                  REFIID riid,
                                  void** ppvObj)
{
    HX_RESULT res = HXR_NOINTERFACE;
    *ppvObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)this;
        res = HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXPlugin2Handler))
    {
        AddRef();
        *ppvObj = (IHXPlugin2Handler*)this;
        res = HXR_OK;
    }

    return res;
}

STDMETHODIMP_(ULONG32) CHXTestPluginHdlr::AddRef(THIS)
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32) CHXTestPluginHdlr::Release(THIS)
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

/*
 *	IHXPlugin2Handler Methods
 */

STDMETHODIMP CHXTestPluginHdlr::Init(THIS_ IUnknown* pContext)
{
    HX_RESULT res = HXR_INVALID_PARAMETER;

    if (pContext)
    {
        res = HXR_OK;
    }

    return res;
}
     
STDMETHODIMP_(ULONG32) CHXTestPluginHdlr::GetNumOfPlugins2(THIS)
{
    return m_list.GetCount();
}
    
STDMETHODIMP 
CHXTestPluginHdlr::GetPluginInfo(THIS_ 
                                 UINT32 unIndex, 
                                 REF(IHXValues*) /*OUT*/ Values)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP 
CHXTestPluginHdlr::FlushCache(THIS)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP
CHXTestPluginHdlr::SetCacheSize(THIS_ ULONG32 nSizeKB)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP
CHXTestPluginHdlr::GetInstance(THIS_ UINT32 index, REF(IUnknown*) pUnknown)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP
CHXTestPluginHdlr::FindIndexUsingValues(THIS_ IHXValues*, 
                                        REF(UINT32) unIndex)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP
CHXTestPluginHdlr::FindPluginUsingValues(THIS_ IHXValues*, 
                                         REF(IUnknown*) pUnk)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP
CHXTestPluginHdlr::FindIndexUsingStrings(THIS_ char* PropName1, 
                                         char* PropVal1, 
                                         char* PropName2, 
                                         char* PropVal2, 
                                         char* PropName3, 
                                         char* PropVal3, 
                                         REF(UINT32) unIndex)
{
    HX_RESULT res = HXR_INVALID_PARAMETER;

    if (PropName1 && PropVal1 && 
        !PropName2 && !PropVal2 && 
        !PropName3 && !PropVal3 &&
        !strcmp(PropName1, PLUGIN_RENDERER_MIME) &&
        strlen(PropVal1))
    {
        CHXSimpleList::Iterator itr = m_list.Begin();
        
        UINT32 uTmp = 0;
        for (; (HXR_OK != res) && (itr != m_list.End()); ++itr)
        {
            CHXString* pStr = (CHXString*)*itr;

            if (!pStr->CompareNoCase(PropVal1))
            {
                unIndex = uTmp;
                res = HXR_OK;
            }

            uTmp++;
        }
    }

    return res;
}


STDMETHODIMP
CHXTestPluginHdlr::FindPluginUsingStrings(THIS_ char* PropName1, 
                                          char* PropVal1, 
                                          char* PropName2, 
                                          char* PropVal2, 
                                          char* PropName3, 
                                          char* PropVal3, 
                                          REF(IUnknown*) pUnk)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP
CHXTestPluginHdlr::FindImplementationFromClassID(THIS_ 
                                                 REFGUID GUIDClassID, 
                                                 REF(IUnknown*) pIUnknown)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP
CHXTestPluginHdlr::Close(THIS)
{
    return HXR_OK;
}

STDMETHODIMP
CHXTestPluginHdlr::SetRequiredPlugins(THIS_ const char** ppszRequiredPlugins)
{
    return HXR_NOTIMPL;
}

void CHXTestPluginHdlr::clearList()
{
    CHXSimpleList::Iterator itr = m_list.Begin();

    for (; itr != m_list.End(); ++itr)
    {
        CHXString* pStr = (CHXString*)*itr;
        HX_DELETE(pStr);
    }
    m_list.RemoveAll();
}

HX_RESULT CHXTestPluginHdlr::addString(const char* pStr, UINT32 uLength)
{
    HX_RESULT res = HXR_INVALID_PARAMETER;

    if (pStr && uLength)
    {
        CHXString* pString = new CHXString(pStr, uLength);

        if (pString && m_list.AddTail(pString))
        {
            res = HXR_OK;
        }
        else
        {
            res = HXR_OUTOFMEMORY;
            HX_DELETE(pString);
        }
    }

    return res;
}
