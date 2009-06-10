/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: memreap_proc.cpp,v 1.4 2004/07/16 06:07:07 tmarshall Exp $
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

/////////////////////////////////////////////////////////////////////////////
//
//
// Module:      server
// File:        memreap_proc.cpp
//
// Description:
//      Memory-reaping proc and callback routines.
//
//      Objects:
//              MemReaperProcessInitCallback    callback for process launch
//              MemReaperCallback               callback for memory reaping,
//                                               listener for config changes
//
// Revisions:
//      jmevissen       1/11/2001       Initial version
//
/////////////////////////////////////////////////////////////////////////////

#include "hxtypes.h"
#include "hxcom.h"
#include "hxerror.h"

#include "memreap_proc.h"
#include "core_proc.h"
#include "proc_container.h"
#include "server_engine.h"
#include "server_context.h"
#include "dispatchq.h"

#include "errmsg_macros.h"
#include "loadinfo.h"
#include "shmem.h"

extern int* VOLATILE pMemreapProcInitMutex;


////////////////////////////////////////////////////////////////////////
//                      MemReaperProcessInitCallback::func
////////////////////////////////////////////////////////////////////////
//
// Description:         Callback that launches the mem-reaper proc.
//
// Parameters:
//
// Returns:
//
// Implementation:
//              New proc is launched from CoreTransferCallback() in
//              _main.cpp.
//
// Revisions:
//      jmevissen       1/11/2001       Initial version
//
////////////////////////////////////////////////////////////////////////
void
MemReaperProcessInitCallback::func(Process* proc)
{
    proc->pc = new ProcessContainer(proc, m_proc->pc);
    proc->pc->dispatchq->init(proc);
    proc->pc->process_type = PTMemory;


    // Init the callback object for memory reaping

    MemReaperCallback* cb = new MemReaperCallback(proc);

    // release startup mutex (protects non-reentrant calls during process creation)

    *pMemreapProcInitMutex = 0;

    // start the loop

    proc->pc->engine->mainloop();
}

////////////////////////////////////////////////////////////////////////
//                      MemReaperCallback::MemReaperCallback
////////////////////////////////////////////////////////////////////////
//
// Description:         The callback object that manages memory-reclaim
//                      operations.  Runs in its own "memreap" thread.
//
// Parameters:
//
// Returns:
//
// Implementation:
//      The config params are read at startup.  If there are not both
//      present, we disable watching for on-the-fly changes.
//
// Revisions:
//      jmevissen       1/11/2001       Initial version
//
////////////////////////////////////////////////////////////////////////
MemReaperCallback::MemReaperCallback(Process* proc)
    : m_lRefCount(0),
      m_pProc(proc),
      m_pScheduler(0),
      m_pRegistry(0),
      m_pPropWatch(0),
      m_iNextBucket(0),
      m_ulNumReclaimed(0),
      m_uiHighLoadSkips(0),
      m_iInterval(0),
      m_uiIntervalPropID(0),
      m_iAge(0),
      m_uiAgePropID(0),
      m_uCallbackHandle(0)
{
    m_timeLastRun.tv_sec = 0;

    this->AddRef();

    HX_RESULT hr;
    IUnknown* pContext = (IUnknown*)proc->pc->server_context;

    hr = pContext->QueryInterface(IID_IHXScheduler,
                                  (void**)&m_pScheduler);

    if (hr == HXR_OK)
    {
        hr = pContext->QueryInterface(IID_IHXRegistry,
                                      (void**)&m_pRegistry);
    }

    // Get the vars from the config file, if present.
    // Else, init them.

    // Get the run interval

    if (hr == HXR_OK)
    {
        m_uiIntervalPropID = m_pRegistry->GetId(CONFIG_INTERVAL_NAME);
        if (m_uiIntervalPropID)
        {
            m_pRegistry->GetIntById(m_uiIntervalPropID, m_iInterval);
        }
        else
        {
            m_iInterval = CONFIG_INTERVAL_DEFAULT;
        }

    // Get the age threshold

        m_uiAgePropID = m_pRegistry->GetId(CONFIG_AGE_THRESHOLD_NAME);
        if (m_uiAgePropID)
        {
            m_pRegistry->GetIntById(m_uiAgePropID, m_iAge);
        }
        else
        {
            m_iAge = CONFIG_AGE_THRESHOLD_DEFAULT;
        }
    }

    // Set a watch on the vars, so we can update on the fly.

    if (hr == HXR_OK)
    {
        hr = m_pRegistry->CreatePropWatch(m_pPropWatch);
    }
    if (hr == HXR_OK)
    {
        hr = m_pPropWatch->Init((IHXPropWatchResponse*)this);
    }
    if (hr == HXR_OK)
    {
        m_uiIntervalPropID = m_pPropWatch->SetWatchById(m_uiIntervalPropID);
        m_uiAgePropID = m_pPropWatch->SetWatchById(m_uiAgePropID);
    }

    if (hr == HXR_OK)
    {
        // Everything init'd okay.  Start the memory-reclaim callback.

#ifdef DEBUG
        LOGMSG(proc->pc->error_handler,
               HXLOG_INFO,
               "memreap callback initialized with interval %d s",
               m_iInterval);
#endif
        if (m_iInterval > 0)
        {
            m_uCallbackHandle = m_pScheduler->RelativeEnter(this, m_iInterval*1000);
        }

        if (!m_uiIntervalPropID ||
            !m_uiAgePropID)
        {
#ifdef DEBUG
            LOGMSG(proc->pc->error_handler,
                   HXLOG_INFO,
                   "memreap: missing config values, on-the-fly changes disabled");
#endif
            m_uiIntervalPropID = m_uiAgePropID = 0;
        }
    }
    else
    {
        LOGMSG(proc->pc->error_handler,
               HXLOG_ERR,
               "Unable to initialize memreap callback for memory-reaping operations");

        // This will ensure that the callback is not re-scheduled through an
        // interval property change (via MemReaperCallback::ModifiedProp() ).
        // (Obsolete comment, but ensure the params are zeroed anyway.)

        m_uiIntervalPropID = m_uiAgePropID = 0;
    }
}

