/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: iasyncio.cpp,v 1.3 2003/09/04 22:35:34 dcollins Exp $ 
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
#include "hxcomm.h"

#include "proc.h"
#include "server_engine.h"
#include "callback.h"
#include "iasyncio.h"

CAsyncIOSelection::CAsyncIOSelection(Process* proc)
                 : m_pProc(proc)
		 , m_lRefCount(0)
{
}

CAsyncIOSelection::~CAsyncIOSelection()
{
}

STDMETHODIMP
CAsyncIOSelection::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
	AddRef();
	*ppvObj = (IUnknown*) this;
	return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXAsyncIOSelection))
    {
	AddRef();
	*ppvObj = (IHXAsyncIOSelection*) this;
	return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32)
CAsyncIOSelection::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32)
CAsyncIOSelection::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
	return m_lRefCount;
    }
    ASSERT(FALSE);
    delete this;
    return 0;
}

STDMETHODIMP
CAsyncIOSelection::Add(IHXCallback*   pCallback,
		       INT32           lFileDescriptor,
		       UINT32          ulFlags)
{
    pCallback->AddRef(); // XXXSMP Underlying callback class should do this!
    switch(ulFlags)
    {
    case PNAIO_READ:
	m_pProc->pc->engine->callbacks.add(HX_READERS, lFileDescriptor, pCallback);
	break;
    case PNAIO_WRITE:
	m_pProc->pc->engine->callbacks.add(HX_WRITERS, lFileDescriptor, pCallback);
	break;
    case PNAIO_EXCEPTION:
	return HXR_NOTIMPL;
	break;
    }

    return HXR_OK;
}

STDMETHODIMP
CAsyncIOSelection::Remove(INT32 lFileDescriptor, UINT32 ulFlags)
{
    // XXXSMP, THIS CALL SHOULD REMOVE THE DAMN REF ON pCallBack! (Assuming
    // IT ACTUALLY ADDREFED IT!)
    switch(ulFlags)
    {
    case PNAIO_READ:
	m_pProc->pc->engine->callbacks.remove(HX_READERS, lFileDescriptor);
	break;
    case PNAIO_WRITE:
	m_pProc->pc->engine->callbacks.remove(HX_WRITERS, lFileDescriptor);
	break;
    case PNAIO_EXCEPTION:
	return HXR_NOTIMPL;
	break;
    }

    return HXR_OK;
}
