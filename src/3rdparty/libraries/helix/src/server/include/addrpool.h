/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: addrpool.h,v 1.2 2003/01/23 23:42:59 damonlan Exp $ 
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
#ifndef _ADDRPOOL_H_
#define _ADDRPOOL_H_



/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXMulticastAddressPool
 * 
 *  Purpose:
 * 
 *	This interface defines a multicast address pool that keeps track 
 *	of addresses in use and allocate open address 
 * 
 *  IID_IHXMulticastAddressPool:
 * 
 *	{00003300-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXMulticastAddressPool, 0x00003300, 0x901, 0x11d1, 0x8b, 0x6, 
					  0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXMulticastAddressPool

DECLARE_INTERFACE_(IHXMulticastAddressPool, IUnknown)
{
    /*
     *  IUnknown methods
     */
    STDMETHOD (QueryInterface)	(THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;
    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /*
     *  IHXMulticastAddressPool methods
     */

    /************************************************************************
     *	Method:
     *	    IHXMulticastAddresrsPool::UseAddress
     *	Purpose:
     *	    Mark this address as used
     */
    STDMETHOD(UseAddress)		(THIS_
					UINT32 ulAddr) PURE;
    /************************************************************************
     *	Method:
     *	    IHXMulticastAddresrsPool::ReleaseAddress
     *	Purpose:
     *	    Mark this address as unused
     */
    STDMETHOD(ReleaseAddress)		(THIS_
					UINT32 ulAddr) PURE;
    /************************************************************************
     *	Method:
     *	    IHXMulticastAddresrsPool::GetNextAddress
     *	Purpose:
     *	    Given a range between ulMin and ulMax, search for the first open 
     *	    address starting from ulLast.  Call with ulLast of 0 for 
     *	    the first time
     */
    STDMETHOD(GetNextAddress)		(THIS_
					UINT32 ulMin,
					UINT32 ulMax,
					UINT32 ulLast,
					REF(UINT32) ulNew) PURE;				
};

#endif // _ADDRPOOL_H_
