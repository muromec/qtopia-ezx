/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: mcstaddr.h,v 1.2 2003/01/23 23:42:50 damonlan Exp $ 
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
#ifndef _MCSTADDR_H_
#define _MCSTADDR_H_

#include "addrpool.h"

class MulticastAddressAllocator
{
public:
    MulticastAddressAllocator();
    ~MulticastAddressAllocator();
    
    HX_RESULT   Init(IHXMulticastAddressPool* pPool);
    void	SetAddressRange(UINT32 ulMin, UINT32 ulMax);
    HX_RESULT	GetNextAddress(REF(UINT32) ulAddr);		
    inline void	ReleaseAddress(UINT32 ulAddr);
    UINT32	GetMin(void) { return m_ulMin; }
    UINT32	GetMax(void) { return m_ulMax; }

private:    
    UINT32  m_ulMin;
    UINT32  m_ulMax;
    UINT32  m_ulLast;
    IHXMulticastAddressPool* m_pPool;
};

inline void    
MulticastAddressAllocator::ReleaseAddress(UINT32 ulAddr)
{
    HX_ASSERT(m_pPool);
    m_pPool->ReleaseAddress(ulAddr);
}

#endif //_MCSTADDR_H_


