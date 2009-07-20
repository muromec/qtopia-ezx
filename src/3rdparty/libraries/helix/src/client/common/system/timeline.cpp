/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: timeline.cpp,v 1.21 2008/02/19 10:22:58 vkathuria Exp $
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

#include "hxcom.h"
#include "hlxclib/stdio.h"
#include "hxresult.h"
#include "hxassert.h"
#include "hxtypes.h"
#include "ihxpckts.h"
#include "pckunpck.h"
#include "hxengin.h"
#include "hxfiles.h"
#include "hxcore.h"
#include "hxprefs.h"
#include "hxausvc.h"
#include "timeval.h"
#include "hxtick.h"
#include "hxsched.h"
#include "hxmap.h"
#include "timeline.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif

//#define LOG_MULTIPLE_DEFERRED_TASKS 1

// static initializations..
#if !defined(HELIX_CONFIG_NOSTATICS)

#if defined(_WINDOWS) || defined(_SYMBIAN)
CHXMapLongToObj Timeline::m_zTimerMap;
#elif defined(_UNIX) || defined (_MACINTOSH) || defined(__TCS__) || defined(_OPENWAVE)
CHXMapPtrToPtr Timeline::m_zTimerMap;
#endif

#else /* #if !defined(HELIX_CONFIG_NOSTATICS) */

#include "globals/hxglobals.h"

#if defined(_WINDOWS) || defined(_SYMBIAN) || defined(_BREW)
typedef CHXMapLongToObj      TimelineMapType;
typedef HXGlobalMapLongToObj GlobalTimelineMapType;
#elif defined(_UNIX) || defined (_MACINTOSH)
typedef CHXMapPtrToPtr      TimelineMapType;
typedef HXGlobalMapPtrToPtr GlobalTimelineMapType;
#endif

const TimelineMapType* const Timeline::m_zTimerMap = NULL;

#endif /* #if !defined(HELIX_CONFIG_NOSTATICS) */



#if defined(_WINDOWS) && !defined(_WIN32) /* WIN16 */
#define MINIMUM_GRANULARITY 55
#elif defined(_MACINTOSH)
#define MINIMUM_GRANULARITY 20
#elif defined(_UNIX) 
#define MINIMUM_GRANULARITY 20
#elif defined(_BREW) 
#define MINIMUM_GRANULARITY 30
extern const IShell*  g_pIShell;
#else
#define MINIMUM_GRANULARITY 20
#endif

#ifdef _MACINTOSH
#include "hxmm.h" //for checking if VM

ULONG32	   gTIMELINE_MUTEX=NULL;

#define CLOCKSET(c) (c.qType & kTMTaskActive)

#endif

#ifdef _MACINTOSH
#ifdef _CARBON

typedef pascal Handle (*NewHandleSysProcPtr)(Size);
CFragConnectionID gInterfaceLibConnID = kInvalidID;
NewHandleSysProcPtr gNewHandleSysProc = nil;
bool gTriedToInitialize = false;

void InitInterfaceLibProcPtrs()
{
	if (gTriedToInitialize) return;
	gTriedToInitialize = true;

	if (gInterfaceLibConnID == kInvalidID)
	{
		GetSharedLibrary("\pInterfaceLib", kCompiledCFragArch, kReferenceCFrag,
			&gInterfaceLibConnID, nil, nil);
	}
	
	if (gInterfaceLibConnID != kInvalidID)
	{
		OSErr err = noErr;
		
		err = FindSymbol(gInterfaceLibConnID, "\pNewHandleSys", (Ptr*)&gNewHandleSysProc, nil);
	}
}

#endif
#endif

/*
 * This class was originally written to keep track of time and report the
 * *current* time to the user. This was the reason that pause, seek and 
 * stuff were implemented. For this it needs to know the user (HXPlayer). 
 * If we wanna use it from other object, the only way would be to pass it 
 * a generic callback. But since IHXCallback's func does not take any 
 * parameter, we cannot pass the *current* time back.
 * 
 * Since this class will eventually go away, I have made it to work so that
 * it can be used by HXPlayer and HXScheduler by passing IUnknown.
 *
 */
Timeline::Timeline(void) :
#if defined(_WINDOWS) || defined(_SYMBIAN) || defined(_BREW)
      m_uiTimerID (0)
    , m_uiTimerIDFixup (0)
    , m_ulLastCallbackTime (0) 
    , m_pAsyncTimer(NULL),
