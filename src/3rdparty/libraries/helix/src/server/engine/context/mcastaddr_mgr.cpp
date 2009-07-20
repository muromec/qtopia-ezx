/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: mcastaddr_mgr.cpp,v 1.3 2003/09/04 22:35:34 dcollins Exp $ 
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
#include "hxmap.h" // CHXMapLongToObj
#include "hxresult.h"	// HX_RESULT
#include "mcastaddr_mgr.h"
#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static char HX_THIS_FILE[] = __FILE__;
#endif


STDMETHODIMP MulticastAddressPool::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
	AddRef();
	*ppvObj = (IUnknown*)this;
	return HXR_OK;
    }
    if (IsEqualIID(riid, IID_IHXMulticastAddressPool))
    {
	AddRef();
	*ppvObj = (IHXMulticastAddressPool*)this;
	return HXR_OK;
    }
    
    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}
STDMETHODIMP_(ULONG32) MulticastAddressPool::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}
STDMETHODIMP_(ULONG32) MulticastAddressPool::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

MulticastAddressPool::MulticastAddressPool(void)
    : m_lRefCount(0)
{
    // hash tbl size of 17 is a little bit too small
    m_mapAddr.InitHashTable(100);
}
MulticastAddressPool::~MulticastAddressPool()
{
    CHXMapLongToObj::Iterator i;
    if (!m_mapAddr.IsEmpty())
    {
	for (i  = m_mapAddr.Begin(); i != m_mapAddr.End(); ++i)
        {
	    delete ((BOOL*)(*i));
	}
	m_mapAddr.RemoveAll(); 
    }        
}

HX_RESULT
MulticastAddressPool::ReleaseAddress(UINT32 ulAddr)
{
    BOOL* pbUsed = NULL;
    if (m_mapAddr.Lookup(ulAddr, (void*&)pbUsed))
    {
	HX_ASSERT(TRUE == *pbUsed);
	delete pbUsed;

	m_mapAddr.RemoveKey(ulAddr);
	return HXR_OK;
    }
    return HXR_FAIL;
}

HX_RESULT
MulticastAddressPool::UseAddress(UINT32 ulAddr)
{
    if (!IsOpen(ulAddr))
    {
	// it's been marked as used
	return HXR_FAIL;
    }
    
    BOOL* pb = new BOOL;
    if (pb)
    {
	*pb = TRUE;
	m_mapAddr.SetAt(ulAddr, pb);
	return HXR_OK;
    }	
    else
    {
	return HXR_FAIL;
    }
}


HX_RESULT
MulticastAddressPool::GetNextAddress(UINT32 ulMin,
				     UINT32 ulMax,
				     UINT32 ulLast,
				     REF(UINT32) ulNew)
{
    HX_ASSERT((0 == ulLast) || ((ulLast >= ulMin) && (ulLast <= ulMax)));

    if (0 != ulLast)
    {
	ulLast++;
    }
    else
    {
	ulLast = ulMin;
    }
    
    // initial check!
    BOUND_CHECK(ulMin, ulMax, ulLast);

    // save off the first possible addr.
    UINT32 ulAddrOrig = ulLast;

    // initial check!
//    BOUND_CHECK(ulMin, ulMax, ulNext);
    
    BOOL* pbUsed = NULL;
    // starting from wherer it is, go through the addr space to find
    // a first non used addr.
    while (!IsOpen(ulLast))
    {	
	// this address is used at this moment, so try next one
	ulLast++;

	if (ulLast == ulAddrOrig)
	{
	    // wrapped...no more address
	    return HXR_FAIL;
	}

	BOUND_CHECK(ulMin, ulMax, ulLast);

	if (ulLast == ulAddrOrig)
	{
	    // no more addr
	    return HXR_FAIL;
	}
    }
    
    // found it
    ulNew = ulLast;
    UseAddress(ulNew);
	
    return HXR_OK;        
}


