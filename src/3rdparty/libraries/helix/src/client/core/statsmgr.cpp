/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: statsmgr.cpp,v 1.10 2008/01/25 05:21:31 vkathuria Exp $
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

#include "hxtypes.h"
#include "hxcom.h"

#include "hlxclib/stdio.h"
#include "hlxclib/stdlib.h"
#include "hxcomm.h"
#include "hxengin.h"
#include "hxmon.h"
#include "hxclreg.h"
#include "chxpckts.h"
#include "hxslist.h"
#include "hxmap.h"
#include "statsmgr.h"
#include "hxstrutl.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif

StatsManager::StatsManager(HXClientRegistry*	pRegistry,
			   UINT32		ulRegistryID,
			   UINT32		ulRepeatedRegistryID)
{
    m_pStatsMap = new CHXMapLongToObj;
    
    HX_RESULT	    hr = HXR_OK;
    IHXBuffer*	    pBuffer = NULL;

    m_lRefCount = 0;

    if (!pRegistry)
    {
	hr = HXR_FAILED;
	goto cleanup;
    }

    m_pRegistry = pRegistry;
    m_pRegistry->AddRef();

    if (HXR_OK == m_pRegistry->GetPropName(ulRepeatedRegistryID, pBuffer))
    {
	m_ulOffset = pBuffer->GetSize();
    }
    HX_RELEASE(pBuffer);

    if (HXR_OK == m_pRegistry->GetPropName(ulRegistryID, pBuffer))
    {
	m_pRegistryName = new char[pBuffer->GetSize() + 1];
	strcpy(m_pRegistryName, (const char*)pBuffer->GetBuffer()); /* Flawfinder: ignore */
    }
    HX_RELEASE(pBuffer);

    m_ulRegistryID = ulRegistryID;
    m_ulRepeatedRegistryID = m_ulRepeatedRegistryID;

    m_pPropWatchList = new CHXSimpleList();

    if (HXR_OK != SetWatch(ulRepeatedRegistryID))
    {
	hr = HXR_UNEXPECTED;
	goto cleanup;
    }

cleanup:

    return;
}

StatsManager::~StatsManager()
{
    HX_VECTOR_DELETE(m_pRegistryName);
    HX_RELEASE(m_pRegistry);
    HX_DELETE(m_pStatsMap);
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//      HXRegistry::QueryInterface
//  Purpose:
//      Implement this to export the interfaces supported by your
//      object.
//
STDMETHODIMP
StatsManager::QueryInterface(REFIID riid, void** ppvObj)
{
    QInterfaceList qiList[] =
        {
            { GET_IIDHANDLE(IID_IHXPropWatchResponse), (IHXPropWatchResponse*)this },
            { GET_IIDHANDLE(IID_IUnknown), (IUnknown*)this },
        };
    
    return ::QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
}   

/////////////////////////////////////////////////////////////////////////
//  Method:
//      HXRegistry::AddRef
//  Purpose:
//      Everyone usually implements this the same... feel free to use
//      this implementation.
//
STDMETHODIMP_(ULONG32)
StatsManager::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}   

/////////////////////////////////////////////////////////////////////////
//  Method:
//      HXRegistry::Release
//  Purpose:
//      Everyone usually implements this the same... feel free to use
//      this implementation.
//
STDMETHODIMP_(ULONG32)
StatsManager::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }
    
    delete this;
    return 0;
}   

HX_RESULT
StatsManager::DoCleanup()
{
    HX_RESULT hr = HXR_OK;

    CHXSimpleList::Iterator i;
    CHXMapLongToObj::Iterator j;

    if (m_pPropWatchList)
    {
	PropWatchEntry*  pPropWatchEntry = NULL;
	
	i = m_pPropWatchList->Begin();
	for (; i != m_pPropWatchList->End(); ++i)
	{
	    pPropWatchEntry = (PropWatchEntry*)(*i);
	    
	    pPropWatchEntry->pPropWatch->ClearWatchById(pPropWatchEntry->ulPropID);

	    HX_RELEASE(pPropWatchEntry->pPropWatch);
	    HX_DELETE(pPropWatchEntry);
	}

	HX_DELETE(m_pPropWatchList);
    }

    StatsMapEntry* pEntry = NULL;

    j = m_pStatsMap->Begin();
    for (; j != m_pStatsMap->End(); ++j)
    {
	pEntry = (StatsMapEntry*)(*j);
	HX_DELETE(pEntry);
    }
    m_pStatsMap->RemoveAll();

    return hr;
}

