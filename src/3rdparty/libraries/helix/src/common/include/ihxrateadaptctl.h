/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: ihxrateadaptctl.h,v 1.2 2005/03/14 19:27:09 bobclark Exp $ 
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
#ifndef IHXRATEADAPTCTL_H
#define IHXRATEADAPTCTL_H

#include "hxcom.h"

/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXClientRateAdaptControl
 * 
 *  Purpose:
 * 
 *	Client rate adaptation control interface
 * 
 *  IID_IHXClientRateAdaptControl:
 * 
 *	{44F5AC8C-654C-414e-9D18-E7A480907009}
 * 
 */
DEFINE_GUID(IID_IHXClientRateAdaptControl, 
0x44f5ac8c, 0x654c, 0x414e, 0x9d, 0x18, 0xe7, 0xa4, 0x80, 0x90, 0x70, 0x9);

#undef  INTERFACE
#define INTERFACE   IHXClientRateAdaptControl

DECLARE_INTERFACE_(IHXClientRateAdaptControl, IUnknown)
{
    /*
     * IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /*
     * IHXClientRateAdaptControl Methods
     */

    /************************************************************************
     *	Method:
     *	    IHXClientRateAdaptControl::Enable
     *	Purpose:
     *	    Enable client rate adaptation for the specified stream
     *
     */
    STDMETHOD(Enable) (THIS_ UINT16 uStreamNum) PURE;

    /************************************************************************
     *	Method:
     *	    IHXClientRateAdaptControl::Disable
     *	Purpose:
     *	    Disable client rate adaptation for the specified stream
     *
     */
    STDMETHOD(Disable) (THIS_ UINT16 uStreamNum) PURE;

    /************************************************************************
     *	Method:
     *	    IHXClientRateAdaptControl::IsEnabled
     *	Purpose:
     *	    Is client rate adaptation enabled for the specified stream
     *
     */
    STDMETHOD(IsEnabled) (THIS_ UINT16 uStreamNum,
                          REF(HXBOOL) bEnabled) PURE;
};


#endif /* IHXRATEADAPTCTL_H */
