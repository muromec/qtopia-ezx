/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: string_test.cpp,v 1.11 2007/07/06 20:35:03 jfinnecy Exp $
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

#include "./string_test.h"

#include "hx_ut_debug.h"
#include "ut_param_util.h"

#include "./param_util.h"

HLXStringTest::HLXStringTest() :
    m_pA(0),
    m_pB(0),
    m_ANthFieldState(0),
    m_BNthFieldState(0),
    m_pABuffer(0),
    m_pBBuffer(0)

{}

HLXStringTest::~HLXStringTest()
{
    delete m_pA;
    delete m_pB;
}

const char* HLXStringTest::DefaultCommandLine() const
{
    return "tstring tstring.in";
}
    
void HLXStringTest::GetCommandInfo(UTVector<HLXUnitTestCmdInfo*>& cmds)
{
    cmds.Resize(65);

    int i = 0;

    HX_ASSERT(i < cmds.Nelements());
    cmds[i++] = new HLXUnitTestCmdInfoDisp<HLXStringTest>(
        this, 
        "CHXString()",
        &HLXStringTest::HandleConstruct1Cmd,
        2);

    HX_ASSERT(i < cmds.Nelements());
    cmds[i++] = new HLXUnitTestCmdInfoDisp<HLXStringTest>(
        this, 
        "CHXString(CHXString)",
        &HLXStringTest::HandleConstruct2Cmd,
        3);

    HX_ASSERT(i < cmds.Nelements());
    cmds[i++] = new HLXUnitTestCmdInfoDisp<HLXStringTest>(
        this, 
        "CHXString(char,int)",
        &HLXStringTest::HandleConstruct3Cmd,
        4);

    HX_ASSERT(i < cmds.Nelements());
    cmds[i++] = new HLXUnitTestCmdInfoDisp<HLXStringTest>(
        this, 
        "CHXString(char*)",
        &HLXStringTest::HandleConstruct4Cmd,
        3);

    HX_ASSERT(i < cmds.Nelements());
    cmds[i++] = new HLXUnitTestCmdInfoDisp<HLXStringTest>(
        this, 
        "CHXString(char*,int)",
        &HLXStringTest::HandleConstruct5Cmd,
        4);

    HX_ASSERT(i < cmds.Nelements());
    cmds[i++] = new HLXUnitTestCmdInfoDisp<HLXStringTest>(
        this, 
        "CHXString(uchar*)",
        &HLXStringTest::HandleConstruct6Cmd,
        3);

    HX_ASSERT(i < cmds.Nelements());
    cmds[i++] = new HLXUnitTestCmdInfoDisp<HLXStringTest>(
        this, 
        "GetLength",
        &HLXStringTest::HandleGetLengthCmd,
        3);

    HX_ASSERT(i < cmds.Nelements());
    cmds[i++] = new HLXUnitTestCmdInfoDisp<HLXStringTest>(
        this, 
        "IsEmpty",
        &HLXStringTest::HandleIsEmptyCmd,
        3);

    HX_ASSERT(i < cmds.Nelements());
    cmds[i++] = new HLXUnitTestCmdInfoDisp<HLXStringTest>(
        this, 
        "Empty",
        &HLXStringTest::HandleEmptyCmd,
        2);

    HX_ASSERT(i < cmds.Nelements());
    cmds[i++] = new HLXUnitTestCmdInfoDisp<HLXStringTest>(
        this, 
        "(char*)",
        &HLXStringTest::HandleCharStarOpCmd,
        3,999);                 // (char*) A|B <expected> [... <alternate returns>]

    HX_ASSERT(i < cmds.Nelements());
    cmds[i++] = new HLXUnitTestCmdInfoDisp<HLXStringTest>(
        this,
        "GetAt",
        &HLXStringTest::HandleGetAtCmd,
        4);                     // GetAt A|B <index> <expected char return>

    HX_ASSERT(i < cmds.Nelements());
    cmds[i++] = new HLXUnitTestCmdInfoDisp<HLXStringTest>(
        this,
        "SetAt",
        &HLXStringTest::HandleSetAtCmd,
        4);                     // SetAt A|B <index> <char>


    HX_ASSERT(i < cmds.Nelements());
    cmds[i++] = new HLXUnitTestCmdInfoDisp<HLXStringTest>(
        this,
        "[]",
        &HLXStringTest::HandleSubscriptCmd,
        4);                     // [] A|B <index> <expected char return>


    // XXXSAB: don't forget to verify that return value ref is same as A|B
    //         on all these assignment ops.

    HX_ASSERT(i < cmds.Nelements());
    cmds[i++] = new HLXUnitTestCmdInfoDisp<HLXStringTest>(
        this,
        "=CHXString",
        &HLXStringTest::HandleAssignOp1Cmd,
        3);                     // =CHXString A|B <from string: A|B>

    HX_ASSERT(i < cmds.Nelements());
    cmds[i++] = new HLXUnitTestCmdInfoDisp<HLXStringTest>(
        this,
        "=char",
        &HLXStringTest::HandleAssignOp2Cmd,
        3);                     // =char A|B <from char>

    HX_ASSERT(i < cmds.Nelements());
    cmds[i++] = new HLXUnitTestCmdInfoDisp<HLXStringTest>(
        this,
        "=char*",
        &HLXStringTest::HandleAssignOp3Cmd,
        3);                     // =char* A|B <from string>

    HX_ASSERT(i < cmds.Nelements());
    cmds[i++] = new HLXUnitTestCmdInfoDisp<HLXStringTest>(
        this,
        "=uchar*",
        &HLXStringTest::HandleAssignOp4Cmd,
        3);                     // =uchar* A|B <from string>

    HX_ASSERT(i < cmds.Nelements());
    cmds[i++] = new HLXUnitTestCmdInfoDisp<HLXStringTest>(
        this,
        "+=CHXString",
        &HLXStringTest::HandleAppendTo1Cmd,
        3);                     // +=CHXString A|B <from string: A|B>

    HX_ASSERT(i < cmds.Nelements());
    cmds[i++] = new HLXUnitTestCmdInfoDisp<HLXStringTest>(
        this,
        "+=char",
        &HLXStringTest::HandleAppendTo2Cmd,
        3);                     // +=char A|B <from char>

    HX_ASSERT(i < cmds.Nelements());
    cmds[i++] = new HLXUnitTestCmdInfoDisp<HLXStringTest>(
        this,
        "+=char*",
        &HLXStringTest::HandleAppendTo3Cmd,
        3);                     // +=char* A|B <from string>


    HX_ASSERT(i < cmds.Nelements());
    cmds[i++] = new HLXUnitTestCmdInfoDisp<HLXStringTest>(
        this,
        "CHXString+CHXString",
        &HLXStringTest::HandleAdd1Cmd,
        4);                     // CHXString+CHXString A|B A|B <expected string>

    HX_ASSERT(i < cmds.Nelements());
    cmds[i++] = new HLXUnitTestCmdInfoDisp<HLXStringTest>(
        this,
        "CHXString+char",
        &HLXStringTest::HandleAdd2Cmd,
        4);                     // CHXString+char A|B <char> <expected string>

    HX_ASSERT(i < cmds.Nelements());
    cmds[i++] = new HLXUnitTestCmdInfoDisp<HLXStringTest>(
        this,
        "char+CHXString",
        &HLXStringTest::HandleAdd3Cmd,
        4);                     // char+CHXString <char> A|B <expected string>

    HX_ASSERT(i < cmds.Nelements());
    cmds[i++] = new HLXUnitTestCmdInfoDisp<HLXStringTest>(
        this,
        "CHXString+char*",
        &HLXStringTest::HandleAdd4Cmd,
        4);                     // CHXString+char* A|B <string> <expected string>

    HX_ASSERT(i < cmds.Nelements());
    cmds[i++] = new HLXUnitTestCmdInfoDisp<HLXStringTest>(
        this,
        "char*+CHXString",
        &HLXStringTest::HandleAdd5Cmd,
        4);                     // char*+CHXString <string> A|B <expected string>

    HX_ASSERT(i < cmds.Nelements());
    cmds[i++] = new HLXUnitTestCmdInfoDisp<HLXStringTest>(
        this,
        "Center",
        &HLXStringTest::HandleCenterCmd,
        3);                     // Center A|B <length>


    HX_ASSERT(i < cmds.Nelements());
    cmds[i++] = new HLXUnitTestCmdInfoDisp<HLXStringTest>(
        this,
        "Compare",
        &HLXStringTest::HandleCompareCmd,
        4);                     // Compare A|B <string> <expected value>

    HX_ASSERT(i < cmds.Nelements());
    cmds[i++] = new HLXUnitTestCmdInfoDisp<HLXStringTest>(
        this,
        "CompareNoCase",
        &HLXStringTest::HandleCompareNoCaseCmd,
        4);                     // CompareNoCase A|B <string> <expected value>


    HX_ASSERT(i < cmds.Nelements());
    cmds[i++] = new HLXUnitTestCmdInfoDisp<HLXStringTest>(
        this,
        "Mid",
        &HLXStringTest::HandleMidCmd,
        4, 5);                  // Mid A|B <index> [<length>] <expected string>

    HX_ASSERT(i < cmds.Nelements());
    cmds[i++] = new HLXUnitTestCmdInfoDisp<HLXStringTest>(
        this,
        "Left",
        &HLXStringTest::HandleLeftCmd,
        4);                     // Left A|B <length> <expected string>

    HX_ASSERT(i < cmds.Nelements());
    cmds[i++] = new HLXUnitTestCmdInfoDisp<HLXStringTest>(
        this,
        "Right",
        &HLXStringTest::HandleRightCmd,
        4);                     // Right A|B <length> <expected string>


    HX_ASSERT(i < cmds.Nelements());
    cmds[i++] = new HLXUnitTestCmdInfoDisp<HLXStringTest>(
        this,
        "CountFields",
        &HLXStringTest::HandleCountFieldsCmd,
        4);                     // CountFields A|B <delimeter char> <expected value>

    HX_ASSERT(i < cmds.Nelements());
    cmds[i++] = new HLXUnitTestCmdInfoDisp<HLXStringTest>(
        this,
        "NthField",
        &HLXStringTest::HandleNthFieldCmd,
        5);                     // NthField A|B <delimeter char> <field index> <expected string>

    HX_ASSERT(i < cmds.Nelements());
    cmds[i++] = new HLXUnitTestCmdInfoDisp<HLXStringTest>(
        this,
        "GetNthField",
        &HLXStringTest::HandleGetNthFieldCmd,
        5);                     // GetNthField A|B <delimeter char> <field index> <expected string>

    HX_ASSERT(i < cmds.Nelements());
    cmds[i++] = new HLXUnitTestCmdInfoDisp<HLXStringTest>(
        this,
        "ResetNthFieldState",
        &HLXStringTest::HandleResetNthFieldStateCmd,
        2);                     // ResetNthFieldState A|B


    HX_ASSERT(i < cmds.Nelements());
    cmds[i++] = new HLXUnitTestCmdInfoDisp<HLXStringTest>(
        this,
        "SpanIncluding",
        &HLXStringTest::HandleSpanIncludingCmd,
        4);                     // SpanIncluding A|B <string> <expected string>

    HX_ASSERT(i < cmds.Nelements());
    cmds[i++] = new HLXUnitTestCmdInfoDisp<HLXStringTest>(
        this,
        "SpanExcluding",
        &HLXStringTest::HandleSpanExcludingCmd,
        4);                     // SpanExcluding A|B <string> <expected string>


    HX_ASSERT(i < cmds.Nelements());
    cmds[i++] = new HLXUnitTestCmdInfoDisp<HLXStringTest>(
        this,
        "MakeUpper",
        &HLXStringTest::HandleMakeUpperCmd,
        2);                     // MakeUpper A|B

    HX_ASSERT(i < cmds.Nelements());
    cmds[i++] = new HLXUnitTestCmdInfoDisp<HLXStringTest>(
        this,
        "MakeLower",
        &HLXStringTest::HandleMakeLowerCmd,
        2);                     // MakeLower A|B


    HX_ASSERT(i < cmds.Nelements());
    cmds[i++] = new HLXUnitTestCmdInfoDisp<HLXStringTest>(
        this,
        "TrimRight",
        &HLXStringTest::HandleTrimRightCmd,
        2);                     // TrimRight A|B

    HX_ASSERT(i < cmds.Nelements());
    cmds[i++] = new HLXUnitTestCmdInfoDisp<HLXStringTest>(
        this,
        "TrimLeft",
        &HLXStringTest::HandleTrimLeftCmd,
        2);                     // TrimLeft A|B


    HX_ASSERT(i < cmds.Nelements());
    cmds[i++] = new HLXUnitTestCmdInfoDisp<HLXStringTest>(
        this,
        "FindChar",
        &HLXStringTest::HandleFind1Cmd,
        4);                     // FindChar A|B <char> <expected value>

    HX_ASSERT(i < cmds.Nelements());
    cmds[i++] = new HLXUnitTestCmdInfoDisp<HLXStringTest>(
        this,
        "FindChar*",
        &HLXStringTest::HandleFind2Cmd,
        4);                     // FindChar* A|B <string> <expected value>

    HX_ASSERT(i < cmds.Nelements());
    cmds[i++] = new HLXUnitTestCmdInfoDisp<HLXStringTest>(
        this,
        "ReverseFind",
        &HLXStringTest::HandleReverseFindCmd,
        4);                     // ReverseFind A|B <char> <expected value>

    HX_ASSERT(i < cmds.Nelements());
    cmds[i++] = new HLXUnitTestCmdInfoDisp<HLXStringTest>(
        this,
        "FindAndReplace",
        &HLXStringTest::HandleFindAndReplaceCmd,
        6);                     // FindAndReplace A|B <search string> <repl string> <repl all bool> <expected value>


    HX_ASSERT(i < cmds.Nelements());
    cmds[i++] = new HLXUnitTestCmdInfoDisp<HLXStringTest>(
        this,
        "FormatInt",
        &HLXStringTest::HandleFormatIntCmd,
        4,13);                  // FormatInt A|B <format string> <data1> [... <data10>]

    HX_ASSERT(i < cmds.Nelements());
    cmds[i++] = new HLXUnitTestCmdInfoDisp<HLXStringTest>(
        this,
        "FormatLong",
        &HLXStringTest::HandleFormatLongCmd,
        4,13);                  // FormatLong A|B <format string> <data1> [... <data10>]

    HX_ASSERT(i < cmds.Nelements());
    cmds[i++] = new HLXUnitTestCmdInfoDisp<HLXStringTest>(
        this,
        "FormatUInt",
        &HLXStringTest::HandleFormatUIntCmd,
        4,13);                  // FormatUInt A|B <format string> <data1> [... <data10>]

    HX_ASSERT(i < cmds.Nelements());
    cmds[i++] = new HLXUnitTestCmdInfoDisp<HLXStringTest>(
        this,
        "FormatULong",
        &HLXStringTest::HandleFormatULongCmd,
        4,13);                  // FormatULong A|B <format string> <data1> [... <data10>]

    HX_ASSERT(i < cmds.Nelements());
    cmds[i++] = new HLXUnitTestCmdInfoDisp<HLXStringTest>(
        this,
        "FormatChar",
        &HLXStringTest::HandleFormatCharCmd,
        4,13);                  // FormatChar A|B <format string> <data1> [... <data10>]

    HX_ASSERT(i < cmds.Nelements());
    cmds[i++] = new HLXUnitTestCmdInfoDisp<HLXStringTest>(
        this,
        "FormatCharStar",
        &HLXStringTest::HandleFormatCharStarCmd,
        4,13);                  // FormatChar* A|B <format string> <data1> [... <data10>]

    HX_ASSERT(i < cmds.Nelements());
    cmds[i++] = new HLXUnitTestCmdInfoDisp<HLXStringTest>(
        this,
        "FormatPtr",
        &HLXStringTest::HandleFormatPtrCmd,
        4,13);                  // FormatPtr A|B <format string> <data1> [... <data10>]

    HX_ASSERT(i < cmds.Nelements());
    cmds[i++] = new HLXUnitTestCmdInfoDisp<HLXStringTest>(
        this,
        "FormatDouble",
        &HLXStringTest::HandleFormatDoubleCmd,
        4,13);                  // FormatDouble A|B <format string> <data1> [... <data10>]

    HX_ASSERT(i < cmds.Nelements());
    cmds[i++] = new HLXUnitTestCmdInfoDisp<HLXStringTest>(
        this,
        "FormatMixed",
        &HLXStringTest::HandleFormatMixedCmd,
        1);                     // FormatMixed

    HX_ASSERT(i < cmds.Nelements());
    cmds[i++] = new HLXUnitTestCmdInfoDisp<HLXStringTest>(
        this,
        "AppendULONG",
        &HLXStringTest::HandleAppendULONGCmd,
        3);                     // AppendULONG A|B <ULONG value>


    HX_ASSERT(i < cmds.Nelements());
    cmds[i++] = new HLXUnitTestCmdInfoDisp<HLXStringTest>(
        this,
        "AppendEndOfLine",
        &HLXStringTest::HandleAppendEndOfLineCmd,
        2);                     // AppendEndOfLine A|B


    HX_ASSERT(i < cmds.Nelements());
    cmds[i++] = new HLXUnitTestCmdInfoDisp<HLXStringTest>(
        this,
        "GetBuffer",
        &HLXStringTest::HandleGetBufferCmd,
        3, 4);                  // GetBuffer A|B <length> [<expected value>]

    HX_ASSERT(i < cmds.Nelements());
    cmds[i++] = new HLXUnitTestCmdInfoDisp<HLXStringTest>(
        this,
        "BufferSet",
        &HLXStringTest::HandleBufferSetCmd,
        3, 4);                  // BufferSet A|B [<index>] <string>

    HX_ASSERT(i < cmds.Nelements());
    cmds[i++] = new HLXUnitTestCmdInfoDisp<HLXStringTest>(
        this,
        "BufferFill",
        &HLXStringTest::HandleBufferFillCmd,
        4, 5);                  // BufferFill A|B [<index>] <string> <repeat count>

    HX_ASSERT(i < cmds.Nelements());
    cmds[i++] = new HLXUnitTestCmdInfoDisp<HLXStringTest>(
        this,
        "BufferEndString",
        &HLXStringTest::HandleBufferEndStringCmd,
        3);                     // BufferEndString A|B <index>

    HX_ASSERT(i < cmds.Nelements());
    cmds[i++] = new HLXUnitTestCmdInfoDisp<HLXStringTest>(
        this,
        "GetBufferSetLength",
        &HLXStringTest::HandleGetBufferSetLengthCmd,
        4);                     // GetBufferSetLength A|B <length> <expected value>

#ifdef XXXSAB
    HX_ASSERT(i < cmds.Nelements());
    cmds[i++] = new HLXUnitTestCmdInfoDisp<HLXStringTest>(
        this,
        "ReleaseBuffer0",
        &HLXStringTest::HandleReleaseBufferCmd,
        2);                     // ReleaseBuffer0 A|B

    HX_ASSERT(i < cmds.Nelements());
    cmds[i++] = new HLXUnitTestCmdInfoDisp<HLXStringTest>(
        this,
        "ReleaseBuffer1",
        &HLXStringTest::HandleReleaseBufferCmd,
        3);                     // ReleaseBuffer1 A|B <length>
#endif /* XXXSAB */

    HX_ASSERT(i < cmds.Nelements());
    cmds[i++] = new HLXUnitTestCmdInfoDisp<HLXStringTest>(
        this,
        "ReleaseBuffer",
        &HLXStringTest::HandleReleaseBufferCmd,
        2, 3);                  // ReleaseBuffer A|B [<length>]

    HX_ASSERT(i < cmds.Nelements());
    cmds[i++] = new HLXUnitTestCmdInfoDisp<HLXStringTest>(
        this,
        "FreeExtra",
        &HLXStringTest::HandleFreeExtraCmd,
        2);                     // FreeExtra A|B


    HX_ASSERT(i < cmds.Nelements());
    cmds[i++] = new HLXUnitTestCmdInfoDisp<HLXStringTest>(
        this,
        "GetAllocLength",
        &HLXStringTest::HandleGetAllocLengthCmd,
        3, 4);                  // GetAllocLength A|B <expected value> [<max expected>]

    HX_ASSERT(i < cmds.Nelements());
    cmds[i++] = new HLXUnitTestCmdInfoDisp<HLXStringTest>(
        this,
        "SetMinBufSize",
        &HLXStringTest::HandleSetMinBufSizeCmd,
        4);                     // SetMinBufSize A|B <new value> <expected value>
}

