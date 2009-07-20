/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxsymbianresolv.h,v 1.5 2005/04/01 21:21:19 ehyche Exp $
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
 * terms of the GNU General Public License Version 2 or later (the
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

#ifndef HXSYMBIANRESOLV_H
#define HXSYMBIANRESOLV_H

#include <e32base.h>
#include <es_sock.h>

#include "hxtypes.h"
#include "hxcom.h"
#include "hxengin.h"
#include "ihxaccesspoint.h"
#include "hxapconresp.h"

class HXSymbianResolver : public CActive,
			  public IHXResolver
{
public:
    HXSymbianResolver(IUnknown* pContext);
    ~HXSymbianResolver();

    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)		(THIS_
					REFIID riid,
					void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)		(THIS);

    STDMETHOD_(ULONG32,Release)		(THIS);

    /*
     *	IHXResolver methods
     */

    STDMETHOD(Init)			(THIS_
					IHXResolverResponse*  pResponse);

    STDMETHOD(GetHostByName)		(THIS_
					const char* pHostName);

private:
    // CActive methods
    void RunL();
    void DoCancel();
    
    static void static_APConnectDone(void* pObj, HX_RESULT status);
    void APConnectDone(HX_RESULT status);

    void StartResolve();
    void DispatchResponse(HX_RESULT status, ULONG32 ulAddr);

    ULONG32 m_lRefCount;
    IHXResolverResponse* m_pResponse;

    IHXAccessPointManager* m_pAPManager;
    HXAccessPointConnectResp* m_pAPResponse;

    HXBOOL m_bInitialized;
    RSocketServ m_sockServ;
    RHostResolver m_resolver;
    HBufC* m_pHostname;
    TNameEntry m_nameEntry;
    ULONG32 m_ulRetryCount;
};

#endif /* HXSYMBIANRESOLV_H */
