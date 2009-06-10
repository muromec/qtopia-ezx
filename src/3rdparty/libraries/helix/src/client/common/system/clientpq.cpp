/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: clientpq.cpp,v 1.12 2009/02/20 22:43:58 rkondru Exp $
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

/****************************************************************************
 * 
 * Client side Priority Queue Time-Based Scheduler.
 * Out there protecting the world!
 *
 */

#include "pckunpck.h"
#include "clientpq.h"
#include "hxthread.h"
#include "hxthreadyield.h"
#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif


ClientPQ::ClientPQ(IUnknown* pContext, CHXID* pIds, IHXMutex* pMutex)
    : PQ(pIds),
      m_pFreeList(NULL),
      m_uNumFreeNodes(0),
      m_uNumNodesToCache(DEFAULT_NODE_CACHE_SIZE),
      m_pMutex(NULL)
{
    HX_ASSERT(((pIds && pMutex) || (!pIds && !pMutex)) && "Creating thread-unsafe ClientPQ");

    if(pMutex)
    {
        // use externally provided mutex
        m_pMutex = pMutex;
        m_pMutex->AddRef();
    }
    else
    {
        // use private mutex
        CreateInstanceCCF(CLSID_IHXMutex, (void**)&m_pMutex, pContext);  
    }
}


ClientPQ::~ClientPQ()
{
    // protect cleanup
    m_pMutex->Lock();

    // destruct base object under thread-safe lock
    PQ::destruct();

    while(m_pFreeList)
    {
	PQElem* pElem = m_pFreeList;
	m_pFreeList = m_pFreeList->m_pNext;
	delete pElem;
    }

    m_pMutex->Unlock();
    HX_RELEASE(m_pMutex);
}


int ClientPQ::execute(Timeval now)
{
    int     nCount = 0;
    PQElem* pElem  = NULL;
    
    //Protect just the _remove_head call with the mutex.
    m_pMutex->Lock();
    pElem = PQ::get_execute_list(now);

    PQElem* pElemNext = NULL;
    while (pElem) 
    {
        // synch with remove.
        pElem->m_bRemoved = TRUE;

        if(!pElem->m_bDefunct)
            nCount++;

        m_pMutex->Unlock();
        pElemNext = PQ::dispatch_element(pElem);
        m_pMutex->Lock();
        
        PQ::destroy_element(pElem);

        if (!pElemNext)
            break;
        pElem = pElemNext; 
        YieldIfRequired(&m_lastTime);		
    }

    m_pMutex->Unlock();

    return nCount;
}

UINT32 ClientPQ::enter(Timeval tmVal, IHXCallback* pCallBack)
{
    UINT32 unRetVal = 0;
    m_pMutex->Lock();
    unRetVal = PQ::enter(tmVal, pCallBack);
    m_pMutex->Unlock();
    return unRetVal;
}

HXBOOL ClientPQ::removeifexists(UINT32 id)
{
    HXBOOL bReturn = FALSE;

    m_pMutex->Lock();
    
    PQElem* pElem = (PQElem*)m_pIds->get(id);
    if( pElem && !pElem->m_bRemoved)
    {
        bReturn = PQ::removeifexists(id);
        pElem->m_bRemoved = bReturn;
    }

    m_pMutex->Unlock();
    
    return bReturn;    
}   

void
ClientPQ::remove(UINT32 id)
{
    m_pMutex->Lock();

    PQElem* pElem = (PQElem*)m_pIds->get(id);

    if (pElem && !pElem->m_bRemoved)
    {
        HX_VERIFY(PQ::removeifexists(id) == TRUE);
        pElem->m_bRemoved = 1;
    }

    m_pMutex->Unlock();

    //HX_VERIFY(removeifexists(id) == TRUE);
}
