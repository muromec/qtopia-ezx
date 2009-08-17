/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: altserverproxycfg.cpp,v 1.4 2003/09/04 22:35:34 dcollins Exp $ 
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
 *  altserverproxycfg.cpp
 *  
 *  Player reconnect config var handler
 */

#include "hxtypes.h"
#include "hxcom.h"
#include "hxerror.h"

#include "hxmon.h"
#include "hxpropw.h"
#include "dispatchq.h"

#include "proc.h"
#include "servreg.h"
#include "hxmap.h"
#include "hxstrutl.h"
#include "servlist.h"

#include "altserverproxy.h"
#include "altserverproxycfg.h"


AltServerProxyConfigHandler::AltServerProxyConfigHandler
    (Process* pProc, ServerRegistry* pRegistry)
    : m_ulRefCount(0)
    , m_pProc(pProc)
    , m_pRegistry(pRegistry)
    , m_pServer(NULL)
    , m_pProxy(NULL)
    , m_pDPath(NULL)
    , m_pRespList(NULL)
    , m_hCB(0)
{
    HX_ASSERT(m_pProc && m_pRegistry);   
//    printf("AltServerProxyConfigHandler::AltServerProxyConfigHandler(): proc: %u\n", Process::get_procnum());
}

AltServerProxyConfigHandler::~AltServerProxyConfigHandler()
{
//    printf("AltServerProxyConfigHandler::~AltServerProxyConfigHandler()\n");
    HX_ASSERT(FALSE);
}

STDMETHODIMP
AltServerProxyConfigHandler::QueryInterface(REFIID riid, void** ppvObj)
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
}

STDMETHODIMP_(ULONG32) 
AltServerProxyConfigHandler::AddRef()
{
    return InterlockedIncrement(&m_ulRefCount);
}

STDMETHODIMP_(ULONG32) 
AltServerProxyConfigHandler::Release()
{
    if (InterlockedDecrement(&m_ulRefCount) > 0)
    {
        return m_ulRefCount;
    }

    delete this;
    return 0;
}

HX_RESULT
AltServerProxyConfigHandler::GetAllDynamicPaths
(
    REF(IHXValues*) pDPath
)
{
    HX_ASSERT(m_pDPath && m_pDPath->m_pAltStrVals);
    pDPath = m_pDPath->m_pAltStrVals;
    pDPath->AddRef();
    return HXR_OK;
}

HX_RESULT
AltServerProxyConfigHandler::GetAllAltServers
(
    REF(IHXValues*) pAlt
)
{
    HX_ASSERT(m_pServer && m_pServer->m_pAltStrVals);
    pAlt = m_pServer->m_pAltStrVals;
    pAlt->AddRef();
    return HXR_OK;
}

HX_RESULT
AltServerProxyConfigHandler::GetAllAltProxies
(
    REF(IHXValues*) pAlt
)
{
    HX_ASSERT(m_pProxy && m_pProxy->m_pAltStrVals);
    pAlt = m_pProxy->m_pAltStrVals;
    pAlt->AddRef();
    return HXR_OK;
}

HX_RESULT 
AltServerProxyConfigHandler::ClearResponse(AltServerProxy* pResp)
{
    HX_ASSERT(m_pRespList);
    
    AltResp* pEntry;
    HXList_iterator i(m_pRespList);
    for (; *i != NULL; ++i)
    {
	pEntry = (AltResp*)*i;
	if (pEntry->m_pResp == pResp)
	{
	    HX_RELEASE(pEntry->m_pResp);
	    m_pRespList->remove(pEntry);	    
	    HX_DELETE(pEntry);
	    return HXR_OK;
	}
    }    
    HX_ASSERT(FALSE);
    return HXR_UNEXPECTED;
}


