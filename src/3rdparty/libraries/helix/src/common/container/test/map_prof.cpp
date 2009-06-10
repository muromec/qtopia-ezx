/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: map_prof.cpp,v 1.8 2007/07/06 20:35:03 jfinnecy Exp $
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

#include "./map_prof.h"

#include "hx_ut_debug.h"
#include "ut_param_util.h"

#include "./gen_map_prof.h"
#include "./param_util.h"

#include "chxmapptrtoptr.h"
#include "chxmapstringtoob.h"
#include "hxmap.h"
#include "hxguidmap.h"

#ifndef _WINCE
#include <errno.h>
#ifndef _WINDOWS
#include <sys/time.h>
#endif
#endif //_WINCE
#include <time.h>

HLXMapProf::HLXMapProf() : m_memCheckpoint(NULL), m_timeCheckpointSec(0), m_timeCheckpointMicro(0)
{
    m_typeArray = new CHXString[2];
    m_array.Add(new GenMapProf<CHXMapPtrToPtr, void*, void*>());
    m_typeArray[0] = "CHXMapPtrToPtr";
}

HLXMapProf::~HLXMapProf()
{
    for (int i = 0; i < m_array.GetSize(); i++)
    {
	delete ((GenMapProfBase*)m_array.GetAt(i));
    }

    m_array.RemoveAll();

    delete[] m_typeArray;
}
    
const char* HLXMapProf::DefaultCommandLine() const
{
    return "pmap pmap.in";
}

void HLXMapProf::GetCommandInfo(UTVector<HLXUnitTestCmdInfo*>& cmds)
{
    int i = 0;
    cmds.Resize(18);
    cmds[i++] = new HLXUnitTestCmdInfoDisp<HLXMapProf>(this, 
				    "MemCheckpoint",
				    &HLXMapProf::HandleMemCheckpointCmd,
				    1);
    cmds[i++] = new HLXUnitTestCmdInfoDisp<HLXMapProf>(this, 
				    "TimeCheckpoint",
				    &HLXMapProf::HandleTimeCheckpointCmd,
				    1);
    cmds[i++] = new HLXUnitTestCmdInfoDisp<HLXMapProf>(this, 
				    "SetSeed",
				    &HLXMapProf::HandleSetSeedCmd,
				    2);
    cmds[i++] = new HLXUnitTestCmdInfoDisp<HLXMapProf>(this, 
				    "CreateElement",
				    &HLXMapProf::HandleCreateElCmd,
				    2);
    cmds[i++] = new HLXUnitTestCmdInfoDisp<HLXMapProf>(this, 
				    "CreateElements",
				    &HLXMapProf::HandleCreateElementsCmd,
				    3);
    cmds[i++] = new HLXUnitTestCmdInfoDisp<HLXMapProf>(this, 
				    "ClearElements",
				    &HLXMapProf::HandleClearElCmd,
				    1);
    cmds[i++] = new HLXUnitTestCmdInfoDisp<HLXMapProf>(this, 
				    "GetCount",
				    &HLXMapProf::HandleGetCountCmd,
				    2);
    cmds[i++] = new HLXUnitTestCmdInfoDisp<HLXMapProf>(this, 
				    "IsEmpty",
				    &HLXMapProf::HandleIsEmptyCmd,
				    2);
    cmds[i++] = new HLXUnitTestCmdInfoDisp<HLXMapProf>(this, 
				    "Lookup",
				    &HLXMapProf::HandleLookupCmd,
				    3);
    cmds[i++] = new HLXUnitTestCmdInfoDisp<HLXMapProf>(this, 
				    "SetAt",
				    &HLXMapProf::HandleSetAtCmd,
				    2);
    cmds[i++] = new HLXUnitTestCmdInfoDisp<HLXMapProf>(this, 
				     "RemoveKey",
				     &HLXMapProf::HandleRemoveKeyCmd,
				     3);
    cmds[i++] = new HLXUnitTestCmdInfoDisp<HLXMapProf>(this, 
				     "RemoveAll",
				     &HLXMapProf::HandleRemoveAllCmd,
				     1);
    cmds[i++] = new HLXUnitTestCmdInfoDisp<HLXMapProf>(this, 
				     "Rhs[]",
				     &HLXMapProf::HandleRhsArrayOpCmd,
				     3);
    cmds[i++] = new HLXUnitTestCmdInfoDisp<HLXMapProf>(this, 
				     "Lhs[]",
				     &HLXMapProf::HandleLhsArrayOpCmd,
				     2);
    cmds[i++] = new HLXUnitTestCmdInfoDisp<HLXMapProf>(this, 
				     "IsNull",
				     &HLXMapProf::HandleIsNullCmd,
				     3);
    cmds[i++] = new HLXUnitTestCmdInfoDisp<HLXMapProf>(this, 
				     "RunMapSpecificProfs",
				     &HLXMapProf::HandleRunMapSpecificProfsCmd,
				     1);

    cmds[i++] = new HLXUnitTestCmdInfoDisp<HLXMapProf>(this, 
				     "ProfileInsertions",
				     &HLXMapProf::HandleProfileInsertionsCmd,
				     2);

    cmds[i++] = new HLXUnitTestCmdInfoDisp<HLXMapProf>(this, 
				     "InitHashTable",
				     &HLXMapProf::HandleInitHashTableCmd,
				     3);

}