HLXCmdBasedTest* HLXStringTest::Clone() const
{
    return new HLXStringTest();
}

bool HLXStringTest::HandleConstruct1Cmd(const UTVector<UTString>& info)
{
    SetStringObj(info[1], new CHXString());

    return true;
}

bool HLXStringTest::HandleConstruct2Cmd(const UTVector<UTString>& info)
{
    bool ret = false;

    CHXString* pStr = GetStringObj(info[2]);

    if (!pStr)
    {
	DPRINTF(D_ERROR, ("Failed to get the string object '%s'\n", 
			  (const char*)info[2]));
    }
    else
    {
	SetStringObj(info[1], new CHXString(*pStr));

	ret = true;
    }

    return ret;
}

bool HLXStringTest::HandleConstruct3Cmd(const UTVector<UTString>& info)
{
    bool ret = false;

    char ch;
    int size;

    if (!UTParamUtil::GetChar(info[2], ch) || 
	!UTParamUtil::GetInt(info[3], size))
    {
	DPRINTF(D_ERROR, ("Failed to convert parameters\n"));
    }
    else
    {
	SetStringObj(info[1], new CHXString(ch, size));

	ret = true;
    }

    return ret;
}

bool HLXStringTest::HandleConstruct4Cmd(const UTVector<UTString>& info)
{
    SetStringObj(info[1], new CHXString((const char*)info[2]));

    return true;
}

