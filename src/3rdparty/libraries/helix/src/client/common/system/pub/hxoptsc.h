/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxoptsc.h,v 1.10 2008/09/07 11:14:32 pbasic Exp $
 * 
 * Portions Copyright (c) 1995-2004 RealNetworks, Inc. All Rights Reserved.
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
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU General Public License Version 2 (the
 * "GPL") in which case the provisions of the GPL are applicable
 * instead of those above. If you wish to allow use of your version of
 * this file only under the terms of the GPL, and not to allow others
 * to use your version of this file under the terms of either the RPSL
 * or RCSL, indicate your decision by deleting the provisions above
 * and replace them with the notice and other provisions required by
 * the GPL. If you do not delete the provisions above, a recipient may
 * use your version of this file under the terms of any one of the
 * RPSL, the RCSL or the GPL.
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

#ifndef _IHXOptimizedScheduler_
#define _IHXOptimizedScheduler_

class  ClientPQ;
class  CHXID;
class  HXMutex;
struct IUnknown;
struct IHXScheduler;

typedef struct _HXTimeval HXTimeval;

#include "timeval.h"

class CAsyncTimer;

// HXOptimizedSchedulerBase is a base class that allows the core
// to control the scheduler object without knowing the actual 
// implementation of the object.
class HXOptimizedSchedulerBase : public IHXOptimizedScheduler
			       , public IHXOptimizedScheduler2
{
public:

    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)		(THIS_
					REFIID riid,
					void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)		(THIS) PURE;

    STDMETHOD_(ULONG32,Release)		(THIS) PURE;

    /*
     *	IHXOptimizedScheduler methods
     */

    /************************************************************************
     *	Method:
     *	    IHXOptimizedScheduler::RelativeEnter
     *	Purpose:
     *	    Schedule a callback to be executed "ms" milliseconds from now
     *	    This function is less percise then AbsoluteEnter and should only
     *	    be used when accurate timing is not critical.
     */
    STDMETHOD_(CallbackHandle,RelativeEnter)	(THIS_
						IHXCallback* pCallback,
						UINT32 ms) PURE;

    /************************************************************************
     *	Method:
     *	    IHXOptimizedScheduler::AbsoluteEnter
     *	Purpose:
     *	    Schedule a callback to be executed at time "tVal".
     */
    STDMETHOD_(CallbackHandle,AbsoluteEnter)	(THIS_
						IHXCallback* pCallback,
						HXTimeval tVal) PURE;

    /************************************************************************
     *	Method:
     *	    IHXOptimizedScheduler::Remove
     *	Purpose:
     *	    Remove a callback from the scheduler.
     */
    STDMETHOD(Remove)		(THIS_
			    	CallbackHandle Handle) PURE;

    /************************************************************************
     *	Method:
     *	    IHXOptimizedScheduler::GetCurrentSchedulerTime
     *	Purpose:
     *	    Gives the current time (in the timeline of the scheduler).
     */
    STDMETHOD_(HXTimeval,GetCurrentSchedulerTime)	(THIS) PURE;

    // IHXOptimizedScheduler2 methods
    STDMETHOD(StartScheduler)		(THIS) PURE;
    STDMETHOD(StopScheduler)		(THIS) PURE;
};

class HXOptimizedScheduler : public HXOptimizedSchedulerBase
{
public:
			HXOptimizedScheduler(IUnknown* pContext);
			~HXOptimizedScheduler();

    /*
     * IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)	(THIS);

    STDMETHOD_(ULONG32,Release)	(THIS);

    /*
     * HXOptimizedScheduler methods
     */
    /************************************************************************
     *	Method:
     *	    IHXOptimizedScheduler::RelativeEnter
     *	Purpose:
     *	    enter objects in the service queue offset from time now
     */
    STDMETHOD_(CallbackHandle,RelativeEnter)	(THIS_
						IHXCallback* pCallback,
						UINT32 ms);

    /************************************************************************
     *	Method:
     *	    IHXOptimizedScheduler::AbsoluteEnter
     *	Purpose:
     *	    enter objects in the service queue at absolute time
     */
    STDMETHOD_(CallbackHandle,AbsoluteEnter)	(THIS_
						IHXCallback* pCallback,
						HXTimeval tVal);

    /************************************************************************
     *	Method:
     *	    IHXOptimizedScheduler::Remove
     *	Purpose:
     *	    remove objects from the service queue
     */
    STDMETHOD(Remove)		(THIS_
			    	CallbackHandle Handle);

    /************************************************************************
     *	Method:
     *	    IHXOptimizedScheduler::GetCurrentSchedulerTime
     *	Purpose:
     *	    gives the current time in the timeline of the scheduler...
     */
    STDMETHOD_(HXTimeval,GetCurrentSchedulerTime)	(THIS);

    STDMETHOD(StartScheduler)	(THIS);
    STDMETHOD(StopScheduler)	(THIS);

    HX_RESULT	ExecuteCurrentFunctions();
    HXBOOL	GetNextEventDueTime(UINT32& ulNumMs);

protected:
    LONG32	    m_lRefCount;
    ClientPQ*       m_pPQ;
    CHXID*          m_pID;
    IHXMutex*       m_pPQMutex;
    IUnknown*	    m_pContext;
    IHXScheduler*  m_pScheduler;

    HXTimeval	    m_CurrentTimeVal;
    UINT32	    m_ulLastUpdateTime;
    UINT32	    m_ulLastSyncTime;
    IHXMutex*	    m_pMutex;

    /* Used to schedule events in a separate thread */
    friend void* ThreadRoutine (void * pArg);
    inline void	UpdateCurrentTime(Timeval* pNow);

    IHXThread*	    m_pThread;
    IHXEvent*	    m_pQuitEvent;
    IHXEvent*	    m_pSleepEvent;

    ULONG32	    m_ulCurrentGranularity;
    HXBOOL	    m_bIsDone;
};

#endif // _IHXOptimizedScheduler_
