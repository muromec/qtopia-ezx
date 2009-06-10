/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: server_control.h,v 1.3 2003/09/04 22:39:08 dcollins Exp $ 
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

#ifndef _SERVER_CONTROL_H_
#define _SERVER_CONTROL_H_

#include "hxtypes.h"
#include "hxcom.h"

#include "hxprefs.h"
#include "plgnhand.h"
#include "simple_callback.h"

class Process;
class ServerControl;
class GlobalServerControl;

class ReconfigCallback : public SimpleCallback
{
public:
    void func(Process* proc);
    IHXWantServerReconfigNotification* m_pCall;
    ServerControl* m_pOrigControl;
};

class ReconfigDoneCallback : public SimpleCallback
{
public:
    void func(Process* proc);
    GlobalServerControl* m_pgsc;
};


class GlobalServerControl : public IHXReconfigServerResponse
{
public:
    GlobalServerControl(Process* p);
    ~GlobalServerControl();

    STDMETHOD(QueryInterface)	(THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32, AddRef)	(THIS);
    STDMETHOD_(ULONG32, Release)(THIS);

    /*
     * Just like in IHXServerControl2 except this takes the
     * originating proc.
     */
    HX_RESULT ReconfigServer(IHXReconfigServerResponse* pResp,
	Process* p);
    /*
     * IHXReconfigServerResponse
     */
    STDMETHOD(ReconfigServerDone)   (THIS_
				    HX_RESULT res,
				    IHXBuffer** pInfo,
				    UINT32 ulNumInfo);
    /*
     * This comes back from the users on the different processes.
     */
    HX_RESULT ReconfigServerDone(Process*);

    /*
     * These are the same as in IHXServerReconfigNotification, except
     * these also take the originating proc.
     */

    HX_RESULT WantReconfigNotification(IHXWantServerReconfigNotification*
					    pResponse, ServerControl*);

    HX_RESULT CancelReconfigNotification(IHXWantServerReconfigNotification*
					  pResponse, ServerControl*);

private:
    friend class ReconfigDoneCallback;
    class WantNotification
    {
    public:
	IHXWantServerReconfigNotification* m_pNot;
	ServerControl* m_pOrigControl;
    };
    void _EndReconfig();
    Process* m_pReconfigResponseProc;
    IHXReconfigServerResponse* m_pReconfigureResponse;
    Process* m_paproc;
    UINT32 m_ulNotificationsSent;
    UINT32 m_ulNumWants;
    UINT32 m_ulWantNotificationsDone;
    CHXSimpleList* m_pWants;
};

class ServerControl: public IHXServerControl,
			public IHXServerControl2,
			public IHXReconfigServerResponse,
			public IHXServerReconfigNotification
{
public:
				ServerControl(Process* pProc);
				~ServerControl();
    STDMETHOD(QueryInterface)	(THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32, AddRef)	(THIS);
    STDMETHOD_(ULONG32, Release)(THIS);
    /*
     * IHXServerControl
     */
    STDMETHOD(ShutdownServer)	(THIS_ UINT32 status);

    /*
     * IHXServerControl2
     */
    STDMETHOD(RestartServer)	(THIS);
    STDMETHOD(ReconfigServer)	(THIS_ IHXReconfigServerResponse* pResp);

    /*
     * IHXReconfigServerResponse
     */
    STDMETHOD(ReconfigServerDone)   (THIS_
				    HX_RESULT res,
				    IHXBuffer** pInfo,
				    UINT32 ulNumInfo);

    /************************************************************************
     * IHXServerReconfigNotification::WantReconfigNotification
     *
     * Purpose:
     *
     *	    Tell the server that you want reconfig notification.
     */
    STDMETHOD(WantReconfigNotification)	(THIS_
		IHXWantServerReconfigNotification* pResponse);
    
    /************************************************************************
     * IHXServerReconfigNotification::CancelReconfigNotification
     *
     * Purpose:
     *
     *	    Tell the server that you no longer want reconfig notification.
     */
    STDMETHOD(CancelReconfigNotification)   (THIS_
		IHXWantServerReconfigNotification* pResponse);

private:
    friend class GlobalServerControl;
    Process*			m_proc;
    LONG32			m_lRefCount;
};

#endif /* _SERVER_CONTROL_H_ */
