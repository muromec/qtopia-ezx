/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: scheduler_test.cpp,v 1.4 2004/07/09 18:43:05 hubbe Exp $
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

#include "./scheduler_test.h"

#include "hx_ut_debug.h"
#include "ut_param_util.h"

#include "microsleep.h"

#include "./timeout_op.h"
#include "./shutdown_op.h"
#include "./remove_op.h"
#include "./test_callback.h"

HXSchedulerTest::HXSchedulerTest() :
    m_pSched(0),
    m_done(false),
    m_failed(false),
    m_nextOpID(0),
    m_finalOpID(0),
    m_highestTimeout(0),
    m_lastCallbackHandle(0)
{
    CreateScheduler();
}

HXSchedulerTest::~HXSchedulerTest()
{
    HX_RELEASE(m_pSched);
}

const char* HXSchedulerTest::DefaultCommandLine() const
{
    return "tscheduler tscheduler.in";
}

void HXSchedulerTest::GetCommandInfo(UTVector<HLXUnitTestCmdInfo*>& cmds)
{
    cmds.Resize(4);
    
    cmds[0] = new HLXUnitTestCmdInfoDisp<HXSchedulerTest>(this, 
				    "Reset",
				    &HXSchedulerTest::HandleResetCmd,
				    1);
    cmds[1] = new HLXUnitTestCmdInfoDisp<HXSchedulerTest>(this, 
				    "Timeout",
				    &HXSchedulerTest::HandleTimeoutCmd,
				    3);
    cmds[2] = new HLXUnitTestCmdInfoDisp<HXSchedulerTest>(this, 
				    "CancelLast",
				    &HXSchedulerTest::HandleCancelLastCmd,
				    3);
    cmds[3] = new HLXUnitTestCmdInfoDisp<HXSchedulerTest>(this, 
				    "RunScheduler",
				    &HXSchedulerTest::HandleRunSchedulerCmd,
				    1);
}

HLXCmdBasedTest* HXSchedulerTest::Clone() const
{
    return new HXSchedulerTest();
}

void HXSchedulerTest::OperationFailed()
{
    m_failed = true;
}

bool HXSchedulerTest::RemoveHandle(CallbackHandle handle)
{
    return (m_pSched->Remove(handle) == HXR_OK);
}

void HXSchedulerTest::StopScheduler()
{
    m_pSched->StopScheduler();
    m_done = true;
}

bool HXSchedulerTest::OnOperation(UINT32 operationID)
{
    bool ret = false;

    if (operationID == m_nextOpID)
    {
	m_nextOpID++;
	ret = true;
    }
    else
    {
	DPRINTF(D_ERROR, ("HXSchedulerTest::OnOperation() : out of order operation detected\n"));
	OperationFailed();
	StopScheduler();
    }

    return ret;
}

ULONG32 HXSchedulerTest::GetMSFromStart()
{
    ULONG32 ret = 0;
    HXTimeval now = m_pSched->GetCurrentSchedulerTime();

    ret = (now.tv_sec - m_startTime.tv_sec) * 1000;
    
    if (now.tv_usec < m_startTime.tv_sec)
    {
	ret -= 1000;
	now.tv_usec += 1000000;
    }

    ret += (now.tv_usec - m_startTime.tv_usec) / 1000;

    return ret;
}

bool HXSchedulerTest::HandleResetCmd(const UTVector<UTString>& info)
{
    m_done = false;
    m_failed = false;
    m_nextOpID = 0;
    m_finalOpID = 0;
    m_highestTimeout = 0;

    CreateScheduler();

    return true;
}

bool HXSchedulerTest::HandleTimeoutCmd(const UTVector<UTString>& info)
{
    // Handles commands of the following form
    // Timeout <operationID> <timeout>

    bool ret = false;

    unsigned int opID = 0;
    unsigned int timeout = 0;

    if (!UTParamUtil::GetUInt(info[1], opID) ||
	!UTParamUtil::GetUInt(info[2], timeout))
    {
	DPRINTF(D_ERROR, ("HXSchedulerTest::HandleTimeoutCmd() : failed to convert parameters\n"));
    }
    else
    {
	AddCallback(opID, timeout, new TimeoutOp(timeout));

	ret = true;
    }

    return ret;
}

bool HXSchedulerTest::HandleCancelLastCmd(const UTVector<UTString>& info)
{
    // Handles commands of the following form
    // CancelLast <operationID> <timeout>

    bool ret = false;

    unsigned int opID = 0;
    unsigned int timeout = 0;

    if (!UTParamUtil::GetUInt(info[1], opID) ||
	!UTParamUtil::GetUInt(info[2], timeout))
    {
	DPRINTF(D_ERROR, ("HXSchedulerTest::HandleCancelLastCmd() : failed to convert parameters\n"));
    }
    else
    {
	AddCallback(opID, timeout, new RemoveOp(m_lastCallbackHandle));

	ret = true;
    }

    return ret;
}

bool HXSchedulerTest::HandleRunSchedulerCmd(const UTVector<UTString>& info)
{
    return RunScheduler();
}

void HXSchedulerTest::AddCallback(unsigned int operationID, 
				  unsigned int timeout,
				  TestOperation* pOp)
{
    CallbackHandle ret = 0;

    IHXCallback* pCB = new HXSchedulerTestCB(operationID,
					     this, 
					     pOp);
    m_lastCallbackHandle = m_pSched->RelativeEnter(pCB, timeout);

    if (m_finalOpID < (operationID + 1))
	m_finalOpID = operationID + 1;

    if (timeout > m_highestTimeout)
	m_highestTimeout = timeout;
}

void HXSchedulerTest::CreateScheduler()
{
    HX_RELEASE(m_pSched);
    
    m_pSched = new HXScheduler(0);
    m_pSched->AddRef();    
}

bool HXSchedulerTest::RunScheduler()
{
    bool ret = false;

    m_nextOpID = 0;
    m_failed = false;
    m_done = false;

    // Add a Shutdown operation to make sure we terminate
    AddCallback(m_finalOpID,
		m_highestTimeout + 500,
		new ShutdownOp());

    m_startTime = m_pSched->GetCurrentSchedulerTime();
    m_pSched->StartScheduler();

    while(!m_done)
    {
	ProcessEvents();
    }

    HX_RELEASE(m_pSched);

    if (m_failed)
    {
	DPRINTF(D_ERROR, ("HXSchedulerTest::RunScheduler() : An operation failed\n"));
    }
    else if (m_finalOpID != m_nextOpID)
    {
	DPRINTF(D_ERROR, ("HXSchedulerTest::RunScheduler() : Next OpID %d did not match Final OpID %d\n",
			  m_nextOpID,
			  m_finalOpID));
    }
    else
	ret = true;

    return ret;
}
