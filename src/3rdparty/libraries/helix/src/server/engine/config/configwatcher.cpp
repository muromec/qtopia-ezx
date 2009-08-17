/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: configwatcher.cpp,v 1.6 2003/09/04 22:35:34 dcollins Exp $ 
 *   
 * Portions Copyright (c) 1995-2003 RealNetworks, Inc. All Rights Reserved.  
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
/*
 *  Class to manage per-datatype license requests
 */

#include "hxtypes.h"
#include "hxcom.h"
#include "hxresult.h"
#include "hxmon.h"
#include "hxassert.h"
#include "debug.h"
#include "hxstrutl.h"
#include "configwatcher.h"

ConfigWatcher::ConfigWatcher(IHXRegistry* pRegistry)
    : m_lRefCount(0)
    , m_pRegistry(pRegistry)
    , m_pActiveRegistry(NULL)
    , m_pResponse(NULL)
    , m_pPropWatch(NULL) 
{

#ifdef _SCRVERBOSE
    debug_level() = 0xf;
#endif
    if (m_pRegistry)
    {
	m_pRegistry->AddRef();

	m_pRegistry->QueryInterface(IID_IHXActiveRegistry, 
		(void**)&m_pActiveRegistry);

    }
}

ConfigWatcher::~ConfigWatcher()
{
    Cleanup();
    HX_RELEASE(m_pRegistry);
    HX_RELEASE(m_pActiveRegistry);
    HX_RELEASE(m_pPropWatch);
}

ConfigWatcherEntry::ConfigWatcherEntry(IHXPropWatchResponse* pResponse, 
	ULONG32 regID, 
	ULONG32 parentID) 
    : m_lRefCount(0)
    , m_pResponse(pResponse)
    , m_pUserResponse(NULL)
    , m_result(HXR_OK)
    , m_type(PT_UNKNOWN)
    , m_pName(NULL)
    , m_pParentName(NULL)
    , m_intVal(0)
    , m_pBufferVal(0)
    , m_bDeleteRequest(FALSE)
    , m_ulNumInfo(0)
    , m_regID(regID)
    , m_parentID(parentID)
    , m_ppInfo(NULL) 
{
    if (m_pResponse)
    {
	m_pResponse->AddRef();
    }
}

ConfigWatcherEntry::~ConfigWatcherEntry()
{
    if (m_pUserResponse)
    {
	//fprintf(stderr, "Send %s response on DELETE!\n", m_pName);
	SendResponse();
    }
    HX_RELEASE(m_pUserResponse);
    HX_RELEASE(m_pResponse);
    HX_RELEASE(m_pBufferVal);
    if (m_ppInfo)
    {
	for (UINT32 i=0; i<m_ulNumInfo; i++)
	    HX_RELEASE(m_ppInfo[i]);
    }
    HX_VECTOR_DELETE(m_pName);
    HX_VECTOR_DELETE(m_pParentName);
}

void ConfigWatcher::Cleanup()
{
    for (CHXMapLongToObj::Iterator i = m_regIDMap.Begin();
	 i != m_regIDMap.End();
	 ++i)
    {
	Unmanage(i.get_key());
    }
    m_regIDMap.RemoveAll();
    HX_RELEASE(m_pResponse);
}

STDMETHODIMP ConfigWatcher::Init(IHXPropWatchResponse* pResp)
{
    Cleanup();

    m_pResponse = pResp;
    if (m_pResponse)
	m_pResponse->AddRef();

    //m_pRegistry->CreatePropWatch(m_pPropWatch);
    //m_pPropWatch->Init((IHXPropWatchResponse*)this);

    return HXR_OK;
}

