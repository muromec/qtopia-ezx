/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: mcstaddr.cpp,v 1.2 2003/01/23 23:42:50 damonlan Exp $ 
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
#include "addrpool.h"	// IHXMulticastAddressPool
#include "mcstaddr.h"
#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static char HX_THIS_FILE[] = __FILE__;
#endif


// checks if x is within min and max.  if x>max, x = min
#define BOUND_CHECK(min, max, x) (x > max ? x = min : x)




MulticastAddressAllocator::MulticastAddressAllocator()
    : m_ulMin(0)
    , m_ulMax(0)
    , m_ulLast(0)	
    , m_pPool(NULL)
{
}

MulticastAddressAllocator::~MulticastAddressAllocator()
{
    HX_RELEASE(m_pPool);
}

HX_RESULT
MulticastAddressAllocator::Init(IHXMulticastAddressPool* pPool)
{
    HX_ASSERT(pPool);
    m_pPool = pPool;
    m_pPool->AddRef();
    return HXR_OK;
}

void    
MulticastAddressAllocator::SetAddressRange(UINT32 ulMin, 
					   UINT32 ulMax)
{
    HX_ASSERT(0 != ulMin && 0 != ulMax);
    HX_ASSERT(ulMin <= ulMax);

    m_ulLast = 0;
    m_ulMin = ulMin;    
    m_ulMax = ulMax;
}

HX_RESULT
MulticastAddressAllocator::GetNextAddress(REF(UINT32) ulAddr)
{
    HX_ASSERT(m_pPool && "Call Init() first");
    HX_ASSERT(m_ulMin && m_ulMax && "Call SetAddressRange() first");
    HX_RESULT theErr;

    theErr = m_pPool->GetNextAddress(m_ulMin, m_ulMax, m_ulLast, ulAddr);
    if (SUCCEEDED(theErr))
    {
	m_ulLast = ulAddr;
    }
    else
    {
	m_ulLast = 0;
    }

    return theErr;
}

