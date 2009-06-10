/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: namedlock.cpp,v 1.1 2004/09/30 02:50:44 jc Exp $ 
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

/*****************************************************************************
** - This class implements a named lock service that is intended to allow 
**   processes to share a mutex easily.
******************************************************************************/
#include <stdio.h>
#include <stdlib.h>

#ifdef _UNIX
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif //_UNIX

#include "hxtypes.h"
#include "hxcom.h"
#include "hxerror.h"
#include "chxpckts.h"	// CHXHeader, CHXValues, CHXBuffer
#include "hxresult.h"	// HX_RESULT
#include "hxmap.h"

#include "imutex.h"
#include "hxnamedlock.h"
#include "namedlock.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static char HX_THIS_FILE[] = __FILE__;
#endif


CNamedLock::CNamedLock(void)
    : m_ulRefCount(0)
    , m_pClassLock(NULL)
{
    m_pClassLock = new Mutex();
    m_pClassLock->AddRef();
}


CNamedLock::~CNamedLock()
{    
    // clean up the map
    CHXMapStringToOb::Iterator i;
    for (i  = m_LockMap.Begin();
	 i != m_LockMap.End();
	 ++i)
    {
        Mutex* pMutex = (Mutex*)(*i);
        HX_RELEASE(pMutex);
    }

    m_LockMap.RemoveAll();

    HX_RELEASE(m_pClassLock);
}


STDMETHODIMP CNamedLock::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
	AddRef();
	*ppvObj = this;
	return HXR_OK;
    }
    if (IsEqualIID(riid, IID_IHXNamedLock))
    {
	AddRef();
	*ppvObj = this;
	return HXR_OK;
    }
    
    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}


STDMETHODIMP_(ULONG32) CNamedLock::AddRef()
{
    return InterlockedIncrement(&m_ulRefCount);
}


STDMETHODIMP_(ULONG32) CNamedLock::Release()
{
    if (InterlockedDecrement(&m_ulRefCount) > 0)
    {
        return m_ulRefCount;
    }

    delete this;
    return 0;
}


STDMETHODIMP 
CNamedLock::GetNamedLock(const char* pszName, IHXMutex** ppMutex)
{
    if (!pszName || !*pszName)
    {
        return HXR_UNEXPECTED;
    }
 
    m_pClassLock->Lock();
    if (m_LockMap.Lookup(pszName, (void*&)*ppMutex))
    {
        // AddRef for this requester
        (*ppMutex)->AddRef();

        m_pClassLock->Unlock();
        return HXR_OK;
    }

    m_pClassLock->Unlock();
    return HXR_UNEXPECTED;
}


STDMETHODIMP 
CNamedLock::CreateNamedLock(const char* pszName)
{
    Mutex* pMutex;

    if (!pszName || !*pszName)
    {
        return HXR_UNEXPECTED;
    }

    m_pClassLock->Lock();
    if (m_LockMap.Lookup(pszName, (void*&)pMutex))
    {
        m_pClassLock->Unlock();
        return HXR_FILE_EXISTS;
    }
    else
    {
        Mutex* pMutex = new Mutex();
        pMutex->AddRef();

        m_LockMap.SetAt(pszName, pMutex);
    } 

    m_pClassLock->Unlock();
    return HXR_OK;
}


STDMETHODIMP 
CNamedLock::DestroyNamedLock(const char* pszName)
{
    if (!pszName || !*pszName)
    {
        return HXR_UNEXPECTED;
    }

    Mutex* pMutex;

    m_pClassLock->Lock();
    if (m_LockMap.Lookup(pszName, (void*&)pMutex))
    {
        HX_RELEASE(pMutex);
        m_LockMap.RemoveKey(pszName);
    }
 
    m_pClassLock->Unlock();
    return HXR_OK;
}

