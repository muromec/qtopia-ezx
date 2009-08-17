/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: mcastaddr_mgr.h,v 1.3 2004/05/03 19:02:47 tmarshall Exp $ 
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


#ifndef _MCASTADDR_MGR_H_
#define _MCASTADDR_MGR_H_

// checks if x is within min and max.  if x>max, x = min
#define BOUND_CHECK(min, max, x) (x > max ? x = min : x)

#include "addrpool.h" // IHXMulticastAddressPool

//#include "hxmap.h" // CHXMapLongToObj
//_INTERFACE IHXMulticastAddressPool;

class MulticastAddressPool : public IHXMulticastAddressPool
{
public:
    MulticastAddressPool(void);

    /* IUnknown */
    STDMETHOD(QueryInterface)	(THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)	(THIS);
    STDMETHOD_(ULONG32,Release)	(THIS);

    /* IHXMulticastAddressPool */
    STDMETHOD(UseAddress)		(THIS_ UINT32 ulAddr);
    STDMETHOD(ReleaseAddress)		(THIS_ UINT32 ulAddr);
    STDMETHOD(GetNextAddress)		(THIS_
					 UINT32 ulMin,  
					 UINT32 ulMax,
					 UINT32 ulLast,
					 REF(UINT32) ulNew);
private:
    ~MulticastAddressPool();
    inline BOOL	    IsOpen(UINT32 ulAddr);

private:
    INT32		m_lRefCount;    
    CHXMapLongToObj	m_mapAddr;
};

inline BOOL
MulticastAddressPool::IsOpen(UINT32 ulAddr)
{
    BOOL* pbUsed = NULL;
#if _DEBUG
    if (m_mapAddr.Lookup(ulAddr, (void*&)pbUsed))
    {
	HX_ASSERT(TRUE == *pbUsed);
	pbUsed = NULL;
    }
#endif

    return !m_mapAddr.Lookup(ulAddr, (void*&)pbUsed);
}

#endif //_MCASTADDR_MGR_H_