#endif
     m_ulGranularity (MINIMUM_GRANULARITY)
    , m_bIsTimerPending (FALSE)
    , m_bPaused (FALSE)
    , m_bTimerFixup (FALSE)
    , m_pPlayer(NULL)
    , m_pScheduler(NULL)
    , m_pScheduler2(NULL)
    , m_pContext(NULL)
#ifdef _MACINTOSH
    , m_uppTask(NULL)
    , m_bUseDeferredTasks(TRUE)
    , m_pInterruptState(NULL)
    , m_bAnyCallbackPending(0)
    , m_bIsQuitting (FALSE)
#ifdef THREADS_SUPPORTED
    , m_pCoreMutex(NULL)
#endif
#endif
{
#ifdef _MACINTOSH
    memset( &m_DeferredTaskStruct, 0, sizeof(m_DeferredTaskStruct) );
    memset( &m_tmInfo, 0, sizeof(m_tmInfo) );
#endif
};

Timeline::~Timeline()
{
    if (m_bIsTimerPending)
    {
	Pause();
    }

    m_bIsTimerPending	= FALSE;

    Done();
    
#ifdef _MACINTOSH
    if (m_uppTask) 
    {
#ifdef _CARBON
	DisposeTimerUPP(m_uppTask);
#else
	DisposeRoutineDescriptor(m_uppTask);
#endif
	m_uppTask = NULL;
    }

    if (m_DeferredTaskStruct.dtAddr != NULL)
    {
	// first ensure that the Gestalt handle doesn't think it's
	// here any more.
	
	Handle dtGestaltHandle = nil;
	Gestalt( kLetInterruptsFinishBeforeQuittingGestalt, (long*)&dtGestaltHandle );
	if ( dtGestaltHandle )
	{
	    // XXXNH: only look at tasks for this process
	    ProcessSerialNumber psn;
	    GetCurrentProcess(&psn);
	    // zip through and if an entry equals this deferred task,
	    // simply zero it out. We won't worry about shuffling
	    // the handle size at this juncture.
	    long hSize = GetHandleSize( dtGestaltHandle );
	    GestaltDeferredStruct* currentPtr = (GestaltDeferredStruct*)*dtGestaltHandle;
	    for ( int i = 0; i < hSize / sizeof(GestaltDeferredStruct); i++ )
	    {
	        unsigned char bSameProcess = FALSE;
	        SameProcess(&(currentPtr[i].psn), &psn, &bSameProcess);
	        
		if (bSameProcess && 
		    currentPtr[i].quitting == &m_bIsQuitting && 
		    currentPtr[i].pending == &m_bAnyCallbackPending )
		{
		    currentPtr[i].quitting = NULL;
		    currentPtr[i].pending = NULL;
		}
	    }    
	}

	
	
	
	
	
#ifdef _CARBON
	DisposeDeferredTaskUPP(m_DeferredTaskStruct.dtAddr);
#else
	DisposeRoutineDescriptor(m_DeferredTaskStruct.dtAddr);
#endif
	m_DeferredTaskStruct.dtAddr = NULL;
    }

    HX_RELEASE(m_pInterruptState);
#ifdef THREADS_SUPPORTED
    HX_RELEASE(m_pCoreMutex);
#endif
#endif

#if defined(_WINDOWS) || defined(_SYMBIAN) || defined(_BREW)
    HX_RELEASE(m_pAsyncTimer);
#endif

    HX_RELEASE(m_pContext);
}

