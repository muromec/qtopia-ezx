/* ***** BEGIN LICENSE BLOCK *****
* Source last modified: $Id: hxsite3.h,v 1.6 2007/08/15 22:21:33 praveenkumar Exp $
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
* Contributor(s): Shy Ward
*
* ***** END LICENSE BLOCK ***** */

#ifndef _HXSITE3_H_
#define _HXSITE3_H_

#include "hxcom.h"           /* For COM. */
#include "hxwintyp.h"        /* For HXxRect. */

/*
* Forward declarations of some interfaces defined or used here-in.
*/
typedef _INTERFACE IHXSurfaceControl IHXSurfaceControl;

typedef enum
{
    Rotate0   = 0,
        Rotate90,
        Rotate180,
        Rotate270
} RotationType;

typedef enum
{
    DSAResume   = 0,
    DSAStop
} DSAState;


/****************************************************************************
*
*  Interface:
*
*	IHXSurfaceControl
*
*  Purpose:
*
*	Interface for IHXSurfaceControl objects.
*
*  IID_IHXSurfaceControl:
*
*	{0x00000D26-0x903-0x11d1-0x8b60a024406d59)
*
*/

/*
* Forward declarations of some interfaces defined or used here-in.
*/

#undef  INTERFACE
#define INTERFACE IHXSurfaceControl


DEFINE_GUID(IID_IHXSurfaceControl, 0x00000D26, 0x903, 0x11d1, 0x8b, 0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59)

DECLARE_INTERFACE_(IHXSurfaceControl, IUnknown)
{
    /* IUnknown methods. */
    STDMETHOD(QueryInterface)   (THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)  (THIS) PURE;
    STDMETHOD_(ULONG32,Release) (THIS) PURE;

    STDMETHOD(SetRotation) (THIS_ UINT32 ulValue) PURE;
    STDMETHOD(GetRotation) (THIS_ REF(UINT32) ulValue) PURE;
    STDMETHOD(SetContrast) (THIS_ UINT32 ulValue) PURE;
    STDMETHOD(GetContrast) (THIS_ REF(UINT32) ulValue) PURE;
    STDMETHOD(SetFrameRect) (THIS_ HXxRect sValue) PURE;
    STDMETHOD(GetFrameRect) (THIS_ REF(HXxRect) sValue) PURE;
    STDMETHOD(SetScaling) (THIS_ HXFLOAT fWidthPerc, HXFLOAT fHeightPerc, HXBOOL bAntiAlias) PURE;
    STDMETHOD(GetScaling) (THIS_ REF(HXFLOAT) fWidthPerc, REF(HXFLOAT) fHeightPerc, REF(HXBOOL) bAntiAlias) PURE;
    STDMETHOD(SetCropRect) (THIS_ HXxRect sValue) PURE;
    STDMETHOD(GetCropRect) (THIS_ REF(HXxRect) sValue) PURE;
    STDMETHOD(SetWindowParameters) (THIS_ HXxRect sWinRect, HXxRect sClipRect) PURE;
    STDMETHOD(SetScalingType) (THIS_ UINT32 uValue) PURE;
    STDMETHOD(GetScalingType) (THIS_ REF(UINT32) uValue) PURE;
};

/****************************************************************************
*
*  Interface:
*
*	IHXSiteRegister
*
*  Purpose:
*
*	Interface for IHXSiteRegister objects.
*
*  IID_IHXSiteRegister:
*
*	{0x719e98e8-0xdcc3-0x4fc1-0x84c013fd3fb5a523}
*
*/

/*
* Forward declarations of some interfaces defined or used here-in.
*/
typedef _INTERFACE IHXSiteRegister IHXSiteRegister;

#undef  INTERFACE
#define INTERFACE IHXSiteRegister

DEFINE_GUID(IID_IHXSiteRegister, 0x719e98e8, 0xdcc3, 0x4fc1, 0x84, 0xc0, 0x13, 0xfd, 0x3f, 0xb5, 0xa5, 0x23);

DECLARE_INTERFACE_(IHXSiteRegister, IUnknown)
{

    // IUnknown methods.
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef) (THIS) PURE;
    STDMETHOD_(ULONG32,Release) (THIS) PURE;
    //adpater methods
    STDMETHOD(RegisterAdapter) (THIS_ IUnknown* m_pSurfaceControlSite) PURE;
    STDMETHOD(DeRegisterAdapter) (THIS_) PURE;
};

/****************************************************************************
*
*  Interface:
*
*	IHXDSAControl
*
*  Purpose:
*
*	Interface for IHXDSAControl objects.
*
*  IID_IHXDSAControl:
*
*	{0x5ce50a2a-0x8a8-0x450a-0xb67489ef122afff1}
*
*/

/*
* Forward declarations of some interfaces defined or used here-in.
*/

DEFINE_GUID(IID_IHXDSAControl,  0x5ce50a2a, 0x8a8, 0x450a, 0xb6, 0x74, 0x89, 0xef, 0x12, 0x2a, 0xff, 0xf1);

#undef  INTERFACE
#define INTERFACE IHXDSAControl

DECLARE_INTERFACE_(IHXDSAControl, IUnknown)
{

    // IUnknown methods.
    STDMETHOD(QueryInterface)       (THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)      (THIS) PURE;
    STDMETHOD_(ULONG32,Release)     (THIS) PURE;
    //DSA Methods
    STDMETHOD(DirectScreenAccessEvent)  (THIS_ HXBOOL bValue) PURE;
    STDMETHOD(UpdateDisplayRegion)      (THIS_ HXxRegion Region) PURE;
    STDMETHOD(RefreshWindow)            (THIS_) PURE;
#ifdef SYMBIAN_ENABLE_MMF_MULTISCREEN_SUPPORT
    STDMETHOD(SetInitScreenNumber)      (THIS_ INT32 lScreenNumber) PURE;
#endif //SYMBIAN_ENABLE_MMF_MULTISCREEN_SUPPORT
};

#endif /* _HXSITE3_H_ */
