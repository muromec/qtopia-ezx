/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: ihxvalues2_test.cpp,v 1.2 2007/07/06 20:35:03 jfinnecy Exp $ 
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
#include "./ihxvalues2_test.h"

#include "hx_ut_debug.h"
#include "ut_param_util.h"
#include "./param_util.h"

CIHXValues2Test::CIHXValues2Test() :
    m_pA(NULL),
    m_pB(NULL)
{}

CIHXValues2Test::~CIHXValues2Test()
{
    HX_RELEASE(m_pA);
    HX_RELEASE(m_pB);
}

const char* CIHXValues2Test::DefaultCommandLine() const
{
    return "tihxvalues2 tihxvalues2.in";
}
    
void CIHXValues2Test::GetCommandInfo(UTVector<HLXUnitTestCmdInfo*>& cmds)
{
    int i = 0;

    cmds.Resize(i + 1);
    cmds[i] = new HLXUnitTestCmdInfoDisp<CIHXValues2Test>(this, 
				    "SetPropertyULONG32",
				    &CIHXValues2Test::HandleSetPropertyULONG32Cmd,
				    5);
    i++;

    cmds.Resize(i + 1);
    cmds[i] = new HLXUnitTestCmdInfoDisp<CIHXValues2Test>(this, 
				    "GetPropertyULONG32",
				    &CIHXValues2Test::HandleGetPropertyULONG32Cmd,
				    5);
    i++;

    cmds.Resize(i + 1);
    cmds[i] = new HLXUnitTestCmdInfoDisp<CIHXValues2Test>(this, 
				    "GetFirstPropertyULONG32",
				    &CIHXValues2Test::HandleGetFirstPropertyULONG32Cmd,
				    5);
    i++;

    cmds.Resize(i + 1);
    cmds[i] = new HLXUnitTestCmdInfoDisp<CIHXValues2Test>(this, 
				    "GetNextPropertyULONG32",
				    &CIHXValues2Test::HandleGetNextPropertyULONG32Cmd,
				    5);
    i++;

    cmds.Resize(i + 1);
    cmds[i] = new HLXUnitTestCmdInfoDisp<CIHXValues2Test>(this, 
				    "SetPropertyBuffer",
				    &CIHXValues2Test::HandleSetPropertyBufferCmd,
				    5);
    i++;

    cmds.Resize(i + 1);
    cmds[i] = new HLXUnitTestCmdInfoDisp<CIHXValues2Test>(this, 
				    "GetPropertyBuffer",
				    &CIHXValues2Test::HandleGetPropertyBufferCmd,
				    5);
    i++;

    cmds.Resize(i + 1);
    cmds[i] = new HLXUnitTestCmdInfoDisp<CIHXValues2Test>(this, 
				    "GetFirstPropertyBuffer",
				    &CIHXValues2Test::HandleGetFirstPropertyBufferCmd,
				    5);
    i++;

    cmds.Resize(i + 1);
    cmds[i] = new HLXUnitTestCmdInfoDisp<CIHXValues2Test>(this, 
				    "GetNextPropertyBuffer",
				    &CIHXValues2Test::HandleGetNextPropertyBufferCmd,
				    5);
    i++;

    cmds.Resize(i + 1);
    cmds[i] = new HLXUnitTestCmdInfoDisp<CIHXValues2Test>(this, 
				    "SetPropertyCString",
				    &CIHXValues2Test::HandleSetPropertyCStringCmd,
				    5);
    i++;

    cmds.Resize(i + 1);
    cmds[i] = new HLXUnitTestCmdInfoDisp<CIHXValues2Test>(this, 
				    "GetPropertyCString",
				    &CIHXValues2Test::HandleGetPropertyCStringCmd,
				    5);
    i++;

    cmds.Resize(i + 1);
    cmds[i] = new HLXUnitTestCmdInfoDisp<CIHXValues2Test>(this, 
				    "GetFirstPropertyCString",
				    &CIHXValues2Test::HandleGetFirstPropertyCStringCmd,
				    5);
    i++;

    cmds.Resize(i + 1);
    cmds[i] = new HLXUnitTestCmdInfoDisp<CIHXValues2Test>(this, 
				    "GetNextPropertyCString",
				    &CIHXValues2Test::HandleGetNextPropertyCStringCmd,
				    5);
    i++;

    
    cmds.Resize(i + 1);
    cmds[i] = new HLXUnitTestCmdInfoDisp<CIHXValues2Test>(this, 
				    "SetPropertyObject",
				    &CIHXValues2Test::HandleSetPropertyObjectCmd,
				    5);
    i++;

    cmds.Resize(i + 1);
    cmds[i] = new HLXUnitTestCmdInfoDisp<CIHXValues2Test>(this, 
				    "GetPropertyObject",
				    &CIHXValues2Test::HandleGetPropertyObjectCmd,
				    5);
    i++;

    cmds.Resize(i + 1);
    cmds[i] = new HLXUnitTestCmdInfoDisp<CIHXValues2Test>(this, 
				    "GetFirstPropertyObject",
				    &CIHXValues2Test::HandleGetFirstPropertyObjectCmd,
				    5);
    i++;

    cmds.Resize(i + 1);
    cmds[i] = new HLXUnitTestCmdInfoDisp<CIHXValues2Test>(this, 
				    "GetNextPropertyObject",
				    &CIHXValues2Test::HandleGetNextPropertyObjectCmd,
				    5);
    i++;


    cmds.Resize(i + 1);
    cmds[i] = new HLXUnitTestCmdInfoDisp<CIHXValues2Test>(this, 
				    "Remove",
				    &CIHXValues2Test::HandleRemoveCmd,
				    4);
    i++;
    
    cmds.Resize(i + 1);
    cmds[i] = new HLXUnitTestCmdInfoDisp<CIHXValues2Test>(this, 
				    "RemoveULONG32",
				    &CIHXValues2Test::HandleRemoveULONG32Cmd,
				    4);
    i++;
    
    cmds.Resize(i + 1);
    cmds[i] = new HLXUnitTestCmdInfoDisp<CIHXValues2Test>(this, 
				    "RemoveBuffer",
				    &CIHXValues2Test::HandleRemoveBufferCmd,
				    4);
    i++;
    
    cmds.Resize(i + 1);
    cmds[i] = new HLXUnitTestCmdInfoDisp<CIHXValues2Test>(this, 
				    "RemoveCString",
				    &CIHXValues2Test::HandleRemoveCStringCmd,
				    4);
    i++;
    
    cmds.Resize(i + 1);
    cmds[i] = new HLXUnitTestCmdInfoDisp<CIHXValues2Test>(this, 
				    "RemoveObject",
				    &CIHXValues2Test::HandleRemoveObjectCmd,
				    4);
    i++;

    cmds.Resize(i + 1);
    cmds[i] = new HLXUnitTestCmdInfoDisp<CIHXValues2Test>(this, 
				    "CheckQueryInterface",
				    &CIHXValues2Test::HandleCheckQueryInterfaceCmd,
				    2);
    i++;

    cmds.Resize(i + 1);
    cmds[i] = new HLXUnitTestCmdInfoDisp<CIHXValues2Test>(this, 
				    "CreateIHXValues2",
				    &CIHXValues2Test::HandleCreateIHXValues2Cmd,
				    2);
    i++;
}