HX_RESULT
Timeline::Init(IUnknown* pContext, 
	       IHXScheduler* pScheduler, 
	       HXBOOL bUseDeferredTask)
{
    if (!pContext)
    {
	return HXR_INVALID_PARAMETER;
    }

    HX_ASSERT(pContext);

    HX_RELEASE(m_pContext);
    HX_RELEASE(m_pScheduler2);
    HX_RELEASE(m_pScheduler);

    m_pContext = pContext;
    HX_ADDREF(m_pContext);

    m_pScheduler = pScheduler;
    HX_ADDREF(m_pScheduler);

#if defined(_WINDOWS) || defined(_SYMBIAN) || defined(_BREW)
    CreateInstanceCCF(CLSID_IHXAsyncTimer, (void**)&m_pAsyncTimer, m_pContext);
#endif

#ifdef _MACINTOSH
    m_bUseDeferredTasks = bUseDeferredTask;
    if (m_bUseDeferredTasks)
    {
#ifdef _CARBON
	m_uppTask = NewTimerUPP((TimerProcPtr)MacTimerProc);
#else
	m_uppTask = NewTimerProc(MacTimerProc);
#endif

	m_tmInfo.tmTask.tmAddr = m_uppTask;
	m_tmInfo.tmRefCon = (long) this;
	m_tmInfo.tmTask.tmWakeUp = 0L;
	m_tmInfo.tmTask.tmReserved = 0L;

	m_DeferredTaskStruct.dtReserved = 0;
	m_DeferredTaskStruct.dtFlags = 0;
#ifdef _CARBON
	m_DeferredTaskStruct.dtAddr = NewDeferredTaskUPP(Timeline::DeferredTaskProc);
#else
	m_DeferredTaskStruct.dtAddr = NewDeferredTaskProc(Timeline::DeferredTaskProc);
#endif
	m_DeferredTaskStruct.dtParam = (long) this;
	m_DeferredTaskStruct.qType = dtQType;
	
	// Gestalt-based code that remembers potential deferred tasks for emergency removal
	
	Handle dtGestaltHandle = nil;
	OSErr err = Gestalt( kLetInterruptsFinishBeforeQuittingGestalt, (long*)&dtGestaltHandle );
	if ( err != noErr )
	{
#ifdef _CARBON
	    InitInterfaceLibProcPtrs();
	    if (gNewHandleSysProc)
	    {
		dtGestaltHandle = (*gNewHandleSysProc)( 0 );
	    }
	    if (!dtGestaltHandle)
	    {
		dtGestaltHandle = NewHandle( 0 );
	    }
#else
	    dtGestaltHandle = NewHandleSys( 0 );
#endif
	    if ( dtGestaltHandle )
	    {
		NewGestaltValue( kLetInterruptsFinishBeforeQuittingGestalt, (long)dtGestaltHandle );
	    }
	}

	if ( dtGestaltHandle )
	{
	    GestaltDeferredStruct gds;
	    gds.quitting = &m_bIsQuitting;
	    gds.pending = &m_bAnyCallbackPending;
	    GetCurrentProcess(&(gds.psn));
	    PtrAndHand( &gds, dtGestaltHandle, sizeof(gds) );
	}
	
    }
#endif /* _MACINTOSH */
    
    // Just get the interface for the scheduler; audio services will provide the
    // player OnTimeSync interface.
    if (!m_pScheduler)
    {
	if( !SUCCEEDED(m_pContext->QueryInterface(IID_IHXScheduler,
                                                  (void**) &m_pScheduler)))
	{
	    return HXR_INVALID_PARAMETER;  //HX_INVALID_OBJECT;
	}
	else
	{
#if defined (_MACINTOSH)
	    HX_RELEASE(m_pInterruptState);
            
	    m_pScheduler->QueryInterface(IID_IHXInterruptState, 
					 (void**)&m_pInterruptState);
#endif /* _MACINTOSH */
        }
    }
    if (m_pScheduler)
    {
        m_pScheduler->QueryInterface(IID_IHXScheduler2, (void**)&m_pScheduler2);
    }
    
    return HXR_OK;
}



