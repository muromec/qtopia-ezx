/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: map_test.cpp,v 1.6 2007/07/06 20:35:03 jfinnecy Exp $
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

#include "./map_test.h"

#include "hx_ut_debug.h"
#include "ut_param_util.h"

#include "./gen_map_test.h"
#include "./param_util.h"

#include "hxmap.h"
#include "hxguidmap.h"

HLXMapTest::HLXMapTest()
{
    m_array.Add(new GenMapTest<CHXMapPtrToPtr, void*, void*>());
    m_array.Add(new GenMapTest<CHXMapStringToOb, CHXString, void*>());
    m_array.Add(new GenMapTest<CHXMapStringToString, CHXString, CHXString>());
    m_array.Add(new GenMapTest<CHXMapLongToObj, LONG32, void*>());
    m_array.Add(new GenMapTest<CHXMapGUIDToObj, GUID, void*>());
}

HLXMapTest::~HLXMapTest()
{
    for (int i = 0; i < m_array.GetSize(); i++)
    {
	delete ((GenMapTestBase*)m_array.GetAt(i));
    }

    m_array.RemoveAll();
}

const char* HLXMapTest::DefaultCommandLine() const
{
    return "tmap tmap.in";
}
    
void HLXMapTest::GetCommandInfo(UTVector<HLXUnitTestCmdInfo*>& cmds)
{
    cmds.Resize(12);
    cmds[0] = new HLXUnitTestCmdInfoDisp<HLXMapTest>(this, 
				    "CreateElement",
				    &HLXMapTest::HandleCreateElCmd,
				    2);
    cmds[1] = new HLXUnitTestCmdInfoDisp<HLXMapTest>(this, 
				    "ClearElements",
				    &HLXMapTest::HandleClearElCmd,
				    1);
    cmds[2] = new HLXUnitTestCmdInfoDisp<HLXMapTest>(this, 
				    "GetCount",
				    &HLXMapTest::HandleGetCountCmd,
				    2);
    cmds[3] = new HLXUnitTestCmdInfoDisp<HLXMapTest>(this, 
				    "IsEmpty",
				    &HLXMapTest::HandleIsEmptyCmd,
				    2);
    cmds[4] = new HLXUnitTestCmdInfoDisp<HLXMapTest>(this, 
				    "Lookup",
				    &HLXMapTest::HandleLookupCmd,
				    3);
    cmds[5] = new HLXUnitTestCmdInfoDisp<HLXMapTest>(this, 
				    "SetAt",
				    &HLXMapTest::HandleSetAtCmd,
				    2);
    cmds[6] = new HLXUnitTestCmdInfoDisp<HLXMapTest>(this, 
				     "RemoveKey",
				     &HLXMapTest::HandleRemoveKeyCmd,
				     3);
    cmds[7] = new HLXUnitTestCmdInfoDisp<HLXMapTest>(this, 
				     "RemoveAll",
				     &HLXMapTest::HandleRemoveAllCmd,
				     1);
    cmds[8] = new HLXUnitTestCmdInfoDisp<HLXMapTest>(this, 
				     "Rhs[]",
				     &HLXMapTest::HandleRhsArrayOpCmd,
				     3);
    cmds[9] = new HLXUnitTestCmdInfoDisp<HLXMapTest>(this, 
				     "Lhs[]",
				     &HLXMapTest::HandleLhsArrayOpCmd,
				     2);
    cmds[10] = new HLXUnitTestCmdInfoDisp<HLXMapTest>(this, 
				     "IsNull",
				     &HLXMapTest::HandleIsNullCmd,
				     3);
    cmds[11] = new HLXUnitTestCmdInfoDisp<HLXMapTest>(this, 
				     "RunMapSpecificTests",
				     &HLXMapTest::HandleRunMapSpecificTestsCmd,
				     1);

}

HLXCmdBasedTest* HLXMapTest::Clone() const
{
    return new HLXMapTest();
}

