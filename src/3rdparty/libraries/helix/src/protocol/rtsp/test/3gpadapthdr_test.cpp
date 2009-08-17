/* ***** BEGIN LICENSE BLOCK ***** 
 * Version: RCSL 1.0/RPSL 1.0 
 *  
 * Portions Copyright (c) 1995-2002 RealNetworks, Inc. All Rights Reserved. 
 *      
 * The contents of this file, and the files included with this file, are 
 * subject to the current version of the RealNetworks Public Source License 
 * Version 1.0 (the "RPSL") available at 
 * http://www.helixcommunity.org/content/rpsl unless you have licensed 
 * the file under the RealNetworks Community Source License Version 1.0 
 * (the "RCSL") available at http://www.helixcommunity.org/content/rcsl, 
 * in which case the RCSL will apply. You may also obtain the license terms 
 * directly from RealNetworks.  You may not use this file except in 
 * compliance with the RPSL or, if you have a valid RCSL with RealNetworks 
 * applicable to this file, the RCSL.  Please see the applicable RPSL or 
 * RCSL for the rights, obligations and limitations governing use of the 
 * contents of the file.  
 *  
 * This file is part of the Helix DNA Technology. RealNetworks is the 
 * developer of the Original Code and owns the copyrights in the portions 
 * it created. 
 *  
 * This file, and the files included with this file, is distributed and made 
 * available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER 
 * EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS ALL SUCH WARRANTIES, 
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS 
 * FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT. 
 * 
 * Technology Compatibility Kit Test Suite(s) Location: 
 *    http://www.helixcommunity.org/content/tck 
 * 
 * Contributor(s): 
 *  
 * ***** END LICENSE BLOCK ***** */ 
#define INITGUID

#include "./3gpadapthdr_test.h"

#include "hx_ut_debug.h"
#include "ut_param_util.h"
#include "hxassert.h"
#include "chxminiccf.h"
#include "pckunpck.h"
#include "ihxlist.h"
#include "hxtbuf.h"
#include "hxlistp.h"

C3gpAdaptationHeaderTest::C3gpAdaptationHeaderTest() :
    m_pA(NULL),
    m_pB(NULL),
    m_pValues(NULL),
    m_pContext(new CHXMiniCCF)
{
    if (m_pContext)
    {
        m_pContext->AddRef();
    }
}

C3gpAdaptationHeaderTest::~C3gpAdaptationHeaderTest()
{
    HX_DELETE(m_pA);
    HX_DELETE(m_pB);
    HX_RELEASE(m_pValues);
    HX_RELEASE(m_pContext);
}

const char* C3gpAdaptationHeaderTest::DefaultCommandLine() const
{
    return "t3gpadapthdr t3gpadapthdr.in";
}
    
