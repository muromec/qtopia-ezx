/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: ut_rateselinfo.cpp,v 1.3 2007/05/01 18:17:22 darrick Exp $
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

#include <string.h>
#include "hxcom.h"
#include "hxengin.h"
#include "hxqosinfo.h"
#include "rateselinfo.h"
#include "ut_rateselinfo.h"

#include "timeval.h"
#include "base_callback.h"
#include "pcktstrm.h"


UTRateSelInfoDriver::UTRateSelInfoDriver()
: UTBaseDriver()
{
    strcpy(m_szClassName, "UT: RateSelectionInfo");
}

/******************************************************************************
 * \brief Run - Run rate selection info unit tests.
 *
 * \return          HXR_OK.
 *****************************************************************************/

HX_RESULT
UTRateSelInfoDriver::Run()
{
    Test_SetGetInfo_Basic();
    Test_SetGetInfo_RSI_NONE();
    Test_SetGetInfo_RSI_DEFAULT_RULE();
    Test_SetGetInfo_RSI_AVGBITRATE();
    Test_SetGetInfo_RSI_STREAMGROUPID();
    Test_SetGetInfo_RSI_SUBSCRIBE_RULE();
    Test_SetGetInfo_RSI_UNSUBSCRIBE_RULE();
    Test_SetGetInfo_RSI_MAX();

    Test_SetGetStrmInfo_Basic();
    Test_SetGetStrmInfo_GetWithoutSetFromSetupStream();
    Test_SetGetStrmInfo_GetFromWrongStream();

    Test_RuleSubscriptions_Basic();
    Test_RuleSubscriptions_GetFromUnsetupStream();

    Test_StrmSetGetInfo_Basic();
    Test_StrmSetGetInfo_RSI_TRACKID();

    Test_StrmSetGetInfo_RSI_NONE();
    Test_StrmSetGetInfo_RSI_BANDWIDTH();
    Test_StrmSetGetInfo_RSI_SDB();
    Test_StrmSetGetInfo_RSI_MAX();

    Test_StrmRuleSubscriptions_Basic();
    Test_StrmRuleSubscriptions_Empty();
    Test_StrmRuleSubscriptions_BigArray();
    Test_StrmRuleSubscriptions_SmallArray();
    Test_StrmRuleSubscriptions_ZeroSizeArray();
    Test_StrmRuleSubscriptions_DoubleSubscribe();
    Test_StrmRuleSubscriptions_DoubleUnsubscribe();

    Summary();
    return HXR_OK;
}


/******************************************************************************
 * 
 *  Test basic RateSelectionInfo::SetInfo()/GetInfo() functionality for 
 *  stream info (valid case). Set Bandwidth, then get it.
 *
 *  Expected: Successful set, successful get. Value retrieved equals
 *            value set.
 *
 * \return          HXR_OK if test passed, HXR_FAIL if test failed.
 *****************************************************************************/

HX_RESULT
UTRateSelInfoDriver::Test_SetGetInfo_Basic()
{
    const static char* z_szTestName = "Test_SetGetInfo_Basic()";
    RateSelectionInfo RateInfo;
    
    if (FAILED(RateInfo.SetInfo(RSI_BANDWIDTH, 150000)))
    {
        ReportFail(z_szTestName);
        return HXR_FAIL;
    }

    UINT32 ulVal = 0;
    if (SUCCEEDED(RateInfo.GetInfo(RSI_BANDWIDTH, ulVal)) && ulVal == 150000)
    {
        ReportPass(z_szTestName);
    }
    else
    {
        ReportFail(z_szTestName);
        return HXR_FAIL;
    }

    return HXR_OK;
}


/******************************************************************************
 *
 * Test RateSelectionInfo::SetInfo()/GetInfo() with invalid parameters. Other
 * testcases use this template as the basis for invalid tests.
 *
 * Expected: HXR_INVALID_PARAMETER on set and get. ulVal outparam unchanged
 * after get.
 *
 * \param   szTestName      [in] Name of test.
 * \param   Id              [in] Id field to test.
 *
 * \return          HXR_OK if test passed, HXR_FAIL if test failed.
 *****************************************************************************/

HX_RESULT
UTRateSelInfoDriver::Test_SetGetInfo_Invalid(const char* szTestName, 
                                             RateSelInfoId Id)
{
    RateSelectionInfo RateInfo;
    
    if (RateInfo.SetInfo(Id, 67890) != HXR_INVALID_PARAMETER)
    {
        ReportFail(szTestName);
        return HXR_FAIL;
    }

    UINT32 ulVal = 12345;

    if ((RateInfo.GetInfo(Id, ulVal) != HXR_INVALID_PARAMETER)
    ||  ulVal != 12345)
    {

        ReportFail(szTestName);
        return HXR_FAIL;
    }

    ReportPass(szTestName);
    return HXR_OK;
}


/******************************************************************************
 *
 * Test RateSelectionInfo::SetInfo()/GetInfo() w/ info id
 * RSI_NONE (invalid case). Set field RSI_NONE, then get it.
 *
 * Expected: HXR_INVALID_PARAMETER on both set and get. ulVal outparam not
 *           modified.
 *
 * \return          HXR_OK if test passed, HXR_FAIL if test failed.
 *****************************************************************************/

HX_RESULT
UTRateSelInfoDriver::Test_SetGetInfo_RSI_NONE()
{
    return Test_SetGetInfo_Invalid("Test_SetGetInfo_RSI_NONE()", RSI_NONE);
}


/******************************************************************************
 *
 * Test RateSelectionInfo::SetInfo()/GetInfo() w/ info id
 * RSI_BANDWIDTH (invalid case). Set field RSI_BANDWIDTH, then get it.
 *
 * Expected: HXR_INVALID_PARAMETER on both set and get. ulVal outparam not
 *           modified.
 *
 * \return          HXR_OK if test passed, HXR_FAIL if test failed.
 *****************************************************************************/

