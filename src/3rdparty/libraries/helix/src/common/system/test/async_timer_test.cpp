/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: async_timer_test.cpp,v 1.5 2007/07/06 20:42:03 jfinnecy Exp $
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
#include "async_timer_test.h"

#include "hx_unit_test.h"
#include "hx_ut_debug.h"

#include "hxthread.h"
#include "hxtick.h"

HXAsyncTimerTest::HXAsyncTimerTest() :
    m_ulTimerID(0),
    m_bFailed(FALSE),
    m_ulLastTime(0),
    m_ulTimeoutPeriod(0),
    m_ulTimeoutCount(0)
{}

HXAsyncTimerTest::~HXAsyncTimerTest()
{}

const char* HXAsyncTimerTest::DefaultCommandLine() const
{
    return "HXAsyncTimerTest";
}

bool HXAsyncTimerTest::Init(int argc, char* argv[])
{
    return true;
}

bool HXAsyncTimerTest::Start()
{
    HXBOOL bRet = FALSE;

    SetInstance(this);

    if (RunTimeoutTest(100) &&
	RunTimeoutTest(250) &&
	RunTimeoutTest(456) &&
	RunTimeoutTest(1000))
	bRet = true;

    return bRet;
}

void HXAsyncTimerTest::Reset()
{

}

bool HXAsyncTimerTest::RunTimeoutTest(ULONG32 ulPeriod)
{
    bool bRet = false;

    m_ulLastTime = GetTickCount();
    m_ulTimeoutCount = 10;
    m_ulTimeoutPeriod = ulPeriod;
    m_bFailed = false;

    m_ulTimerID = HXAsyncTimer::SetTimer(m_ulTimeoutPeriod, 
					 &HXAsyncTimerTest::TimerFunc);

    ULONG32 ulStartTime = GetTickCount();
    ULONG32 ulEndTime = ulStartTime + 2 * m_ulTimeoutPeriod * m_ulTimeoutCount;

    while((!m_bFailed) &&
	  (m_ulTimeoutCount > 0) && 
	  (GetTickCount() < ulEndTime))
    {
	ProcessEvents();
    }

    if (!m_bFailed && (m_ulTimeoutCount == 0))
    {
	bRet = true;
    }

    if (m_ulTimerID)
    {
	HXAsyncTimer::KillTimer(m_ulTimerID);
	m_ulTimerID = 0;
    }

    return bRet;
}

void HXAsyncTimerTest::OnTimeout(UINT32 idEvent, ULONG32 ulTime)
{
    ULONG32 ulTimeDelta = ulTime - m_ulLastTime;
    ULONG32 ulFudge = 2 * SchedulerQuantum();

    DPRINTF(D_INFO, ("HXAsyncTimerTest::OnTimeout() : delta %d period %d\n",
		     ulTimeDelta,
		     m_ulTimeoutPeriod));
    
    ULONG32 ulDelta = 0;

    if (ulTimeDelta > m_ulTimeoutPeriod)
	ulDelta = ulTimeDelta - m_ulTimeoutPeriod;
    else
	ulDelta = m_ulTimeoutPeriod - ulTimeDelta;
    

    if (ulDelta > ulFudge)
    {
	DPRINTF(D_INFO, ("HXAsyncTimerTest::OnTimeout() : Delta was too large\n"));
	m_bFailed = true;
    }

    m_ulTimeoutCount--;
    m_ulLastTime = ulTime;
}

void HXAsyncTimerTest::TimerFunc(void* handle, UINT32 uMesg, UINT32 idEvent, 
				 ULONG32 ulTime)
{
    DPRINTF(D_INFO, ("HXAsyncTimerTest::TimerFunc(%p, %d, %d, %d)\n",
		     handle, uMesg, idEvent, ulTime));

    HXAsyncTimerTest* pTest = HXAsyncTimerTest::GetInstance();

    if (pTest)
	pTest->OnTimeout(idEvent, ulTime);
}

static HXAsyncTimerTest* z_pTestInstance = 0;

HXAsyncTimerTest* HXAsyncTimerTest::GetInstance()
{
    return z_pTestInstance;
}

void HXAsyncTimerTest::SetInstance(HXAsyncTimerTest* pInstance)
{
    z_pTestInstance = pInstance;
}
