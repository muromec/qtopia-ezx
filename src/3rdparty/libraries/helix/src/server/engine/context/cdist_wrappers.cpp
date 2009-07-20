/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: cdist_wrappers.cpp,v 1.5 2003/09/04 22:35:34 dcollins Exp $ 
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
/****************************************************************************
 * Description:	
 *	Wrappers for cdist interface.
 *
 *	(1) wrapper for cdist thread (IHXContentDistribution).
 *
 *	(2) wrapper for cache-advise plugins (IHXContentDistributionAdvise).
 *
 * Also included are Callback objects to facilitate interaction with advise 
 * plugins running on their own thread.  The CDWPluginCallback is instantiated
 * by the streamer and runs on the plugin; the CDWStreamerCallback is
 * instantiated by the plugin and runs on the streamer.
 *
 * Debug info can be dumped by setting ContentSubscription.DebugLevel.  I
 * map the standard debuglevel symbols as follows:
 *
 *	D_INFO		(0x00000002)	->	general info
 * 	D_ENTRY		(0x00000004)	-> 	function entry
 *	D_STATE		(0x00000008)	->	constructor/destructor entry
 *	D_XFER		(0x00000010)	->	verbose rule-evaluation info
 */

#include <stdio.h>

#include "hxtypes.h"
#include "hxcom.h"
#include "hxiids.h"
#include "hxfiles.h"
#include "hxcomm.h"
#include "hxplugn.h"
#include "hxmon.h"
#include "ihxpckts.h"
#include "hxerror.h"
#include "defslice.h"

#include "hxslist.h"
#include "plgnhand.h"
#include "proc.h"
#include "dispatchq.h"
#include "misc_container.h"

//servsup
#include "cdist_defs.h"

#include "cdist_wrappers.h"

/*********************************************************************
 * CDistWrapper class
 * (Not implemented yet.)
 */
CDistWrapper::CDistWrapper()
    : m_lRefCount(0)
{
}

CDistWrapper::~CDistWrapper()
{
}

STDMETHODIMP
CDistWrapper::URLExists(THIS_
			const char* pPath,
			IHXContentDistributionResponse* pResp)
{
    return HXR_OK;
}

STDMETHODIMP
CDistWrapper::RequestBlocks(THIS)
{
    return HXR_OK;
}

STDMETHODIMP
CDistWrapper::OnFetchedBlocks(THIS)
{
    return HXR_OK;
}

STDMETHODIMP
CDistWrapper::OnCachePurge(THIS)
{
    return HXR_OK;
}


/***********************************************************************
 *
 * Standard IUnknown implementations
 * 
*/

STDMETHODIMP
CDistWrapper::QueryInterface(REFIID riid, void** ppvObj)
{
    CDPRINTF(CDIST_D_ENTRY, ("Entering CDistWrapper::QueryInterface (%p)\n", this));

    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXContentDistribution))
    {
        AddRef();
        *ppvObj = (IHXContentDistribution*)this;
        return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32)
CDistWrapper::AddRef()
{
    CDPRINTF(CDIST_D_ENTRY, ("Entering CDistWrapper::AddRef (%p)\n", this));
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32)
CDistWrapper::Release()
{
    CDPRINTF(CDIST_D_ENTRY, ("Entering CDistWrapper::Release (%p)\n", this));
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }
    
    delete this;
    return 0;
}

/***********************************************************
 * CDistAdviseWrapper
 *
 * This is supposed to take care of serializing calls to 
 * cdist-advise plugins.  That is, FSManager just needs instantiate
 * one of these wrappers and call OnLocalResult() on it, etc.
 *
 * You can mix and match multiload and non-multiload plugins; 
 * callbacks are used to talk to non-multiloads.
 *
 * It is NOT re-entrant.  Instantiate a new wrapper for each
 * file request.  (This also ensures you pick up changes to the
 * "ContentSubscription.Enabled" var.)
 *
 * jmevissen, 4/2001
 */

// Class vars

BOOL			CDistAdviseWrapper::zm_bLicensedSubscriber = FALSE;
BOOL			CDistAdviseWrapper::zm_bLicensedSubscriberValid = FALSE;
BOOL			CDistAdviseWrapper::zm_bCDistConfigPresent = FALSE;
CDistMIIStatistics*	CDistAdviseWrapper::zm_pMIIStats = NULL;
BOOL			CDistAdviseWrapper::zm_bForceRTSPImport = FALSE;