HX_RESULT
UTRateSelInfoDriver::Test_SetGetInfo_RSI_DEFAULT_RULE()
{
    return Test_SetGetInfo_Invalid("Test_SetGetInfo_RSI_DEFAULT_RULE()", 
                                       RSI_DEFAULT_RULE);
}


/******************************************************************************
 * UTRateSelInfoDriver::Test_SetGetInfo_RSI_AVGBITRATE()
 *
 * Test RateSelectionInfo::SetInfo()/GetInfo() w/ info id
 * RSI_AVGBITRATE (invalid case). Set field RSI_AVGBITRATE, then get it.
 *
 * Expected: HXR_INVALID_PARAMETER on both set and get. ulVal outparam not
 *           modified.
 *
 * \return          HXR_OK if test passed, HXR_FAIL if test failed.
 *****************************************************************************/

HX_RESULT
UTRateSelInfoDriver::Test_SetGetInfo_RSI_AVGBITRATE()
{
    return Test_SetGetInfo_Invalid("Test_SetGetInfo_RSI_AVGBITRATE()", 
                                       RSI_AVGBITRATE);
}


/******************************************************************************
 * UTRateSelInfoDriver::Test_SetGetInfo_RSI_STREAMGROUPID()
 *
 * Test RateSelectionInfo::SetInfo()/GetInfo() w/ info id
 * RSI_STREAMGROUPID (invalid case). Set field RSI_STREAMGROUPID, then get it.
 *
 * Expected: HXR_INVALID_PARAMETER on both set and get. ulVal outparam not
 *           modified.
 *
 * \return          HXR_OK if test passed, HXR_FAIL if test failed.
 *****************************************************************************/

HX_RESULT
UTRateSelInfoDriver::Test_SetGetInfo_RSI_STREAMGROUPID()
{
    return Test_SetGetInfo_Invalid("Test_SetGetInfo_RSI_STREAMGROUPID()", 
                                       RSI_STREAMGROUPID);
}


/******************************************************************************
 * UTRateSelInfoDriver::Test_SetGetInfo_RSI_SUBSCRIBE_RULE()
 *
 * Test RateSelectionInfo::SetInfo()/GetInfo() w/ info id
 * RSI_SUBSCRIBE_RULE (invalid case). Set field RSI_SUBSCRIBE_RULE, then get it.
 *
 * Expected: HXR_INVALID_PARAMETER on both set and get. ulVal outparam not
 *           modified.
 *
 * \return          HXR_OK if test passed, HXR_FAIL if test failed.
 *****************************************************************************/

HX_RESULT
UTRateSelInfoDriver::Test_SetGetInfo_RSI_SUBSCRIBE_RULE()
{
    return Test_SetGetInfo_Invalid("Test_SetGetInfo_RSI_SUBSCRIBE_RULE()", 
                                       RSI_SUBSCRIBE_RULE);
}


/******************************************************************************
 * UTRateSelInfoDriver::Test_SetGetInfo_RSI_UNSUBSCRIBE_RULE()
 *
 * Test RateSelectionInfo::SetInfo()/GetInfo() w/ info id
 * RSI_UNSUBSCRIBE_RULE (invalid case). Set field RSI_UNSUBSCRIBE_RULE, then get it.
 *
 * Expected: HXR_INVALID_PARAMETER on both set and get. ulVal outparam not
 *           modified.
 *
 * \return          HXR_OK if test passed, HXR_FAIL if test failed.
 *****************************************************************************/

HX_RESULT
UTRateSelInfoDriver::Test_SetGetInfo_RSI_UNSUBSCRIBE_RULE()
{
    return Test_SetGetInfo_Invalid("Test_SetGetInfo_RSI_UNSUBSCRIBE_RULE()", 
                                       RSI_UNSUBSCRIBE_RULE);
}


/******************************************************************************
 * UTRateSelInfoDriver::Test_SetGetInfo_RSI_MAX()
 *
 * Test RateSelectionInfo::SetInfo()/GetInfo() w/ info id
 * RSI_MAX (invalid case). Set field RSI_MAX, then get it.
 *
 * Expected: HXR_INVALID_PARAMETER on both set and get. ulVal outparam not
 *           modified.
 *
 * \return          HXR_OK if test passed, HXR_FAIL if test failed.
 *****************************************************************************/

HX_RESULT
UTRateSelInfoDriver::Test_SetGetInfo_RSI_MAX()
{
    return Test_SetGetInfo_Invalid("Test_SetGetInfo_RSI_MAX()", 
                                       RSI_MAX);
}


/******************************************************************************
 * UTRateSelInfoDriver::Test_SetGetStrmInfo_Basic()
 *
 * Test basic RateSelectionInfo::SetInfo()/GetInfo() functionality for 
 * stream info (valid case). Set default rule for arbitrary stream, then get it.
 *
 * Expected: Successful set, successful get. Value retrieved equals
 *           value set.
 *
 * \return          HXR_OK if test passed, HXR_FAIL if test failed.
 *****************************************************************************/

HX_RESULT
UTRateSelInfoDriver::Test_SetGetStrmInfo_Basic()
{
    const static char* z_szTestName = "Test_SetGetStrmInfo_Basic()";
    RateSelectionInfo RateInfo;
    
    if (FAILED(RateInfo.SetInfo(RSI_DEFAULT_RULE, 200, 6)))
    {
        ReportFail(z_szTestName);
        return HXR_FAIL;
    }

    UINT32 ulVal = 0;
    if (FAILED(RateInfo.GetInfo(RSI_DEFAULT_RULE, 200, ulVal)) || ulVal != 6)
    {
        ReportFail(z_szTestName);
        return HXR_FAIL;
    }

    UINT16 RegisteredStreams[1];
    RegisteredStreams[0] = 0xFFFF;

    if (RateInfo.GetNumRegisteredStreams() != 1)
    {
        ReportFail(z_szTestName);
        return HXR_FAIL;
    }

    if (FAILED(RateInfo.GetRegisteredLogicalStreamIds(1, RegisteredStreams))
    ||  RegisteredStreams[0] != 200)
    {
        ReportFail(z_szTestName);
        return HXR_FAIL;
    }

    ReportPass(z_szTestName);

    return HXR_OK;
}