void C3gpAdaptationHeaderTest::GetCommandInfo(UTVector<HLXUnitTestCmdInfo*>& cmds)
{
    int i = 0;

    cmds.Resize(i + 1);
    HX_ASSERT(i < cmds.Nelements());
    cmds[i++] = new HLXUnitTestCmdInfoDisp<C3gpAdaptationHeaderTest>(
        this,
        "Construct",
        &C3gpAdaptationHeaderTest::HandleConstructCmd,
        2);

    cmds.Resize(i + 1);
    HX_ASSERT(i < cmds.Nelements());
    cmds[i++] = new HLXUnitTestCmdInfoDisp<C3gpAdaptationHeaderTest>(
        this,
        "GetValues",
        &C3gpAdaptationHeaderTest::HandleGetValuesCmd,
        3);

    cmds.Resize(i + 1);
    HX_ASSERT(i < cmds.Nelements());
    cmds[i++] = new HLXUnitTestCmdInfoDisp<C3gpAdaptationHeaderTest>(
        this,
        "Parse",
        &C3gpAdaptationHeaderTest::HandleParseCmd,
        4);

    cmds.Resize(i + 1);
    HX_ASSERT(i < cmds.Nelements());
    cmds[i++] = new HLXUnitTestCmdInfoDisp<C3gpAdaptationHeaderTest>(
        this,
        "Init",
        &C3gpAdaptationHeaderTest::HandleInitCmd,
        3);

    cmds.Resize(i + 1);
    HX_ASSERT(i < cmds.Nelements());
    cmds[i++] = new HLXUnitTestCmdInfoDisp<C3gpAdaptationHeaderTest>(
        this,
        "GetStringProp",
        &C3gpAdaptationHeaderTest::HandleGetStringPropCmd,
        4);

    cmds.Resize(i + 1);
    HX_ASSERT(i < cmds.Nelements());
    cmds[i++] = new HLXUnitTestCmdInfoDisp<C3gpAdaptationHeaderTest>(
        this,
        "GetULONG32Prop",
        &C3gpAdaptationHeaderTest::HandleGetULONG32PropCmd,
        4);

    cmds.Resize(i + 1);
    HX_ASSERT(i < cmds.Nelements());
    cmds[i++] = new HLXUnitTestCmdInfoDisp<C3gpAdaptationHeaderTest>(
        this,
        "Compare",
        &C3gpAdaptationHeaderTest::HandleCompareCmd,
        2);

    cmds.Resize(i + 1);
    HX_ASSERT(i < cmds.Nelements());
    cmds[i++] = new HLXUnitTestCmdInfoDisp<C3gpAdaptationHeaderTest>(
        this,
        "CreateValues",
        &C3gpAdaptationHeaderTest::HandleCreateValuesCmd,
        1);

    cmds.Resize(i + 1);
    HX_ASSERT(i < cmds.Nelements());
    cmds[i++] = new HLXUnitTestCmdInfoDisp<C3gpAdaptationHeaderTest>(
        this,
        "SetValues",
        &C3gpAdaptationHeaderTest::HandleSetValuesCmd,
        3);

    cmds.Resize(i + 1);
    HX_ASSERT(i < cmds.Nelements());
    cmds[i++] = new HLXUnitTestCmdInfoDisp<C3gpAdaptationHeaderTest>(
        this,
        "SetStringProp",
        &C3gpAdaptationHeaderTest::HandleSetStringPropCmd,
        3);

    cmds.Resize(i + 1);
    HX_ASSERT(i < cmds.Nelements());
    cmds[i++] = new HLXUnitTestCmdInfoDisp<C3gpAdaptationHeaderTest>(
        this,
        "SetULONG32Prop",
        &C3gpAdaptationHeaderTest::HandleSetULONG32PropCmd,
        3);

    cmds.Resize(i + 1);
    HX_ASSERT(i < cmds.Nelements());
    cmds[i++] = new HLXUnitTestCmdInfoDisp<C3gpAdaptationHeaderTest>(
        this,
        "GetString",
        &C3gpAdaptationHeaderTest::HandleGetStringCmd,
        4);
}

HLXCmdBasedTest* C3gpAdaptationHeaderTest::Clone() const
{
    return new C3gpAdaptationHeaderTest();
}

bool 
C3gpAdaptationHeaderTest::HandleConstructCmd(const UTVector<UTString>& info)
{
    bool ret = false;
    
    if (info[1] == "A")
    {
        HX_DELETE(m_pA);
        
        m_pA = new C3gpAdaptationHeader();

        if (m_pA)
        {
            ret = true;
        }
    }
    else if (info[1] == "B")
    {
        HX_DELETE(m_pB);
        
        m_pB = new C3gpAdaptationHeader();

        if (m_pB)
        {
            ret = true;
        }
    }
    else
    {
        DPRINTF(D_ERROR, ("Invalid header ID %s\n", info[1]));
    }

    return ret;
}

bool 
C3gpAdaptationHeaderTest::HandleGetValuesCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    C3gpAdaptationHeader* pHdr = GetHdr(info[1]);
    unsigned int expected;

    if (!pHdr)
    {
        DPRINTF(D_ERROR, ("Invalid header ID %s\n", info[1]));
    }
    else if (!UTParamUtil::GetUInt(info[2], expected))
    {
        DPRINTF(D_ERROR, ("Failed to convert parameter\n"));
    }
    else
    {
        HX_RELEASE(m_pValues);
        HX_RESULT res = pHdr->GetValues(m_pValues);

        if ((unsigned int)res != expected)
        {
            DPRINTF(D_ERROR, ("Got %08x, expected %08x\n",
                              res, expected));
        }
        else
        {
            ret = true;
        }
    }

    return ret;
}