HLXCmdBasedTest* HLXMapProf::Clone() const
{
    return new HLXMapProf();
}

namespace
{
    bool PropagateCall(CHXPtrArray& arr,
                       bool (GenMapProfBase::* FN)())
    {
        bool ret = true;

        for (int i = 0; ret && (i < arr.GetSize()); i++)
        {
            GenMapProfBase* pObj = (GenMapProfBase*)arr[i];

            if (pObj)
                ret = (pObj->*FN)();
        }

        return ret;
    }

    template <typename ARG1TYPE>
    bool PropagateCall(CHXPtrArray& arr,
                       bool (GenMapProfBase::* FN)(ARG1TYPE),
                       ARG1TYPE arg1)
    {
        bool ret = true;

        for (int i = 0; ret && (i < arr.GetSize()); i++)
        {
            GenMapProfBase* pObj = (GenMapProfBase*)arr[i];

            if (pObj)
                ret = (pObj->*FN)(arg1);
        }

        return ret;
    }

    template <typename ARG1TYPE, typename ARG2TYPE>
    bool PropagateCall(CHXPtrArray& arr,
                       bool (GenMapProfBase::* FN)(ARG1TYPE, ARG2TYPE),
                       ARG1TYPE arg1, ARG2TYPE arg2)
    {
        bool ret = true;

        for (int i = 0; ret && (i < arr.GetSize()); i++)
        {
            GenMapProfBase* pObj = (GenMapProfBase*)arr[i];

            if (pObj)
                ret = (pObj->*FN)(arg1, arg2);
        }

        return ret;
    }
};

bool HLXMapProf::HandleMemCheckpointCmd(const UTVector<UTString>& /*info*/)
{
    bool ret = true;

    void* prev = m_memCheckpoint;

#if !defined(_SYMBIAN) && !defined(_WINDOWS)
    m_memCheckpoint = sbrk(0);
#else
    m_memCheckpoint = 0;
#endif    

    if (prev)
    {
        DPRINTF(D_INFO, ("HlxMapProf::MemCheckpoint: %p -> %p (%d bytes)\n",
                         prev, m_memCheckpoint,
                         (char*)m_memCheckpoint - (char*)prev));
    }
    else
    {
        DPRINTF(D_INFO, ("HlxMapProf::MemCheckpoint: %p\n", m_memCheckpoint));
    }

    return ret;
}