HLXCmdBasedTest* CIHXValues2Test::Clone() const
{
    return new CIHXValues2Test();
}

bool 
CIHXValues2Test::HandleSetPropertyULONG32Cmd(const UTVector<UTString>& info)
{
    bool ret = false;
    unsigned int expected = 0;
    unsigned int val = 0;
    
    IHXValues2* pValues = NULL;
    
    if (!getValues(info[1], pValues) ||
        !UTParamUtil::GetUInt(info[2], expected) || 
        !UTParamUtil::GetUInt(info[4], val))
    {
        DPRINTF (D_ERROR, ("CIHXValues2Test::HandleSetPropertyULONG32Cmd : failed to convert parameters\n"));
    }
    else
    {
        HX_RESULT res = pValues->SetPropertyULONG32(info[3], val);
        
        if (res != (HX_RESULT)expected)
        {
            DPRINTF (D_ERROR, ("CIHXValues2Test::HandleSetPropertyULONG32Cmd : expected %08x got %08x\n", expected, res));
        }
        else
        {
            ret = true;
        }
    }

    return ret;
}

bool 
CIHXValues2Test::HandleGetPropertyULONG32Cmd(const UTVector<UTString>& info)
{
    bool ret = false;
    unsigned int expectedRes;
    unsigned int expectedVal;
    
    IHXValues2* pValues = NULL;
    
    if (!getValues(info[1], pValues) ||
        !UTParamUtil::GetUInt(info[2], expectedRes) ||
        !UTParamUtil::GetUInt(info[4], expectedVal))
    {
        DPRINTF (D_ERROR, ("CIHXValues2Test::HandleGetPropertyULONG32Cmd : failed to convert parameters\n"));
    }
    else
    {
        ULONG32 val;
        HX_RESULT res = pValues->GetPropertyULONG32(info[3], val);

        if (res != (HX_RESULT) expectedRes)
        {
            DPRINTF (D_ERROR, ("CIHXValues2Test::HandleGetPropertyULONG32Cmd : expected 0x%08x got 0x%08x\n",
                               expectedRes, res));
        }
        else if ((HXR_OK == res) &&
                 (val != expectedVal))
        {
            DPRINTF (D_ERROR, ("CIHXValues2Test::HandleGetPropertyULONG32Cmd : expected %u got %u\n",
                               expectedVal, val));
        }
        else
        {
            ret = true;
        }
    }
    
    return ret;
}

bool 
CIHXValues2Test::HandleGetFirstPropertyULONG32Cmd(const UTVector<UTString>& info)
{
    bool ret = false;

    unsigned int expectedRes = 0;
    unsigned int expectedVal = 0;
    IHXValues2* pSrc = NULL; 
    
    if (!getValues(info[1], pSrc) ||
        !UTParamUtil::GetUInt(info[2], expectedRes) ||
        !UTParamUtil::GetUInt(info[4], expectedVal))
    {
        DPRINTF (D_ERROR, ("CIHXValues2Test::HandleGetFirstPropertyULONG32Cmd : failed to convert parameters\n"));
    }
    else
    {
        const char* pKey = NULL;
        ULONG32 val;

        HX_RESULT res = pSrc->GetFirstPropertyULONG32(pKey, val);
        
        if (res != (HX_RESULT)expectedRes)
        {
            DPRINTF (D_ERROR, ("CIHXValues2Test::HandleGetFirstPropertyULONG32Cmd : expected %08x got %08x\n", expectedRes, res));
        }
        else if (HXR_OK == res)
        {
            if (info[3] != pKey)
            {
                DPRINTF (D_ERROR, ("CIHXValues2Test::HandleGetFirstPropertyULONG32Cmd : expected key '%s' got '%s'\n", (const char*)info[3], pKey));
            }
            else if (expectedVal != val)
            {
                DPRINTF (D_ERROR, ("CIHXValues2Test::HandleGetFirstPropertyULONG32Cmd : expected val %u got '%u\n", 
                                   expectedVal,
                                   val));
            }
            else
            {
                ret = true;
            }
        }
        else
        {
            // handle HXR_OK != res case
            ret = true;
        }
    }

    return ret;
}