/*
 * FSManager checks for cdist by invoking IsOK() on this wrapper.
 * Three things are necessary for cdist to run:
 *
 * (1) ContentSubscription list in .cfg file;
 * (2) ContentDistribution.Subscriber license key enabled;
 * (3) At least one cdist-advise plugin present.
 * (4) ContentSubscription.Enabled, if present, be non-zero.
 *
 * Currently, a new wrapper is instantiated for each request, so we
 * only need check these once in the constructor.  If more persisten
 * wrappers are ever used, more checks (for example) will be needed on
 * the Enabled var.
 *
 * We need a separate advise-plugin list for each proc.  We keep the
 * list in the proc container, in a manner similar to the fs plugins.
 */
CDistAdviseWrapper::CDistAdviseWrapper(IUnknown* pContext,
				       Process* proc)
    : m_lRefCount(0)
    , m_pPluginList(proc->pc->cached_cdist_advise_list)
    , m_bOK(TRUE)
    , m_pIter(NULL)
    , m_pResp(NULL)
    , m_pRequest(NULL)
    , m_pURL(NULL)
    , m_pAdditional(NULL)
    , m_pProc(proc)
{
    CDPRINTF(D_STATE, ("Constructing CDistAdviseWrapper (%p)\n", this));

    if (!zm_bLicensedSubscriberValid)
    {
	// check the license

	INT32 tmp;
	IHXRegistry* pRegistry=NULL;
	pContext->QueryInterface(IID_IHXRegistry,
				 (void**)&pRegistry);

	if (!pRegistry ||
	    pRegistry->GetIntByName(REGISTRY_CDIST_SUBSCRIBER, tmp) != HXR_OK)
	{
	    tmp = LICENSE_CDIST_SUBSCRIBER;
	}

	zm_bLicensedSubscriber = (tmp != 0);
	zm_bLicensedSubscriberValid = TRUE;

	if (!zm_bLicensedSubscriber)
	{
	    CDPRINTF(D_INFO, ("Not a licensed subscriber for "
			      "content distribution.\n"));
	}

	// check whether cdist specified in .cfg

	if (pRegistry)
	{
	    if (pRegistry->GetId(CDIST_REGISTRY_ROOT))
	    {
		zm_bCDistConfigPresent = TRUE;

		// if cdist is enabled in registry, but not licensed, 
		// print a warning.
		if (!zm_bLicensedSubscriber)
		{
		    INT32 enabled;
		    if (HXR_OK == pRegistry->
			GetIntByName(CDIST_REGISTRY_SUB_ENABLED, enabled) &&
			enabled)
		    {
			IHXErrorMessages* pLog = NULL;
			pContext->QueryInterface(IID_IHXErrorMessages, (void**)&pLog);
			pLog->Report(HXLOG_WARNING,
				     HXR_NOT_LICENSED,
				     0,
				     "CDist: Content Distribution feature is "
				     "configured but not licensed.",
				     "");
			HX_RELEASE (pLog);
		    }
		}
	    }
	    else
	    {
		zm_bCDistConfigPresent = FALSE;
	    }

	    // get cdist stats struct - has copy of registry's "enabled" var
	    // Referring to a cached pointer is more efficient than getting
	    // CDIST_REGISTRY_SUB_ENABLED every time.

	    IHXBuffer* pBuf = NULL;
	    if (HXR_OK == pRegistry->GetBufByName(CDIST_REGISTRY_STATISTICS, pBuf))
	    {
		zm_pMIIStats = *((CDistMIIStatistics**)pBuf->GetBuffer());
		pBuf->Release();
	    }

	    // check for forced transport

	    if (HXR_OK != pRegistry->GetIntByName(CDIST_REGISTRY_RTSP_IMPORT, tmp))
	    {
		tmp = CDIST_DEFAULT_RTSP_IMPORT;
	    }
	    zm_bForceRTSPImport = (tmp != 0);

	    pRegistry->Release();
	}
    }

    if (!zm_bLicensedSubscriber || !zm_bCDistConfigPresent ||
	!zm_pMIIStats || !zm_pMIIStats->m_ubEnabled)
    {
	m_bOK = FALSE;
	return;
    }

    // get or build the proc's plugin list

//    if (!zm_bPluginListValid)
    if (!m_pPluginList)
    {
	// build the plugin list.

	HXList* pList = new HXList;

	CHXSimpleList*
	    pPlugins = proc->pc->plugin_handler->m_misc_handler->m_pPlugins;

	LISTPOSITION pos = pPlugins->GetHeadPosition();
	while(pos)
	{
	    IUnknown* pUnknown = NULL;
	    IHXContentDistributionAdvise* pCDAdvise = NULL;
	    IHXPlugin*  pPluginInterface = NULL;
	    CDWPluginListElem* pElem = NULL;

	    // Get next plugin
	    PluginHandler::Plugin* pPlugin = 
		(PluginHandler::Plugin*)pPlugins->GetAt(pos);

	    if (HXR_OK == pPlugin->GetInstance(&pUnknown) && pUnknown)
	    {
		// Query for the interface
		if ((pUnknown->QueryInterface(IID_IHXContentDistributionAdvise, 
					      (void**) &pCDAdvise) == HXR_OK) && 
		    (HXR_OK == pUnknown->QueryInterface(IID_IHXPlugin,
							(void**) &pPluginInterface)))
		{
		    pPluginInterface->InitPlugin(pContext);

		    if (pPlugin->m_load_multiple)
		    {
			pElem = new CDWPluginListElem(pCDAdvise);
			pElem->m_bLoadMultiple = TRUE;
		    }
		    else
		    {
			pElem = new CDWPluginListElem(NULL);
			pElem->m_bLoadMultiple = FALSE;
			pElem->m_iProcnum = ((Process*)pPlugin->m_process)->procnum();
		    }
		
		    if (pElem)
		    {
			INT32 prio = 0;
			pCDAdvise->GetPriority(prio);
			pElem->m_lPriority = prio;
		    }
		}

		HX_RELEASE(pCDAdvise);
		HX_RELEASE(pPluginInterface);
		HX_RELEASE(pUnknown);
	    }

	    if (pElem)
	    {
		HXList_iterator i(pList);

		if ((*i) == 0)
		{
		    pList->insert(pElem);
		}

		// Enumerate the list of priority-sorted plugins we're building.
		// short list, so do simple insertion sort

		for (; *i != 0; ++i)
		{
		    INT32 pluginPrio = ((CDWPluginListElem*)(*i))->m_lPriority;
		    if (pElem->m_lPriority < pluginPrio)
		    {
			pList->insert_before(pElem, *i);
			break;
		    }

		    if (*i == pList->peek_tail())
		    {
			pList->insert_after(pElem, *i);
			break;
		    }
		}
	    }

	    pPlugins->GetNext(pos);
	}

	if (pList->size == 0)
	{
	    IHXErrorMessages* pLog = NULL;
	    pContext->QueryInterface(IID_IHXErrorMessages, (void**)&pLog);
	    pLog->Report(HXLOG_ERR,
			 HXR_MISSING_COMPONENTS,
			 0,
			 "CDist: No valid cdist-advise plugins found. "
			 "Cannot use content distribution feature.",
			 "");
	    HX_RELEASE (pLog);
	    m_bOK = FALSE;
	}

	// copy created list to the proc container, where it remains forever
	proc->pc->cached_cdist_advise_list = pList;
	m_pPluginList = pList;
//	zm_bPluginListValid = TRUE;
    }

//    m_pPluginList = zm_pPluginList;
    if (m_bOK) m_bOK = (m_pPluginList->size != 0);
}

