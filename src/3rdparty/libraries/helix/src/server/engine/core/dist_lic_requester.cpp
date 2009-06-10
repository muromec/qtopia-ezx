/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: dist_lic_requester.cpp,v 1.2 2003/01/23 23:42:54 damonlan Exp $ 
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hxtypes.h"
#include "hxcom.h"

#include "debug.h"

#include "hxmap.h"
#include "dispatchq.h"
#include "proc.h"
#include "mutex.h"
#include "servlist.h"
#include "id.h"

#include "base_errmsg.h"
#include "hxstrutl.h"

#include "dist_lic_requester.h"

#define DL_AWAITING_LICENSE 0x01
#define DL_GOT_LICENSE 0x02
#define DL_DENIED_LICENSE 0x03

DistributedLicenseRequester::DistributedLicenseRequester()
    : m_nLockedBy(-1)
    , m_ulCount(0)
{
    m_pMutex = HXMutexCreate();
    HXMutexInit(m_pMutex);

    m_pLicenseRequesterList = new HXList;
    // m_pCIDMap = new CHXMapLongToObj;
    m_pCbMap = new CHXMapPtrToPtr;
}

// doesn't actually get called since we never release this object
DistributedLicenseRequester::~DistributedLicenseRequester()
{
    // aak -- should never reach here in the normal run of the server
    HX_DELETE(m_pLicenseRequesterList);
    // HX_DELETE(m_pCIDMap);
    HX_DELETE(m_pCbMap);
    HX_ASSERT(m_nLockedBy == 0);
    HXMutexDestroy(m_pMutex);
}

DistLicReqStatus
DistributedLicenseRequester::LicenseRequestStatus(ULONG32 ulCID, Process* pProc)
{
    PL_LOCK(pProc);
    LicenseRequesterListElem* pElem = 0;
    DistLicReqStatus dummy = DL_NOT_REGISTERED;
    if (TRUE == m_pCIDMap->Lookup(ulCID, (void *&)pElem))
#ifndef XXXAAK_DIST_LIC_REQ_DEBUG
	dummy = pElem->m_eDLRS;
#else
    {
	if (pElem->m_eDLRS == DL_AWAITING_LICENSE)
	{
	    fprintf(stderr, "DLR(%p)::LRM(%lu) -- awaiting\n", this, ulCID);
	}
	else if (pElem->m_eDLRS == DL_GOT_LICENSE)
	{
	    fprintf(stderr, "DLR(%p)::LRM(%lu) -- allow\n", this, ulCID);
	}
	else if (pElem->m_eDLRS == DL_DENIED_LICENSE)
	{
	    fprintf(stderr, "DLR(%p)::LRM(%lu) -- deny\n", this, ulCID);
	}
	else
	{
	    fprintf(stderr, "DLR(%p)::LRM(%lu) -- houston we have a problem\n", 
		this, ulCID);
	}
	dummy = pElem->m_eDLRS;
    }
    else
	fprintf(stderr, "DLR(%p)::LRM(%lu) -- not registered\n", this, ulCID);
#endif
    PL_UNLOCK(pProc);
    return dummy;
}

/*
 * put the callback in the m_pCIDMap and the lic requester list
 * return the ulCID
 */
DistLicReqStatus
DistributedLicenseRequester::Insert(ULONG32 ulCID, IHXCallback* pCb, 
    IHXCallback*& pParentCb, Process* pProc)
{
    PL_LOCK(pProc);

    // if a lic request was already made then just return
    DistLicReqStatus ret = (DistLicReqStatus)DL_AWAITING_LICENSE;

    // new client so create a new entry
    LicenseRequesterCallback* pLRCb = new LicenseRequesterCallback(pCb);
    LicenseRequesterListElem* pElem 
	= new LicenseRequesterListElem(ulCID, pLRCb, pCb, 
	    (DistLicReqStatus)DL_AWAITING_LICENSE, pProc);
    m_pLicenseRequesterList->insert(pElem);
    // (*m_pCIDMap)[ulCID] = (void *)pElem;
    (*m_pCbMap)[pCb] = (void *)pElem;
    m_ulCount++;
#ifdef XXXAAK_DIST_LIC_REQ_DEBUG
    fprintf(stderr, "DLR::I(cid(%d), cb(%p)) -- count(%ld)\n", 
	ulCID, pCb, m_ulCount);
#endif

    PL_UNLOCK(pProc);
    return ret;
}

/*
 * try and execute nNumLicenses number of callbacks
 * return back the (nNumLicenses - num of fired callbacks)
 */
