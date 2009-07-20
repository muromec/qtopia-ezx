/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxnetif.cpp,v 1.6 2007/07/06 21:57:58 jfinnecy Exp $
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

#include "hxcom.h"
#include <string.h>
#include <stdio.h>
#include <time.h>
#include "hxresult.h"
#include "hxslist.h"
#include "netbyte.h"
#include "hxengin.h"
#include "OT_net.h"
#include "hxnetif.h"

HXNetInterface::HXNetInterface(IUnknown* pContext)
		: m_lRefCount(0)
		, m_bInitialized(FALSE)
		, m_pNetInterfaceList(NULL)
		, m_pSinkList(NULL)
{
}

HXNetInterface::~HXNetInterface()
{
    Close();
}

STDMETHODIMP
HXNetInterface::QueryInterface(REFIID riid, void**ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
	AddRef();
	*ppvObj = this;
	return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXNetInterfaces))
    {
	AddRef();
	*ppvObj = (IHXNetInterfaces*)this;
	return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::AddRef
//  Purpose:
//	Everyone usually implements this the same... feel free to use
//	this implementation.
//
STDMETHODIMP_(ULONG32) 
HXNetInterface::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::Release
//  Purpose:
//	Everyone usually implements this the same... feel free to use
//	this implementation.
//
STDMETHODIMP_(ULONG32) 
HXNetInterface::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

STDMETHODIMP
HXNetInterface::UpdateNetInterfaces(void)
{
    HX_RESULT rc = HXR_OK;    
    HXBOOL bChanged = FALSE;

    bChanged = IsNetInterfaceChanged();

    if (!m_bInitialized)
    {
	m_bInitialized = TRUE;
    }
    else if (bChanged && m_pSinkList)
    {
        CHXSimpleList::Iterator ndx = m_pSinkList->Begin();
        for (; ndx != m_pSinkList->End(); ++ndx)
        {
            IHXNetInterfacesAdviseSink* pSink = (IHXNetInterfacesAdviseSink*) (*ndx);
            pSink->NetInterfacesUpdated();
        }
    }
    
    return rc;
}

STDMETHODIMP_(UINT32)
HXNetInterface::GetNumOfNetInterfaces()
{
    if (!m_bInitialized)
    {
	UpdateNetInterfaces();
    }
    return m_pNetInterfaceList ? m_pNetInterfaceList->GetCount() : 0;
}


STDMETHODIMP
HXNetInterface::GetNetInterfaces(UINT16	lIndex,
				  REF(NIInfo*)	pNIInfo)
{
    HX_RESULT  rc = HXR_OK;
    int        i = 0;
    CHXSimpleList::Iterator iter;
    
    pNIInfo = NULL;
    
    if (!m_bInitialized)
    {
	UpdateNetInterfaces();
    }
    
    if (m_pNetInterfaceList)
    {
	iter = m_pNetInterfaceList->Begin();
	for (; iter != m_pNetInterfaceList->End(); ++iter, ++i)
	{
	    NIInfo* pInfo = (NIInfo*)(*iter);
	    if (i == lIndex)
	    {
		pNIInfo = pInfo;
		break;
	    }
	}
    }
    
    if (!pNIInfo)
    {
	rc = HXR_FAILED;
    }
    
  cleanup:
    
    return rc;
}

STDMETHODIMP
HXNetInterface::AddAdviseSink(IHXNetInterfacesAdviseSink* pSink)
{
    HX_RESULT	rc = HXR_OK;

    if (!m_pSinkList)
    {
	m_pSinkList = new CHXSimpleList();
    }

    pSink->AddRef();
    m_pSinkList->AddTail(pSink);

    return rc;
}

STDMETHODIMP
HXNetInterface::RemoveAdviseSink(IHXNetInterfacesAdviseSink* pSink)
{
    HX_RESULT	rc = HXR_OK;

    LISTPOSITION lPosition = m_pSinkList->Find(pSink);

    if (!lPosition)
    {
	rc = HXR_UNEXPECTED;
	goto cleanup;
    }

    m_pSinkList->RemoveAt(lPosition);
    pSink->Release();

cleanup:

    return rc;
}

HX_RESULT
HXNetInterface::RetrieveNetInterface(CHXSimpleList*& pNetInterfaceList)
{
    HX_RESULT     		rc = HXR_OK;
    NIInfo*       		pNIInfo = NULL; 
    InetInterfaceInfo		info;

    pNIInfo = new NIInfo;
    
    if (!pNIInfo)
    {    
    	rc = HXR_OUTOFMEMORY;
    	goto cleanup;
    }
    	
    if (HXR_OK != OT_net::NetGetMyAddr(&pNIInfo->ulNetAddress) ||
  	HXR_OK != OT_net::NetGetIfaceInfo(&info))
    {
	rc = HXR_FAILED;
	goto cleanup;
    }
            
    pNIInfo->status = NI_OPER_STATUS_OPERATIONAL;
    // XXX HP reminder to upgrade it for IPv6
    HX_ASSERT(FALSE);
    //pNIInfo->ulNetMask = info.fNetmask;

    if (!pNetInterfaceList)
    {
	pNetInterfaceList = new CHXSimpleList;
    }
    pNetInterfaceList->AddTail(pNIInfo);
 
 cleanup:
 
    if (HXR_OK != rc)
    {
	HX_DELETE(pNIInfo);
    }
  	
    return rc;
}

HXBOOL
HXNetInterface::IsNetInterfaceChanged(void)
{
    HXBOOL            bResult = FALSE;
    CHXSimpleList*  pTempNetInterfaceList = new CHXSimpleList();

    RetrieveNetInterface(pTempNetInterfaceList);
    
    if (pTempNetInterfaceList && m_pNetInterfaceList)
    {
	if (pTempNetInterfaceList->GetCount() != m_pNetInterfaceList->GetCount())
	{
	    bResult = TRUE;
	}
	else
	{
	    CHXSimpleList::Iterator ndx0 = pTempNetInterfaceList->Begin();
	    CHXSimpleList::Iterator ndx1 = m_pNetInterfaceList->Begin();
	    for (; ndx0 != pTempNetInterfaceList->End() && ndx1 != m_pNetInterfaceList->End(); ++ndx0, ++ndx1)
	    {
		NIInfo* pInfo0 = (NIInfo*)(*ndx0);
		NIInfo* pInfo1 = (NIInfo*)(*ndx1);

                if (pInfo0->pAddressInfo && pInfo1->pAddressInfo)
                {
                    if (pInfo0->pAddressInfo->pAddress &&
                        pInfo1->pAddressInfo->pAddress)
                    {
                        if (0 != strcmp((const char*)pInfo0->pAddressInfo->pAddress->GetBuffer(),
                                        (const char*)pInfo1->pAddressInfo->pAddress->GetBuffer()))
                        {
                            bResult = TRUE;
                            break;
                        }
                    }
                    else if (pInfo0->pAddressInfo->pAddress != pInfo1->pAddressInfo->pAddress)
                    {
                        bResult = TRUE;    
                        break;
                    }
                }
                else if (pInfo0->pAddressInfo != pInfo1->pAddressInfo)
                {
                    bResult = TRUE;
                    break;
                }
	    }
	}
    }
    else if (pTempNetInterfaceList != m_pNetInterfaceList)
    {
	bResult = TRUE;
    }
    
    if (bResult)
    {
	Reset(m_pNetInterfaceList); 
	HX_DELETE(m_pNetInterfaceList);

	m_pNetInterfaceList = pTempNetInterfaceList;
    }
    else
    {
	Reset(pTempNetInterfaceList);
	HX_DELETE(pTempNetInterfaceList);
    }
    
    return bResult;
}
    
void
HXNetInterface::Reset(CHXSimpleList* pNetInterfaceList)
{
    if (pNetInterfaceList)
    {
	while (pNetInterfaceList->GetCount())
	{
	    NIInfo* pNIInfo = (NIInfo*)pNetInterfaceList->RemoveHead();
	    HX_DELETE(pNIInfo);
	}
    }
}

void
HXNetInterface::Close(void)
{
    Reset(m_pNetInterfaceList);
    HX_DELETE(m_pNetInterfaceList);

    if (m_pSinkList)
    {
	HX_ASSERT(m_pSinkList->GetCount() == 0);
	CHXSimpleList::Iterator ndx = m_pSinkList->Begin();
	for (; ndx != m_pSinkList->End(); ++ndx)
	{
	    IHXNetInterfacesAdviseSink* pSink = (IHXNetInterfacesAdviseSink*) (*ndx);
	    HX_RELEASE(pSink);
	}
	HX_DELETE(m_pSinkList);
    }
}
