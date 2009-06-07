/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: tsmoothtime.cpp,v 1.3 2005/03/14 19:36:42 bobclark Exp $
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

#include "tsmoothtime.h"

CHXTimeSyncSmootherTest::CHXTimeSyncSmootherTest()
{
    m_pSmoother = new CHXTimeSyncSmoother();
}

CHXTimeSyncSmootherTest::~CHXTimeSyncSmootherTest()
{
    HX_DELETE(m_pSmoother);
}

void CHXTimeSyncSmootherTest::GetCommandInfo(UTVector<HLXUnitTestCmdInfo*>& cmds)
{
    cmds.Resize(5);

    cmds[0] = new HLXUnitTestCmdInfoDisp<CHXTimeSyncSmootherTest>(this, 
                                                                  "CHXTimeSyncSmoother()",
                                                                  &CHXTimeSyncSmootherTest::HandleConstructorCmd,
                                                                  1);
    cmds[1] = new HLXUnitTestCmdInfoDisp<CHXTimeSyncSmootherTest>(this, 
                                                                  "OnTimeSyncTick",
                                                                  &CHXTimeSyncSmootherTest::HandleOnTimeSyncTickCmd,
                                                                  3);
    cmds[2] = new HLXUnitTestCmdInfoDisp<CHXTimeSyncSmootherTest>(this, 
                                                                  "GetTimeNowTick",
                                                                  &CHXTimeSyncSmootherTest::HandleGetTimeNowTickCmd,
                                                                  3);
    cmds[3] = new HLXUnitTestCmdInfoDisp<CHXTimeSyncSmootherTest>(this, 
                                                                  "WasLastSampleGood",
                                                                  &CHXTimeSyncSmootherTest::HandleWasLastSampleGoodCmd,
                                                                  2);
    cmds[4] = new HLXUnitTestCmdInfoDisp<CHXTimeSyncSmootherTest>(this, 
                                                                  "SetVelocity",
                                                                  &CHXTimeSyncSmootherTest::HandleSetVelocityCmd,
                                                                  2);
}

const char* CHXTimeSyncSmootherTest::DefaultCommandLine() const
{
    return "tsmoothtime tsmoothtime.in -a";
}

HLXCmdBasedTest* CHXTimeSyncSmootherTest::Clone() const
{
    return new CHXTimeSyncSmootherTest();
}

bool CHXTimeSyncSmootherTest::HandleConstructorCmd(const UTVector<UTString>& params)
{
    bool bRet = false;

    HX_DELETE(m_pSmoother);
    m_pSmoother = new CHXTimeSyncSmoother();
    if (m_pSmoother)
    {
        bRet = true;
    }

    return bRet;
}

bool CHXTimeSyncSmootherTest::HandleOnTimeSyncTickCmd(const UTVector<UTString>& params)
{
    bool bRet = false;

    UINT32 ulTime = strtoul(params[1], NULL, 10);
    UINT32 ulTick = strtoul(params[2], NULL, 10);
    if (m_pSmoother)
    {
        m_pSmoother->OnTimeSyncTick(ulTime, ulTick);
        bRet = true;
    }

    return bRet;
}

bool CHXTimeSyncSmootherTest::HandleGetTimeNowTickCmd(const UTVector<UTString>& params)
{
    bool bRet = false;

    UINT32 ulTick         = strtoul(params[1], NULL, 10);
    UINT32 ulExpectedTime = strtoul(params[2], NULL, 10);
    if (m_pSmoother)
    {
        UINT32 ulTime = m_pSmoother->GetTimeNowTick(ulTick);
        if (ulTime == ulExpectedTime)
        {
            bRet = true;
        }
    }

    return bRet;
}

bool CHXTimeSyncSmootherTest::HandleWasLastSampleGoodCmd(const UTVector<UTString>& params)
{
    bool bRet = false;

    HXBOOL bExpectedResult = FALSE;
    if (!strcmp((const char*) params[1], "TRUE"))
    {
        bExpectedResult = TRUE;
    }
    if (m_pSmoother)
    {
        HXBOOL bActualResult = m_pSmoother->WasLastSampleGood();
        if (bActualResult == bExpectedResult)
        {
            bRet = true;
        }
    }

    return bRet;
}

bool CHXTimeSyncSmootherTest::HandleSetVelocityCmd(const UTVector<UTString>& params)
{
    bool bRet = false;

    INT32 lVelocity = (INT32) strtol(params[1], NULL, 10);
    if (m_pSmoother)
    {
        m_pSmoother->SetVelocity(lVelocity);
        bRet = true;
    }

    return bRet;
}