bool HLXStringTest::HandleConstruct5Cmd(const UTVector<UTString>& info)
{
    bool ret = false;
    int size;

    if (!UTParamUtil::GetInt(info[3], size))
    {
	DPRINTF(D_ERROR, ("Failed to convert parameters\n"));
    }
    else
    {
	SetStringObj(info[1], new CHXString((const char*)(info[2]), size));

	ret = true;
    }

    return ret;
}

bool HLXStringTest::HandleConstruct6Cmd(const UTVector<UTString>& info)
{
    SetStringObj(info[1], new CHXString((const unsigned char*)(const char*)(info[2])));

    return true;
}

bool HLXStringTest::HandleGetLengthCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    CHXString* pStr = GetStringObj(info[1]);
    int expected = 0;

    if (!pStr)
    {
	DPRINTF(D_ERROR, ("Failed to get the string object '%s'\n", 
			  (const char*)info[1]));
    }
    else if (!UTParamUtil::GetInt(info[2], expected))
    {
	DPRINTF(D_ERROR, ("Failed to convert parameters\n"));
    }
    else if ((int)pStr->GetLength() != expected)
    {
	DPRINTF (D_ERROR, ("Got %lu expected %d\n", 
			   (unsigned long)pStr->GetLength(), 
			   expected));
    }
    else
	ret = true;

    return ret;
}

bool HLXStringTest::HandleIsEmptyCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    CHXString* pStr = GetStringObj(info[1]);
    bool expected = false;

    if (!pStr)
    {
	DPRINTF(D_ERROR, ("Failed to get the string object '%s'\n", 
			  (const char*)info[1]));
    }
    else if (!UTParamUtil::GetBool(info[2], expected))
    {
	DPRINTF(D_ERROR, ("Failed to convert parameters\n"));
    }
    else if ((pStr->IsEmpty() == TRUE) != expected)
    {
	DPRINTF(D_ERROR, ("Got %d expected %d\n", 
			  pStr->IsEmpty(),
			  expected));
    }
    else
	ret = true;

    return ret;
}

bool HLXStringTest::HandleEmptyCmd(const UTVector<UTString>& info)
{
    bool ret = false;
    CHXString* pStr = GetStringObj(info[1]);

    if (!pStr)
    {
	DPRINTF(D_ERROR, ("Failed to get the string object '%s'\n", 
			  (const char*)info[1]));
    }
    else
    {
	pStr->Empty();
	ret = true;
    }

    return ret;
}

bool HLXStringTest::HandleCharStarOpCmd(const UTVector<UTString>& info)
{
    bool ret = false;
    CHXString* pStr = GetStringObj(info[1]);
    int i;

    if (!pStr)
    {
	DPRINTF(D_ERROR, ("Failed to get the string object '%s'\n", 
			  (const char*)info[1]));
    }

    for (i = 2; i < info.Nelements(); ++i)
    {
        if (info[i] == (const char*)*pStr)
        {
            ret = true;
            break;
        }
    }

    if (! ret)
    {
	DPRINTF(D_ERROR, ("Got \"%s\" expected%s \"%s\"",
                          (const char*)*pStr,
                          info.Nelements() > 3 ? " one of" : "",
                          (const char*)info[2]));
        for (i = 3; i < info.Nelements(); ++i)
            DPRINTF(D_ERROR, (", \"%s\"", (const char*)info[i]));
        DPRINTF(D_ERROR, ("\n"));
    }

    return ret;
}

bool HLXStringTest::HandleGetAtCmd(const UTVector<UTString>& info)
{
    bool ret = false;
    CHXString* pStr = GetStringObj(info[1]);
    int index, expected, got;

    if (!pStr)
    {
	DPRINTF(D_ERROR, ("Failed to get the string object '%s'\n", 
			  (const char*)info[1]));
    }
    else if (!UTParamUtil::GetInt(info[2], index))
    {
	DPRINTF(D_ERROR, ("Failed to convert parameters\n"));
    }
    else if ((got = pStr->GetAt(index)) != (expected = *((const char*)info[3])))
    {
        DPRINTF(D_ERROR, ("HLXStringTest::GetAt: Got %c[0x%02x] expected %c[0x%02x]\n",
                          got, got, expected, expected));
    }
    else ret = true;

    return ret;
}