bool 
CIHXValues2Test::HandleGetNextPropertyULONG32Cmd(const UTVector<UTString>& info)
{
    bool ret = false;

    unsigned int expectedRes = 0;
    unsigned int expectedVal = 0;
    IHXValues2* pSrc = NULL; 
    
    if (!getValues(info[1], pSrc) ||
        !UTParamUtil::GetUInt(info[2], expectedRes) ||
        !UTParamUtil::GetUInt(info[4], expectedVal))
    {
        DPRINTF (D_ERROR, ("CIHXValues2Test::HandleGetNextPropertyULONG32Cmd : failed to convert parameters\n"));
    }
    else
    {
        const char* pKey = NULL;
        ULONG32 val;

        HX_RESULT res = pSrc->GetNextPropertyULONG32(pKey, val);
        
        if (res != (HX_RESULT)expectedRes)
        {
            DPRINTF (D_ERROR, ("CIHXValues2Test::HandleGetNextPropertyULONG32Cmd : expected %08x got %08x\n", expectedRes, res));
        }
        else if (HXR_OK == res)
        {
            if (info[3] != pKey)
            {
                DPRINTF (D_ERROR, ("CIHXValues2Test::HandleGetNextPropertyULONG32Cmd : expected key '%s' got '%s'\n", (const char*)info[3], pKey));
            }
            else if (expectedVal != val)
            {
                DPRINTF (D_ERROR, ("CIHXValues2Test::HandleGetNextPropertyULONG32Cmd : expected val %u got '%u\n", 
                                   expectedVal,
                                   val));
            }
            else
            {
                ret = true;
            }
        }
        else
        {
            // handle HXR_OK != res case
            ret = true;
        }
    }

    return ret;
}

bool 
CIHXValues2Test::HandleSetPropertyBufferCmd(const UTVector<UTString>& info)
{
    bool ret = false;
    unsigned int expected = 0;
    
    IHXValues2* pValues = NULL;
    
    if (!getValues(info[1], pValues) ||
        !UTParamUtil::GetUInt(info[2], expected))
    {
        DPRINTF (D_ERROR, ("CIHXValues2Test::HandleSetPropertyBufferCmd : failed to convert parameters\n"));
    }
    else
    {
        IHXBuffer* pBuf = stringToBuffer(info[4]);

        if (pBuf)
        {
            HX_RESULT res = pValues->SetPropertyBuffer(info[3], pBuf);
            
            if (res != (HX_RESULT)expected)
            {
                DPRINTF (D_ERROR, ("CIHXValues2Test::HandleSetPropertyBufferCmd : expected %08x got %08x\n", expected, res));
            }
            else
            {
                ret = true;
            }
        }
        HX_RELEASE(pBuf);
    }

    return ret;
}

bool 
CIHXValues2Test::HandleGetPropertyBufferCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    unsigned int expectedRes;

    IHXValues2* pValues = NULL;
    
    if (!getValues(info[1], pValues) ||
        !UTParamUtil::GetUInt(info[2], expectedRes))
    {
        DPRINTF (D_ERROR, ("CIHXValues2Test::HandleGetPropertyBufferCmd : failed to convert parameters\n"));
    }
    else
    {
        IHXBuffer* pExpected = stringToBuffer(info[4]);
        IHXBuffer* pVal = NULL;

        HX_RESULT res = pValues->GetPropertyBuffer(info[3], pVal);
        
        if (!pExpected)
        {
            DPRINTF (D_ERROR, ("CIHXValues2Test::HandleGetPropertyBufferCmd : failed to create expected buffer\n"));
        }
        else if (res != (HX_RESULT) expectedRes)
        {
            DPRINTF (D_ERROR, ("CIHXValues2Test::HandleGetPropertyBufferCmd : expected 0x%08x got 0x%08x\n",
                               expectedRes, res));
        }
        else if ((HXR_OK == res) &&
                 ((pExpected->GetSize() != pVal->GetSize()) ||
                  (memcmp(pExpected->GetBuffer(), pVal->GetBuffer(), 
                          pVal->GetSize()))))
        {
            DPRINTF (D_ERROR, ("CIHXValues2Test::HandleGetPropertyBufferCmd : expected '%s' got '%s'\n",
                               pExpected->GetBuffer(), 
                               pVal->GetBuffer()));
        }
        else
        {
            ret = true;
        }

        HX_RELEASE(pExpected);
        HX_RELEASE(pVal);
    }

    return ret;
}

