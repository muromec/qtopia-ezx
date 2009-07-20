/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxvctrl.h,v 1.4 2007/07/06 21:58:18 jfinnecy Exp $
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

#ifndef _HXVCTRL_H_
#define _HXVCTRL_H_

/****************************************************************************
 *
 *  Interface:
 *
 *  IHXVideoControl
 *
 *  Purpose:
 *
 *  Interface to be implemented by all video renderers. The client engine
 *  can use this interface to control generic video parameters: brightness,
 *  contrast, saturation, and hue.
 *
 *  IID_IHXVideoControl:
 *
 *	{00000900-b4c8-11d0-9995-00a0248da5f0}
 *
 */
DEFINE_GUID(IID_IHXVideoControl, 0x00000900, 0xb4c8, 0x11d0, 0x99, 0x95, 0x0, 0xa0, 0x24, 0x8d, 0xa5, 0xf0);

#undef  INTERFACE
#define INTERFACE   IHXVideoControl

DECLARE_INTERFACE_(IHXVideoControl, IUnknown)
{
    /*
     * IUnknown methods:
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /*
     * IHXVideoControl methods:
     */

    /************************************************************************
     *  Methods:
     *      IHXVideoControl::GetBrightness
     *      IHXVideoControl::SetBrightness
     *	Purpose:
     *      Gets/sets image brightness.
     */
    STDMETHOD_(float, GetBrightness)  (THIS) PURE;

    STDMETHOD(SetBrightness)  (THIS_
                float Brightness) PURE;

#   define  MAX_BRIGHTNESS  1.
#   define  DEF_BRIGHTNESS  0.
#   define  MIN_BRIGHTNESS  -1.

    /************************************************************************
     *  Methods:
     *      IHXVideoControl::GetContrast
     *      IHXVideoControl::SetContrast
     *	Purpose:
     *      Gets/sets image contrast.
     */
    STDMETHOD_(float, GetContrast)  (THIS) PURE;

    STDMETHOD(SetContrast)  (THIS_
                float Contrast) PURE;

#   define  MAX_CONTRAST    1.
#   define  DEF_CONTRAST    0.
#   define  MIN_CONTRAST    -1.

    /************************************************************************
     *  Methods:
     *      IHXVideoControl::GetSaturation
     *      IHXVideoControl::SetSaturation
     *	Purpose:
     *      Gets/sets color saturation.
     */
    STDMETHOD_(float, GetSaturation)  (THIS) PURE;

    STDMETHOD(SetSaturation)  (THIS_
                float Saturation) PURE;

#   define  MAX_SATURATION  1.
#   define  DEF_SATURATION  0.
#   define  MIN_SATURATION  -1.

    /************************************************************************
     *  Methods:
     *      IHXVideoControl::GetHue
     *      IHXVideoControl::SetHue
     *	Purpose:
     *      Gets/sets hue (color rotation angle).
     */
    STDMETHOD_(float, GetHue)  (THIS) PURE;

    STDMETHOD(SetHue)  (THIS_
                float Hue) PURE;

#   define  MAX_HUE         1.
#   define  DEF_HUE         0.
#   define  MIN_HUE         -1.

    /************************************************************************
     *  Methods:
     *      IHXVideoControl::GetSharpness
     *      IHXVideoControl::SetSharpness
     *	Purpose:
     *      Gets/sets sharpness.
     */
    STDMETHOD_(float, GetSharpness)  (THIS) PURE;

    STDMETHOD(SetSharpness)  (THIS_
                float Sharpness) PURE;

#   define  MAX_SHARPNESS  1.
#   define  DEF_SHARPNESS  -1.
#   define  MIN_SHARPNESS  -1.


    /************************************************************************
     *  Methods:
     *      IHXVideoControl::SetModeSharpness
     *	Purpose:
     *     Sets sharpness mode depending upon if the deblocking filter is on/off
     */
    STDMETHOD(SetModeSharpness)	(THIS_ 
				    UINT16 dFlag) PURE;



};

#include "hxcomptr.h"
DEFINE_SMART_PTR(IHXVideoControl)

#endif /* _HXVCTRL_H_ */
