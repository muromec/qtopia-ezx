/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: resolver_dispatch.cpp,v 1.2 2003/01/23 23:42:54 damonlan Exp $ 
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
#include "debug.h"
#include "_main.h"
#include "dispatchq.h"
#include "proc.h"
#include "sockio.h"
#include "resolver_dispatch.h"
#include "resolver_info.h"
#include "resolver_proc.h"
#include "netbyte.h"
#include "server_engine.h"

void
ResolverDispatch::ResolverRequestCallback::func(Process* proc)
{
    DPRINTF(D_INFO, ("Looking up address %s\n", host_name));

    struct hostent* h = 0;
    BOOL bDidUnlock = FALSE;

    if (proc->pc->engine->m_bMutexProtection)
    {
	proc->pc->engine->m_bMutexProtection = FALSE;
	HXMutexUnlock(g_pServerMainLock);
	bDidUnlock = TRUE;
	h = gethostbyname(host_name);
    }
    else
	h = gethostbyname(host_name);

    if (bDidUnlock && !proc->pc->engine->m_bMutexProtection)
    {
	HXMutexLock(g_pServerMainLock);
	proc->pc->engine->m_bMutexProtection = TRUE;
    }

    if (h)
    {
	resolver_cb->host_ip = DwToHost(*(ULONG32*)h->h_addr);
    }
    else
    {
	resolver_cb->host_ip = (ULONG32)-1;
    }

    proc->pc->dispatchq->send(proc, resolver_cb, calling_procnum);

    proc->pc->resolver_info->RequestDone(proc);

    /*
     * Make sure any pending requests get executed
     */

    if (proc->pc->rdispatch->waiting())
    {
	proc->pc->rdispatch->clearing_waiters();
	ResolverProcessReadyCallback* cb = new ResolverProcessReadyCallback;
	proc->pc->dispatchq->send(proc, cb, calling_procnum);
    }

    delete[] host_name;
    delete this;
}

int
ResolverDispatch::Send(Process* proc, char* host_name, ResolverCallback* resolver_cb)
{
    int best;

    best = proc->pc->resolver_info->BestResolver();

    if (best < 0)
    {
	if (m_ulWaiting + 1 > m_ulCreating * RESOLVER_CAPACITY_VALUE)
	{
	    CreateResolverCallback* cb = new CreateResolverCallback;
	    cb->calling_procnum = proc->procnum();
	    m_ulCreating++;
	    proc->pc->dispatchq->send(proc, cb, PROC_RM_CONTROLLER);
	}
	PendingRequest* p = new PendingRequest(proc, host_name, resolver_cb);
	p->m_pBack = NULL;
	p->m_pNext = m_pWaiters;
	if (m_pWaiters)
	    m_pWaiters->m_pBack = p;
	m_pWaiters = p;
	m_ulWaiting++;

	return -1;
    }
    else
    {
	DPRINTF(D_INFO, ("Dispatching request to proc #%d!\n", best));
	proc->pc->resolver_info->RequestStart(proc, best);
	ResolverRequestCallback* cb = new ResolverRequestCallback;
	cb->host_name = new_string(host_name);
	cb->calling_procnum = proc->procnum();
	cb->resolver_cb = resolver_cb;
	proc->pc->dispatchq->send(proc, cb, best);

	return 0;
    }
}

void
ResolverDispatch::process()
{
    PendingRequest* pNext;
    PendingRequest* pThis;
    pThis = m_pWaiters;
    while (pThis)
    {
	pNext = pThis->m_pNext;
	if (pThis->m_pBack)
	{
	    pThis->m_pBack->m_pNext = pThis->m_pNext;
	    if (pThis->m_pNext)
		pThis->m_pNext->m_pBack = pThis->m_pBack;
	}
	else
	{
	    m_pWaiters = pNext;
	    if (pNext)
		pNext->m_pBack = NULL;
	}
	m_ulWaiting--;
	int retval = Send(pThis->m_proc, pThis->m_host_name, pThis->m_cb);
	delete pThis;

	/*
	 * If the Send was not completed then return out
	 */

	if (retval < 0)
	    break;
	
	pThis = pNext;
    }

    m_bCallbackPending = FALSE;
}
