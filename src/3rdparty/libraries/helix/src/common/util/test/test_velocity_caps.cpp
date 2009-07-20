/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: test_velocity_caps.cpp,v 1.3 2007/07/06 20:39:29 jfinnecy Exp $
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
#include "hxcom.h"
#include "hxplayvelocity.h"
#include "test_velocity_caps.h"

CHXPlaybackVelocityCapsTest::CHXPlaybackVelocityCapsTest()
{
    m_ppCaps[0] = new CHXPlaybackVelocityCaps();
    m_ppCaps[1] = new CHXPlaybackVelocityCaps();
}

CHXPlaybackVelocityCapsTest::~CHXPlaybackVelocityCapsTest()
{
    HX_DELETE(m_ppCaps[0]);
    HX_DELETE(m_ppCaps[1]);
}

void CHXPlaybackVelocityCapsTest::GetCommandInfo(UTVector<HLXUnitTestCmdInfo*>& cmds)
{
    cmds.Resize(6);

    cmds[0] = new HLXUnitTestCmdInfoDisp<CHXPlaybackVelocityCapsTest>(this, 
                                                                      "CHXPlaybackVelocityCaps()",
                                                                      &CHXPlaybackVelocityCapsTest::HandleConstructorCmd,
                                                                      2);
    cmds[1] = new HLXUnitTestCmdInfoDisp<CHXPlaybackVelocityCapsTest>(this, 
                                                                      "GetNumRanges",
                                                                      &CHXPlaybackVelocityCapsTest::HandleGetNumRangesCmd,
                                                                      3);
    cmds[2] = new HLXUnitTestCmdInfoDisp<CHXPlaybackVelocityCapsTest>(this, 
                                                                      "GetRange",
                                                                      &CHXPlaybackVelocityCapsTest::HandleGetRangeCmd,
                                                                      5);
    cmds[3] = new HLXUnitTestCmdInfoDisp<CHXPlaybackVelocityCapsTest>(this, 
                                                                      "AddRange",
                                                                      &CHXPlaybackVelocityCapsTest::HandleAddRangeCmd,
                                                                      5);
    cmds[4] = new HLXUnitTestCmdInfoDisp<CHXPlaybackVelocityCapsTest>(this, 
                                                                      "CombineCapsLogicalAnd",
                                                                      &CHXPlaybackVelocityCapsTest::HandleCombineCapsLogicalAndCmd,
                                                                      2);
    cmds[5] = new HLXUnitTestCmdInfoDisp<CHXPlaybackVelocityCapsTest>(this, 
                                                                       "IsCapable",
                                                                       &CHXPlaybackVelocityCapsTest::HandleIsCapableCmd,
                                                                       4);
}

const char* CHXPlaybackVelocityCapsTest::DefaultCommandLine() const
{
    return "test_velocity_caps test_velocity_caps.in -a";
}

HLXCmdBasedTest* CHXPlaybackVelocityCapsTest::Clone() const
{
    return new CHXPlaybackVelocityCapsTest();
}

bool CHXPlaybackVelocityCapsTest::HandleConstructorCmd(const UTVector<UTString>& params)
{
    bool bRet = false;

    UINT32 ulIndex = strtoul(params[1], NULL, 10);
    if (ulIndex < 2)
    {
        HX_DELETE(m_ppCaps[ulIndex]);
        m_ppCaps[ulIndex] = new CHXPlaybackVelocityCaps();
        if (m_ppCaps[ulIndex])
        {
            bRet = true;
        }
    }

    return bRet;
}

bool CHXPlaybackVelocityCapsTest::HandleGetNumRangesCmd(const UTVector<UTString>& params)
{
    bool bRet = false;

    UINT32 ulIndex = strtoul(params[1], NULL, 10);
    if (ulIndex < 2 && m_ppCaps[ulIndex])
    {
        UINT32 ulExpectedNum = strtoul(params[2], NULL, 10);
        UINT32 ulActualNum   = m_ppCaps[ulIndex]->GetNumRanges();
        if (ulExpectedNum == ulActualNum)
        {
            bRet = true;
        }
    }

    return bRet;
}

bool CHXPlaybackVelocityCapsTest::HandleGetRangeCmd(const UTVector<UTString>& params)
{
    bool bRet = false;

    UINT32 ulIndex = strtoul(params[1], NULL, 10);
    if (ulIndex < 2 && m_ppCaps[ulIndex])
    {
        UINT32    ulRangeIndex = strtoul(params[2], NULL, 10);
        INT32     lExpectedMin = (INT32) strtol(params[3], NULL, 10);
        INT32     lExpectedMax = (INT32) strtol(params[4], NULL, 10);
        INT32     lActualMin   = 0;
        INT32     lActualMax   = 0;
        HX_RESULT retVal       = m_ppCaps[ulIndex]->GetRange(ulRangeIndex, lActualMin, lActualMax);
        if (SUCCEEDED(retVal) &&
            lExpectedMin == lActualMin &&
            lExpectedMax == lActualMax)
        {
            bRet = true;
        }
    }

    return bRet;
}

bool CHXPlaybackVelocityCapsTest::HandleAddRangeCmd(const UTVector<UTString>& params)
{
    bool bRet = false;

    UINT32 ulIndex = strtoul(params[1], NULL, 10);
    if (ulIndex < 2 && m_ppCaps[ulIndex])
    {
        INT32     lMinToAdd  = strtol(params[2], NULL, 10);
        INT32     lMaxToAdd  = strtol(params[3], NULL, 10);
        UINT32    ulExpRet   = strtoul(params[4], NULL, 10);
        HX_RESULT retVal     = m_ppCaps[ulIndex]->AddRange(lMinToAdd, lMaxToAdd);
        if ((ulExpRet && SUCCEEDED(retVal)) ||
            (!ulExpRet && FAILED(retVal)))
        {
            bRet = true;
        }
    }

    return bRet;
}

bool CHXPlaybackVelocityCapsTest::HandleCombineCapsLogicalAndCmd(const UTVector<UTString>& params)
{
    bool bRet = false;

    UINT32 ulIndex = strtoul(params[1], NULL, 10);
    if (ulIndex < 2 && m_ppCaps[0] && m_ppCaps[1])
    {
        UINT32    ulOtherIndex = (ulIndex == 0 ? 1 : 0);
        HX_RESULT retVal       = m_ppCaps[ulIndex]->CombineCapsLogicalAnd(m_ppCaps[ulOtherIndex]);
        if (SUCCEEDED(retVal))
        {
            bRet = true;
        }
    }

    return bRet;
}

bool CHXPlaybackVelocityCapsTest::HandleIsCapableCmd(const UTVector<UTString>& params)
{
    bool bRet = false;

    UINT32 ulIndex = strtoul(params[1], NULL, 10);
    if (ulIndex < 2 && m_ppCaps[ulIndex])
    {
        INT32  lVelToCheck = strtol(params[2], NULL, 10);
        UINT32 ulExpRet    = strtoul(params[3], NULL, 10);
        HXBOOL   bExpRet     = (ulExpRet ? TRUE : FALSE);
        HXBOOL   bActRet     = m_ppCaps[ulIndex]->IsCapable(lVelToCheck);
        if (bActRet == bExpRet)
        {
            bRet = true;
        }
    }

    return bRet;
}