bool HLXStringTest::HandleSetAtCmd(const UTVector<UTString>& info)
{
    bool ret = false;
    CHXString* pStr = GetStringObj(info[1]);
    int index;

    if (!pStr)
    {
	DPRINTF(D_ERROR, ("Failed to get the string object '%s'\n", 
			  (const char*)info[1]));
    }
    else if (!UTParamUtil::GetInt(info[2], index))
    {
	DPRINTF(D_ERROR, ("Failed to convert parameters\n"));
    }
    else
    {
        ret = true;
        pStr->SetAt(index, *((const char*)info[3]));
    }

    return ret;
}

#define SUBTEST(TYPE) \
    if ((got = (*pStr)[(TYPE)index]) != expected) { \
        DPRINTF(D_ERROR, ("operator[](" #TYPE "): Got %c[0x%02x] expected %c[0x%02x]\n", \
                          got, got, expected, expected)); \
        ret = false; \
    }
bool HLXStringTest::HandleSubscriptCmd(const UTVector<UTString>& info)
{
    bool ret = false;
    CHXString* pStr = GetStringObj(info[1]);
    int index;

    if (!pStr)
    {
	DPRINTF(D_ERROR, ("Failed to get the string object '%s'\n", 
			  (const char*)info[1]));
    }
    else if (!UTParamUtil::GetInt(info[2], index))
    {
	DPRINTF(D_ERROR, ("Failed to convert parameters\n"));
    }
    else
    {
        // cycle through various subscript types and make sure that they
        // all work.
        char expected = *((const char*)info[3]);
        char got;
        ret = true;

        SUBTEST(short);
        SUBTEST(unsigned short);
        SUBTEST(int);
        SUBTEST(unsigned int);
        SUBTEST(long);
        SUBTEST(unsigned long);
    }

    return ret;
}
#undef SUBTEST

bool HLXStringTest::HandleAssignOp1Cmd(const UTVector<UTString>& info)
{
    bool ret = true;
    CHXString* pDest = GetStringObj(info[1]);
    CHXString* pSrc = GetStringObj(info[2]);

    if (!pDest)
    {
	DPRINTF(D_ERROR, ("Failed to get the string object '%s'\n", 
			  (const char*)info[1]));
        ret = false;
    }
    if (!pSrc)
    {
	DPRINTF(D_ERROR, ("Failed to get the string object '%s'\n", 
			  (const char*)info[2]));
        ret = false;
    }

    if (ret)
    {
        const CHXString& result = (*pDest = *pSrc);
        if (&result != pDest)
        {
            DPRINTF(D_ERROR, ("operator=(const CHXString&) didn't return object: got %p expected %p\n",
                              &result, pDest));
            ret = false;
        }
    }

    return ret;
}

bool HLXStringTest::HandleAssignOp2Cmd(const UTVector<UTString>& info)
{
    bool ret = false;
    CHXString* pStr = GetStringObj(info[1]);
    char ch = 0;

    if (!pStr)
    {
	DPRINTF(D_ERROR, ("Failed to get the string object '%s'\n", 
			  (const char*)info[1]));
    }
    else if (!UTParamUtil::GetChar(info[2], ch))
    {
	DPRINTF(D_ERROR, ("Failed to convert parameters\n"));
    }
    else
    {
        const CHXString& result = (*pStr = ch);
        if (&result != pStr)
        {
            DPRINTF(D_ERROR, ("operator=(char) didn't return object: got %p expected %p\n",
                              &result, pStr));
        }
        else ret = true;
    }

    return ret;
}

bool HLXStringTest::HandleAssignOp3Cmd(const UTVector<UTString>& info)
{
    bool ret = false;
    CHXString* pStr = GetStringObj(info[1]);

    if (!pStr)
    {
	DPRINTF(D_ERROR, ("Failed to get the string object '%s'\n", 
			  (const char*)info[1]));
    }
    else
    {
        const CHXString& result = (*pStr = (const char*)info[2]);
        if (&result != pStr)
        {
            DPRINTF(D_ERROR, ("operator=(char) didn't return object: got %p expected %p\n",
                              &result, pStr));
        }
        else ret = true;
    }

    return ret;
}

bool HLXStringTest::HandleAssignOp4Cmd(const UTVector<UTString>& info)
{
    bool ret = false;
    CHXString* pStr = GetStringObj(info[1]);

    if (!pStr)
    {
	DPRINTF(D_ERROR, ("Failed to get the string object '%s'\n", 
			  (const char*)info[1]));
    }
    else
    {
        const CHXString& result = (*pStr = (const unsigned char*)(const char*)info[2]);
        if (&result != pStr)
        {
            DPRINTF(D_ERROR, ("operator=(char) didn't return object: got %p expected %p\n",
                              &result, pStr));
        }
        else ret = true;
    }

    return ret;
}

bool HLXStringTest::HandleAppendTo1Cmd(const UTVector<UTString>& info)
{
    bool ret = true;
    CHXString* pDest = GetStringObj(info[1]);
    CHXString* pSrc = GetStringObj(info[2]);

    if (!pDest)
    {
	DPRINTF(D_ERROR, ("Failed to get the string object '%s'\n", 
			  (const char*)info[1]));
        ret = false;
    }
    if (!pSrc)
    {
	DPRINTF(D_ERROR, ("Failed to get the string object '%s'\n", 
			  (const char*)info[2]));
        ret = false;
    }

    if (ret)
    {
        const CHXString& result = (*pDest += *pSrc);
        if (&result != pDest)
        {
            DPRINTF(D_ERROR, ("operator=(const CHXString&) didn't return object: got %p expected %p\n",
                              &result, pDest));
            ret = false;
        }
    }

    return ret;
}

bool HLXStringTest::HandleAppendTo2Cmd(const UTVector<UTString>& info)
{
    bool ret = true;
    CHXString* pStr = GetStringObj(info[1]);

    if (!pStr)
    {
	DPRINTF(D_ERROR, ("Failed to get the string object '%s'\n", 
			  (const char*)info[1]));
    }
    else
    {
        const CHXString& result = (*pStr += *((const char*)info[2]));
        if (&result != pStr)
        {
            DPRINTF(D_ERROR, ("operator=(const CHXString&) didn't return object: got %p expected %p\n",
                              &result, pStr));
            ret = false;
        }
    }
    return ret;
}

bool HLXStringTest::HandleAppendTo3Cmd(const UTVector<UTString>& info)
{
    bool ret = true;
    CHXString* pStr = GetStringObj(info[1]);

    if (!pStr)
    {
	DPRINTF(D_ERROR, ("Failed to get the string object '%s'\n", 
			  (const char*)info[1]));
    }
    else
    {
        const CHXString& result = (*pStr += (const char*)info[2]);
        if (&result != pStr)
        {
            DPRINTF(D_ERROR, ("operator=(const CHXString&) didn't return object: got %p expected %p\n",
                              &result, pStr));
            ret = false;
        }
    }

    return ret;
}

bool HLXStringTest::HandleAdd1Cmd(const UTVector<UTString>& info)
{
    bool ret = true;
    CHXString* pStr1 = GetStringObj(info[1]);
    CHXString* pStr2 = GetStringObj(info[2]);

    if (!pStr1)
    {
	DPRINTF(D_ERROR, ("Failed to get the string object '%s'\n", 
			  (const char*)info[1]));
        ret = false;
    }
    if (!pStr2)
    {
	DPRINTF(D_ERROR, ("Failed to get the string object '%s'\n", 
			  (const char*)info[2]));
        ret = false;
    }

    if (ret)
    {
        CHXString got = *pStr1 + *pStr2;
        if (strcmp (got, (const char*)info[3]) != 0)
        {
            DPRINTF(D_ERROR, ("Got \"%s\" expected \"%s\"\n",
                              (const char*)got,
                              (const char*)info[3]));
            ret = false;
        }
    }

    return ret;
}

bool HLXStringTest::HandleAdd2Cmd(const UTVector<UTString>& info)
{
    bool ret = true;
    CHXString* pStr1 = GetStringObj(info[1]);

    if (!pStr1)
    {
	DPRINTF(D_ERROR, ("Failed to get the string object '%s'\n", 
			  (const char*)info[1]));
        ret = false;
    }

    if (ret)
    {
        CHXString got = *pStr1 + *((const char*)info[2]);
        if (strcmp (got, (const char*)info[3]) != 0)
        {
            DPRINTF(D_ERROR, ("Got \"%s\" expected \"%s\"\n",
                              (const char*)got,
                              (const char*)info[3]));
            ret = false;
        }
    }

    return ret;
}


bool HLXStringTest::HandleAdd3Cmd(const UTVector<UTString>& info)
{
    bool ret = true;
    CHXString* pStr2 = GetStringObj(info[2]);

    if (!pStr2)
    {
	DPRINTF(D_ERROR, ("Failed to get the string object '%s'\n", 
			  (const char*)info[2]));
        ret = false;
    }

    if (ret)
    {
        CHXString got = *((const char*)info[1]) + *pStr2;
        if (strcmp (got, (const char*)info[3]) != 0)
        {
            DPRINTF(D_ERROR, ("Got \"%s\" expected \"%s\"\n",
                              (const char*)got,
                              (const char*)info[3]));
            ret = false;
        }
    }

    return ret;
}


