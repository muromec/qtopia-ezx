/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: core_proc.h,v 1.10 2005/01/25 22:55:21 jzeng Exp $
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

#ifndef _CORE_PROC_H_
#define _CORE_PROC_H_

#include "simple_callback.h"
#include "base_callback.h"
#include "callback.h"


#if !defined COREPROC_LOADPLUGINS && !defined _WIN32
#define COREPROC_LOADPLUGINS
#endif

#if !defined WIN32
class FDPassSocket;
#endif
class Client;
class Config;
class CHXSimpleList;
class DispatchQueue;
class Process;
class ServerRegistry;
class ServerAccessControl;
class LBLAcceptor;

struct IHXBuffer;
class LBLConnDispatch;

class CoreProcessInitCallback : public SimpleCallback
{
public:
    void                func(Process* proc);
    int                 argc;
    char**              argv;
    int* VOLATILE       controller_ready;
    int* VOLATILE       controller_waiting_on_core;
    UINT32*             return_uid;
    UINT32*             return_gid;
    DispatchQueue*      dispatch_queue;
    Process**           core_proc;

private:
                        ~CoreProcessInitCallback() {};
    void                _initializeListenRespObjects(
                            Process* proc,
                            Config* config,
                            ServerRegistry* registry,
                            int backlog);

};

class TimeZoneCheckCallback : public IHXCallback
{

protected:

    Process* m_pProc;
    UINT32 m_RefCount;

public:

    TimeZoneCheckCallback(Process* pProc);


    // IUnknown methods

    STDMETHODIMP QueryInterface(REFIID riid,
                                void** ppvObj);

    STDMETHODIMP_(UINT32) AddRef();

    STDMETHODIMP_(UINT32) Release();


    // IHXCallback methods

    STDMETHODIMP Func();

    HX_RESULT ScheduleFirstCheckCallback();
    HX_RESULT SetTimeZone();
};

class CoreSystemReadyCallback : public SimpleCallback
{
public:
    void                func(Process* proc);
#if !defined WIN32
    FDPassSocket*       fdp;
#endif
private:
                        ~CoreSystemReadyCallback() {};

};

class CoreFDPassSocketCallback : public SimpleCallback
{
public:
    CoreFDPassSocketCallback() {m_pLBLConnDispatch = NULL;
                                m_pAllowSendMutex = NULL;
                                m_bNeedEnableStreamer = FALSE; }
    void                func(Process* proc);
#if !defined WIN32
    FDPassSocket*       fdp;
#endif
    int                 streamer_num;
    LBLAcceptor*        acceptor;
    VOLATILE BOOL* m_pAllowSendMutex;
    LBLConnDispatch*    m_pLBLConnDispatch;
    BOOL                m_bNeedEnableStreamer;
private:
                        ~CoreFDPassSocketCallback() {};

};

class CoreInvokeAcceptorCallback : public SimpleCallback
{
public:
    void                func(Process* proc);
    LBLAcceptor*        acceptor;
private:
                        ~CoreInvokeAcceptorCallback() {};

};

class CHXServSocket;

class CorePassSockCallback : public BaseCallback
{
public:
    STDMETHOD(Func)     (THIS);
    CHXServSocket*      m_pSock;
    Process*            m_pProc;
    int                 m_iNewProc;
};

#ifdef PAULM_LEAKCHECK
class MemChecker : public BaseCallback
{
public:
    MemChecker(Process* pProc);
    STDMETHOD(Func)     (THIS);
    Process*            m_pProc;
    enum State
    {
        ALLOC_ON, ALLOC_OFF
    };
    State m_state;
    BOOL m_bOnce;
private:

};
#endif /* PAULM_LEAKCHECK */

#endif /* _CORE_PROC_H_ */
