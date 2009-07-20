/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: idist_lic_requester.cpp,v 1.2 2003/01/23 23:42:53 damonlan Exp $ 
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

#include "idist_lic_requester.h"
#include "dist_lic_requester.h"

HXDistributedLicenseRequester::HXDistributedLicenseRequester(Process* pProc)
    : m_ulRefCount(0)
    , m_pProc(pProc)
    , m_pDistributedLicenseRequester(pProc->pc->license_requester)
{
}

// doesn't actually get called since we never release this object
HXDistributedLicenseRequester::~HXDistributedLicenseRequester()
{
}

STDMETHODIMP
HXDistributedLicenseRequester::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IHXDistributedLicenseRequester))
    {
        AddRef();
        *ppvObj = (IHXRNDistributedLicenseRequester*)this;
        return HXR_OK;
    }
    
    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}   

STDMETHODIMP_(ULONG32)
HXDistributedLicenseRequester::AddRef()
{
    return InterlockedIncrement(&m_ulRefCount);
}   

STDMETHODIMP_(ULONG32)
HXDistributedLicenseRequester::Release()
{
    if (InterlockedDecrement(&m_ulRefCount) > 0)
    {
        return m_ulRefCount;
    }
    
    delete this;
    return 0;
}   

STDMETHODIMP_(DistLicReqStatus)
HXDistributedLicenseRequester::LicenseRequestStatus(ULONG32 ulClientID)
{
    return m_pDistributedLicenseRequester->LicenseRequestStatus(ulClientID,
	m_pProc);
}

STDMETHODIMP_(DistLicReqStatus)
HXDistributedLicenseRequester::Insert(ULONG32 ulClientID, IHXCallback* pCb,
    IHXCallback*& pParentCb)
{
#ifdef XXXAAK_DIST_LIC_REQ_DEBUG
    fprintf(stderr, "RNDLR(%p)::I(ulCID(%lu), pCb(%p)) -- "
	"m_pProc(num(%d), pid(%d))\n", this, ulClientID, pCb,
	m_pProc->procnum(), m_pProc->procid(m_pProc->procnum()));
#endif
    return m_pDistributedLicenseRequester->Insert(ulClientID, pCb, pParentCb,
	m_pProc);
}

STDMETHODIMP_(INT32)
HXDistributedLicenseRequester::Execute(INT32 nNumLicenses)
{
    return m_pDistributedLicenseRequester->Execute(nNumLicenses, m_pProc);
}

STDMETHODIMP
HXDistributedLicenseRequester::Remove(ULONG32 ulClientID, IHXCallback* pCb)
{
    return m_pDistributedLicenseRequester->Remove(ulClientID, pCb, m_pProc);
}

STDMETHODIMP
HXDistributedLicenseRequester::RemoveAll()
{
    return m_pDistributedLicenseRequester->RemoveAll(m_pProc);
}

STDMETHODIMP_(UINT32)
HXDistributedLicenseRequester::Count()
{
    return m_pDistributedLicenseRequester->Count(m_pProc);
}

