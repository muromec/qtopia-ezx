/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxpcmkr.h,v 1.6 2007/07/06 20:43:42 jfinnecy Exp $
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

#ifndef _HXPCMKR_H_
#define _HXPCMKR_H_

/****************************************************************************
 *  Includes
 */
#include "hxtypes.h"
#include "hxcomm.h"


/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXPaceMakerResponse
 *
 *  Purpose:
 *
 *	Response Interface for Pacemaker object.
 *
 *  IID_IHXPaceMakerResponse
 *
 *	{B4D81B50-4DAC-11d5-A938-00010251B340}
 *
 */
DEFINE_GUID(IID_IHXPaceMakerResponse, 0xb4d81b50, 0x4dac, 0x11d5, 0xa9, 0x38, 
	0x0, 0x1, 0x2, 0x51, 0xb3, 0x40);

#undef  INTERFACE
#define INTERFACE   IHXPaceMakerResponse

DECLARE_INTERFACE_(IHXPaceMakerResponse, IUnknown)
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
     *	IHXPaceMakerResponse
     */
    STDMETHOD(OnPaceStart)	(THIS_
				ULONG32 ulId) PURE;
    
    STDMETHOD(OnPaceEnd)	(THIS_
				ULONG32 ulId) PURE;
    
    STDMETHOD(OnPace)		(THIS_
				ULONG32 ulId) PURE;
};


/****************************************************************************
 * 
 *  Interface:
 *
 *	IHXPaceMaker
 *
 *  Purpose:
 *
 *	Response Interface for Pacemaker object.
 *
 *  IID_IHXPaceMaker
 *
 *	{1223EFF0-4DAD-11d5-A938-00010251B340}
 *
 */
DEFINE_GUID(IID_IHXPaceMaker, 0x1223eff0, 0x4dad, 0x11d5, 0xa9, 0x38, 
	0x0, 0x1, 0x2, 0x51, 0xb3, 0x40);

#undef  INTERFACE
#define INTERFACE   IHXPaceMaker

/*
 *  The IHXCommonClassFactory supports creating an instance
 *  of this object.
 */
#define CLSID_IHXPaceMaker IID_IHXPaceMaker

DECLARE_INTERFACE_(IHXPaceMaker, IUnknown)
{
    /*
     *	IUnknown 
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;


    /*
     *	IHXPaceMaker
     */
    STDMETHOD(Start)		(THIS_
				IHXPaceMakerResponse* pResponse,
				LONG32 lPriority,
				ULONG32 ulInterval,
				REF(ULONG32) ulId) PURE;

    STDMETHOD(Stop)		(THIS) PURE;

    STDMETHOD(Suspend)		(THIS_ 
				 HXBOOL bSuspend) PURE;

    STDMETHOD(Signal)		(THIS) PURE;

    STDMETHOD(WaitForStop)	(THIS) PURE;

    STDMETHOD(WaitForSuspend)	(THIS) PURE;

    STDMETHOD(SetPriority)	(THIS_
				LONG32 lPriority) PURE;

    STDMETHOD(SetInterval)	(THIS_
				ULONG32 ulInterval) PURE;
};

#endif // _HXPCMKR_H_