/******************************************************************************
 * UTRateSelInfoDriver::Test_SetGetStrmInfo_GetWithoutSetFromSetupStream()
 *
 * Test RateSelectionInfo::SetInfo()/GetInfo() where stream selection info is
 * created (stream is setup) with SetInfo() for a variable, but get a variable
 * that hasn't been set yet (valid test). Also get DEFAULT_RULE since that has
 * an expected value.
 *
 * It is valid to retrieve unassigned values from a stream if it is setup.
 *
 * Expected: Successful set, successful get. First value retrieved equals 0.
 * GetInfo() with id of RSI_DEFAULT_RULE yields value of INVALID_RULE_NUM.
 *
 * \return          HXR_OK if test passed, HXR_FAIL if test failed.
 *****************************************************************************/

HX_RESULT
UTRateSelInfoDriver::Test_SetGetStrmInfo_GetWithoutSetFromSetupStream()
{
    const static char* z_szTestName = "Test_SetGetStrmInfo_GetWithoutSetFromSetupStream()";
    RateSelectionInfo RateInfo;
    
    if (FAILED(RateInfo.SetInfo(RSI_STREAMGROUPID, 200, 100)))
    {
        ReportFail(z_szTestName);
        return HXR_FAIL;
    }

    UINT32 ulVal = 0;
    if (FAILED(RateInfo.GetInfo(RSI_AVGBITRATE, 200, ulVal)) || ulVal != 0)
    {
        ReportFail(z_szTestName);
        return HXR_FAIL;
    }

    ulVal = 0;
    if (FAILED(RateInfo.GetInfo(RSI_DEFAULT_RULE, 200, ulVal))
    ||  ulVal != INVALID_RULE_NUM)
    {
        ReportFail(z_szTestName);
        return HXR_FAIL;
    }

    UINT16 RegisteredStreams[1];
    RegisteredStreams[0] = 0xFFFF;

    if (RateInfo.GetNumRegisteredStreams() != 1)
    {
        ReportFail(z_szTestName);
        return HXR_FAIL;
    }

    if (FAILED(RateInfo.GetRegisteredLogicalStreamIds(1, RegisteredStreams))
    ||  RegisteredStreams[0] != 200)
    {
        ReportFail(z_szTestName);
        return HXR_FAIL;
    }

    ReportPass(z_szTestName);
    return HXR_FAIL;
}


/******************************************************************************
 * UTRateSelInfoDriver::Test_SetGetStrmInfo_GetFromWrongStream()
 *
 * Test RateSelectionInfo::SetInfo()/GetInfo() where GetInfo() is called for 
 * different stream than SetInfo() and stream provided to GetInfo() is not 
 * setup.
 *
 * Expected: Successful set, HXR_FAIL on get. ulVal is unchanged following get.
 *
 * \return          HXR_OK if test passed, HXR_FAIL if test failed.
 *****************************************************************************/

HX_RESULT
UTRateSelInfoDriver::Test_SetGetStrmInfo_GetFromWrongStream()
{
    const static char* z_szTestName = "Test_SetGetStrmInfo_GetFromWrongStream()";
    RateSelectionInfo RateInfo;
    
    if (FAILED(RateInfo.SetInfo(RSI_STREAMGROUPID, 200, 100)))
    {
        ReportFail(z_szTestName);
        return HXR_FAIL;
    }

    UINT32 ulVal = 12345;
    if ((RateInfo.GetInfo(RSI_STREAMGROUPID, 201, ulVal) != HXR_FAIL) 
    ||  ulVal != 12345)
    {
        ReportFail(z_szTestName);
        return HXR_FAIL;
    }

    UINT16 RegisteredStreams[1];
    RegisteredStreams[0] = 0xFFFF;

    if (RateInfo.GetNumRegisteredStreams() != 1)
    {
        ReportFail(z_szTestName);
        return HXR_FAIL;
    }

    if (FAILED(RateInfo.GetRegisteredLogicalStreamIds(1, RegisteredStreams))
    ||  RegisteredStreams[0] != 200)
    {
        ReportFail(z_szTestName);
        return HXR_FAIL;
    }

    ReportPass(z_szTestName);
    return HXR_FAIL;
}


/******************************************************************************
 * UTRateSelInfoDriver::Test_RuleSubscriptions_Basic()
 *
 * Test basic rule subscriptions. (valid test). Subscribe to rules 0, 1, 3 for
 * stream 100. Subscribe to rules 2, 3, 4 for stream 101. Unsubscribe from rule
 * 1 for stream 100. Unsubscribe from rule 3 for stream 101. 
 * GetNumSubscribedRules() for both streams, GetSubscribedRules() for both
 * streams.
 *
 * Expected: GetNumSubscribedRules() returns 2 for both streams. 
 * GetSubscribedRules() for stream 100 returns array containing {0, 3}.
 * GetSubscribedRules() for stream 101 returns array containing {2, 4}.
 *
 * \return          HXR_OK if test passed, HXR_FAIL if test failed.
 *****************************************************************************/

