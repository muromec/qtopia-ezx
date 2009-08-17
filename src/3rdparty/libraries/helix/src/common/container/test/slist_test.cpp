/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: slist_test.cpp,v 1.5 2004/07/09 18:21:31 hubbe Exp $
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

#include "./slist_test.h"

#include "hx_ut_debug.h"
#include "ut_param_util.h"

#include "./param_util.h"

HLXSListTest::HLXSListTest() :
    m_pos(0)
{}

HLXSListTest::~HLXSListTest()
{}

const char* HLXSListTest::DefaultCommandLine() const
{
    return "tslist tslist.in";
}
    
void HLXSListTest::GetCommandInfo(UTVector<HLXUnitTestCmdInfo*>& cmds)
{
    cmds.Resize(34);
    cmds[0] = new HLXUnitTestCmdInfoDisp<HLXSListTest>(this, 
				    "GetCount",
				    &HLXSListTest::HandleGetCountCmd,
				    2);
    cmds[1] = new HLXUnitTestCmdInfoDisp<HLXSListTest>(this, 
				    "IsEmpty",
				    &HLXSListTest::HandleIsEmptyCmd,
				    2);
    cmds[2] = new HLXUnitTestCmdInfoDisp<HLXSListTest>(this, 
				    "GetHead",
				    &HLXSListTest::HandleGetHeadCmd,
				    2);
    cmds[3] = new HLXUnitTestCmdInfoDisp<HLXSListTest>(this, 
				    "ReplaceHead",
				    &HLXSListTest::HandleReplaceHeadCmd,
				    2);
    cmds[4] = new HLXUnitTestCmdInfoDisp<HLXSListTest>(this, 
				    "GetTail",
				    &HLXSListTest::HandleGetTailCmd,
				    2);
    cmds[5] = new HLXUnitTestCmdInfoDisp<HLXSListTest>(this, 
				    "ReplaceTail",
				    &HLXSListTest::HandleReplaceTailCmd,
				    2);
    cmds[6] = new HLXUnitTestCmdInfoDisp<HLXSListTest>(this, 
				    "RemoveHead",
				    &HLXSListTest::HandleRemoveHeadCmd,
				    1);
    cmds[7] = new HLXUnitTestCmdInfoDisp<HLXSListTest>(this, 
				    "RemoveTail",
				    &HLXSListTest::HandleRemoveTailCmd,
				    1);
    cmds[8] = new HLXUnitTestCmdInfoDisp<HLXSListTest>(this, 
				    "AddHead",
				    &HLXSListTest::HandleAddHeadCmd,
				    2);
    cmds[9] = new HLXUnitTestCmdInfoDisp<HLXSListTest>(this, 
				    "AddHeadList",
				    &HLXSListTest::HandleAddHeadListCmd,
				    2);
    cmds[10] = new HLXUnitTestCmdInfoDisp<HLXSListTest>(this, 
				    "AddTail",
				    &HLXSListTest::HandleAddTailCmd,
				    2);
    cmds[11] = new HLXUnitTestCmdInfoDisp<HLXSListTest>(this, 
				    "AddTailList",
				    &HLXSListTest::HandleAddTailListCmd,
				    2);
    cmds[12] = new HLXUnitTestCmdInfoDisp<HLXSListTest>(this, 
				    "RemoveAll",
				    &HLXSListTest::HandleRemoveAllCmd,
				    1);
    cmds[13] = new HLXUnitTestCmdInfoDisp<HLXSListTest>(this, 
				    "GetHeadPosition",
				    &HLXSListTest::HandleGetHeadPosCmd,
				    1);
    cmds[14] = new HLXUnitTestCmdInfoDisp<HLXSListTest>(this, 
				    "GetTailPosition",
				    &HLXSListTest::HandleGetTailPosCmd,
				    1);
    cmds[15] = new HLXUnitTestCmdInfoDisp<HLXSListTest>(this, 
				    "GetNext",
				    &HLXSListTest::HandleGetNextCmd,
				    2);
    cmds[16] = new HLXUnitTestCmdInfoDisp<HLXSListTest>(this, 
				    "ReplaceNext",
				    &HLXSListTest::HandleReplaceNextCmd,
				    2);
    cmds[17] = new HLXUnitTestCmdInfoDisp<HLXSListTest>(this, 
				    "GetPrev",
				    &HLXSListTest::HandleGetPrevCmd,
				    2);
    cmds[18] = new HLXUnitTestCmdInfoDisp<HLXSListTest>(this, 
				    "ReplacePrev",
				    &HLXSListTest::HandleReplacePrevCmd,
				    2);
    cmds[19] = new HLXUnitTestCmdInfoDisp<HLXSListTest>(this, 
				    "GetAtNext",
				    &HLXSListTest::HandleGetAtNextCmd,
				    3);
    cmds[20] = new HLXUnitTestCmdInfoDisp<HLXSListTest>(this, 
				    "GetAtPrev",
				    &HLXSListTest::HandleGetAtPrevCmd,
				    3);
    cmds[21] = new HLXUnitTestCmdInfoDisp<HLXSListTest>(this, 
				    "ReplaceAtPrev",
				    &HLXSListTest::HandleReplaceAtPrevCmd,
				    2);
    cmds[22] = new HLXUnitTestCmdInfoDisp<HLXSListTest>(this, 
				    "GetAt",
				    &HLXSListTest::HandleGetAtCmd,
				    2);
    cmds[23] = new HLXUnitTestCmdInfoDisp<HLXSListTest>(this, 
				    "ReplaceAt",
				    &HLXSListTest::HandleReplaceAtCmd,
				    2);
    cmds[24] = new HLXUnitTestCmdInfoDisp<HLXSListTest>(this, 
				    "SetAt",
				    &HLXSListTest::HandleSetAtCmd,
				    2);
    cmds[25] = new HLXUnitTestCmdInfoDisp<HLXSListTest>(this, 
				    "RemoveAt",
				    &HLXSListTest::HandleRemoveAtCmd,
				    1);
    cmds[26] = new HLXUnitTestCmdInfoDisp<HLXSListTest>(this, 
				    "InsertBefore",
				    &HLXSListTest::HandleInsertBeforeCmd,
				    2);
    cmds[27] = new HLXUnitTestCmdInfoDisp<HLXSListTest>(this, 
				    "InsertAfter",
				    &HLXSListTest::HandleInsertAfterCmd,
				    2);
    cmds[28] = new HLXUnitTestCmdInfoDisp<HLXSListTest>(this, 
				    "Find",
				    &HLXSListTest::HandleFindCmd,
				    3);
    cmds[29] = new HLXUnitTestCmdInfoDisp<HLXSListTest>(this, 
				    "FindIndex",
				    &HLXSListTest::HandleFindIndexCmd,
				    2);
    cmds[30] = new HLXUnitTestCmdInfoDisp<HLXSListTest>(this, 
				    "IsPosValid",
				    &HLXSListTest::HandleIsPosValidCmd,
				    2);
    cmds[31] = new HLXUnitTestCmdInfoDisp<HLXSListTest>(this, 
				    "ClearPos",
				    &HLXSListTest::HandleClearPosCmd,
				    1);
    cmds[32] = new HLXUnitTestCmdInfoDisp<HLXSListTest>(this, 
				    "TestIterator",
				    &HLXSListTest::HandleTestIteratorCmd,
				    1);
    cmds[33] = new HLXUnitTestCmdInfoDisp<HLXSListTest>(this, 
				    "Dump",
				    &HLXSListTest::HandleDumpCmd,
				    1, 2);
}