////////////////////////////////////////////////////////////////////////
//                      MemReaperCallback::~MemReaperCallback
////////////////////////////////////////////////////////////////////////
//
// Description:         Destructor.  Should never be invoked.
//
// Parameters:
//
// Returns:
//
// Implementation:
//              The object persists for the life of the server; is always
//              listening for changes to the config param "MemoryReclaimInterval."
//
// Revisions:
//      jmevissen       1/11/2001       Initial version
//
////////////////////////////////////////////////////////////////////////
MemReaperCallback::~MemReaperCallback()
{
    DPRINTF(D_ERROR, ("MemReaperCallback destructed, shouldn't happen.\n"));

    if (m_pScheduler)
    {
        m_pScheduler->Release();
        m_pScheduler = 0;
    }
    if (m_pRegistry)
    {
        m_pRegistry->Release();
        m_pRegistry = 0;
    }
    if (m_pPropWatch)
    {
        m_pPropWatch->ClearWatchById(m_uiAgePropID);
        m_pPropWatch->Release();
        m_pPropWatch = 0;
    }
}

////////////////////////////////////////////////////////////////////////
//                      MemReaperCallback::Func
////////////////////////////////////////////////////////////////////////
//
// Description:         Callback that invokes SharedMemory:: routine(s)
//                      to reap idle memory buckets.
//
// Parameters:
//
// Returns:
//
// Implementation:
//      - call ReclaimByAge() to actually do the work of reclaiming.
//      - re-queue ourselves to run after the reclaim interval.
//
//      Tweaks:
//      If load is not normal, requeue ourselves up to three times
//      (NUM_HIGH_LOAD_SKIPS) before going ahead and running.  Requeue
//      interval is 1 hour if reclaim interval is more than an hour,
//      else the lesser of 5 minutes and the reclaim interval.  (This
//      could be a little more sophisticated; see the
//      SharedMemoryGarbageCollector SOD.)
//
//      Divide up the load to allow other procs to run if needed.
//
//
// Revisions:
//      jmevissen       1/8/2001        Initial version
//
////////////////////////////////////////////////////////////////////////
STDMETHODIMP
MemReaperCallback::Func()
{
    // Are we in the middle of a reclaim?

    if (m_iNextBucket)
    {
        return Func2();
    }

    HX_RESULT   hr = HXR_OK;

    // Check the load.  Initiate a reclaim if the load is normal, or if
    // we've already skipped 3 times.

    if (m_pProc->pc->loadinfo->GetLoadState() != NormalLoad &&
        m_uiHighLoadSkips < NUM_HIGH_LOAD_SKIPS)
    {
#ifdef DEBUG
        LOGMSG(m_pProc->pc->error_handler,
               HXLOG_INFO,
               "Defering memory-reclaim due to above-average load (memreap)");
#endif
        ++m_uiHighLoadSkips;
        if (m_iInterval > 3600)
        {
            m_uCallbackHandle =
                m_pScheduler->RelativeEnter(this, 3600*1000);
        }
        else if (m_iInterval > 300)
        {
            m_uCallbackHandle =
                m_pScheduler->RelativeEnter(this, 300*1000);
        }
        else
        {
            m_uCallbackHandle =
                m_pScheduler->RelativeEnter(this, m_iInterval*1000);
        }
        return HXR_OK;
   }

    // Load is okay (or we shouldn't skip anymore); do the reclaim
    // after refreshing the age config param

    m_uiHighLoadSkips = 0;

//    if (m_uiAgePropID &&
//      m_pRegistry->GetIntById(m_uiAgePropID, m_iAge) == HXR_OK)
//    {
        if (m_iAge >= 0)
        {
#ifdef DEBUG
            LOGMSG(m_pProc->pc->error_handler,
                   HXLOG_INFO,
                   "Reclaiming unused memory aged %d s (memreap)",
                   m_iAge);
#endif
            m_ulNumReclaimed = 0;
            hr = Func2();       // Starts the reclaim slices
        }
        else
        {
            LOGMSG(m_pProc->pc->error_handler,
                   HXLOG_INFO,
                   "Memory-reclaim disabled, age param is %d (memreap)",
                   m_iAge);
        }
//    }

    return hr;
}