HX_RESULT
AltServerProxyConfigHandler::SetResponse(AltServerProxy* pResp)
{
    HX_ASSERT(pResp);
        
    if (!m_pRespList)
    {
	m_pRespList = new HXList();
    }	

#ifdef _DEBUG
    AltResp* pEntry;
    HXList_iterator i(m_pRespList);
    for (; *i != NULL; ++i)
    {
	pEntry = (AltResp*)*i;
	HX_ASSERT(pEntry->m_pResp != pResp);
    }        
#endif    

    pResp->AddRef();	
    AltResp* resp = new AltResp;
    resp->m_pResp = pResp;
    resp->m_iProcNum = Process::get_procnum();
    m_pRespList->insert(resp);
    return HXR_OK;
}

/******************************************************************************
 * Inits
 */
HX_RESULT
AltServerProxyConfigHandler::Init()
{            
    /* init */
    InitServer();
    InitProxy();
    InitDynamicPath();    
    return HXR_OK;	
}

HX_RESULT
AltServerProxyConfigHandler::InitServer()
{
    m_pServer= new Alternates(m_pProc, m_pRegistry, this, ALT_SERVER);
    m_pServer->AddRef();
        
    LoadAll(ALT_SERVER);
    return HXR_OK;
}

HX_RESULT
AltServerProxyConfigHandler::InitProxy()
{
    m_pProxy = new Alternates(m_pProc, m_pRegistry, this, ALT_PROXY);
    m_pProxy->AddRef();
    
    LoadAll(ALT_PROXY);    
    return HXR_OK;
}

HX_RESULT
AltServerProxyConfigHandler::InitDynamicPath()
{
    m_pDPath = new Alternates(m_pProc, m_pRegistry, this, ALT_DPATH);
    m_pDPath->AddRef();

    /*
     * DynamicPath has been loaded in m_pDPath.  Nothing more to do
     */    
    return HXR_OK;
}


/*
 * LoadAll() - Server and Proxy
 */
HX_RESULT 
AltServerProxyConfigHandler::LoadAll(ServerProxyEnum type)
{
    HX_RESULT theErr = HXR_FAIL;
    IHXValues* pComp = NULL;
    
    if (ALT_SERVER == type)
    {
	m_pServer->SetWatch("config.ServerAlternates.RedirectRules");
	theErr = m_pRegistry->GetPropList("config.ServerAlternates.RedirectRules", pComp, m_pProc);
    }
    else
    {
        theErr = m_pProxy->SetProxyDefaultAltStrVals();
	return theErr;	
    }

    if (FAILED(theErr))
    {
	return theErr;	
    }

    UINT32 ulRegID;
    const char* pName;

    if (HXR_OK == pComp->GetFirstPropertyULONG32(pName, ulRegID))
    {	
	CreateAlternates(pName, ulRegID, type);
	while (HXR_OK == pComp->GetNextPropertyULONG32(pName, ulRegID))
	{
	    CreateAlternates(pName, ulRegID, type);
	}	
    }
    
    
    return HXR_OK;    
}

HX_RESULT
AltServerProxyConfigHandler::CreateAlternates
(
    const char* pName, 
    UINT32 ulRegID, 
    ServerProxyEnum type
)
{
    HX_ASSERT(pName && ulRegID);

    HX_RESULT theErr;
    IHXBuffer* pPathPrefix = NULL;
    IHXValues* pAlternates = NULL;
    UINT32 ulLen = 0;

    ulLen = strlen(pName) + strlen(".Alternates") + 1;
    char* pEntryName = new char[ulLen];

    sprintf(pEntryName, "%s.Rule", pName);	
    if (ALT_SERVER == type)
    {
	m_pServer->SetWatch(pEntryName);
    }
    else
    {
	m_pProxy->SetWatch(pEntryName);    
    }
            
    theErr = m_pRegistry->GetStr(pEntryName, pPathPrefix, m_pProc);
    if (FAILED(theErr))
    {
	goto bail;
    }

    sprintf(pEntryName, "%s.Alternates", pName);
    if (ALT_SERVER == type)
    {
	m_pServer->SetWatch(pEntryName);
    }
    else
    {
	m_pProxy->SetWatch(pEntryName);	
    }

    theErr = m_pRegistry->GetPropList(pEntryName, pAlternates, m_pProc);
    if (FAILED(theErr))
    {
	goto bail;
    }
    

    HX_ASSERT(pPathPrefix && pAlternates);

    /*
     * Create Alternate String that'll be sent to a client
     */
    theErr = CreateAlternateString((const char*)pPathPrefix->GetBuffer(), 
	pAlternates, type);

// single exit point
bail:
    HX_VECTOR_DELETE(pEntryName);
    HX_RELEASE(pPathPrefix);
    HX_RELEASE(pAlternates);

    return theErr;        
}