HLXCmdBasedTest* HLXSListTest::Clone() const
{
    return new HLXSListTest();
}

void* HLXSListTest::CreateValue(int val)
{
    return new int(val);
}

void HLXSListTest::DestroyValue(void* ptr)
{
    delete (int*)ptr;
}

bool HLXSListTest::GetValue(void* ptr, int& val)
{
    bool ret = false;

    if (ptr)
    {
	val = *((int*)ptr);
	ret = true;
    }

    return ret;
}

bool HLXSListTest::FindValue(int value, LISTPOSITION pos, void*& pValue)
{
    bool ret = false;

    pValue = 0;

    for(; pos && !pValue; m_list.GetNext(pos))
    {
	int cur = 0;

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

bool HLXSListTest::HandleGetCountCmd(const UTVector<UTString>& info)
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

bool HLXSListTest::HandleIsEmptyCmd(const UTVector<UTString>& info)
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

bool HLXSListTest::HandleGetHeadCmd(const UTVector<UTString>& info)
{
    bool ret = false;
    
    int expected = 0;
    int result = 0;
    if (!UTParamUtil::GetInt(info[1], expected))
    {
	DPRINTF(D_ERROR, ("npSList::HandleGetHeadCmd : failed to convert parameter\n"));
    }
    else if (!GetValue(m_list.GetHead(), result))
    {
	DPRINTF(D_ERROR, ("npSList::HandleGetHeadCmd : failed to get value\n"));
    }
    else if (result != expected)
    {
	DPRINTF(D_ERROR, ("npSList::HandleGetHeadCmd : got %d expected %d\n",
			  result,
			  expected));
    }
    else
	ret = true;

    return ret;
}

bool HLXSListTest::HandleReplaceHeadCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    int value = 0;
    if (!UTParamUtil::GetInt(info[1], value))
    {
	DPRINTF(D_ERROR, ("npSList::HandleReplaceHeadCmd : failed to convert parameter\n"));
    }
    else
    {
	DestroyValue(m_list.GetHead());
	m_list.GetHead() = CreateValue(value);
	ret = true;
    }
    
    return ret;
}

bool HLXSListTest::HandleGetTailCmd(const UTVector<UTString>& info)
{
    bool ret = false;
    
    int expected = 0;
    int result = 0;
    if (!UTParamUtil::GetInt(info[1], expected))
    {
	DPRINTF(D_ERROR, ("npSList::HandleGetTailCmd : failed to convert parameter\n"));
    }
    else if (!GetValue(m_list.GetTail(), result))
    {
	DPRINTF(D_ERROR, ("npSList::HandleGetTailCmd : failed to get value\n"));
    }
    else if (result != expected)
    {
	DPRINTF(D_ERROR, ("npSList::HandleGetTailCmd : got %d expected %d\n",
			  result,
			  expected));
    }
    else
	ret = true;

    return ret;
}

bool HLXSListTest::HandleReplaceTailCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    int value = 0;
    if (!UTParamUtil::GetInt(info[1], value))
    {
	DPRINTF(D_ERROR, ("npSList::HandleReplaceTailCmd : failed to convert parameter\n"));
    }
    else
    {
	DestroyValue(m_list.GetHead());
	m_list.GetTail() = CreateValue(value);
	ret = true;
    }

    return ret;
}

