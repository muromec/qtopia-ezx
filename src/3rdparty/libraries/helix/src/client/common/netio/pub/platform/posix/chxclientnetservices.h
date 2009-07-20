/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: chxclientnetservices.h,v 1.7 2006/04/20 23:49:13 ping Exp $
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

#ifndef CHXCLIENTNETSERVICES_H__
#define CHXCLIENTNETSERVICES_H__


#include "hxnet.h"
#include "sockimp.h"
#if defined(THREADS_SUPPORTED)
#include "hxthreadtaskdriver.h"
#endif

class CHXClientNetServicesBase : public CHXNetServices
{
public:
    CHXClientNetServicesBase();
    virtual ~CHXClientNetServicesBase();

    // CHXNetServices
    HX_RESULT CreateSocket  (HXSockFamily f,
                             HXSockType t,
                             HXSockProtocol p,
                             HX_SOCK s,
                             IHXSocket** ppSock);

    //IHXNetServices
    STDMETHOD(CreateSocket) (THIS_ IHXSocket** ppSock);

protected:
    struct HXSOCKET_CREATE_PARAMS
    {
        HXSockFamily    f;
        HXSockType      t;
        HXSockProtocol  p;
        HX_SOCK         sock;
    };

    virtual HX_RESULT CreateSocketHelper(IHXSocket** ppSock, 
                                         const HXSOCKET_CREATE_PARAMS* pParams = 0) = 0;

    HX_RESULT CreateClientSocket(IHXSocket*& pSock, 
                                 CHXNetServices* pNetServices, 
                                 const HXSOCKET_CREATE_PARAMS* pParams);
};

class CHXClientNetServices : public CHXClientNetServicesBase
{
public:
    CHXClientNetServices();
    virtual ~CHXClientNetServices();

    // IHXContextUser
    STDMETHOD (RegisterContext)	    (THIS_  IUnknown* pIContext);

    // IHXNetServices
    STDMETHOD(CreateResolver)       (THIS_ IHXResolve** ppResolver);

protected:

    virtual HX_RESULT CreateSocketHelper(IHXSocket** ppSock, 
                                         const HXSOCKET_CREATE_PARAMS* pParams = 0);
};

#if defined(THREADS_SUPPORTED)

class CHXNetThreadClientNetServices : public CHXClientNetServices
{
public:
    CHXNetThreadClientNetServices();
    ~CHXNetThreadClientNetServices();

    // IHXContextUser
    STDMETHOD (RegisterContext)	    (THIS_  IUnknown* pIContext);

    //IHXNetServices
    STDMETHOD(CreateResolver)       (THIS_ IHXResolve** ppResolver);

protected:
    // helpers for CreateSocket()
    HX_RESULT CreateSocketHelper(IHXSocket** ppSock, 
                                 const HXSOCKET_CREATE_PARAMS* pParams = 0);

    HX_RESULT DoDriverInit();

private:
    UINT32              m_ulDriverThreadID;
    HXThreadTaskDriver* m_pDriver;
};

#endif

CHXClientNetServices*   CreateClientNetServices(IUnknown* pContext);

#endif /*CHXCLIENTNETSERVICES_H__*/