bool HLXStringTest::HandleAdd4Cmd(const UTVector<UTString>& info)
{
    bool ret = true;
    CHXString* pStr1 = GetStringObj(info[1]);

    if (!pStr1)
    {
	DPRINTF(D_ERROR, ("Failed to get the string object '%s'\n", 
			  (const char*)info[1]));
        ret = false;
    }

    if (ret)
    {
        CHXString got = *pStr1 + (const char*)info[2];
        if (strcmp (got, (const char*)info[3]) != 0)
        {
            DPRINTF(D_ERROR, ("Got \"%s\" expected \"%s\"\n",
                              (const char*)got,
                              (const char*)info[3]));
            ret = false;
        }
    }

    return ret;
}


bool HLXStringTest::HandleAdd5Cmd(const UTVector<UTString>& info)
{
    bool ret = true;
    CHXString* pStr2 = GetStringObj(info[2]);

    if (!pStr2)
    {
	DPRINTF(D_ERROR, ("Failed to get the string object '%s'\n", 
			  (const char*)info[2]));
        ret = false;
    }

    if (ret)
    {
        CHXString got = (const char*)info[1] + *pStr2;
        if (strcmp (got, (const char*)info[3]) != 0)
        {
            DPRINTF(D_ERROR, ("Got \"%s\" expected \"%s\"\n",
                              (const char*)got,
                              (const char*)info[3]));
            ret = false;
        }
    }

    return ret;
}


bool HLXStringTest::HandleCenterCmd(const UTVector<UTString>& info)
{
    bool ret = false;
    CHXString* pStr = GetStringObj(info[1]);
    int len;

    if (!pStr)
    {
	DPRINTF(D_ERROR, ("Failed to get the string object '%s'\n", 
			  (const char*)info[1]));
    }
    else if (!UTParamUtil::GetInt(info[2], len))
    {
	DPRINTF(D_ERROR, ("Failed to convert parameters\n"));
    }
    else
    {
        pStr->Center(len);
        ret = true;
    }

    return ret;
}

bool HLXStringTest::HandleCompareCmd(const UTVector<UTString>& info)
{
    bool ret = false;
    CHXString* pStr = GetStringObj(info[1]);
    int expected = -9999;
    bool bNumberExpected = (info[3] != "-" && info[3] != "+");

    if (!pStr)
    {
	DPRINTF(D_ERROR, ("Failed to get the string object '%s'\n", 
			  (const char*)info[1]));
    }
    else if (bNumberExpected && !UTParamUtil::GetInt(info[3], expected))
    {
	DPRINTF(D_ERROR, ("Failed to convert parameters\n"));
    }
    else
    {
        INT32 got = pStr->Compare(info[2]);
        if ((info[3] == "+" && got > 0) ||
            (info[3] == "-" && got < 0) ||
            (bNumberExpected && got == expected)) ret = true;
        else DPRINTF(D_ERROR, ("Got %d expected %s\n",
                               got, (const char*)info[3]));
    }

    return ret;
}

bool HLXStringTest::HandleCompareNoCaseCmd(const UTVector<UTString>& info)
{
    bool ret = false;
    CHXString* pStr = GetStringObj(info[1]);
    int expected = -9999;
    bool bNumberExpected = (info[3] != "-" && info[3] != "+");

    if (!pStr)
    {
	DPRINTF(D_ERROR, ("Failed to get the string object '%s'\n", 
			  (const char*)info[1]));
    }
    else if (bNumberExpected && !UTParamUtil::GetInt(info[3], expected))
    {
	DPRINTF(D_ERROR, ("Failed to convert parameters\n"));
    }
    else
    {
        INT32 got = pStr->CompareNoCase(info[2]);
        if ((info[3] == "+" && got > 0) ||
            (info[3] == "-" && got < 0) ||
            (bNumberExpected && got == expected)) ret = true;
        else DPRINTF(D_ERROR, ("Got %d expected %s\n",
                               got, (const char*)info[3]));
    }

    return ret;
}

bool HLXStringTest::HandleMidCmd(const UTVector<UTString>& info)
{
    bool ret = true;
    CHXString* pStr = GetStringObj(info[1]);
    int index;
    const char* expected = NULL;
    int length = -9999;

    if (!pStr)
    {
	DPRINTF(D_ERROR, ("Failed to get the string object '%s'\n", 
			  (const char*)info[1]));
        ret = false;
    }
    else if (!UTParamUtil::GetInt(info[2], index))
    {
	DPRINTF(D_ERROR, ("Failed to convert parameters\n"));
        ret = false;
    }
    else if (info.Nelements() == 4)
    {
        expected = (const char*)info[3];
    }
    else if (info.Nelements() == 5)
    {
        if (!UTParamUtil::GetInt(info[3], length))
        {
            DPRINTF(D_ERROR, ("Failed to convert parameters\n"));
            ret = false;
        }
        else expected = (const char*)info[4];
    }
    else
    {
        DPRINTF(D_ERROR, ("Bad number of arguments: %ld\n",
                          (long)info.Nelements()));
        ret = false;
    }

    if (ret)
    {
        CHXString got =
            (length == -9999 ? pStr->Mid(index) : pStr->Mid(index, length));
        if (strcmp(got, expected) != 0)
        {
            DPRINTF(D_ERROR, ("Got \"%s\" expected \"%s\"\n",
                              (const char*)got,
                              expected));
            ret = false;
        }
    }

    return ret;
}

bool HLXStringTest::HandleLeftCmd(const UTVector<UTString>& info)
{
    bool ret = true;
    CHXString* pStr = GetStringObj(info[1]);
    const char* expected = (const char*)info[3];
    int length = -9999;

    if (!pStr)
    {
	DPRINTF(D_ERROR, ("Failed to get the string object '%s'\n", 
			  (const char*)info[1]));
        ret = false;
    }
    else if (!UTParamUtil::GetInt(info[2], length))
    {
	DPRINTF(D_ERROR, ("Failed to convert parameters\n"));
        ret = false;
    }
    else
    {
        CHXString got = pStr->Left(length);
        if (strcmp(got, expected) != 0)
        {
            DPRINTF(D_ERROR, ("Got \"%s\" expected \"%s\"\n",
                              (const char*)got,
                              expected));
            ret = false;
        }
    }

    return ret;
}


bool HLXStringTest::HandleRightCmd(const UTVector<UTString>& info)
{
    bool ret = true;
    CHXString* pStr = GetStringObj(info[1]);
    const char* expected = (const char*)info[3];
    int length = -9999;

    if (!pStr)
    {
	DPRINTF(D_ERROR, ("Failed to get the string object '%s'\n", 
			  (const char*)info[1]));
        ret = false;
    }
    else if (!UTParamUtil::GetInt(info[2], length))
    {
	DPRINTF(D_ERROR, ("Failed to convert parameters\n"));
        ret = false;
    }
    else
    {
        CHXString got = pStr->Right(length);
        if (strcmp(got, expected) != 0)
        {
            DPRINTF(D_ERROR, ("Got \"%s\" expected \"%s\"\n",
                              (const char*)got,
                              expected));
            ret = false;
        }
    }

    return ret;
}

bool HLXStringTest::HandleCountFieldsCmd(const UTVector<UTString>& info)
{
    bool ret = true;
    CHXString* pStr = GetStringObj(info[1]);
    unsigned int expected;

    if (!pStr)
    {
	DPRINTF(D_ERROR, ("Failed to get the string object '%s'\n", 
			  (const char*)info[1]));
        ret = false;
    }
    else if (!UTParamUtil::GetUInt(info[3], expected))
    {
	DPRINTF(D_ERROR, ("Failed to convert parameters\n"));
        ret = false;
    }
    else
    {
        ULONG32 got = pStr->CountFields(*((const char*)info[2]));
        if (got != expected)
        {
            DPRINTF(D_ERROR, ("Got %lu expected %lu\n",
                              (unsigned long)got, (unsigned long)expected));
            ret = false;
        }
    }

    return ret;
}

bool HLXStringTest::HandleNthFieldCmd(const UTVector<UTString>& info)
{
    bool ret = true;
    CHXString* pStr = GetStringObj(info[1]);
    unsigned int field;

    if (!pStr)
    {
	DPRINTF(D_ERROR, ("Failed to get the string object '%s'\n", 
			  (const char*)info[1]));
        ret = false;
    }
    else if (!UTParamUtil::GetUInt(info[3], field))
    {
	DPRINTF(D_ERROR, ("Failed to convert parameters\n"));
        ret = false;
    }
    else
    {
        CHXString got = pStr->NthField(*((const char*)info[2]),
                                       (ULONG32)field);
        const char* expected = (const char*)info[4];

        if (strcmp(got, expected) != 0)
        {
            DPRINTF(D_ERROR, ("Got \"%s\" expected \"%s\"\n",
                              (const char*)got, expected));
            ret = false;
        }
    }

    return ret;
}


