/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: idist_lic_requester.h,v 1.3 2003/09/04 22:39:08 dcollins Exp $ 
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

#ifndef _IDIST_LIC_REQUESTER_H_
#define _IDIST_LIC_REQUESTER_H_

#include <limits.h>

#include "hxtypes.h"
#include "hxcom.h"
#include "hxdist_lic_requester.h"
#include "proc_container.h"
#include "proc.h"

class DistributedLicenseRequester;

/*
 * 
 *  Class:
 *      HXDistributedLicenseRequester
 *
 *  Purpose:
 *      Wrapper class for DistributedLicenseRequester just like
 *  HXRegistry is for ServerRegistry.
 *
 */
class HXDistributedLicenseRequester : public IHXRNDistributedLicenseRequester
{
public:
			HXDistributedLicenseRequester(Process* p);
		        ~HXDistributedLicenseRequester();

    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)	(THIS);

    STDMETHOD_(ULONG32,Release)	(THIS);

    /*
     *	IHXRNDistributedLicenseRequester methods
     */
    STDMETHOD_(DistLicReqStatus,LicenseRequestStatus)(THIS_
    				ULONG32 ulClientID);
    STDMETHOD_(DistLicReqStatus,Insert)(THIS_
    				ULONG32 ulClientID,
				IHXCallback* pCb,
				REF(IHXCallback*) pParentCb);
    STDMETHOD_(INT32,Execute)	(THIS_
				INT32 nNumLicences);
    STDMETHOD(Remove)		(THIS_
    				ULONG32 ulClientID,
				IHXCallback* pCb);
    STDMETHOD(RemoveAll)	(THIS);
    STDMETHOD_(UINT32,Count)	(THIS);

protected:
    UINT32			m_ulRefCount;
    Process*                    m_pProc;
    DistributedLicenseRequester* m_pDistributedLicenseRequester;
};

#endif /* _IDIST_LIC_REQUESTER_H_ */