STDMETHODIMP ConfigWatcher::Manage(UINT32 regID)
{
    ConfigWatcherEntry* pEntry;

    if (!m_pRegistry || !m_pActiveRegistry)
	return HXR_NOT_INITIALIZED;

    void* pVoid;
    if (!m_regIDMap.Lookup(regID, pVoid))
    {
	ULONG32 parentID = m_pRegistry->FindParentIdById(regID);
	pEntry = new ConfigWatcherEntry(m_pResponse, regID, parentID);
	pEntry->AddRef();
	IHXBuffer* pBuf = NULL;
	if (pEntry && HXR_OK == m_pRegistry->GetPropName(regID, pBuf)) 
	{ 
	    pEntry->m_pName = new_string((const char*)pBuf->GetBuffer());

	    if (PT_COMPOSITE == m_pRegistry->GetTypeById(regID))
	    {
		//m_pPropWatch->SetWatchById(regID);
	    }

	    m_pActiveRegistry->SetAsActive(pEntry->m_pName, 
		    (IHXActivePropUser*) pEntry);

	    m_regIDMap[regID] = pEntry;
	    HX_RELEASE(pBuf);
	    DPRINTF(D_INFO, ("Watching %s%s(0x%d)\n", 
		    pEntry->m_type == PT_COMPOSITE ? "C" : "A",
		    pEntry->m_pName, regID  ));
	}
    }
    return HXR_OK;
}
	    
STDMETHODIMP ConfigWatcher::Unmanage(UINT32 regID)
{
    ConfigWatcherEntry* pEntry;
    void* pVoid;
    if (m_regIDMap.Lookup(regID, pVoid))
    {
	pEntry = (ConfigWatcherEntry*)pVoid;
	pEntry->SendResponse();
	DPRINTF(D_INFO, ("Unwatching %s%s(0x%d)\n", 
		pEntry->m_type == PT_COMPOSITE ? "C" : "A",
		pEntry->m_pName, regID  ));

	if (PT_COMPOSITE == m_pRegistry->GetTypeById(regID))
	{
	    //m_pPropWatch->ClearWatchById(regID);
	}

	m_pActiveRegistry->SetAsInactive(pEntry->m_pName, 
		(IHXActivePropUser*) pEntry);
	HX_RELEASE(pEntry);
	m_regIDMap.RemoveKey(regID);
    }
    return HXR_OK;
}

BOOL ConfigWatcher::IsPending(UINT32 regID)
{
    ConfigWatcherEntry* pEntry;

    void* pVoid;
    if (m_regIDMap.Lookup(regID, pVoid))
    {
	pEntry = (ConfigWatcherEntry*)pVoid;
	return (pEntry->m_pUserResponse != NULL);
    }
    
    return FALSE;
}


BOOL ConfigWatcher::IsManaged(UINT32 regID)
{
    void* pEntry;
    return m_regIDMap.Lookup(regID, pEntry);
}

STDMETHODIMP ConfigWatcher::SetResponseValue(
	UINT32 regID, 
	HX_RESULT result,
	IHXBuffer* ppInfo[],
	UINT32 ulNumInfo)
{
    ConfigWatcherEntry* pEntry;
    void* pVoid;
    if (m_regIDMap.Lookup(regID, pVoid))
    {
	pEntry = (ConfigWatcherEntry*)pVoid;
	pEntry->m_result = result;
	if (ppInfo)
	{
	    for (UINT32 i=0; i<ulNumInfo; i++)
		ppInfo[i]->AddRef();
	}
	pEntry->m_ulNumInfo = ulNumInfo;
	pEntry->m_ppInfo = ppInfo;
    }
    return HXR_OK;
}

STDMETHODIMP ConfigWatcher::SetResponseValueInt(
	UINT32 regID, 
	HX_RESULT result,
	INT32 val,
	IHXBuffer* ppInfo[],
	UINT32 ulNumInfo)
{
    ConfigWatcherEntry* pEntry;
    void* pVoid;
    if (m_regIDMap.Lookup(regID, pVoid))
    {
	pEntry = (ConfigWatcherEntry*)pVoid;
	pEntry->m_result = result;
	pEntry->m_intVal = val;
	if (ppInfo)
	{
	    for (UINT32 i=0; i<ulNumInfo; i++)
		ppInfo[i]->AddRef();
	}
	pEntry->m_ulNumInfo = ulNumInfo;
	pEntry->m_ppInfo = ppInfo;
    }
    return HXR_OK;
}