HX_RESULT
StatsManager::Copy()
{
    HX_RESULT	    hr = HXR_OK;
    INT32	    lValue = 0;
    IHXBuffer*	    pName = NULL;
    IHXBuffer*	    pBuffer = NULL;
    StatsMapEntry*  pEntry = NULL;
    CHXMapLongToObj::Iterator i;

    i = m_pStatsMap->Begin();
    for (; i != m_pStatsMap->End(); ++i)
    {
	pEntry = (StatsMapEntry*)(*i);

	switch(pEntry->type)
	{
	    case PT_INTEGER:
		m_pRegistry->GetIntById(pEntry->ulFrom, lValue);		    
		m_pRegistry->SetIntById(pEntry->ulTo, lValue);
		break;
	    case PT_INTREF:		
		m_pRegistry->GetIntById(pEntry->ulFrom, lValue);

		m_pRegistry->GetPropName(pEntry->ulTo, pName);
		m_pRegistry->AddIntRef((const char*)pName->GetBuffer(), &lValue);
		HX_RELEASE(pName);
		break;
	    case PT_STRING:
		if (HXR_OK == m_pRegistry->GetStrById(pEntry->ulFrom, pBuffer) &&
		    pBuffer)
		{
		    m_pRegistry->SetStrById(pEntry->ulTo, pBuffer);
		}
		HX_RELEASE(pBuffer);
		break;
	    case PT_BUFFER:
		if (HXR_OK == m_pRegistry->GetBufById(pEntry->ulFrom, pBuffer) &&
		    pBuffer)
		{
		    m_pRegistry->SetBufById(pEntry->ulTo, pBuffer);
		}
		HX_RELEASE(pBuffer);
		break;
	    default:
		break;
	}
    }

    return hr;
}
    
HX_RESULT
StatsManager::SetWatch(UINT32 ulRegistryID)
{
    HX_RESULT	    hr = HXR_OK;
    PropWatchEntry* pPropWatchEntry = NULL;
    IHXPropWatch*  pPropWatch = NULL;

    if (HXR_OK != m_pRegistry->CreatePropWatch(pPropWatch))
    {
	hr = HXR_FAILED;
	goto cleanup;
    }

    if (HXR_OK != pPropWatch->Init((IHXPropWatchResponse*)this))
    {
	hr = HXR_FAILED;
	goto cleanup;
    }

    pPropWatch->SetWatchById(ulRegistryID);

    pPropWatchEntry = new PropWatchEntry;
    pPropWatchEntry->ulPropID = ulRegistryID;
    pPropWatchEntry->pPropWatch = pPropWatch;

    m_pPropWatchList->AddTail(pPropWatchEntry);

cleanup:

    if (HXR_OK != hr)
    {	
	HX_RELEASE(pPropWatch);
	HX_DELETE(pPropWatchEntry);
    }

    return hr;
}