HX_RESULT
AltServerProxyConfigHandler::CreateAlternateString
(
    const char* pKey, 
    IHXValues* pAltNames,
    ServerProxyEnum type
)
{
    HX_ASSERT(pAltNames);

    HX_RESULT theErr = HXR_FAIL;
    const char* pName = NULL;
    UINT32 ulRegID = 0;

    CHXSimpleList pList;
    UINT32 ulTotalStrSize = 0;

    if (SUCCEEDED(pAltNames->GetFirstPropertyULONG32(pName, ulRegID)))
    {	
//	printf("CreateAlternateString: name %s regid %u\n", pName, ulRegID);
	theErr = AccumulateAltStr(ulRegID, &pList, ulTotalStrSize, type);
	
	while (SUCCEEDED(pAltNames->GetNextPropertyULONG32(pName, ulRegID)))
	{
//	    printf("CreateAlternateString: name %s regid %u\n", pName, ulRegID);
	    theErr = AccumulateAltStr(ulRegID, &pList, ulTotalStrSize, type);
	}	
    }

    UINT32 ulCount = pList.GetCount();
    if (SUCCEEDED(theErr) && ulCount)
    {
	HX_ASSERT(ulTotalStrSize);
	IHXBuffer* pAlt = NULL;
	m_pProc->pc->common_class_factory->CreateInstance(
	    CLSID_IHXBuffer, (void**)&pAlt);

	pAlt->SetSize(ulTotalStrSize + ulCount + 3);
	char* pcOrig = (char*)pAlt->GetBuffer();
	
	char* pc = pcOrig;
	IHXBuffer* pAltStr = NULL;
	UINT32 ulStrLen = 0;
	
	while (!pList.IsEmpty())
	{
	    pAltStr = (IHXBuffer*)pList.RemoveHead();
	    HX_ASSERT(pAltStr);
	    ulStrLen = pAltStr->GetSize();
	    
	    memcpy(pc, pAltStr->GetBuffer(), ulStrLen);
	    pc += ulStrLen; 

	    if (!pList.IsEmpty())
	    {
		// more to come
		*pc++ = ',';
	    }

	    HX_RELEASE(pAltStr);
	}

	*pc = '\0';

	// reset size
	HX_ASSERT((ulTotalStrSize+ulCount+3) >= (strlen(pcOrig)+1));
	pAlt->SetSize(strlen(pcOrig) + 1);

//printf("key: %s AlternateString: %s\n", pKey, pcOrig);
	if (ALT_SERVER == type)
	{
	    theErr = m_pServer->m_pAltStrVals->SetPropertyCString(pKey, pAlt);
	    HX_ASSERT(SUCCEEDED(theErr));	    	    
	}
	else
	{
	    theErr = m_pProxy->m_pAltStrVals->SetPropertyCString(pKey, pAlt);
	    HX_ASSERT(SUCCEEDED(theErr));	    	    
	}

	HX_RELEASE(pAlt);

	return HXR_OK;
    }
            

    return HXR_UNEXPECTED;
}