bool HLXStringTest::HandleResetNthFieldStateCmd(const UTVector<UTString>& info)
{
    bool ret = true;
    CHXString* pStr = GetStringObj(info[1]);

    // XXXSAB: Should we have a way to set this NthFieldState thing to garbage???

    if (!pStr)
    {
	DPRINTF(D_ERROR, ("Failed to get the string object '%s'\n", 
			  (const char*)info[1]));
        ret = false;
    }
    else
    {
        if (pStr == m_pA) m_ANthFieldState = 0;
        else if (pStr == m_pB) m_BNthFieldState = 0;
    }

    return ret;
}

bool HLXStringTest::HandleGetNthFieldCmd(const UTVector<UTString>& info)
{
    bool ret = true;
    CHXString* pStr = GetStringObj(info[1]);
    unsigned int field;

    if (!pStr)
    {
	DPRINTF(D_ERROR, ("Failed to get the string object '%s'\n", 
			  (const char*)info[1]));
        ret = false;
    }
    else if (!UTParamUtil::GetUInt(info[3], field))
    {
	DPRINTF(D_ERROR, ("Failed to convert parameters\n"));
        ret = false;
    }
    else
    {
        UINT64& state = (pStr == m_pA) ? m_ANthFieldState : m_BNthFieldState;

        CHXString got = pStr->GetNthField(*((const char*)info[2]),
                                          (ULONG32)field,
                                          state);
        const char* expected = (const char*)info[4];

        if (strcmp(got, expected) != 0)
        {
            DPRINTF(D_ERROR, ("Got \"%s\" expected \"%s\"\n",
                              (const char*)got, expected));
            ret = false;
        }
    }

    return ret;
}


bool HLXStringTest::HandleSpanIncludingCmd(const UTVector<UTString>& info)
{
    bool ret = false;
    CHXString* pStr = GetStringObj(info[1]);

    if (!pStr)
    {
	DPRINTF(D_ERROR, ("Failed to get the string object '%s'\n", 
			  (const char*)info[1]));
    }
    else
    {
        CHXString got = pStr->SpanIncluding((const char*)info[2]);
        if (strcmp(got, (const char*)info[3]) == 0) ret = true;
        else DPRINTF(D_ERROR, ("Got \"%s\" expected \"%s\"\n",
                               (const char*)got, (const char*)info[3]));
    }

    return ret;
}

bool HLXStringTest::HandleSpanExcludingCmd(const UTVector<UTString>& info)
{
    bool ret = false;
    CHXString* pStr = GetStringObj(info[1]);

    if (!pStr)
    {
	DPRINTF(D_ERROR, ("Failed to get the string object '%s'\n", 
			  (const char*)info[1]));
    }
    else
    {
        CHXString got = pStr->SpanExcluding((const char*)info[2]);
        if (strcmp(got, (const char*)info[3]) == 0) ret = true;
        else DPRINTF(D_ERROR, ("Got \"%s\" expected \"%s\"\n",
                               (const char*)got, (const char*)info[3]));
    }

    return ret;
}

bool HLXStringTest::HandleMakeUpperCmd(const UTVector<UTString>& info)
{
    bool ret = false;
    CHXString* pStr = GetStringObj(info[1]);

    if (!pStr)
    {
	DPRINTF(D_ERROR, ("Failed to get the string object '%s'\n", 
			  (const char*)info[1]));
    }
    else
    {
        ret = true;
        pStr->MakeUpper();
    }

    return ret;
}

bool HLXStringTest::HandleMakeLowerCmd(const UTVector<UTString>& info)
{
    bool ret = false;
    CHXString* pStr = GetStringObj(info[1]);

    if (!pStr)
    {
	DPRINTF(D_ERROR, ("Failed to get the string object '%s'\n", 
			  (const char*)info[1]));
    }
    else
    {
        ret = true;
        pStr->MakeLower();
    }

    return ret;
}

bool HLXStringTest::HandleTrimRightCmd(const UTVector<UTString>& info)
{
    bool ret = false;
    CHXString* pStr = GetStringObj(info[1]);

    if (!pStr)
    {
	DPRINTF(D_ERROR, ("Failed to get the string object '%s'\n", 
			  (const char*)info[1]));
    }
    else
    {
        ret = true;
        pStr->TrimRight();
    }

    return ret;
}

bool HLXStringTest::HandleTrimLeftCmd(const UTVector<UTString>& info)
{
    bool ret = false;
    CHXString* pStr = GetStringObj(info[1]);

    if (!pStr)
    {
	DPRINTF(D_ERROR, ("Failed to get the string object '%s'\n", 
			  (const char*)info[1]));
    }
    else
    {
        ret = true;
        pStr->TrimLeft();
    }


    return ret;
}

bool HLXStringTest::HandleFind1Cmd(const UTVector<UTString>& info)
{
    bool ret = false;
    CHXString* pStr = GetStringObj(info[1]);
    int expected;

    if (!pStr)
    {
	DPRINTF(D_ERROR, ("Failed to get the string object '%s'\n", 
			  (const char*)info[1]));
    }
    else if (!UTParamUtil::GetInt(info[3], expected))
    {
	DPRINTF(D_ERROR, ("Failed to convert parameters\n"));
    }
    else
    {
        INT32 got = pStr->Find(*((const char*)info[2]));
        if (got == expected) ret = true;
        else DPRINTF(D_ERROR, ("Got %ld expected %ld\n",
                               (long)got, (long)expected));
    }

    return ret;
}

bool HLXStringTest::HandleFind2Cmd(const UTVector<UTString>& info)
{
    bool ret = false;
    CHXString* pStr = GetStringObj(info[1]);
    int expected;

    if (!pStr)
    {
	DPRINTF(D_ERROR, ("Failed to get the string object '%s'\n", 
			  (const char*)info[1]));
    }
    else if (!UTParamUtil::GetInt(info[3], expected))
    {
	DPRINTF(D_ERROR, ("Failed to convert parameters\n"));
    }
    else
    {
        INT32 got = pStr->Find((const char*)info[2]);
        if (got == expected) ret = true;
        else DPRINTF(D_ERROR, ("Got %ld expected %ld\n",
                               (long)got, (long)expected));
    }

    return ret;
}

bool HLXStringTest::HandleReverseFindCmd(const UTVector<UTString>& info)
{
    bool ret = false;
    CHXString* pStr = GetStringObj(info[1]);
    int expected;

    if (!pStr)
    {
	DPRINTF(D_ERROR, ("Failed to get the string object '%s'\n", 
			  (const char*)info[1]));
    }
    else if (!UTParamUtil::GetInt(info[3], expected))
    {
	DPRINTF(D_ERROR, ("Failed to convert parameters\n"));
    }
    else
    {
        INT32 got = pStr->ReverseFind(*((const char*)info[2]));
        if (got == expected) ret = true;
        else DPRINTF(D_ERROR, ("Got %ld expected %ld\n",
                               (long)got, (long)expected));
    }

    return ret;
}

bool HLXStringTest::HandleFindAndReplaceCmd(const UTVector<UTString>& info)
{
    bool ret = false;
    CHXString* pStr = GetStringObj(info[1]);
    bool expected, bReplaceAll;

    if (!pStr)
    {
	DPRINTF(D_ERROR, ("Failed to get the string object '%s'\n", 
			  (const char*)info[1]));
    }
    else if (!UTParamUtil::GetBool(info[4], bReplaceAll) ||
             !UTParamUtil::GetBool(info[5], expected))
    {
	DPRINTF(D_ERROR, ("Failed to convert parameters\n"));
    }
    else
    {
        bool got = pStr->FindAndReplace((const char*)info[2],
                                        (const char*)info[3],
                                        bReplaceAll) ? true : false;
        if (got == expected) ret = true;
        else DPRINTF(D_ERROR, ("Got %s expected %s\n",
                               got ? "TRUE" : "FALSE",
                               expected ? "TRUE" : "FALSE"));
    }

    return ret;
}

namespace
{
    inline bool
    GetCharStar (const char* strRep, const char*& value)
    {
        value = strRep;
        return true;
    }

    inline bool
    GetPtr (const char* strRep, void*& value)
    {
        unsigned long iValue;
        bool ret = UTParamUtil::GetULong(strRep, iValue);
        if (ret) value = (void*)iValue;
        return ret;
    }

