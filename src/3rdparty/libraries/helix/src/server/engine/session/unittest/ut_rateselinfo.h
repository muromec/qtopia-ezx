/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: ut_rateselinfo.h,v 1.2 2007/05/01 18:17:22 darrick Exp $
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

#ifndef _UT_RATE_SEL_INFO_H_
#define _UT_RATE_SEL_INFO_H_

#include "ut_basedriver.h"

class UTRateSelInfoDriver : public UTBaseDriver
{
public:

    UTRateSelInfoDriver();

    virtual HX_RESULT Run();

    // Unit tests for RateSelectionInfo.

    HX_RESULT Test_SetGetInfo_Basic();

    HX_RESULT Test_SetGetInfo_RSI_NONE();
    HX_RESULT Test_SetGetInfo_RSI_DEFAULT_RULE();
    HX_RESULT Test_SetGetInfo_RSI_AVGBITRATE();
    HX_RESULT Test_SetGetInfo_RSI_STREAMGROUPID();
    HX_RESULT Test_SetGetInfo_RSI_SUBSCRIBE_RULE();
    HX_RESULT Test_SetGetInfo_RSI_UNSUBSCRIBE_RULE();
    HX_RESULT Test_SetGetInfo_RSI_MAX();

    HX_RESULT Test_SetGetStrmInfo_Basic();

    HX_RESULT Test_SetGetStrmInfo_GetWithoutSetFromSetupStream();
    HX_RESULT Test_SetGetStrmInfo_GetFromWrongStream();
    
    HX_RESULT Test_RuleSubscriptions_Basic();
    HX_RESULT Test_RuleSubscriptions_GetFromUnsetupStream();


    // Unit tests for StreamRateSelectionInfo.

    HX_RESULT Test_StrmSetGetInfo_Basic();
    HX_RESULT Test_StrmSetGetInfo_RSI_TRACKID();

    HX_RESULT Test_StrmSetGetInfo_RSI_NONE();
    HX_RESULT Test_StrmSetGetInfo_RSI_BANDWIDTH();
    HX_RESULT Test_StrmSetGetInfo_RSI_SDB();
    HX_RESULT Test_StrmSetGetInfo_RSI_MAX();

    HX_RESULT Test_StrmRuleSubscriptions_Basic();

    HX_RESULT Test_StrmRuleSubscriptions_Empty();
    HX_RESULT Test_StrmRuleSubscriptions_DoubleSubscribe();
    HX_RESULT Test_StrmRuleSubscriptions_DoubleUnsubscribe();
    HX_RESULT Test_StrmRuleSubscriptions_BigArray();
    HX_RESULT Test_StrmRuleSubscriptions_SmallArray();
    HX_RESULT Test_StrmRuleSubscriptions_ZeroSizeArray();



protected:

    HX_RESULT Test_SetGetInfo_Invalid(const char* szTestName,
                                      RateSelInfoId Id);

    HX_RESULT Test_SetGetStrmInfo_Invalid(const char* szTestName,
                                          RateSelInfoId Id);

    HX_RESULT Test_StrmSetGetInfo_Invalid(const char* szTestName,
                                          RateSelInfoId Id);

};

#endif //defined _UT_RATE_SEL_INFO_H_
