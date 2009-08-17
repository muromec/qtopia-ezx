/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: qos_sig_bus_ctl.cpp,v 1.15 2003/09/06 03:29:57 jc Exp $ 
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
#include "hxcom.h"
#include "ihxpckts.h"
#include "hxqosinfo.h"
#include "hxqossig.h"
#include "hxqos.h"

#include "mutex.h"
#include "chxmapstringtoob.h"
#include "servlist.h"
#include "timeval.h"

#include "streamer_container.h"
#include "proc.h"
#include "proc_container.h"
#include "globals.h"
#include "streamer_info.h"

#include "qos_sig_bus_ctl.h"
#include "qos_sig_bus.h"

/* QoSSignalBusController Implementation */
QoSSignalBusController::QoSSignalBusController():
    m_lRefCount(0)
{
    m_pStreamerProcs = new Process* [MAX_THREADS];
    memset(m_pStreamerProcs, 0, (MAX_THREADS) * sizeof(Process*));
    m_MapLock = HXCreateMutex();
}

QoSSignalBusController::~QoSSignalBusController()
{
    m_ResponseMap.RemoveAll();
    HX_DELETE(m_pStreamerProcs);
    HXDestroyMutex(m_MapLock);
}

HX_RESULT
QoSSignalBusController::AddStreamer (Process* pProc)
{
    if (pProc->pc->process_type != PTStreamer)
    {
	return HXR_FAIL;
    }

    HX_ASSERT((((StreamerContainer*)(pProc->pc))->m_ulStreamerNum) <=
	      MAX_THREADS);
    
    m_pStreamerProcs [(((StreamerContainer*)(pProc->pc))->m_ulStreamerNum)] =
	pProc;

    return HXR_OK;
 }

/* QoSSignalBusController */
HX_RESULT
QoSSignalBusController::CreateSignalBus(Process* pProc, 
					IHXBuffer* pSessionId)
{
    if ((!pSessionId) || (!pSessionId->GetBuffer()) || 
	(pProc->pc->process_type != PTStreamer))
    {
	return HXR_INVALID_PARAMETER;
    }
    
    StreamerContainer* pStreamer = ((StreamerContainer*)(pProc->pc));

    void*            pTestItem = NULL;
    IHXQoSSignalBus* pNewBus = NULL;
    
    /* Do not create a signal bus for this id if one is already in the map*/
    HXMutexLock(pStreamer->m_BusMapLock, TRUE);
    pStreamer->m_BusMap.Lookup(((const char*)(pSessionId->GetBuffer())), pTestItem);
    HXMutexUnlock(pStreamer->m_BusMapLock);

    if (pTestItem)
    {
	return HXR_FAIL;
    }

    pNewBus = (IHXQoSSignalBus*)(new QoSSignalBus(pProc));
    pNewBus->AddRef();

    if (FAILED(pNewBus->Init(pSessionId)))
    {
    	pNewBus->Release();
    	return HXR_FAIL;
    }

    HXMutexLock(pStreamer->m_BusMapLock, TRUE);
    pStreamer->m_BusMap.SetAt(((const char*)(pSessionId->GetBuffer())), (void*)pNewBus);
    HXMutexUnlock(pStreamer->m_BusMapLock);
    
    /* notify responses waiting for the creation of this bus */
    NotifyMapEntry*  pEntry = NULL;
    
    HXMutexLock(m_MapLock, TRUE);
    m_ResponseMap.Lookup(((const char*)(pSessionId->GetBuffer())), (void*&)pEntry);
    HXMutexUnlock(m_MapLock);

    if (pEntry)
    {
	pEntry->Notify(pNewBus);

	HXMutexLock(m_MapLock, TRUE);
	m_ResponseMap.RemoveKey(((const char*)(pSessionId->GetBuffer())));
	HXMutexUnlock(m_MapLock);

	HX_DELETE(pEntry);
    }

    return HXR_OK;
}