HX_RESULT
AltServerProxyConfigHandler::AccumulateAltStr
(
    UINT32 ulRegID, 
    CHXSimpleList* pList, 
    REF(UINT32) ulTotalSize,
    ServerProxyEnum type
)
{
    HX_ASSERT(pList);
    HX_ASSERT(ulRegID);

    HX_RESULT theErr = HXR_FAIL;
    IHXBuffer* pAltName = NULL;
    IHXBuffer* pAltStr = NULL;
    
    if (FAILED(m_pRegistry->GetStr(ulRegID, pAltName, m_pProc)))
    {
	HX_ASSERT(!"Shouldn't happen");
	return HXR_FAIL;
    }

    if (ALT_SERVER == type)
    {
	m_pServer->SetWatch(ulRegID);
	theErr = m_pServer->GetAlternate((const char*)pAltName->GetBuffer(), pAltStr);
    }
    else
    {
	m_pProxy->SetWatch(ulRegID);
	theErr = m_pProxy->GetAlternate((const char*)pAltName->GetBuffer(), pAltStr);
    }
    
    if (FAILED(theErr))
    {
	HX_ASSERT(!pAltStr);
	HX_RELEASE(pAltName);
	return theErr;
    }
    
    pList->AddTail((void*)pAltStr);
    ulTotalSize += pAltStr->GetSize();

    return theErr;    
}

void
AltServerProxyConfigHandler::GetReadyToReload()
{
    if (m_hCB)
    {	
	// reschedule.
	m_pProc->pc->engine->schedule.remove(m_hCB);
	m_hCB = 0;	
    }
    
    m_hCB = 
	m_pProc->pc->engine->schedule.enter(
	    m_pProc->pc->engine->now + Timeval(0.50),
	    this);
}

STDMETHODIMP 
AltServerProxyConfigHandler::Func()
{
    m_hCB = 0;

    HX_ASSERT(m_pServer && m_pProxy && m_pDPath);

    HX_ALTERNATES_MOD_FLAG type = 0;
        
    if (!m_pServer->m_bIsValid)
    {
//printf("%p: HX_ALT_SERVER: %u\n", this, HX_ALT_SERVER);
	HX_SET_ALTERNATES_FLAG(type, HX_ALT_SERVER);
	HX_RELEASE(m_pServer);
	InitServer();
    }
    if (!m_pProxy->m_bIsValid)
    {
//printf("%p: HX_ALT_PROXY: %u\n", this, HX_ALT_PROXY);
	HX_SET_ALTERNATES_FLAG(type, HX_ALT_PROXY);
	HX_RELEASE(m_pProxy);
	InitProxy();
    }
    if (!m_pDPath->m_bIsValid)
    {	
//printf("%p: HX_ALT_DPATH: %u\n", this, HX_ALT_DPATH);
	HX_SET_ALTERNATES_FLAG(type, HX_ALT_DPATH);
	HX_RELEASE(m_pDPath);
	InitDynamicPath();
    }

    DispatchResponse(type);

    return HXR_OK;
}

void
AltServerProxyConfigHandler::DispatchResponse(HX_ALTERNATES_MOD_FLAG type)
{
//    printf("dispatching list %p\n", m_pRespList);

    AltResp* pEntry;
    HXList_iterator i(m_pRespList);
    for (; *i != NULL; ++i)
    {
	pEntry = (AltResp*)*i;

	OnWrapper* cb = new OnWrapper();
	cb->m_pResp = pEntry->m_pResp; 
	cb->m_pResp->AddRef();
	cb->m_type = type;
	m_pProc->pc->dispatchq->send(m_pProc, cb, pEntry->m_iProcNum);
	
//	m_pRespList->remove(pEntry);
//	HX_DELETE(pEntry);
    }    
}

void
OnWrapper::func(Process* proc)
{
//    printf("OnWrapper: %u\n", Process::get_procnum());
    m_pResp->OnModifiedEntry(m_type);	
    HX_RELEASE(m_pResp);    
}


