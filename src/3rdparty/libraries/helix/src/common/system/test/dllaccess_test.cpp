/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: dllaccess_test.cpp,v 1.6 2004/07/09 18:19:41 hubbe Exp $
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

#include "./dllaccess_test.h"

#include "hx_ut_debug.h"
#include "ut_param_util.h"
#include "hxresult.h"

typedef HX_RESULT (*IntFunc)(int& intVal);

DLLAccessTest::DLLAccessTest() :
    m_pDllAccess(0)
{}

const char* DLLAccessTest::DefaultCommandLine() const
{
    return "tdllaccess tdllaccess.in";
}
    
void DLLAccessTest::GetCommandInfo(UTVector<HLXUnitTestCmdInfo*>& cmds)
{
    cmds.Resize(13);

    cmds[0] = new HLXUnitTestCmdInfoDisp<DLLAccessTest>(this, 
				    "DLLAccess()",
				    &DLLAccessTest::HandleConstructor1Cmd,
				    1);
    cmds[1] = new HLXUnitTestCmdInfoDisp<DLLAccessTest>(this, 
				    "DLLAccess(dllName,nLibType)",
				    &DLLAccessTest::HandleConstructor2Cmd,
				    3);
    cmds[2] = new HLXUnitTestCmdInfoDisp<DLLAccessTest>(this, 
				    "Open",
				    &DLLAccessTest::HandleOpenCmd,
				    4);
    cmds[3] = new HLXUnitTestCmdInfoDisp<DLLAccessTest>(this, 
				    "Close",
				    &DLLAccessTest::HandleCloseCmd,
				    2);
    cmds[4] = new HLXUnitTestCmdInfoDisp<DLLAccessTest>(this, 
				    "GetSymbol",
				    &DLLAccessTest::HandleGetSymbolCmd,
				    3);
    cmds[5] = new HLXUnitTestCmdInfoDisp<DLLAccessTest>(this, 
				    "GetError",
				    &DLLAccessTest::HandleGetErrorCmd,
				    2);
    cmds[6] = new HLXUnitTestCmdInfoDisp<DLLAccessTest>(this, 
				    "GetErrorString",
				    &DLLAccessTest::HandleGetErrorStringCmd,
				    2);
    cmds[7] = new HLXUnitTestCmdInfoDisp<DLLAccessTest>(this, 
				    "GetDLLName",
				    &DLLAccessTest::HandleGetDLLNameCmd,
				    3);
    cmds[8] = new HLXUnitTestCmdInfoDisp<DLLAccessTest>(this, 
				    "GetVersion",
				    &DLLAccessTest::HandleGetVersionCmd,
				    3);
    cmds[9] = new HLXUnitTestCmdInfoDisp<DLLAccessTest>(this, 
				    "CreateName1",
				    &DLLAccessTest::HandleCreateName1Cmd,
				    4);
    cmds[10] = new HLXUnitTestCmdInfoDisp<DLLAccessTest>(this, 
				    "CreateName2",
				    &DLLAccessTest::HandleCreateName2Cmd,
				    6);
    cmds[11] = new HLXUnitTestCmdInfoDisp<DLLAccessTest>(this, 
				    "IsOpen",
				    &DLLAccessTest::HandleIsOpenCmd,
				    2);
    cmds[12] = new HLXUnitTestCmdInfoDisp<DLLAccessTest>(this, 
				    "CallIntFunc",
				    &DLLAccessTest::HandleCallIntFuncCmd,
				    3);
}

HLXCmdBasedTest* DLLAccessTest::Clone() const
{
    return new DLLAccessTest();
}

bool DLLAccessTest::HandleConstructor1Cmd(const UTVector<UTString>& info)
{
    delete m_pDllAccess;

    m_pDllAccess = new DLLAccess();

    return (m_pDllAccess != 0);
}

bool DLLAccessTest::HandleConstructor2Cmd(const UTVector<UTString>& info)
{
    delete m_pDllAccess;
    m_pDllAccess = 0;

    unsigned int libType = 0;
    if (!UTParamUtil::GetUInt(info[2], libType))
    {
	DPRINTF(D_ERROR, ("DLLAccessTest::HandleConstructor2Cmd() : Failed to convert parameters\n"));
    }
    else
    {
	m_pDllAccess = new DLLAccess((const char*)info[1], (UINT16)libType);
    }

    return (m_pDllAccess != 0);
}

bool DLLAccessTest::HandleOpenCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    unsigned int libType = 0;
    int expected = -1;

    if (!m_pDllAccess)
    {
	DPRINTF(D_ERROR, ("DLLAccessTest::HandleOpenCmd() : No DLLAccess object\n"));
    }
    else if (!UTParamUtil::GetUInt(info[2], libType) ||
	     !UTParamUtil::GetInt(info[3], expected))
    {
	DPRINTF(D_ERROR, ("DLLAccessTest::HandleOpenCmd() : Failed to convert parameters\n"));
    }
    else
    {
	int result = m_pDllAccess->open(info[1], (UINT16)libType);

	if (result != expected)
	{
	    DPRINTF(D_ERROR,("DLLAccessTest::HandleOpenCmd() : Got %d, expected %d\n",
			     result,
			     expected));
	}
	else
	    ret = true;
    }

    return ret;
}

