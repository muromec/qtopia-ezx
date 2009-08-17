/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: thxurlrep.cpp,v 1.3 2004/12/15 21:24:30 liam_murray Exp $
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
#include "thxurlrep.h"
#include "hxescapeutil.h"


#include "hx_ut_debug.h"
#include "ut_param_util.h"

bool ConvertToBool(const char* pStr, bool& value)
{
    int intVal = strtol(pStr, 0, 10);
    value = (intVal == 1);
    return true;
}

HXURLRepTest::HXURLRepTest()
{}

HXURLRepTest::~HXURLRepTest()
{}

void HXURLRepTest::GetCommandInfo(UTVector<HLXUnitTestCmdInfo*>& cmds)
{
    cmds.Resize(16);

    int idx = 0;
    cmds[idx++] = new HLXUnitTestCmdInfoDisp<HXURLRepTest>(this, 
				    "HXURLRep(url)",
				    &HXURLRepTest::HandleConstructor1Cmd,
				    2);
    cmds[idx++] = new HLXUnitTestCmdInfoDisp<HXURLRepTest>(this, 
				    "HXURLRep(common)",
				    &HXURLRepTest::HandleConstructor2Cmd,
				    6);
    cmds[idx++] = new HLXUnitTestCmdInfoDisp<HXURLRepTest>(this, 
				    "HXURLRep(all)",
				    &HXURLRepTest::HandleConstructor3Cmd,
				    9);
    cmds[idx++] = new HLXUnitTestCmdInfoDisp<HXURLRepTest>(this, 
				    "==",
				    &HXURLRepTest::HandleEqualCmd,
				    3);
    cmds[idx++] = new HLXUnitTestCmdInfoDisp<HXURLRepTest>(this, 
				    "!=",
				    &HXURLRepTest::HandleNotEqualCmd,
				    3);
    cmds[idx++] = new HLXUnitTestCmdInfoDisp<HXURLRepTest>(this, 
				    "String",
				    &HXURLRepTest::HandleStringCmd,
				    2);
    cmds[idx++] = new HLXUnitTestCmdInfoDisp<HXURLRepTest>(this, 
				    "CheckState",
				    &HXURLRepTest::HandleCheckStateCmd,
				    6);
    cmds[idx++] = new HLXUnitTestCmdInfoDisp<HXURLRepTest>(this, 
				    "Path",
				    &HXURLRepTest::HandlePathCmd,
				    2);
    cmds[idx++] = new HLXUnitTestCmdInfoDisp<HXURLRepTest>(this, 
				    "Query",
				    &HXURLRepTest::HandleQueryCmd,
				    2);
    cmds[idx++] = new HLXUnitTestCmdInfoDisp<HXURLRepTest>(this, 
				    "ApplyRelative",
				    &HXURLRepTest::HandleApplyRelativeCmd,
				    3);
    cmds[idx++] = new HLXUnitTestCmdInfoDisp<HXURLRepTest>(this, 
				    "IsValid",
				    &HXURLRepTest::HandleIsValidCmd,
				    2);
    cmds[idx++] = new HLXUnitTestCmdInfoDisp<HXURLRepTest>(this, 
				    "IsFullyParsed",
				    &HXURLRepTest::HandleIsFullyParsedCmd,
				    2);
    cmds[idx++] = new HLXUnitTestCmdInfoDisp<HXURLRepTest>(this, 
				    "UserInfo",
				    &HXURLRepTest::HandleUserInfoCmd,
				    2);
    cmds[idx++] = new HLXUnitTestCmdInfoDisp<HXURLRepTest>(this, 
				    "Normalize",
				    &HXURLRepTest::HandleNormalizeCmd,
				    1);
    cmds[idx++] = new HLXUnitTestCmdInfoDisp<HXURLRepTest>(this, 
				    "Protocol",
				    &HXURLRepTest::HandleSchemeCmd,
				    2);
    cmds[idx++] = new HLXUnitTestCmdInfoDisp<HXURLRepTest>(this, 
				    "PathOffset",
				    &HXURLRepTest::HandlePathOffsetCmd,
				    2);

    HX_ASSERT(cmds.Nelements() == idx);
}

const char* HXURLRepTest::DefaultCommandLine() const
{
    return "thxurlrep thxurlrep.in -a";
}

HLXCmdBasedTest* HXURLRepTest::Clone() const
{
    return new HXURLRepTest();
}


bool HXURLRepTest::HandleNormalizeCmd(const UTVector<UTString>& params)
{
    // Handles commands of the following form
    // Normalize
    m_url.Normalize();

    return true;
}

bool HXURLRepTest::HandleConstructor1Cmd(const UTVector<UTString>& params)
{
    // Handles commands of the following form
    // HXURLRep(url) <url>
    m_url = HXURLRep((const char*)params[1]);

    return true;
}

