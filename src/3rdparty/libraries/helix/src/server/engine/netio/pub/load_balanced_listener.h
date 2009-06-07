/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: load_balanced_listener.h,v 1.3 2004/05/03 19:02:49 tmarshall Exp $ 
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

#ifndef _LOAD_BALANCED_LISTENER_H_
#define _LOAD_BALANCED_LISTENER_H_

#include "simple_callback.h"
#include "plgnhand.h"
#include "acceptor.h"

class Process;
class CHXSimpleList;
class LBLAcceptor;
class LBLConnDispatch;
class CoreInvokeAcceptorCallback;
struct IHXListenResponse;

class LoadBalancedListenerManager
{
public:

    /*
     * Run in the CoreProc's process space.
     */
    LoadBalancedListenerManager(Process* pCoreProc);
    ~LoadBalancedListenerManager();

    /*
     * Run in the Plugin's process space (calling Listen response
     * is valid.
     */
    void* AddListener(Process* pPluginProc, REFIID ID, UINT32 ulLocalAddr,
		     UINT16 port, IHXListenResponse* pListenResponse,
		     UINT32 ulReserveDescriptors, UINT32 m_ulReserveSockets);

    void RemoveListener(Process* pProc,
	REFIID ID, void* pLoadBalancedListenerHandle);

    class ListeningProcessEntry
    {
    public:
	Process*	    m_pPluginProc;
	IHXListenResponse* m_pListenResponse;
    };

    class ListenerEntry
    {
    public:
	ListenerEntry()
	    : m_ppAcceptors(0)
	    , m_ulNumAcceptors(0)
	{}
    	IID			m_ID;
	UINT32			m_ulLocalAddr;
	UINT16			m_unPort;
	LBLAcceptor**		m_ppAcceptors;
	UINT32			m_ulNumAcceptors;
	UINT32			m_ulReserveDescriptors;
	UINT32			m_ulReserveSockets;

	CHXSimpleList*		m_pListeners; /* ListeningProcessEntry */

	class CreateAcceptorCallback : public SimpleCallback
	{
	public:
	    void            func(Process *proc);
	    ListenerEntry*  m_pEntry;
	private:
			    ~CreateAcceptorCallback() {};

	};
	class DestroyAcceptorCallback : public SimpleCallback
	{
	public:
	    void            func(Process *proc);
	    ListenerEntry*  m_pEntry;
	private:
			    ~DestroyAcceptorCallback() {};

	};
    };

private:
    Process*		m_pCoreProc;
    CHXSimpleList*	m_pListenEntries; /* ListenerEntry */

    friend class LBLAcceptor;
};

class LBLAcceptor : public Acceptor
{
public:
			LBLAcceptor(Process* pProc,
			    LoadBalancedListenerManager::ListenerEntry* _entry);
			~LBLAcceptor();

    void		Accepted(TCPIO* tcp_io, sockaddr_in peer,
			     int peerlen, int port);

    int			GetBestProcess(REF(IHXListenResponse*) pResp,
				       REF(PluginHandler::Plugin*) pPlugin);
    LBLConnDispatch*				    m_pCDispatch;
private:
    LoadBalancedListenerManager::ListenerEntry*	    m_pEntry;
    BOOL					    m_bDefunct;

    friend class CoreInvokeAcceptorCallback;
    friend class LoadBalancedListenerManager;
};

#endif