////////////////////////////////////////////////////////////////////////
//                      MemReaperCallback::Func2
////////////////////////////////////////////////////////////////////////
//
// Description:         Callback for dividing up reclaim work; reclaim
//                      just one bucket before yielding.
//
// Parameters:
//
// Returns:
//
// Implementation:
//              You can switch to ReclaimByAge(m_iAge) if you don't
//              want to divide up the work.  Just be sure the
//              need-to-run-again test fails.
//
// Revisions:
//      jmevissen       1/12/2001       Initial version
//
////////////////////////////////////////////////////////////////////////
STDMETHODIMP
MemReaperCallback::Func2()
{
    // reclaim the next bucket

    m_ulNumReclaimed +=
        SharedMemory::ReclaimByAge(m_iAge, m_iNextBucket);

    // need to run again?

    if (++m_iNextBucket < LAST_BUCKET)
    {
        m_uCallbackHandle =
            m_pScheduler->RelativeEnter(this, 0);
    }
    else
    {
#ifdef DEBUG
        LOGMSG(m_pProc->pc->error_handler,
               HXLOG_INFO,
               "%d pages reclaimed (memreap)",
               m_ulNumReclaimed);
#endif
        m_ulNumReclaimed = 0;
        m_iNextBucket = 0;

        // set time that we ran:

        m_timeLastRun = m_pScheduler->GetCurrentSchedulerTime();

        // run again at the specified interval.
        // interval == 0  => turn off reclaim callback.

        if (m_iInterval > 0)
        {
            m_uCallbackHandle =
                m_pScheduler->RelativeEnter(this, m_iInterval*1000);
        }
        else
        {
            m_uCallbackHandle = 0;
        }
    }
    return HXR_OK;
}