bool HXURLRepTest::HandleConstructor2Cmd(const UTVector<UTString>& params)
{
    // Handles commands of the following form
    // HXURLRep(common) <0-3 for type> <scheme> <host> <port> <path>

    int port = strtol(params[4], 0, 10);

    int type = strtol(params[1], 0, 10);
    if (type < 0 || type > 3)
    {
        return false;
    }
    
    m_url = HXURLRep(HXURLRep::Type(type), 
                    (const char*)params[2], 
                    (const char*)params[3], 
                    port, 
                    (const char*)params[5]);
    return true;
}

bool HXURLRepTest::HandleConstructor3Cmd(const UTVector<UTString>& params)
{
    // Handles commands of the following form
    // HXURLRep(all) <0-3 for type> <scheme> <userInfo> <host> <port> <path> <query> <frag>

    int port = strtol(params[5], 0, 10);

    int type = strtol(params[1], 0, 10);
    if (type < 0 || type > 3)
    {
        return false;
    }
    

    m_url = HXURLRep(HXURLRep::Type(type), 
                    (const char*)params[2], 
                    (const char*)params[3],
                    (const char*)params[4],
                    port, 
                    (const char*)params[6], 
                    (const char*)params[7],
                    (const char*)params[8]);
    return true;
}



bool HXURLRepTest::HandleEqualCmd(const UTVector<UTString>& params)
{
    DPRINTF(D_ERROR, ("HXURLRepTest::HandleEqualCmd() : not implemented\n"));
    return false;
#if (0)
    // == <url> <expected bool result>
    bool ret = false;
    
    HXURLRep url((const char*)params[1]);
    bool expected = false;

    if (!ConvertToBool(params[2], expected))
    {
	DPRINTF(D_ERROR, ("HXURLRepTest::HandleEqualCmd() : failed to convert parameters\n"));
    }
    else
    {
	bool result = (m_url == url);

	if (result != expected)
	{
	    DPRINTF(D_ERROR, ("HXURLRepTest::HandleEqualCmd() : expected %d got %d\n",
			      expected,
			      result));
	}
	else
	    ret = true;
	    
    }

    return ret;
#endif
}

bool HXURLRepTest::HandleNotEqualCmd(const UTVector<UTString>& params)
{
    DPRINTF(D_ERROR, ("HXURLRepTest::HandleNotEqualCmd() : not implemented\n"));
    return false;
#if (0)
    // != <url> <expected bool result>
    bool ret = false;

    HXURLRep url((const char*)params[1]);
    bool expected = false;

    if (!ConvertToBool(params[2], expected))
    {
	DPRINTF(D_ERROR, ("HXURLRepTest::HandleNotEqualCmd() : failed to convert parameters\n"));
    }
    else
    {
	bool result = (m_url != url);

	if (result != expected)
	{
	    DPRINTF(D_ERROR, ("HXURLRepTest::HandleNotEqualCmd() : expected %d got %d\n",
			      expected,
			      result));
	}
	else
	    ret = true;
	    
    }

    return ret;
#endif
}

bool HXURLRepTest::HandlePathOffsetCmd(const UTVector<UTString>& params)
{
    // PrefixLength <expected prefix length>
    bool ret = true;

    UINT32 cch= m_url.GetPathOffset();
    UINT32 cchExpected = strtoul(params[1], 0, 10);
    if (cch != cchExpected)
    {
	DPRINTF (D_ERROR, ("HXURLRepTest::HandlePathOffsetCmd() : expected '%lu' got '%lu'\n",
			   (const char*)cchExpected,
			   (const char*)cch));

	ret = false;
    }

    return ret;
}

bool HXURLRepTest::HandleStringCmd(const UTVector<UTString>& params)
{
    // String <expected string rep>
    bool ret = true;

    if (m_url.String() != params[1])
    {
	DPRINTF (D_ERROR, ("HXURLRepTest::HandleStringCmd() : expected '%s' got '%s'\n",
			   (const char*)params[1],
			   (const char*)m_url.String()));

	ret = false;
    }

    return ret;
}

