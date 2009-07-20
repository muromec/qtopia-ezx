/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: altserverproxy.cpp,v 1.4 2003/09/04 22:35:34 dcollins Exp $ 
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

#include "hxtypes.h"
#include "hxcom.h"
#include "hxerror.h"

#include "hxmon.h"
#include "proc.h"
#include "ihxpckts.h"
#include "netbyte.h"
#include "server_engine.h"
#include "errmsg_macros.h"

#include "hxmap.h"
#include "hxstrutl.h"
#include "servlist.h"
#include "base_callback.h"

#include "altserv.h"
#include "altserverproxycfg.h"
#include "altserverproxy.h"


AltServerProxy::AltServerProxy(Process* pProc)
    : m_ulRefCount(0)
    , m_pProc(pProc)
    , m_pCfgHandler(NULL)
    , m_pServerAltMap(NULL)
    , m_pProxyAltMap(NULL)
    , m_pDPath(NULL)
    , m_pRespMap(NULL)
{    
//    printf("AltServerProxy: proc_num: %u\n", Process::get_procnum());
}


AltServerProxy::~AltServerProxy()
{
    HX_RELEASE(m_pCfgHandler);

    m_pCfgHandler->ClearResponse(this);

    CHXMapStringToOb::Iterator i;
    for (i = m_pServerAltMap->Begin(); i != m_pServerAltMap->End(); ++i)
    {
	((IHXBuffer*)*i)->Release();
    }	
    delete m_pServerAltMap;    
    
    for (i = m_pProxyAltMap->Begin(); i != m_pProxyAltMap->End(); ++i)
    {
	((IHXBuffer*)*i)->Release();
    }	
    delete m_pProxyAltMap;    

    CHXMapPtrToPtr::Iterator j;
    for (j = m_pRespMap->Begin(); j != m_pRespMap->End(); ++j)
    {
	((IHXAlternateServerProxyResponse*)*j)->Release();
    }	
    delete m_pRespMap;    

    HX_RELEASE(m_pDPath);
}

HX_RESULT
AltServerProxy::SetCfgHandler(AltServerProxyConfigHandler* pCfgHandler)
{
    HX_ASSERT(pCfgHandler);
    HX_ASSERT(!m_pCfgHandler);

    if (!m_pProc || !pCfgHandler)
    {
	ERRMSG(m_pProc->pc->error_handler, 
		"Failed to initialize player reconnection\n");

	return HXR_INVALID_PARAMETER;
    }
    else
    {
	m_pCfgHandler = pCfgHandler;
	m_pCfgHandler->AddRef();
    }
    

    IHXValues* pServAlts = NULL;
    IHXValues* pProxAlts = NULL;

    m_pCfgHandler->SetResponse(this);
    m_pCfgHandler->GetAllAltServers(pServAlts);
    m_pCfgHandler->GetAllAltProxies(pProxAlts);

    // we'll use IHXValue for this
    m_pCfgHandler->GetAllDynamicPaths(m_pDPath);

    m_pServerAltMap = new CHXMapStringToOb();
    m_pProxyAltMap  = new CHXMapStringToOb();

    m_pRespMap = new CHXMapPtrToPtr();
    
    CreateMap(pServAlts, m_pServerAltMap);
    CreateMap(pProxAlts, m_pProxyAltMap);

    HX_RELEASE(pServAlts);
    HX_RELEASE(pProxAlts);

#if 0
    TestServerProxy();
    TestServer();
    TestProxy();
#endif
    return HXR_OK;
}

STDMETHODIMP
AltServerProxy::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
	AddRef();
	*ppvObj = (IUnknown*)(IHXAlternateServerProxy*)this;
	return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXAlternateServerProxy))
    {
	AddRef();
	*ppvObj = (IHXAlternateServerProxy*)this;
	return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32) 
AltServerProxy::AddRef()
{
    return InterlockedIncrement(&m_ulRefCount);
}

STDMETHODIMP_(ULONG32) 
AltServerProxy::Release()
{
    if (InterlockedDecrement(&m_ulRefCount) > 0)
    {
        return m_ulRefCount;
    }

    delete this;
    return 0;
}

void 
AltServerProxy::CreateMap(IHXValues* pVal, CHXMapStringToOb* pMap)
{
    HX_ASSERT(pVal);
    HX_ASSERT(pMap);
#ifdef _DEBUG
    IHXBuffer* p;
#endif    

    const char* pc;
    IHXBuffer* pBuf;
    
    if (HXR_OK == pVal->GetFirstPropertyCString(pc, pBuf))
    {
//	printf("alt:\n"); 
//	printf("\t%s: %s\n", pc, (char*)pBuf->GetBuffer());
	
	HX_ASSERT(!pMap->Lookup(pc, (void*&)p));

	// take the AddRef() above
	pMap->SetAt(pc, pBuf);

	while (HXR_OK == pVal->GetNextPropertyCString(pc, pBuf))
	{
//	    printf("\t%s: %s\n", pc, (char*)pBuf->GetBuffer());	    

	    HX_ASSERT(!pMap->Lookup(pc, (void*&)p));

	    // take the AddRef() above
	    pMap->SetAt(pc, pBuf);
	}
    }    
}