bool 
CIHXValues2Test::HandleGetFirstPropertyBufferCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    unsigned int expected = 0;
    IHXValues2* pSrc = NULL; 

    if (!getValues(info[1], pSrc) ||
        !UTParamUtil::GetUInt(info[2], expected))
    {
        DPRINTF (D_ERROR, ("CIHXValues2Test::HandleGetFirstPropertyBufferCmd : failed to convert parameters\n"));
    }
    else
    {
        IUnknown* pUnk = NULL;
        const char* pKey = NULL;
        IHXBuffer* pExpected = stringToBuffer(info[4]);
        IHXBuffer* pVal = NULL;
        HX_RESULT res = pSrc->GetFirstPropertyBuffer(pKey, pVal);
        
        if (!pExpected)
        {
            DPRINTF (D_ERROR, ("CIHXValues2Test::HandleGetFirstPropertyBufferCmd : failed to create expected buffer\n"));
        }
        else if (res != (HX_RESULT)expected)
        {
            DPRINTF (D_ERROR, ("CIHXValues2Test::HandleGetFirstPropertyBufferCmd : expected %08x got %08x\n", expected, res));
        }
        else if (HXR_OK == res)
        {
            if (info[3] != pKey)
            {
                DPRINTF (D_ERROR, ("CIHXValues2Test::HandleGetFirstPropertyBufferCmd : expected key '%s' got '%s'\n", (const char*)info[3], pKey));
            }
            else if ((pExpected->GetSize() != pVal->GetSize()) ||
                     (memcmp(pExpected->GetBuffer(), pVal->GetBuffer(),
                             pExpected->GetSize())))
            {
                DPRINTF (D_ERROR, ("CIHXValues2Test::HandleGetFirstPropertyBufferCmd : expected val '%s' got '%s'\n", 
                                   pExpected->GetBuffer(),
                                   pVal->GetBuffer()));
            }
            else
            {
                ret = true;
            }
        }
        else
        {
            // handle HXR_OK != res case
            ret = true;
        }
        
        HX_RELEASE(pExpected);
        HX_RELEASE(pVal);
    }

    return ret;
}

bool 
CIHXValues2Test::HandleGetNextPropertyBufferCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    unsigned int expected = 0;
    IHXValues2* pSrc = NULL; 

    if (!getValues(info[1], pSrc) ||
        !UTParamUtil::GetUInt(info[2], expected))
    {
        DPRINTF (D_ERROR, ("CIHXValues2Test::HandleGetNextPropertyBufferCmd : failed to convert parameters\n"));
    }
    else
    {
        IUnknown* pUnk = NULL;
        const char* pKey = NULL;
        IHXBuffer* pExpected = stringToBuffer(info[4]);
        IHXBuffer* pVal = NULL;
        HX_RESULT res = pSrc->GetNextPropertyBuffer(pKey, pVal);
        
        if (!pExpected)
        {
            DPRINTF (D_ERROR, ("CIHXValues2Test::HandleGetNextPropertyBufferCmd : failed to create expected buffer\n"));
        }
        else if (res != (HX_RESULT)expected)
        {
            DPRINTF (D_ERROR, ("CIHXValues2Test::HandleGetNextPropertyBufferCmd : expected %08x got %08x\n", expected, res));
        }
        else if (HXR_OK == res)
        {
            if (info[3] != pKey)
            {
                DPRINTF (D_ERROR, ("CIHXValues2Test::HandleGetNextPropertyBufferCmd : expected key '%s' got '%s'\n", (const char*)info[3], pKey));
            }
            else if ((pExpected->GetSize() != pVal->GetSize()) ||
                     (memcmp(pExpected->GetBuffer(), pVal->GetBuffer(),
                             pExpected->GetSize())))
            {
                DPRINTF (D_ERROR, ("CIHXValues2Test::HandleGetNextPropertyBufferCmd : expected val '%s' got '%s'\n", 
                                   pExpected->GetBuffer(),
                                   pVal->GetBuffer()));
            }
            else
            {
                ret = true;
            }
        }
        else
        {
            // handle HXR_OK != res case
            ret = true;
        }
        
        HX_RELEASE(pExpected);
        HX_RELEASE(pVal);
    }

    return ret;
}

bool 
CIHXValues2Test::HandleSetPropertyCStringCmd(const UTVector<UTString>& info)
{
    bool ret = false;
    unsigned int expected = 0;
    
    IHXValues2* pValues = NULL;
    
    if (!getValues(info[1], pValues) ||
        !UTParamUtil::GetUInt(info[2], expected))
    {
        DPRINTF (D_ERROR, ("CIHXValues2Test::HandleSetPropertyBufferCmd : failed to convert parameters\n"));
    }
    else
    {
        IHXBuffer* pBuf = stringToBuffer(info[4]);

        if (pBuf)
        {
            HX_RESULT res = pValues->SetPropertyCString(info[3], pBuf);
            
            if (res != (HX_RESULT)expected)
            {
                DPRINTF (D_ERROR, ("CIHXValues2Test::HandleSetPropertyCStringCmd : expected %08x got %08x\n", expected, res));
            }
            else
            {
                ret = true;
            }
        }
        HX_RELEASE(pBuf);
    }

    return ret;
}