CDistAdviseWrapper::~CDistAdviseWrapper()
{
    CDPRINTF(D_STATE, ("Destructing CDistAdviseWrapper (%p)\n", this));

    delete m_pIter;
    HX_RELEASE(m_pResp);
    HX_RELEASE(m_pRequest);
    HX_RELEASE(m_pURL);
    HX_RELEASE(m_pAdditional);

    /*
     *  Don't delete this, it belongs to the process.
     */
    m_pPluginList = 0;
}

STDMETHODIMP
CDistAdviseWrapper::GetPriority(REF(INT32) lPriority)
{
    CDPRINTF(CDIST_D_ENTRY, ("Entering CDistAdviseWrapper::GetPriority (%p)\n", this));

    // This method is prioritizing the plugins.  It has no meaning
    // for this wrapper.

    return HXR_FAIL;
}

STDMETHODIMP
CDistAdviseWrapper::OnLocalResult(IHXContentDistributionAdviseResponse* pResp,
				  IHXRequest* pRequest,
				  BOOL bFound)
{
    CDPRINTF(CDIST_D_ENTRY, ("Entering CDistAdviseWrapper::OnLocalResult (%p)\n", this));
    if (!m_bOK) return HXR_FAIL;

    AddRef();

    // enumerate the plugins.

    ASSERT(!m_pIter);
    m_pIter = new HXList_iterator(m_pPluginList);

    ASSERT(!m_pResp);
    m_pResp = pResp;
    pResp->AddRef();

    m_pRequest = pRequest;
    pRequest->AddRef();

    m_bFound = bFound;

    NextOnLocalResult();

    return HXR_OK;
}