HX_RESULT
UTRateSelInfoDriver::Test_RuleSubscriptions_Basic()
{
    const static char* z_szTestName = "Test_StrmRuleSubscriptions_Basic()";

    RateSelectionInfo RateInfo;

    if (FAILED(RateInfo.SetInfo(RSI_SUBSCRIBE_RULE, 100, 0)))
    {
        ReportFail(z_szTestName);
        return HXR_FAIL;
    }

    if (FAILED(RateInfo.SetInfo(RSI_SUBSCRIBE_RULE, 100, 1)))
    {
        ReportFail(z_szTestName);
        return HXR_FAIL;
    }

    if (FAILED(RateInfo.SetInfo(RSI_SUBSCRIBE_RULE, 100, 3)))
    {
        ReportFail(z_szTestName);
        return HXR_FAIL;
    }

    if (FAILED(RateInfo.SetInfo(RSI_SUBSCRIBE_RULE, 101, 2)))
    {
        ReportFail(z_szTestName);
        return HXR_FAIL;
    }

    if (FAILED(RateInfo.SetInfo(RSI_SUBSCRIBE_RULE, 101, 4)))
    {
        ReportFail(z_szTestName);
        return HXR_FAIL;
    }

    if (FAILED(RateInfo.SetInfo(RSI_SUBSCRIBE_RULE, 101, 3)))
    {
        ReportFail(z_szTestName);
        return HXR_FAIL;
    }

    if (FAILED(RateInfo.SetInfo(RSI_UNSUBSCRIBE_RULE, 100, 1)))
    {
        ReportFail(z_szTestName);
        return HXR_FAIL;
    }

    if (FAILED(RateInfo.SetInfo(RSI_UNSUBSCRIBE_RULE, 101, 3)))
    {
        ReportFail(z_szTestName);
        return HXR_FAIL;
    }

    if ((RateInfo.GetNumSubscribedRules(100) != 2)
    ||  (RateInfo.GetNumSubscribedRules(101) != 2))
    {
        ReportFail(z_szTestName);
        return HXR_FAIL;
    }

    UINT16 Rules[2];
    memset(Rules, 0xFFFF, sizeof(UINT16) * 2);

    if (FAILED(RateInfo.GetSubscribedRules(100, 2, Rules)))
    {
        ReportFail(z_szTestName);
        return HXR_FAIL;
    }

    if ((Rules[0] != 0)
    ||  (Rules[1] != 3))
    {
        ReportFail(z_szTestName);
        
        return HXR_FAIL;
    }

    memset(Rules, 0xFFFF, sizeof(UINT16) * 2);

    if (FAILED(RateInfo.GetSubscribedRules(101, 2, Rules)))
    {
        ReportFail(z_szTestName);
        return HXR_FAIL;
    }

    if ((Rules[0] != 2)
    ||  (Rules[1] != 4))
    {
        ReportFail(z_szTestName);
        
        return HXR_FAIL;
    }

    UINT16 RegisteredStreams[2];
    memset(RegisteredStreams, 0xFFFF, sizeof(UINT16) * 2);

    if (RateInfo.GetNumRegisteredStreams() != 2)
    {
        ReportFail(z_szTestName);
        return HXR_FAIL;
    }

    if (FAILED(RateInfo.GetRegisteredLogicalStreamIds(2, RegisteredStreams))
    ||  RegisteredStreams[0] != 100
    ||  RegisteredStreams[1] != 101)
    {
        ReportFail(z_szTestName);
        return HXR_FAIL;
    }

    ReportPass(z_szTestName);    
    return HXR_OK;
}


/******************************************************************************
 * UTRateSelInfoDriver::Test_RuleSubscriptions_GetFromUnsetupStream()
 *
 * Attempt to get rule count and rule list from stream that has not been setup
 * (has no info object associated with it). 
 *
 * Expected: GetNumSubscribedRules() returns 0 for stream. GetSubscribedRules() 
 * returns HXR_FAIL. Array passed into GetSubscribedRules() is unchanged after
 * call.
 *
 * \return          HXR_OK if test passed, HXR_FAIL if test failed.
 *****************************************************************************/

HX_RESULT
UTRateSelInfoDriver::Test_RuleSubscriptions_GetFromUnsetupStream()
{
    const static char* z_szTestName = "Test_RuleSubscriptions_GetFromUnsetupStream()";
    RateSelectionInfo RateInfo;

    if ((RateInfo.GetNumSubscribedRules(100) != 0))
    {
        ReportFail(z_szTestName);
        return HXR_FAIL;
    }

    UINT16 Rules[2];
    memset(Rules, 0xFFFF, sizeof(UINT16) * 2);

    if ((RateInfo.GetSubscribedRules(100, 2, Rules) != HXR_FAIL)
    ||  (Rules[0] != 0xFFFF) || (Rules[1] != 0xFFFF))
    {
        ReportFail(z_szTestName);
        return HXR_FAIL;
    }

    UINT16 RegisteredStreams[1];
    RegisteredStreams[0] = 0xFFFF;

    if (RateInfo.GetNumRegisteredStreams() != 0)
    {
        ReportFail(z_szTestName);
        return HXR_FAIL;
    }

    if (FAILED(RateInfo.GetRegisteredLogicalStreamIds(1, RegisteredStreams))
    ||  RegisteredStreams[0] != 0xFFFF)
    {
        ReportFail(z_szTestName);
        return HXR_FAIL;
    }


    ReportPass(z_szTestName);    
    return HXR_OK;
}


/******************************************************************************
 * UTRateSelInfoDriver::Test_StrmSetGetInfo_Basic()
 *
 * Test basic StreamRateSelectionInfo::SetInfo()/GetInfo() functionality for 
 * stream info (valid case). Set Link-Char MBW, then get it.
 *
 * Expected: Successful set, successful get. Value retrieved equals
 *           value set.
 *
 * \return          HXR_OK if test passed, HXR_FAIL if test failed.
 *****************************************************************************/