HX_RESULT
Timeline::Pause(void)
{
#if defined(HELIX_CONFIG_NOSTATICS)
    TimelineMapType& m_zTimerMap =
	GlobalTimelineMapType::Get(&Timeline::m_zTimerMap);
#endif
    
    m_bPaused = TRUE;

    if (!m_bIsTimerPending)
    {
	return HXR_OK;
    }

    m_bIsTimerPending	= FALSE;

    // Kill the timer for this timeline
#if defined(_WINDOWS) || defined(_SYMBIAN) || defined(_BREW)
    if (!m_bTimerFixup) 
    {
	// This is the "old code".  Execute it just as before the
	// timerfixup functionality was introduced.
	if(m_pAsyncTimer && m_uiTimerID)
	{
	    m_pAsyncTimer->KillTimer(m_uiTimerID);
	}
        TimerObjects* pTimerObject = 0;
	if (m_zTimerMap.Lookup((LONG32) m_uiTimerID, (void*&) pTimerObject))    
        {
	    if (pTimerObject->m_uiTimer == m_uiTimerID)	
	    {
		m_zTimerMap.RemoveKey((LONG32) m_uiTimerID);	    
		delete pTimerObject;	
	    }
	}
    }
    else
    {
        TimerObjects* pTimerObject = 0;
	HXBOOL deletedFlag = FALSE;
	if(m_uiTimerID)
	{
            if( m_pAsyncTimer )
            {
                m_pAsyncTimer->KillTimer(m_uiTimerID);
            }
            
            if( m_zTimerMap.Lookup((LONG32) m_uiTimerID, (void*&) pTimerObject) )
	    {
		m_zTimerMap.RemoveKey((LONG32) m_uiTimerID);	    
		delete pTimerObject;	
		pTimerObject = NULL;
		deletedFlag = TRUE;
	    }
	}
	if(m_uiTimerIDFixup)
	{
            if( m_pAsyncTimer )
            {
                m_pAsyncTimer->KillTimer(m_uiTimerIDFixup);
            }
	    if (m_zTimerMap.Lookup((LONG32) m_uiTimerIDFixup, (void*&) pTimerObject))
	    {
	        m_zTimerMap.RemoveKey((LONG32) m_uiTimerIDFixup);
		if (deletedFlag == FALSE)
		    delete pTimerObject;
		pTimerObject = NULL;
	    }
	}
    }

    m_uiTimerID = 0;
    m_uiTimerIDFixup = 0;

#elif defined (_UNIX) || defined(_MACINTOSH) || defined(__TCS__)

#ifdef _MACINTOSH
    if ( m_bUseDeferredTasks && CLOCKSET(m_tmInfo.tmTask) )
    {
	::RmvTime( (QElemPtr)&m_tmInfo );
	
	m_bAnyCallbackPending = 0;

    }
#endif

    Timeline* pTimeline;
    if (m_zTimerMap.Lookup(this, (void*&) pTimeline))
    {
	HX_ASSERT(pTimeline == this);
	if (pTimeline == this)
	{
	    m_zTimerMap.RemoveKey(this);
	}
    }
#endif /* _UNIX || _MACINTOSH */

    return HXR_OK;
}

void Timeline::Done(void)
{
    HX_RELEASE(m_pScheduler2);
    HX_RELEASE(m_pScheduler);
#if defined (_MACINTOSH)
#if defined (_DEBUG) && defined(LOG_MULTIPLE_DEFERRED_TASKS)
    HXBOOL bBetterWaitForCallbacks = FALSE;
    if ( m_uiDeferredTaskDepth > 0 )
    {
	char tmpStr[255]; /* Flawfinder: ignore */
	sprintf(tmpStr, "m_uiDeferredTaskDepth = %u;g", m_uiDeferredTaskDepth); /* Flawfinder: ignore */
//	DebugStr(c2pstr(tmpStr));
// don't spoil the timing by hitting MacsBug
	bBetterWaitForCallbacks = TRUE;
    }
#endif
    m_bIsQuitting = TRUE;
    // sit-n-spin awaiting interrupts to finish.
    for ( int i = 0; i < 50; i++ )
    {
	unsigned long dummyTix;
	Delay( 6, &dummyTix );
	if ( !m_bAnyCallbackPending )
	{
	    i = 50;
	}
    }
#if defined(_DEBUG) && defined(LOG_MULTIPLE_DEFERRED_TASKS)
    if ( bBetterWaitForCallbacks )
    {
	if ( m_uiDeferredTaskDepth > 0 )
	{
	    DebugStr( "\pOops! After several seconds the deferred task didn't fire off;g" );
	}
	else
	{
	    DebugStr( "\pNice... the deferred task fired off and we're good to go;g" );
	}
    }
#endif
    m_bIsQuitting = FALSE;
#endif
}

