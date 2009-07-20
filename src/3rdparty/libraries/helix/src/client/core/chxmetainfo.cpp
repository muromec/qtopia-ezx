/* ***** BEGIN LICENSE BLOCK *****
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
#include "hxcomm.h"
#include "hxengin.h"
#include "hxcore.h"
#include "hxmap.h"
#include "hxstrutl.h"
#include "pckunpck.h"
#include "hxslist.h"
#include "chxmetainfo.h"
#include "buffutil.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE
static const char HX_THIS_FILE[] = __FILE__;
#endif

const char* const g_pMetaProperties[] =
{
#ifdef HELIX_FEATURE_DRM
    "LicenseInfo",
#endif
    "Title",
    "Author",
    "Copyright",
    "Abstract",
    "Keywords",
    "Description"
};

CHXMetaInfo::CHXMetaInfo(IUnknown* pContext) 
            : m_lRefCount(0)
            , m_ulKnownProperties(0)
            , m_pContext(NULL)
            , m_pPropertyMap(NULL)
            , m_pSinkList(NULL)
{
    m_pContext = pContext;
    HX_ADDREF(m_pContext);

    m_ulKnownProperties = sizeof(g_pMetaProperties) / sizeof(*g_pMetaProperties);
}

CHXMetaInfo::~CHXMetaInfo()
{
    Close();
}

void CHXMetaInfo::Close()
{
    if (m_pSinkList)
    {
        // somebody forget to call RemoveAdviseSink()
        HX_ASSERT(!m_pSinkList->GetCount());

	CHXSimpleList::Iterator it = m_pSinkList->Begin();
	for (; it != m_pSinkList->End(); ++it)
	{
            IHXMediaPropertyAdviseSink* pSink = (IHXMediaPropertyAdviseSink*)(*it);
            HX_RELEASE(pSink);
        }

        HX_DELETE(m_pSinkList);
    }

    ResetMetaInfo();
    HX_DELETE(m_pPropertyMap);    

    HX_RELEASE(m_pContext);
}

STDMETHODIMP CHXMetaInfo::QueryInterface(REFIID riid, void** ppvObj)
{
    QInterfaceList qiList[] =
        {
            { GET_IIDHANDLE(IID_IHXMediaProperty), (IHXMediaProperty*)this },
        };
    
    return ::QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
}

STDMETHODIMP_(ULONG32) CHXMetaInfo::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32) CHXMetaInfo::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
	return m_lRefCount;
    }

    delete this;
    return 0;
}

// IHXMediaProperty
STDMETHODIMP
CHXMetaInfo::GetPropertyCount(REF(UINT32) ulCount)
{
    ulCount = m_pPropertyMap ? m_pPropertyMap->GetCount() : 0;
    return HXR_OK;   
}

STDMETHODIMP
CHXMetaInfo::GetPropertyNameAt(UINT32 ulIndex, REF(IHXBuffer*) pName)
{
    HX_RESULT   res = HXR_FAIL;
    CHXMapStringToOb::Iterator it;
    UINT16 i = 0;

    pName = NULL;
    const char* pszName = NULL;

    if (!m_pPropertyMap)
    {
        goto exit;
    }

    if (ulIndex >= (UINT32)m_pPropertyMap->GetCount())
    {
        res = HXR_INVALID_PARAMETER;
        goto exit;
    }

    it = m_pPropertyMap->Begin();
    for (i = 0; it != m_pPropertyMap->End(); ++it, i++)
    {
        if (i == ulIndex)
        {
            pszName = it.get_key();
            break;
        }
    }
        
exit:

    if (pszName)
    {
        res = CreateStringBufferCCF(pName, pszName, m_pContext);
    }

    return res;
}

STDMETHODIMP
CHXMetaInfo::GetPropertyStringValue(const char* pszName, REF(IHXBuffer*) pValue)
{
    HX_RESULT   res = HXR_FAIL;
    void*       pPropValue = NULL;

    pValue = NULL;

    if (!m_pPropertyMap || 0 == m_pPropertyMap->GetCount())
    {
        goto exit;
    }
    
    if (m_pPropertyMap->Lookup(pszName, (void*&)pPropValue))
    {
        pValue = (IHXBuffer*)pPropValue;
    }

exit:

    if (pValue)
    {
        pValue->AddRef();
        res = HXR_OK;
    }

    return res;
}

STDMETHODIMP
CHXMetaInfo::IsPropertyReadOnly(const char* pszName, REF(HXBOOL) bReadOnly)
{
    HX_RESULT   res = HXR_FAIL;
    HXBOOL      bValidName = FALSE;
    UINT32 i = 0;

    if (!pszName)
    {
        res = HXR_INVALID_PARAMETER;
        goto exit;
    }

    bReadOnly = TRUE;

    for (i = 0; i < m_ulKnownProperties; i++)
    {
        if (strcasecmp(g_pMetaProperties[i], pszName) == 0)
        {
            bValidName = bReadOnly = TRUE;
            break;
        }
    }

exit:

    if (bValidName)
    {
        res = HXR_OK;
    }

    return res;
}

STDMETHODIMP
CHXMetaInfo::SetPropertyString(const char* pszName, IHXBuffer* pValue)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP
CHXMetaInfo::AddSink(IHXMediaPropertyAdviseSink* pSink)
{
    HX_RESULT   res = HXR_FAIL;

    if (!pSink)
    {
        res = HXR_INVALID_PARAMETER;
        goto exit;
    }

    if (!m_pSinkList)
    {
        m_pSinkList = new CHXSimpleList();
    }

    if (!m_pSinkList->Find(pSink))
    {
        m_pSinkList->AddTail(pSink);
        pSink->AddRef();        
    }

    res = HXR_OK;

exit:

    return res;
}

STDMETHODIMP
CHXMetaInfo::RemoveSink(IHXMediaPropertyAdviseSink* pSink)
{
    HX_RESULT       res = HXR_FAIL;
    LISTPOSITION    pos = 0;

    if (!pSink)
    {
        res = HXR_INVALID_PARAMETER;
        goto exit;
    }

    if (m_pSinkList && m_pSinkList->GetCount())
    {
        pos = m_pSinkList->Find(pSink);
        if (pos)
        {
            m_pSinkList->RemoveAt(pos);
            pSink->Release();
        }
    }

    res = HXR_OK;

exit:

    return res;
}

HX_RESULT
CHXMetaInfo::UpdateMetaInfo(IHXValues* pValues)
{
    HX_RESULT   res = HXR_FAIL;
    HXBOOL      bUpdate = FALSE;
    IHXBuffer*  pTmpPropValue = NULL;
    IHXBuffer*  pNewPropValue = NULL;
    IHXBuffer*  pOldPropValue = NULL;
    UINT32 i = 0;

    if (!pValues)
    {
        res = HXR_INVALID_PARAMETER;
        goto exit;
    }

    for (i = 0; i < m_ulKnownProperties; i++)
    {
        if (SUCCEEDED(pValues->GetPropertyBuffer(g_pMetaProperties[i], pNewPropValue)))
        {
            bUpdate = TRUE;
            HX_RELEASE(pNewPropValue);
            break;
        }
    }

    if (bUpdate)
    {
        if (!m_pPropertyMap)
        {
            m_pPropertyMap = new CHXMapStringToOb;
        }

        if (m_pPropertyMap)
        {
            for (UINT32 i = 0; i < m_ulKnownProperties; i++)
            {
		// make sure all MetaInfo GetPropertyBuffer are NULL terminated
                if (SUCCEEDED(pValues->GetPropertyBuffer(g_pMetaProperties[i], pTmpPropValue)) && 
		    SUCCEEDED(PXUtilities::GetNullTerminatedBuffer(m_pContext, pTmpPropValue, pNewPropValue)))
                {
                    if (m_pPropertyMap->Lookup(g_pMetaProperties[i], (void*&)pOldPropValue))
                    {
                        m_pPropertyMap->SetAt(g_pMetaProperties[i], pNewPropValue);
                        MetaInfoChanged(g_pMetaProperties[i], pOldPropValue, pNewPropValue);
                        HX_RELEASE(pOldPropValue);
                    }
                    else
                    {
                        m_pPropertyMap->SetAt(g_pMetaProperties[i], pNewPropValue);                               
                        MetaInfoAdded(g_pMetaProperties[i], pNewPropValue);
                    }
		    HX_RELEASE(pTmpPropValue);
                }
                else if (m_pPropertyMap->Lookup(g_pMetaProperties[i], (void*&)pOldPropValue))
                {
                    m_pPropertyMap->Remove(g_pMetaProperties[i]);
                    MetaInfoRemoved(g_pMetaProperties[i], pOldPropValue);
                    HX_RELEASE(pOldPropValue);
                }
            }
        }
    }

    res = HXR_OK;

exit:

    return res;
}

void
CHXMetaInfo::ResetMetaInfo()
{
    if (!m_pPropertyMap || !m_pPropertyMap->GetCount())
    {
        return;
    }

    CHXMapStringToOb::Iterator it = m_pPropertyMap->Begin();
    for (; it != m_pPropertyMap->End(); ++it)
    {
        IHXBuffer* pPropValue = (IHXBuffer*)(*it);
        HX_RELEASE(pPropValue);
    }
    m_pPropertyMap->RemoveAll();    
}

void
CHXMetaInfo::MetaInfoAdded(const char* pszName, IHXBuffer* pValue)
{
    if (m_pSinkList)
    {
	CHXSimpleList::Iterator it = m_pSinkList->Begin();
	for (; it != m_pSinkList->End(); ++it)
	{
            IHXMediaPropertyAdviseSink* pSink = (IHXMediaPropertyAdviseSink*)(*it);
            pSink->OnMediaPropertyStringAdded(pszName, pValue);
        }
    }
}

void        
CHXMetaInfo::MetaInfoChanged(const char* pszName, IHXBuffer* pOldValue, IHXBuffer* pNewValue)
{
    if (m_pSinkList)
    {
	CHXSimpleList::Iterator it = m_pSinkList->Begin();
	for (; it != m_pSinkList->End(); ++it)
	{
            IHXMediaPropertyAdviseSink* pSink = (IHXMediaPropertyAdviseSink*)(*it);
            pSink->OnMediaPropertyStringChanged(pszName, pOldValue, pNewValue);
        }
    }
}

void
CHXMetaInfo::MetaInfoRemoved(const char* pszName, IHXBuffer* pValue)
{
    if (m_pSinkList)
    {
	CHXSimpleList::Iterator it = m_pSinkList->Begin();
	for (; it != m_pSinkList->End(); ++it)
	{
            IHXMediaPropertyAdviseSink* pSink = (IHXMediaPropertyAdviseSink*)(*it);
            pSink->OnMediaPropertyStringRemoved(pszName, pValue);
        }
    }
}