HX_RESULT
CDistAdviseWrapper::NextOnLocalResult()
{
    if (**m_pIter != 0)
    {
	CDWPluginListElem* pElem = (CDWPluginListElem*) **m_pIter;

	if (pElem->m_bLoadMultiple)
	{
	    pElem->GetObj()->OnLocalResult(this,
					   m_pRequest,
					   m_bFound);
	}
	else
	{
	    CDWPluginCallback* cb = 
		new CDWPluginCallback(ON_LOCAL_RESULT,
				      this,
				      m_pRequest,
				      m_bFound,
				      m_pProc->procnum()); // our procnum for reply

	    m_pProc->pc->dispatchq->send(m_pProc,
					 cb,
					 pElem->m_iProcnum);	// send-to procnum
	}
    }
    else
    {
	// No more plugins to enum.  Clean up, call back to our response object

	delete m_pIter;
	m_pIter = 0;
	HX_RELEASE(m_pRequest);

	IHXContentDistributionAdviseResponse* pResp = m_pResp;
	m_pResp = NULL;

	pResp->OnLocalResultDone(HXR_OK);
	pResp->Release();

	Release();
    }
    return HXR_OK;
}

HX_RESULT
CDistAdviseWrapper::OnCacheRequest(IHXContentDistributionAdviseResponse* pResp,
				   IHXRequest* pRequest)
{
    CDPRINTF(CDIST_D_ENTRY, ("Entering CDistAdviseWrapper::OnCacheRequest (%p)\n", this));
    if (!m_bOK) return HXR_FAIL;

    AddRef();

    // enumerate the plugins.

    ASSERT(!m_pIter);
    m_pIter = new HXList_iterator(m_pPluginList);

    ASSERT(!m_pResp);
    m_pResp = pResp;
    pResp->AddRef();

    m_pRequest = pRequest;
    pRequest->AddRef();

    NextOnCacheRequest();

    return HXR_OK;
}

HX_RESULT
CDistAdviseWrapper::NextOnCacheRequest()
{
    if (**m_pIter != 0)
    {
	CDWPluginListElem* pElem = (CDWPluginListElem*) **m_pIter;

	if (pElem->m_bLoadMultiple)
	{
	    pElem->GetObj()->OnCacheRequest(this, m_pRequest);
	}
	else
	{
	    CDWPluginCallback* cb = 
		new CDWPluginCallback(ON_CACHE_REQUEST,
				      this,
				      m_pRequest,
				      m_bFound,
				      m_pProc->procnum()); // our procnum for reply

	    m_pProc->pc->dispatchq->send(m_pProc,
					 cb,
					 pElem->m_iProcnum);	// send-to procnum
	}
    }
    else
    {
	// No more plugins to enum.  Clean up, call back to our response object

	delete m_pIter;
	m_pIter = 0;
	HX_RELEASE(m_pRequest);

	IHXContentDistributionAdviseResponse* pResp = m_pResp;
	m_pResp = NULL;

	pResp->OnCacheRequestDone(HXR_OK);
	pResp->Release();

	Release();
    }
    return HXR_OK;
}

HX_RESULT
CDistAdviseWrapper::OnCacheResult(IHXContentDistributionAdviseResponse* pResp,
				  IHXRequest* pRequest,
				  BOOL bFound)
{
    CDPRINTF(CDIST_D_ENTRY, ("Entering CDistAdviseWrapper::OnCacheResult (%p)\n", this));
    if (!m_bOK) return HXR_FAIL;

    // enumerate the plugins.

    AddRef();

    ASSERT(!m_pIter);
    m_pIter = new HXList_iterator(m_pPluginList);

    ASSERT(!m_pResp);
    m_pResp = pResp;
    pResp->AddRef();

    m_pRequest = pRequest;
    pRequest->AddRef();

    m_bFound = bFound;

    NextOnCacheResult();

    return HXR_OK;
}

HX_RESULT
CDistAdviseWrapper::NextOnCacheResult()
{
    if (**m_pIter != 0)
    {
	CDWPluginListElem* pElem = (CDWPluginListElem*) **m_pIter;

	if (pElem->m_bLoadMultiple)
	{
	    pElem->GetObj()->OnCacheResult(this,
					   m_pRequest,
					   m_bFound);
	}
	else
	{
	    CDWPluginCallback* cb = 
		new CDWPluginCallback(ON_CACHE_RESULT,
				      this,
				      m_pRequest,
				      m_bFound,
				      m_pProc->procnum()); // our procnum for reply

	    m_pProc->pc->dispatchq->send(m_pProc,
					 cb,
					 pElem->m_iProcnum);	// send-to procnum
	}
    }
    else
    {
	// No more plugins to enum.  Clean up, call back to our response object

	delete m_pIter;
	m_pIter = 0;
	HX_RELEASE(m_pRequest);

	IHXContentDistributionAdviseResponse* pResp = m_pResp;
	m_pResp = NULL;

	pResp->OnCacheResultDone(HXR_OK);
	pResp->Release();

	Release();
    }
    return HXR_OK;
}