HX_RESULT Timeline::Resume(void)
{
#if defined(HELIX_CONFIG_NOSTATICS)
    TimelineMapType& m_zTimerMap =
	GlobalTimelineMapType::Get(&Timeline::m_zTimerMap);
#endif
	
    m_bPaused = FALSE;

    if (m_bIsTimerPending)
    {
	return HXR_OK;  
    }
    
    m_ulLastCallbackTime    = HX_GET_TICKCOUNT();

    // Set a timer
#if defined(_WINDOWS) || defined(_SYMBIAN) || defined(_BREW)

    if (m_pAsyncTimer)
    {
	m_uiTimerID = m_pAsyncTimer->SetTimer(m_ulGranularity,
                                              (TIMERPROC)NonMMTimerProc);
        if( m_bTimerFixup )
        {
            m_uiTimerIDFixup = m_pAsyncTimer->SetTimer(m_ulGranularity,
                                                       (TIMERPROC)NonMMTimerProc);
        }
    }
    
#elif defined(_MACINTOSH)

    if (m_bUseDeferredTasks)
    {
	OSErr	e = noErr;

	if (CLOCKSET(m_tmInfo.tmTask))
	{
	    ::RmvTime((QElemPtr) &m_tmInfo);
	    
	    m_bAnyCallbackPending = 0;
	}

	m_tmInfo.tmTask.tmAddr = m_uppTask;
	m_tmInfo.tmRefCon = (long) this;
	m_tmInfo.tmTask.tmWakeUp = 0L;
	m_tmInfo.tmTask.tmReserved = 0L;

	::InsTime((QElemPtr) &m_tmInfo);
	::PrimeTime((QElemPtr) &m_tmInfo, m_ulGranularity);	
	
	m_bAnyCallbackPending = 1;

    }
#endif

    // add this timer to the static timer map
#if defined(_WINDOWS) || defined(_SYMBIAN) || defined(_BREW)
    TimerObjects*   pTimerObject    = new TimerObjects;
    if (!pTimerObject)
    {
	if (!m_bTimerFixup)
	{
	    // This is the "old code".  Execute it just as before the
	    // timerfixup functionality was introduced.
	    if (m_pAsyncTimer)
	    {
		m_pAsyncTimer->KillTimer(m_uiTimerID);
	    }
	}
	else 
	{
            if( m_pAsyncTimer )
            {
                if( m_uiTimerID )
                {
                    m_pAsyncTimer->KillTimer(m_uiTimerID);
                }
                if( m_uiTimerIDFixup )
                {
                    m_pAsyncTimer->KillTimer(m_uiTimerIDFixup);
                }
            }
            
	}
	return HXR_OUTOFMEMORY;
    }

    pTimerObject->m_pTimeline	    = this;
    pTimerObject->m_uiTimer	    = m_uiTimerID;

    m_zTimerMap.SetAt((LONG32) m_uiTimerID, (void*) pTimerObject);
    if (m_bTimerFixup)
        m_zTimerMap.SetAt((LONG32) m_uiTimerIDFixup, (void*) pTimerObject);

#elif defined(_UNIX) || defined(_MACINTOSH) || defined(__TCS__) || defined(_OPENWAVE)
    m_zTimerMap.SetAt(this, this);
#endif

#if defined(_MACINTOSH) && defined(_DEBUG) && defined(LOG_MULTIPLE_DEFERRED_TASKS)
    if ( m_bIsTimerPending )
    {
	DebugStr( "\pTimeLine::Resume -- m_bIsTimerPending already true!;g" );
    }
#endif
    m_bIsTimerPending	= TRUE;

    return HXR_OK;
}

HX_RESULT
Timeline::Seek(ULONG32 ulSeekTime)
{
    if (!m_bPaused)
    {
	Pause();
	Resume();
    }

    return HXR_OK;
}

HX_RESULT
Timeline::SetStartTime(ULONG32 ulTime)
{
    return HXR_OK;
}

HX_RESULT
Timeline::SetGranularity(ULONG32 ulNumMillisecs)
{
    m_ulGranularity = ulNumMillisecs;
    return HXR_OK;
}

HX_RESULT
Timeline::OnTimeSync(HXBOOL bAtInterrupt)
{

#ifdef _MACINTOSH
    if ((InterlockedIncrement(&gTIMELINE_MUTEX) > 1) && bAtInterrupt)
    {
        InterlockedDecrement(&gTIMELINE_MUTEX);
	return HXR_OK;
    }
#endif

//{FILE* f1 = ::fopen("d:\\temp\\timeline.txt", "a+"); ::fprintf(f1, "%p: %lu \n", this, HX_GET_TICKCOUNT());::fclose(f1);}

    if (!m_bPaused && m_pScheduler2)
    {
	m_pScheduler2->OnTimeSync(bAtInterrupt);
    }

#ifdef _MACINTOSH
    InterlockedDecrement(&gTIMELINE_MUTEX);
#endif

    return HXR_OK;
}