INT32
DistributedLicenseRequester::Execute(INT32 nNumLicenses, Process* pProc)
{
    PL_LOCK(pProc);

    if (!m_ulCount)
    {
	PL_UNLOCK(pProc);
	return HXR_OK;
    }

    HXList_iterator iter(m_pLicenseRequesterList);
    int j;
    for (j = 0; *iter != 0 && j < nNumLicenses && m_ulCount > 0; 
	++iter, j++, m_ulCount--)
    {
	LicenseRequesterListElem* pElem = (LicenseRequesterListElem *)*iter;
#ifdef XXXAAK_DIST_LIC_REQ_DEBUG
	fprintf(stderr, "DLR::E(nl(%d), pElem(%p)->cid(%lu)) - proc(num(%d), "
	    "pid(%d)) -- send to proc(num(%d), pid(%d)\n", 
	    nNumLicenses, pElem, pElem->m_ulCID, pProc->procnum(),
	    pProc->procid(pProc->procnum()),
	    pElem->m_pProc->procnum(),
	    pElem->m_pProc->procid(pElem->m_pProc->procnum()));
#endif
	pElem->m_pCb->AddRef();
	pElem->m_pCb->SetStatus((DistLicReqStatus)DL_GOT_LICENSE, pProc);
	pProc->pc->dispatchq->send(pProc, pElem->m_pCb, 
	    pElem->m_pProc->procnum());
	// since now the cb is going to b executed anyways 
	// and it deletes itself
	m_pLicenseRequesterList->remove(pElem);
	pElem->m_bCbDispatched = TRUE;
	// pElem->m_pCb = 0;
    }

    /* 
     * 0 --- all nNumLicenses callbacks fired
     * > 0 - there were less new clients than the nNumLicenses
     */
    INT32 nRemainingLic = nNumLicenses-j;
#ifdef XXXAAK_DIST_LIC_REQ_DEBUG
    fprintf(stderr, "DLR::E() - remain(%ld)\n", nRemainingLic);
#endif

    PL_UNLOCK(pProc);
    return nRemainingLic;
}

HX_RESULT
DistributedLicenseRequester::Remove(ULONG32 ulCID, IHXCallback* pCb, Process* pProc)
{
    PL_LOCK(pProc);
    HX_RESULT ret = HXR_OK;
    LicenseRequesterListElem* pElem = 0;
#ifdef XXXAAK_DIST_LIC_REQ_DEBUG
    if (ulCID && TRUE == m_pCIDMap->Lookup(ulCID, (void *&)pElem))
    {
	fprintf(stderr, "DLR::R(cid(%d)) -- found in cid map!\n", ulCID);
	m_pCIDMap->RemoveKey(ulCID);
    }
#endif
    if (pCb && TRUE == m_pCbMap->Lookup(pCb, (void *&)pElem))
    {
#ifdef XXXAAK_DIST_LIC_REQ_DEBUG
	fprintf(stderr, "DLR::R(cid(%d)) -- found in cb map!\n", ulCID);
#endif
	m_pCbMap->RemoveKey(pCb);
    }
    HXList_iterator iter(m_pLicenseRequesterList);
    for ( ; *iter != 0; ++iter)
    {
	LicenseRequesterListElem* pElem1 
	    = (LicenseRequesterListElem *)*iter;
	if (pElem == pElem1)
	{
#ifdef XXXAAK_DIST_LIC_REQ_DEBUG
	    fprintf(stderr, "DLR::R(cid(%d)) -- found in list!\n", ulCID);
#endif
	    if (!pElem->m_bCbDispatched)
	    {
		pElem->m_pCb->AddRef();
		pElem->m_pCb->SetStatus((DistLicReqStatus)DL_DENIED_LICENSE, pProc);
		pProc->pc->dispatchq->send(pProc, pElem->m_pCb, 
		    pElem->m_pProc->procnum());
#ifdef XXXAAK_DIST_LIC_REQ_DEBUG
		fprintf(stderr, "DLR::R(cid(%d)) -- dispatched LRC()\n",
		    pElem->m_ulCID);
#endif
	    }
	    m_pLicenseRequesterList->remove(pElem);
	    m_ulCount--;
	    break;
	}
    }
#ifdef XXXAAK_DIST_LIC_REQ_DEBUG
    fprintf(stderr, "DLR::R(cid(%d)) -- deleting LRLE(%p)\n", ulCID, pElem);
#endif
    if (pElem)
	delete pElem;
    else
	ret = HXR_FAIL;

    PL_UNLOCK(pProc);
    return ret;
}

/*
 * RemoveAll() gets called from within the subscriber's process which
 * is different from the process which Insert()ed the callback in the
 * list.
 */
