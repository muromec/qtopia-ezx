/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: ptr_array_test.cpp,v 1.5 2004/07/09 18:21:31 hubbe Exp $
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

#include "./ptr_array_test.h"

#include "hx_ut_debug.h"
#include "ut_param_util.h"
#include "./param_util.h"

HLXPtrArrayTest::HLXPtrArrayTest()
{}

const char* HLXPtrArrayTest::DefaultCommandLine() const
{
    return "tptrarray tptrarray.in";
}
    
void HLXPtrArrayTest::GetCommandInfo(UTVector<HLXUnitTestCmdInfo*>& cmds)
{
    cmds.Resize(16);

    cmds[0] = new HLXUnitTestCmdInfoDisp<HLXPtrArrayTest>(this, 
				    "IsEmpty",
				    &HLXPtrArrayTest::HandleIsEmptyCmd,
				    2);
    cmds[1] = new HLXUnitTestCmdInfoDisp<HLXPtrArrayTest>(this, 
				    "GetSize",
				    &HLXPtrArrayTest::HandleGetSizeCmd,
				    2);
    cmds[2] = new HLXUnitTestCmdInfoDisp<HLXPtrArrayTest>(this, 
				    "GetUpperBound",
				    &HLXPtrArrayTest::HandleGetUpperBoundCmd,
				    2);

    cmds[3] = new HLXUnitTestCmdInfoDisp<HLXPtrArrayTest>(this, 
				    "SetSize",
				    &HLXPtrArrayTest::HandleSetSizeCmd,
				    3);
    cmds[4] = new HLXUnitTestCmdInfoDisp<HLXPtrArrayTest>(this, 
				    "FreeExtra",
				    &HLXPtrArrayTest::HandleFreeExtraCmd,
				    1);
    cmds[5] = new HLXUnitTestCmdInfoDisp<HLXPtrArrayTest>(this, 
				    "RemoveAll",
				    &HLXPtrArrayTest::HandleRemoveAllCmd,
				    1);
    cmds[6] = new HLXUnitTestCmdInfoDisp<HLXPtrArrayTest>(this, 
				    "GetAt",
				    &HLXPtrArrayTest::HandleGetAtCmd,
				    3);
    cmds[7] = new HLXUnitTestCmdInfoDisp<HLXPtrArrayTest>(this, 
				    "SetAt",
				    &HLXPtrArrayTest::HandleSetAtCmd,
				    3);
    cmds[8] = new HLXUnitTestCmdInfoDisp<HLXPtrArrayTest>(this, 
				    "ElementAt",
				    &HLXPtrArrayTest::HandleElementAtCmd,
				    3);
    cmds[9] = new HLXUnitTestCmdInfoDisp<HLXPtrArrayTest>(this, 
				    "SetAtGrow",
				    &HLXPtrArrayTest::HandleSetAtGrowCmd,
				    3);
    cmds[10] = new HLXUnitTestCmdInfoDisp<HLXPtrArrayTest>(this, 
				    "Add",
				    &HLXPtrArrayTest::HandleAddCmd,
				    2);
    cmds[11] = new HLXUnitTestCmdInfoDisp<HLXPtrArrayTest>(this, 
				    "ArrayOp",
				    &HLXPtrArrayTest::HandleArrayOpCmd,
				    3);
    cmds[12] = new HLXUnitTestCmdInfoDisp<HLXPtrArrayTest>(this, 
				    "InsertAt",
				    &HLXPtrArrayTest::HandleInsertAtCmd,
				    4);
    cmds[13] = new HLXUnitTestCmdInfoDisp<HLXPtrArrayTest>(this, 
				    "RemoveAt",
				    &HLXPtrArrayTest::HandleRemoveAtCmd,
				    3);
    cmds[14] = new HLXUnitTestCmdInfoDisp<HLXPtrArrayTest>(this, 
				    "InsertArrayAt",
				    &HLXPtrArrayTest::HandleInsertArrayAtCmd,
				    3);
    cmds[15] = new HLXUnitTestCmdInfoDisp<HLXPtrArrayTest>(this, 
				    "IsNull",
				    &HLXPtrArrayTest::HandleIsNullCmd,
				    3);
}

HLXCmdBasedTest* HLXPtrArrayTest::Clone() const
{
    return new HLXPtrArrayTest();
}