void
AltServerProxy::CleanMap(CHXMapStringToOb* pMap)
{
    HX_ASSERT(pMap);
    
/**********************************
 * XXXGo - need to clean this up!!!
 */
 /*
    CHXMapStringToOb::Iterator i;
    for (i = pMap->Begin(); i != pMap->End(); ++i)
    {
	
	((IHXBuffer*)*i)->Release();
    }	        
*/    
    pMap->RemoveAll();    
}


/**************************************************************************
*  IHXAlternateServerProxy				     ref: altserv.h
*/
STDMETHODIMP 
AltServerProxy::Init(IHXAlternateServerProxyResponse* pResp)				 
{
    HX_ASSERT(m_pRespMap);

#ifdef _DEBUG
    void* p;
#endif    
    HX_ASSERT(!m_pRespMap->Lookup(pResp, p));

    pResp->AddRef();
    m_pRespMap->SetAt(pResp, pResp);
    return HXR_OK;
}

STDMETHODIMP 
AltServerProxy::ClearResponse(IHXAlternateServerProxyResponse* pResp)
{
    IHXAlternateServerProxyResponse* v;
    if (m_pRespMap->Lookup(pResp, (void*&)v))
    {
	HX_ASSERT(pResp == v);
	v->Release();
	m_pRespMap->Remove(pResp);
    }
    else
    {
	HX_ASSERT(!"not in a map");
    }
    return HXR_OK;
}    

STDMETHODIMP_(BOOL) 
AltServerProxy::IsEnabled(const char* pURL)
{
    HX_ASSERT(pURL);
    HX_ASSERT(m_pDPath);

    const char* pDPath = NULL;
    UINT32 ulLen = 0;

    // don't need the host
    char* pRsrc = (char*)strchr(pURL, '/');        
        
    if (HXR_OK == m_pDPath->GetFirstPropertyULONG32(pDPath, ulLen))
    {	
	if (strncasecmp(pRsrc, pDPath, ulLen) == 0)
	{
	    // dynamic path!
	    return FALSE;
	}
	
	while (HXR_OK == m_pDPath->GetNextPropertyULONG32(pDPath, ulLen))
	{
	    if (strncasecmp(pRsrc, pDPath, ulLen) == 0)
	    {
	    	// dynamic path!
	    	return FALSE;
	    }
	}	
    }    

    return TRUE;
}

STDMETHODIMP 
AltServerProxy::GetAltServerProxy
(
    const char* pURL,
    REF(IHXBuffer*) pAltServ,
    REF(IHXBuffer*) pAltProx
)
{
    HX_ASSERT(pURL);

    char* pc;
    char* pHost = new_string(pURL);
    char* pRsrc = strchr(pHost, '/');

    if (!pHost || !pRsrc)
    {
	HX_VECTOR_DELETE(pHost);
	return HXR_INVALID_PARAMETER;
    }

    HX_RESULT theErrServ = FindAltServers(pRsrc, pAltServ);

    *pRsrc = '\0';
    pc = strchr(pHost, ':');
    if (pc)
    {
	// get rid of port
	*pc = '\0';
    }
    
    HX_RESULT theErrProx = FindAltProxies(pHost, pAltProx);

    HX_VECTOR_DELETE(pHost);

    if (FAILED(theErrProx) && FAILED(theErrServ))
    {
	HX_RELEASE(pAltProx);
	HX_RELEASE(pAltServ);
	return HXR_FAIL;
    }

    HX_ASSERT(pAltProx || pAltServ);    
    return HXR_OK;    
}


STDMETHODIMP 
AltServerProxy::GetAltServers
(				 
    const char* pURL, 
    REF(IHXBuffer*) pAlt
)
{
    HX_ASSERT(pURL);
    HX_RESULT theErr;

    char* pc = (char*)strchr(pURL, '/');
    if (!pc)
    {
    	return HXR_INVALID_PARAMETER;
    }
    char* pRsrc = new_string(pc);

    theErr = FindAltServers(pRsrc, pAlt);
    HX_VECTOR_DELETE(pRsrc);

    return theErr;    
}