Alternates::Alternates
(
    Process* pProc, 
    ServerRegistry* pRegistry, 
    AltServerProxyConfigHandler* pCfgHandler,
    ServerProxyEnum type
)
    : m_ulRefCount(0)
    , m_pProc(pProc)
    , m_pRegistry(pRegistry)
    , m_pAlternateMap(NULL)
    , m_type(type)
    , m_pCfgHandler(pCfgHandler)

    , m_bIsValid(TRUE)
    , m_pAltStrVals(NULL)    
    , m_pWatchList(NULL)
{
    m_pAlternateMap = new CHXMapStringToOb();
    m_pProc->pc->common_class_factory->CreateInstance(
	CLSID_IHXValues, (void**)&m_pAltStrVals);

    m_pWatchList = new HXList();

    LoadAll();    
}

Alternates::~Alternates()
{
    HX_RELEASE(m_pAltStrVals);

    CHXMapStringToOb::Iterator i;
    for (i = m_pAlternateMap->Begin(); i != m_pAlternateMap->End(); ++i)
    {
	((IHXBuffer*)*i)->Release();
    }	
    delete m_pAlternateMap;

    WatchListElm* pElm;
    HXList_iterator j(m_pWatchList);
    for (; *j != NULL; ++j)
    {
    
	pElm = (WatchListElm*)*j;
	HX_ASSERT(pElm->m_ulHandle);
	m_pRegistry->ClearWatch(pElm->m_ulHandle, this, m_pProc);
	delete pElm;
	
    }        
    
    delete m_pWatchList;
}

STDMETHODIMP
Alternates::QueryInterface(REFIID riid, void** ppvObj)
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

STDMETHODIMP_(ULONG32) 
Alternates::AddRef()
{
    return InterlockedIncrement(&m_ulRefCount);
}

STDMETHODIMP_(ULONG32) 
Alternates::Release()
{
    if (InterlockedDecrement(&m_ulRefCount) > 0)
    {
        return m_ulRefCount;
    }

    delete this;
    return 0;
}


STDMETHODIMP Alternates::AddedProp
(
    const UINT32		id, 
    const HXPropType		propType, 
    const UINT32		ulParentID
)
{
//printf("%p: AddedProp() proc: %u\n", Process::get_procnum());
    m_bIsValid = FALSE;
    m_pCfgHandler->GetReadyToReload();    
    return HXR_OK;
}
STDMETHODIMP Alternates::ModifiedProp
(
    const UINT32		id,
    const HXPropType   	propType,
    const UINT32		ulParentID
)
{
//printf("%p: ModifiedProp() proc: %u\n", this, Process::get_procnum());
    m_bIsValid = FALSE;
    m_pCfgHandler->GetReadyToReload();    
    return HXR_OK;
}

STDMETHODIMP Alternates::DeletedProp
(
    const UINT32		id,
    const UINT32		ulParentID
)
{
//printf("%p: DeletedProp() proc: %u\n", this, Process::get_procnum());
    m_bIsValid = FALSE;
    m_pCfgHandler->GetReadyToReload();    
    return HXR_OK;
}

UINT32
Alternates::SetWatch(UINT32 ulRegID)
{
#if 0
    IHXBuffer* p = NULL;
    m_pRegistry->GetPropName(ulRegID, p, m_pProc);
    if (p)
    {
	printf("%p: watch: %s\n", this, p->GetBuffer());
	p->Release();
    }
    else
    {
	printf("%p: watch failed.....: %s\n", this, ulRegID);
    }
#endif

    ServerPropWatch* cb = new ServerPropWatch;
    cb->m_pResponse = this;
    cb->proc = m_pProc;
    cb->procnum = m_pProc->procnum();
    UINT32 h;
    if (h = m_pRegistry->SetWatch(ulRegID, cb))
    {
	WatchListElm* pElm = new WatchListElm();
	pElm->m_ulHandle = h;
	m_pWatchList->insert(pElm);   
	return h;
    }	

    // failed...
    delete cb;
    return 0;
}

