/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: imutex.cpp,v 1.2 2003/01/23 23:42:53 damonlan Exp $ 
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

#define HX_MUTEX_USE_MANAGED_LOCKS

#include "imutex.h"

Mutex::Mutex()
{
    m_ulRefCount = 0;
    m_Mutex = HXMutexCreate();
}

Mutex::~Mutex()
{
    HXMutexDestroy(m_Mutex);
}

STDMETHODIMP
Mutex::Lock()
{
#ifdef _AIX
    //AIX was HB failing without the bWait=TRUE flag:
    HXMutexLock(m_Mutex, TRUE);
#else
    //XXXDC this is temporary, we should not be doing tight non-delaying
    // spinlocks here!  FIXME for 9.01!
    HXMutexLock(m_Mutex);
#endif

    return HXR_OK;
}

STDMETHODIMP
Mutex::TryLock()
{
    return HXMutexTryLock(m_Mutex) == TRUE ? HXR_OK : HXR_FAIL;
}

STDMETHODIMP
Mutex::Unlock()
{
    HXMutexUnlock(m_Mutex);

    return HXR_OK;
}


STDMETHODIMP
Mutex::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXMutex))
    {
        AddRef();
	*ppvObj = (IHXMutex*)this;
	return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

ULONG32
Mutex::AddRef()
{
    return InterlockedIncrement(&m_ulRefCount);
}

ULONG32
Mutex::Release()
{
    if (InterlockedDecrement(&m_ulRefCount) > 0)
    {
	return m_ulRefCount;
    }

    delete this;
    return 0;
}
