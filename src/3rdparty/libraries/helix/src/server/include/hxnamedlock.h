/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: hxnamedlock.h,v 1.1 2004/09/30 05:00:05 jc Exp $ 
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

#ifndef _HXNAMEDLOCK_H_
#define _HXNAMEDLOCK_H_



/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXNamedLock
 * 
 *  Purpose:
 *  
 *	Provide a named lock service so processes can easily share a mutex
 * 
 *  IID_IHXNamedLock:
 * 
 *	{00003400-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXNamedLock, 0xa5814cdc, 0x232a, 0x448d, 0x82, 0x4, 
                                     0x98, 0xe4, 0xd6, 0x64, 0x45, 0xb5);

#undef  INTERFACE
#define INTERFACE   IHXNamedLock

DECLARE_INTERFACE_(IHXNamedLock, IUnknown)
{
    /*
     *  IUnknown methods
     */
    STDMETHOD (QueryInterface)	(THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;
    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /*
     *  IHXNamedLock methods
     */

    /************************************************************************
     *	Method:
     *	    IHXNamedLock::GetNamedLock
     *	Purpose:
     *	    Get the lock associated with this name or create one if it does 
     *      not exist. 
     */
    STDMETHOD(GetNamedLock)		    (THIS_
                                             const char* pszName,
                                             IHXMutex** ppMutex) PURE;
    /************************************************************************
     *	Method:
     *	    IHXNamedLock::CreateNamedLock
     *	Purpose:
     *	    Create a lock associated with this name. 
     */
    STDMETHOD(CreateNamedLock)		    (THIS_
                                             const char* pszName) PURE;

    /************************************************************************
     *	Method:
     *	    IHXNamedLock::DestroyNamedLock
     *	Purpose:
     *	    Get the lock associated with this name or create one if it does 
     *      not exist. 
     */
    STDMETHOD(DestroyNamedLock)		    (THIS_ const char* pszName) PURE;
};

#endif




