/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: strlist_test.cpp,v 1.5 2004/07/09 18:21:31 hubbe Exp $
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

#include "./strlist_test.h"

#include "hx_ut_debug.h"
#include "ut_param_util.h"

#include "./param_util.h"

HLXStrListTest::HLXStrListTest() :
    m_pos(0)
{}

HLXStrListTest::~HLXStrListTest()
{}

const char* HLXStrListTest::DefaultCommandLine() const
{
    return "tstrlist tstrlist.in";
}
    
void HLXStrListTest::GetCommandInfo(UTVector<HLXUnitTestCmdInfo*>& cmds)
{
    cmds.Resize(36);
    cmds[0] = new HLXUnitTestCmdInfoDisp<HLXStrListTest>(this, 
				    "GetCount",
				    &HLXStrListTest::HandleGetCountCmd,
				    2);
    cmds[1] = new HLXUnitTestCmdInfoDisp<HLXStrListTest>(this, 
				    "IsEmpty",
				    &HLXStrListTest::HandleIsEmptyCmd,
				    2);
    cmds[2] = new HLXUnitTestCmdInfoDisp<HLXStrListTest>(this, 
				    "GetHead",
				    &HLXStrListTest::HandleGetHeadCmd,
				    2);
    cmds[3] = new HLXUnitTestCmdInfoDisp<HLXStrListTest>(this, 
				    "ReplaceHead",
				    &HLXStrListTest::HandleReplaceHeadCmd,
				    2);
    cmds[4] = new HLXUnitTestCmdInfoDisp<HLXStrListTest>(this, 
				    "GetTail",
				    &HLXStrListTest::HandleGetTailCmd,
				    2);
    cmds[5] = new HLXUnitTestCmdInfoDisp<HLXStrListTest>(this, 
				    "ReplaceTail",
				    &HLXStrListTest::HandleReplaceTailCmd,
				    2);
    cmds[6] = new HLXUnitTestCmdInfoDisp<HLXStrListTest>(this, 
				    "RemoveHead",
				    &HLXStrListTest::HandleRemoveHeadCmd,
				    1);
    cmds[7] = new HLXUnitTestCmdInfoDisp<HLXStrListTest>(this, 
				    "RemoveTail",
				    &HLXStrListTest::HandleRemoveTailCmd,
				    1);
    cmds[8] = new HLXUnitTestCmdInfoDisp<HLXStrListTest>(this, 
				    "RemoveAll",
				    &HLXStrListTest::HandleRemoveAllCmd,
				    1);
    cmds[9] = new HLXUnitTestCmdInfoDisp<HLXStrListTest>(this, 
				    "GetHeadPosition",
				    &HLXStrListTest::HandleGetHeadPosCmd,
				    1);
    cmds[10] = new HLXUnitTestCmdInfoDisp<HLXStrListTest>(this, 
				    "GetTailPosition",
				    &HLXStrListTest::HandleGetTailPosCmd,
				    1);
    cmds[11] = new HLXUnitTestCmdInfoDisp<HLXStrListTest>(this, 
				    "GetNext",
				    &HLXStrListTest::HandleGetNextCmd,
				    2);
    cmds[12] = new HLXUnitTestCmdInfoDisp<HLXStrListTest>(this, 
				    "GetPrev",
				    &HLXStrListTest::HandleGetPrevCmd,
				    2);
    cmds[13] = new HLXUnitTestCmdInfoDisp<HLXStrListTest>(this, 
				    "ReplacePrev",
				    &HLXStrListTest::HandleReplacePrevCmd,
				    2);
    cmds[14] = new HLXUnitTestCmdInfoDisp<HLXStrListTest>(this, 
				    "GetAtNext",
				    &HLXStrListTest::HandleGetAtNextCmd,
				    3);
    cmds[15] = new HLXUnitTestCmdInfoDisp<HLXStrListTest>(this, 
				    "GetAtPrev",
				    &HLXStrListTest::HandleGetAtPrevCmd,
				    3);
    cmds[16] = new HLXUnitTestCmdInfoDisp<HLXStrListTest>(this, 
				    "ReplaceAtPrev",
				    &HLXStrListTest::HandleReplaceAtPrevCmd,
				    2);
    cmds[17] = new HLXUnitTestCmdInfoDisp<HLXStrListTest>(this, 
				    "GetAt",
				    &HLXStrListTest::HandleGetAtCmd,
				    2);
    cmds[18] = new HLXUnitTestCmdInfoDisp<HLXStrListTest>(this, 
				    "ReplaceAt",
				    &HLXStrListTest::HandleReplaceAtCmd,
				    2);
    cmds[19] = new HLXUnitTestCmdInfoDisp<HLXStrListTest>(this, 
				    "SetAt",
				    &HLXStrListTest::HandleSetAtCmd,
				    2);
    cmds[20] = new HLXUnitTestCmdInfoDisp<HLXStrListTest>(this, 
				    "RemoveAt",
				    &HLXStrListTest::HandleRemoveAtCmd,
				    1);
    cmds[21] = new HLXUnitTestCmdInfoDisp<HLXStrListTest>(this, 
				    "InsertBefore",
				    &HLXStrListTest::HandleInsertBeforeCmd,
				    2);
    cmds[22] = new HLXUnitTestCmdInfoDisp<HLXStrListTest>(this, 
				    "InsertAfter",
				    &HLXStrListTest::HandleInsertAfterCmd,
				    2);
    cmds[23] = new HLXUnitTestCmdInfoDisp<HLXStrListTest>(this, 
				    "Find",
				    &HLXStrListTest::HandleFindCmd,
				    3);
    cmds[24] = new HLXUnitTestCmdInfoDisp<HLXStrListTest>(this, 
				    "FindIndex",
				    &HLXStrListTest::HandleFindIndexCmd,
				    2);
    cmds[25] = new HLXUnitTestCmdInfoDisp<HLXStrListTest>(this, 
				    "IsPosValid",
				    &HLXStrListTest::HandleIsPosValidCmd,
				    2);
    cmds[26] = new HLXUnitTestCmdInfoDisp<HLXStrListTest>(this, 
				    "ClearPos",
				    &HLXStrListTest::HandleClearPosCmd,
				    1);
    cmds[27] = new HLXUnitTestCmdInfoDisp<HLXStrListTest>(this, 
				    "TestIterator",
				    &HLXStrListTest::HandleTestIteratorCmd,
				    1);
    cmds[28] = new HLXUnitTestCmdInfoDisp<HLXStrListTest>(this, 
				    "AddHeadString",
				    &HLXStrListTest::HandleAddHeadStringCmd,
				    2);
    cmds[29] = new HLXUnitTestCmdInfoDisp<HLXStrListTest>(this, 
				    "AddTailString",
				    &HLXStrListTest::HandleAddTailStringCmd,
				    2);
    cmds[30] = new HLXUnitTestCmdInfoDisp<HLXStrListTest>(this, 
				    "AddStringAlphabetic",
				    &HLXStrListTest::HandleAddStringAlphabeticCmd,
				    2);
    cmds[31] = new HLXUnitTestCmdInfoDisp<HLXStrListTest>(this, 
				    "FindString",
				    &HLXStrListTest::HandleFindStringCmd,
				    4);
    cmds[32] = new HLXUnitTestCmdInfoDisp<HLXStrListTest>(this, 
				    "FindPrefixSubstring",
				    &HLXStrListTest::HandleFindPrefixSubstringCmd,
				    4);
    cmds[33] = new HLXUnitTestCmdInfoDisp<HLXStrListTest>(this, 
				    "RemoveHeadString",
				    &HLXStrListTest::HandleRemoveHeadStringCmd,
				    1);
    cmds[34] = new HLXUnitTestCmdInfoDisp<HLXStrListTest>(this, 
				    "RemoveTailString",
				    &HLXStrListTest::HandleRemoveTailStringCmd,
				    1);
    cmds[35] = new HLXUnitTestCmdInfoDisp<HLXStrListTest>(this, 
				    "Dump",
				    &HLXStrListTest::HandleDumpCmd,
				    1, 2);
}

