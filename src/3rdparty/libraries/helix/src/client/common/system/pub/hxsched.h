/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxsched.h,v 1.14 2006/09/12 06:20:47 milko Exp $
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
 * terms of the GNU General Public License Version 2 or later (the
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

#include "hxthread.h"

#ifndef _IHXSCHEDULER_
#define _IHXSCHEDULER_

#include "hxengin.h"

class ClientPQ;
class Timeline;
class HXMutex;
struct IUnknown;
class CHXID;

typedef struct _HXTimeval HXTimeval;

#include "timeval.h"
#include "hxmovavg.h"

class CAsyncTimer;

class HXScheduler : public IHXScheduler
		  , public IHXScheduler2
{
protected:
    inline CallbackHandle _AbsoluteEnter(IHXCallback* pCallback, Timeval tVal);
    inline UINT32 _Execute(ClientPQ* pScheduler, HXBOOL &bImmediatesPending);
    inline UINT32 _GetNextDueTimeInMs(ClientPQ* pScheduler);

    LONG32	m_lRefCount;
    ClientPQ*	m_pScheduler;
    HXBOOL      m_bUseDeferredTask;

    // Is set to TRUE for most of the time except when we need to remove all
    // callbacks from the scheduler without actually exectuing the functions..
    static HXBOOL	m_sbProcess;
    
    /* special PQ for interrupt safe tasks */
    ClientPQ*   m_pInterruptTimeScheduler;	/* services at either interrupt or non-interrupt time */
    ClientPQ*   m_pInterruptTimeOnlyScheduler;	/* serevics only at interrupt time */
    CHXID*      m_pID;

    IUnknown*   m_pContext;

    /* semaphore to prevent a deferred interrupt task from interrupting
     * a system task
     */
    HXBOOL		m_bLocked; 

public:
			HXScheduler(IUnknown* pContext);
			~HXScheduler();

    /*
     * IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)	(THIS);

    STDMETHOD_(ULONG32,Release)	(THIS);

    /*
     * HXScheduler methods
     */
    /************************************************************************
     *	Method:
     *	    IHXScheduler::RelativeEnter
     *	Purpose:
     *	    enter objects in the service queue offset from time now
     */
    STDMETHOD_(CallbackHandle,RelativeEnter)	(THIS_
						IHXCallback* pCallback,
						UINT32 ms);

    /************************************************************************
     *	Method:
     *	    IHXScheduler::AbsoluteEnter
     *	Purpose:
     *	    enter objects in the service queue at absolute time
     */
    STDMETHOD_(CallbackHandle,AbsoluteEnter)	(THIS_
						IHXCallback* pCallback,
						HXTimeval tVal);

    /************************************************************************
     *	Method:
     *	    IHXScheduler::Remove
     *	Purpose:
     *	    remove objects from the service queue
     */
    STDMETHOD(Remove)		(THIS_
			    	CallbackHandle Handle);

    /************************************************************************
     *	Method:
     *	    IHXScheduler::GetCurrentSchedulerTime
     *	Purpose:
     *	    gives the current time in the timeline of the scheduler...
     */
    STDMETHOD_(HXTimeval,GetCurrentSchedulerTime)	(THIS);

    /*
     *	IHXScheduler2 methods
     */
    STDMETHOD(StartScheduler)		    (THIS);
    STDMETHOD(StopScheduler)		    (THIS);
    /************************************************************************
     *	Method:
     *		OnTimeSync
     *	Purpose:
     *		called by audio services OR the player timer to set the 
     *		current time of playback..
     *
     */
    STDMETHOD(OnTimeSync)		    (THIS_
					    HXBOOL bAtInterrupt);
    STDMETHOD(SetInterrupt)		    (THIS_
					    HXBOOL bInterruptenable);
    STDMETHOD_(HXBOOL, IsAtInterruptTime)   (THIS);
    STDMETHOD(SetMutex)			    (THIS_
					    IHXMutex* pMutex);
    STDMETHOD(GetMutex)			    (THIS_
					    REF(IHXMutex*) pMutex);
    STDMETHOD(NotifyPlayState)		    (THIS_
					    HXBOOL bInPlayingState);
    STDMETHOD_(HXBOOL, GetNextEventDueTimeDiff)(THIS_
					       ULONG32 &ulEarliestDueTimeDiff);
    STDMETHOD(WaitForNextEvent)		    (THIS_
					    ULONG32 ulTimeout);
    STDMETHOD_(ULONG32, GetThreadID)	    (THIS);
    STDMETHOD_(HXBOOL, AreImmediatesPending)(THIS);

    HX_RESULT	ExecuteCurrentFunctions(HXBOOL bAtInterrupt=FALSE);

    void	UseDeferredTask(HXBOOL bUseDeferredTask) 
		    {m_bUseDeferredTask = bUseDeferredTask;};
    HX_RESULT	StartSchedulerTimerFixup();
    HX_RESULT   StartSchedulerImplementation(HXBOOL TimerFixup);
    
    HXBOOL	IsEmpty();

    UINT32      GetGranularity();
    
protected:
    UINT32	    m_ulThreadID;
    Timeval	    m_CurrentTimeVal;
    UINT32	    m_ulLastUpdateTime;
    IHXMutex*	    m_pCoreMutex;
    HXBOOL	    m_bIsInterruptEnabled;
    Timeval 	    m_headTime;
    Timeval	    m_interruptHeadTime;
    Timeval	    m_interruptOnlyHeadTime;
    UINT32	    m_ulSystemNextDueTime;
    UINT32	    m_ulInterruptNextDueTime;
    UINT32	    m_ulInterruptOnlyNextDueTime;
    IHXEvent*	    m_pWaitEvent;
    HXBOOL	    m_bWaitPending;
    HXBOOL	    m_bWaitedEventFired;

    HXBOOL	    m_bImmediatesPending;

#if defined(_WIN32) || defined(THREADS_SUPPORTED)
    CAsyncTimer*    m_pAsyncTimer;
#endif 

    Timeline*		m_pTimeline;
    ULONG32		m_ulCurrentGranularity;
    inline HXBOOL	UpdateCurrentTime(HXBOOL bAtInterrupt,
                                          HXBOOL& bShouldServiceSystem, 
                                          HXBOOL& bShouldServiceInterrupt,
					  HXBOOL& bShouldServiceInterruptOnly);
    void GetTime(Timeval* pCurrentTimeVal);
    
};



class HXSchedulerTimer : public HXMovingAverage
{
  public:
    HXSchedulerTimer();
    ~HXSchedulerTimer();

    //Sets the desired average frequency you want calls to Kick() or
    //OnTimeSync to be. In MilliSeconds.
    void Init(UINT32 ulFreqInMS);
    
    //Samples are the timestamps just before and just after a call to
    //OnTimeSync or Kick(). It will return the suggested sleep time
    //in *MICRO*seconds.
    UINT32 AddSample(UINT32 before, UINT32 after);

    //Returns number of *MICRO*seconds to sleep.
    UINT32 GetSleepTime() const;

  private:

    HXMovingAverage movAverage;
    UINT32          m_ulLastCallTime;
    UINT32          m_ulTargetTime;
    UINT32          m_ulSleepTime;

    //The size of our moving average table.
    static const int zm_nMovingAveSize = 5;

    //XXXgfw Below is an alternate method for computing sleep time
    //from the samples. It is being left in place so that we can test
    //with it after several other changes get checked in.
    //INT32 m_ulPrevTS1;
    //INT32 m_lTimeDeficit;
    
    
};


#endif // _IHXSCHEDULER_