HX_RESULT
QoSSignalBusController::DestroySignalBus(Process* pProc, IHXBuffer* pSessionId)
{
    if ((!pSessionId) || (!pSessionId->GetBuffer()) || 
	(pProc->pc->process_type != PTStreamer))
    {
	return HXR_INVALID_PARAMETER;
    }

    StreamerContainer* pStreamer = ((StreamerContainer*)(pProc->pc));
    IHXQoSSignalBus*  pBus = NULL;

    HXMutexLock(pStreamer->m_BusMapLock, TRUE);
    pStreamer->m_BusMap.Lookup(((const char*)(pSessionId->GetBuffer())), (void*&)pBus);
    HXMutexUnlock(pStreamer->m_BusMapLock);

    if (pBus)
    {
	HXMutexLock(pStreamer->m_BusMapLock, TRUE);
	pStreamer->m_BusMap.RemoveKey(((const char*)(pSessionId->GetBuffer())));
	HXMutexUnlock(pStreamer->m_BusMapLock);

	pBus->Close();
	pBus->Release();
	pBus = NULL;
    }

    return HXR_OK;
}

HX_RESULT
QoSSignalBusController::GetSignalBus(Process* pProc, IHXBuffer* pSessionId, 
				     IHXQoSSignalSourceResponse* pResp)
{
    if ((!pSessionId) || (!pSessionId->GetBuffer())
	|| (!pResp))
    {
	return HXR_INVALID_PARAMETER;
    }

    StreamerContainer* pStreamer = NULL;
    IHXQoSSignalBus*   pBus      = NULL;

    if (pProc->pc->process_type == PTStreamer)
    {
	pStreamer = ((StreamerContainer*)(pProc->pc));

	HXMutexLock(pStreamer->m_BusMapLock, TRUE);
	pStreamer->m_BusMap.Lookup(((const char*)(pSessionId->GetBuffer())), (void*&)pBus);
	HXMutexUnlock(pStreamer->m_BusMapLock);
    }
    else
    {
	for (UINT16 i = 0; i < pProc->pc->streamer_info->Number(); i++)
	{
	    if (m_pStreamerProcs[i])
	    {
		pStreamer = ((StreamerContainer*)((m_pStreamerProcs [i])->pc));

		HXMutexLock(pStreamer->m_BusMapLock, TRUE);
		pStreamer->
		    m_BusMap.Lookup(((const char*)(pSessionId->GetBuffer())), (void*&)pBus);
		HXMutexUnlock(pStreamer->m_BusMapLock);

		if (pBus)
		{
		    break;
		}
	    }
	}
    }

    if (pBus)
    {
	pResp->SignalBusReady(HXR_OK, pBus, pSessionId);
    }
    else
    {
	NotifyMapEntry*  pElem = NULL;
	
	HXMutexLock(m_MapLock, TRUE);
	m_ResponseMap.Lookup(((const char*)(pSessionId->GetBuffer())), (void*&)pElem);
	HXMutexUnlock(m_MapLock);
	
	if (pElem)
	{
	    pElem->AddResponse(pResp);
	}
	else
	{
	    pElem = new NotifyMapEntry(pSessionId);
	    pElem->AddResponse(pResp);
	    
	    HXMutexLock(m_MapLock, TRUE);
	    m_ResponseMap.SetAt(((const char*)(pSessionId->GetBuffer())), (void*)pElem);
	    HXMutexUnlock(m_MapLock);
	}
    }

    return HXR_OK;
}



HX_RESULT
QoSSignalBusController::ReleaseResponseObject(Process* pProc, IHXBuffer* pSessionId, 
				     IHXQoSSignalSourceResponse* pResp)
{
	NotifyMapEntry*  pElem = NULL;
	
	HXMutexLock(m_MapLock, TRUE);
	m_ResponseMap.Lookup(((const char*)(pSessionId->GetBuffer())), (void*&)pElem);
	HXMutexUnlock(m_MapLock);
	
	if (pElem)
	{
	    pElem->RemoveResponse(pResp);
	}

    return HXR_OK;
}