STDMETHODIMP ConfigWatcher::SetResponseValueBuffer(
	UINT32 regID, 
	HX_RESULT result,
	IHXBuffer* pBuf,
	IHXBuffer* ppInfo[],
	UINT32 ulNumInfo)
{
    ConfigWatcherEntry* pEntry;
    void* pVoid;
    if (m_regIDMap.Lookup(regID, pVoid))
    {
	pEntry = (ConfigWatcherEntry*)pVoid;
	pEntry->m_result = result;
	pEntry->m_pBufferVal = pBuf;
	if (pBuf)
	    pBuf->AddRef();
	if (ppInfo)
	{
	    for (UINT32 i=0; i<ulNumInfo; i++)
		ppInfo[i]->AddRef();
	}
	pEntry->m_ulNumInfo = ulNumInfo;
	pEntry->m_ppInfo = ppInfo;
    }
    return HXR_OK;
}

STDMETHODIMP ConfigWatcherEntry::SendResponse()
{
    //fprintf(stderr, "(%p,%d)", this, m_type);
    if (m_pUserResponse)
    {
	DPRINTF(D_INFO, ("Sending response for %s(0x%x): ", 
		m_pName, m_result));
	if (!m_bDeleteRequest)
	{
	    switch (m_type)
	    {
	    case PT_INTEGER:
		{
		    DPRINTF(D_INFO, ("INT %d\n", m_intVal));
		    m_pUserResponse->SetActiveIntDone(
			    m_result,
			    m_pName,
			    m_intVal,
			    m_ppInfo,
			    m_ulNumInfo);
		    m_intVal = 0;
		}
		break;
	    case PT_BUFFER:
		{
		    m_pUserResponse->SetActiveBufDone(
			    m_result,
			    m_pName,
			    m_pBufferVal,
			    m_ppInfo,
			    m_ulNumInfo);
		    HX_RELEASE(m_pBufferVal);
		}
		break;
	    case PT_STRING:
		{
		    DPRINTF(D_INFO, ("STR %s\n", 
			    (const char*)m_pBufferVal->GetBuffer()));
		    m_pUserResponse->SetActiveStrDone(
			    m_result,
			    m_pName,
			    m_pBufferVal,
			    m_ppInfo,
			    m_ulNumInfo);
		    HX_RELEASE(m_pBufferVal);
		}
		break;
	    default:
		HX_ASSERT(0 && "Bad watch type");
	    }
	    HX_RELEASE(m_pUserResponse);
	    if (m_ppInfo)
	    {
		for (UINT32 i=0; i<m_ulNumInfo; i++)
		    m_ppInfo[i]->Release();
		m_ppInfo = NULL;
		m_ulNumInfo = 0;
	    }
	} 
	else 
	{
	    DPRINTF(D_INFO, ("DEL\n"));
	    m_pUserResponse->DeleteActivePropDone(
		    m_result,
		    m_pParentName ? m_pParentName : m_pName,
		    m_ppInfo,
		    m_ulNumInfo);
	    HX_RELEASE(m_pUserResponse);
	}
    }
    return HXR_OK;
}

STDMETHODIMP ConfigWatcher::SendPendingResponses()
{
    for (CHXMapLongToObj::Iterator i = m_regIDMap.Begin();
	 i != m_regIDMap.End();
	 ++i)
    {
	ConfigWatcherEntry* pEntry = (ConfigWatcherEntry*)*i;
	pEntry->SendResponse();
    }
    return HXR_OK;
}

STDMETHODIMP ConfigWatcher::GetPendingValueInt(UINT32 regID, REF(INT32) val)
{
    val = 0;
    ConfigWatcherEntry* pEntry = NULL;
    void* pVoid;
    if (m_regIDMap.Lookup(regID, pVoid))
    {
	pEntry = (ConfigWatcherEntry*)pVoid;
	if (pEntry->m_pUserResponse)
	{
	    val = pEntry->m_intVal;
	    return HXR_OK;
	}
    }
    return HXR_FAIL;
}

STDMETHODIMP ConfigWatcher::GetPendingValueBuffer(UINT32 regID, 
	REF(IHXBuffer*) pBuf)
{
    pBuf = 0;
    ConfigWatcherEntry* pEntry = NULL;
    void* pVoid;
    if (m_regIDMap.Lookup(regID, pVoid))
    {
	pEntry = (ConfigWatcherEntry*)pVoid;
	if (pEntry->m_pUserResponse)
	{
	    pBuf = pEntry->m_pBufferVal;
	    if (pBuf)
		pBuf->AddRef();
	    return (pEntry->m_pBufferVal) ? HXR_OK : HXR_FAIL;
	}
    }
    return HXR_FAIL;
}

