/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: hxservrdttran.h,v 1.2 2003/07/23 19:10:39 damonlan Exp $ 
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
#ifndef _HXSERVERRDTTRAN_H_
#define _HXSERVERRDTTRAN_H_

#include "hxcom.h"

/****************************************************************************
 * 
 *  Interface:
 * 
 *      IHXServerRDTTransport
 * 
 *  Purpose:
 * 
 *      Methods the server needs to support legacy rdt transport functionality
 * 
 *  IID_IHXServerRDTTransport
 *
 *      {00000304-b4c8-11d0-9995-00a0248da5f0}
 * 
 */
DEFINE_GUID(IID_IHXServerRDTTransport, 0x00000304, 0xb4c8, 0x11d0, 0x99, 0x95,
		    0x0, 0xa0, 0x24, 0x8d, 0xa5, 0xf0);

#undef  INTERFACE
#define INTERFACE   IHXServerRDTTransport

DECLARE_INTERFACE_(IHXServerRDTTransport, IUnknown)
{
    STDMETHOD(SetDeliveryBandwidth) (THIS_
                                     UINT32 ulBackOff,
                                     UINT32 ulBandwidth) PURE;
    
    STDMETHOD(SetAggregationLimits) (THIS_
				     UINT32* pAggregStat,
                                     UINT32 ulAggregateTo,
                                     UINT32 ulAggregateHighest) PURE;
};


#endif // _HXSERVERRDTTRAN_H_
