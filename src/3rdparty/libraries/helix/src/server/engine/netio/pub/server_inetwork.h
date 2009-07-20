/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: server_inetwork.h,v 1.3 2003/09/04 22:39:09 dcollins Exp $ 
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

#ifndef _SERVER_INETWORK_H_
#define _SERVER_INETWORK_H_

#include "hxcom.h"
#include "hxengin.h"
#include "sockio.h"
#include "base_callback.h"
#include "proc.h"
#include "server_engine.h"
#include "inetwork.h"
#include "resolver_dispatch.h"

class TCPIO;
class SIO;
class CByteQueue;
class CServerResolverContext;

#ifdef _UNIX
#include "resolver_mux.h"

class Resolver2CoreCallback : public SimpleCallback,
				public ResolverMUXResp
{
public:
    Resolver2CoreCallback(const char* pHostname, Process* pOrigProc,
	    			CServerResolverContext* pContext);
    ~Resolver2CoreCallback();
    
    void func(Process* proc);
    HX_RESULT GetHostByNameDone(HX_RESULT status, ULONG32 ulAddr);
    
private:
    enum state{NONE, IN_CORE, BACK};
    state m_state;
    char* m_pHostname;
    Process* m_pCoreProc;
    Process* m_pOrigProc;
    CServerResolverContext* m_pContext;
    ULONG32 m_ulAddr;
    HX_RESULT m_status;
};
#endif

class CServerResolverContext: public IHXResolverContext
{
public:
    CServerResolverContext(Process* pProc);
    ~CServerResolverContext();

    STDMETHOD(GetHostByName)	(THIS_ const char* pHostName);

    class ResolverContextCallback: public ResolverCallback
    {
    public:
        ResolverContextCallback();
	void ArmCallback(CServerResolverContext* pContext);
        ~ResolverContextCallback();
	STDMETHOD_(ULONG32,AddRef)  (THIS);
	STDMETHOD_(ULONG32,Release) (THIS);
	void func(Process* pProc);
	CServerResolverContext* m_pContext;

    private:
	LONG32			m_lRefCount;
    };
    friend class ResolverContextCallback;
#if defined _UNIX
    friend class Resolver2CoreCallback;
#endif
    ResolverContextCallback*	m_pResolverCallback;

private:
    Process*			m_proc;
    BOOL			m_bResolverCallbackPending;
};

class CServerListenSocketContext : public IHXListenSocketContext
				 , public IHXLoadBalancedListen
{
public:

	CServerListenSocketContext(Process* pProc, Engine* pEngine,
				IHXErrorMessages* pMessages,
				ServerAccessControl* pAccessCtrl);
	~CServerListenSocketContext();

	STDMETHOD(QueryInterface)(THIS_ REFIID riid, void** ppvObj);

	STDMETHOD_(ULONG32,AddRef)  (THIS);

	STDMETHOD_(ULONG32,Release) (THIS);

	STDMETHOD(SetID)            (THIS_
				    REFIID ID);

	STDMETHOD(SetReserveLimit)  (THIS_
				    UINT32          ulDescriptors,
				    UINT32          ulSockets);

	STDMETHOD(Init)             (THIS_
				    UINT32              ulLocalAddr,        
				    UINT16              port,
				    IHXListenResponse* pListenResponse);
private:
    Process*	m_pProc;
    BOOL	m_bLoadBalancedListener;
    IID		m_LoadBalancedListenerID;
    void*	m_pLoadBalancedListenerHandle;
    UINT32	m_ulReserveDescriptors;
    UINT32	m_ulReserveSockets;
};

#endif /*_SERVER_INETWORK_H_*/