UINT32
Alternates::SetWatch(const char* pName)
{
//printf("%p: watch: %s\n", this, pName);

    ServerPropWatch* cb = new ServerPropWatch;
    cb->m_pResponse = this;
    cb->proc = m_pProc;
    cb->procnum = m_pProc->procnum();
    UINT32 h;
    if (h = m_pRegistry->SetWatch(pName, cb))
    {
	WatchListElm* pElm = new WatchListElm();
	pElm->m_ulHandle = h;
	m_pWatchList->insert(pElm);   
	return h;
    }
    
    // failed...
    delete cb;
    return 0;
}

HX_RESULT
Alternates::GetAlternate(const char* pName, REF(IHXBuffer*)pAltStr)
{    
    if (m_pAlternateMap->Lookup(pName, (void*&)pAltStr))
    {
	pAltStr->AddRef();
	HX_ASSERT(m_bIsValid);	
	return HXR_OK;
    }

    return HXR_FAIL;;
}

HX_RESULT
Alternates::SetProxyDefaultAltStrVals()
{
    HX_RESULT theErr;
    IHXBuffer* pAlts;
    CHXMapStringToOb::Iterator i;
    UINT32 ulTotalStrSize = 0;
    UINT32 ulCount = 0;

    for (i = m_pAlternateMap->Begin(); i != m_pAlternateMap->End(); ++i)
    {
	ulTotalStrSize += ((IHXBuffer*)*i)->GetSize();
        ulCount++;
    }
    
    if ((ulCount == 0) || (ulTotalStrSize == 0))
    {
        return HXR_OK;
    }

    m_pProc->pc->common_class_factory->CreateInstance(
	CLSID_IHXBuffer, (void**)&pAlts);

    pAlts->SetSize(ulTotalStrSize + ulCount + 4);
    char* pcOrig = (char*)pAlts->GetBuffer();
    
    char* pc = pcOrig;
    IHXBuffer* pAltStr = NULL;
    UINT32 ulStrLen = 0;
    
    for (i = m_pAlternateMap->Begin(); i != m_pAlternateMap->End(); ++i)
    {
        pAltStr = ((IHXBuffer*)*i);
        HX_ASSERT(pAltStr);
        ulStrLen = pAltStr->GetSize();

        memcpy(pc, pAltStr->GetBuffer(), ulStrLen);
        pc += ulStrLen; 

        *pc++ = ',';
    }

    *(pc-1) = '\0';

    // reset size
    HX_ASSERT((ulTotalStrSize+ulCount+4) >= (strlen(pcOrig)+1));
    pAlts->SetSize(strlen(pcOrig) + 1);

//printf("key: %s AlternateString: %s\n", pKey, pcOrig);
    theErr = m_pAltStrVals->SetPropertyCString("*", pAlts);
    HX_ASSERT(SUCCEEDED(theErr));
    

    HX_RELEASE(pAlts);

    return theErr;
}

HX_RESULT
Alternates::LoadAll(void)
{
    HX_RESULT theErr;
    IHXValues* pComp = NULL;

    if (ALT_SERVER == m_type)
    {
	SetWatch("config.ServerAlternates.Alternates");	
	theErr = m_pRegistry->GetPropList("config.ServerAlternates.Alternates", 
	    pComp, m_pProc);
    }
    else if (ALT_PROXY == m_type)
    {
	SetWatch("config.ProxyAlternates.Alternates");
	theErr = m_pRegistry->GetPropList("config.ProxyAlternates.Alternates", 
	    pComp, m_pProc);	
    }
    else
    {
	SetWatch("config.ServerAlternates.ExcludePaths");
	theErr = m_pRegistry->GetPropList("config.ServerAlternates.ExcludePaths", pComp, m_pProc);		
    }

    if (FAILED(theErr))
    {
        return theErr;
    }

    const char* pName;
    UINT32 ulRegID;
    if (HXR_OK == pComp->GetFirstPropertyULONG32(pName, ulRegID))
    {
	if (ALT_DPATH == m_type)
	{   
	    CreateDPathVals(pName, ulRegID);
	}
	else
	{
	    HX_ASSERT(ALT_SERVER == m_type || ALT_PROXY == m_type);
	    CreateAltString(pName, ulRegID);
	}	    
    
        while (HXR_OK == pComp->GetNextPropertyULONG32(pName, ulRegID))
        {
	    if (ALT_DPATH == m_type)
	    {
		CreateDPathVals(pName, ulRegID);
	    }
	    else
	    {
		HX_ASSERT(ALT_SERVER == m_type || ALT_PROXY == m_type);
		CreateAltString(pName, ulRegID);
	    }		
        }
    }

    // all is good
    m_bIsValid = TRUE;

    HX_RELEASE(pComp);
    return HXR_OK;
}

