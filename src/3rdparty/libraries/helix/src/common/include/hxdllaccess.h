/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxdllaccess.h,v 1.11 2007/07/06 20:43:41 jfinnecy Exp $
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

#ifndef _HXDLLACCESS_H_
#define _HXDLLACCESS_H_

/****************************************************************************
 *  Defines:
 */
/****************************************************************************
 *  DLL types:
 */
#define HXDLLTYPE_NOT_DEFINED	0	// Arbitrary DLLs (no predefined path used)
#define HXDLLTYPE_PLUGIN	1	// Plug-ins
#define HXDLLTYPE_CODEC		2	// Codecs
#define HXDLLTYPE_ENCSDK	3	// Encoder SDK DLLs 
#define HXDLLTYPE_COMMON	4	// Common libraries       
#define HXDLLTYPE_UPDATE	5	// Setup/Upgrade libraries
// HXDLLTYPEs 5-10 are reserved for backward compatibility reasons

/****************************************************************************
 *  Service specific error codes:
 */
#define HXDLL_OK		0
#define HXDLL_NO_LOAD		1
#define HXDLL_BAD_SYMBOL	2
#define HXDLL_OUT_OF_MEMORY	3

/****************************************************************************
 *  Dll default version key
 */
#define HXDLL_DFLT_VERSION	0xFFFFFFFF


/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXDllAccess
 *
 *  Purpose:
 *
 *	Interface for DLL Access
 *
 *  IID_IHXDllAccess
 *
 *	{D5A71AA1-A6ED-479f-9FC6-F06B99142691}
 *
 */
DEFINE_GUID(IID_IHXDllAccess,
0xd5a71aa1, 0xa6ed, 0x479f, 0x9f, 0xc6, 0xf0, 0x6b, 0x99, 0x14, 0x26, 0x91);

#define CLSID_IHXDllAccess IID_IHXDllAccess

#undef  INTERFACE
#define INTERFACE   IHXDllAccess

DECLARE_INTERFACE_(IHXDllAccess, IUnknown)
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
     *	IHXDllAccess methods
     */
    /************************************************************************
     * Method:
     *	    IHXDllAccess::Open()
     * Purpose:
     *	    Open a Dll of the specified name and type.  
     */
    STDMETHOD(Open)		(THIS_
				const char* pDllName,
				UINT16 uLibType
				) PURE;

    /************************************************************************
     * Method:
     *	    IHXDllAccess::Close()
     * Purpose:
     *	    Close opened Dll.
     */
    STDMETHOD(Close)		(THIS) PURE;

    /************************************************************************
     * Method:
     *	    IHXDllAccess::GetSymbol()
     * Purpose:
     *	    Obtain pointer to symbol in opened Dll.
     */
    STDMETHOD_(void*, GetSymbol)	(THIS_
					const char* pSymbolName
					) PURE;

    /************************************************************************
     * Method:
     *	    IHXDllAccess::GetError()
     * Purpose:
     *	    Get service specific error code (0 = no error).
     */
    STDMETHOD_(ULONG32,GetError)	(THIS) PURE;

    /************************************************************************
     * Method:
     *	    IHXDllAccess::GetErrorString()
     * Purpose:
     *	    Get last error string.
     */
    STDMETHOD_(const char*, GetErrorString)	(THIS) PURE;

    /************************************************************************
     * Method:
     *	    IHXDllAccess::GetDllName()
     * Purpose:
     *	    Get opened Dll name
     */
    STDMETHOD_(const char*, GetDllName)		(THIS) PURE;

    /************************************************************************
     * Method:
     *	    IHXDllAccess::GetDllVersion()
     * Purpose:
     *	    Get opened Dll vesion
     */
    STDMETHOD_(const char*, GetDllVersion)	(THIS) PURE;
    
    /************************************************************************
     * Method:
     *	    IHXDllAccess::IsOpen()
     * Purpose:
     *	    Tells if Dll is opened:
     *		TRUE  = dll is opened
     *		FALSE = dll is not opened	
     */
    STDMETHOD_(HXBOOL,IsOpen)	(THIS) PURE;
};

#if defined(HELIX_FEATURE_ALLOW_DEPRECATED_SMARTPOINTERS)
#include "hxtsmartpointer.h"
HXT_MAKE_SMART_PTR(IHXDllAccess)
#endif

#endif  // _HXDLLACCESS_H_