HLXCmdBasedTest* HLXStrListTest::Clone() const
{
    return new HLXStrListTest();
}

CHXString* HLXStrListTest::CreateValue(const char* pStr)
{
    return new CHXString(pStr);
}

void HLXStrListTest::DestroyValue(void* pStr)
{
    delete (CHXString*)pStr;
}

bool HLXStrListTest::GetValue(void* pStr, CHXString& val)
{
    bool ret = false;

    if (pStr)
    {
	val = *((CHXString*)pStr);
	ret = true;
    }

    return ret;
}

bool HLXStrListTest::FindValue(const CHXString& value, 
			      LISTPOSITION pos, void*& pValue)
{
    bool ret = false;

    pValue = 0;

    for(; pos && !pValue; m_list.GetNext(pos))
    {
	CHXString cur;

	if ((GetValue(m_list.GetAt(pos), cur)) &&
	    (cur == value))
	    pValue = m_list.GetAt(pos);
    }

    if (pValue == 0)
    {
	pValue = CreateValue(value);
	ret = true;
    }

    return ret;
}

bool HLXStrListTest::HandleGetCountCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    int expected = 0;

    if (!UTParamUtil::GetInt(info[1], expected))
    {
	DPRINTF(D_ERROR, ("npSList::HandleGetCountCmd : failed to convert parameter\n"));
    }
    else if (m_list.GetCount() != expected)
    {
	DPRINTF(D_ERROR, ("npSList::HandleGetCountCmd : got %d expected %d\n",
			  m_list.GetCount(),
			  expected));
    }
    else
	ret = true;

    return ret;
}

