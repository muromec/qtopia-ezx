/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: servsked.h,v 1.3 2003/03/11 00:44:45 dcollins Exp $ 
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

#ifndef _SERVSKED_H_
#define _SERVSKED_H_

#include "hxengin.h"
#include "hxcomm.h"

class Process;

class ServerScheduler: public IHXScheduler,
		       public IHXAccurateClock
{
public:
    ServerScheduler(Process* pProc);
    ~ServerScheduler();

    STDMETHOD(QueryInterface)   (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)  (THIS);
    STDMETHOD_(ULONG32,Release) (THIS);
    STDMETHOD_(CallbackHandle,RelativeEnter) 
					(THIS_ IHXCallback* pCallback,
				         UINT32 ms);
    STDMETHOD_(CallbackHandle,AbsoluteEnter) 
					(THIS_ IHXCallback* pCallback,
				         HXTimeval tVal);
    STDMETHOD(Remove)		(THIS_ CallbackHandle handle);
    STDMETHOD_(HXTimeval,GetCurrentSchedulerTime) (THIS);    
    STDMETHOD_(HXTimeval,GetTimeOfDay) (THIS);

private:
    LONG32	m_lRefCount;
    Process*	m_pProc;
};

class ServerIScheduler: public IHXThreadSafeScheduler,
                        public IHXScheduler
{
public:
    ServerIScheduler(Process* pProc);
    ~ServerIScheduler();

    STDMETHOD(QueryInterface)   (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)  (THIS);
    STDMETHOD_(ULONG32,Release) (THIS);
    STDMETHOD_(CallbackHandle,RelativeEnter)
                                       (THIS_ IHXCallback* pCallback,
                                        UINT32 ms);
    STDMETHOD_(CallbackHandle,AbsoluteEnter)
                                       (THIS_ IHXCallback* pCallback,
                                        HXTimeval tVal);
    STDMETHOD(Remove)          (THIS_ CallbackHandle handle);
    STDMETHOD_(HXTimeval,GetCurrentSchedulerTime) (THIS);

private:
    LONG32     m_lRefCount;
    Process*   m_pProc;
};

#endif /* _SERVSKED_H_ */