STDMETHODIMP ConfigWatcherEntry::SetActiveInt(const char* pName,
	    UINT32 ul,
	    IHXActivePropUserResponse* pResponse)
{
    if (!pName || !pResponse)
	return HXR_UNEXPECTED;

    DPRINTF(D_INFO, ("Request change %s to %d\n", pName, ul));

    if (!strcasecmp(pName, m_pName))
    {
	if (m_pUserResponse)
	{
	    pResponse->SetActiveIntDone(
		HXR_UNEXPECTED,
		pName,
		ul, 
		NULL,
		0);
	    return HXR_UNEXPECTED;
	}

	HX_RESULT result;
	m_result = HXR_NOT_INITIALIZED;
	m_type = PT_INTEGER;
	m_intVal = ul;

	m_pUserResponse = pResponse;
	pResponse->AddRef();

	result = m_pResponse->ModifiedProp(m_regID, PT_INTEGER, m_parentID);

	if (m_result == HXR_NOT_INITIALIZED)
	{
	    m_result = result;
	}
    } 
    else 
    {
	//fprintf(stderr, "Short circuit\n");
	pResponse->SetActiveIntDone(
	    HXR_OK,
	    pName,
	    ul,
	    NULL,
	    0);
    }

    return HXR_OK;
}

STDMETHODIMP ConfigWatcherEntry::SetActiveBuf(const char* pName,
	    IHXBuffer* pBuf,
	    IHXActivePropUserResponse* pResponse)
{
    if (!pName || !pResponse)
	return HXR_UNEXPECTED;

    DPRINTF(D_INFO, ("Request change %s to %s\n", 
	    pName, pBuf ? (const char*)pBuf->GetBuffer() : "NULL"));

    if (!strcasecmp(pName, m_pName))
    {
	HX_RESULT result;

	if (m_pUserResponse)
	{
	    pResponse->SetActiveBufDone(
		HXR_UNEXPECTED,
		pName,
		pBuf, 
		NULL,
		0);
	    return HXR_UNEXPECTED;
	}

	m_result = HXR_NOT_INITIALIZED;
	m_type = PT_BUFFER;
	m_pBufferVal = pBuf;
	pBuf->AddRef();

	result = m_pResponse->ModifiedProp(m_regID, PT_BUFFER, m_parentID);

	if (m_result == HXR_NOT_INITIALIZED)
	{
	    m_result = result;
	}
    }
    else 
    {
	//fprintf(stderr, "Short circuit\n");
	pResponse->SetActiveBufDone(
	    HXR_OK,
	    pName,
	    pBuf,
	    NULL,
	    0);
    }

    return HXR_OK;
}

STDMETHODIMP ConfigWatcherEntry::SetActiveStr(const char* pName,
	    IHXBuffer* pBuf,
	    IHXActivePropUserResponse* pResponse)
{
    if (!pName || !pResponse)
	return HXR_UNEXPECTED;

    DPRINTF(D_INFO, ("Request change %s to %s\n", 
	    pName, pBuf ? (const char*)pBuf->GetBuffer() : "NULL"));

    if (!strcasecmp(pName, m_pName))
    {
	HX_RESULT result;

	if (m_pUserResponse)
	{
	    pResponse->SetActiveStrDone(
		HXR_UNEXPECTED,
		pName,
		pBuf, 
		NULL,
		0);
	    return HXR_UNEXPECTED;
	}

	HX_RELEASE(m_pBufferVal);
	m_result = HXR_NOT_INITIALIZED;
	m_type = PT_STRING;
	m_pBufferVal = pBuf;
	pBuf->AddRef();

	m_pUserResponse = pResponse;
	pResponse->AddRef();

	result = m_pResponse->ModifiedProp(m_regID, PT_STRING, m_parentID);

	if (m_result == HXR_NOT_INITIALIZED)
	{
	    m_result = result;
	}
    }
    else 
    {
	//fprintf(stderr, "Short circuit\n");
	pResponse->SetActiveStrDone(
	    HXR_OK,
	    pName,
	    pBuf,
	    NULL,
	    0);
    }

    return HXR_OK;
}