bool HLXMapProf::HandleTimeCheckpointCmd(const UTVector<UTString>& /*info*/)
{
#ifndef _WINCE
    struct timeval prev = {m_timeCheckpointSec, m_timeCheckpointMicro};
    struct timeval next = {0, 0};

//XXXgfw look like this test has only worked on linux.    
#ifndef _WINDOWS    
    if (gettimeofday(&next, NULL) != 0)
    {
        DPRINTF(D_INFO, ("HlxMapProf::TimeCheckpoint: gettimeofday() error number %d\n",
                         errno));
        return false;
    }

    if (prev.tv_sec || prev.tv_usec)
    {
        long diffSec = next.tv_sec - prev.tv_sec;
        long diffUsec = next.tv_usec - prev.tv_usec;
        if (diffUsec < 0)
        {
            --diffSec;
            diffUsec += 1000000;
        }

        DPRINTF(D_INFO, ("HlxMapProf::TimeCheckpoint: %ld.%06ld -> %ld.%06ld (%ld.%06ld sec)\n",
                         prev.tv_sec, prev.tv_usec,
                         next.tv_sec, next.tv_usec,
                         diffSec, diffUsec));
    }
    else
    {
        DPRINTF(D_INFO, ("HlxMapProf::TimeCheckpoint: %ld.%06ld\n",
                         next.tv_sec, next.tv_usec));
    }
    m_timeCheckpointSec = next.tv_sec;
    m_timeCheckpointMicro = next.tv_usec;
#else
        DPRINTF(D_INFO, ("This test has not been ported to windows yet. Maybe others.\n"));
#endif

#endif //_WINCE
    return true;
}

bool HLXMapProf::HandleSetSeedCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    unsigned int seed = 0;
    if (!UTParamUtil::GetUInt(info[1], seed))
    {
	DPRINTF(D_ERROR, ("HLXMapProf::HandleSetSeedCmd : failed to convert parameter\n"));	
    }
    else
    {
        ret = true;
        srand(seed);
    }

    return ret;
}

bool HLXMapProf::HandleCreateElCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    int index = 0;
    if (!UTParamUtil::GetInt(info[1], index))
    {
	DPRINTF(D_ERROR, ("HLXMapProf::HandleCreateElCmd : failed to convert parameter\n"));	
    }
    else
        ret = PropagateCall(m_array, &GenMapProfBase::CreateElement, index);

    return ret;
}

#ifdef XXXSAB
bool HLXMapProf::CreateElement(int index)
{
    bool ret = true;

    for (int i = 0; ret && (i < m_array.GetSize()); i++)
    {
	GenMapProfBase* pObj = (GenMapProfBase*)m_array[i];

	if (pObj)
	    ret = pObj->CreateElement(index);
    }

    return ret;
}
#endif /* XXXSAB */

bool HLXMapProf::HandleCreateElementsCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    int index = 0;
    int num = 0;
    if (!UTParamUtil::GetInt(info[1], index) ||
        !UTParamUtil::GetInt(info[2], num))
    {
	DPRINTF(D_ERROR, ("HLXMapProf::HandleCreateElementsCmd : failed to convert parameter\n"));	
    }
    else
	ret = PropagateCall(m_array, &GenMapProfBase::CreateElements,
                            index, num);

    return ret;
}

bool HLXMapProf::HandleClearElCmd(const UTVector<UTString>& /*info*/)
{
    return PropagateCall(m_array, &GenMapProfBase::ClearElements);
    // return ClearElements();
}

#ifdef XXXSAB
bool HLXMapProf::ClearElements()
{
    bool ret = true;

    for (int i = 0; ret && (i < m_array.GetSize()); i++)
    {
	GenMapProfBase* pObj = (GenMapProfBase*)m_array[i];

	if (pObj)
	    ret = pObj->ClearElements();
    }

    return ret;
}
#endif /* XXXSAB */

bool HLXMapProf::HandleGetCountCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    int expected = 0;
    if (!UTParamUtil::GetInt(info[1], expected))
    {
	DPRINTF(D_ERROR, ("HLXMapProf::HandleGetCountCmd : failed to convert parameter\n"));	
    }
    else
	ret = GetCount(expected);

    return ret;
}