bool HLXMapTest::HandleCreateElCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    int index = 0;
    if (!UTParamUtil::GetInt(info[1], index))
    {
	DPRINTF(D_ERROR, ("HLXMapTest::HandleCreateElCmd : failed to convert parameter\n"));	
    }
    else
	ret = CreateElement(index);

    return ret;
}

bool HLXMapTest::CreateElement(int index)
{
    bool ret = true;

    for (int i = 0; ret && (i < m_array.GetSize()); i++)
    {
	GenMapTestBase* pObj = (GenMapTestBase*)m_array[i];

	if (pObj)
	    ret = pObj->CreateElement(index);
    }

    return ret;
}

bool HLXMapTest::HandleClearElCmd(const UTVector<UTString>& /*info*/)
{
    return ClearElements();
}

bool HLXMapTest::ClearElements()
{
    bool ret = true;

    for (int i = 0; ret && (i < m_array.GetSize()); i++)
    {
	GenMapTestBase* pObj = (GenMapTestBase*)m_array[i];

	if (pObj)
	    ret = pObj->ClearElements();
    }

    return ret;
}

bool HLXMapTest::HandleGetCountCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    int expected = 0;
    if (!UTParamUtil::GetInt(info[1], expected))
    {
	DPRINTF(D_ERROR, ("HLXMapTest::HandleGetCountCmd : failed to convert parameter\n"));	
    }
    else
	ret = GetCount(expected);

    return ret;
}

bool HLXMapTest::GetCount(int expected)
{
    bool ret = true;

    for (int i = 0; ret && (i < m_array.GetSize()); i++)
    {
	GenMapTestBase* pObj = (GenMapTestBase*)m_array[i];

	if (pObj)
	    ret = pObj->GetCount(expected);
    }

    return ret;
}

bool HLXMapTest::HandleIsEmptyCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    bool expected = false;
    if (!UTParamUtil::GetBool(info[1],expected))
    {
	DPRINTF(D_ERROR, ("HLXMapTest::HandleIsEmptyCmd : failed to convert parameter\n"));	
    }
    else
	ret = IsEmpty(expected);

    return ret;
}

bool HLXMapTest::IsEmpty(bool expected)
{
    bool ret = true;

    for (int i = 0; ret && (i < m_array.GetSize()); i++)
    {
	GenMapTestBase* pObj = (GenMapTestBase*)m_array[i];

	if (pObj)
	    ret = pObj->IsEmpty(expected);
    }

    return ret;
}

bool HLXMapTest::HandleLookupCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    int index = 0;
    bool expected = false;
    if (!UTParamUtil::GetInt(info[1], index) ||
	!UTParamUtil::GetBool(info[2],expected))
    {
	DPRINTF(D_ERROR, ("HLXMapTest::HandleLookupCmd : failed to convert parameter\n"));	
    }
    else
	ret = Lookup(index, expected);

    return ret;
}

bool HLXMapTest::Lookup(int index, bool expected)
{
    bool ret = true;

    for (int i = 0; ret && (i < m_array.GetSize()); i++)
    {
	GenMapTestBase* pObj = (GenMapTestBase*)m_array[i];

	if (pObj)
	    ret = pObj->Lookup(index, expected);
    }

    return ret;
}

bool HLXMapTest::HandleSetAtCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    int index = 0;
    if (!UTParamUtil::GetInt(info[1], index))
    {
	DPRINTF(D_ERROR, ("HLXMapTest::HandleSetCmd : failed to convert parameter\n"));	
    }
    else
	ret = SetAt(index);

    return ret;
}

bool HLXMapTest::SetAt(int index)
{
    bool ret = true;

    for (int i = 0; ret && (i < m_array.GetSize()); i++)
    {
	GenMapTestBase* pObj = (GenMapTestBase*)m_array[i];

	if (pObj)
	    ret = pObj->SetAt(index);
    }
    
    return ret;
}

