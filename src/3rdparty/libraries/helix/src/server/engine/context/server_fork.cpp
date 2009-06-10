/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: server_fork.cpp,v 1.3 2003/09/04 22:35:34 dcollins Exp $ 
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

#include "hxcomm.h"

#include "proc.h"
#include "server_fork.h"
#include "hxassert.h"

#ifdef _LINUX
extern void exec_as_resolver();
#endif

extern int shared_ready;

ServerFork::ServerFork(Process* proc)
                 : m_pProc(proc)
		 , m_lRefCount(0)
{
}

ServerFork::~ServerFork()
{
}

STDMETHODIMP
ServerFork::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
	AddRef();
	*ppvObj = (IUnknown*)(IHXServerFork*)this;
	return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXServerFork))
    {
	AddRef();
	*ppvObj = (IHXServerFork*) this;
	return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXResolverExec))
    {
	AddRef();
	*ppvObj = (IHXResolverExec*)this;
	return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32)
ServerFork::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32)
ServerFork::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
	return m_lRefCount;
    }
    ASSERT(FALSE);
    delete this;
    return 0;
}

INT32
ServerFork::Fork()
{
#ifdef _UNIX
    INT32 pid = fork();

    switch (pid)
    {
        case 0:     /* Child */
	    shared_ready = 0;

	    return pid;
            break;

        case -1:    /* Error */
	    return -1;
	    break;

	default:    /* Parent */
	    return pid;
	    break;
    }
#else
    return -1;
#endif
}

STDMETHODIMP
ServerFork::ResolverExec(int readfd, int writefd)
{
#ifdef _LINUX
    close(fileno(stdout));
    close(fileno(stdin));
    dup2(readfd, fileno(stdin));
    dup2(writefd, fileno(stdout));

    exec_as_resolver();
#endif
    return HXR_FAIL;
}