STDMETHODIMP
CDistAdviseWrapper::OnSiteCacheResult(IHXContentDistributionAdviseResponse* pResp,
				      IHXRequest* pRequest,
				      BOOL bFound)
{
    CDPRINTF(CDIST_D_ENTRY, ("Entering CDistAdviseWrapper::OnSiteCacheResult (%p)\n", this));
    if (!m_bOK) return HXR_FAIL;

    // enumerate the plugins.

    AddRef();

    ASSERT(!m_pIter);
    m_pIter = new HXList_iterator(m_pPluginList);

    ASSERT(!m_pResp);
    m_pResp = pResp;
    pResp->AddRef();

    m_pRequest = pRequest;
    pRequest->AddRef();

    m_bFound = bFound;

    NextOnSiteCacheResult();

    return HXR_OK;
}

HX_RESULT
CDistAdviseWrapper::NextOnSiteCacheResult()
{
    if (**m_pIter != 0)
    {
	CDWPluginListElem* pElem = (CDWPluginListElem*) **m_pIter;

	if (pElem->m_bLoadMultiple)
	{
	    pElem->GetObj()->OnSiteCacheResult(this,
					       m_pRequest,
					       m_bFound);
	}
	else
	{
	    CDWPluginCallback* cb = 
		new CDWPluginCallback(ON_SITE_CACHE_RESULT,
				      this,
				      m_pRequest,
				      m_bFound,
				      m_pProc->procnum()); // our procnum for reply

	    m_pProc->pc->dispatchq->send(m_pProc,
					 cb,
					 pElem->m_iProcnum);	// send-to procnum
	}
    }
    else
    {
	// No more plugins to enum.  Clean up, call back to our response object

	delete m_pIter;
	m_pIter = 0;
	HX_RELEASE(m_pRequest);

	IHXContentDistributionAdviseResponse* pResp = m_pResp;
	m_pResp = NULL;

	pResp->OnSiteCacheResultDone(HXR_OK);
	pResp->Release();

	Release();
    }
    return HXR_OK;
}

STDMETHODIMP
CDistAdviseWrapper::OnPurgeCacheURL(IHXContentDistributionAdviseResponse* pResp,
				    IHXBuffer* pURL,
				    IHXValues* pAdditional)
{
    CDPRINTF(CDIST_D_ENTRY, ("Entering CDistAdviseWrapper::OnPurgeCacheURL (%p)\n", this));
    if (!m_bOK) return HXR_FAIL;

    // enumerate the plugins.

    AddRef();

    ASSERT(!m_pIter);
    m_pIter = new HXList_iterator(m_pPluginList);

    ASSERT(!m_pResp);
    m_pResp = pResp;
    pResp->AddRef();

    ASSERT(!m_pURL);
    m_pURL = pURL;
    if (pURL) pURL->AddRef();

    ASSERT(!m_pAdditional);
    m_pAdditional = pAdditional;
    if (pAdditional) pAdditional->AddRef();

    NextOnPurgeCacheURL();

    return HXR_OK;
}

HX_RESULT
CDistAdviseWrapper::NextOnPurgeCacheURL()
{
    if (**m_pIter != 0)
    {
	CDWPluginListElem* pElem = (CDWPluginListElem*) **m_pIter;

	if (pElem->m_bLoadMultiple)
	{
	    pElem->GetObj()->OnPurgeCacheURL(this,
					     m_pURL,
					     m_pAdditional);
	}
	else
	{
	    CDWPluginCallback* cb = 
		new CDWPluginCallback(ON_PURGE_CACHE_URL,
				      this,
				      m_pURL,
				      m_pAdditional,
				      m_pProc->procnum()); // our procnum for reply

	    m_pProc->pc->dispatchq->send(m_pProc,
					 cb,
					 pElem->m_iProcnum);	// send-to procnum
	}
    }
    else
    {
	// No more plugins to enum.  Clean up, call back to our response object

	delete m_pIter;
	m_pIter = 0;

	HX_RELEASE(m_pURL);
	HX_RELEASE(m_pAdditional);


	IHXContentDistributionAdviseResponse* pResp = m_pResp;
	m_pResp = NULL;

	pResp->OnCacheResultDone(HXR_OK);
	pResp->Release();

	Release();
    }
    return HXR_OK;
}

