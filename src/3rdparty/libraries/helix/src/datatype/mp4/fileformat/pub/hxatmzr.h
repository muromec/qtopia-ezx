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

#ifndef _HXATMZR_H_
#define _HXATMZR_H_

/****************************************************************************
 *  Includes
 */
#include "hxtypes.h"
#include "hxcom.h"
#include "qtbatom.h"

/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXAtomizerResponse
 *
 *  Purpose:
 *
 *	Response Interface for Atomizer object.
 *
 *  IID_IHXAtomizerResponse
 *
 *	{72BC0330-0041-11d4-9523-00902742C923}
 *
 */
DEFINE_GUID(IID_IHXAtomizerResponse, 0x72bc0330, 0x41, 0x11d4, 0x95, 0x23,
	    0x0, 0x90, 0x27, 0x42, 0xc9, 0x23);

#undef  INTERFACE
#define INTERFACE   IHXAtomizerResponse

DECLARE_INTERFACE_(IHXAtomizerResponse, IUnknown)
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
     *	IHXAtomizerResponse methods
     */
    STDMETHOD(AtomReady)	(THIS_
				HX_RESULT status,
				CQTAtom	*pRootAtom) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXAtomizationCommander
 *
 *  Purpose:
 *
 *	Atomization Commander Interface for Atomizer object.
 *
 *  IID_IHXAtomizationCommander
 *
 *	{AFDCD230-004B-11d4-9523-00902742C923}
 *
 */
typedef enum
{
    ATMZR_CMD_SKIP,
    ATMZR_CMD_LOAD,
    ATMZR_CMD_OUTLINE,
    ATMZR_CMD_STOP
} QTAtomizerCmd;

DEFINE_GUID(IID_IHXAtomizationCommander, 0xafdcd230, 0x4b, 0x11d4, 0x95, 0x23,
	    0x0, 0x90, 0x27, 0x42, 0xc9, 0x23);

#undef  INTERFACE
#define INTERFACE   IHXAtomizationCommander

DECLARE_INTERFACE_(IHXAtomizationCommander, IUnknown)
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
     *	IHXAtomizationCommander methods
     */
    STDMETHOD_(QTAtomizerCmd,GetAtomCommand)	(THIS_
						QTAtomType AtomType,
						CQTAtom *pParent) PURE;
};

#endif  // _HXATMZR_H_