HX_RESULT
UTRateSelInfoDriver::Test_StrmSetGetInfo_Basic()
{
    const static char* z_szTestName = "Test_StrmSetGetInfo_Basic()";
    StreamRateSelectionInfo StrmInfo;
    
    if (FAILED(StrmInfo.SetInfo(RSI_LINKCHAR_MBW, 150000)))
    {
        ReportFail(z_szTestName);
        return HXR_FAIL;
    }

    UINT32 ulVal = 0;
    if (SUCCEEDED(StrmInfo.GetInfo(RSI_LINKCHAR_MBW, ulVal)) && ulVal == 150000)
    {
        ReportPass(z_szTestName);
    }
    else
    {
        ReportFail(z_szTestName);
        return HXR_FAIL;
    }

    return HXR_OK;
}


/******************************************************************************
 * 
 *  Test RateSelectionInfo::SetInfo()/GetInfo() functionality for "track id"
 *  stream info (valid case). Try to get track id, then set it, then get it 
 *  again. Track id differs from basic test because it returns failure if not
 *  initially set.
 *
 *  Expected: First get returns HXR_FAIL. Set succeeds, subsequent get succeeds
 *  and returns correct value.
 *
 * \return          HXR_OK if test passed, HXR_FAIL if test failed.
 *****************************************************************************/

HX_RESULT
UTRateSelInfoDriver::Test_StrmSetGetInfo_RSI_TRACKID()
{
    const static char* z_szTestName = "Test_StrmSetGetInfo_RSI_TRACKID()";
    StreamRateSelectionInfo StrmInfo;
    
    UINT32 ulVal = 0;

    if (SUCCEEDED(StrmInfo.GetInfo(RSI_TRACKID, ulVal)) || ulVal)
    {
        ReportFail(z_szTestName);
        return HXR_FAIL;
    }

    if (FAILED(StrmInfo.SetInfo(RSI_TRACKID, 12345)))
    {
        ReportFail(z_szTestName);
        return HXR_FAIL;
    }

    if (SUCCEEDED(StrmInfo.GetInfo(RSI_TRACKID, ulVal)) && ulVal == 12345)
    {
        ReportPass(z_szTestName);
    }
    else
    {
        ReportFail(z_szTestName);
        return HXR_FAIL;
    }

    return HXR_OK;
}


/******************************************************************************
 * UTRateSelInfoDriver::Test_StrmSetGetInfo_Invalid()
 *
 * Test StreamRateSelectionInfo::SetInfo()/GetInfo() invalid cases. Used by 
 * tests which set and get invalid id fields.
 *
 * Expected: HXR_INVALID_PARAMETER on both set and get. ulVal outparam not
 *           modified.
 *
 * \return          HXR_OK if test passed, HXR_FAIL if test failed.
 *****************************************************************************/

HX_RESULT
UTRateSelInfoDriver::Test_StrmSetGetInfo_Invalid(const char* szTestName,
                                                 RateSelInfoId Id)
{
    StreamRateSelectionInfo StrmInfo;    
    
    if (StrmInfo.SetInfo(Id, 67890) != HXR_INVALID_PARAMETER)
    {
        ReportFail(szTestName);
        return HXR_FAIL;
    }

    UINT32 ulVal = 12345;
    if ((StrmInfo.GetInfo(Id, ulVal) == HXR_INVALID_PARAMETER)
    &&  ulVal == 12345)
    {
        ReportPass(szTestName);
    }
    else
    {
        ReportFail(szTestName);
        return HXR_FAIL;
    }

    return HXR_OK;
}


/******************************************************************************
 * UTRateSelInfoDriver::Test_StrmSetGetInfo_RSI_NONE()
 *
 * Test StreamRateSelectionInfo::SetInfo()/GetInfo() w/ info id
 * RSI_NONE (invalid case). Set field RSI_NONE, then get it.
 *
 * Expected: HXR_INVALID_PARAMETER on both set and get. ulVal outparam not
 *           modified.
 *
 * \return          HXR_OK if test passed, HXR_FAIL if test failed.
 *****************************************************************************/

HX_RESULT
UTRateSelInfoDriver::Test_StrmSetGetInfo_RSI_NONE()
{
    return Test_StrmSetGetInfo_Invalid("Test_StrmSetGetInfo_RSI_NONE()", RSI_NONE);
}


/******************************************************************************
 * UTRateSelInfoDriver::Test_StrmSetGetInfo_RSI_BANDWIDTH()
 *
 * Test StreamRateSelectionInfo::SetInfo()/GetInfo() w/ info id
 * RSI_BANDWIDTH (invalid case). Set field RSI_BANDWIDTH, then get it.
 *
 * Expected: HXR_INVALID_PARAMETER on both set and get. ulVal outparam not
 *           modified.
 *
 * \return          HXR_OK if test passed, HXR_FAIL if test failed.
 *****************************************************************************/

HX_RESULT
UTRateSelInfoDriver::Test_StrmSetGetInfo_RSI_BANDWIDTH()
{
    return Test_StrmSetGetInfo_Invalid("Test_StrmSetGetInfo_RSI_BANDWIDTH()", 
                                       RSI_BANDWIDTH);
}


/******************************************************************************
 * UTRateSelInfoDriver::Test_StrmSetGetInfo_RSI_SDB()
 *
 * Test StreamRateSelectionInfo::SetInfo()/GetInfo() w/ info id
 * RSI_SDB (invalid case). Set field RSI_SDB, then get it.
 *
 * Expected: HXR_INVALID_PARAMETER on both set and get. ulVal outparam not
 *           modified.
 *
 * \return          HXR_OK if test passed, HXR_FAIL if test failed.
 ******************************************************************************/