/***********************************************************************
 *
 * Standard IUnknown implementations
 * 
*/
STDMETHODIMP
CDistAdviseWrapper::QueryInterface(REFIID riid, void** ppvObj)
{
    CDPRINTF(CDIST_D_ENTRY, ("Entering CDistAdviseWrapper::QueryInterface (%p)\n", this));

    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)(IHXContentDistributionAdvise*) this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXContentDistributionAdvise))
    {
        AddRef();
        *ppvObj = (IHXContentDistributionAdvise*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXContentDistributionAdviseResponse))
    {
        AddRef();
        *ppvObj = (IHXContentDistributionAdviseResponse*)this;
        return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32)
CDistAdviseWrapper::AddRef()
{
    CDPRINTF(CDIST_D_ENTRY, ("Entering CDistAdviseWrapper::AddRef (%p)\n", this));
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32)
CDistAdviseWrapper::Release()
{
    CDPRINTF(CDIST_D_ENTRY, ("Entering CDistAdviseWrapper::Release (%p)\n", this));
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }
    
    delete this;
    return 0;
}

/********************************************************
 * IHXContentDistributionAdviseResponse methods
 */

STDMETHODIMP
CDistAdviseWrapper::OnLocalResultDone(THIS_
		  HX_RESULT status)
{
    CDPRINTF(CDIST_D_ENTRY, ("Entering CDistAdviseWrapper::OnLocalResultDone (%p)\n", this));

    ++(*m_pIter);
    NextOnLocalResult();
    return HXR_OK;
}

STDMETHODIMP
CDistAdviseWrapper::OnCacheRequestDone(THIS_
		   HX_RESULT status)
{
    CDPRINTF(CDIST_D_ENTRY, ("Entering CDistAdviseWrapper::OnCacheRequestDone (%p)\n", this));

    ++(*m_pIter);
    NextOnCacheRequest();
    return HXR_OK;
}

STDMETHODIMP
CDistAdviseWrapper::OnCacheResultDone(THIS_
				      HX_RESULT status)
{
    CDPRINTF(CDIST_D_ENTRY, ("Entering CDistAdviseWrapper::OnCacheResultDone (%p)\n", this));

    ++(*m_pIter);
    NextOnCacheResult();
    return HXR_OK;
}

STDMETHODIMP
CDistAdviseWrapper::OnPurgeCacheURLDone(THIS_
					HX_RESULT status)
{
    CDPRINTF(CDIST_D_ENTRY, ("Entering CDistAdviseWrapper::OnPurgeCacheURLDone (%p)\n", this));

    ++(*m_pIter);
    NextOnPurgeCacheURL();
    return HXR_OK;
}

STDMETHODIMP
CDistAdviseWrapper::OnSiteCacheResultDone(THIS_
					  HX_RESULT status)
{
    CDPRINTF(CDIST_D_ENTRY, ("Entering CDistAdviseWrapper::OnSiteCacheResultDone (%p)\n", this));
    return HXR_NOTIMPL;
}

/*************************************************************************
 * Elements for plugin list
 */
CDWPluginListElem::CDWPluginListElem(IHXContentDistributionAdvise* pCDAdvise)
    : m_pCDAdvise(pCDAdvise)
    , m_bLoadMultiple(TRUE)
    , m_iProcnum(0)
    , m_lPriority(0)
{ 
    CDPRINTF(D_STATE, ("Constructing CDWPluginListElem (%p)\n", this));
    if (pCDAdvise) pCDAdvise->AddRef();
}

// These shouldn't be deleted, but here's a destructor anyway.
CDWPluginListElem::~CDWPluginListElem()
{ 
    CDPRINTF(D_STATE, ("Destructing CDWPluginListElem (%p)\n", this));
    ASSERT(0);
    HX_RELEASE(m_pCDAdvise); 
}

/*************************************************************************
 * Callback object for responses from advise thread to streamer.
 */

CDWStreamerCallback::CDWStreamerCallback(CDW_RESPONSE_TYPE type,
					 HX_RESULT result,
					 IHXContentDistributionAdviseResponse* pResp)
    : m_Type(type)
    , m_iResult(result)
    , m_pResp(pResp)
{
    CDPRINTF(D_STATE, ("Constructing CDWStreamerCallback (%p)\n", this));
}

CDWStreamerCallback::~CDWStreamerCallback()
{
    CDPRINTF(D_STATE, ("Destructing CDWStreamerCallback (%p)\n", this));
}