bool 
CIHXValues2Test::HandleGetPropertyCStringCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    unsigned int expectedRes;

    IHXValues2* pValues = NULL;
    
    if (!getValues(info[1], pValues) ||
        !UTParamUtil::GetUInt(info[2], expectedRes))
    {
        DPRINTF (D_ERROR, ("CIHXValues2Test::HandleGetPropertyCStringCmd : failed to convert parameters\n"));
    }
    else
    {
        IHXBuffer* pExpected = stringToBuffer(info[4]);
        IHXBuffer* pVal = NULL;

        HX_RESULT res = pValues->GetPropertyCString(info[3], pVal);
        
        if (!pExpected)
        {
            DPRINTF (D_ERROR, ("CIHXValues2Test::HandleGetPropertyCStringCmd : failed to create expected buffer\n"));
        }
        else if (res != (HX_RESULT) expectedRes)
        {
            DPRINTF (D_ERROR, ("CIHXValues2Test::HandleGetPropertyCStringCmd : expected 0x%08x got 0x%08x\n",
                               expectedRes, res));
        }
        else if ((HXR_OK == res) &&
                 ((pExpected->GetSize() != pVal->GetSize()) ||
                  (memcmp(pExpected->GetBuffer(), pVal->GetBuffer(), 
                          pVal->GetSize()))))
        {
            DPRINTF (D_ERROR, ("CIHXValues2Test::HandleGetPropertyCStringCmd : expected '%s' got '%s'\n",
                               pExpected->GetBuffer(), 
                               pVal->GetBuffer()));
        }
        else
        {
            ret = true;
        }

        HX_RELEASE(pExpected);
        HX_RELEASE(pVal);
    }

    return ret;
}

bool 
CIHXValues2Test::HandleGetFirstPropertyCStringCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    unsigned int expected = 0;
    IHXValues2* pSrc = NULL; 
    
    if (!getValues(info[1], pSrc) ||
        !UTParamUtil::GetUInt(info[2], expected))
    {
        DPRINTF (D_ERROR, ("CIHXValues2Test::HandleGetFirstPropertyCStringCmd : failed to convert parameters\n"));
    }
    else
    {
        IUnknown* pUnk = NULL;
        const char* pKey = NULL;
        IHXBuffer* pExpected = stringToBuffer(info[4]);
        IHXBuffer* pVal = NULL;
        HX_RESULT res = pSrc->GetFirstPropertyCString(pKey, pVal);
        
        if (!pExpected)
        {
            DPRINTF (D_ERROR, ("CIHXValues2Test::HandleGetFirstPropertyCStringCmd : failed to create expected buffer\n"));
        }
        else if (res != (HX_RESULT)expected)
        {
            DPRINTF (D_ERROR, ("CIHXValues2Test::HandleGetFirstPropertyCStringCmd : expected %08x got %08x\n", expected, res));
        }
        else if (HXR_OK == res)
        {
            if (info[3] != pKey)
            {
                DPRINTF (D_ERROR, ("CIHXValues2Test::HandleGetFirstPropertyCStringCmd : expected key '%s' got '%s'\n", (const char*)info[3], pKey));
            }
            else if ((pExpected->GetSize() != pVal->GetSize()) ||
                     (memcmp(pExpected->GetBuffer(), pVal->GetBuffer(),
                             pExpected->GetSize())))
            {
                DPRINTF (D_ERROR, ("CIHXValues2Test::HandleGetFirstPropertyCStringCmd : expected val '%s' got '%s'\n", 
                                   pExpected->GetBuffer(),
                                   pVal->GetBuffer()));
            }
            else
            {
                ret = true;
            }
        }
        else
        {
            // handle HXR_OK != res case
            ret = true;
        }
        
        HX_RELEASE(pExpected);
        HX_RELEASE(pVal);
    }

    return ret;
}

bool 
CIHXValues2Test::HandleGetNextPropertyCStringCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    unsigned int expected = 0;
    IHXValues2* pSrc = NULL; 

    if (!getValues(info[1], pSrc) ||
        !UTParamUtil::GetUInt(info[2], expected))
    {
        DPRINTF (D_ERROR, ("CIHXValues2Test::HandleGetNextPropertyCStringCmd : failed to convert parameters\n"));
    }
    else
    {
        IUnknown* pUnk = NULL;
        const char* pKey = NULL;
        IHXBuffer* pExpected = stringToBuffer(info[4]);
        IHXBuffer* pVal = NULL;
        HX_RESULT res = pSrc->GetNextPropertyCString(pKey, pVal);
        
        if (!pExpected)
        {
            DPRINTF (D_ERROR, ("CIHXValues2Test::HandleGetNextPropertyCStringCmd : failed to create expected buffer\n"));
        }
        else if (res != (HX_RESULT)expected)
        {
            DPRINTF (D_ERROR, ("CIHXValues2Test::HandleGetNextPropertyCStringCmd : expected %08x got %08x\n", expected, res));
        }
        else if (HXR_OK == res)
        {
            if (info[3] != pKey)
            {
                DPRINTF (D_ERROR, ("CIHXValues2Test::HandleGetNextPropertyCStringCmd : expected key '%s' got '%s'\n", (const char*)info[3], pKey));
            }
            else if ((pExpected->GetSize() != pVal->GetSize()) ||
                     (memcmp(pExpected->GetBuffer(), pVal->GetBuffer(),
                             pExpected->GetSize())))
            {
                DPRINTF (D_ERROR, ("CIHXValues2Test::HandleGetNextPropertyCStringCmd : expected val '%s' got '%s'\n", 
                                   pExpected->GetBuffer(),
                                   pVal->GetBuffer()));
            }
            else
            {
                ret = true;
            }
        }
        else
        {
            // handle HXR_OK != res case
            ret = true;
        }
        
        HX_RELEASE(pExpected);
        HX_RELEASE(pVal);
    }

    return ret;
}