bool HLXSListTest::HandleRemoveHeadCmd(const UTVector<UTString>& /*info*/)
{
    DestroyValue(m_list.RemoveHead());

    return true;
}

bool HLXSListTest::HandleRemoveTailCmd(const UTVector<UTString>& /*info*/)
{
    DestroyValue(m_list.RemoveTail());

    return true;
}

bool HLXSListTest::HandleAddHeadCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    int value = 0;
    if (!UTParamUtil::GetInt(info[1], value))
    {
	DPRINTF(D_ERROR, ("npSList::HandleAddHeadCmd : failed to convert parameter\n"));
    }
    else
    {
	m_list.AddHead(CreateValue(value));
	ret = true;
    }

    return ret;
}

bool HLXSListTest::HandleAddHeadListCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    CHXSimpleList list;
    if (!GetList(info[1], list))
    {
	DPRINTF(D_ERROR, ("npSList::HandleAddHeadListCmd : failed to convert parameter\n"));
    }
    else
    {
	m_list.AddHead(&list);
	ret = true;
    }

    return ret;
}

bool HLXSListTest::HandleAddTailCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    int value = 0;
    if (!UTParamUtil::GetInt(info[1], value))
    {
	DPRINTF(D_ERROR, ("npSList::HandleAddTailCmd : failed to convert parameter\n"));
    }
    else
    {
	m_list.AddTail(CreateValue(value));
	ret = true;
    }

    return ret;
}

bool HLXSListTest::HandleAddTailListCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    CHXSimpleList list;
    if (!GetList(info[1], list))
    {
	DPRINTF(D_ERROR, ("npSList::HandleAddTailListCmd : failed to convert parameter\n"));
    }
    else
    {
	m_list.AddTail(&list);
	ret = true;
    }

    return ret;
}

bool HLXSListTest::HandleRemoveAllCmd(const UTVector<UTString>& /*info*/)
{
    m_list.RemoveAll();

    return true;
}

bool HLXSListTest::HandleGetHeadPosCmd(const UTVector<UTString>& /*info*/)
{
    m_pos = m_list.GetHeadPosition();
    return true;
}

bool HLXSListTest::HandleGetTailPosCmd(const UTVector<UTString>& /*info*/)
{
    m_pos = m_list.GetTailPosition();
    return true;
}

bool HLXSListTest::HandleGetNextCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    int expected = 0;
    int result = 0;
    if (!UTParamUtil::GetInt(info[1], expected))
    {
	DPRINTF(D_ERROR, ("npSList::HandleGetNextCmd : failed to convert parameter\n"));
    }
    else if (!GetValue(m_list.GetNext(m_pos), result))
    {
	DPRINTF(D_ERROR, ("npSList::HandleGetNextCmd : failed to get value\n"));
    }
    else if (result != expected)
    {
	DPRINTF(D_ERROR, ("npSList::HandleGetNextCmd : got %d expected %d\n",
			  result,
			  expected));
    }
    else
	ret = true;

    return ret;
}