////////////////////////////////////////////////////////////////////////
//                      MemReaperCallback::ModifiedProp
////////////////////////////////////////////////////////////////////////
//
// Description:         Callback for watched-property modified event.
//                      We're watching the config var for the reclaim
//                      interval and age threshold.
//
// Parameters:
//
// Returns:
//
// Implementation:
//      If age threshold changed, simply update the member var.  There's
//      no problem with changing the threshold in the middle of a reclaim.
//
//      If the interval is changed, reschedule the callback to account for
//      the new interval.  (That is, count time we've already waited toward
//      the new interval value.)  Cancel old callback.  Schedule from
//      now if the reclaim feature was previously off (interval = 0).
//
//      Ignore rescheduling if we're in the middle of a reclaim; the next
//      callback will be scheduled at the new interval when we're done.
//
//
// Revisions:
//      jmevissen       1/8/2001        Initial version
//
////////////////////////////////////////////////////////////////////////
STDMETHODIMP
MemReaperCallback::ModifiedProp
(
    const UINT32      id,
    const HXPropType propType,
    const UINT32      ulParentID
)
{
    if (!m_pRegistry) return HXR_OK;

    // If the age threshold changes, simply update the var.

    if (id == m_uiAgePropID)
    {
        m_pRegistry->GetIntById(id, m_iAge);
        LOGMSG(m_pProc->pc->error_handler,
               HXLOG_INFO,
               "Admin changed memory-reclaim age threshold to %d s (memreap)",
               m_iAge);
    }

    // If the interval changes, update var and reschedule.

    if (id == m_uiIntervalPropID)
    {
        if (m_pRegistry->GetIntById(id, m_iInterval) == HXR_OK)
        {
            LOGMSG(m_pProc->pc->error_handler,
                   HXLOG_INFO,
                   "Admin changed memory-reclaim interval to %d s (memreap)",
                   m_iInterval);
        }

        // Reschedule callback if not in the middle of a reclaim.
        // Else, the new interval will get picked up when the reclaim is finished.

        if (!m_iNextBucket)
        {
            // re-queue callback as though originally set with new interval
            // (or now, if callback should've happened already)

            if (m_uCallbackHandle)
            {
                m_pScheduler->Remove(m_uCallbackHandle);
            }

            // only schedule the callback if the interval is nonzero and positive.

            if (m_iInterval > 0)
            {
                HXTimeval now = m_pScheduler->GetCurrentSchedulerTime();
                tv_sec_t diff = now.tv_sec - m_timeLastRun.tv_sec;

                // if there wasn't an old callback scheduled, set diff to zero
                // (that is, schedule next callback from right now)

                if (!m_uCallbackHandle) diff = 0;

                if (diff > m_iInterval)
                {
                    m_uCallbackHandle =
                        m_pScheduler->RelativeEnter(this, 0);
                }
                else
                {
                    m_uCallbackHandle =
                        m_pScheduler->RelativeEnter(this, (m_iInterval-diff) * 1000);
                }
            }
            else
            {
                m_uCallbackHandle = 0;
            }
        }
    }
    return HXR_OK;
}


////////////////////////////////////////////////////////////////////////
//                      MemReaperCallback::AddedProp
//                      MemReaperCallback::DeletedProp
////////////////////////////////////////////////////////////////////////
//
// Description:         Do-nothing minimal implementations for PropWatch.
//
// Revisions:
//      jmevissen       1/8/2001        Initial version
//
////////////////////////////////////////////////////////////////////////
STDMETHODIMP
MemReaperCallback::AddedProp
(
    const UINT32      id,
    const HXPropType propType,
    const UINT32      ulParentID
)
{
    return HXR_OK;
}

STDMETHODIMP
MemReaperCallback::DeletedProp(const UINT32 id,
                               const UINT32 ulParentID)
{
    return HXR_OK;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//      IUnknown::QueryInterface
//  Purpose:
//      Implement this to export the interfaces supported by your
//      object.
//
STDMETHODIMP
MemReaperCallback::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)(IHXCallback*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXCallback))
    {
        AddRef();
        *ppvObj = (IHXCallback*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXPropWatchResponse))
    {
        AddRef();
        *ppvObj = (IHXPropWatchResponse*)this;
        return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//      IUnknown::AddRef
//  Purpose:
//      Everyone usually implements this the same... feel free to use
//      this implementation.
//
STDMETHODIMP_(ULONG32)
MemReaperCallback::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//      IUnknown::Release
//  Purpose:
//      Everyone usually implements this the same... feel free to use
//      this implementation.
//
STDMETHODIMP_(ULONG32)
MemReaperCallback::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}