void
CDWStreamerCallback::func(Process* proc)
{
    CDPRINTF(CDIST_D_ENTRY, ("Entering CDWStreamerCallback::func (%p)\n", this));

    switch(m_Type)
    {
    case ON_LOCAL_RESULT_DONE:
	m_pResp->OnLocalResultDone(m_iResult);
	break;

    case ON_CACHE_REQUEST_DONE:
	m_pResp->OnCacheRequestDone(m_iResult);
	break;

    case ON_CACHE_RESULT_DONE:
	m_pResp->OnCacheResultDone(m_iResult);
	break;

    case ON_SITE_CACHE_RESULT_DONE:
	m_pResp->OnSiteCacheResultDone(m_iResult);
	break;

    case ON_PURGE_CACHE_URL_DONE:
	m_pResp->OnPurgeCacheURLDone(m_iResult);
	break;
    }

    delete this;
}

/*************************************************************************
 * Callback object for calls from streamer to separate advise thread.
 */
CDWPluginCallback::CDWPluginCallback(CDW_CALL_TYPE type,
				     IHXContentDistributionAdviseResponse* pResp,
				     IHXRequest* pRequest,
				     BOOL found,
				     int procnum)
    : m_Type(type)
    , m_pRequest(pRequest)
    , m_pResp(pResp)
    , m_bFound(found)

    , m_pURL(NULL)
    , m_pAdditional(NULL)

    , m_lRefCount(0)
    , m_iProcnum(procnum)
{
    CDPRINTF(D_STATE, ("Constructing CDWPluginCallback (%p)\n", this));

    // CDistAdviseWrapper addref's itself (pResp) and the request, so 
    // we don't need to addref/release them here.  If you add references
    // to either after calling m_pResp->OnXxxDone() in func(), then you might 
    // need addref's here.
}

CDWPluginCallback::CDWPluginCallback(CDW_CALL_TYPE 	type,
				     IHXContentDistributionAdviseResponse* pResp,
				     IHXBuffer*	pURL,
				     IHXValues*	pAdditional,
				     int 		procnum)
    : m_Type(type)
    , m_pRequest(NULL)
    , m_pResp(pResp)

    , m_pURL(pURL)
    , m_pAdditional(pAdditional)

    , m_lRefCount(0)
    , m_iProcnum(procnum)
{
    CDPRINTF(D_STATE, ("Constructing CDWPluginCallback (%p)\n", this));

    // CDistAdviseWrapper addref's itself (pResp) and the request, so 
    // we don't need to addref/release them here.  If you add references
    // to either after calling m_pResp->OnXxxDone() in func(), then you might 
    // need addref's here.

    // ditto for pURL and pAdditional.
}

CDWPluginCallback::~CDWPluginCallback()
{
    // See note in constructors before adding Release()s here.

    CDPRINTF(D_STATE, ("Destructing CDWPluginCallback (%p)\n", this));
}


/*
 * Note: this object is deleted via reference counting, usually
 * in the response callback.  (Ie, func() doesn't call "delete this".)
 */
void
CDWPluginCallback::func(Process* proc)
{
    CDPRINTF(CDIST_D_ENTRY, ("Entering CDWPluginCallback::func (%p)\n", this));

    AddRef();

    m_pProc = proc;

    // I store an instance of the plugin in the proc container, so we
    // don't have to instantiate one every time through.

    MiscContainer* pMisc = (MiscContainer*)proc->pc;
    pMisc->m_pInstance->QueryInterface(IID_IHXContentDistributionAdvise,
				       (void**)&m_pCDAdvise);
    ASSERT(m_pCDAdvise);

    HX_RESULT hresult;

    switch(m_Type)
    {
    case ON_LOCAL_RESULT:
	hresult = m_pCDAdvise->OnLocalResult(this,
					     m_pRequest,
					     m_bFound);
	if (HXR_OK != hresult)
	{
	    OnLocalResultDone(hresult);
	}
	break;

    case ON_CACHE_REQUEST:
	hresult = m_pCDAdvise->OnCacheRequest(this,
					      m_pRequest);
	if (HXR_OK != hresult)
	{
	    OnCacheRequestDone(hresult);
	}
	break;

    case ON_CACHE_RESULT:
	hresult = m_pCDAdvise->OnCacheResult(this,
					     m_pRequest,
					     m_bFound);
	if (HXR_OK != hresult)
	{
	    OnCacheResultDone(hresult);
	}
	break;

    case ON_SITE_CACHE_RESULT:
	hresult = m_pCDAdvise->OnSiteCacheResult(this,
						 m_pRequest,
						 m_bFound);
	if (HXR_OK != hresult)
	{
	    OnSiteCacheResultDone(hresult);
	}
	break;

    case ON_PURGE_CACHE_URL:
	hresult = m_pCDAdvise->OnPurgeCacheURL(this,
					       m_pURL,
					       m_pAdditional);
	if (HXR_OK != hresult)
	{
	    OnPurgeCacheURLDone(hresult);
	}
	break;

    default:
	HX_RELEASE(m_pCDAdvise);
	Release();
	break;
    }
}