HX_RESULT
DistributedLicenseRequester::RemoveAll(Process* pProc)
{
    PL_LOCK(pProc);

#ifdef XXXAAK_DIST_LIC_REQ_DEBUG
    fprintf(stderr, "DLR::RA() -- count(%lu)\n", m_ulCount);
#endif
    if (m_ulCount)
    {
	HXList_iterator	iter(m_pLicenseRequesterList);
	for (; *iter != 0; ++iter)
	{
	    LicenseRequesterListElem* pElem = (LicenseRequesterListElem *)*iter;
	    LicenseRequesterListElem* pDummy = 0;
#ifdef XXXAAK_DIST_LIC_REQ_DEBUG
	    if (TRUE == m_pCIDMap->Lookup(pElem->m_ulCID, (void *&)pDummy))
		m_pCIDMap->RemoveKey(pElem->m_ulCID);
#endif
	    if (TRUE == m_pCbMap->Lookup(pElem->m_pCb->m_pCb, (void *&)pDummy))
		m_pCbMap->RemoveKey(pElem->m_pCb->m_pCb);
	    if (!pElem->m_bCbDispatched)
	    {
		pElem->m_pCb->AddRef();
		pElem->m_pCb->SetStatus((DistLicReqStatus)DL_DENIED_LICENSE, pProc);
		pProc->pc->dispatchq->send(pProc, pElem->m_pCb, 
		    pElem->m_pProc->procnum());
#ifdef XXXAAK_DIST_LIC_REQ_DEBUG
		fprintf(stderr, "DLR::RA(cid(%d)) -- dispatched LRC()\n",
		    pElem->m_ulCID);
#endif
	    }
	    m_pLicenseRequesterList->remove(pElem);
	    m_ulCount--;
#ifdef XXXAAK_DIST_LIC_REQ_DEBUG
	    fprintf(stderr, "DLR::R(cid(%d)) -- deleting LRLE(%p)\n", 
		pElem->m_ulCID, pElem);
#endif
	    HX_DELETE(pElem);
	}
    }

    PL_UNLOCK(pProc);
    return HXR_OK;
}

UINT32
DistributedLicenseRequester::Count(Process* pProc)
{
    PL_LOCK(pProc);
    UINT32 ulCount = m_ulCount;
#ifdef XXXAAK_DIST_LIC_REQ_DEBUG
    fprintf(stderr, "%d: DLR(%p)::Count(%lu)\n", 
	pProc->procid(pProc->procnum()), this, ulCount);
#endif
    PL_UNLOCK(pProc);
    return ulCount;
}

/*
 * LicenseRequestcallback methods
 */
LicenseRequesterCallback::LicenseRequesterCallback(IHXCallback* pCb)
    : SimpleCallback()
    , m_pCb(pCb)
    , m_pDLRS(0)
    , m_eLicStatus((DistLicReqStatus)DL_AWAITING_LICENSE)
    , m_lRefCount(0)
    , m_nLockedBy(-1)
{
    if (pCb)
    {
	pCb->AddRef();
	if (HXR_OK == pCb->QueryInterface(IID_IHXDistributedLicenseRequestStatus,
	    (void **)&m_pDLRS))
	{
	    m_pDLRS->SetStatus((DistLicReqStatus)DL_AWAITING_LICENSE);
	}
    }
    m_pMutex = HXMutexCreate();
    HXMutexInit(m_pMutex);
#ifdef XXXAAK_DIST_LIC_REQ_DEBUG
    fprintf(stderr, "LRC(%p)::LRC()\n", this);
#endif
}

LicenseRequesterCallback::~LicenseRequesterCallback()
{
    HXMutexDestroy(m_pMutex);
#ifdef XXXAAK_DIST_LIC_REQ_DEBUG
    fprintf(stderr, "LRC(%p)::~LRC() -- end\n", this);
#endif
}

INT32
LicenseRequesterCallback::AddRef()
{
#ifdef XXXAAK_DIST_LIC_REQ_DEBUG
    fprintf(stderr, "LRC(%p)::AddRef(%ld)\n", this, m_lRefCount+1);
#endif
    return HXAtomicIncRetINT32(&m_lRefCount);
}   

INT32
LicenseRequesterCallback::Release()
{
    if (HXAtomicDecRetINT32(&m_lRefCount) > 0)
    {
#ifdef XXXAAK_DIST_LIC_REQ_DEBUG
	fprintf(stderr, "LRC(%p)::Release(%ld)\n", this, m_lRefCount);
#endif
        return m_lRefCount;
    }
    
#ifdef XXXAAK_DIST_LIC_REQ_DEBUG
    fprintf(stderr, "LRC(%p)::Release(%ld) - b4 deleting this\n", this, m_lRefCount);
#endif
    delete this;
    return 0;
}   

void
LicenseRequesterCallback::SetStatus(DistLicReqStatus eLicStatus, Process* pProc)
{
    LRC_LOCK(pProc);
    m_pDLRS->SetStatus(eLicStatus);
    LRC_UNLOCK(pProc);
}

void
LicenseRequesterCallback::func(Process* pProc)
{
    LRC_LOCK(pProc);
#ifdef XXXAAK_DIST_LIC_REQ_DEBUG
    fprintf(stderr, "LRC(%p)::Func() -- b4 m_pCb(%p)->Func(proc(num(%d), "
	"pid(%d))\n", this, m_pCb, pProc->procnum(),
	pProc->procid(pProc->procnum()));
#endif
    m_pCb->Func();
    HX_RELEASE(m_pCb);
    HX_RELEASE(m_pDLRS);
    LRC_UNLOCK(pProc);
    // instead of 'delete this'
    Release();
}
