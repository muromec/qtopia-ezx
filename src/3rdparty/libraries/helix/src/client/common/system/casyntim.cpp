/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: casyntim.cpp,v 1.14 2007/07/06 21:58:03 jfinnecy Exp $
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

#include "hxtypes.h"
#include "hxresult.h"
#include "hxtick.h"

#include "timeval.h"
#include "pq.h"
#include "hxthread.h"

#include "hxcom.h"
#include "hxengin.h"
#include "hxsched.h"
#include "thrdutil.h"
#include "pckunpck.h"
#include "hxmsgs.h"
#include "casyntim.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE             
static const char HX_THIS_FILE[] = __FILE__;
#endif

//XXXgfw when we do thread priorities on linux we will change this.
#ifdef _UNIX
#define THREAD_PRIORITY_HIGHEST 1
#define THREAD_PRIORITY_NORMAL  0
#elif defined (_CARBON)
#define THREAD_PRIORITY_HIGHEST 10000
#define THREAD_PRIORITY_NORMAL 100
#elif defined(_SYMBIAN)
#include <e32std.h>
#define THREAD_PRIORITY_HIGHEST EPriorityMuchMore
#define THREAD_PRIORITY_NORMAL EPriorityNormal
#elif defined(_OPENWAVE)
#define THREAD_PRIORITY_HIGHEST 1
#define THREAD_PRIORITY_NORMAL  0
#endif

void* ThreadRoutine2(void * pArg);

CAsyncTimer::CAsyncTimer(IUnknown* pContext, HXScheduler* pScheduler)
{
    m_pContext = pContext;
    HX_ADDREF(m_pContext);

    m_pScheduler    = pScheduler;
    m_ulStartTime   = 0;
    m_ulGranularity = 100;
    m_pThread       = NULL;
    m_pQuitEvent    = NULL;
    m_uPlayingStateCount = 0;
}

CAsyncTimer::~CAsyncTimer()
{
    StopTimer();
    HX_RELEASE(m_pContext);
}

//  timegettime

HX_RESULT CAsyncTimer::StartTimer(void)
{
    HX_RESULT theErr = HXR_OK;

    theErr = CreateEventCCF((void**)&m_pQuitEvent, m_pContext, NULL, TRUE);
    if (HXR_OK == theErr)
    {
        theErr = CreateInstanceCCF(CLSID_IHXThread, (void**)&m_pThread, m_pContext);
        if (!theErr)
        {
            theErr = m_pThread->CreateThread(ThreadRoutine2, (void*) this, 0);
            if (SUCCEEDED(theErr))
            {
                m_pThread->SetThreadName("Asynchronous Timer Thread");
            }
        }
    }

    return theErr;
}

HX_RESULT CAsyncTimer::StopTimer(void)
{
    if (m_pThread)
    {
        HXThreadMessage msg(HXMSG_QUIT, NULL, NULL);
        if (m_pThread->PostMessage(&msg, NULL) == HXR_OK)
        {
            m_pQuitEvent->Wait(ALLFS);
        }
        m_pThread->Exit(0);
        HX_RELEASE(m_pThread);
    }

    HX_RELEASE(m_pQuitEvent);

    return HXR_OK;
}

HXBOOL CAsyncTimer::InTimerThread(void)
{
    HXBOOL bRet = FALSE;

    if (m_pThread)
    {
        UINT32 ulThreadId = 0;

        m_pThread->GetThreadId(ulThreadId);

        if (ulThreadId == HXGetCurrentThreadID())
        {
            bRet = TRUE;
        }
    }

    return bRet;
}

void CAsyncTimer::NotifyPlayState(HXBOOL bInPlayingState)
{
    HX_ASSERT(bInPlayingState || (m_uPlayingStateCount > 0));

    if (bInPlayingState)
    {
        m_uPlayingStateCount++;
        HX_ASSERT(m_pThread != NULL);
        if (m_uPlayingStateCount == 1 && m_pThread)
        {
            m_pThread->SetPriority(THREAD_PRIORITY_HIGHEST);
        }
    }
    else if (m_uPlayingStateCount > 0)
    {
        m_uPlayingStateCount--;
        HX_ASSERT(m_pThread != NULL);
        if (m_uPlayingStateCount == 0 && m_pThread)
        {
            m_pThread->SetPriority(THREAD_PRIORITY_NORMAL);
        }
    }
}