    template <class T>
    bool HandleFormat (CHXString& str, const UTVector<UTString>& info,
                       bool (*GetValue)(const char* strRep, T& retValue))
    {
        const char* fmt = (const char*)info[2];
        T* values = new T[info.Nelements() - 3];
        
        int beginData = 3;
        int endData = info.Nelements();

        int infoIdx = beginData;
        int valIdx = 0;

        for (; infoIdx < endData; ++infoIdx, ++valIdx)
        {
            if (!GetValue(info[infoIdx], values[valIdx]))
            {
                DPRINTF(D_ERROR, ("Failed to convert arg \"%s\"\n",
                                  (const char*)info[infoIdx]));
                return false;
            }
        }

        switch (valIdx)
        {
            case 1:
                str.Format(fmt, values[0]);
                break;
            case 2:
                str.Format(fmt, values[0], values[1]);
                break;
            case 3:
                str.Format(fmt, values[0], values[1], values[2]);
                break;
            case 4:
                str.Format(fmt, values[0], values[1], values[2], values[3]);
                break;
            case 5:
                str.Format(fmt, values[0], values[1], values[2], values[3],
                           values[4]);
                break;
            case 6:
                str.Format(fmt, values[0], values[1], values[2], values[3],
                           values[4], values[5]);
                break;
            case 7:
                str.Format(fmt, values[0], values[1], values[2], values[3],
                           values[4], values[5], values[6]);
                break;
            case 8:
                str.Format(fmt, values[0], values[1], values[2], values[3],
                           values[4], values[5], values[6], values[7]);
                break;
            case 9:
                str.Format(fmt, values[0], values[1], values[2], values[3],
                           values[4], values[5], values[6], values[7],
                           values[8]);
                break;
            case 10:
                str.Format(fmt, values[0], values[1], values[2], values[3],
                           values[4], values[5], values[6], values[7],
                           values[8], values[9]);
                break;
            default:
                DPRINTF(D_ERROR, ("Unsupported num of data values (%d)\n",
                                  valIdx));
                return false;
                break;
        }

	delete [] values;
        return true;
    }
};

bool HLXStringTest::HandleFormatIntCmd(const UTVector<UTString>& info)
{
    bool ret = false;
    CHXString* pStr = GetStringObj(info[1]);

    if (!pStr)
    {
	DPRINTF(D_ERROR, ("Failed to get the string object '%s'\n", 
			  (const char*)info[1]));
    }
    else ret = HandleFormat<int>(*pStr, info, UTParamUtil::GetInt);

    return ret;
}

bool HLXStringTest::HandleFormatLongCmd(const UTVector<UTString>& info)
{
    bool ret = false;
    CHXString* pStr = GetStringObj(info[1]);

    if (!pStr)
    {
	DPRINTF(D_ERROR, ("Failed to get the string object '%s'\n", 
			  (const char*)info[1]));
    }
    else ret = HandleFormat<long int>(*pStr, info, UTParamUtil::GetLong);

    return ret;
}

bool HLXStringTest::HandleFormatUIntCmd(const UTVector<UTString>& info)
{
    bool ret = false;
    CHXString* pStr = GetStringObj(info[1]);

    if (!pStr)
    {
	DPRINTF(D_ERROR, ("Failed to get the string object '%s'\n", 
			  (const char*)info[1]));
    }
    else ret = HandleFormat<unsigned int>(*pStr, info, UTParamUtil::GetUInt);

    return ret;
}

bool HLXStringTest::HandleFormatULongCmd(const UTVector<UTString>& info)
{
    bool ret = false;
    CHXString* pStr = GetStringObj(info[1]);

    if (!pStr)
    {
	DPRINTF(D_ERROR, ("Failed to get the string object '%s'\n", 
			  (const char*)info[1]));
    }
    else ret = HandleFormat<unsigned long int>(*pStr, info, 
					       UTParamUtil::GetULong);

    return ret;
}

bool HLXStringTest::HandleFormatCharCmd(const UTVector<UTString>& info)
{
    bool ret = false;
    CHXString* pStr = GetStringObj(info[1]);

    if (!pStr)
    {
	DPRINTF(D_ERROR, ("Failed to get the string object '%s'\n", 
			  (const char*)info[1]));
    }
    else ret = HandleFormat<char>(*pStr, info, UTParamUtil::GetChar);

    return ret;
}

bool HLXStringTest::HandleFormatCharStarCmd(const UTVector<UTString>& info)
{
    bool ret = false;
    CHXString* pStr = GetStringObj(info[1]);

    if (!pStr)
    {
	DPRINTF(D_ERROR, ("Failed to get the string object '%s'\n", 
			  (const char*)info[1]));
    }
    else ret = HandleFormat<const char*>(*pStr, info, GetCharStar);

    return ret;
}

bool HLXStringTest::HandleFormatPtrCmd(const UTVector<UTString>& info)
{
    bool ret = false;
    CHXString* pStr = GetStringObj(info[1]);

    if (!pStr)
    {
	DPRINTF(D_ERROR, ("Failed to get the string object '%s'\n", 
			  (const char*)info[1]));
    }
    else ret = HandleFormat<void*>(*pStr, info, GetPtr);

    return ret;
}

bool HLXStringTest::HandleFormatDoubleCmd(const UTVector<UTString>& info)
{
    bool ret = false;
    CHXString* pStr = GetStringObj(info[1]);

    if (!pStr)
    {
	DPRINTF(D_ERROR, ("Failed to get the string object '%s'\n", 
			  (const char*)info[1]));
    }
    else ret = HandleFormat<double>(*pStr, info, UTParamUtil::GetDouble);

    return ret;
}

bool HLXStringTest::HandleFormatMixedCmd(const UTVector<UTString>& /*info*/)
{
    CHXString str;

    // XXXSAB - now what???

    const char* expected = "";
    char* ptr = (char*)0x60616263;

    str.Format ("%p,%d,%u,%c",
                ptr, (int)ptr, (unsigned int)ptr, (char)ptr);
#ifdef _WINDOWS
    expected = "60616263,1616994915,1616994915,c";
#else
    expected = "0x60616263,1616994915,1616994915,c";
#endif /* _WINDOWS */
    if (str != expected)
    {
        DPRINTF(D_ERROR, ("FormatMixed #1: Got \"%s\" expected \"%s\"\n",
                          (const char*)str, expected));
        return false;
    }

    ptr = 0;
    str.Format ("%p,%p,%p,%p", ptr, ptr+1, ptr+2, ptr+3);
#ifdef _WINDOWS
    expected = "00000000,00000001,00000002,00000003";
#elif defined(_SYMBIAN)
    expected = "0x0,0x1,0x2,0x3";
#else
    expected = "(nil),0x1,0x2,0x3";
#endif /* _WINDOWS */
    if (str != expected)
    {
        DPRINTF(D_ERROR, ("FormatMixed #2: Got \"%s\" expected \"%s\"\n",
                          (const char*)str, expected));
        return false;
    }

    str.Format ("%6d,%5u,%x", 1,1,1);
    expected = "     1,    1,1";
    if (str != expected)
    {
        DPRINTF(D_ERROR, ("FormatMixed #3: Got \"%s\" expected \"%s\"\n",
                          (const char*)str, expected));
        return false;
    }

    return true;
}

bool HLXStringTest::HandleAppendULONGCmd(const UTVector<UTString>& info)
{
    bool ret = false;
    CHXString* pStr = GetStringObj(info[1]);
    unsigned int value;

    if (!pStr)
    {
	DPRINTF(D_ERROR, ("Failed to get the string object '%s'\n", 
			  (const char*)info[1]));
    }
    else if (!UTParamUtil::GetUInt(info[2], value))
    {
	DPRINTF(D_ERROR, ("Failed to convert parameters\n"));
    }
    else
    {
        ret = true;
        pStr->AppendULONG((ULONG32)value);
    }

    return ret;
}

bool HLXStringTest::HandleAppendEndOfLineCmd(const UTVector<UTString>& info)
{
    bool ret = false;
    CHXString* pStr = GetStringObj(info[1]);

    if (!pStr)
    {
	DPRINTF(D_ERROR, ("Failed to get the string object '%s'\n", 
			  (const char*)info[1]));
    }
    else
    {
        UINT32 beforeLength = pStr->GetLength();
        pStr->AppendEndOfLine();

        // Check that the proper EOL sequence was added
        if (pStr->IsEmpty())
        {
            DPRINTF(D_ERROR, ("Failed to add EOL sequence: string is now empty\n"));
        }
        else
        {
            const char* pEnd = (const char*)*pStr + pStr->GetLength() - 1;
#if defined(_UNIX) 
            ret = (*pEnd == '\n') &&
                (pStr->GetLength() == (beforeLength+1));
#elif defined(_MACINTOSH)
            ret = (*pEnd == '\r') &&
                (pStr->GetLength() == (beforeLength+1));
#elif defined(_WINDOWS) || defined(_SYMBIAN)
            ret = (*(pEnd-1) == '\r' && *pEnd == '\n') &&
                (pStr->GetLength() == (beforeLength+2));
#else
           ret = (*pEnd == '\n') &&
                (pStr->GetLength() == (beforeLength+1));
#endif
            if (!ret)
            {
                DPRINTF(D_ERROR, ("Failed to add EOL: got \"%s\"\n",
                              (const char*)*pStr));
            }
        }
    }

    return ret;
}