bool HLXMapProf::GetCount(int expected)
{
    bool ret = true;

    for (int i = 0; ret && (i < m_array.GetSize()); i++)
    {
	GenMapProfBase* pObj = (GenMapProfBase*)m_array[i];

	if (pObj)
	    ret = pObj->GetCount(expected);
    }

    return ret;
}

bool HLXMapProf::HandleIsEmptyCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    bool expected = false;
    if (!UTParamUtil::GetBool(info[1],expected))
    {
	DPRINTF(D_ERROR, ("HLXMapProf::HandleIsEmptyCmd : failed to convert parameter\n"));	
    }
    else
	ret = IsEmpty(expected);

    return ret;
}

bool HLXMapProf::IsEmpty(bool expected)
{
    bool ret = true;

    for (int i = 0; ret && (i < m_array.GetSize()); i++)
    {
	GenMapProfBase* pObj = (GenMapProfBase*)m_array[i];

	if (pObj)
	    ret = pObj->IsEmpty(expected);
    }

    return ret;
}

bool HLXMapProf::HandleLookupCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    int index = 0;
    bool expected = false;
    if (!UTParamUtil::GetInt(info[1], index) ||
	!UTParamUtil::GetBool(info[2],expected))
    {
	DPRINTF(D_ERROR, ("HLXMapProf::HandleLookupCmd : failed to convert parameter\n"));	
    }
    else
	ret = Lookup(index, expected);

    return ret;
}

bool HLXMapProf::Lookup(int index, bool expected)
{
    bool ret = true;

    for (int i = 0; ret && (i < m_array.GetSize()); i++)
    {
	GenMapProfBase* pObj = (GenMapProfBase*)m_array[i];

	if (pObj)
	    ret = pObj->Lookup(index, expected);
    }

    return ret;
}

bool HLXMapProf::HandleSetAtCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    int index = 0;
    if (!UTParamUtil::GetInt(info[1], index))
    {
	DPRINTF(D_ERROR, ("HLXMapProf::HandleSetCmd : failed to convert parameter\n"));	
    }
    else
	ret = SetAt(index);

    return ret;
}

bool HLXMapProf::SetAt(int index)
{
    bool ret = true;

    for (int i = 0; ret && (i < m_array.GetSize()); i++)
    {
	GenMapProfBase* pObj = (GenMapProfBase*)m_array[i];

	if (pObj)
	    ret = pObj->SetAt(index);
    }
    
    return ret;
}

bool HLXMapProf::HandleRemoveKeyCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    int index = 0;
    bool expected = false;
    if (!UTParamUtil::GetInt(info[1], index) ||
	!UTParamUtil::GetBool(info[2],expected))
    {
	DPRINTF(D_ERROR, ("HLXMapProf::HandleRemoveKeyCmd : failed to convert parameter\n"));	
    }
    else
	ret = RemoveKey(index, expected);

    return ret;
}

bool HLXMapProf::RemoveKey(int index, bool expected)
{
    bool ret = true;

    for (int i = 0; ret && (i < m_array.GetSize()); i++)
    {
	GenMapProfBase* pObj = (GenMapProfBase*)m_array[i];

	if (pObj)
	    ret = pObj->RemoveKey(index, expected);
    }

    return ret;
}

bool HLXMapProf::HandleRemoveAllCmd(const UTVector<UTString>& /*info*/)
{
    bool ret = true;

    for (int i = 0; ret && (i < m_array.GetSize()); i++)
    {
	GenMapProfBase* pObj = (GenMapProfBase*)m_array[i];

	if (pObj)
	    ret = pObj->RemoveAll();
    }

    return ret;
}

bool HLXMapProf::HandleRhsArrayOpCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    int index = 0;
    bool expected = false;
    if (!UTParamUtil::GetInt(info[1], index) ||
	!UTParamUtil::GetBool(info[2], expected))
    {
	DPRINTF(D_ERROR, ("HLXMapProf::HandleRhsArrayOpCmd : failed to convert parameter\n"));	
    }
    else
    {
	ret = RhsArrayOp(index, expected);
    }

    return ret;
}