STDMETHODIMP
StatsManager::AddedProp(const UINT32		ulHash,
			const HXPropType	type,
			const UINT32		ulParentHash)
{
    HX_RESULT	hr = HXR_OK;
    INT32	lValue = 0;
    UINT32	ulRegistryID = 0;
    char	szRegKeyName[MAX_DISPLAY_NAME] = {0}; /* Flawfinder: ignore */
    HXPropType	theType = PT_UNKNOWN;
    IHXBuffer*	pBuffer = NULL;

    if (HXR_OK == m_pRegistry->GetPropName(ulHash, pBuffer))
    {
	// apparently the type returned from IHXPropWatchReponse is always
	// PT_UNKNOWN !!
	theType = m_pRegistry->GetTypeById(ulHash);

	// exclude leading "Repeat.*."
	SafeSprintf(szRegKeyName, MAX_DISPLAY_NAME, "%s.%s", m_pRegistryName, (const char*)(pBuffer->GetBuffer() + m_ulOffset));
	HX_RELEASE(pBuffer);

	ulRegistryID = m_pRegistry->GetId(szRegKeyName);
	if (!ulRegistryID)
	{
	    switch (theType)
	    {
	    case PT_INTEGER:
		m_pRegistry->GetIntById(ulHash, lValue);		    
		ulRegistryID = m_pRegistry->AddInt(szRegKeyName, lValue);
		break;
	    case PT_INTREF:
		m_pRegistry->GetIntById(ulHash, lValue);
		ulRegistryID = m_pRegistry->AddIntRef(szRegKeyName, &lValue);
		break;
	    case PT_STRING:
		m_pRegistry->GetStrById(ulHash, pBuffer);
		ulRegistryID = m_pRegistry->AddStr(szRegKeyName, pBuffer);
		HX_RELEASE(pBuffer);
		break;
	    case PT_BUFFER:
		m_pRegistry->GetBufById(ulHash, pBuffer);
		ulRegistryID = m_pRegistry->AddBuf(szRegKeyName, pBuffer);
		HX_RELEASE(pBuffer);
		break;
	    case PT_COMPOSITE:
		hr = m_pRegistry->AddComp(szRegKeyName);
		break;
	    default:
		break;
	    }
	}

	if (PT_COMPOSITE != theType)
	{
	    StatsMapEntry* pEntry = new StatsMapEntry;
	    pEntry->ulFrom = ulHash;
	    pEntry->ulTo = ulRegistryID;
	    pEntry->type = theType;

	    m_pStatsMap->SetAt((INT32)ulHash, pEntry);
	}
	else
	{
	    hr = SetWatch(ulHash);
	}
    }

    return hr;
}

STDMETHODIMP
StatsManager::ModifiedProp(const UINT32		ulHash,
			   const HXPropType	type,
			   const UINT32		ulParentHash)
{
    return HXR_OK;
}

STDMETHODIMP
StatsManager::DeletedProp(const UINT32		ulHash,
			  const UINT32		ulParentHash)
{
    HX_RESULT	hr = HXR_OK;
    UINT32	ulRegistryID = 0;
    char	szRegKeyName[MAX_DISPLAY_NAME] = {0}; /* Flawfinder: ignore */
    IHXBuffer*	pBuffer = NULL;

    if (HXR_OK == m_pRegistry->GetPropName(ulHash, pBuffer))
    {
	// exclude leading "Repeat.*."
	SafeSprintf(szRegKeyName, MAX_DISPLAY_NAME, "%s.%s", m_pRegistryName, (const char*)(pBuffer->GetBuffer() + m_ulOffset));
	HX_RELEASE(pBuffer);

	ulRegistryID = m_pRegistry->GetId(szRegKeyName);
	if (ulRegistryID)
	{
	    m_pRegistry->DeleteById(ulRegistryID);

	    StatsMapEntry* pEntry = NULL;
	    if (m_pStatsMap->Lookup((INT32)ulHash, (void*&)pEntry))
	    {
		HX_DELETE(pEntry);
		m_pStatsMap->RemoveKey((INT32)ulHash);
	    }
	}
    }
    
    return HXR_OK;
}

HX_RESULT
StatsManager::UpdateRegistry(UINT32 ulRegistryID)
{
    IHXBuffer* pBuffer = NULL;

    if (HXR_OK == m_pRegistry->GetPropName(ulRegistryID, pBuffer))
    {
	HX_VECTOR_DELETE(m_pRegistryName);

	m_pRegistryName = new char[pBuffer->GetSize() + 1];
	strcpy(m_pRegistryName, (const char*)pBuffer->GetBuffer()); /* Flawfinder: ignore */
    }
    HX_RELEASE(pBuffer);

    m_ulRegistryID = ulRegistryID;

    return HXR_OK;
}
