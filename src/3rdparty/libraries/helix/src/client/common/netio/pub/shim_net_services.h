/* ***** BEGIN LICENSE BLOCK *****
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

#ifndef SHIM_NET_SERVICES_H
#define SHIM_NET_SERVICES_H

#include "hxprefs.h"
#include "hxnet.h"
#include "hxnetapi.h"
#include "ihxcontextuser.h"
#include "unkimp.h"
#include "baseobj.h"
#include "hxcbobj.h"



class CHXClientNetServicesShim : public CUnknownIMP
			       , public IHXNetServices
#ifdef HELIX_FEATURE_SECURE_SOCKET
			       , public IHXSecureNetServices
#endif
			       , public IHXContextUser
			       , public CHXBaseCountingObject
{
public:
    DECLARE_UNKNOWN(CHXClientNetServicesShim)
    CHXClientNetServicesShim();
    virtual ~CHXClientNetServicesShim();

    // IHXNetServices methods (overridden from CHXClientNetServices)
    STDMETHOD(CreateResolver)       (THIS_ IHXResolve** ppResolver);
    STDMETHOD(CreateSockAddr)       (THIS_ HXSockFamily f, IHXSockAddr** ppAddr);
    STDMETHOD(CreateListeningSocket)(THIS_ IHXListeningSocket** ppSock);
    STDMETHOD(CreateSocket)         (THIS_ IHXSocket** ppSock);
#ifdef HELIX_FEATURE_SECURE_SOCKET
    STDMETHOD(CreateSecureSocket)(THIS_ IHXSecureSocket** ppSock) ;
#endif
    // IHXContextUser
    STDMETHOD (RegisterContext)	    (THIS_  IUnknown* pIContext);

    HX_RESULT _InternalQI(REFIID riid, void** ppvObj);

    // CHXClientNetServices
    virtual void Close(void);

protected:
    IUnknown*           m_punkContext;    
    HXBOOL		m_bUseShim;
    HXNetworkServices*	m_pOldNetServicesImp;
    IHXNetworkServices* m_pOldNetServices;   // used if shim is 'on'
    IHXNetServices*     m_pDefNetServices;   // used if shim is 'off'

#ifdef _UNIX    
# if defined(_UNIX_THREADED_NETWORK_IO)
    HXBOOL              m_bNetworkThreading;
# endif    
    IHXScheduler*       m_pScheduler;
    CHXGenericCallback* m_pUnixNetworkPump;
    friend void UnixNetworkPump(void* pArg);
#endif

};

#endif /* #ifndef SHIM_NET_SERVICES_H */
