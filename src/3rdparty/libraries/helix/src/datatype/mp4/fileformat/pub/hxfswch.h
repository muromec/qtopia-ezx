/* ***** BEGIN LICENSE BLOCK ***** 
 * Version: RCSL 1.0/RPSL 1.0 
 *  
 * Portions Copyright (c) 1995-2002 RealNetworks, Inc. All Rights Reserved. 
 *      
 * The contents of this file, and the files included with this file, are 
 * subject to the current version of the RealNetworks Public Source License 
 * Version 1.0 (the "RPSL") available at 
 * http://www.helixcommunity.org/content/rpsl unless you have licensed 
 * the file under the RealNetworks Community Source License Version 1.0 
 * (the "RCSL") available at http://www.helixcommunity.org/content/rcsl, 
 * in which case the RCSL will apply. You may also obtain the license terms 
 * directly from RealNetworks.  You may not use this file except in 
 * compliance with the RPSL or, if you have a valid RCSL with RealNetworks 
 * applicable to this file, the RCSL.  Please see the applicable RPSL or 
 * RCSL for the rights, obligations and limitations governing use of the 
 * contents of the file.  
 *  
 * This file is part of the Helix DNA Technology. RealNetworks is the 
 * developer of the Original Code and owns the copyrights in the portions 
 * it created. 
 *  
 * This file, and the files included with this file, is distributed and made 
 * available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER 
 * EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS ALL SUCH WARRANTIES, 
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS 
 * FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT. 
 * 
 * Technology Compatibility Kit Test Suite(s) Location: 
 *    http://www.helixcommunity.org/content/tck 
 * 
 * Contributor(s): 
 *  
 * ***** END LICENSE BLOCK ***** */ 

#ifndef _HXFSWCH_H_
#define _HXFSWCH_H_

/****************************************************************************
 *  Includes
 */
#include "hxtypes.h"
#include "hxcom.h"
#include "hxfiles.h"
#include "ihxpckts.h"

/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXFileSwitcher
 *
 *  Purpose:
 *
 *	File Switcher Interface
 *
 *  IID_IHXFileSwitcher
 *
 *	{55CC55B0-01BA-11d4-9523-00902742C923}
 *
 */
DEFINE_GUID(IID_IHXFileSwitcher, 0x55cc55b0, 0x1ba, 0x11d4, 0x95, 0x23,
	    0x0, 0x90, 0x27, 0x42, 0xc9, 0x23);

#undef  INTERFACE
#define INTERFACE   IHXFileSwitcher

DECLARE_INTERFACE_(IHXFileSwitcher, IUnknown)
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
     *	IHXFileSwitcher methods
     */
    STDMETHOD(Init)	(THIS_
			IHXFileObject* pFileObject,
			ULONG32 ulFlags,
			IHXFileResponse* pResponse,
			IUnknown* pContext,
			UINT16 uChildCount
			) PURE;

    STDMETHOD(Read)	(THIS_
			ULONG32 ulSize,
			IHXFileResponse *pResponse,
			const char* pFileName = NULL
			) PURE;
    
    STDMETHOD(Write)	(THIS_
			IHXBuffer* pBuffer,
			IHXFileResponse *pResponse,
			const char* pFileName = NULL
			) PURE;

    STDMETHOD(Seek)	(THIS_
			ULONG32 ulOffset,
			HXBOOL bRelative,
			IHXFileResponse *pResponse,
			const char* pFileName = NULL
			) PURE;

    STDMETHOD(Close)	(THIS_
			IHXFileResponse *pResponse,
			const char* pFileName = NULL
			) PURE;

    STDMETHOD(Advise)	(THIS_
			ULONG32 ulInfo,
			const char* pFileName = NULL) PURE;
};

#endif  // _HXFSWCH_H_