STDMETHODIMP 
AltServerProxy::GetAltProxies
(
    const char* pURL, 
    REF(IHXBuffer*) pAlt
)
{
    HX_ASSERT(pURL);
    HX_RESULT theErr;

    char* pHost = new_string(pURL);
    char* pc = (char*)strchr(pHost, '/');
    if (!pc)
    {
    	return HXR_INVALID_PARAMETER;
    }
    *pc = '\0';
    pc = (char*)strchr(pHost, ':');
    if (pc)
    {
	*pc = '\0';
    }    

    theErr = FindAltProxies(pHost, pAlt);
    HX_VECTOR_DELETE(pHost);

    return theErr;
}


/*
 * pRsrc will be modified
 */
HX_RESULT
AltServerProxy::FindAltServers(const char* pRsrc, REF(IHXBuffer*) pAlt)
{
//    printf("FindAltServers: %s\n", pRsrc);

    HX_ASSERT(pRsrc);
    HX_ASSERT('/' == *pRsrc);

    char*	pc	= NULL;    

    if (m_pServerAltMap->Lookup(pRsrc, (void*&)pAlt))
    {
	// lucky day!
//	printf("\t\tfound: %s\n", pAlt->GetBuffer());	
	pAlt->AddRef();
	return HXR_OK;
    }

#ifdef _DEBUG
    const char* pcDebug = pRsrc + (strlen(pRsrc) - 1);
#endif    

    // replace the last '/' and try to find the match
    while (pRsrc && (pc = (char*)strrchr(pRsrc, '/')))
    {

	HX_ASSERT((pc+1) <= pcDebug);
    
	*(pc+1) = '\0';

//	printf("\tlooking: %s\n", pRsrc);
	if (m_pServerAltMap->Lookup(pRsrc, (void*&)pAlt))
	{
	    // found it!	    
//	    printf("\t\tfound: %s\n", pAlt->GetBuffer());
	    pAlt->AddRef();
	    return HXR_OK;	
	}	
	*pc = '\0';
    }

    return HXR_FAIL;            
}



/*
 * pRsrc will be modified
 */
HX_RESULT
AltServerProxy::FindAltProxies(const char* pAddr, REF(IHXBuffer*) pAlt)
{
//    printf("FindAltProxies: %s\n", pAddr);

    HX_ASSERT(pAddr);

    char*	pc	= NULL;
    
    if (m_pProxyAltMap->Lookup("*", (void*&)pAlt))
    {
//    	printf("\t\tfound: %s\n", pAlt->GetBuffer());

	pAlt->AddRef();
	return HXR_OK;
    }

    if (m_pProxyAltMap->Lookup(pAddr, (void*&)pAlt))
    {
//    	printf("\t\tfound: %s\n", pAlt->GetBuffer());

	pAlt->AddRef();
	return HXR_OK;
    }

    if (IsNumericAddr(pAddr, strlen(pAddr)))
    {
	// replacing from the end
	const char* pcTemp = pAddr + strlen(pAddr) - 1;

	while (pAddr && (pc = (char*)strrchr(pAddr, '.')))
	{	    
	    HX_ASSERT(pc < pcTemp);
	    if ((pcTemp - pc) == 1)
	    {
		// only one letter after the '.'.  Just replace it
		*(pc+1) = '*';		
	    }
	    else
	    {
		HX_ASSERT((pcTemp - pc) > 1);
		*(pc+1) = '*';
		*(pc+2) = '\0';
	    }

//	    printf("\tlooking: %s\n", pAddr);
	    if (m_pProxyAltMap->Lookup(pAddr, (void*&)pAlt))
	    {
		// found it!	    
//		printf("\t\tfound: %s\n", pAlt->GetBuffer());
		pAlt->AddRef();
		return HXR_OK;	
	    }	
	    *pc = '\0';   	
	    pcTemp = pc-1;
    	}
	
    }
    else    
    {
	// replacing from the start.
	char* pcTemp = (char*)pAddr;
	
	while (pcTemp && (pc = strchr(pcTemp, '.')))
	{
	    HX_ASSERT(pcTemp < pc);
	    pcTemp = pc-1;
	    *pcTemp = '*';

//	    printf("\tlooking: %s\n", pcTemp);
	    if (m_pProxyAltMap->Lookup(pcTemp, (void*&)pAlt))
	    {
		// found it!	    
//		printf("\t\tfound: %s\n", pAlt->GetBuffer());
		
		pAlt->AddRef();
		return HXR_OK;	
	    }	
	    pcTemp = pc+1;	    	    
	}
    }   

    return HXR_FAIL;            
}


