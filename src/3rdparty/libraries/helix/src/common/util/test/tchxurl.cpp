/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: tchxurl.cpp,v 1.2 2007/07/06 20:39:29 jfinnecy Exp $
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


#include "tchxurl.h"
#include "ihxpckts.h"
#include "hx_ut_debug.h"
#include "ut_param_util.h"


CHXURLTest::CHXURLTest()
: m_pURL(0)
, m_pProps(0)
{}

CHXURLTest::~CHXURLTest()
{
HX_DELETE(m_pURL);
HX_RELEASE(m_pProps);
}

#define ADD_CMD(idx, name, className, classMethod, argCount) \
cmds[idx] = new HLXUnitTestCmdInfoDisp<className> (this, name, &className::classMethod, argCount + 1)

void CHXURLTest::GetCommandInfo(UTVector<HLXUnitTestCmdInfo*>& cmds)
{
    cmds.Resize(10);

    UINT32 idx = 0;

    // args: escaped string
    ADD_CMD(idx++, "Construct", CHXURLTest, HandleConstruct,    1);

    // args: expected result
    ADD_CMD(idx++, "URL", CHXURLTest, HandleURL,          1);

    // args: expected bool result
    ADD_CMD(idx++, "ParseError", CHXURLTest, HandleParseError,          1);

    // args: expected result
    ADD_CMD(idx++, "FixedURL", CHXURLTest, HandleFixedURL,          1);

    // args: expected url
    ADD_CMD(idx++, "PropURL", CHXURLTest, HandlePropURL,   1);

    // args: expected scheme
    ADD_CMD(idx++, "PropScheme", CHXURLTest, HandlePropScheme,   1);

    // args: expected host
    ADD_CMD(idx++, "PropHost", CHXURLTest, HandlePropHost,   1);

    // args: expected path
    ADD_CMD(idx++, "PropPath", CHXURLTest, HandlePropPath,   1);

    // args: expected fullpath
    ADD_CMD(idx++, "PropFullPath", CHXURLTest, HandlePropFullPath,   1);

    // args: expected resource
    ADD_CMD(idx++, "PropResource", CHXURLTest, HandlePropResource,   1);


    HX_ASSERT(cmds.Nelements() == idx);

}

const char* CHXURLTest::DefaultCommandLine() const
{
    return "tchxurl tchxurl.in -a";
}

HLXCmdBasedTest* CHXURLTest::Clone() const
{
    return new CHXURLTest();
}

// "Construct" <string>
bool CHXURLTest::HandleConstruct(const UTVector<UTString>& params)
{
    HX_DELETE(m_pURL);
    HX_RELEASE(m_pProps);
    m_pURL = new CHXURL((const char*)params[1]);
    if (m_pURL)
    {
        m_pProps = m_pURL->GetProperties();
        if (m_pProps)
        {
            return true;
        }
    }

    DPRINTF (D_ERROR, ("CHXURLTest::HandleConstruct() : contruct failed\n"));

    return false;
}

bool ConvertToBool(const char* pStr)
{
    int intVal = strtol(pStr, 0, 10);
    return (intVal == 1);
}

// "ParseError" <expected bool>
bool CHXURLTest::HandleParseError(const UTVector<UTString>& params)
{
    bool ret = false;
    if (m_pURL)
    {
        bool expected = ConvertToBool(params[1]);
        bool err = !SUCCEEDED(m_pURL->GetLastError());
        if (expected == err)
        {
            ret = true;
        }
        else
        {
	    DPRINTF (D_ERROR, ("CHXURLTest::HandleURL() : expected '%s' got '%d'\n",
			   (const char*)params[1],
                           (err ? 1 : 0)));
        }
    }

    return ret;
}

// "URL" <expected>
bool CHXURLTest::HandleURL(const UTVector<UTString>& params)
{
    bool ret = false;

    if (m_pURL)
    {
        if (0 == stricmp(m_pURL->GetURL(), params[1]))
        {
            ret = true;
        }
        else
        {
	    DPRINTF (D_ERROR, ("CHXURLTest::HandleURL() : expected '%s' got '%s'\n",
			   (const char*)params[1],
			   (const char*)m_pURL->GetURL()));
        }
    }

    return ret;
}

// "FixedURL" <expected>
bool CHXURLTest::HandleFixedURL(const UTVector<UTString>& params)
{
    bool ret = false;

    if (m_pURL)
    {
        if (0 == stricmp(m_pURL->GetFixedURL(),params[1]))
        {
            ret = true;
        }
        else
        {
	    DPRINTF (D_ERROR, ("CHXURLTest::HandleFixedURL() : expected '%s' got '%s'\n",
			   (const char*)params[1],
			   (const char*)m_pURL->GetFixedURL()));
        }
    }

    return ret;
}

// "Prop" helper for buffer (string) props
bool CHXURLTest::HandleStringPropHelper(const UTVector<UTString>& params, 
                                  const char* pKey)
{
    bool ret = false;

    if (m_pProps)
    {
        IHXBuffer* pBuf = 0;
        HX_RESULT hr = m_pProps->GetPropertyBuffer(pKey, pBuf);
        if (SUCCEEDED(hr))
        {
            const char* pStr = (const char*)pBuf->GetBuffer();
            if (0 == stricmp(params[1], pStr))
            {
                ret = true;
            }
            else
            {
                DPRINTF (D_ERROR, ("CHXURLTest::HandlePropHelper() : expected '%s' got '%s'\n",
			   (const char*)params[1],
			   pStr));
            }
            HX_RELEASE(pBuf);
        }
        else
        {
            ret = (params[1].length() == 0);
            if (!ret)
            {
                DPRINTF (D_ERROR, ("CHXURLTest::HandlePropHelper() : expected '%s' got ''\n",
			   (const char*)params[1]));
            }
        }

    }

    return ret;
}

bool CHXURLTest::HandlePropURL(const UTVector<UTString>& params)
{
    return HandleStringPropHelper(params, PROPERTY_URL);
}

bool CHXURLTest::HandlePropScheme(const UTVector<UTString>& params)
{
    return HandleStringPropHelper(params, PROPERTY_SCHEME);
}

bool CHXURLTest::HandlePropHost(const UTVector<UTString>& params)
{
    return HandleStringPropHelper(params, PROPERTY_HOST);
}

bool CHXURLTest::HandlePropPath(const UTVector<UTString>& params)
{
    return HandleStringPropHelper(params, PROPERTY_PATH);
}

bool CHXURLTest::HandlePropFullPath(const UTVector<UTString>& params)
{
    return HandleStringPropHelper(params, PROPERTY_FULLPATH);
}

bool CHXURLTest::HandlePropResource(const UTVector<UTString>& params)
{
    return HandleStringPropHelper(params, PROPERTY_RESOURCE);
}



    