bool HLXStrListTest::HandleIsEmptyCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    bool expected = false;

    if (!UTParamUtil::GetBool(info[1], expected))
    {
	DPRINTF(D_ERROR, ("npSList::HandleIsEmptyCmd : failed to convert parameter\n"));
    }
    else if ((m_list.IsEmpty() == TRUE) != expected)
    {
	DPRINTF(D_ERROR, ("npSList::HandleIsEmptyCmd : got %d expected %d\n",
			  m_list.IsEmpty(),
			  expected));
    }
    else
	ret = true;
    
    return ret;
}

bool HLXStrListTest::HandleGetHeadCmd(const UTVector<UTString>& info)
{
    bool ret = false;
    
    CHXString expected(info[1]);
    CHXString result;
    if (!GetValue(m_list.GetHead(), result))
    {
	DPRINTF(D_ERROR, ("npSList::HandleGetHeadCmd : failed to get value\n"));
    }
    else if (result != expected)
    {
	DPRINTF(D_ERROR, ("npSList::HandleGetHeadCmd : got '%s' expected '%s'\n",
			  (const char*)result,
			  (const char*)expected));
    }
    else
	ret = true;

    return ret;
}

bool HLXStrListTest::HandleReplaceHeadCmd(const UTVector<UTString>& info)
{
    CHXString value(info[1]);

    DestroyValue(m_list.GetHead());
    m_list.GetHead() = CreateValue(value);
    
    return true;
}

bool HLXStrListTest::HandleGetTailCmd(const UTVector<UTString>& info)
{
    bool ret = false;
    
    CHXString expected(info[1]);
    CHXString result;
    if (!GetValue(m_list.GetTail(), result))
    {
	DPRINTF(D_ERROR, ("npSList::HandleGetTailCmd : failed to get value\n"));
    }
    else if (result != expected)
    {
	DPRINTF(D_ERROR, ("npSList::HandleGetTailCmd : got '%s' expected '%s'\n",
			  (const char*)result,
			  (const char*)expected));
    }
    else
	ret = true;

    return ret;
}

bool HLXStrListTest::HandleReplaceTailCmd(const UTVector<UTString>& info)
{
    CHXString value(info[1]);

    DestroyValue(m_list.GetHead());
    m_list.GetTail() = CreateValue(value);

    return true;
}