bool HLXPtrArrayTest::HandleIsEmptyCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    bool expected = false;

    if (!UTParamUtil::GetBool(info[1], expected))
    {
	DPRINTF(D_ERROR, ("HLXPtrArrayTest::HandleIsEmptyCmd : failed to convert parameter\n"));	
    }
    else if ((m_array.IsEmpty() == TRUE) != expected)
    {
	DPRINTF(D_ERROR, ("HLXPtrArrayTest::HandleIsEmptyCmd : got %d expected %d\n",
			  m_array.IsEmpty(),
			  expected));
    }
    else
	ret = true;

    return ret;
}

bool HLXPtrArrayTest::HandleGetSizeCmd(const UTVector<UTString>& info)
{
    bool ret = false;
    
    int expected = 0;
    if (!UTParamUtil::GetInt(info[1], expected))
    {
	DPRINTF (D_ERROR, ("HLXPtrArrayTest::HandleGetSizeCmd() : failed parameter conversion\n"));
    }
    else if (expected != m_array.GetSize())
    {
	DPRINTF (D_ERROR, ("HLXPtrArrayTest::HandleGetSizeCmd() : got %d expected %d\n",
			   m_array.GetSize(),
			   expected));
    }
    else
	ret = true;

    return ret;
}

bool HLXPtrArrayTest::HandleGetUpperBoundCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    int expected = 0;
    if (!UTParamUtil::GetInt(info[1], expected))
    {
	DPRINTF (D_ERROR, ("HLXPtrArrayTest::HandleGetUpperBoundCmd() : failed parameter conversion\n"));
    }
    else if (expected != m_array.GetUpperBound())
    {
	DPRINTF (D_ERROR, ("HLXPtrArrayTest::HandleGetUpperBoundCmd() : got %d expected%d\n",
			   m_array.GetUpperBound(),
			   expected));
    }
    else
	ret = true;

    return ret;
}

bool HLXPtrArrayTest::HandleSetSizeCmd(const UTVector<UTString>& info)
{
    bool ret = false;
    
    int newSize = 0;
    int growSize = 0;

    if (!UTParamUtil::GetInt(info[1], newSize) ||
	!UTParamUtil::GetInt(info[2], growSize))
    {
	DPRINTF (D_ERROR, ("HLXPtrArrayTest::HandleSetSizeCmd : failed to convert parameters\n"));
    }
    else 
    {
	m_array.SetSize(newSize, growSize);
	ret = true;
    }

    return ret;
}

bool HLXPtrArrayTest::HandleFreeExtraCmd(const UTVector<UTString>& /*info*/)
{
    m_array.FreeExtra();

    return true;
}

bool HLXPtrArrayTest::HandleRemoveAllCmd(const UTVector<UTString>& /*info*/)
{
    m_array.RemoveAll();

    return true;
}

bool HLXPtrArrayTest::HandleGetAtCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    int index = 0;
    int expected = 0;

    if (!UTParamUtil::GetInt(info[1], index) ||
	!UTParamUtil::GetInt(info[2], expected))
    {
	DPRINTF (D_ERROR, ("HLXPtrArrayTest::HandleGetAtCmd : failed to convert parameters\n"));
    }
    else if (!m_array.GetAt(index))
    {
	DPRINTF (D_ERROR, ("HLXPtrArrayTest::HandleGetAtCmd : index %d does not contain a value\n", 
			   index));
    }
    else
    {
	int result = *((int*)m_array.GetAt(index));

	if (result != expected)
	{
	    DPRINTF (D_ERROR, ("HLXPtrArrayTest::HandleGetAtCmd : got %d expected %d\n",
			       result, 
			       expected));
	}
	else
	    ret = true;
    }

    return ret;
}

bool HLXPtrArrayTest::HandleSetAtCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    int index = 0;
    int value = 0;
    if (!UTParamUtil::GetInt(info[1], index) ||
	!UTParamUtil::GetInt(info[2], value))
    {
	DPRINTF (D_ERROR, ("HLXPtrArrayTest::HandleSetAtCmd : failed to convert parameters\n"));
    }
    else 
    {
	m_array.SetAt(index, new int(value));
	ret = true;
    }

    return ret;
}