bool HLXSListTest::HandleReplaceNextCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    int value = 0;
    if (!UTParamUtil::GetInt(info[1], value))
    {
	DPRINTF(D_ERROR, ("npSList::HandleReplaceNextCmd : failed to convert parameter\n"));
    }
    else
    {
	LISTPOSITION tmpPos = m_pos;
	DestroyValue(m_list.GetNext(tmpPos));
	m_list.GetNext(m_pos) = CreateValue(value);
	ret = true;
    }

    return ret;
}

bool HLXSListTest::HandleGetPrevCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    int expected = 0;
    int result = 0;
    if (!UTParamUtil::GetInt(info[1], expected))
    {
	DPRINTF(D_ERROR, ("npSList::HandleGetPrevCmd : failed to convert parameter\n"));
    }
    else if (!GetValue(m_list.GetPrev(m_pos), result))
    {
	DPRINTF(D_ERROR, ("npSList::HandleGetPrevCmd : failed to get value\n"));
    }
    else if (result != expected)
    {
	DPRINTF(D_ERROR, ("npSList::HandleGetPrevCmd : got %d expected %d\n",
			  result,
			  expected));
    }
    else
	ret = true;

    return ret;
}

bool HLXSListTest::HandleReplacePrevCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    int value = 0;
    if (!UTParamUtil::GetInt(info[1], value))
    {
	DPRINTF(D_ERROR, ("npSList::HandleReplacePrevCmd : failed to convert parameter\n"));
    }
    else
    {
	LISTPOSITION tmpPos = m_pos;
	DestroyValue(m_list.GetPrev(tmpPos));
	m_list.GetPrev(m_pos) = CreateValue(value);
	ret = true;
    }

    return ret;
}

bool HLXSListTest::HandleGetAtNextCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    bool expectValid = false;
    int expected = 0;
    if (!UTParamUtil::GetBool(info[1], expectValid) ||
	!UTParamUtil::GetInt(info[2], expected))
    {
	DPRINTF(D_ERROR, ("npSList::HandleGetAtNextCmd : failed to convert parameter\n"));
    }
    else
    {
	int result = 0;
	bool resultValid = GetValue(m_list.GetAtNext(m_pos), result);

	if (expectValid != resultValid)
	{
	    DPRINTF(D_ERROR, ("npSList::HandleGetAtNextCmd : got valid=%d expected valid=%d\n",
			      resultValid,
			      expectValid));
	}
	else if (expectValid && (result != expected))
	{
	    DPRINTF(D_ERROR, ("npSList::HandleGetAtNextCmd : got %d expected %d\n",
			      result,
			      expected));
	}
	else
	    ret = true;
    }

    return ret;
}

bool HLXSListTest::HandleGetAtPrevCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    bool expectValid = false;
    int expected = 0;
    if (!UTParamUtil::GetBool(info[1], expectValid) ||
	!UTParamUtil::GetInt(info[2], expected))
    {
	DPRINTF(D_ERROR, ("npSList::HandleGetAtPrevCmd : failed to convert parameter\n"));
    }
    else
    {
	int result = 0;
	bool resultValid = GetValue(m_list.GetAtPrev(m_pos), result);

	if (expectValid != resultValid)
	{
	    DPRINTF(D_ERROR, ("npSList::HandleGetAtPrevCmd : got valid=%d expected valid=%d\n",
			      resultValid,
			      expectValid));
	}
	else if (expectValid && (result != expected))
	{
	    DPRINTF(D_ERROR, ("npSList::HandleGetAtPrevCmd : got %d expected %d\n",
			      result,
			      expected));
	}
	else
	    ret = true;
    }

    return ret;
}

bool HLXSListTest::HandleReplaceAtPrevCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    int value = 0;
    if (!UTParamUtil::GetInt(info[1], value))
    {
	DPRINTF(D_ERROR, ("npSList::HandleReplaceAtPrevCmd : failed to convert parameter\n"));
    }
    else
    {
	LISTPOSITION tmpPos = m_pos;
	DestroyValue(m_list.GetAtPrev(tmpPos));
	m_list.GetAtPrev(m_pos) = CreateValue(value);
	ret = true;
    }


    return ret;
}