HX_RESULT
AltServerProxy::OnModifiedEntry(HX_ALTERNATES_MOD_FLAG type)
{
//printf("%p: AltServerProxy::OnModifiedEntryServer() proc: %u\n", this, Process::get_procnum());

    IHXValues* pAltVals = NULL;

    if (HX_GET_ALTERNATES_FLAG(type, HX_ALT_SERVER))
    {
	// server config has been modified
    	m_pCfgHandler->GetAllAltServers(pAltVals);
    	CleanMap(m_pServerAltMap);
    	CreateMap(pAltVals, m_pServerAltMap);
    	HX_RELEASE(pAltVals);
    }
    if (HX_GET_ALTERNATES_FLAG(type, HX_ALT_PROXY))
    {
    	m_pCfgHandler->GetAllAltProxies(pAltVals);    
    	CleanMap(m_pProxyAltMap);
    	CreateMap(pAltVals, m_pProxyAltMap);	
    	HX_RELEASE(pAltVals);
    }
    if (HX_GET_ALTERNATES_FLAG(type, HX_ALT_DPATH))
    {
	HX_RELEASE(m_pDPath);
	m_pCfgHandler->GetAllDynamicPaths(m_pDPath);	
    }

    /* deal with response */
    HX_ASSERT(m_pRespMap);

    IHXAlternateServerProxyResponse* pResp;
    CHXMapPtrToPtr::Iterator i;    
    for (i = m_pRespMap->Begin(); i != m_pRespMap->End(); ++i)
    {
	pResp = (IHXAlternateServerProxyResponse*)*i;
	pResp->OnModifiedEntry(type);	
    }	
/*    
    HX_ASSERT(m_pRespMap);
    CHXMapPtrToPtr* pResps = m_pRespMap;
    m_pRespMap = new CHXMapPtrToPtr();

    IHXAlternateServerProxyResponse* pResp;
    CHXMapPtrToPtr::Iterator i;    
    for (i = pResps->Begin(); i != pResps->End(); ++i)
    {
	pResp = (IHXAlternateServerProxyResponse*)*i;
	pResp->OnModifiedEntry(type);	
	pResp->Release();
    }	
    HX_DELETE(pResps);
*/
    return HXR_OK;
}


#if 0
/******************************************************************************
 * TESTING...
 */
void 
AltServerProxy::TestServerProxy()
{
//    printf("***** testing server/proxy *****\n");
    IHXBuffer* pAlt1 = NULL;
    IHXBuffer* pAlt2 = NULL;
    
    char* pcSet[] = 
    {
	"172.23.100.1:34/test.rm",
	"foo.com/foo/foo.rm",
	"12.12.12.2/foo/bar/foo.rm"
    };
    int size = sizeof(pcSet) / sizeof(pcSet[0]);
    
    for (int i = 0; i < size; i++)
    {
	if (HXR_OK == GetAltServerProxy(pcSet[i], pAlt1, pAlt2))
	{
	    if (pAlt1)
	    {
//		printf("\t\tfound %s\n", pAlt1->GetBuffer());	
		HX_RELEASE(pAlt1);
	    }	
	    if (pAlt2)
	    {
//		printf("\t\tfound %s\n", pAlt2->GetBuffer());	
		HX_RELEASE(pAlt2);
	    }				
	}
	else
	{
//	    printf("\t\tfailed...\n");
	}
    }

}


void 
AltServerProxy::TestServer()
{
//    printf("***** testing server *****\n");
    IHXBuffer* pAlt = NULL;

    char* pcSet[] = 
    {
	"172.23.100.1:34/test.rm",
	"foo.com/foo/foo.rm",
	"12.12.12.2/foo/bar/foo.rm"
    };
    int size = sizeof(pcSet) / sizeof(pcSet[0]);
    
    for (int i = 0; i < size; i++)
    {
	if (HXR_OK == GetAltServers(pcSet[i], pAlt))
	{
//	    printf("\t\tfound %s\n", pAlt->GetBuffer());	
	    HX_RELEASE(pAlt);
	}
	else
	{
//	    printf("\t\tfailed...\n");
	}
    }

}

void 
AltServerProxy::TestProxy()
{
//    printf("***** testing proxy *****\n");
    IHXBuffer* pAlt = NULL;

    char* pcSet[] =
    {
	"172.23.100.1:23/foo.rm",
	"172.23.101.5:2/test.wav",
	"172.23.100.2/foo/bar/foo",
	"abc.def.com:502/lkdf",
	"ddd.def.com/ld/ls/ls/ls/ls"
    };

    int size = sizeof(pcSet) / sizeof(pcSet[0]);
    
    char* pc;

    for (int i = 0; i < size; i++)
    {
	if (HXR_OK == GetAltProxies(pcSet[i], pAlt))
	{
//	    printf("\t\tfound %s\n", pAlt->GetBuffer());	
	    HX_RELEASE(pAlt);
	}
	else
	{
//	    printf("\t\tfailed...\n");
	}
    }
}
#endif 
