/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: upgrdcol.cpp,v 1.10 2008/02/19 10:22:58 vkathuria Exp $
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
#include "ihxpckts.h"
#include "pckunpck.h"
#include "upgrdcol.h"
#include "hlxclib/stdlib.h"
#include "hxordval.h"
#include "hxbuffer.h"
#include "hxstrutl.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif

#define UPGRADE_NAME_SIZE   128

struct HXUpgradeInfo
{
    HXUpgradeType m_UpgradeType;
    UINT32 m_MajorVersion;
    UINT32 m_MinorVersion;
    char m_Name[UPGRADE_NAME_SIZE]; /* Flawfinder: ignore */
};


const char* const z_szURLTranslationChars = ";/?:@=&\x7F \"<>#%{}|\\^~[]\'";

HXUpgradeCollection::HXUpgradeCollection(IUnknown* pContext):	
    m_lRefCount(0),	
    m_pComponents(NULL),
    m_pURLParseElements(NULL),
    m_pContext(pContext)
{
    HX_ADDREF(m_pContext);
}

HXUpgradeCollection::~HXUpgradeCollection(void)
{
    RemoveAll();
    HX_RELEASE(m_pContext);
}

/************************************************************************
 *  Method:
 *    IUnknown::QueryInterface
 */
STDMETHODIMP HXUpgradeCollection::QueryInterface(REFIID riid,
void** ppvObj)
{
    QInterfaceList qiList[] =
        {
            { GET_IIDHANDLE(IID_IHXUpgradeCollection), (IHXUpgradeCollection*)this },
            { GET_IIDHANDLE(IID_IHXUpgradeCollection2), (IHXUpgradeCollection2*)this },
            { GET_IIDHANDLE(IID_IUnknown), (IUnknown*)(IHXUpgradeCollection*)this },
        };
    
    return ::QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
}

/************************************************************************
 *  Method:
 *    IUnknown::AddRef
 */
STDMETHODIMP_(ULONG32) HXUpgradeCollection::AddRef(void)
{
    return InterlockedIncrement(&m_lRefCount);
}

/************************************************************************
 *  Method:
 *    IUnknown::Release
 */
STDMETHODIMP_(ULONG32) HXUpgradeCollection::Release(void)
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

/************************************************************************
 *  Method:
 *    IHXUpgradeCollection::Add
 */
STDMETHODIMP_(UINT32) HXUpgradeCollection::Add(HXUpgradeType upgradeType, 
IHXBuffer* pPluginId, UINT32 majorVersion, UINT32 minorVersion)
{
    if (!m_pComponents)
    {
	m_pComponents = new CHXPtrArray;
	if (!m_pComponents)
	{
	    // XXXNH: should we return this? 0 would indicate success...
	    return (UINT32)-1;
	}
    }

    HXUpgradeInfo* pInfo = new HXUpgradeInfo;
    pInfo->m_UpgradeType = upgradeType;
    pInfo->m_MajorVersion = majorVersion;
    pInfo->m_MinorVersion = minorVersion;
    pInfo->m_Name[0] = 0;
    if (pPluginId)
    	SafeStrCpy(&(pInfo->m_Name[0]), (const char*) pPluginId->GetBuffer(), UPGRADE_NAME_SIZE);

    return(m_pComponents->Add(pInfo));
}


/************************************************************************
 *	Method:
 *		IHXUpgradeCollection::Remove
 */
STDMETHODIMP HXUpgradeCollection::Remove(UINT32 index)
{
    if (m_pComponents)
    {
	if ((UINT32)m_pComponents->GetSize() > index)
	{
	    HXUpgradeInfo* pInfo = (HXUpgradeInfo*)(*m_pComponents)[(int)index];
	    m_pComponents->RemoveAt((int)index);
	    delete pInfo;

	    return(HXR_OK);
	}
    }

    return(HXR_FAIL);
}

/************************************************************************
 *	Method
 *		IHXUpgradeCollection::RemoveAll
 */
