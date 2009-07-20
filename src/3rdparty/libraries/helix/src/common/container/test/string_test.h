/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: string_test.h,v 1.5 2007/07/06 20:35:03 jfinnecy Exp $
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

#ifndef STRING_TEST_H
#define STRING_TEST_H

#include "hx_cmd_based_test.h"

#include "hxstring.h"

class HLXStringTest : public HLXCmdBasedTest
{
public:
    HLXStringTest();
    ~HLXStringTest();

    virtual const char* DefaultCommandLine() const;    
    virtual void GetCommandInfo(UTVector<HLXUnitTestCmdInfo*>& cmds);
    virtual HLXCmdBasedTest* Clone() const;

protected:
    bool HandleConstruct1Cmd(const UTVector<UTString>& info);
    bool HandleConstruct2Cmd(const UTVector<UTString>& info);
    bool HandleConstruct3Cmd(const UTVector<UTString>& info);
    bool HandleConstruct4Cmd(const UTVector<UTString>& info);
    bool HandleConstruct5Cmd(const UTVector<UTString>& info);
    bool HandleConstruct6Cmd(const UTVector<UTString>& info);

    bool HandleGetLengthCmd(const UTVector<UTString>& info);
    bool HandleIsEmptyCmd(const UTVector<UTString>& info);
    bool HandleEmptyCmd(const UTVector<UTString>& info);

    bool HandleCharStarOpCmd(const UTVector<UTString>& info);

    bool HandleGetAtCmd(const UTVector<UTString>& info);
    bool HandleSetAtCmd(const UTVector<UTString>& info);

    bool HandleSubscriptCmd(const UTVector<UTString>& info);

    bool HandleAssignOp1Cmd(const UTVector<UTString>& info);
    bool HandleAssignOp2Cmd(const UTVector<UTString>& info);
    bool HandleAssignOp3Cmd(const UTVector<UTString>& info);
    bool HandleAssignOp4Cmd(const UTVector<UTString>& info);

    bool HandleAppendTo1Cmd(const UTVector<UTString>& info);
    bool HandleAppendTo2Cmd(const UTVector<UTString>& info);
    bool HandleAppendTo3Cmd(const UTVector<UTString>& info);

    bool HandleAdd1Cmd(const UTVector<UTString>& info);
    bool HandleAdd2Cmd(const UTVector<UTString>& info);
    bool HandleAdd3Cmd(const UTVector<UTString>& info);
    bool HandleAdd4Cmd(const UTVector<UTString>& info);
    bool HandleAdd5Cmd(const UTVector<UTString>& info);

    bool HandleCenterCmd(const UTVector<UTString>& info);

    bool HandleCompareCmd(const UTVector<UTString>& info);
    bool HandleCompareNoCaseCmd(const UTVector<UTString>& info);

    bool HandleMidCmd(const UTVector<UTString>& info);
    bool HandleLeftCmd(const UTVector<UTString>& info);
    bool HandleRightCmd(const UTVector<UTString>& info);

    bool HandleCountFieldsCmd(const UTVector<UTString>& info);
    bool HandleResetNthFieldStateCmd(const UTVector<UTString>& info);
    bool HandleGetNthFieldCmd(const UTVector<UTString>& info);
    bool HandleNthFieldCmd(const UTVector<UTString>& info);

    bool HandleSpanIncludingCmd(const UTVector<UTString>& info);
    bool HandleSpanExcludingCmd(const UTVector<UTString>& info);

    bool HandleMakeUpperCmd(const UTVector<UTString>& info);
    bool HandleMakeLowerCmd(const UTVector<UTString>& info);

    bool HandleTrimRightCmd(const UTVector<UTString>& info);
    bool HandleTrimLeftCmd(const UTVector<UTString>& info);

    bool HandleFind1Cmd(const UTVector<UTString>& info);
    bool HandleFind2Cmd(const UTVector<UTString>& info);
    bool HandleReverseFindCmd(const UTVector<UTString>& info);
    bool HandleFindAndReplaceCmd(const UTVector<UTString>& info);

    bool HandleFormatIntCmd(const UTVector<UTString>& info);
    bool HandleFormatLongCmd(const UTVector<UTString>& info);
    bool HandleFormatUIntCmd(const UTVector<UTString>& info);
    bool HandleFormatULongCmd(const UTVector<UTString>& info);
    bool HandleFormatCharCmd(const UTVector<UTString>& info);
    bool HandleFormatCharStarCmd(const UTVector<UTString>& info);
    bool HandleFormatPtrCmd(const UTVector<UTString>& info);
    bool HandleFormatDoubleCmd(const UTVector<UTString>& info);
    bool HandleFormatMixedCmd(const UTVector<UTString>& info);

    bool HandleAppendULONGCmd(const UTVector<UTString>& info);

    bool HandleAppendEndOfLineCmd(const UTVector<UTString>& info);

    bool HandleGetBufferCmd(const UTVector<UTString>& info);
    bool HandleGetBufferSetLengthCmd(const UTVector<UTString>& info);
    bool HandleBufferSetCmd(const UTVector<UTString>& info);
    bool HandleBufferFillCmd(const UTVector<UTString>& info);
    bool HandleBufferEndStringCmd(const UTVector<UTString>& info);
    bool HandleReleaseBufferCmd(const UTVector<UTString>& info);

    bool HandleFreeExtraCmd(const UTVector<UTString>& info);

    bool HandleGetAllocLengthCmd(const UTVector<UTString>& info);
    bool HandleSetMinBufSizeCmd(const UTVector<UTString>& info);

private:
    CHXString* GetStringObj(const UTString& strName);
    void SetStringObj(const UTString& strName, CHXString* pNewStr);

    CHXString*  m_pA;
    CHXString*  m_pB;

    UINT64      m_ANthFieldState;
    UINT64      m_BNthFieldState;

    char*       m_pABuffer;
    char*       m_pBBuffer;
};

#endif // STRING_TEST_H