/*
 *  ********* WINDOWS ONLY *****************
 */
#if defined(_WINDOWS) || defined(_SYMBIAN) || defined(_BREW)
/*
 *  This is a Windows Timer callback proc
 */
void CALLBACK Timeline::NonMMTimerProc( 
    void*   hwnd,	// handle of window for timer messages 
    UINT32  uMsg,	// WM_TIMER message
    UINT32  idTimer,	// timer identifier
    ULONG32 dwTime 	// current system time
)
{
#if defined(HELIX_CONFIG_NOSTATICS)
    TimelineMapType& m_zTimerMap =
	GlobalTimelineMapType::Get(&Timeline::m_zTimerMap);
#endif
	
    TimerObjects*   pTimerObject = NULL;

    if (m_zTimerMap.Lookup((LONG32) idTimer, (void*&) pTimerObject))
    {
        // MUST call this BEFORE OnTimeSync, because OnTimeSync could
        // delete the object
	KillTimerFixup(idTimer, pTimerObject);
        HX_ASSERT( pTimerObject);
        HX_ASSERT( pTimerObject->m_pTimeline );
	pTimerObject->m_pTimeline->OnTimeSync();
    }
    return;
}

void Timeline::KillTimerFixup(UINT  idTimer, struct TimerObjects*  pTimerObject)
{
#if defined(HELIX_CONFIG_NOSTATICS)
    TimelineMapType& m_zTimerMap =
	GlobalTimelineMapType::Get(&Timeline::m_zTimerMap);
#endif

    if (pTimerObject->m_pTimeline->m_bTimerFixup == FALSE)
	return;

    if (pTimerObject->m_pTimeline->m_uiTimerIDFixup == 0)
	return;

    if (pTimerObject->m_pTimeline->m_uiTimerID == 0)
	return;

    UINT32 ulTimerID      = pTimerObject->m_pTimeline->m_uiTimerID;
    UINT32 ulTimerIDFixup = pTimerObject->m_pTimeline->m_uiTimerIDFixup;
    IHXAsyncTimer* pTimer = pTimerObject->m_pTimeline->m_pAsyncTimer;

    if(ulTimerID == idTimer)
    {
	if(pTimer)
	{
	    pTimer->KillTimer(ulTimerIDFixup);
	}
	m_zTimerMap.RemoveKey( ulTimerIDFixup);
	pTimerObject->m_pTimeline->m_uiTimerIDFixup = 0;
    }
    else if(ulTimerIDFixup == idTimer)
    {
	if(pTimer)
	{
	    pTimer->KillTimer(ulTimerID);
	}
	m_zTimerMap.RemoveKey(ulTimerID);
	pTimerObject->m_pTimeline->m_uiTimerID = 0;
    }

    return;
}
#endif

/*
 *  ********* MAC ONLY *****************
 */
#ifdef _MACINTOSH
/*
 *  This is the Mac Timer callback proc (deferred task)
 */
pascal void 
Timeline::MacTimerProc(TMInfoPtr task)
{
    HXMM_INTERRUPTON();

    /*
     *	Setup and then install a deferred task 
     */
    if (task != NULL && task->tmRefCon != NULL)
    {
	Timeline* pTheTimeline = (Timeline*)task->tmRefCon;
#if defined(_DEBUG) && defined(LOG_MULTIPLE_DEFERRED_TASKS)
	// m_bIsTimerPending better be true right now
	if ( !pTheTimeline->m_bIsTimerPending )
	{
	    DebugStr( "\pMacTimerProc -- m_bIsTimerPending is FALSE!;g" );
	}
#endif
	if (pTheTimeline->m_DeferredTaskStruct.dtAddr != NULL)
	{
#if defined (_DEBUG) && defined(LOG_MULTIPLE_DEFERRED_TASKS)
	    if ( pTheTimeline->m_uiDeferredTaskDepth > 0 )
	    {
		char tmpStr[255]; /* Flawfinder: ignore */
		sprintf(tmpStr, "m_uiDeferredTaskDepth = %u;g", pTheTimeline->m_uiDeferredTaskDepth); /* Flawfinder: ignore */
		DebugStr(c2pstr(tmpStr));
	    }
#endif
	    if ( pTheTimeline->m_bIsQuitting )
	    {
		// short-circuit and don't install deferred task
		pTheTimeline->m_bAnyCallbackPending = 0;
#if defined (_DEBUG) && defined(LOG_MULTIPLE_DEFERRED_TASKS)
	    	DebugStr( "\pIt's quitting so we don't install deferred task;g" );
#endif
	    }
	    else
	    {
		// now that we're running on OS X, call the "deferred task" immediately.
		// Using a deferred thread is an artifact of OS 9: using deferred time
		// because it's not quite as nasty as hard interrupt time, plus there were
		// things that assumed they could only be interrupted "once". Those should
		// now all be THREAD safe (i.e. OS X) and calling deferred time oughtn't
		// be necessary.
		Timeline::DeferredTaskProc((long)pTheTimeline);
	    }
	}
    }
    
    HXMM_INTERRUPTOFF();
}

