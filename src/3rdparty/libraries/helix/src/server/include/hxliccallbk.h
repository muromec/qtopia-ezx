/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: hxliccallbk.h,v 1.3 2003/03/05 15:40:08 dcollins Exp $ 
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
 

#ifndef _HXLICCALLBK_H_
#define _HXLICCALLBK_H_

typedef _INTERFACE	IUnknown			IUnknown;


/*  
 *  Interface: IHXLicenseCallback
 *
 */
// {886BD894-4FE8-4ef9-AA6D-66C587BF5661}

DEFINE_GUID(IID_IHXLicenseCallback, 
0x886bd894, 0x4fe8, 0x4ef9, 0xaa, 0x6d, 0x66, 0xc5, 0x87, 0xbf, 0x56, 0x61);

#define CLSID_IHXLicenseCallback	IID_IHXLicenseCallback

#undef  INTERFACE
#define INTERFACE   IHXLicenseCallback

typedef ULONG32 LicenseCallbkID;

DECLARE_INTERFACE_(IHXLicenseCallback, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /*
     *  IHXLicenseCallback methods
     */

    STDMETHOD(Register) (THIS_
                IHXCallback* pResponse) PURE;

    STDMETHOD(DeRegister) (THIS_
                IHXCallback* pCallback) PURE;

};

#endif