STDMETHODIMP HXUpgradeCollection::RemoveAll(void)
{
    if (m_pComponents)
    {
	UINT32 size = m_pComponents->GetSize();

	for(UINT32 i = 0;i < size;i++)
    	    delete (*m_pComponents)[(int)i];

	m_pComponents->RemoveAll();
	HX_DELETE(m_pComponents);
    }

    HX_RELEASE(m_pURLParseElements);

    return(HXR_OK);
}

/************************************************************************
 *	Method:
 *		IHXUpgradeCollection::GetCount
 */
STDMETHODIMP_(UINT32) HXUpgradeCollection::GetCount(void)
{
    return m_pComponents ? (m_pComponents->GetSize()) : 0;
}

/************************************************************************
 *	Method:
 *		IHXUpgradeCollection::GetAt
 */
STDMETHODIMP HXUpgradeCollection::GetAt(UINT32 index, 
HXUpgradeType& upgradeType, IHXBuffer* pPluginId, UINT32& majorVersion, 
UINT32&	minorVersion)
{
    if (m_pComponents)
    {
	if ((UINT32)m_pComponents->GetSize() > index && pPluginId)
	{
	    HXUpgradeInfo* pInfo = (HXUpgradeInfo*)(*m_pComponents)[(int)index];
	    upgradeType = pInfo->m_UpgradeType;
	    majorVersion = pInfo->m_MajorVersion;
	    minorVersion = pInfo->m_MinorVersion;
	    pPluginId->Set((const UCHAR *)&(pInfo->m_Name[0]), ::strlen(&(pInfo->m_Name[0])) + 1);

	    return(HXR_OK);
	}
    }

    return(HXR_FAIL);
}

/************************************************************************
 *	Method:
 *		IHXUpgradeCollection::AddURLParseElement
 *	Purpose:
 *		Adds name-value pair for RUP URL parsing:
 *		URL-encoded values substitute names.
 *
 */
STDMETHODIMP
HXUpgradeCollection::AddURLParseElement(const char* pName, const char* pValue)
{
    if(!m_pURLParseElements)
    {
	m_pURLParseElements = (IHXValues*)new CHXOrderedValues;
	if(m_pURLParseElements)
	    m_pURLParseElements->AddRef();
    }

    if(!m_pURLParseElements)
	return HXR_FAILED;

    // We have to URL encode the Value as it's going to be used in RUP URLs.
    IHXBuffer* pURLEncValue = NULL;
    if (HXR_OK == CreateAndSetBufferCCF(pURLEncValue, (Byte*)pValue, 
				        strlen(pValue) + 1, m_pContext))
    {
	/*
	// Maximum possible size for URL encoded pValue.
	pURLEncValue->SetSize(strlen(pValue) * 3 + 1);
	char* pszBuffer = (char*)pURLEncValue->GetBuffer();
	pszBuffer[0] = 0;

	for(UINT16 nIndex = 0; nIndex < strlen(pValue); nIndex++)
	{
	    Byte nChar = pValue[nIndex];
	    if(nChar <= 0x1F || nChar >= 0x80 && nChar <= 0xFF ||
	       strchr(z_szURLTranslationChars, nChar))
	    {
		wsprintf(pszBuffer + strlen(pszBuffer), "%%%02X", nChar);
	    }
	    else
	    {
		wsprintf(pszBuffer + strlen(pszBuffer), "%c", nChar);
	    }
	}

	// Reset the size
	pURLEncValue->SetSize(strlen(pszBuffer) + 1);
	*/

	m_pURLParseElements->SetPropertyCString(pName, pURLEncValue);
	HX_RELEASE(pURLEncValue);
    }

    return HXR_OK;
}

/************************************************************************
 *	Method:
 *		IHXUpgradeCollection::GetURLParseElements
 *	Purpose:
 *		Gets name-value pair for RUP URL parsing.
 *
 */
STDMETHODIMP 
HXUpgradeCollection::GetURLParseElements(REF(IHXValues*) pURLParseElements)
{
    pURLParseElements = m_pURLParseElements;
    if(pURLParseElements)
    {
	pURLParseElements->AddRef();
	return HXR_OK;
    }

    return HXR_FAILED;
}