pascal void Timeline::DeferredTaskProc(long param)
{
    HXMM_INTERRUPTON();

    Timeline* pTheTimeline = (Timeline*)param;
		     
    if (pTheTimeline)
    {
#ifdef THREADS_SUPPORTED
	HX_LOCK(pTheTimeline->m_pCoreMutex);
#endif

	pTheTimeline->m_bAnyCallbackPending = 0;
	
	if ( !pTheTimeline->m_bIsQuitting )
	{
	    pTheTimeline->InterruptTimeSync();
	}
#if defined (_DEBUG) && defined(LOG_MULTIPLE_DEFERRED_TASKS)
	else
	{
	    DebugStr( "\pIn DeferredTaskProc, short-circuiting because we're quitting;g" );
	}
#endif
#ifdef THREADS_SUPPORTED
	HX_UNLOCK(pTheTimeline->m_pCoreMutex);
#endif
    }

#if defined (_DEBUG) && defined(LOG_MULTIPLE_DEFERRED_TASKS)
    if ( pTheTimeline )
    {
	if ( pTheTimeline->m_uiDeferredTaskDepth != 0 )
	{
	    char tmpStr[255]; /* Flawfinder: ignore */
	    sprintf(tmpStr, "m_uiDeferredTaskDepth = %u;g", pTheTimeline->m_uiDeferredTaskDepth); /* Flawfinder: ignore */
	    DebugStr(c2pstr(tmpStr));
	}
    }
#endif
    HXMM_INTERRUPTOFF();
}

void 
Timeline::InterruptTimeSync(void)
{ 
    if (!m_bUseDeferredTasks)
    {
    	ULONG32 ulCurTime = HX_GET_TICKCOUNT();
    	if ( ulCurTime > (m_ulLastCallbackTime + m_ulGranularity) )
    	{
	    OnTimeSync(FALSE);
    	}
    }
    else
    {
    	if (m_pInterruptState)
    	{
	    m_pInterruptState->EnterInterruptState();
	}
    
    	OnTimeSync(TRUE);
    
#if defined (_DEBUG) && defined(LOG_MULTIPLE_DEFERRED_TASKS)
	if ( m_bIsQuitting )
	{
	    DebugStr( "\pIn InterruptTimeSync, about to ::PrimeTime, and we're QUITTING!;g" );
	}
#endif
    	
	if ( !m_bIsQuitting )
	{
	    ::PrimeTime((QElemPtr) &m_tmInfo, m_ulGranularity); //restart timer
	    m_bAnyCallbackPending = 1;
	}
    
    	if (m_pInterruptState)
    	{
       	    m_pInterruptState->LeaveInterruptState();
    	}
    }
} 
#endif //_MACINTOSH

/*
 *  ********* MAC & UNIX ONLY *****************
 */
#if defined(_UNIX) || defined(_MACINTOSH) || defined(__TCS__) || defined(_OPENWAVE)
void
Timeline::CallAllTimeSyncs(void)
{    
#if defined(HELIX_CONFIG_NOSTATICS)
    TimelineMapType& m_zTimerMap =
	GlobalTimelineMapType::Get(&Timeline::m_zTimerMap);
#endif

    CHXMapPtrToPtr::Iterator ndxTimeline = m_zTimerMap.Begin();
    for (; ndxTimeline != m_zTimerMap.End(); ++ndxTimeline)
    {
	Timeline* pCurrentTimeline = (Timeline*)(*ndxTimeline);
	pCurrentTimeline->OnTimeSync();
    }
}
#endif /* _UNIX || _MACINTOSH */

