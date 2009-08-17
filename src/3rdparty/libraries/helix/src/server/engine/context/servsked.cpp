/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: servsked.cpp,v 1.4 2003/09/04 22:35:34 dcollins Exp $ 
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

#include "hxtypes.h"
#include "hxcom.h"
#include "hxerror.h"
#include "proc.h"
#include "servsked.h"
#include "timeval.h"
#include "server_engine.h"
#include "servpq.h"
#include "globals.h"

ServerScheduler::ServerScheduler(Process* pProc):
    m_lRefCount(0),
    m_pProc(pProc)
{
}

ServerScheduler::~ServerScheduler()
{
}

/* IHXScheduler methods */

STDMETHODIMP
ServerScheduler::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
	AddRef();
	*ppvObj = (IUnknown*)(IHXScheduler*)this;
	return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXScheduler))
    {
	AddRef();
	*ppvObj = (IHXScheduler*)this;
	return HXR_OK;
    }
    else if ((*g_bITimerAvailable) && IsEqualIID(riid, IID_IHXAccurateClock))
    {
	AddRef();
	*ppvObj = (IHXAccurateClock*)this;
	return HXR_OK;
    }
    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(UINT32)
ServerScheduler::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(UINT32)
ServerScheduler::Release()
{
    if(InterlockedDecrement(&m_lRefCount) > 0)
    {
	return m_lRefCount;
    }
    delete this;
    return 0;
}

STDMETHODIMP_(CallbackHandle)
ServerScheduler::AbsoluteEnter(IHXCallback* pCallback, HXTimeval sTVal)
{
    Timeval tVal((INT32)sTVal.tv_sec, (INT32)sTVal.tv_usec);
    return m_pProc->pc->engine->schedule.enter(tVal, pCallback);
}

STDMETHODIMP_(CallbackHandle)
ServerScheduler::RelativeEnter(IHXCallback* pCallback, UINT32 ms)
{
    /*
     * A RelativeEnter() of 0 ms is a special case that needs to be
     * AbsoluteEnter() of 0
     */

    if (ms == 0)
    {
	HXTimeval rVal;

	rVal.tv_sec = rVal.tv_usec = 0;
	return AbsoluteEnter(pCallback, rVal);
    }

    UINT32 usecs = 0;
    UINT32 secs = 0;

    if (ms > 4000000)
    {
	secs = ms / 1000;
	usecs = (ms % 1000) * 1000;
    }
    else
    {
	usecs = ms * 1000;
	secs = 0;

	if(usecs >= 1000000)
	{
	    secs = usecs / 1000000;
	    usecs = usecs % 1000000;
	}
    }

    Timeval tVal((INT32)secs, (INT32)usecs);
    Timeval tNow = m_pProc->pc->engine->now;
    return m_pProc->pc->engine->schedule.enter(tNow + tVal, pCallback);
}

STDMETHODIMP
ServerScheduler::Remove(CallbackHandle handle)
{
    m_pProc->pc->engine->schedule.remove(handle);
    return HXR_OK;
}

STDMETHODIMP_(HXTimeval)
ServerScheduler::GetCurrentSchedulerTime()
{
    Timeval tVal = m_pProc->pc->engine->now;
    // stupid conversion stuff until it gets changed...
    HXTimeval timeVal;
    timeVal.tv_sec = tVal.tv_sec;
    timeVal.tv_usec = tVal.tv_usec;
    return timeVal;
}

STDMETHODIMP_(HXTimeval)
ServerScheduler::GetTimeOfDay()
{
    Timeval tVal = *g_pNow;
    // stupid conversion stuff until it gets changed...
    HXTimeval timeVal;
    timeVal.tv_sec = tVal.tv_sec;
    timeVal.tv_usec = tVal.tv_usec;
    return timeVal;
}



/***********************************************************************
 * Thread-safe scheduler
 */

ServerIScheduler::ServerIScheduler(Process* pProc):
    m_lRefCount(0),
    m_pProc(pProc)
{
}

ServerIScheduler::~ServerIScheduler()
{
}

/* IHXScheduler methods */

STDMETHODIMP
ServerIScheduler::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
       AddRef();
       *ppvObj = (IUnknown*)(IHXScheduler*)this;
       return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXThreadSafeScheduler))
    {
       AddRef();
       *ppvObj = (IHXThreadSafeScheduler*)this;
       return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXScheduler))
    {
       AddRef();
       *ppvObj = (IHXScheduler*)this;
       return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(UINT32)
ServerIScheduler::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(UINT32)
ServerIScheduler::Release()
{
    if(InterlockedDecrement(&m_lRefCount) > 0)
    {
       return m_lRefCount;
    }
    delete this;
    return 0;
}

STDMETHODIMP_(CallbackHandle)
ServerIScheduler::AbsoluteEnter(IHXCallback* pCallback, HXTimeval sTVal)
{
    Timeval tVal((INT32)sTVal.tv_sec, (INT32)sTVal.tv_usec);
    return m_pProc->pc->engine->ischedule.enter(tVal, pCallback);
}

STDMETHODIMP_(CallbackHandle)
ServerIScheduler::RelativeEnter(IHXCallback* pCallback, UINT32 ms)
{
    /*
     * A RelativeEnter() of 0 ms is a special case that needs to be
     * AbsoluteEnter() of 0
     */

    if (ms == 0)
    {
       HXTimeval rVal;

       rVal.tv_sec = rVal.tv_usec = 0;
       return AbsoluteEnter(pCallback, rVal);
    }

    UINT32 usecs = 0;
    UINT32 secs = 0;

    if (ms > 4000000)
    {
       secs = ms / 1000;
       usecs = (ms % 1000) * 1000;
    }
    else
    {
       usecs = ms * 1000;
       secs = 0;

       if(usecs >= 1000000)
       {
           secs = usecs / 1000000;
           usecs = usecs % 1000000;
       }
    }

    Timeval tVal((INT32)secs, (INT32)usecs);
    Timeval tNow = m_pProc->pc->engine->now;
    return m_pProc->pc->engine->ischedule.enter(tNow + tVal, pCallback);
}
STDMETHODIMP
ServerIScheduler::Remove(CallbackHandle handle)
{
    m_pProc->pc->engine->ischedule.remove(handle);
    return HXR_OK;
}

STDMETHODIMP_(HXTimeval)
ServerIScheduler::GetCurrentSchedulerTime()
{
    Timeval tVal = m_pProc->pc->engine->now;
    // stupid conversion stuff until it gets changed...
    HXTimeval timeVal;
    timeVal.tv_sec = tVal.tv_sec;
    timeVal.tv_usec = tVal.tv_usec;
    return timeVal;
}