bool CIHXValues2Test::HandleSetPropertyObjectCmd(const UTVector<UTString>& info)
{
    bool ret = false;
    unsigned int expected = 0;

    IHXValues2* pSrc = NULL;
    IHXValues2* pDest = NULL;

    if (info[4] == info[1])
    {
        DPRINTF (D_ERROR, ("CIHXValues2Test::HandleSetPropertyObjectCmd : pSrc and pDest can't be the same object\n"));
    }
    else if (!getValues(info[4], pSrc) ||
             !getValues(info[1], pDest) ||
             !UTParamUtil::GetUInt(info[2], expected))
    {
        DPRINTF (D_ERROR, ("CIHXValues2Test::HandleSetPropertyObjectCmd : failed to convert parameters\n"));
    }
    else
    {
        HX_RESULT res = pDest->SetPropertyObject(info[3], pSrc);
        
        if (res != (HX_RESULT)expected)
        {
            DPRINTF (D_ERROR, ("CIHXValues2Test::HandleSetPropertyObjectCmd : expected %08x got %08x\n", expected, res));
        }
        else
        {
            ret = true;
        }
    }

    return ret;
}

bool CIHXValues2Test::HandleGetPropertyObjectCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    unsigned int expected = 0;
    IHXValues2* pSrc = NULL; 
    IHXValues2* pDest = NULL;

    if (info[4] == info[1])
    {
        DPRINTF (D_ERROR, ("CIHXValues2Test::HandleGetPropertyObjectCmd : pSrc and pDest can't be the same object\n"));
    }
    else if (!getValues(info[4], pDest) ||
             !getValues(info[1], pSrc) ||
             !UTParamUtil::GetUInt(info[2], expected))
    {
        DPRINTF (D_ERROR, ("CIHXValues2Test::HandleGetPropertyObjectCmd : failed to convert parameters\n"));
    }
    else
    {
        IUnknown* pUnk = NULL;

        HX_RESULT res = pSrc->GetPropertyObject(info[3], pUnk);
        
        if (res != (HX_RESULT)expected)
        {
            DPRINTF (D_ERROR, ("CIHXValues2Test::HandleGetPropertyObjectCmd : expected %08x got %08x\n", expected, res));
        }
        else if (HXR_OK == res)
        {
            if (HXR_OK != pUnk->QueryInterface(IID_IHXValues2,
                                               (void**)&pDest))
            {
                DPRINTF (D_ERROR, ("CIHXValues2Test::HandleGetPropertyObjectCmd : IHXValues2 QI failed\n"));
            }
            else
            {
                ret = setValues(info[4], pDest);

                // Remove AddRef() caused by QI
                HX_RELEASE(pDest);
            }
        }
        else
        {
            // handle HXR_OK != res case
            ret = true;
        }

        HX_RELEASE(pUnk);
    }

    return ret;
}

bool CIHXValues2Test::HandleGetFirstPropertyObjectCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    unsigned int expected = 0;
    IHXValues2* pSrc = NULL; 
    IHXValues2* pDest = NULL;

    if (info[4] == info[1])
    {
        DPRINTF (D_ERROR, ("CIHXValues2Test::HandleGetFirstPropertyObjectCmd : pSrc and pDest can't be the same object\n"));
    }
    else if (!getValues(info[4], pDest) ||
             !getValues(info[1], pSrc) ||
             !UTParamUtil::GetUInt(info[2], expected))
    {
        DPRINTF (D_ERROR, ("CIHXValues2Test::HandleGetFirstPropertyObjectCmd : failed to convert parameters\n"));
    }
    else
    {
        IUnknown* pUnk = NULL;
        const char* pKey = NULL;
        HX_RESULT res = pSrc->GetFirstPropertyObject(pKey, pUnk);
        
        if (res != (HX_RESULT)expected)
        {
            DPRINTF (D_ERROR, ("CIHXValues2Test::HandleGetFirstPropertyObjectCmd : expected %08x got %08x\n", expected, res));
        }
        else if (HXR_OK == res)
        {
            if (info[3] != pKey)
            {
                DPRINTF (D_ERROR, ("CIHXValues2Test::HandleGetFirstPropertyObjectCmd : expected '%s' got '%s'\n", (const char*)info[3], pKey));
            }
            else if (HXR_OK != pUnk->QueryInterface(IID_IHXValues2,
                                               (void**)&pDest))
            {
                DPRINTF (D_ERROR, ("CIHXValues2Test::HandleGetFirstPropertyObjectCmd : IHXValues2 QI failed\n"));
            }
            else
            {
                ret = setValues(info[4], pDest);

                // Remove AddRef() caused by QI
                HX_RELEASE(pDest);
            }
        }
        else
        {
            // handle HXR_OK != res case
            ret = true;
        }

        HX_RELEASE(pUnk);
    }

    return ret;
}