/* IUnknown */
STDMETHODIMP
QoSSignalBusController::QueryInterface(REFIID riid, void** ppvObj)
{
    if(IsEqualIID(riid, IID_IUnknown))
    {
	AddRef();
	*ppvObj = (IUnknown*)this;
	return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(UINT32)
QoSSignalBusController::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(UINT32)
QoSSignalBusController::Release()
{
    if(InterlockedDecrement(&m_lRefCount) > 0)
    {
	return m_lRefCount;
    }
    delete this;
    return 0;
}

/* QoSSignalSource Implementation */
QoSSignalSource::QoSSignalSource(Process* pProc, QoSSignalBusController* pCtl):
    m_lRefCount(0),
    m_pCtl(pCtl),
    m_pProc(pProc)
    
{
    if (m_pCtl)
    {
	m_pCtl->AddRef();
    }
}

QoSSignalSource::~QoSSignalSource()
{
    HX_RELEASE(m_pCtl);
}

/* IHXQoSSignalSource */
STDMETHODIMP
QoSSignalSource::GetSignalBus(IHXBuffer* pSessionId, 
			      IHXQoSSignalSourceResponse* pResp)
{
    /* Don't check params, proxied methods will take care of that */
    if (!m_pCtl)
    {
	return HXR_FAIL;
    }

    return m_pCtl->GetSignalBus(m_pProc, pSessionId, pResp);
}

STDMETHODIMP
QoSSignalSource::ReleaseResponseObject(IHXBuffer* pSessionId, 
			      IHXQoSSignalSourceResponse* pResp)
{
    /* Don't check params, proxied methods will take care of that */
    if (!m_pCtl)
    {
	return HXR_FAIL;
    }

    return m_pCtl->ReleaseResponseObject(m_pProc, pSessionId, pResp);
}

/* IUnknown */
STDMETHODIMP
QoSSignalSource::QueryInterface(REFIID riid, void** ppvObj)
{
    if(IsEqualIID(riid, IID_IUnknown))
    {
	AddRef();
	*ppvObj = (IUnknown*)(IHXQoSSignalSource*)this;
	return HXR_OK;
    }
    else if(IsEqualIID(riid, IID_IHXQoSSignalSource))
    {
	AddRef();
	*ppvObj = (IHXQoSSignalSource*)this;
	return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(UINT32)
QoSSignalSource::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(UINT32)
QoSSignalSource::Release()
{
    if(InterlockedDecrement(&m_lRefCount) > 0)
    {
	return m_lRefCount;
    }
    delete this;
    return 0;
}

/* Container Classes */
NotifyMapEntry::NotifyMapEntry (IHXBuffer* pId) :
    m_pId (pId)
{
    if (m_pId)
    {
	m_pId->AddRef();
    }
}
    
NotifyMapEntry::~NotifyMapEntry ()
{
    NotifyListElem* pElem = NULL;
    HXList_iterator j(&m_NotifyList);
    
    for (; *j != 0; ++j)
    {
	m_NotifyList.remove(pElem);
	HX_DELETE(pElem);
    }

    HX_RELEASE(m_pId);
}

void
NotifyMapEntry::AddResponse (IHXQoSSignalSourceResponse* pResp)
{
    if (pResp)
    {
	m_NotifyList.insert(new NotifyListElem(pResp));
    }
}

void
NotifyMapEntry::RemoveResponse (IHXQoSSignalSourceResponse* pResp)
{
    if (pResp)
    {
	NotifyListElem* pElem;
	HXList_iterator j(&m_NotifyList);
	
	for (; *j != 0; ++j)
	{
	    pElem = (NotifyListElem*)(*j);

	    if (pElem->m_pResp == pResp)
	    {
                m_NotifyList.remove(pElem);
                HX_DELETE(pElem);
                break;
	    }
	}
    }
}

void
NotifyMapEntry::Notify (IHXQoSSignalBus* pBus)
{
    if (pBus)
    {
	NotifyListElem* pElem = NULL;
	HXList_iterator j(&m_NotifyList);
	
	for (; *j != 0; ++j)
	{
	    pElem = (NotifyListElem*)(*j);

	    if (pElem->m_pResp)
	    {
		(pBus) ? pElem->m_pResp->SignalBusReady(HXR_OK, pBus, m_pId) :
		    pElem->m_pResp->SignalBusReady(HXR_FAIL, NULL, m_pId); 
	    }
	}
    }
}

NotifyListElem::NotifyListElem(IHXQoSSignalSourceResponse* pResp) :
    m_pResp(pResp)
{
    if (m_pResp)
    {
	m_pResp->AddRef();
    }
}

NotifyListElem::~NotifyListElem()
{
    HX_RELEASE(m_pResp);
}
