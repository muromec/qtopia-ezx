/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: imalloc.cpp,v 1.3 2003/09/04 22:35:34 dcollins Exp $ 
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

#include <string.h>

#include "hxtypes.h"
#include "hxcom.h"

#include "imalloc.h"
#include "shmem.h"

STDMETHODIMP IMallocContext::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)(IMalloc*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IMalloc))
    {
        AddRef();
        *ppvObj = (IMalloc*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXFastAlloc))
    {
        AddRef();
        *ppvObj = (IHXFastAlloc*)this;
        return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32) IMallocContext::AddRef()
{
    return InterlockedIncrement(&m_ulRefCount);
}

STDMETHODIMP_(ULONG32) IMallocContext::Release()
{
    if (InterlockedDecrement(&m_ulRefCount) > 0)
    {
        return m_ulRefCount;
    }

    delete this;
    return 0;
}

STDMETHODIMP_(void*)
IMallocContext::Alloc (UINT32 count)
{
    return (void *)new char[count];
}

STDMETHODIMP_(void*)
IMallocContext::Realloc (void* pMem, UINT32 count)
{
    delete[] (char*)pMem;
    return (void *)new char[count];
}

STDMETHODIMP_(void)
IMallocContext::Free (void* pMem)
{
    delete[] (char*)pMem;
}

STDMETHODIMP_(UINT32)
IMallocContext::GetSize (void* pMem)
{
    return 0; // XXXSMP
}

STDMETHODIMP_(BOOL)
IMallocContext::DidAlloc (void* pMem)
{
    return 0; // XXXSMP
}

STDMETHODIMP_(void)
IMallocContext::HeapMinimize ()
{
    return;
}

STDMETHODIMP_(void*)
IMallocContext::FastAlloc (UINT32 count)
{
    return (void *)m_pCache->CacheNew(count);
}

STDMETHODIMP_(void)
IMallocContext::FastFree (void* pMem)
{
    m_pCache->CacheDelete((char *)pMem);
}
