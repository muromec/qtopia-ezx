/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: licensecallback.cpp,v 1.2 2003/01/23 23:42:53 damonlan Exp $ 
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

#include "hxcom.h"
#include "hxengin.h"
#include "hxliccallbk.h"
#include "hxmon.h"
#include "ihxpckts.h"

#include "hxslist.h"
#include "proc.h"
#include "server_context.h"

#include "licensecallback.h"

LicenseCallbackManager::LicenseCallbackManager (Process* pProc):m_lRefCount(0),
                                    m_pRegistry(NULL), m_pWatch(NULL)
{
	m_pCBList = new CHXSimpleList();
    
    pProc->pc->server_context->QueryInterface(IID_IHXRegistry, (void**)&m_pRegistry);
    m_pRegistry->CreatePropWatch(m_pWatch);
    m_pWatch->Init(this);

    m_pWatch->SetWatchOnRoot();
}

LicenseCallbackManager::~LicenseCallbackManager()
{
    CHXSimpleList::Iterator i;

    for (i = m_pCBList->Begin(); i != m_pCBList->End(); ++i)
    {
        ((IHXCallback*)(*i))->Release();
    }
    m_pCBList->RemoveAll();

    m_pWatch->Release();
    m_pRegistry->Release();
}

// IUnknown imps

/////////////////////////////////////////////////////////////////////////
//  Method:
//      IUnknown::AddRef
//  Purpose:
//      Everyone usually implements this the same... feel free to use
//      this implementation.
//
STDMETHODIMP_(UINT32) 
LicenseCallbackManager::AddRef()
{   
    return InterlockedIncrement(&m_lRefCount);
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//      IUnknown::Release
//  Purpose:
//      Everyone usually implements this the same... feel free to use
//      this implementation.
//
STDMETHODIMP_(UINT32) 
LicenseCallbackManager::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
	return m_lRefCount;
    }
    delete this;
    return 0;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//      IUnknown::QueryInterface
//  Purpose:
//      Implement this to export the interfaces supported by your 
//      object.
//
STDMETHODIMP 
LicenseCallbackManager::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
    	AddRef();
	    *ppvObj = (IUnknown*)(IHXPropWatchResponse*)this;
	    return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXPropWatchResponse))
    {
	    AddRef();
    	*ppvObj = (IHXPropWatchResponse*)this;
	    return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXLicenseCallback))
    {
        AddRef();
        *ppvObj = (IHXLicenseCallback*)this;
        return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP LicenseCallbackManager::Register(IHXCallback* pCallback)
{
    HX_ASSERT(m_pCBList);
    HX_ASSERT(pCallback);

    m_pCBList->AddHead((void*)pCallback);
    return HXR_OK;
}

STDMETHODIMP LicenseCallbackManager::DeRegister(IHXCallback* pCallback)
{
    HX_ASSERT(m_pCBList);
    HX_ASSERT(pCallback);
    
    LISTPOSITION pos = m_pCBList->Find(pCallback, NULL);
    if (pos)
    {
       pCallback->Release();
       m_pCBList->RemoveAt(pos);
    }
    return HXR_OK;
}

STDMETHODIMP LicenseCallbackManager::AddedProp(const UINT32 ulID,
                const HXPropType propType, const UINT32 ulParentID)
{
    IHXBuffer* pBuffer = NULL;

    if (propType == PT_COMPOSITE) 
    {
        if (m_pRegistry->GetPropName(ulID, pBuffer) == HXR_OK)
        {
            if (!strncmp((const char*)pBuffer->GetBuffer(), "license", 7))
            {
                CHXSimpleList::Iterator i;

                for (i = m_pCBList->Begin(); i != m_pCBList->End(); ++i)
                {
                    ((IHXCallback*)(*i))->Func();
                    ((IHXCallback*)(*i))->Release();    
                }
                m_pCBList->RemoveAll();
            }
        }
    }
    
    return HXR_OK;
}

STDMETHODIMP LicenseCallbackManager::ModifiedProp(const UINT32 ulID,
                const HXPropType propType, const UINT32 ulParentID)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP LicenseCallbackManager::DeletedProp(const UINT32 ulID, 
                                                 const UINT32 ulParentId)
{
    return HXR_NOTIMPL;
}