bool HLXStringTest::HandleGetBufferCmd(const UTVector<UTString>& info)
{
    bool ret = false;
    CHXString* pStr = GetStringObj(info[1]);
    int length;

    if (!pStr)
    {
	DPRINTF(D_ERROR, ("Failed to get the string object '%s'\n", 
			  (const char*)info[1]));
    }
    else if (!UTParamUtil::GetInt(info[2], length))
    {
	DPRINTF(D_ERROR, ("Failed to convert parameters\n"));
    }
    else
    {
        // NOTE: If didn't get at least 4 elements in 'info', then we
        //       don't care what's in the returned buffer...

        char*& p = (pStr == m_pA ? m_pABuffer : m_pBBuffer);
        p = pStr->GetBuffer(length);

        if (!p) DPRINTF(D_ERROR, ("Got NULL from GetBuffer()\n"));
        else if (info.Nelements() >= 4 &&
                 strcmp(p, (const char*)info[3]) != 0)
        {
            DPRINTF(D_ERROR, ("Got \"%s\" expected \"%s\"\n",
                              p, (const char*)info[3]));
        }
        else ret = true;
    }

    return ret;
}

bool HLXStringTest::HandleGetBufferSetLengthCmd(const UTVector<UTString>& info)
{
    bool ret = false;
    CHXString* pStr = GetStringObj(info[1]);
    int length;

    if (!pStr)
    {
	DPRINTF(D_ERROR, ("Failed to get the string object '%s'\n", 
			  (const char*)info[1]));
    }
    else if (!UTParamUtil::GetInt(info[2], length))
    {
	DPRINTF(D_ERROR, ("Failed to convert parameters\n"));
    }
    else
    {
        // NOTE: If didn't get at least 4 elements in 'info', then we
        //       don't care what's in the returned buffer...

        char*& p = (pStr == m_pA ? m_pABuffer : m_pBBuffer);
        p = pStr->GetBufferSetLength(length);

        if (!p) DPRINTF(D_ERROR, ("Got NULL from GetBuffer()\n"));
        else if (info.Nelements() >= 4 &&
                 strcmp(p, (const char*)info[3]) != 0)
        {
            DPRINTF(D_ERROR, ("Got \"%s\" expected \"%s\"\n",
                              p, (const char*)info[3]));
        }
        else ret = true;
    }

    return ret;
}

bool HLXStringTest::HandleBufferSetCmd(const UTVector<UTString>& info)
{
    bool ret = false;
    CHXString* pStr = GetStringObj(info[1]);
    int index = 0;

    if (!pStr)
    {
	DPRINTF(D_ERROR, ("Failed to get the string object '%s'\n", 
			  (const char*)info[1]));
    }
    else if (info.Nelements() >= 4 && !UTParamUtil::GetInt(info[2], index))
    {
	DPRINTF(D_ERROR, ("Failed to convert parameters\n"));
    }
    else
    {
        char* p = (pStr == m_pA ? m_pABuffer : m_pBBuffer);
        const char* pValue = (const char*)info[info.Nelements()-1];

        memcpy (p+index, pValue, strlen(pValue)); /* Flawfinder: ignore */
        ret = true;
    }

    return ret;
}

bool HLXStringTest::HandleBufferFillCmd(const UTVector<UTString>& info)
{
    bool ret = false;
    CHXString* pStr = GetStringObj(info[1]);
    int index = 0;
    int repeat = 0;

    if (!pStr)
    {
	DPRINTF(D_ERROR, ("Failed to get the string object '%s'\n", 
			  (const char*)info[1]));
    }
    else if (!UTParamUtil::GetInt(info[info.Nelements() - 1], repeat) ||
             (info.Nelements() >= 5 && !UTParamUtil::GetInt(info[2], index)))
    {
	DPRINTF(D_ERROR, ("Failed to convert parameters\n"));
    }
    else
    {
        char* p = (pStr == m_pA ? m_pABuffer : m_pBBuffer);
        const char* pValue = (const char*)info[info.Nelements()-2];
        size_t len = strlen(pValue);

        for (p += index; repeat > 0; p += len, --repeat)
            memcpy (p, pValue, len); /* Flawfinder: ignore */
        
        ret = true;
    }

    return ret;
}

bool HLXStringTest::HandleBufferEndStringCmd(const UTVector<UTString>& info)
{
    bool ret = false;
    CHXString* pStr = GetStringObj(info[1]);
    int index = 0;

    if (!pStr)
    {
	DPRINTF(D_ERROR, ("Failed to get the string object '%s'\n", 
			  (const char*)info[1]));
    }
    else if (!UTParamUtil::GetInt(info[2], index))
    {
	DPRINTF(D_ERROR, ("Failed to convert parameters\n"));
    }
    else
    {
        char* p = (pStr == m_pA ? m_pABuffer : m_pBBuffer);
        *(p+index) = 0;
        ret = true;
    }

    return ret;
}

bool HLXStringTest::HandleReleaseBufferCmd(const UTVector<UTString>& info)
{
    bool ret = false;
    CHXString* pStr = GetStringObj(info[1]);
    int length = -9999;

    if (!pStr)
    {
	DPRINTF(D_ERROR, ("Failed to get the string object '%s'\n", 
			  (const char*)info[1]));
    }
    else if (info.Nelements() >= 3 && !UTParamUtil::GetInt(info[2], length))
    {
	DPRINTF(D_ERROR, ("Failed to convert parameters\n"));
    }
    else
    {
        ret = true;
        if (length == -9999) pStr->ReleaseBuffer();
        else pStr->ReleaseBuffer(length);

        if (pStr == m_pA) m_pABuffer = 0;
        else m_pBBuffer = 0;
    }

    return ret;
}

bool HLXStringTest::HandleFreeExtraCmd(const UTVector<UTString>& info)
{
    bool ret = false;
    CHXString* pStr = GetStringObj(info[1]);

    if (!pStr)
    {
	DPRINTF(D_ERROR, ("Failed to get the string object '%s'\n", 
			  (const char*)info[1]));
    }
    else
    {
        ret = true;
        pStr->FreeExtra();
    }

    return ret;
}

bool HLXStringTest::HandleGetAllocLengthCmd(const UTVector<UTString>& info)
{
    bool ret = false;
    CHXString* pStr = GetStringObj(info[1]);
    int expected, maxExpected = -1;

    if (!pStr)
    {
	DPRINTF(D_ERROR, ("Failed to get the string object '%s'\n", 
			  (const char*)info[1]));
    }
    else if (!UTParamUtil::GetInt(info[2], expected) ||
             (info.Nelements() >= 4 && !UTParamUtil::GetInt(info[3], maxExpected)))
    {
	DPRINTF(D_ERROR, ("Failed to convert parameters\n"));
    }
    else if (pStr->GetAllocLength() < expected ||
             pStr->GetAllocLength() > (maxExpected > expected ?
                                       maxExpected : expected))
    {
        if (maxExpected > expected)
            DPRINTF (D_ERROR, ("Got %d expected %d..%d\n",
                               pStr->GetAllocLength(), 
                               expected, maxExpected));
        else
            DPRINTF (D_ERROR, ("Got %d expected %d\n", 
                               pStr->GetAllocLength(), 
                               expected));
    }
    else ret = true;

    return ret;
}

bool HLXStringTest::HandleSetMinBufSizeCmd(const UTVector<UTString>& info)
{
    bool ret = false;
    CHXString* pStr = GetStringObj(info[1]);
    int value, expected;

    if (!pStr)
    {
	DPRINTF(D_ERROR, ("Failed to get the string object '%s'\n", 
			  (const char*)info[1]));
    }
    else if (!UTParamUtil::GetInt(info[2], value) || 
	     !UTParamUtil::GetInt(info[3], expected))
    {
	DPRINTF(D_ERROR, ("Failed to convert parameters\n"));
    }
    else
    {
        INT32 got = pStr->SetMinBufSize(value);
        if (got == expected) ret = true;
	else DPRINTF (D_ERROR, ("Got %d expected %d\n", got, expected));
    }

    return ret;
}

CHXString* HLXStringTest::GetStringObj(const UTString& strName)
{
    CHXString* pRet = 0;

    if (strName == "A")
	pRet = m_pA;
    else if (strName == "B")
	pRet = m_pB;
    else
    {
	DPRINTF(D_ERROR, ("GetStringObj() : get '%s' failed\n",
			  (const char*)strName));
    }

    return pRet;
}

void HLXStringTest::SetStringObj(const UTString& strName, CHXString* pNewStr)
{
    if (strName == "A")
    {
	delete m_pA;
	m_pA = pNewStr;
    }
    else if (strName == "B")
    {
	delete m_pB;
	m_pB = pNewStr;
    }
    else
	DPRINTF(D_ERROR, ("SetStringObj() : set '%s' failed\n",
			  (const char*)strName));
}