bool HLXPtrArrayTest::HandleElementAtCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    int index = 0;
    int expected = 0;

    if (!UTParamUtil::GetInt(info[1], index) ||
	!UTParamUtil::GetInt(info[2], expected))
    {
	DPRINTF (D_ERROR, ("HLXPtrArrayTest::HandleElementAtCmd : failed to convert parameters\n"));
    }
    else 
    {
	int result = *((int*)m_array.ElementAt(index));

	if (result != expected)
	{
	    DPRINTF (D_ERROR, ("HLXPtrArrayTest::HandleElementAtCmd : got %d expected %d\n",
			       result, 
			       expected));
	}
	else
	    ret = true;
    }

    return ret;
}

bool HLXPtrArrayTest::HandleSetAtGrowCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    int index = 0;
    int value = 0;
    if (!UTParamUtil::GetInt(info[1], index) ||
	!UTParamUtil::GetInt(info[2], value))
    {
	DPRINTF (D_ERROR, ("HLXPtrArrayTest::HandleSetAtGrowCmd : failed to convert parameters\n"));
    }
    else 
    {
	m_array.SetAtGrow(index, new int(value));
	ret = true;
    }

    return ret;
}

bool HLXPtrArrayTest::HandleAddCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    int value = 0;

    if (!UTParamUtil::GetInt(info[1], value))
    {
	DPRINTF(D_ERROR,("HLXPtrArrayTest::HandleAddCmd : failed parameter conversion\n"));
    }
    else
    {
	m_array.Add(new int(value));
	ret = true;
    }


    return ret;
}

bool HLXPtrArrayTest::HandleArrayOpCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    int index = 0;
    int expected = 0;

    if (!UTParamUtil::GetInt(info[1], index) ||
	!UTParamUtil::GetInt(info[2], expected))
    {
	DPRINTF (D_ERROR, ("HLXPtrArrayTest::HandleArrayOpCmd : failed to convert parameters\n"));
    }
    else 
    {
	int result = *((int*)m_array[index]);

	if (result != expected)
	{
	    DPRINTF (D_ERROR, ("HLXPtrArrayTest::HandleArrayOpCmd : got %d expected %d\n",
			       result, 
			       expected));
	}
	else
	    ret = true;
    }

    return ret;
}

bool HLXPtrArrayTest::HandleInsertAtCmd(const UTVector<UTString>& info)
{
    bool ret = false;
        
    int index = 0;
    int value = 0;
    int count = 0;

    if (!UTParamUtil::GetInt(info[1], index) ||
	!UTParamUtil::GetInt(info[2], value) ||
	!UTParamUtil::GetInt(info[3], count))
    {
	DPRINTF (D_ERROR, ("HLXPtrArrayTest::HandleInsertAtCmd : failed to convert parameters\n"));
    }
    else 
    {
	m_array.InsertAt(index, new int(value), count);
	ret = true;
    }

    return ret;
}

bool HLXPtrArrayTest::HandleRemoveAtCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    int index = 0;
    int count = 0;
    if (!UTParamUtil::GetInt(info[1], index) ||
	!UTParamUtil::GetInt(info[2], count))
    {
	DPRINTF (D_ERROR, ("HLXPtrArrayTest::HandleRemoveAtCmd : failed to convert parameters\n"));
    }
    else 
    {
	m_array.RemoveAt(index, count);
	ret = true;
    }
    return ret;
}

bool HLXPtrArrayTest::HandleInsertArrayAtCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    int index = 0;
    CHXPtrArray array;
    
    if (!UTParamUtil::GetInt(info[1], index) ||
	!GetArray(info[2], array))
    {
	DPRINTF (D_ERROR, ("HLXPtrArrayTest::HandlInsertArrayAtCmd : failed to convert parameters\n"));
    }
    else
    {
	m_array.InsertAt(index, &array);
	ret = true;
    }

    return ret;
}

bool HLXPtrArrayTest::HandleIsNullCmd(const UTVector<UTString>& info)
{
    bool ret = false;
    
    int index = 0;
    bool expected = false;

    if (!UTParamUtil::GetInt(info[1], index) ||
	!UTParamUtil::GetBool(info[2], expected))
    {
	DPRINTF (D_ERROR, ("HLXPtrArrayTest::HandleIsNullCmd : failed to convert parameters\n"));
    }
    else
    {
	bool result = (m_array.GetAt(index) == 0);

	if (result != expected)
	{
	    DPRINTF (D_ERROR, ("HLXPtrArrayTest::HandleIsNullCmd : got %d expected %d\n",
			       result,
			       expected));
	}
	else
	    ret = true;
    }

    return ret;
}