/*
 * Create server=xxx;port=xxx for each Alternate.
 * This string may be combined in AltServerProxyConfigHandler to create
 * an actual string for each PathPrefix/Rule
 */
HX_RESULT
Alternates::CreateAltString(const char* pName, UINT32 ulRegID)
{
//    printf("name: %s, val: %u\n", pName, ulRegID);
    
    HX_RESULT	theErr	= HXR_FAIL;
    IHXBuffer* pHost	= NULL;
    INT32	lPort	= 0;

    UINT32	ulHostID = 0;
    UINT32	ulPortID = 0;


    /* get regIDs */
    char* pEntryStr = new char[strlen(pName) + 6];
    
    sprintf(pEntryStr, "%s.Host", pName);
    ulHostID = m_pRegistry->GetId(pEntryStr, m_pProc);
    
    sprintf(pEntryStr, "%s.Port", pName);
    ulPortID = m_pRegistry->GetId(pEntryStr, m_pProc);

    HX_VECTOR_DELETE(pEntryStr);
    
    if (!ulHostID || !ulPortID)
    {
	return HXR_FAIL;    
    }
    
    theErr = m_pRegistry->GetStr(ulHostID, pHost, m_pProc);
    if (FAILED(theErr))
    {
	return HXR_FAIL;	
    }

    theErr = m_pRegistry->GetInt(ulPortID, &lPort, m_pProc);
    if (FAILED(theErr))
    {
	HX_RELEASE(pHost);
	return HXR_FAIL;
    }
    
    /* create a string */
    IHXBuffer* pAltStr = NULL;
    m_pProc->pc->common_class_factory->CreateInstance(
	CLSID_IHXBuffer, (void**)&pAltStr);

    pAltStr->SetSize(pHost->GetSize() + 128);
    char* pc = (char*)pAltStr->GetBuffer();    
    
    sprintf(pc, "server=%s;port=%d", pHost->GetBuffer(), lPort);

    HX_ASSERT((strlen(pc)+1) <= (pHost->GetSize()+128));
    pAltStr->SetSize(strlen(pc));
    
//    printf("\tAlternate: %s\n", pc);

    /* now add watch */
    SetWatch(ulRegID);
    SetWatch(ulHostID);
    SetWatch(ulPortID);

    /* add it to the map...take AddRef() */
    HX_ASSERT(!pEntryStr);
    pEntryStr = (char*)strrchr(pName, '.');
    pEntryStr++;
    m_pAlternateMap->SetAt(pEntryStr, pAltStr);    

    HX_RELEASE(pHost);
    return HXR_OK;    
}

HX_RESULT
Alternates::CreateDPathVals(const char* pName, UINT32 ulRegID)
{
//printf("CreateDPathVals: %s\n", pName);

    // config.ServerAlternates.ExcludePaths.pName
    HX_ASSERT(pName && ulRegID);

    HX_RESULT theErr = HXR_FAIL;
    IHXBuffer* pBuf = NULL;
    
    theErr = m_pRegistry->GetStr(pName, pBuf, m_pProc);
    
    if (FAILED(theErr))
    {
	return HXR_FAIL;
    }

//    SetWatch(ulRegID);
    SetWatch(pName);

    // oh, well...put the length as a value...
    const char* pDPath = (const char*)pBuf->GetBuffer();
    m_pAltStrVals->SetPropertyULONG32(pDPath, strlen(pDPath));
    pBuf->Release();
    
    return HXR_OK;                    
}