STDMETHODIMP ConfigWatcherEntry::DeleteActiveProp(const char* pName,
	    IHXActivePropUserResponse* pResponse)
{
    if (!pName || !pResponse)
	return HXR_UNEXPECTED;

    DPRINTF(D_INFO, ("Got delete request for %s\n", pName));

    HX_RESULT result;

    if (!strcasecmp(pName, m_pName) && !m_pParentName)
    {
	if (m_pUserResponse)
	{
	    pResponse->DeleteActivePropDone(
		HXR_UNEXPECTED,
		pName,
		NULL,
		0);
	    return HXR_UNEXPECTED;
	}

	m_result = HXR_NOT_INITIALIZED;
	m_bDeleteRequest = TRUE;
	m_pUserResponse = pResponse;
	pResponse->AddRef();

	result = m_pResponse->DeletedProp(m_regID, m_parentID);

	if (m_result == HXR_NOT_INITIALIZED)
	{
	    m_result = result;
	}
    }
    else 
    {
	//fprintf(stderr, "Short circuit\n");
	pResponse->DeleteActivePropDone(
	    HXR_OK,
	    pName,
	    NULL,
	    0);
    }


    return HXR_OK;
}

STDMETHODIMP
ConfigWatcher::AddedProp(const UINT32 ulId, const HXPropType propType,
        const UINT32            ulParentID)
{
    if (m_pResponse)
    {
        m_pResponse->AddedProp(ulId, propType, ulParentID);
    }

    return HXR_OK;
}

STDMETHODIMP
ConfigWatcher::ModifiedProp(
        const UINT32            ulId,
        const HXPropType       propType,
        const UINT32            ulParentID)
{
    if (m_pResponse)
    {
        m_pResponse->ModifiedProp(ulId, propType, ulParentID);
    }

    return HXR_OK;
}

STDMETHODIMP
ConfigWatcher::DeletedProp(
        const UINT32            ulId,
        const UINT32            ulParentID)
{
    if (m_pResponse)
    {
        m_pResponse->DeletedProp(ulId, ulParentID);
    }

    return HXR_OK;
}



/////////////////////////////////////////////////////////////////////////
//  Method:
//      IUnknown::AddRef
//  Purpose:
//      Everyone usually implements this the same... feel free to use
//      this implementation.
//
STDMETHODIMP_(UINT32) ConfigWatcher::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//      IUnknown::Release
//  Purpose:
//      Everyone usually implements this the same... feel free to use
//      this implementation.
//
STDMETHODIMP_(UINT32) ConfigWatcher::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
	return m_lRefCount;
    }
    delete this;
    return 0;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//      IUnknown::QueryInterface
//  Purpose:
//      Implement this to export the interfaces supported by your 
//      object.
//
STDMETHODIMP ConfigWatcher::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
	AddRef();
	*ppvObj = (IUnknown*)(IHXPropWatchResponse*)this;
	return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXPropWatchResponse))
    {
	AddRef();
	*ppvObj = (IHXPropWatchResponse*)this;
	return HXR_OK;
    }
    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//      IUnknown::AddRef
//  Purpose:
//      Everyone usually implements this the same... feel free to use
//      this implementation.
//
STDMETHODIMP_(UINT32) ConfigWatcherEntry::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//      IUnknown::Release
//  Purpose:
//      Everyone usually implements this the same... feel free to use
//      this implementation.
//
STDMETHODIMP_(UINT32) ConfigWatcherEntry::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
	return m_lRefCount;
    }
    delete this;
    return 0;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//      IUnknown::QueryInterface
//  Purpose:
//      Implement this to export the interfaces supported by your 
//      object.
//
STDMETHODIMP ConfigWatcherEntry::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
	AddRef();
	*ppvObj = (IUnknown*)(IHXActivePropUser*)this;
	return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXActivePropUser))
    {
	AddRef();
	*ppvObj = (IHXActivePropUser*)this;
	return HXR_OK;
    }
    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