bool CIHXValues2Test::HandleGetNextPropertyObjectCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    unsigned int expected = 0;
    IHXValues2* pSrc = NULL; 
    IHXValues2* pDest = NULL;

    if (info[4] == info[1])
    {
        DPRINTF (D_ERROR, ("CIHXValues2Test::HandleGetNextPropertyObjectCmd : pSrc and pDest can't be the same object\n"));
    }
    else if (!getValues(info[4], pDest) ||
             !getValues(info[1], pSrc) ||
             !UTParamUtil::GetUInt(info[2], expected))
    {
        DPRINTF (D_ERROR, ("CIHXValues2Test::HandleGetNextPropertyObjectCmd : failed to convert parameters\n"));
    }
    else
    {
        IUnknown* pUnk = NULL;
        const char* pKey = NULL;
        HX_RESULT res = pSrc->GetNextPropertyObject(pKey, pUnk);
        
        if (res != (HX_RESULT)expected)
        {
            DPRINTF (D_ERROR, ("CIHXValues2Test::HandleGetNextPropertyObjectCmd : expected %08x got %08x\n", expected, res));
        }
        else if (HXR_OK == res)
        {
            if (info[3] != pKey)
            {
                DPRINTF (D_ERROR, ("CIHXValues2Test::HandleGetNextPropertyObjectCmd : expected '%s' got '%s'\n", (const char*)info[3], pKey));
            }
            else if (HXR_OK != pUnk->QueryInterface(IID_IHXValues2,
                                               (void**)&pDest))
            {
                DPRINTF (D_ERROR, ("CIHXValues2Test::HandleGetNextPropertyObjectCmd : IHXValues2 QI failed\n"));
            }
            else
            {
                ret = setValues(info[4], pDest);

                // Remove AddRef() caused by QI
                HX_RELEASE(pDest);
            }
        }
        else
        {
            // handle HXR_OK != res case
            ret = true;
        }

        HX_RELEASE(pUnk);
    }

    return ret;
}

bool 
CIHXValues2Test::HandleRemoveCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    unsigned int expected = 0;
    IHXValues2* pValues = NULL;
        
    if (!getValues(info[1], pValues) ||
        !UTParamUtil::GetUInt(info[2], expected))
    {
        DPRINTF (D_ERROR, ("CIHXValues2Test::HandleRemoveCmd : failed to convert parameters\n"));
    }
    else
    {
        HX_RESULT res = pValues->Remove(info[3]);

        if (res != (HX_RESULT)expected)
        {
            DPRINTF (D_ERROR, ("CIHXValues2Test::HandleRemoveCmd : expected %08x got %08x\n", expected, res));
        }
        else
        {
            ret = true;
        }
    }
    
    return ret;
}

bool 
CIHXValues2Test::HandleRemoveULONG32Cmd(const UTVector<UTString>& info)
{
    bool ret = false;

    unsigned int expected = 0;
    IHXValues2* pValues = NULL;
        
    if (!getValues(info[1], pValues) ||
        !UTParamUtil::GetUInt(info[2], expected))
    {
        DPRINTF (D_ERROR, ("CIHXValues2Test::HandleRemoveULONG32Cmd : failed to convert parameters\n"));
    }
    else
    {
        HX_RESULT res = pValues->RemoveULONG32(info[3]);

        if (res != (HX_RESULT)expected)
        {
            DPRINTF (D_ERROR, ("CIHXValues2Test::HandleRemoveULONG32Cmd : expected %08x got %08x\n", expected, res));
        }
        else
        {
            ret = true;
        }
    }
    
    return ret;
}

bool 
CIHXValues2Test::HandleRemoveBufferCmd(const UTVector<UTString>& info)
{
    bool ret = false;
    unsigned int expected = 0;
    IHXValues2* pValues = NULL;
        
    if (!getValues(info[1], pValues) ||
        !UTParamUtil::GetUInt(info[2], expected))
    {
        DPRINTF (D_ERROR, ("CIHXValues2Test::HandleRemoveBufferCmd : failed to convert parameters\n"));
    }
    else
    {
        HX_RESULT res = pValues->RemoveBuffer(info[3]);

        if (res != (HX_RESULT)expected)
        {
            DPRINTF (D_ERROR, ("CIHXValues2Test::HandleRemoveBufferCmd : expected %08x got %08x\n", expected, res));
        }
        else
        {
            ret = true;
        }
    }
    
    return ret;
}

bool 
CIHXValues2Test::HandleRemoveCStringCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    unsigned int expected = 0;
    IHXValues2* pValues = NULL;
        
    if (!getValues(info[1], pValues) ||
        !UTParamUtil::GetUInt(info[2], expected))
    {
        DPRINTF (D_ERROR, ("CIHXValues2Test::HandleRemoveCStringCmd : failed to convert parameters\n"));
    }
    else
    {
        HX_RESULT res = pValues->RemoveCString(info[3]);

        if (res != (HX_RESULT)expected)
        {
            DPRINTF (D_ERROR, ("CIHXValues2Test::HandleRemoveCStringCmd : expected %08x got %08x\n", expected, res));
        }
        else
        {
            ret = true;
        }
    }
    
    return ret;
}

bool 
CIHXValues2Test::HandleRemoveObjectCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    unsigned int expected = 0;
    IHXValues2* pValues = NULL;
        
    if (!getValues(info[1], pValues) ||
        !UTParamUtil::GetUInt(info[2], expected))
    {
        DPRINTF (D_ERROR, ("CIHXValues2Test::HandleRemoveObjectCmd : failed to convert parameters\n"));
    }
    else
    {
        HX_RESULT res = pValues->RemoveObject(info[3]);

        if (res != (HX_RESULT)expected)
        {
            DPRINTF (D_ERROR, ("CIHXValues2Test::HandleRemoveObjectCmd : expected %08x got %08x\n", expected, res));
        }
        else
        {
            ret = true;
        }
    }

    return ret;
}

