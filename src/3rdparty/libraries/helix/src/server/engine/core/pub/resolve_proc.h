/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: resolve_proc.h,v 1.1 2004/11/15 22:39:58 tmarshall Exp $
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

#ifndef _RESOLVE_PROC_H_
#define _RESOLVE_PROC_H_

#include "simple_callback.h"

_INTERFACE IHXSockAddr;
_INTERFACE IHXAddrInfo;

class CServNativeResolve;

#define MAX_RESOLVER_PROCESSES 16

class CResolveProcPool
{
public:
    CResolveProcPool(void);
    ~CResolveProcPool(void);

    void        GetProc(CServNativeResolve* pResolve, Process* pCallingProc);
    void        PutProc(Process* pProc);

protected:
    HX_MUTEX            m_Mutex;
    UINT32              m_nProcesses;
    CHXSimpleList       m_waiters;
    Process*            m_pool[MAX_RESOLVER_PROCESSES];
};

class CreateResolveCallback : public SimpleCallback
{
public:
    virtual ~CreateResolveCallback(void) {}

    virtual void func(Process* pProc);

    Process*            m_pCallingProc;
    CResolveProcPool*   m_pCaller;

    SimpleCallback*     resolver_cb;
};

class ResolveProcessInitCallback : public SimpleCallback
{
public:
    virtual ~ResolveProcessInitCallback(void) {}

    virtual void func(Process* pProc);

    Process*            m_pProc;                // Main's Process Class
    Process*            m_pCallingProc;
    CResolveProcPool*   m_pCaller;
};

class ResolveProcessReadyCallback : public SimpleCallback
{
public:
    virtual ~ResolveProcessReadyCallback(void) {}

    virtual void func(Process* pProc);

    Process*            m_pCallingProc;
    CResolveProcPool*   m_pCaller;
    Process*            m_pResolveProc;
};

class ResolveDispatchAddrRequest : public SimpleCallback
{
public:
    virtual ~ResolveDispatchAddrRequest(void) {}

    virtual void func(Process* proc);

    Process*            m_pCallingProc;
    CServNativeResolve* m_pCaller;
    const char*         m_pNode;
    const char*         m_pService;
    IHXAddrInfo*        m_pHints;
};

class ResolveDispatchAddrResponse : public SimpleCallback
{
public:
    virtual ~ResolveDispatchAddrResponse(void) {}

    virtual void func(Process* proc);

    Process*            m_pCallingProc;
    CServNativeResolve* m_pCaller;
    HX_RESULT           m_hxr;
    UINT32              m_nVecLen;
    IHXSockAddr**       m_ppAddrVec;
};

class ResolveDispatchNameRequest : public SimpleCallback
{
public:
    virtual ~ResolveDispatchNameRequest(void) {}

    virtual void func(Process* proc);

    Process*            m_pCallingProc;
    CServNativeResolve* m_pCaller;
    IHXSockAddr*        m_pAddr;
    UINT32              m_uFlags;
};

class ResolveDispatchNameResponse : public SimpleCallback
{
public:
    virtual ~ResolveDispatchNameResponse(void) {}

    virtual void func(Process* proc);

    int                 m_iCallingProcNum;
    CServNativeResolve* m_pCaller;
    HX_RESULT           m_hxr;
    char                m_szNode[256]; //XXX
    char                m_szService[256]; //XXX
};

#endif /* _RESOLVE_PROC_H_ */