bool HLXStrListTest::HandleRemoveHeadCmd(const UTVector<UTString>& /*info*/)
{
    DestroyValue(m_list.RemoveHead());

    return true;
}

bool HLXStrListTest::HandleRemoveTailCmd(const UTVector<UTString>& /*info*/)
{
    DestroyValue(m_list.RemoveTail());

    return true;
}

bool HLXStrListTest::HandleRemoveAllCmd(const UTVector<UTString>& /*info*/)
{
    m_list.RemoveAll();

    return true;
}

bool HLXStrListTest::HandleGetHeadPosCmd(const UTVector<UTString>& /*info*/)
{
    m_pos = m_list.GetHeadPosition();
    return true;
}

bool HLXStrListTest::HandleGetTailPosCmd(const UTVector<UTString>& /*info*/)
{
    m_pos = m_list.GetTailPosition();
    return true;
}

bool HLXStrListTest::HandleGetNextCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    CHXString expected(info[1]);
    CHXString result;
    if (!GetValue(m_list.GetNext(m_pos), result))
    {
	DPRINTF(D_ERROR, ("npSList::HandleGetNextCmd : failed to get value\n"));
    }
    else if (result != expected)
    {
	DPRINTF(D_ERROR, ("npSList::HandleGetNextCmd : got '%s' expected '%s'\n",
			  (const char*)result,
			  (const char*)expected));
    }
    else
	ret = true;

    return ret;
}

bool HLXStrListTest::HandleGetPrevCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    CHXString expected(info[1]);
    CHXString result;
    if (!GetValue(m_list.GetPrev(m_pos), result))
    {
	DPRINTF(D_ERROR, ("npSList::HandleGetPrevCmd : failed to get value\n"));
    }
    else if (result != expected)
    {
	DPRINTF(D_ERROR, ("npSList::HandleGetPrevCmd : got '%s' expected '%s'\n",
			  (const char*)result,
			  (const char*)expected));
    }
    else
	ret = true;

    return ret;
}

bool HLXStrListTest::HandleReplacePrevCmd(const UTVector<UTString>& info)
{
    CHXString value(info[1]);

    LISTPOSITION tmpPos = m_pos;
    DestroyValue(m_list.GetPrev(tmpPos));
    m_list.GetPrev(m_pos) = CreateValue(value);
    return true;
}

bool HLXStrListTest::HandleGetAtNextCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    bool expectValid = false;
    CHXString expected(info[2]);
    if (!UTParamUtil::GetBool(info[1], expectValid))
    {
	DPRINTF(D_ERROR, ("npSList::HandleGetAtNextCmd : failed to convert parameter\n"));
    }
    else
    {
	CHXString result;
	bool resultValid = GetValue(m_list.GetAtNext(m_pos), result);

	if (expectValid != resultValid)
	{
	    DPRINTF(D_ERROR, ("npSList::HandleGetAtNextCmd : got valid=%d expected valid=%d\n",
			      resultValid,
			      expectValid));
	}
	else if (expectValid && (result != expected))
	{
	    DPRINTF(D_ERROR, ("npSList::HandleGetAtNextCmd : got '%s' expected '%s'\n",
			      (const char*)result,
			      (const char*)expected));
	}
	else
	    ret = true;
    }

    return ret;
}

bool HLXStrListTest::HandleGetAtPrevCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    bool expectValid = false;
    CHXString expected(info[2]);
    if (!UTParamUtil::GetBool(info[1], expectValid))
    {
	DPRINTF(D_ERROR, ("npSList::HandleGetAtPrevCmd : failed to convert parameter\n"));
    }
    else
    {
	CHXString result;
	bool resultValid = GetValue(m_list.GetAtPrev(m_pos), result);

	if (expectValid != resultValid)
	{
	    DPRINTF(D_ERROR, ("npSList::HandleGetAtPrevCmd : got valid=%d expected valid=%d\n",
			      resultValid,
			      expectValid));
	}
	else if (expectValid && (result != expected))
	{
	    DPRINTF(D_ERROR, ("npSList::HandleGetAtPrevCmd : got '%s' expected '%s'\n",
			      (const char*)result,
			      (const char*)expected));
	}
	else
	    ret = true;
    }

    return ret;
}

