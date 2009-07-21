/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: thxurlrep.h,v 1.3 2004/12/15 21:24:30 liam_murray Exp $
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
 
#ifndef URL_REP_TEST_H
#define URL_REP_TEST_H

#include "hxurlrep.h"

#include "hx_cmd_based_test.h"

class HXURLRepTest : public HLXCmdBasedTest
{
public:
    HXURLRepTest();
    virtual ~HXURLRepTest();

    virtual const char* DefaultCommandLine() const;
    virtual void GetCommandInfo(UTVector<HLXUnitTestCmdInfo*>& cmds);
    virtual HLXCmdBasedTest* Clone() const;

protected:
    bool HandleConstructor1Cmd(const UTVector<UTString>& params);
    bool HandleConstructor2Cmd(const UTVector<UTString>& params);
    bool HandleConstructor3Cmd(const UTVector<UTString>& params);
    bool HandleEqualCmd(const UTVector<UTString>& params);
    bool HandleNotEqualCmd(const UTVector<UTString>& params);
    bool HandleStringCmd(const UTVector<UTString>& params);
    bool HandleCheckStateCmd(const UTVector<UTString>& params);
    bool HandlePathCmd(const UTVector<UTString>& params);
    bool HandleQueryCmd(const UTVector<UTString>& params);
    bool HandleSchemeCmd(const UTVector<UTString>& params);
    bool HandleUserInfoCmd(const UTVector<UTString>& params);
    bool HandleApplyRelativeCmd(const UTVector<UTString>& params);
    bool HandleIsValidCmd(const UTVector<UTString>& params);
    bool HandleIsFullyParsedCmd(const UTVector<UTString>& params);
    bool HandleNormalizeCmd(const UTVector<UTString>& params);
    bool HandlePathOffsetCmd(const UTVector<UTString>& params);


private:
    HXURLRep m_url;
};

#endif // URL_REP_TEST_H