bool 
C3gpAdaptationHeaderTest::HandleParseCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    C3gpAdaptationHeader* pHdr = GetHdr(info[1]);
    unsigned int expected;

    if (!pHdr)
    {
        DPRINTF(D_ERROR, ("Invalid header ID %s\n", info[1]));
    }
    else if (!UTParamUtil::GetUInt(info[2], expected))
    {
        DPRINTF(D_ERROR, ("Failed to convert parameter\n"));
    }
    else
    {
        HX_RESULT res = pHdr->Parse(info[3], info[3].length());

        if ((unsigned int)res != expected)
        {
            DPRINTF(D_ERROR, ("Got %08x, expected %08x\n",
                              res, expected));
        }
        else
        {
            ret = true;
        }
    }

    return ret;
}

bool 
C3gpAdaptationHeaderTest::HandleInitCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    C3gpAdaptationHeader* pHdr = GetHdr(info[1]);
    unsigned int expected;

    if (!pHdr)
    {
        DPRINTF(D_ERROR, ("Invalid header ID %s\n", info[1]));
    }
    else if (!UTParamUtil::GetUInt(info[2], expected))
    {
        DPRINTF(D_ERROR, ("Failed to convert parameter\n"));
    }
    else
    {
        HX_RESULT res = pHdr->Init(m_pContext);

        if ((unsigned int)res != expected)
        {
            DPRINTF(D_ERROR, ("Got %08x, expected %08x\n",
                              res, expected));
        }
        else
        {
            ret = true;
        }
    }

    return ret;
}

bool 
C3gpAdaptationHeaderTest::HandleGetStringPropCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    unsigned int expected;

    if (!UTParamUtil::GetUInt(info[2], expected))
    {
        DPRINTF(D_ERROR, ("Failed to convert parameter\n"));
    }
    else if (!m_pValues)
    {
        DPRINTF(D_ERROR, ("m_pValues not set\n"));
    }
    else
    {
        IHXBuffer* pBuf = NULL;
        HX_RESULT res = m_pValues->GetPropertyCString(info[1], pBuf);

        if ((unsigned int)res != expected)
        {
            DPRINTF(D_ERROR, ("Got %08x, expected %08x\n",
                              res, expected));
        }
        else if (HXR_OK != res)
        {
            // We got the expected error code
            ret = true;
        }
        else if (pBuf->GetSize() !=  (UINT32)(info[3].length() + 1))
        {
            DPRINTF(D_ERROR, ("Strings are not the same length\n"));
        }
        else if (strncmp((char*)pBuf->GetBuffer(), info[3], info[3].length()))
        {
            DPRINTF(D_ERROR, ("Got '%s', expected '%s'\n",
                              pBuf->GetBuffer(),
                              info[3]));
        }
        else
        {
            ret = true;
        }

        HX_RELEASE(pBuf);
    }

    return ret;
}

bool 
C3gpAdaptationHeaderTest::HandleGetULONG32PropCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    unsigned int expectedRes;
    unsigned int expectedValue;
    if (!UTParamUtil::GetUInt(info[2], expectedRes) ||
        !UTParamUtil::GetUInt(info[3], expectedValue))
    {
        DPRINTF(D_ERROR, ("Failed to convert parameter\n"));
    }
    else if (!m_pValues)
    {
        DPRINTF(D_ERROR, ("m_pValues not set\n"));
    }
    else
    {
        ULONG32 ulValue;
        HX_RESULT res = m_pValues->GetPropertyULONG32(info[1], ulValue);

        if ((unsigned int)res != expectedRes)
        {
            DPRINTF(D_ERROR, ("Got %08x, expected %08x\n",
                              res, expectedRes));
        }
        else if (HXR_OK != res)
        {
            // We got the expected error code
            ret = true;
        }
        else if ((unsigned int)ulValue != expectedValue)
        {
            DPRINTF(D_ERROR, ("Got %lu, expected %u\n",
                              ulValue, expectedValue));
        }
        else
        {
            ret = true;
        }
    }

    return ret;
}