HX_RESULT
UTRateSelInfoDriver::Test_StrmSetGetInfo_RSI_SDB()
{
    return Test_StrmSetGetInfo_Invalid("Test_StrmSetGetInfo_RSI_SDB()", 
                                       RSI_SDB);
}


/******************************************************************************
 * UTRateSelInfoDriver::Test_StrmSetGetInfo_RSI_MAX()
 *
 * Test StreamRateSelectionInfo::SetInfo()/GetInfo() w/ info id
 * RSI_MAX (invalid case). Set field RSI_MAX, then get it.
 *
 * Expected: HXR_INVALID_PARAMETER on both set and get. ulVal outparam not
 *           modified.
 *
 * \return          HXR_OK if test passed, HXR_FAIL if test failed.
 *****************************************************************************/

HX_RESULT
UTRateSelInfoDriver::Test_StrmSetGetInfo_RSI_MAX()
{
    return Test_StrmSetGetInfo_Invalid("Test_StrmSetGetInfo_RSI_MAX()", 
                                       RSI_MAX);
}


/******************************************************************************
 * UTRateSelInfoDriver::Test_StrmRuleSubscriptions_Basic()
 *
 * Test basic rule subscriptions. (valid test). Subscribe to rules 0, 1, 3, 4. 
 * Unsubscribe from rule 3. Then GetNumSubscribedRules(), then 
 * GetSubscribedRules().
 *
 * Expected: GetNumSubscribedRules() returns 3. GetSubscribedRules() returns 
 * array containing {0, 1, 4}.
 *
 * \return          HXR_OK if test passed, HXR_FAIL if test failed.
 *****************************************************************************/

HX_RESULT
UTRateSelInfoDriver::Test_StrmRuleSubscriptions_Basic()
{
    const static char* z_szTestName = "Test_StrmRuleSubscriptions_Basic()";

    StreamRateSelectionInfo StrmInfo;

    if (FAILED(StrmInfo.SetInfo(RSI_SUBSCRIBE_RULE, 0)))
    {
        ReportFail(z_szTestName);
        return HXR_FAIL;
    }

    if (FAILED(StrmInfo.SetInfo(RSI_SUBSCRIBE_RULE, 1)))
    {
        ReportFail(z_szTestName);
        return HXR_FAIL;
    }

    if (FAILED(StrmInfo.SetInfo(RSI_SUBSCRIBE_RULE, 3)))
    {
        ReportFail(z_szTestName);
        return HXR_FAIL;
    }

    if (FAILED(StrmInfo.SetInfo(RSI_SUBSCRIBE_RULE, 4)))
    {
        ReportFail(z_szTestName);
        return HXR_FAIL;
    }

    if (FAILED(StrmInfo.SetInfo(RSI_UNSUBSCRIBE_RULE, 3)))
    {
        ReportFail(z_szTestName);
        return HXR_FAIL;
    }

    if (StrmInfo.GetNumSubscribedRules() != 3)
    {
        ReportFail(z_szTestName);
        return HXR_FAIL;
    }

    UINT16 Rules[3];
    memset(Rules, 0xFFFF, 3);

    if (FAILED(StrmInfo.GetSubscribedRules(3, Rules)))
    {
        ReportFail(z_szTestName);        
        return HXR_FAIL;
    }

    if ((Rules[0] != 0)
    ||  (Rules[1] != 1)
    ||  (Rules[2] != 4))
    {
        ReportFail(z_szTestName);        
        return HXR_FAIL;
    }

    ReportPass(z_szTestName);    
    return HXR_OK;
}


/******************************************************************************
 * UTRateSelInfoDriver::Test_StrmRuleSubscriptions_Empty()
 *
 * Test getting rule subscriptions when no rules subscribed (valid test). 
 * GetNumSubscribedRules(), then GetSubscribedRules().
 *
 * Expected: GetNumSubscribedRules() returns 0. GetSubscribedRules() returns 
 * empty array.
 *
 * \return          HXR_OK if test passed, HXR_FAIL if test failed.
 *****************************************************************************/

HX_RESULT
UTRateSelInfoDriver::Test_StrmRuleSubscriptions_Empty()
{
    const static char* z_szTestName = "Test_StrmRuleSubscriptions_Empty()";

    StreamRateSelectionInfo StrmInfo;

    if (StrmInfo.GetNumSubscribedRules() != 0)
    {
        ReportFail(z_szTestName);
        return HXR_FAIL;
    }

    UINT16 Rules[3];
    memset(Rules, 0xFFFF, sizeof(UINT16) * 3);

    if (FAILED(StrmInfo.GetSubscribedRules(3, Rules)))
    {
        ReportFail(z_szTestName);
        return HXR_FAIL;
    }

    for (UINT16 i = 0; i < 3; i++)
    {
        if (Rules[i] != 0xFFFF)
        {            
            ReportFail(z_szTestName);
            
            return HXR_FAIL;
        }
    }

    ReportPass(z_szTestName);
    
    return HXR_OK;
}


/******************************************************************************
 * UTRateSelInfoDriver::Test_StrmRuleSubscriptions_BigArray()
 *
 * Test rule subscriptions. (valid test). Subscribe to rules 0, 1, 3, 4. 
 * Unsubscribe from rule 3. Then GetNumSubscribedRules(), then 
 * GetSubscribedRules() with an indicated array size larger than total number 
 * of subscribed rules (for this case, 5 elements).
 *
 * Expected: GetNumSubscribedRules() returns 3. GetSubscribedRules() returns 
 * array containing {0, 1, 4, 0xFFFF, 0xFFFF}.
 *
 * \return          HXR_OK if test passed, HXR_FAIL if test failed.
 *****************************************************************************/

