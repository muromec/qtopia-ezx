/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: hxdist_lic_requester.h,v 1.2 2003/01/23 23:42:59 damonlan Exp $ 
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

#ifndef _HX_DIST_LIC_REQUESTER_H_
#define _HX_DIST_LIC_REQUESTER_H_

#include <limits.h>

typedef _INTERFACE	IUnknown			IUnknown;
typedef _INTERFACE	IHXCallback			IHXCallback;
typedef _INTERFACE	IHXBuffer			IHXBuffer;

typedef enum _DistLicReqStatus
{
      DL_NOT_REGISTERED = 0x00
    , DL_AWAITING_LICENSE = 0x01
    , DL_GOT_LICENSE
    , DL_DENIED_LICENSE
} DistLicReqStatus;

#define DL_PARENT_CLIENT 0x01
#define DL_CHILD_CLIENT 0x02

/*
 * 
 *  Interface:
 *
 *	IHXRNDistributedLicenseRequester
 *
 *  Purpose:
 *
 *  IID_IHXDistributedLicenseRequester:
 *
 *
 */
DEFINE_GUID(IID_IHXDistributedLicenseRequester, 
    0x00005702, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXRNDistributedLicenseRequester

DECLARE_INTERFACE_(IHXRNDistributedLicenseRequester, IUnknown)
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
     *	IHXRNDistributedLicenseRequester methods
     */
    STDMETHOD_(DistLicReqStatus,LicenseRequestStatus)(THIS_
    				ULONG32 ulClientID) PURE;
    STDMETHOD_(DistLicReqStatus,Insert)(THIS_
    				ULONG32 ulClientID,
				IHXCallback* pCb,
				REF(IHXCallback*) pParentCb) PURE;
    STDMETHOD_(INT32,Execute)	(THIS_
				INT32 nNumLicences) PURE;
    STDMETHOD(Remove)		(THIS_
    				ULONG32 ulClientID,
				IHXCallback* pCb) PURE;
    STDMETHOD(RemoveAll)	(THIS) PURE;
    STDMETHOD_(UINT32,Count)	(THIS) PURE;
};

#if 1
/*
 * 
 *  Interface:
 *
 *	IHXDistributedLicenseRequestStatus
 *
 *  Purpose:
 *
 *  IID_IHXDistributedLicenseRequestStatus:
 *
 *
 */
DEFINE_GUID(IID_IHXDistributedLicenseRequestStatus, 
    0x00005703, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXDistributedLicenseRequestStatus

DECLARE_INTERFACE_(IHXDistributedLicenseRequestStatus, IUnknown)
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
     *	IHXDistributedLicenseRequestStatus methods
     */
    STDMETHOD_(void,SetStatus)	(THIS_
    				DistLicReqStatus eDLRS) PURE;
    STDMETHOD_(DistLicReqStatus,GetStatus)(THIS) PURE;
};
#endif /* 0 */

#endif /* _HXRMA_DIST_LIC_REQUESTER_H_ */