bool CIHXValues2Test::HandleCheckQueryInterfaceCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    IHXValues2* pSrc = NULL;

    if (!getValues(info[1], pSrc))
    {
        DPRINTF (D_ERROR, ("CIHXValues2Test::HandleCheckQueryInterfaceCmd : failed to convert parameters\n"));
    }
    else if (pSrc)
    {
        IHXValues* pValues = NULL;
        IHXValues2* pValues2a = NULL;
        IHXValues2* pValues2b = NULL;
        IUnknown* pUnknown1 = NULL;
        IUnknown* pUnknown2 = NULL;
        IUnknown* pUnknown3 = NULL;

        if (HXR_OK != pSrc->QueryInterface(IID_IHXValues,
                                           (void**)&pValues))
        {
            DPRINTF (D_ERROR, ("CIHXValues2Test::HandleCheckQueryInterfaceCmd : failed IHXValues QI\n"));
        }
        else if (HXR_OK != pSrc->QueryInterface(IID_IHXValues2,
                                                     (void**)&pValues2a))
        {
            DPRINTF (D_ERROR, ("CIHXValues2Test::HandleCheckQueryInterfaceCmd : failed IHXValues2 QI\n"));
        }
        else if (HXR_OK != pSrc->QueryInterface(IID_IUnknown,
                                                     (void**)&pUnknown1))
        {
            DPRINTF (D_ERROR, ("CIHXValues2Test::HandleCheckQueryInterfaceCmd : failed IUnknown QI\n"));
        }
        else if (HXR_OK != pValues->QueryInterface(IID_IUnknown,
                                                   (void**)&pUnknown2))
        {
            DPRINTF (D_ERROR, ("CIHXValues2Test::HandleCheckQueryInterfaceCmd : failed IUnknown QI from IHXValues\n"));
        }
        else if (HXR_OK != pValues2a->QueryInterface(IID_IUnknown,
                                                     (void**)&pUnknown3))
        {
            DPRINTF (D_ERROR, ("CIHXValues2Test::HandleCheckQueryInterfaceCmd : failed IUnknown QI from IHXValues2\n"));
        }
        else if (HXR_OK != pValues->QueryInterface(IID_IHXValues2,
                                                   (void**)&pValues2b))
        {
            DPRINTF (D_ERROR, ("CIHXValues2Test::HandleCheckQueryInterfaceCmd : failed IHXValues2 QI from IHXValues\n"));
        }
        else if (!pUnknown1)
        {
            DPRINTF (D_ERROR, ("CIHXValues2Test::HandleCheckQueryInterfaceCmd : pUnknown1 is NULL\n"));
        }
        else if (pUnknown1 != pUnknown2)
        {
            DPRINTF (D_ERROR, ("CIHXValues2Test::HandleCheckQueryInterfaceCmd : pUnknown1 != pUnknown2\n"));
        }
        else if (pUnknown2 != pUnknown3)
        {
            DPRINTF (D_ERROR, ("CIHXValues2Test::HandleCheckQueryInterfaceCmd : pUnknown2 != pUnknown3\n"));
        }
        else if (!pValues2a)
        {
            DPRINTF (D_ERROR, ("CIHXValues2Test::HandleCheckQueryInterfaceCmd : pValues2a is NULL\n"));
        }
        else if (pSrc != pValues2a)
        {
            DPRINTF (D_ERROR, ("CIHXValues2Test::HandleCheckQueryInterfaceCmd : pSrc != pValues2a\n"));
        }
        else if (pValues2a != pValues2b)
        {
            DPRINTF (D_ERROR, ("CIHXValues2Test::HandleCheckQueryInterfaceCmd : pValues2a != pValues2b\n"));
        }
        else
        {
            ret = true;
        }

        HX_RELEASE(pValues);
        HX_RELEASE(pValues2a);
        HX_RELEASE(pValues2b);
        HX_RELEASE(pUnknown1);
        HX_RELEASE(pUnknown2);
        HX_RELEASE(pUnknown3);
    }

    return ret;
}


bool 
CIHXValues2Test::HandleCreateIHXValues2Cmd(const UTVector<UTString>& info)
{
    bool ret = false;

    IHXValues2* pValues = new CHXHeader();

    if (pValues)
    {
        pValues->AddRef();
        ret = setValues(info[1], pValues);
        HX_RELEASE(pValues);
    }
    else
    {
        DPRINTF (D_ERROR, ("CIHXValues2Test::HandleCreateIHXValues2Cmd : failed to create an IHXValues2 object\n"));
    }

    return ret;
}

bool CIHXValues2Test::getValues(const char* pName, IHXValues2*& pValues)
{
    bool bRet = false;

    if (!strcmp(pName, "A"))
    {
        pValues = m_pA;
        bRet = true;
    }
    else if (!strcmp(pName, "B"))
    {
        pValues = m_pB;
        bRet = true;
    }

    return bRet;
}

bool CIHXValues2Test::setValues(const char* pName, IHXValues2* pValues)
{
    bool bRet = false;

    if (!strcmp(pName, "A"))
    {
        HX_RELEASE(m_pA);
        m_pA = pValues;

        if (m_pA)
        {
            m_pA->AddRef();
        }

        bRet = true;
    }
    else if (!strcmp(pName, "B"))
    {
        HX_RELEASE(m_pB);
        m_pB = pValues;

        if (m_pB)
        {
            m_pB->AddRef();
        }

        bRet = true;
    }

    return bRet;
}

IHXBuffer* 
CIHXValues2Test::stringToBuffer(const char* pStr) const
{
    IHXBuffer* pRet = NULL;

    if (pStr)
    {
        pRet = new CHXBuffer;
        if (pRet)
        {
            pRet->AddRef();

            if (HXR_OK != pRet->Set((UCHAR*)pStr, strlen(pStr)))
            {
                HX_RELEASE(pRet);
            }
        }
    }
    return pRet;
}