/*
 * Standard IUnknown implementations
 */
STDMETHODIMP
CDWPluginCallback::QueryInterface(REFIID riid, void** ppvObj)
{
    CDPRINTF(CDIST_D_ENTRY, ("Entering CDWPluginCallback::QueryInterface (%p)\n", this));

    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*) this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXContentDistributionAdviseResponse))
    {
        AddRef();
        *ppvObj = (IHXContentDistributionAdviseResponse*)this;
        return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32)
CDWPluginCallback::AddRef()
{
    CDPRINTF(CDIST_D_ENTRY, ("Entering CDWPluginCallback::AddRef (%p)\n", this));
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32)
CDWPluginCallback::Release()
{
    CDPRINTF(CDIST_D_ENTRY, ("Entering CDWPluginCallback::Release (%p)\n", this));
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }
    
    delete this;
    return 0;
}

/********************************************************
 * IHXContentDistributionAdviseResponse methods
 */

STDMETHODIMP
CDWPluginCallback::OnLocalResultDone(THIS_
				     HX_RESULT status)
{
    CDPRINTF(CDIST_D_ENTRY, ("Entering CDWPluginCallback::OnLocalResultDone (%p)\n", this));

    CDWStreamerCallback* cb = 
	new CDWStreamerCallback(ON_LOCAL_RESULT_DONE,
				status,
				m_pResp);

    m_pProc->pc->dispatchq->send(m_pProc,
                                 cb,
                                 m_iProcnum);

    HX_RELEASE(m_pCDAdvise);
    Release();

    return HXR_OK;
}

STDMETHODIMP
CDWPluginCallback::OnCacheRequestDone(THIS_
				      HX_RESULT status)
{
    CDPRINTF(CDIST_D_ENTRY, ("Entering CDWPluginCallback::OnCacheRequestDone (%p)\n", this));

    CDWStreamerCallback* cb = 
	new CDWStreamerCallback(ON_CACHE_REQUEST_DONE,
				status,
				m_pResp);

    m_pProc->pc->dispatchq->send(m_pProc,
                                 cb,
                                 m_iProcnum);

    HX_RELEASE(m_pCDAdvise);
    Release();

    return HXR_OK;
}

STDMETHODIMP
CDWPluginCallback::OnCacheResultDone(THIS_
				     HX_RESULT status)
{
    CDPRINTF(CDIST_D_ENTRY, ("Entering CDWPluginCallback::OnCacheResultDone (%p)\n", this));

    CDWStreamerCallback* cb = 
	new CDWStreamerCallback(ON_CACHE_RESULT_DONE,
				status,
				m_pResp);

    m_pProc->pc->dispatchq->send(m_pProc,
                                 cb,
                                 m_iProcnum);

    HX_RELEASE(m_pCDAdvise);
    Release();

    return HXR_OK;
}

STDMETHODIMP
CDWPluginCallback::OnSiteCacheResultDone(THIS_
					 HX_RESULT status)
{
    CDPRINTF(CDIST_D_ENTRY, ("Entering CDWPluginCallback::OnSiteCacheResultDone "
		       "(%p)\n", this));

    CDWStreamerCallback* cb = 
	new CDWStreamerCallback(ON_SITE_CACHE_RESULT_DONE,
				status,
				m_pResp);

    m_pProc->pc->dispatchq->send(m_pProc,
                                 cb,
                                 m_iProcnum);

    HX_RELEASE(m_pCDAdvise);
    Release();

    return HXR_OK;
}

STDMETHODIMP
CDWPluginCallback::OnPurgeCacheURLDone(THIS_
				       HX_RESULT status)
{
    CDPRINTF(CDIST_D_ENTRY, ("Entering CDWPluginCallback::OnPurgeCacheURLDone (%p)\n", this));

    CDWStreamerCallback* cb = 
	new CDWStreamerCallback(ON_PURGE_CACHE_URL_DONE,
				status,
				m_pResp);

    m_pProc->pc->dispatchq->send(m_pProc,
                                 cb,
                                 m_iProcnum);

    HX_RELEASE(m_pCDAdvise);
    Release();

    return HXR_OK;
}