bool HLXSListTest::HandleGetAtCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    int expected = 0;
    if (!UTParamUtil::GetInt(info[1], expected))
    {
	DPRINTF(D_ERROR, ("npSList::HandleGetAtCmd : failed to convert parameter\n"));
    }
    else
    {
	int result = 0;

	if (!GetValue(m_list.GetAt(m_pos), result))
	{
	    DPRINTF(D_ERROR, ("npSList::HandleGetAtCmd : invalid value\n"));
	}
	else if (result != expected)
	{
	    DPRINTF(D_ERROR, ("npSList::HandleGetAtCmd : got %d expected %d\n",
			      result,
			      expected));
	}
	else
	    ret = true;
    }

    return ret;
}

bool HLXSListTest::HandleReplaceAtCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    int value = 0;
    if (!UTParamUtil::GetInt(info[1], value))
    {
	DPRINTF(D_ERROR, ("npSList::HandleReplaceAtCmd : failed to convert parameter\n"));
    }
    else
    {
	DestroyValue(m_list.GetAt(m_pos));
	m_list.GetAt(m_pos) = CreateValue(value);
	ret = true;
    }
    
    return ret;
}

bool HLXSListTest::HandleSetAtCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    int value = 0;
    if (!UTParamUtil::GetInt(info[1], value))
    {
	DPRINTF(D_ERROR, ("npSList::HandleSetAtCmd : failed to convert parameter\n"));
    }
    else
    {
	DestroyValue(m_list.GetAt(m_pos));
	m_list.SetAt(m_pos, CreateValue(value));
	ret = true;
    }

    return ret;
}

bool HLXSListTest::HandleRemoveAtCmd(const UTVector<UTString>& /*info*/)
{
    m_pos = m_list.RemoveAt(m_pos);

    return true;
}

bool HLXSListTest::HandleInsertBeforeCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    int value = 0;
    if (!UTParamUtil::GetInt(info[1], value))
    {
	DPRINTF(D_ERROR, ("npSList::HandleInsertBeforeCmd : failed to convert parameter\n"));
    }
    else
    {
	m_pos = m_list.InsertBefore(m_pos, CreateValue(value));
	ret = true;
    }

    return ret;
}

bool HLXSListTest::HandleInsertAfterCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    int value = 0;
    if (!UTParamUtil::GetInt(info[1], value))
    {
	DPRINTF(D_ERROR, ("npSList::HandleInsertAfterCmd : failed to convert parameter\n"));
    }
    else
    {
	m_pos = m_list.InsertAfter(m_pos, CreateValue(value));
	ret = true;
    }

    return ret;
}

bool HLXSListTest::HandleFindCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    int value = 0;
    bool useCurrentPos = false;
    if (!UTParamUtil::GetInt(info[1], value) ||
	!UTParamUtil::GetBool(info[2], useCurrentPos))
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

bool HLXSListTest::HandleFindIndexCmd(const UTVector<UTString>& info)
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

bool HLXSListTest::HandleIsPosValidCmd(const UTVector<UTString>& info)
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

bool HLXSListTest::HandleClearPosCmd(const UTVector<UTString>& /*info*/)
{
    m_pos = 0;

    return true;
}

bool HLXSListTest::HandleTestIteratorCmd(const UTVector<UTString>& /*info*/)
{
    bool ret = true;

    LISTPOSITION pos = m_list.GetHeadPosition();
    CHXSimpleList::Iterator itr = m_list.Begin();

    for(; ret && (itr != m_list.End()) && pos; ++itr)
    {
	int expected = 0;
	int result = 0;

	if (!GetValue(m_list.GetNext(pos), expected))
	{
	    DPRINTF (D_ERROR,("HLXSListTest::HandleTestIteratorCmd() : failed to get expected value\n"));
	    ret = false;
	}
	else if (!GetValue(*itr, result))
	{
	    DPRINTF (D_ERROR,("HLXSListTest::HandleTestIteratorCmd() : failed to get result value\n"));
	    ret = false;
	}
	else if (expected != result)
	{
	    DPRINTF (D_ERROR,("HLXSListTest::HandleTestIteratorCmd() : got %d expected %d\n",
			      result,
			      expected));
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
		DPRINTF (D_ERROR,("HLXSListTest::HandleTestIteratorCmd() : Position is not at the end\n"));
		ret = false;
	    }
	}
	else
	{
	    DPRINTF (D_ERROR,("HLXSListTest::HandleTestIteratorCmd() : iterator not at the end\n"));
	    ret = false;
	}
    }

    return ret;
}

bool HLXSListTest::HandleDumpCmd(const UTVector<UTString>& info)
{
    if (info.Nelements() >= 2) m_list.Dump((const char*)info[1]);
    else m_list.Dump();
    return true;
}
