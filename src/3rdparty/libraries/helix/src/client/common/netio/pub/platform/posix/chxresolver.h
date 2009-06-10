/* ***** BEGIN LICENSE BLOCK *****
 * Version: RCSL 1.0/RPSL 1.0
 *
 * Portions Copyright (c) 1995-2002 RealNetworks, Inc. All Rights Reserved.
 *
 * The contents of this file, and the files included with this file, are
 * subject to the current version of the RealNetworks Public Source License
 * Version 1.0 (the "RPSL") available at
 * http://www.helixcommunity.org/content/rpsl unless you have licensed
 * the file under the RealNetworks Community Source License Version 1.0
 * (the "RCSL") available at http://www.helixcommunity.org/content/rcsl,
 * in which case the RCSL will apply. You may also obtain the license terms
 * directly from RealNetworks.  You may not use this file except in
 * compliance with the RPSL or, if you have a valid RCSL with RealNetworks
 * applicable to this file, the RCSL.  Please see the applicable RPSL or
 * RCSL for the rights, obligations and limitations governing use of the
 * contents of the file.
 *
 * This file is part of the Helix DNA Technology. RealNetworks is the
 * developer of the Original Code and owns the copyrights in the portions
 * it created.
 *
 * This file, and the files included with this file, is distributed and made
 * available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 *
 * Technology Compatibility Kit Test Suite(s) Location:
 *    http://www.helixcommunity.org/content/tck
 *
 * Contributor(s):
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef CHXRESOLVER_H__
#define CHXRESOLVER_H__

#include "unkimp.h"
#include "hxnet.h"
#include "hxtaskmanager.h"
#include "hxnetdrvloader.h"

class NameInfoTask;
class ResolverTask;

// class CHXResolver
class CHXResolver 
: public CUnknownIMP
, public IHXResolve
{
// IUnknown
    DECLARE_UNKNOWN_NOCREATE(CHXResolver)
private:
    CHXResolver(const CHXResolver&);
    CHXResolver& operator=(const CHXResolver&);

public:
    // XXX HP, revise the constructor after IHXNetServices can be QI-ed
    //         from pContext
    CHXResolver(IUnknown* pContext, IHXNetServices* pServices);
    ~CHXResolver();

// IHXResolve
    STDMETHOD(Init) (THIS_ IHXResolveResponse* pResponse);
    STDMETHOD(Close)                (THIS);
    STDMETHOD(GetAddrInfo)          (THIS_ const char* pNode, const char* pServ, IHXAddrInfo* pHints);
    STDMETHOD(GetNameInfo)          (THIS_ IHXSockAddr* pAddr, UINT32 uFlags);

private:
    friend class NameInfoTask;
    friend class ResolverTask;
    void OnNameInfoTaskComplete(HX_RESULT hr, NameInfoTask* pTask);
    void OnResolverTaskComplete(HX_RESULT hr, ResolverTask* pResolverTask);


    IHXResolveResponse*     m_pResponse;
    HXTaskManager*          m_pTaskManager;
    IUnknown*		    m_pContext;
    IHXNetServices*         m_pNetServices;
    HXNetDrvLoader          m_netDrvLoader;
};


#endif /* ndef CHXRESOLVER_H__ */