bool HLXMapProf::RhsArrayOp(int index, bool expected)
{
    bool ret = true;

    for (int i = 0; ret && (i < m_array.GetSize()); i++)
    {
	GenMapProfBase* pObj = (GenMapProfBase*)m_array[i];

	if (pObj)
	{
	    ret = pObj->RhsArrayOp(index, expected);
	}
    }

    return ret;
}

bool HLXMapProf::HandleLhsArrayOpCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    int index = 0;
    if (!UTParamUtil::GetInt(info[1], index))
    {
	DPRINTF(D_ERROR, ("HLXMapProf::HandleLhsArrayOpCmd : failed to convert parameter\n"));	
    }
    else
	ret = LhsArrayOp(index);


    return ret;
}

bool HLXMapProf::LhsArrayOp(int index)
{
    bool ret = true;

    for (int i = 0; ret && (i < m_array.GetSize()); i++)
    {
	GenMapProfBase* pObj = (GenMapProfBase*)m_array[i];

	if (pObj)
	    ret = pObj->LhsArrayOp(index);
    }

    return ret;
}

bool HLXMapProf::HandleIsNullCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    int index = 0;
    bool expected = false;
    if (!UTParamUtil::GetInt(info[1], index) ||
	!UTParamUtil::GetBool(info[2], expected))
    {
	DPRINTF(D_ERROR, ("HLXMapProf::HandleIsNullCmd : failed to convert parameter\n"));	
    }
    else
    {
	ret = IsNull(index, expected);
    }

    return ret;
}

bool HLXMapProf::IsNull(int index, bool expected)
{
    bool ret = true;

    for (int i = 0; ret && (i < m_array.GetSize()); i++)
    {
	GenMapProfBase* pObj = (GenMapProfBase*)m_array[i];

	if (pObj)
	{
	    ret = pObj->IsNull(index, expected);
	}
    }

    return ret;
}

bool HLXMapProf::HandleRunMapSpecificProfsCmd(const UTVector<UTString>& /*info*/)
{
    bool ret = true;

    for (int i = 0; ret && (i < m_array.GetSize()); i++)
    {
	GenMapProfBase* pObj = (GenMapProfBase*)m_array[i];

	if (pObj)
	    ret = pObj->RunMapSpecificProfs();
    }

    return ret;
}

bool HLXMapProf::HandleProfileInsertionsCmd(const UTVector<UTString>& info)
{
    bool ret = false;
    int typeIdx = -1;

    if (!UTParamUtil::GetInt(info[1], typeIdx) ||
        (typeIdx < 0 || typeIdx >= m_array.GetSize()))
    {
	DPRINTF(D_ERROR, ("HLXMapProf::HandleProfileInsertionsCmd : failed to convert parameter\n"));	
    }
    else
    {
	GenMapProfBase* pObj = (GenMapProfBase*)m_array[typeIdx];

	if (pObj)
	    ret = pObj->ProfileInsertions(m_typeArray[typeIdx]);
    }

    return ret;
}

bool HLXMapProf::HandleInitHashTableCmd(const UTVector<UTString>& info)
{
    bool ret = false;
    int typeIdx = -1;
    int num = 0;

    if (!UTParamUtil::GetInt(info[1], typeIdx) ||
        (typeIdx < 0 || typeIdx >= m_array.GetSize()) ||
	!UTParamUtil::GetInt(info[2], num))
    {
	DPRINTF(D_ERROR, ("HLXMapProf::HandleInitHashTableCmd : failed to convert parameter\n"));	
    }
    else
    {
	GenMapProfBase* pObj = (GenMapProfBase*)m_array[typeIdx];

	if (pObj)
	    ret = pObj->InitHashTable(num);
    }

    return ret;
}