bool DLLAccessTest::HandleCloseCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    int expected = -1;

    if (!m_pDllAccess)
    {
	DPRINTF(D_ERROR, ("DLLAccessTest::HandleCloseCmd() : No DLLAccess object\n"));
    }
    else if (!UTParamUtil::GetInt(info[1], expected))
    {
	DPRINTF(D_ERROR, ("DLLAccessTest::HandleCloseCmd() : Failed to convert parameters\n"));
    }
    else
    {
	int result = m_pDllAccess->close();

	if (result != expected)
	{
	    DPRINTF(D_ERROR, ("DLLAccessTest::HandleCloseCmd() : Got %d, expected %d\n",
			      result,
			      expected));
	}
	else
	    ret = true;
    }

    return ret;
}

bool DLLAccessTest::HandleGetSymbolCmd(const UTVector<UTString>& info)
{
    bool ret = false;
    
    bool expected = false;

    if (!m_pDllAccess)
    {
	DPRINTF(D_ERROR, ("DLLAccessTest::HandleGetSymbolCmd() : No DLLAccess object\n"));
    }
    else if (!UTParamUtil::GetBool(info[2], expected))
    {
	DPRINTF(D_ERROR, ("DLLAccessTest::HandleGetSymbolCmd() : Failed to convert parameters\n"));
    }
    else
    {
	bool result = (m_pDllAccess->getSymbol(info[1]) != 0);
	
	if (result != expected)
	{
	    DPRINTF(D_ERROR, ("DLLAccessTest::HandleGetSymbolCmd() : Got %d, expected %d\n",
			      result,
			      expected));
	}
	else
	    ret = true;
    }

    return ret;
}

bool DLLAccessTest::HandleGetErrorCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    int expected = -1;

    if (!m_pDllAccess)
    {
	DPRINTF(D_ERROR, ("DLLAccessTest::HandleGetErrorCmd() : No DLLAccess object\n"));
    }
    else if (!UTParamUtil::GetInt(info[1], expected))
    {
	DPRINTF(D_ERROR, ("DLLAccessTest::HandleGetErrorCmd() : Failed to convert parameters\n"));
    }
    else
    {
	int result = m_pDllAccess->getError();

	if (result != expected)
	{
	    DPRINTF(D_ERROR, ("DLLAccessTest::HandleGetErrorCmd() : Got %d, expected %d\n",
			      result,
			      expected));
	}
	else
	    ret = true;
    }

    return ret;
}

bool DLLAccessTest::HandleGetErrorStringCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    if (!m_pDllAccess)
    {
	DPRINTF(D_ERROR, ("DLLAccessTest::HandleGetErrorCmd() : No DLLAccess object\n"));
    }
    else if (info[1] != m_pDllAccess->getErrorString())
    {
	DPRINTF(D_ERROR, ("DLLAccessTest::HandleGetErrorCmd() : Got '%s', expected '%s'\n",
			  m_pDllAccess->getErrorString(),
			  (const char*)info[1]));
    }
    else
	ret = true;

    return ret;
}

bool DLLAccessTest::HandleGetDLLNameCmd(const UTVector<UTString>& info)
{
    bool ret = false;
    bool expected = false;

    if (!m_pDllAccess)
    {
	DPRINTF(D_ERROR, ("DLLAccessTest::HandleGetDLLNameCmd() : No DLLAccess object\n"));
    }
    else if (!UTParamUtil::GetBool(info[1], expected))
    {
	DPRINTF(D_ERROR, ("DLLAccessTest::HandleGetDLLNameCmd() : Failed to convert parameters\n"));
    }
    else 
    {
	const char* pDllName = m_pDllAccess->getDLLName();
	bool result = (pDllName != 0);
	
	if (result != expected)
	{
	    DPRINTF(D_ERROR, ("DLLAccessTest::HandleGetDLLNameCmd() : Got %d, expected %d\n",
			      result,
			      expected));
	}
	else if (pDllName && (info[2] != pDllName))
	{
	    DPRINTF(D_ERROR, ("DLLAccessTest::HandleGetDLLNameCmd() : Got '%s', expected '%s'\n",
			      pDllName,
			      (const char*)info[2]));
	}
	else
	    ret = true;
    }

    return ret;
}

