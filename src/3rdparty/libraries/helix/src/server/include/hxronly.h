/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: hxronly.h,v 1.2 2003/01/23 23:42:59 damonlan Exp $ 
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
#ifndef _HXRONLY_H_
#define _HXRONLY_H_

typedef _INTERFACE	IUnknown			IUnknown;
typedef _INTERFACE	IHXInternalSetPropReadOnly	IHXInternalSetPropReadOnly;


/*
 * 
 *  Interface:
 *
 *	IHXInternalSetPropReadOnly
 *
 *  Purpose:
 *
 *	To make read only properties writable for certain purposes. It allows
 *  to change back the property to read only after the writing is done.
 *
 *  THIS IS AN INTERNAL ONLY API! DONOT EXPOSE IT TO THE CUSTOMER!
 *
 *  IID_IHXInternalSetPropReadOnly
 *
 *	{b8676e90-625c-11d4-968500c0f0320910}
 *
 */
DEFINE_GUID(IID_IHXInternalSetPropReadOnly,
    0xb8676e90, 0x625c, 0x11d4, 0x96, 0x85, 0x00, 0xc0, 0xf0, 0x32, 0x09, 0x10);

#undef  INTERFACE
#define INTERFACE   IHXInternalSetPropReadOnly

DECLARE_INTERFACE_(IHXInternalSetPropReadOnly, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    STDMETHOD (SetReadOnlyByName)	(THIS_
					const char* pName) PURE;
    STDMETHOD (SetReadOnlyById)		(THIS_
					UINT32 ulRegId) PURE;
    STDMETHOD (SetWritableByName)	(THIS_
					const char* pName) PURE;
    STDMETHOD (SetWritableById)		(THIS_
					UINT32 ulRegId) PURE;
};

#endif /* _HXRONLY_H_ */