void CAsyncTimer::SetStartTime(ULONG32 ulStartTime)
{
    m_ulStartTime = ulStartTime;
};

void CAsyncTimer::SetGranularity(ULONG32 ulGranularity)
{
    m_ulGranularity = ulGranularity;
};

#ifndef _UNIX
void* ThreadRoutine2(void * pArg)
{
    CAsyncTimer* pAsyncTimer = (CAsyncTimer*) pArg;
    ULONG32 ulSleepTime = pAsyncTimer->m_ulGranularity;

    ULONG32 ulStartTime = HX_GET_TICKCOUNT();

    IUnknown*       pContext = pAsyncTimer->m_pContext;
    IHXThread*      pThread = pAsyncTimer->m_pThread;
    IHXAsyncTimer*  pHelixAsyncTimer=NULL;
    HXThreadMessage msg;
    HXBOOL          bDone   = FALSE;
    UINT32          ulLastTimerCallback = ulStartTime;
    
    CreateInstanceCCF(CLSID_IHXAsyncTimer, (void**)&pHelixAsyncTimer, pContext);
    HX_ASSERT(pHelixAsyncTimer);
    UINT32 ulTimerId =  pHelixAsyncTimer->SetTimer(ulSleepTime, pThread);
    
    while (!bDone && pThread->GetMessage(&msg, 0,0) == HXR_OK)
    {
        switch (msg.m_ulMessage)
        {
            case HXMSG_ASYNC_TIMER: 
                {
                    ULONG32 ulCurrentTime = HX_GET_TICKCOUNT();
                    if (CALCULATE_ELAPSED_TICKS(ulLastTimerCallback, ulCurrentTime) 
                            >= ulSleepTime)
                    {
                        ulLastTimerCallback = ulCurrentTime;
                        pAsyncTimer->m_pScheduler->OnTimeSync(TRUE);
                    }
                }
                break;
            case HXMSG_QUIT:
                bDone   = TRUE;
                break;
            default:
                pThread->DispatchMessage(&msg);
                break;
        }
    }

    if( pHelixAsyncTimer && ulTimerId )
    {
        pHelixAsyncTimer->KillTimer(ulTimerId);
        HX_RELEASE(pHelixAsyncTimer);
    }

    pAsyncTimer->m_pQuitEvent->SignalEvent();

    return (void*) 0;
}
#else
void* ThreadRoutine2(void * pArg)
{
    CAsyncTimer*     pAsyncTimer = (CAsyncTimer*) pArg;
    IHXThread*       pThread     = pAsyncTimer->m_pThread;
    HXBOOL           bDone       = FALSE;
    UINT32           ulTS1       = 0;
    UINT32           ulTS2       = 0;
    UINT32           ulSleep     = 0;
    HXThreadMessage  msg;
    HXSchedulerTimer timer;
    
    //Init the scheduler timing helper
    timer.Init(pAsyncTimer->m_ulGranularity);

    while( !bDone && pThread->PeekMessage(&msg,0,0,1) )
    {
        switch (msg.m_ulMessage)
        {
            case HXMSG_QUIT:
                bDone   = TRUE;
                break;
            default:
                break;
        }

        ulTS1 = HX_GET_TICKCOUNT();
        pAsyncTimer->m_pScheduler->OnTimeSync(TRUE);
        ulTS2 = HX_GET_TICKCOUNT();

        ulSleep = timer.AddSample(ulTS1, ulTS2);

        if( 0 != ulSleep )
        {
            usleep(ulSleep);
        }
    }

    pAsyncTimer->m_pQuitEvent->SignalEvent();

    return (void*) 0;
}
#endif
