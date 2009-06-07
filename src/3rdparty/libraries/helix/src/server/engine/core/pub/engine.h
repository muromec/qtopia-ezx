/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: engine.h,v 1.2 2003/01/23 23:42:54 damonlan Exp $ 
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

#ifndef _ENGINE_H_
#define _ENGINE_H_

#include <setjmp.h>
#include "timeval.h"
#include "servpq.h"
#include "callback_container.h"

class SharedUDPPortReader;

class Engine {
public:
			    Engine();
	virtual		    ~Engine() { }
	virtual void	    mainloop() { }
	virtual void	    preevent() { }
	virtual void	    postevent() { }
	virtual void	    Dispatch() { }
	virtual void	    RegisterDesc() { }
	virtual void	    UnRegisterDesc() { }
	virtual void	    RegisterSock() { }
	virtual void	    UnRegisterSock() { }
#ifdef _UNIX
	virtual void	    HandleBadFds() { }
#endif
	virtual int	    i_own_mutex(){return 0;}

	SharedUDPPortReader*	GetSharedUDPReader() { return m_pSharedUDPReader; }
	void			SetSharedUDPReader(SharedUDPPortReader* reader)
					{ m_pSharedUDPReader = reader; }

	CallbackContainer   callbacks;
	ServPQ	    	    schedule;
	ServPQ		    ischedule;
	Timeval		    now;
	virtual void*	    get_proc(){return 0;} 
	UINT32		    m_ulMainloopIterations;

	// Streamers init this when they want to use a shared resend port.
	SharedUDPPortReader* m_pSharedUDPReader;

#if defined _WIN32
	void KillSelect();
	void InitKillSelect();
	class CatchKillSelect : public IHXCallback
	{
	public:
	    /*
	     *  IUnknown methods
	     */
	    STDMETHOD(QueryInterface)	(THIS_
					REFIID riid,
					void** ppvObj);

	    STDMETHOD_(ULONG32,AddRef)	(THIS);

	    STDMETHOD_(ULONG32,Release)	(THIS);

	    /*
	     *  IHXCallback methods
	     */

	    /************************************************************************
	     *	Method:
	     *	    IHXCallback::Func
	     *	Purpose:
	     *	    This is the function that will be called when a callback is
	     *	    to be executed.
	     */
	    STDMETHOD(Func)		(THIS);
	    SOCKET m_sock;
	};
	SOCKET m_sock;
#endif
};

#endif /* _ENGINE_H_ */