bool DLLAccessTest::HandleGetVersionCmd(const UTVector<UTString>& info)
{
    bool ret = false;
    bool expected = false;

    if (!m_pDllAccess)
    {
	DPRINTF(D_ERROR, ("DLLAccessTest::HandleGetDLLNameCmd() : No DLLAccess object\n"));
    }
    else if (!UTParamUtil::GetBool(info[1], expected))
    {
	DPRINTF(D_ERROR, ("DLLAccessTest::HandleGetVersionCmd() : Failed to convert parameters\n"));
    }
    else 
    {
	const char* pVersion = m_pDllAccess->getVersion();
	bool result = (pVersion != 0);
	
	if (result != expected)
	{
	    DPRINTF(D_ERROR, ("DLLAccessTest::HandleGetVersionCmd() : Got %d, expected %d\n",
			      result,
			      expected));
	}
	else if (pVersion && (info[2] != pVersion))
	{
	    DPRINTF(D_ERROR, ("DLLAccessTest::HandleGetVersionCmd() : Got '%s', expected '%s'\n",
			      pVersion,
			      (const char*)info[1]));
	}
	else
	    ret = true;
    }

    return ret;
}

bool DLLAccessTest::HandleCreateName1Cmd(const UTVector<UTString>& info)
{
    bool ret = false;

    if (!m_pDllAccess)
    {
	DPRINTF(D_ERROR, ("DLLAccessTest::HandleCreateName1Cmd() : No DLLAccess object\n"));
    }
    else
    {
	char buf[100]; /* Flawfinder: ignore */
	UINT32 bufLen = sizeof(buf);

	m_pDllAccess->CreateName((const char*)info[1], (const char*)info[2],
				 buf, bufLen);

	if (info[3] != buf)
	{
	    DPRINTF(D_ERROR, ("DLLAccessTest::HandleCreateName1Cmd() : Got '%s', expected '%s'\n",
			      buf,
			      (const char*)info[3]));
	}
	else
	    ret = true;
    }

    return ret;
}

bool DLLAccessTest::HandleCreateName2Cmd(const UTVector<UTString>& info)
{
    bool ret = false;

    unsigned int major = 0;
    unsigned int minor = 0;

    if (!m_pDllAccess)
    {
	DPRINTF(D_ERROR, ("DLLAccessTest::HandleCreateName2Cmd() : No DLLAccess object\n"));
    }
    else if (!UTParamUtil::GetUInt(info[3], major) ||
	     !UTParamUtil::GetUInt(info[4], minor))
    {
	DPRINTF(D_ERROR, ("DLLAccessTest::HandleCreateName2Cmd() : Failed to convert parameters\n"));
    }
    else
    {
	char buf[100]; /* Flawfinder: ignore */
	UINT32 bufLen = sizeof(buf);

	m_pDllAccess->CreateName((const char*)info[1], (const char*)info[2],
				 buf, bufLen,
				 major, minor);

	if (info[5] != buf)
	{
	    DPRINTF(D_ERROR, ("DLLAccessTest::HandleCreateName2Cmd() : Got '%s', expected '%s'\n",
			      buf,
			      (const char*)info[5]));
	}
	else
	    ret = true;
    }

    return ret;
}

bool DLLAccessTest::HandleIsOpenCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    bool expected = false;

    if (!m_pDllAccess)
    {
	DPRINTF(D_ERROR, ("DLLAccessTest::HandleIsOpenCmd() : No DLLAccess object\n"));
    }
    else if (!UTParamUtil::GetBool(info[1], expected))
    {
	DPRINTF(D_ERROR, ("DLLAccessTest::HandleIsOpenCmd() : Failed to convert parameters\n"));
    }
    else if (expected != (m_pDllAccess->isOpen() == TRUE))
    {
	DPRINTF(D_ERROR, ("DLLAccessTest::HandleIsOpenCmd() : Got %d expected %d\n",
			  m_pDllAccess->isOpen(),
			  expected));
    }
    else
	ret = true;

    return ret;
}

bool DLLAccessTest::HandleCallIntFuncCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    int expected = -1;

    if (!m_pDllAccess)
    {
	DPRINTF(D_ERROR, ("DLLAccessTest::HandleIsOpenCmd() : No DLLAccess object\n"));
    }
    else if (!UTParamUtil::GetInt(info[2], expected))
    {
	DPRINTF(D_ERROR, ("DLLAccessTest::HandleIsOpenCmd() : Failed to convert parameters\n"));
    }
    else
    {
	IntFunc pIntFunc = (IntFunc)m_pDllAccess->getSymbol(info[1]);

	if (!pIntFunc)
	{
	    DPRINTF(D_ERROR, ("DLLAccessTest::HandleIsOpenCmd() : Failed to get symbol '%s'\n",
			      (const char*)info[1]));
	}
	else
	{
	    int result = -1;

	    pIntFunc(result);
	    
	    if (result != expected)
	    {
		DPRINTF(D_ERROR, ("DLLAccessTest::HandleIsOpenCmd() : Got %d, expected %d\n",
				  result,
				  expected));
	    }
	    else
		ret = true;
	}
    }

    return ret;
}