HX_RESULT
UTRateSelInfoDriver::Test_StrmRuleSubscriptions_BigArray()
{
    const static char* z_szTestName = "Test_StrmRuleSubscriptions_BigArray()";

    StreamRateSelectionInfo StrmInfo;

    if (FAILED(StrmInfo.SetInfo(RSI_SUBSCRIBE_RULE, 0)))
    {
        ReportFail(z_szTestName);
        return HXR_FAIL;
    }

    if (FAILED(StrmInfo.SetInfo(RSI_SUBSCRIBE_RULE, 1)))
    {
        ReportFail(z_szTestName);
        return HXR_FAIL;
    }

    if (FAILED(StrmInfo.SetInfo(RSI_SUBSCRIBE_RULE, 3)))
    {
        ReportFail(z_szTestName);
        return HXR_FAIL;
    }

    if (FAILED(StrmInfo.SetInfo(RSI_SUBSCRIBE_RULE, 4)))
    {
        ReportFail(z_szTestName);
        return HXR_FAIL;
    }

    if (FAILED(StrmInfo.SetInfo(RSI_UNSUBSCRIBE_RULE, 3)))
    {
        ReportFail(z_szTestName);
        return HXR_FAIL;
    }

    if (StrmInfo.GetNumSubscribedRules() != 3)
    {
        ReportFail(z_szTestName);
        return HXR_FAIL;
    }

    UINT16 Rules[5];
    memset(Rules, 0xFFFF, sizeof(UINT16) * 5);

    if (FAILED(StrmInfo.GetSubscribedRules(5, Rules)))
    {
        ReportFail(z_szTestName);
        return HXR_FAIL;
    }

    if ((Rules[0] != 0)
    ||  (Rules[1] != 1)
    ||  (Rules[2] != 4)
    ||  (Rules[3] != 0xFFFF)
    ||  (Rules[4] != 0xFFFF))
    {
        ReportFail(z_szTestName);
        
        return HXR_FAIL;
    }

    ReportPass(z_szTestName);
    
    return HXR_OK;
}


/******************************************************************************
 * UTRateSelInfoDriver::Test_StrmRuleSubscriptions_SmallArray()
 *
 * Test rule subscriptions. (valid test). Subscribe to rules 0, 1, 3, 4. 
 * Unsubscribe from rule 3. Then GetNumSubscribedRules(), then 
 * GetSubscribedRules() with an indicated array size smaller than total number 
 * of subscribed rules (for this case, 2 elements).
 *
 * Expected: GetNumSubscribedRules() returns 3. GetSubscribedRules() returns 
 * array containing {0, 1, 0xFFFF}.
 *
 * \return          HXR_OK if test passed, HXR_FAIL if test failed.
 *****************************************************************************/

HX_RESULT
UTRateSelInfoDriver::Test_StrmRuleSubscriptions_SmallArray()
{
    const static char* z_szTestName = "Test_StrmRuleSubscriptions_SmallArray()";

    StreamRateSelectionInfo StrmInfo;

    if (FAILED(StrmInfo.SetInfo(RSI_SUBSCRIBE_RULE, 0)))
    {
        ReportFail(z_szTestName);
        return HXR_FAIL;
    }

    if (FAILED(StrmInfo.SetInfo(RSI_SUBSCRIBE_RULE, 1)))
    {
        ReportFail(z_szTestName);
        return HXR_FAIL;
    }

    if (FAILED(StrmInfo.SetInfo(RSI_SUBSCRIBE_RULE, 3)))
    {
        ReportFail(z_szTestName);
        return HXR_FAIL;
    }

    if (FAILED(StrmInfo.SetInfo(RSI_SUBSCRIBE_RULE, 4)))
    {
        ReportFail(z_szTestName);
        return HXR_FAIL;
    }

    if (FAILED(StrmInfo.SetInfo(RSI_UNSUBSCRIBE_RULE, 3)))
    {
        ReportFail(z_szTestName);
        return HXR_FAIL;
    }

    if (StrmInfo.GetNumSubscribedRules() != 3)
    {
        ReportFail(z_szTestName);
        return HXR_FAIL;
    }

    UINT16 Rules[3];
    memset(Rules, 0xFFFF, sizeof(UINT16) * 3);

    if (FAILED(StrmInfo.GetSubscribedRules(2, Rules)))
    {
        ReportFail(z_szTestName);
        return HXR_FAIL;
    }

    if ((Rules[0] != 0)
    ||  (Rules[1] != 1)
    ||  (Rules[2] != 0xFFFF))
    {
        ReportFail(z_szTestName);
        
        return HXR_FAIL;
    }

    ReportPass(z_szTestName);
    
    return HXR_OK;
}


/******************************************************************************
 * UTRateSelInfoDriver::Test_StrmRuleSubscriptions_ZeroSizeArray()
 *
 * Test rule subscriptions. (valid test). Subscribe to rules 0, 1, 3, 4. 
 * Unsubscribe from rule 3. Then GetNumSubscribedRules(), then 
 * GetSubscribedRules() with an indicated array size if 0.
 *
 * Expected: GetNumSubscribedRules() returns 3. GetSubscribedRules() returns 
 * array containing {0xFFFF, 0xFFFF, 0xFFFF}.
 *
 * \return          HXR_OK if test passed, HXR_FAIL if test failed.
 *****************************************************************************/