bool HLXStrListTest::HandleReplaceAtPrevCmd(const UTVector<UTString>& info)
{
    CHXString value(info[1]);
    
    LISTPOSITION tmpPos = m_pos;
    DestroyValue(m_list.GetAtPrev(tmpPos));
    m_list.GetAtPrev(m_pos) = CreateValue(value);

    return true;
}

bool HLXStrListTest::HandleGetAtCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    CHXString expected(info[1]);
    CHXString result;

    if (!GetValue(m_list.GetAt(m_pos), result))
    {
	DPRINTF(D_ERROR, ("npSList::HandleGetAtCmd : invalid value\n"));
    }
    else if (result != expected)
    {
	DPRINTF(D_ERROR, ("npSList::HandleGetAtCmd : got '%s' expected '%s'\n",
			  (const char*)result,
			  (const char*)expected));
    }
    else
	ret = true;

    return ret;
}

bool HLXStrListTest::HandleReplaceAtCmd(const UTVector<UTString>& info)
{
    CHXString value(info[1]);
    
    DestroyValue(m_list.GetAt(m_pos));
    m_list.GetAt(m_pos) = CreateValue(value);
    return true;
}

bool HLXStrListTest::HandleSetAtCmd(const UTVector<UTString>& info)
{
    CHXString value(info[1]);

    DestroyValue(m_list.GetAt(m_pos));
    m_list.SetAt(m_pos, CreateValue(value));

    return true;
}

bool HLXStrListTest::HandleRemoveAtCmd(const UTVector<UTString>& /*info*/)
{
    m_pos = m_list.RemoveAt(m_pos);

    return true;
}

bool HLXStrListTest::HandleInsertBeforeCmd(const UTVector<UTString>& info)
{
    CHXString value(info[1]);
    
    m_pos = m_list.InsertBefore(m_pos, CreateValue(value));
    return true;
}

bool HLXStrListTest::HandleInsertAfterCmd(const UTVector<UTString>& info)
{
    CHXString value(info[1]);
    
    m_pos = m_list.InsertAfter(m_pos, CreateValue(value));
    return true;
}

bool HLXStrListTest::HandleFindCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    CHXString value(info[1]);
    bool useCurrentPos = false;
    if (!UTParamUtil::GetBool(info[2], useCurrentPos))
    {
	DPRINTF(D_ERROR, ("npSList::HandleFindCmd : failed to convert parameter\n"));
    }
    else
    {
	LISTPOSITION pos = (useCurrentPos) ? m_pos : m_list.GetHeadPosition();

	void* pValue = 0;
	bool destroyValue = FindValue(value, pos, pValue);

	if (useCurrentPos)
	    m_pos = m_list.Find(pValue, pos);
	else
	    m_pos =  m_list.Find(pValue);

	if (destroyValue)
	    DestroyValue(pValue);

	ret = true;
    }

    return ret;
}

bool HLXStrListTest::HandleFindIndexCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    int index = 0;
    if (!UTParamUtil::GetInt(info[1], index))
    {
	DPRINTF(D_ERROR, ("npSList::HandleFindCmd : failed to convert parameter\n"));
    }
    else
    {
	m_pos = m_list.FindIndex(index);
	ret = true;
    }

    return ret;
}

bool HLXStrListTest::HandleIsPosValidCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    bool expected = false;
    if (!UTParamUtil::GetBool(info[1], expected))
    {
	DPRINTF(D_ERROR, ("npSList::HandleIsPosValidCmd : failed to convert parameter\n"));
    }
    else 
    {
	bool result = (m_pos != 0);
	if (result != expected)
	{
	    DPRINTF(D_ERROR, ("npSList::HandleIsPosValidCmd : got %d expected %d\n",
			      result,
			      expected));
	}
	else
	    ret = true;
    }

    return ret;
}

bool HLXStrListTest::HandleClearPosCmd(const UTVector<UTString>& /*info*/)
{
    m_pos = 0;

    return true;
}

