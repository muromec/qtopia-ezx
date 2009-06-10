/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: resolver_dispatch.h,v 1.3 2004/05/03 19:02:48 tmarshall Exp $ 
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

#ifndef _RESOLVER_DISPATCH_H_
#define _RESOLVER_DISPATCH_H_

#include "simple_callback.h"
#include "hxslist.h"
#include "hxstrutl.h"

class ResolverCallback : public SimpleCallback
{
public:
    virtual void	func(Process *proc) = 0;
    ULONG32		host_ip;
};

class PendingRequest
{
public:
    PendingRequest(Process* proc, char* host_name, ResolverCallback* cb);
    ~PendingRequest();

    Process*		m_proc;
    char*		m_host_name;
    ResolverCallback*	m_cb;
    PendingRequest* m_pBack;
    PendingRequest* m_pNext;
};

inline
PendingRequest::PendingRequest(Process* proc, char* host_name, ResolverCallback* cb)
{
    m_proc = proc;
    m_host_name = new_string(host_name);
    m_cb = cb;
    m_pNext = NULL;
    m_pBack = NULL;
}

inline
PendingRequest::~PendingRequest()
{
    delete[] m_host_name;
}

class ResolverDispatch {
public:
    	    	    	ResolverDispatch(Process* proc);
    int			Send(Process* proc, char* host_name, ResolverCallback* resolver_cb);
    void		process();
    BOOL		waiting();
    void		clearing_waiters();
private:
    class ResolverRequestCallback : public SimpleCallback
    {
    public:
	void			func(Process *proc);
	char*			host_name;
	int			calling_procnum;
	ResolverCallback*	resolver_cb;
    private:
			~ResolverRequestCallback() {};

    };

    Process*		m_proc;
    PendingRequest*	m_pWaiters;
    UINT32		m_ulWaiting;
public:
    UINT32		m_ulCreating;
private:
    BOOL		m_bCallbackPending;
};

inline
ResolverDispatch::ResolverDispatch(Process* proc)
{
    m_proc = proc;
    m_bCallbackPending = FALSE;
    m_pWaiters = NULL;
    m_ulWaiting = 0;
    m_ulCreating = 0;
}

inline BOOL
ResolverDispatch::waiting()
{
    return m_pWaiters && !m_bCallbackPending;
}

inline void
ResolverDispatch::clearing_waiters()
{
    m_bCallbackPending = TRUE;
}
#endif