bool HLXMapTest::HandleRemoveKeyCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    int index = 0;
    bool expected = false;
    if (!UTParamUtil::GetInt(info[1], index) ||
	!UTParamUtil::GetBool(info[2],expected))
    {
	DPRINTF(D_ERROR, ("HLXMapTest::HandleRemoveKeyCmd : failed to convert parameter\n"));	
    }
    else
	ret = RemoveKey(index, expected);

    return ret;
}

bool HLXMapTest::RemoveKey(int index, bool expected)
{
    bool ret = true;

    for (int i = 0; ret && (i < m_array.GetSize()); i++)
    {
	GenMapTestBase* pObj = (GenMapTestBase*)m_array[i];

	if (pObj)
	    ret = pObj->RemoveKey(index, expected);
    }

    return ret;
}

bool HLXMapTest::HandleRemoveAllCmd(const UTVector<UTString>& /*info*/)
{
    bool ret = true;

    for (int i = 0; ret && (i < m_array.GetSize()); i++)
    {
	GenMapTestBase* pObj = (GenMapTestBase*)m_array[i];

	if (pObj)
	    ret = pObj->RemoveAll();
    }

    return ret;
}

bool HLXMapTest::HandleRhsArrayOpCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    int index = 0;
    bool expected = false;
    if (!UTParamUtil::GetInt(info[1], index) ||
	!UTParamUtil::GetBool(info[2], expected))
    {
	DPRINTF(D_ERROR, ("HLXMapTest::HandleRhsArrayOpCmd : failed to convert parameter\n"));	
    }
    else
    {
	ret = RhsArrayOp(index, expected);
    }

    return ret;
}

bool HLXMapTest::RhsArrayOp(int index, bool expected)
{
    bool ret = true;

    for (int i = 0; ret && (i < m_array.GetSize()); i++)
    {
	GenMapTestBase* pObj = (GenMapTestBase*)m_array[i];

	if (pObj)
	{
	    ret = pObj->RhsArrayOp(index, expected);
	}
    }

    return ret;
}

bool HLXMapTest::HandleLhsArrayOpCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    int index = 0;
    if (!UTParamUtil::GetInt(info[1], index))
    {
	DPRINTF(D_ERROR, ("HLXMapTest::HandleLhsArrayOpCmd : failed to convert parameter\n"));	
    }
    else
	ret = LhsArrayOp(index);


    return ret;
}

bool HLXMapTest::LhsArrayOp(int index)
{
    bool ret = true;

    for (int i = 0; ret && (i < m_array.GetSize()); i++)
    {
	GenMapTestBase* pObj = (GenMapTestBase*)m_array[i];

	if (pObj)
	    ret = pObj->LhsArrayOp(index);
    }

    return ret;
}

bool HLXMapTest::HandleIsNullCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    int index = 0;
    bool expected = false;
    if (!UTParamUtil::GetInt(info[1], index) ||
	!UTParamUtil::GetBool(info[2], expected))
    {
	DPRINTF(D_ERROR, ("HLXMapTest::HandleIsNullCmd : failed to convert parameter\n"));	
    }
    else
    {
	ret = IsNull(index, expected);
    }

    return ret;
}

bool HLXMapTest::IsNull(int index, bool expected)
{
    bool ret = true;

    for (int i = 0; ret && (i < m_array.GetSize()); i++)
    {
	GenMapTestBase* pObj = (GenMapTestBase*)m_array[i];

	if (pObj)
	{
	    ret = pObj->IsNull(index, expected);
	}
    }

    return ret;
}

bool HLXMapTest::HandleRunMapSpecificTestsCmd(const UTVector<UTString>& /*info*/)
{
    bool ret = true;

    for (int i = 0; ret && (i < m_array.GetSize()); i++)
    {
	GenMapTestBase* pObj = (GenMapTestBase*)m_array[i];

	if (pObj)
	    ret = pObj->RunMapSpecificTests();
    }

    return ret;
}