bool HLXStrListTest::HandleTestIteratorCmd(const UTVector<UTString>& /*info*/)
{
    bool ret = true;

    LISTPOSITION pos = m_list.GetHeadPosition();
    CHXSimpleList::Iterator itr = m_list.Begin();

    for(; ret && (itr != m_list.End()) && pos; ++itr)
    {
	CHXString expected;
	CHXString result;

	if (!GetValue(m_list.GetNext(pos), expected))
	{
	    DPRINTF (D_ERROR,("HLXStrListTest::HandleTestIteratorCmd() : failed to get expected value\n"));
	    ret = false;
	}
	else if (!GetValue(*itr, result))
	{
	    DPRINTF (D_ERROR,("HLXStrListTest::HandleTestIteratorCmd() : failed to get result value\n"));
	    ret = false;
	}
	else if (expected != result)
	{
	    DPRINTF (D_ERROR,("HLXStrListTest::HandleTestIteratorCmd() : got '%s' expected '%s'\n",
			      (const char*)result,
			      (const char*)expected));
	    ret = false;
	}
    }

    if (ret)
    {
	// The comparisons are arranged this way so 
	// that I can test the == operator
	if (itr == m_list.End())
	{
	    if (pos != 0)
	    {
		DPRINTF (D_ERROR,("HLXStrListTest::HandleTestIteratorCmd() : Position is not at the end\n"));
		ret = false;
	    }
	}
	else
	{
	    DPRINTF (D_ERROR,("HLXStrListTest::HandleTestIteratorCmd() : iterator not at the end\n"));
	    ret = false;
	}
    }

    return ret;
}

bool HLXStrListTest::HandleAddHeadStringCmd(const UTVector<UTString>& info)
{
    m_list.AddHeadString(info[1]);
    return true;
}

bool HLXStrListTest::HandleAddTailStringCmd(const UTVector<UTString>& info)
{
    m_list.AddTailString(info[1]);
    return true;
}

bool HLXStrListTest::HandleAddStringAlphabeticCmd(const UTVector<UTString>& info)
{
    m_list.AddStringAlphabetic(info[1]);
    return true;
}

bool HLXStrListTest::HandleFindStringCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    bool useCurrentPos = false;
    bool isCaseSensitive = false;
    if (!UTParamUtil::GetBool(info[2], useCurrentPos) ||
	!UTParamUtil::GetBool(info[3], isCaseSensitive))
    {
	DPRINTF(D_ERROR, ("npSList::HandleFindStringCmd : failed to convert parameter\n"));
    }
    else
    {
	LISTPOSITION pos = (useCurrentPos) ? m_pos : 0;

	m_pos = m_list.FindString(info[1], pos, isCaseSensitive);

	ret = true;
    }

    return ret;
}

bool HLXStrListTest::HandleFindPrefixSubstringCmd(const UTVector<UTString>& info)
{
    bool ret = false;
    
    bool useCurrentPos = false;
    bool isCaseSensitive = false;
    if (!UTParamUtil::GetBool(info[2], useCurrentPos) ||
	!UTParamUtil::GetBool(info[3], isCaseSensitive))
    {
	DPRINTF(D_ERROR, ("npSList::HandleFindPrefixSubstringCmd : failed to convert parameter\n"));
    }
    else
    {
	LISTPOSITION pos = (useCurrentPos) ? m_pos : 0;

	m_pos = m_list.FindPrefixSubstring(info[1], pos, isCaseSensitive);

	ret = true;
    }

    return ret;
}

bool HLXStrListTest::HandleRemoveHeadStringCmd(const UTVector<UTString>& /*info*/)
{
    m_list.RemoveHeadString();
    return true;
}

bool HLXStrListTest::HandleRemoveTailStringCmd(const UTVector<UTString>& /*info*/)
{
    m_list.RemoveTailString();
    return true;
}

bool HLXStrListTest::HandleDumpCmd(const UTVector<UTString>& info)
{
    if (info.Nelements() >= 2) m_list.Dump((const char*)info[1]);
    else m_list.Dump();
    return true;
}