HX_RESULT
UTRateSelInfoDriver::Test_StrmRuleSubscriptions_ZeroSizeArray()
{
    const static char* z_szTestName = "Test_StrmRuleSubscriptions_ZeroSizeArray()";

    StreamRateSelectionInfo StrmInfo;

    if (FAILED(StrmInfo.SetInfo(RSI_SUBSCRIBE_RULE, 0)))
    {
        ReportFail(z_szTestName);
        return HXR_FAIL;
    }

    if (FAILED(StrmInfo.SetInfo(RSI_SUBSCRIBE_RULE, 1)))
    {
        ReportFail(z_szTestName);
        return HXR_FAIL;
    }

    if (FAILED(StrmInfo.SetInfo(RSI_SUBSCRIBE_RULE, 3)))
    {
        ReportFail(z_szTestName);
        return HXR_FAIL;
    }

    if (FAILED(StrmInfo.SetInfo(RSI_SUBSCRIBE_RULE, 4)))
    {
        ReportFail(z_szTestName);
        return HXR_FAIL;
    }

    if (FAILED(StrmInfo.SetInfo(RSI_UNSUBSCRIBE_RULE, 3)))
    {
        ReportFail(z_szTestName);
        return HXR_FAIL;
    }

    if (StrmInfo.GetNumSubscribedRules() != 3)
    {
        ReportFail(z_szTestName);
        return HXR_FAIL;
    }

    UINT16 Rules[3];
    memset(Rules, 0xFFFF, sizeof(UINT16) * 3);

    if (FAILED(StrmInfo.GetSubscribedRules(0, Rules)))
    {
        ReportFail(z_szTestName);
        return HXR_FAIL;
    }

    for (UINT16 i = 0; i < 3; i++)
    {
        if (Rules[i] != 0xFFFF)
        {
            ReportFail(z_szTestName);
            return HXR_FAIL;
        }
    }

    ReportPass(z_szTestName);
    return HXR_OK;
}


/******************************************************************************
 * UTRateSelInfoDriver::Test_StrmRuleSubscriptions_DoubleSubscribe()
 *
 * Test rule subscriptions. (invalid test)
 * Subscribe to rule 1, then subscribe to rule 1 again.
 * Then GetNumSubscribedRules, then GetSubscribedRules().
 *
 * Expected: Second subscription to rule 1 fails. GetNumSubscribedRules() 
 *           returns 1. GetSubscribedRules() returns array containing {1}.
 *
 * \return          HXR_OK if test passed, HXR_FAIL if test failed.
 *****************************************************************************/

HX_RESULT
UTRateSelInfoDriver::Test_StrmRuleSubscriptions_DoubleSubscribe()
{
    const static char* z_szTestName = "Test_StrmRuleSubscriptions_DoubleSubscribe()";

    StreamRateSelectionInfo StrmInfo;

    if (FAILED(StrmInfo.SetInfo(RSI_SUBSCRIBE_RULE, 1)))
    {
        ReportFail(z_szTestName);
        return HXR_FAIL;
    }

    if (StrmInfo.SetInfo(RSI_SUBSCRIBE_RULE, 1) != HXR_UNEXPECTED)
    {
        ReportFail(z_szTestName);
        return HXR_FAIL;
    }

    if (StrmInfo.GetNumSubscribedRules() != 1)
    {
        ReportFail(z_szTestName);
        return HXR_FAIL;
    }

    UINT16 Rules[1];
    memset(Rules, 0xFFFF, sizeof(UINT16));

    if (FAILED(StrmInfo.GetSubscribedRules(1, Rules)))
    {
        ReportFail(z_szTestName);
        
        return HXR_FAIL;
    }

    if (Rules[0] != 1)
    {
        ReportFail(z_szTestName);
        
        return HXR_FAIL;
    }

    ReportPass(z_szTestName);
    
    return HXR_OK;
}

/******************************************************************************
 * UTRateSelInfoDriver::Test_StrmRuleSubscriptions_DoubleUnsubscribe()
 *
 * Test rule subscriptions. (invalid test)
 * Subscribe to rules 0, 1, 3, 4. Unsubscribe from rule 3 twice. 
 * Then GetNumSubscribedRules, then GetSubscribedRules().
 *
 * Expected: Second unsubscribe returns HXR_FAIL. GetNumSubscribedRules() 
 *           returns 3. GetSubscribedRules() returns array containing {0, 1, 4}.
 *
 * \return          HXR_OK if test passed, HXR_FAIL if test failed.
 *****************************************************************************/

HX_RESULT
UTRateSelInfoDriver::Test_StrmRuleSubscriptions_DoubleUnsubscribe()
{
    const static char* z_szTestName = "Test_StrmRuleSubscriptions_DoubleUnsubscribe()";

    StreamRateSelectionInfo StrmInfo;

    if (FAILED(StrmInfo.SetInfo(RSI_SUBSCRIBE_RULE, 0)))
    {
        ReportFail(z_szTestName);
        return HXR_FAIL;
    }

    if (FAILED(StrmInfo.SetInfo(RSI_SUBSCRIBE_RULE, 1)))
    {
        ReportFail(z_szTestName);
        return HXR_FAIL;
    }

    if (FAILED(StrmInfo.SetInfo(RSI_SUBSCRIBE_RULE, 3)))
    {
        ReportFail(z_szTestName);
        return HXR_FAIL;
    }

    if (FAILED(StrmInfo.SetInfo(RSI_SUBSCRIBE_RULE, 4)))
    {
        ReportFail(z_szTestName);
        return HXR_FAIL;
    }

    if (FAILED(StrmInfo.SetInfo(RSI_UNSUBSCRIBE_RULE, 3)))
    {
        ReportFail(z_szTestName);
        return HXR_FAIL;
    }

    if (StrmInfo.GetNumSubscribedRules() != 3)
    {
        ReportFail(z_szTestName);
        return HXR_FAIL;
    }

    UINT16 Rules[3];
    memset(Rules, 0xFFFF, sizeof(UINT16) * 3);

    if (FAILED(StrmInfo.GetSubscribedRules(3, Rules)))
    {
        ReportFail(z_szTestName);
        return HXR_FAIL;
    }

    if ((Rules[0] != 0)
    ||  (Rules[1] != 1)
    ||  (Rules[2] != 4))
    {
        ReportFail(z_szTestName);
        
        return HXR_FAIL;
    }

    ReportPass(z_szTestName);
    
    return HXR_OK;
}