bool HXURLRepTest::HandleCheckStateCmd(const UTVector<UTString>& params)
{
    // CheckState <expected protocol> <expected host> <expected port> <expected escaped path> <expected escaped query>
    bool ret = false;

    int port = strtol(params[3], 0, 10); 
   
    if (m_url.Scheme() != params[1])
    {
	DPRINTF(D_ERROR, ("HXURLRepTest::HandleCheckStateCmd() : Scheme() expected '%s' got '%s'\n",
			  (const char*)params[1],
			  (const char*)m_url.Scheme()));
    }
    else if (m_url.Host() != params[2])
    {
	DPRINTF(D_ERROR, ("HXURLRepTest::HandleCheckStateCmd() : Host() expected '%s' got '%s'\n",
			  (const char*)params[2],
			  (const char*)m_url.Host()));
    }
    else if (m_url.Port() != port)
    {
	DPRINTF(D_ERROR, ("HXURLRepTest::HandleCheckStateCmd() : Scheme() expected '%ld' got '%ld'\n",
			  port,
			  m_url.Port()));
    }
    else if (m_url.Path() != params[4])
    {
	DPRINTF(D_ERROR, ("HXURLRepTest::HandleCheckStateCmd() : Path() expected '%s' got '%s'\n",
			  (const char*)params[4],
			  (const char*)m_url.Path()));
    }
    else if (m_url.Query() != params[5])
    {
	DPRINTF(D_ERROR, ("HXURLRepTest::HandleCheckStateCmd() : Query() expected '%s' got '%s'\n",
			  (const char*)params[5],
			  (const char*)m_url.Query()));
    }
    else
	ret = true;
    
    
    return ret;
}

bool HXURLRepTest::HandlePathCmd(const UTVector<UTString>& params)
{
    // Path <expected path>
    bool ret = false;

    if (m_url.Path() != params[1])
    {
	DPRINTF(D_ERROR,("HXURLRepTest::HandlePathCmd() : expected '%s' got '%s'\n",			 
			 (const char*)params[1],
			 (const char*)m_url.Path()));
    }
    else
	ret = true;

	
    return ret;
}

bool HXURLRepTest::HandleUserInfoCmd(const UTVector<UTString>& params)
{
    // UserInfo <expected user info string>
    bool ret = false;

    if (m_url.UserInfo() != params[1])
    {
	DPRINTF(D_ERROR,("HXURLRepTest::HandleUserInfoCmd() : expected '%s' got '%s'\n",
			 (const char*)params[1],
			 (const char*)m_url.UserInfo()));
    }
    else
	ret = true;
	

    return ret;
}

bool HXURLRepTest::HandleSchemeCmd(const UTVector<UTString>& params)
{
    // Scheme <expected scheme>
    bool ret = false;

    if (m_url.Scheme() != params[1])
    {
	DPRINTF(D_ERROR,("HXURLRepTest::HandleSchemeCmd() : expected '%s' got '%s'\n",
			 (const char*)params[1],
			 (const char*)m_url.Scheme()));
    }
    else
    {
	ret = true;
    }
	

    return ret;
}

bool HXURLRepTest::HandleQueryCmd(const UTVector<UTString>& params)
{
    // Query <expected query>
    bool ret = false;
    if (m_url.Query() != params[1])
    {
	DPRINTF(D_ERROR,("HXURLRepTest::HandleQueryCmd() : expected '%s' got '%s'\n",
			 (const char*)params[1],
			 (const char*)m_url.Query()));
    }
    else
	ret = true;
	

    return ret;
}

bool HXURLRepTest::HandleApplyRelativeCmd(const UTVector<UTString>& params)
{
    // ApplyRelative <url to apply> <0 = apply failed, 1 apply success>
    bool ret = false;

    bool expected = false;

    if (!ConvertToBool(params[2], expected))
    {
	DPRINTF(D_ERROR, ("HXURLRepTest::HandleApplyRelativeCmd() : failed to convert parameters\n"));
    }
    else
    {
	bool result = m_url.ApplyRelative(HXURLRep((const char*)params[1]));

	if (result != expected)
	{
	    DPRINTF(D_ERROR, ("HXURLRepTest::HandleApplyRelativeCmd() : expected %d got %d\n",
			      expected,
			      result));
	}
	else
	    ret = true;
    }

    return ret;
}

bool HXURLRepTest::HandleIsFullyParsedCmd(const UTVector<UTString>& params)
{
    // IsFullyParsed <expected true/false>
    bool ret = false;

    bool expected = false;

    if (!ConvertToBool(params[1], expected))
    {
	DPRINTF(D_ERROR, ("HXURLRepTest::HandleValidCmd() : failed to convert parameters\n"));
    }
    else if (m_url.IsFullyParsed() != expected)
    {
	DPRINTF(D_ERROR, ("HXURLRepTest::HandleValidCmd() : Valid() expected %d got %d\n",
			  expected,
			  m_url.IsValid()));
    }
    else
    {
	ret = true;
    }

    return ret;
}


bool HXURLRepTest::HandleIsValidCmd(const UTVector<UTString>& params)
{
    // IsValid <expected true/false>
    bool ret = false;

    bool expected = false;

    if (!ConvertToBool(params[1], expected))
    {
	DPRINTF(D_ERROR, ("HXURLRepTest::HandleValidCmd() : failed to convert parameters\n"));
    }
    else if (m_url.IsValid() != expected)
    {
	DPRINTF(D_ERROR, ("HXURLRepTest::HandleValidCmd() : Valid() expected %d got %d\n",
			  expected,
			  m_url.IsValid()));
    }
    else
    {
	ret = true;
    }

    return ret;
}

