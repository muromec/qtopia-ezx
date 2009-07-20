/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: tconvert_test.cpp,v 1.4 2007/07/06 20:39:29 jfinnecy Exp $
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

#include "hx_ut_debug.h"
#include "ut_param_util.h"

#include "tconvert_test.h"



HLXTConvertTest::HLXTConvertTest()
{
    ;
}

HLXTConvertTest::~HLXTConvertTest()
{
    ;
}
    
void HLXTConvertTest::GetCommandInfo(UTVector<HLXUnitTestCmdInfo*>& cmds)
{
    cmds.Resize(8);

    cmds[0] = new HLXUnitTestCmdInfoDisp<HLXTConvertTest>(this, 
				    "Init",
				    &HLXTConvertTest::HandleInitCmd,
				    2,
				    3);
    cmds[1] = new HLXUnitTestCmdInfoDisp<HLXTConvertTest>(this, 
				    "SetHXAnchor",
				    &HLXTConvertTest::HandleSetHXAnchorCmd,
				    2);
    cmds[2] = new HLXUnitTestCmdInfoDisp<HLXTConvertTest>(this, 
				    "SetRTPAnchor",
				    &HLXTConvertTest::HandleSetRTPAnchorCmd,
				    2);
    cmds[3] = new HLXUnitTestCmdInfoDisp<HLXTConvertTest>(this, 
				    "SetAnchor",
				    &HLXTConvertTest::HandleSetAnchorCmd,
				    3);
    cmds[4] = new HLXUnitTestCmdInfoDisp<HLXTConvertTest>(this, 
				    "Hxa2Rtp",
				    &HLXTConvertTest::HandleHxa2RtpCmd,
				    3,
				    4);
    cmds[5] = new HLXUnitTestCmdInfoDisp<HLXTConvertTest>(this, 
				    "Rtp2Hxa",
				    &HLXTConvertTest::HandleRtp2HxaCmd,
				    3,
				    4);
    cmds[6] = new HLXUnitTestCmdInfoDisp<HLXTConvertTest>(this, 
				    "Hxa2Rtp_Raw",
				    &HLXTConvertTest::HandleHxa2RtpRawCmd,
				    3,
				    4);
    cmds[7] = new HLXUnitTestCmdInfoDisp<HLXTConvertTest>(this, 
				    "Rtp2Hxa_Raw",
				    &HLXTConvertTest::HandleRtp2HxaRawCmd,
				    3,
				    4);
}

HLXCmdBasedTest* HLXTConvertTest::Clone() const
{
    return new HLXTConvertTest();
}


/*****************************************************************************
 *  DataFile Commands
 */ 
bool HLXTConvertTest::HandleInitCmd(const UTVector<UTString>& info)
{
    bool bRetVal = false;
    unsigned int val1;
    unsigned int val2;

    if (info.Nelements() == 2)
    {
	if (UTParamUtil::GetUInt(info[1], val1))
	{
	    m_TSConverter.ReInit(CHXTimestampConverter::SAMPLES, val1);
	    bRetVal = true;
	}
    }
    else if (info.Nelements() == 3)
    {
	if (UTParamUtil::GetUInt(info[1], val1) &&
	    UTParamUtil::GetUInt(info[2], val2))
	{
	    m_TSConverter.ReInit(CHXTimestampConverter::FACTORS, val1, val2);
	    bRetVal = true;
	}
    }

    return bRetVal;
}

bool HLXTConvertTest::HandleSetHXAnchorCmd(const UTVector<UTString>& info)
{
    bool bRetVal = false;
    unsigned int val1;

    if (UTParamUtil::GetUInt(info[1], val1))
    {
	m_TSConverter.setHXAnchor(val1);
	bRetVal = true;
    }

    return bRetVal;
}

bool HLXTConvertTest::HandleSetRTPAnchorCmd(const UTVector<UTString>& info)
{
    bool bRetVal = false;
    unsigned int val1;

    if (UTParamUtil::GetUInt(info[1], val1))
    {
	m_TSConverter.setRTPAnchor(val1);
	bRetVal = true;
    }

    return bRetVal;
}

bool HLXTConvertTest::HandleSetAnchorCmd(const UTVector<UTString>& info)
{
    bool bRetVal = false;
    unsigned int val1;
    unsigned int val2;

    if (UTParamUtil::GetUInt(info[1], val1) &&
	UTParamUtil::GetUInt(info[2], val2))
    {
	m_TSConverter.setAnchor(val1, val2);
	bRetVal = true;
    }

    return bRetVal;
}

bool HLXTConvertTest::HandleConversionCmd(const UTVector<UTString>& info, 
					  ConversionType convType)
{
    bool bRetVal = false;
    unsigned int valIn;
    unsigned int valOut;
    unsigned int valTolerance = 0;
    unsigned int result;
    int diff;

    if (UTParamUtil::GetUInt(info[1], valIn) &&
	UTParamUtil::GetUInt(info[2], valOut))
    {
	bRetVal = true;

	switch (convType)
	{
	case HXA2RTP:
	    result = m_TSConverter.hxa2rtp(valIn);
	    break;
	case RTP2HXA:
	    result = m_TSConverter.rtp2hxa(valIn);
	    break;
	case HXA2RTPRAW:
	    result = m_TSConverter.hxa2rtp_raw(valIn);
	    break;
	case RTP2HXARAW:
	    result = m_TSConverter.rtp2hxa_raw(valIn);
	    break;
	default:
	    bRetVal = false;
	}

	if (bRetVal && (info.Nelements() > 3))
	{
	    bRetVal = UTParamUtil::GetUInt(info[3], valTolerance);
	}

	if (bRetVal)
	{
	    diff = result - valOut;
	    if (diff < 0)
	    {
		diff = -diff;
	    }

	    bRetVal = (diff <= valTolerance);
	}
    }

    return bRetVal;
}

bool HLXTConvertTest::HandleHxa2RtpCmd(const UTVector<UTString>& info)
{
    return HandleConversionCmd(info, HXA2RTP);
}

bool HLXTConvertTest::HandleRtp2HxaCmd(const UTVector<UTString>& info)
{
    return HandleConversionCmd(info, RTP2HXA);
}

bool HLXTConvertTest::HandleHxa2RtpRawCmd(const UTVector<UTString>& info)
{
    return HandleConversionCmd(info, HXA2RTPRAW);
}

bool HLXTConvertTest::HandleRtp2HxaRawCmd(const UTVector<UTString>& info)
{
    return HandleConversionCmd(info, RTP2HXARAW);
}