bool C3gpAdaptationHeaderTest::HandleCompareCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    bool expected;

    if (!UTParamUtil::GetBool(info[1], expected))
    {
        DPRINTF(D_ERROR, ("Failed to convert parameter\n"));
    }
    else if (!m_pA)
    {
        DPRINTF(D_ERROR, ("m_pA not set\n"));
    }
    else if (!m_pB)
    {
        DPRINTF(D_ERROR, ("m_pB not set\n"));
    }
    else
    {
        bool res = (*m_pA == *m_pB);

        if (res != expected)
        {
            DPRINTF(D_ERROR, ("Got %u, expected %u\n", res, expected));
        }
        else
        {
            ret = true;
        }
    }

    return ret;
}


bool 
C3gpAdaptationHeaderTest::HandleCreateValuesCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    HX_RELEASE(m_pValues);

    if (HXR_OK == CreateValuesCCF(m_pValues, m_pContext))
    {
        ret = true;
    }
    
    return ret;
}

bool 
C3gpAdaptationHeaderTest::HandleSetValuesCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    C3gpAdaptationHeader* pHdr = GetHdr(info[1]);
    unsigned int expected;

    if (!pHdr)
    {
        DPRINTF(D_ERROR, ("Invalid header ID %s\n", info[1]));
    }
    else if (!UTParamUtil::GetUInt(info[2], expected))
    {
        DPRINTF(D_ERROR, ("Failed to convert parameter\n"));
    }
    else
    {
        HX_RESULT res = pHdr->SetValues(m_pValues);

        if ((unsigned int)res != expected)
        {
            DPRINTF(D_ERROR, ("Got %08x, expected %08x\n",
                              res, expected));
        }
        else
        {
            ret = true;
        }
    }

    return ret;
}

bool 
C3gpAdaptationHeaderTest::HandleSetStringPropCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    if (!m_pValues)
    {
        DPRINTF(D_ERROR, ("m_pValues not set\n"));
    }
    else if (HXR_OK == SetCStringPropertyCCF(m_pValues, 
                                             info[1], info[2], m_pContext))
    {
        ret = true;
    }

    return ret;
}

bool 
C3gpAdaptationHeaderTest::HandleSetULONG32PropCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    unsigned int value;
    if (!UTParamUtil::GetUInt(info[2], value))
    {
        DPRINTF(D_ERROR, ("Failed to convert parameter\n"));
    }
    else if (!m_pValues)
    {
        DPRINTF(D_ERROR, ("m_pValues not set\n"));
    }
    else if (HXR_OK == m_pValues->SetPropertyULONG32(info[1], 
                                                     (ULONG32)value))
    {
        ret = true;
    }

    return ret;
}

bool 
C3gpAdaptationHeaderTest::HandleGetStringCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    C3gpAdaptationHeader* pHdr = GetHdr(info[1]);
    unsigned int expected;

    if (!pHdr)
    {
        DPRINTF(D_ERROR, ("Invalid header ID %s\n", info[1]));
    }
    else if (!UTParamUtil::GetUInt(info[2], expected))
    {
        DPRINTF(D_ERROR, ("Failed to convert parameter\n"));
    }
    else
    {
        IHXBuffer* pStr = NULL;
        HX_RESULT res = pHdr->GetString(pStr);

        if ((unsigned int)res != expected)
        {
            DPRINTF(D_ERROR, ("Got %08x, expected %08x\n",
                              res, expected));
        }
        else if (HXR_OK != res)
        {
            // We expected a failure
            ret = true;
        }
        else if ((pStr->GetSize() != (info[3].length() + 1)) ||
                 (memcmp(pStr->GetBuffer(), info[3], pStr->GetSize())))
        {
            DPRINTF(D_ERROR, ("Got '%s', expected '%s'\n",
                              pStr->GetBuffer(), 
                              (const char*)info[3]));
        }
        else
        {
            ret = true;
        }

        HX_RELEASE(pStr);
    }

    return ret;
}


C3gpAdaptationHeader* 
C3gpAdaptationHeaderTest::GetHdr(const UTString& name)
{
    C3gpAdaptationHeader* pRet = NULL;

    if (name == "A")
    {
        pRet = m_pA;
    }
    else if (name == "B")
    {
        pRet = m_pB;
    }

    return pRet;
}
