/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: altgrpselect_test.cpp,v 1.3 2004/08/10 19:00:39 acolwell Exp $
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

#include "./altgrpselect_test.h"

#include "hx_ut_debug.h"
#include "ut_param_util.h"

#include "chxminiccf.h"
#include "chxlistctx.h"

#include "maxbwaltfilt.h"
#include "highbwaltfilt.h"
#include "langaltfilt.h"
#include "mimealtfilt.h"

CHXAltGroupSelectorTest::CHXAltGroupSelectorTest() :
    m_pContext(0),
    m_pCCF(0),
    m_pPluginHdlr(0),
    m_pParser(0),
    m_pA(0),
    m_pB(0),
    m_pList(0),
    m_pListItr(0)
{
    CHXListContext* pContext = new CHXListContext;

    if (pContext)
    {
        pContext->AddRef();

        HX_RESULT res = HXR_OUTOFMEMORY;

        IUnknown* pCCF = new CHXMiniCCF();

        if (pCCF)
        {
            pCCF->AddRef();

            res = pContext->AddIUnknown(pCCF);

            HX_RELEASE(pCCF);
        }

        if (HXR_OK == res)
        {
            m_pPluginHdlr = new CHXTestPluginHdlr;
            
            if (m_pPluginHdlr)
            {
                m_pPluginHdlr->AddRef();

                res = m_pPluginHdlr->Init(pContext);

                if (HXR_OK == res)
                {
                    res = pContext->AddIUnknown(m_pPluginHdlr);

                    if (HXR_OK != res)
                    {
                        m_pPluginHdlr->Close();
                    }
                }

                if (HXR_OK != res)
                {
                    HX_RELEASE(m_pPluginHdlr);
                }
            }
        }

        if (HXR_OK == res)
        {
            m_pContext = pContext;
            m_pContext->AddRef();
        }

        HX_RELEASE(pContext);
    }
    
    if (m_pContext)
    {
	m_pContext->QueryInterface(IID_IHXCommonClassFactory,
				   (void**)&m_pCCF);
        
        m_altTestHlpr.Init(m_pContext);
        m_selector.Init(m_pContext);
    }
}

CHXAltGroupSelectorTest::~CHXAltGroupSelectorTest()
{    
    delete m_pParser;
    m_pParser = 0;

    HX_RELEASE(m_pCCF);

    if (m_pPluginHdlr)
    {
        m_pPluginHdlr->Close();
        HX_RELEASE(m_pPluginHdlr);
    }
    HX_RELEASE(m_pContext);
    HX_RELEASE(m_pA);
    HX_RELEASE(m_pB);
    HX_RELEASE(m_pList);
    HX_RELEASE(m_pListItr);
}

const char* CHXAltGroupSelectorTest::DefaultCommandLine() const
{
    return "taltgrpselect taltgrpselect.in";
}
    
void CHXAltGroupSelectorTest::GetCommandInfo(UTVector<HLXUnitTestCmdInfo*>& cmds)
{
    int index = 0;

    cmds.Resize(index + 1);
    cmds[index++] = 
	new HLXUnitTestCmdInfoDisp<CHXAltGroupSelectorTest>(this, 
							   "Init",
							   &CHXAltGroupSelectorTest::HandleInitCmd,
							   2);

    cmds.Resize(index + 1);
    cmds[index++] = 
	new HLXUnitTestCmdInfoDisp<CHXAltGroupSelectorTest>(this, 
							   "Parse",
							   &CHXAltGroupSelectorTest::HandleParseCmd,
							   4);

    cmds.Resize(index + 1);
    cmds[index++] = 
	new HLXUnitTestCmdInfoDisp<CHXAltGroupSelectorTest>(this, 
							   "Reset",
							   &CHXAltGroupSelectorTest::HandleResetCmd,
							   1);

    cmds.Resize(index + 1);
    cmds[index++] = 
	new HLXUnitTestCmdInfoDisp<CHXAltGroupSelectorTest>(this, 
							   "Select",
							   &CHXAltGroupSelectorTest::HandleSelectCmd,
							   1);

    cmds.Resize(index + 1);
    cmds[index++] = 
	new HLXUnitTestCmdInfoDisp<CHXAltGroupSelectorTest>(this, 
							   "AddMaxBWFilter",
							   &CHXAltGroupSelectorTest::HandleAddMaxBWFilterCmd,
							   2);

    cmds.Resize(index + 1);
    cmds[index++] = 
	new HLXUnitTestCmdInfoDisp<CHXAltGroupSelectorTest>(this, 
							   "AddLanguageFilter",
							   &CHXAltGroupSelectorTest::HandleAddLanguageFilterCmd,
							   2);
    cmds.Resize(index + 1);
    cmds[index++] = 
	new HLXUnitTestCmdInfoDisp<CHXAltGroupSelectorTest>(this, 
							   "AddHighestBWFilter",
							   &CHXAltGroupSelectorTest::HandleAddHighestBWFilterCmd,
							   1);
    cmds.Resize(index + 1);
    cmds[index++] = 
	new HLXUnitTestCmdInfoDisp<CHXAltGroupSelectorTest>(this, 
							   "AddMimetypeFilter",
							   &CHXAltGroupSelectorTest::HandleAddMimetypeFilterCmd,
							   1);

    cmds.Resize(index + 1);
    cmds[index++] = 
	new HLXUnitTestCmdInfoDisp<CHXAltGroupSelectorTest>(this, 
							   "SetMimetypes",
							   &CHXAltGroupSelectorTest::HandleSetMimetypesCmd,
							   2);
    
    cmds.Resize(index + 1);
    cmds[index++] = 
	new HLXUnitTestCmdInfoDisp<CHXAltGroupSelectorTest>(this, 
							   "IntCount",
							   &CHXAltGroupSelectorTest::HandleIntCountCmd,
							   3);
    cmds.Resize(index + 1);
    cmds[index++] = 
	new HLXUnitTestCmdInfoDisp<CHXAltGroupSelectorTest>(this, 
							   "StringCount",
							   &CHXAltGroupSelectorTest::HandleStringCountCmd,
							   3);
    cmds.Resize(index + 1);
    cmds[index++] = 
	new HLXUnitTestCmdInfoDisp<CHXAltGroupSelectorTest>(this, 
							   "BufferCount",
							   &CHXAltGroupSelectorTest::HandleBufferCountCmd,
							   3);
    cmds.Resize(index + 1);
    cmds[index++] = 
	new HLXUnitTestCmdInfoDisp<CHXAltGroupSelectorTest>(this, 
							   "ObjectCount",
							   &CHXAltGroupSelectorTest::HandleObjectCountCmd,
							   3);

    cmds.Resize(index + 1);
    cmds[index++] = 
	new HLXUnitTestCmdInfoDisp<CHXAltGroupSelectorTest>(this, 
							   "GetInt",
							   &CHXAltGroupSelectorTest::HandleGetIntCmd,
							   4);
    cmds.Resize(index + 1);
    cmds[index++] = 
	new HLXUnitTestCmdInfoDisp<CHXAltGroupSelectorTest>(this, 
							   "GetString",
							   &CHXAltGroupSelectorTest::HandleGetStringCmd,
							   4);

    cmds.Resize(index + 1);
    cmds[index++] = 
	new HLXUnitTestCmdInfoDisp<CHXAltGroupSelectorTest>(this, 
							   "GetBuffer",
							   &CHXAltGroupSelectorTest::HandleGetBufferCmd,
							   4);
    cmds.Resize(index + 1);
    cmds[index++] = 
	new HLXUnitTestCmdInfoDisp<CHXAltGroupSelectorTest>(this, 
							   "GetIHXValues",
							   &CHXAltGroupSelectorTest::HandleGetIHXValuesCmd,
							   4);
    cmds.Resize(index + 1);
    cmds[index++] = 
	new HLXUnitTestCmdInfoDisp<CHXAltGroupSelectorTest>(this, 
							   "GetIHXList",
							   &CHXAltGroupSelectorTest::HandleGetIHXListCmd,
							   3);

    cmds.Resize(index + 1);
    cmds[index++] = 
	new HLXUnitTestCmdInfoDisp<CHXAltGroupSelectorTest>(this, 
							   "ListGetCount",
							   &CHXAltGroupSelectorTest::HandleListGetCountCmd,
							   2);
    cmds.Resize(index + 1);
    cmds[index++] = 
	new HLXUnitTestCmdInfoDisp<CHXAltGroupSelectorTest>(this, 
							   "ListItrBegin",
							   &CHXAltGroupSelectorTest::HandleListItrBeginCmd,
							   1);
    cmds.Resize(index + 1);
    cmds[index++] = 
	new HLXUnitTestCmdInfoDisp<CHXAltGroupSelectorTest>(this, 
							   "ListItrGetBuffer",
							   &CHXAltGroupSelectorTest::HandleListItrGetBufferCmd,
							   2);
    cmds.Resize(index + 1);
    cmds[index++] = 
	new HLXUnitTestCmdInfoDisp<CHXAltGroupSelectorTest>(this, 
							   "ListItrNext",
							   &CHXAltGroupSelectorTest::HandleListItrNextCmd,
							   2);

    // Add alt test helper commands
    m_altTestHlpr.GetCommandInfo(cmds);
}

HLXCmdBasedTest* CHXAltGroupSelectorTest::Clone() const
{
    return new CHXAltGroupSelectorTest();
}

bool CHXAltGroupSelectorTest::HandleInitCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    ULONG32 ulVersion = 0;

    delete m_pParser;
    m_pParser = 0;

    if (!UTParamUtil::GetULong(info[1], ulVersion))
    {
	DPRINTF(D_ERROR, ("CHXAltGroupSelectorTest::HandleInitCmd : failed to convert parameter\n"));	
    }
    else
    {
	m_pParser = new SDPMediaDescParser(ulVersion);

	HX_RESULT res = m_pParser->Init(m_pContext);

	if (HXR_OK != res)
	{
	    DPRINTF(D_ERROR, ("CHXAltGroupSelectorTest::HandleInitCmd : failed to init parser %08x\n", res));
	}
	else
	{
	    ret = true;
	}
    }

    return ret;
}

bool CHXAltGroupSelectorTest::HandleParseCmd(const UTVector<UTString>& info)
{
    bool ret = false;
    IHXBuffer* pBuf = 0;
    HX_RESULT expectedRes;
    unsigned int expectedNum;

    //DPRINTF (D_ERROR, ("--------\n"));

    if (!GetHXResult(info[1], expectedRes) ||
	!UTParamUtil::GetUInt(info[2], expectedNum))
    {
	DPRINTF(D_ERROR, ("CHXAltGroupSelectorTest::HandleParseCmd : failed to convert parameter\n"));	
    }
    else if (m_pCCF &&
	     (HXR_OK == m_pCCF->CreateInstance(CLSID_IHXBuffer,
					       (void**)&pBuf)) &&
	     (HXR_OK == pBuf->Set((const unsigned char*)(const char*)info[3], 
				  info[3].length())))
    {
        IHXValues** ppValues = NULL;
        UINT16 nValues = 0;
	HX_RESULT res = m_pParser->Parse(pBuf, nValues, ppValues);

	if (res != expectedRes)
	{
	    DPRINTF(D_ERROR, ("CHXAltGroupSelectorTest::HandleParseCmd : Parse result expected %08x, got %08x\n",
			      expectedRes, res));
	}
	else if (nValues != expectedNum)
	{
	    DPRINTF(D_ERROR, ("CHXAltGroupSelectorTest::HandleParseCmd : nValues expected %u, got %u\n",
			      expectedNum, nValues));
	}
	else if (HXR_OK != m_altTestHlpr.SetHeaders(nValues, ppValues))
        {
            DPRINTF(D_ERROR, ("CHXAltGroupSelectorTest::HandleParseCmd : failed to copy headers\n"));
        }
        else
	{
	    ret = true;
	}

        for (UINT16 i = 0; i < nValues; i++)
        {
            HX_RELEASE(ppValues[i]);
        }
        delete [] ppValues;
        ppValues = NULL;
    }

    HX_RELEASE(pBuf);

    return ret;
}

bool 
CHXAltGroupSelectorTest::HandleResetCmd(const UTVector<UTString>& info)
{
    m_selector.Reset();
    return true;
}

bool 
CHXAltGroupSelectorTest::HandleSelectCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    UINT32 uHdrCount = m_altTestHlpr.HeaderCount();
    IHXValues** pInHeaders = m_altTestHlpr.Headers();
    IHXValues** pOutHeaders = new IHXValues*[uHdrCount];

    if (pOutHeaders)
    {
        memset(pOutHeaders, 0, sizeof(IHXValues*) * uHdrCount);

        HX_RESULT res = m_selector.Select(uHdrCount, pInHeaders, pOutHeaders);
        
        if (HXR_OK == res)
        {
            res = m_altTestHlpr.SetHeaders(uHdrCount, pOutHeaders);

            if (HXR_OK == res)
            {
                ret = true;
            }
            else
            {
                DPRINTF(D_ERROR, ("CHXAltGroupSelectorTest::HandleSelectCmd : SetHeaders return an error %08x\n", res));
            }
        }
        else
        {
            DPRINTF(D_ERROR, ("CHXAltGroupSelectorTest::HandleSelectCmd : Select return an error %08x\n", res));
        }

        for (UINT32 i = 0; i < uHdrCount; i++)
        {
            HX_RELEASE(pOutHeaders[i]);
        }
        delete [] pOutHeaders;
        pOutHeaders = NULL;
    }

    return ret;
}

bool 
CHXAltGroupSelectorTest::HandleAddMaxBWFilterCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    unsigned int uBW;

    if (!UTParamUtil::GetUInt(info[1], uBW))
    {
	DPRINTF(D_ERROR, ("CHXAltGroupSelectorTest::HandleAddMaxBWFilterCmd : failed to convert parameter\n"));	
    }
    else
    {
        CHXMaxBWAltIDFilter* pFilt = new CHXMaxBWAltIDFilter;

        if (pFilt)
        {
            HX_RESULT res = pFilt->Init(uBW);
            
            
            if (HXR_OK == res)
            {
                res = m_selector.AddFilter(pFilt);
                
                if (HXR_OK == res)
                {
                    ret = true;
                }
                else
                {
                    DPRINTF(D_ERROR, ("CHXAltGroupSelectorTest::HandleAddMaxBWFilterCmd : Unexpected error from AddFilter() %08x\n", res));
                    
                    HX_DELETE(pFilt);
                }
            }
            else
            {
                DPRINTF(D_ERROR, ("CHXAltGroupSelectorTest::HandleAddMaxBWFilterCmd : Unexpected error from Init() %08x\n", res));
            }
        }
        else
        {
            DPRINTF(D_ERROR, ("CHXAltGroupSelectorTest::HandleAddMaxBWFilterCmd : Failed to create filter\n"));
        }
    }
    return ret;
}

bool 
CHXAltGroupSelectorTest::HandleAddLanguageFilterCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    CHXLanguageAltIDFilter* pFilt = new CHXLanguageAltIDFilter;

    if (pFilt)
    {
        HX_RESULT res = pFilt->Init(info[1]);
        
        if (HXR_OK == res)
        {
            res = m_selector.AddFilter(pFilt);
            
            if (HXR_OK == res)
            {
                ret = true;
            }
            else
            {
                DPRINTF(D_ERROR, ("CHXAltGroupSelectorTest::HandleAddLanguageFilterCmd : Unexpected error from AddFilter() %08x\n", res));

                HX_DELETE(pFilt);
            }
        }
        else
        {
            DPRINTF(D_ERROR, ("CHXAltGroupSelectorTest::HandleAddLanguageFilterCmd : Unexpected error from Init() %08x\n", res));
        }
    }
    else
    {
        DPRINTF(D_ERROR, ("CHXAltGroupSelectorTest::HandleAddLanguageFilterCmd : Failed to create filter\n"));
    }

    return ret;
}

bool 
CHXAltGroupSelectorTest::HandleAddHighestBWFilterCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    CHXHighestBWAltIDFilter* pFilt = new CHXHighestBWAltIDFilter;

    if (pFilt)
    {
        HX_RESULT res = m_selector.AddFilter(pFilt);
            
        if (HXR_OK == res)
        {
            ret = true;
        }
        else
        {
            DPRINTF(D_ERROR, ("CHXAltGroupSelectorTest::HandleAddHighestBWFilterCmd : Unexpected error from AddFilter() %08x\n", res));

            HX_DELETE(pFilt);
        }
    }
    else
    {
        DPRINTF(D_ERROR, ("CHXAltGroupSelectorTest::HandleAddHighestBWFilterCmd : Failed to create filter\n"));
    }

    return ret;
}

bool 
CHXAltGroupSelectorTest::HandleAddMimetypeFilterCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    CHXMimetypeAltIDFilter* pFilt = new CHXMimetypeAltIDFilter;

    if (pFilt)
    {
        HX_RESULT res = pFilt->Init(m_pContext);

        if (HXR_OK == res)
        {
            res = m_selector.AddFilter(pFilt);
            
            if (HXR_OK == res)
            {
                ret = true;
            }
            else
            {
                DPRINTF(D_ERROR, ("CHXAltGroupSelectorTest::HandleAddMimetypeFilterCmd : Unexpected error from AddFilter() %08x\n", res));
            }
        }
        else
        {
            DPRINTF(D_ERROR, ("CHXAltGroupSelectorTest::HandleAddMimetypeFilterCmd : Failed to initialize filter. %08x\n", res));
        }

        if (!ret)
        {
            HX_DELETE(pFilt);
        }
    }
    else
    {
        DPRINTF(D_ERROR, ("CHXAltGroupSelectorTest::HandleAddMimetypeFilterCmd : Failed to create filter\n"));
    }

    return ret;
}

bool 
CHXAltGroupSelectorTest::HandleSetMimetypesCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    if (m_pPluginHdlr)
    {
        HX_RESULT res = m_pPluginHdlr->SetMimetypes(info[1]);

        if (HXR_OK == res)
        {
            ret = true;
        }
        else
        {
            DPRINTF(D_ERROR, ("CHXAltGroupSelectorTest::HandleSetMimetypesCmd : Failed to set mimetypes %08x\n", res));
        }
    }
    else
    {
        DPRINTF(D_ERROR, ("CHXAltGroupSelectorTest::HandleSetMimetypesCmd : m_pPluginHdlr is not set\n"));
    }

    return ret;
}

bool CHXAltGroupSelectorTest::GetHXResult(const char* pStr, 
					 HX_RESULT& res) const
{
    bool ret = false;

    res = 0;

    if ((pStr[0] == '0') &&
	(pStr[1] == 'x'))
    {
	pStr += 2;

	if (*pStr)
	{
	    int i = 0;

	    ret = true;

	    for(; ret && *pStr; *pStr++, i++)
	    {
		res <<= 4;
		if ((*pStr >= '0') && (*pStr <= '9'))
		{
		    res += *pStr - '0';
		}
		else if ((*pStr >= 'a') && (*pStr <= 'f'))
		{
		    res += (*pStr - 'a') + 10;
		}
		else if ((*pStr >= 'A') && (*pStr <= 'F'))
		{
		    res += (*pStr - 'A') + 10;
		}
		else
		{
		    DPRINTF(D_ERROR, ("CHXAltGroupSelectorTest::GetHXResult : invalid character '%c'\n", *pStr));

		    ret = false;
		}
	    }

	    if (ret && (i != 8))
	    {
		DPRINTF(D_ERROR, ("CHXAltGroupSelectorTest::GetHXResult : wrong number of hex digits\n"));
		ret = false;
	    }
	}
    }
    else
    {
	DPRINTF(D_ERROR, ("CHXAltGroupSelectorTest::GetHXResult : invalid hex string\n"));
    }

    return ret;
}

bool CHXAltGroupSelectorTest::HandleIntCountCmd(const UTVector<UTString>& info)
{
    bool ret = false;
    unsigned int valueIndex = 0;
    unsigned int expectedCount = 0;

    
    //DPRINTF (D_ERROR, ("========\n"));
    IHXValues* pHdr = NULL;

    if (!UTParamUtil::GetUInt(info[2], expectedCount))
    {
	DPRINTF(D_ERROR, ("CHXAltGroupSelectorTest::HandleIntCountCmd : failed to convert parameter\n"));	
    }
    else if (HXR_OK != getIHXValues(info[1], pHdr))
    {
	DPRINTF(D_ERROR, ("CHXAltGroupSelectorTest::HandleIntCountCmd : invalid value index\n"));
    }
    else
    {
	unsigned int ulCount = 0;

	const char* pName = 0;
	ULONG32 ulValue = 0;
	HX_RESULT res = pHdr->GetFirstPropertyULONG32(pName, ulValue);

	while (HXR_OK == res)
	{
	    //    DPRINTF (D_ERROR, ("Int %s -> %lu\n", pName, ulValue));
	    ulCount++;
	    res = pHdr->GetNextPropertyULONG32(pName, ulValue);
	}

	if (ulCount != expectedCount)
	{
	    DPRINTF(D_ERROR, ("CHXAltGroupSelectorTest::HandleIntCountCmd : expected %lu, got %lu\n",
			      expectedCount, ulCount));
	}
	else
	{
	    ret = true;
	}
    }

    return ret;
}

bool CHXAltGroupSelectorTest::HandleStringCountCmd(const UTVector<UTString>& info)
{
    bool ret = false;
    unsigned int valueIndex = 0;
    unsigned int expectedCount = 0;

    
    //DPRINTF (D_ERROR, ("=*=*=*=*\n"));
    IHXValues* pHdr = NULL;

    if (!UTParamUtil::GetUInt(info[2], expectedCount))
    {
	DPRINTF(D_ERROR, ("CHXAltGroupSelectorTest::HandleStringCountCmd : failed to convert parameter\n"));	
    }
    else if (HXR_OK != getIHXValues(info[1], pHdr))
    {
	DPRINTF(D_ERROR, ("CHXAltGroupSelectorTest::HandleStringCountCmd : invalid value index\n"));
    }
    else
    {
	unsigned int ulCount = 0;

	const char* pName = 0;
	IHXBuffer* pValue = 0;
	HX_RESULT res = pHdr->GetFirstPropertyCString(pName, pValue);

	while (HXR_OK == res)
	{
	    //DPRINTF (D_ERROR, ("String %s -> '%s'\n", 
	    //		       pName, 
	    //		       pValue->GetBuffer()));
	    ulCount++;

	    HX_RELEASE(pValue);

	    res = pHdr->GetNextPropertyCString(pName, pValue);
	}

	HX_RELEASE(pValue);

	if (ulCount != expectedCount)
	{
	    DPRINTF(D_ERROR, ("CHXAltGroupSelectorTest::HandleStringCountCmd : expected %lu, got %lu\n",
			      expectedCount, ulCount));
	}
	else
	{
	    ret = true;
	}
    }

    return ret;
}

bool CHXAltGroupSelectorTest::HandleBufferCountCmd(const UTVector<UTString>& info)
{    
    bool ret = false;

    unsigned int valueIndex = 0;
    unsigned int expectedCount = 0;
    
    //DPRINTF (D_ERROR, ("-=#=-=#=-\n"));
    IHXValues* pHdr = NULL;

    if (!UTParamUtil::GetUInt(info[2], expectedCount))
    {
	DPRINTF(D_ERROR, ("CHXAltGroupSelectorTest::HandleBufferCountCmd : failed to convert parameter\n"));	
    }
    else if (HXR_OK != getIHXValues(info[1], pHdr))
    {
	DPRINTF(D_ERROR, ("CHXAltGroupSelectorTest::HandleBufferCountCmd : invalid value index\n"));
    }
    else
    {
	unsigned int ulCount = 0;

	const char* pName = 0;
	IHXBuffer* pValue = 0;
	HX_RESULT res = pHdr->GetFirstPropertyBuffer(pName, pValue);

	while (HXR_OK == res)
	{
	    //PrintBuffer(pName, pValue);

	    ulCount++;

	    HX_RELEASE(pValue);

	    res = pHdr->GetNextPropertyBuffer(pName, pValue);
	}

	HX_RELEASE(pValue);

	if (ulCount != expectedCount)
	{
	    DPRINTF(D_ERROR, ("CHXAltGroupSelectorTest::HandleBufferCountCmd : expected %lu, got %lu\n",
			      expectedCount, ulCount));
	}
	else
	{
	    ret = true;
	}
    }

    return ret;
}

bool CHXAltGroupSelectorTest::HandleObjectCountCmd(const UTVector<UTString>& info)
{    
    bool ret = false;

    unsigned int valueIndex = 0;
    unsigned int expectedCount = 0;

    IHXValues* pHdr = NULL;

    if (!UTParamUtil::GetUInt(info[2], expectedCount))
    {
	DPRINTF(D_ERROR, ("CHXAltGroupSelectorTest::HandleObjectCountCmd : failed to convert parameter\n"));	
    }
    else if (HXR_OK != getIHXValues(info[1], pHdr))
    {
	DPRINTF(D_ERROR, ("CHXAltGroupSelectorTest::HandleObjectCountCmd : invalid value index\n"));
    }
    else
    {
        IHXValues2* pHdr2 = NULL;

        pHdr->QueryInterface(IID_IHXValues2, (void**)&pHdr2);

        if (pHdr)
        {
            unsigned int ulCount = 0;

            const char* pName = 0;
            IUnknown* pValue = 0;
            HX_RESULT res = pHdr2->GetFirstPropertyObject(pName, pValue);

            while (HXR_OK == res)
            {
                ulCount++;

                HX_RELEASE(pValue);

                res = pHdr2->GetNextPropertyObject(pName, pValue);
            }

            HX_RELEASE(pValue);

            if (ulCount != expectedCount)
            {
                DPRINTF(D_ERROR, ("CHXAltGroupSelectorTest::HandleObjectCountCmd : expected %lu, got %lu\n",
                                  expectedCount, ulCount));
            }
            else
            {
                ret = true;
            }
        }

        HX_RELEASE(pHdr2);
    }

    return ret;
}

bool CHXAltGroupSelectorTest::HandleGetIntCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    unsigned int valueIndex = 0;
    unsigned int expectedValue = 0;
    IHXValues* pHdr = NULL;

    if (!UTParamUtil::GetUInt(info[3], expectedValue))
    {
	DPRINTF(D_ERROR, ("CHXAltGroupSelectorTest::HandleGetIntCmd : failed to convert parameter\n"));	
    }
    else if (HXR_OK != getIHXValues(info[1], pHdr))
    {
	DPRINTF(D_ERROR, ("CHXAltGroupSelectorTest::HandleGetIntCmd : invalid value index\n"));
    }
    else
    {
	ULONG32 ulValue = 0;
	HX_RESULT res = pHdr->GetPropertyULONG32(info[2], ulValue);

	if (HXR_OK != res)
	{
	    DPRINTF(D_ERROR, ("CHXAltGroupSelectorTest::HandleGetIntCmd : get failed\n"));
	}
	else if (ulValue != expectedValue)
	{
	    DPRINTF(D_ERROR, ("CHXAltGroupSelectorTest::HandleGetIntCmd : expected %lu, got %lu\n",
			      expectedValue, ulValue));
	}
	else
	{
	    ret = true;
	}
    }

    return ret;
}

bool CHXAltGroupSelectorTest::HandleGetStringCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    unsigned int valueIndex = 0;
    IHXValues* pHdr = NULL;


    if (HXR_OK != getIHXValues(info[1], pHdr))
    {
	DPRINTF(D_ERROR, ("CHXAltGroupSelectorTest::HandleGetStringCmd : invalid value index\n"));
    }
    else
    {
	IHXBuffer* pValue = 0;
	HX_RESULT res = pHdr->GetPropertyCString(info[2], pValue);

	if (HXR_OK != res)
	{
	    DPRINTF(D_ERROR, ("CHXAltGroupSelectorTest::HandleGetStringCmd : get failed\n"));
	}
	else if (strcmp((const char*)pValue->GetBuffer(), info[3]))
	{
	    DPRINTF(D_ERROR, ("CHXAltGroupSelectorTest::HandleGetStringCmd : expected '%s', got '%s'\n",
			      (const char*)info[3], 
			      (const char*)pValue->GetBuffer()));
	}
	else
	{
	    ret = true;
	}

	HX_RELEASE(pValue);
    }

    return ret;
}

bool CHXAltGroupSelectorTest::HandleGetBufferCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    unsigned int valueIndex = 0;
    IHXBuffer* pExpected = 0;
    IHXValues* pHdr = NULL;

    if (HXR_OK != getIHXValues(info[1], pHdr))
    {
	DPRINTF(D_ERROR, ("CHXAltGroupSelectorTest::HandleGetStringCmd : invalid value index\n"));
    }
    else if (!GetBuffer(info[3], pExpected))
    {
	DPRINTF(D_ERROR, ("CHXAltGroupSelectorTest::HandleGetBufferCmd : failed to convert parameter\n"));	
    }
    else
    {
	IHXBuffer* pValue = 0;
	HX_RESULT res = pHdr->GetPropertyBuffer(info[2], pValue);

	if (HXR_OK != res)
	{
	    DPRINTF(D_ERROR, ("CHXAltGroupSelectorTest::HandleGetBufferCmd : get failed\n"));
	}
	else if (pValue->GetSize() != pExpected->GetSize())
	{
	    DPRINTF(D_ERROR, ("CHXAltGroupSelectorTest::HandleGetBufferCmd : size expected %lu, got %lu\n",
			      pExpected->GetSize(),
			      pValue->GetSize()));
	}
	else if (memcmp(pValue->GetBuffer(), pExpected->GetBuffer(), 
			pValue->GetSize()))
	{
	    DPRINTF(D_ERROR, ("CHXAltGroupSelectorTest::HandleGetBufferCmd : buffers don't match\n"));
	}
	else
	{
	    ret = true;
	}

	HX_RELEASE(pValue);
    }

    HX_RELEASE(pExpected);

    return ret;
}

bool CHXAltGroupSelectorTest::HandleGetIHXValuesCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    IHXValues* pHdr = NULL;

    if (HXR_OK != getIHXValues(info[1], pHdr))
    {
	DPRINTF(D_ERROR, ("CHXAltGroupSelectorTest::HandleGetIHXValuesCmd : invalid value index\n"));
    }
    else if ((info[3] != "A") && (info[3] != "B"))
    {
	DPRINTF(D_ERROR, ("CHXAltGroupSelectorTest::HandleGetIHXValuesCmd : failed to convert parameter\n"));	
    }
    else
    {
        IHXValues2* pHdr2 = NULL;
        
	HX_RESULT res = pHdr->QueryInterface(IID_IHXValues2, (void**)&pHdr2);

        if(HXR_OK != res)
        {
            DPRINTF(D_ERROR, ("CHXAltGroupSelectorTest::HandleGetIHXValuesCmd : QI for IHXValues2 failed\n"));
        }
        else
        {
            IUnknown* pUnk = NULL;

            IHXValues* pValue = NULL;

            res = pHdr2->GetPropertyObject(info[2], pUnk);
            
            if (HXR_OK != res)
            {
                DPRINTF(D_ERROR, ("CHXAltGroupSelectorTest::HandleGetIHXValuesCmd : get failed\n"));
            }
            else if (HXR_OK != pUnk->QueryInterface(IID_IHXValues, 
                                                    (void**)&pValue))
            {
                DPRINTF(D_ERROR, ("CHXAltGroupSelectorTest::HandleGetIHXValuesCmd : QI for IHXValues failed\n"));
            }
            else
            {
                if (info[3] == "A")
                {
                    HX_RELEASE(m_pA);
                    m_pA = pValue;
                    if (m_pA)
                    {
                        m_pA->AddRef();
                    }
                }
                else if (info[3] == "B")
                {
                    HX_RELEASE(m_pB);
                    m_pB = pValue;
                    if (m_pB)
                    {
                        m_pB->AddRef();
                    }
                }
                
                ret = true;
            }

            HX_RELEASE(pValue);
            HX_RELEASE(pUnk);
	}

        HX_RELEASE(pHdr2);
    }

    return ret;
}

bool 
CHXAltGroupSelectorTest::HandleGetIHXListCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    IHXValues* pHdr = NULL;

    if (HXR_OK != getIHXValues(info[1], pHdr))
    {
	DPRINTF(D_ERROR, ("CHXAltGroupSelectorTest::HandleGetIHXListCmd : invalid value index\n"));
    }
    else
    {
        IHXValues2* pHdr2 = NULL;
        
	HX_RESULT res = pHdr->QueryInterface(IID_IHXValues2, (void**)&pHdr2);

        if(HXR_OK != res)
        {
            DPRINTF(D_ERROR, ("CHXAltGroupSelectorTest::HandleGetIHXListCmd : QI for IHXValues2 failed\n"));
        }
        else
        {
            IUnknown* pUnk = NULL;

            IHXList* pList = NULL;

            res = pHdr2->GetPropertyObject(info[2], pUnk);
            
            if (HXR_OK != res)
            {
                DPRINTF(D_ERROR, ("CHXAltGroupSelectorTest::HandleGetIHXListCmd : get failed\n"));
            }
            else if (HXR_OK != pUnk->QueryInterface(IID_IHXList, 
                                                    (void**)&pList))
            {
                DPRINTF(D_ERROR, ("CHXAltGroupSelectorTest::HandleGetIHXListCmd : QI for IHXList failed\n"));
            }
            else
            {
                HX_RELEASE(m_pList);
                m_pList = pList;
                if (m_pList)
                {
                    m_pList->AddRef();
                }

                ret = true;
            }

            HX_RELEASE(pList);
            HX_RELEASE(pUnk);
	}

        HX_RELEASE(pHdr2);
    }

    return ret;
}

bool 
CHXAltGroupSelectorTest::HandleListGetCountCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    unsigned int expectedCount;

    if (!UTParamUtil::GetUInt(info[1], expectedCount))
    {
	DPRINTF(D_ERROR, ("CHXAltGroupSelectorTest::HandleListGetCountCmd : failed to convert parameter\n"));	
    }
    else if (!m_pList)
    {
        DPRINTF(D_ERROR, ("CHXAltGroupSelectorTest::HandleListGetCountCmd : m_pList not set\n"));
    }
    else if (expectedCount != m_pList->GetCount())
    {
        DPRINTF(D_ERROR, ("CHXAltGroupSelectorTest::HandleListGetCountCmd : got %u expected %u\n",
                          m_pList->GetCount(), expectedCount));
    }
    else
    {
        ret = true;
    }

    return ret;
}

bool 
CHXAltGroupSelectorTest::HandleListItrBeginCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    if (!m_pList)
    {
        DPRINTF(D_ERROR, ("CHXAltGroupSelectorTest::HandleListItrBeginCmd : m_pList is not set\n"));
    }
    else
    {
        HX_RELEASE(m_pListItr);

        m_pListItr = m_pList->Begin();

        if (m_pListItr)
        {
            ret = true;
        }
        else
        {
            DPRINTF(D_ERROR, ("CHXAltGroupSelectorTest::HandleListItrBeginCmd : failed to create iterator\n"));
        }
    }

    return ret;
}

bool 
CHXAltGroupSelectorTest::HandleListItrGetBufferCmd(const UTVector<UTString>& info)
{
    bool ret = false;
    IHXBuffer* pExpected = 0;

    if (!m_pListItr)
    {
        DPRINTF(D_ERROR, ("CHXAltGroupSelectorTest::HandleListItrGetBufferCmd : m_pListItr is not set\n"));
    }
    else if (!GetBuffer(info[1], pExpected))
    {
	DPRINTF(D_ERROR, ("CHXAltGroupSelectorTest::HandleListItrGetBufferCmd : failed to convert parameter\n"));	
    }
    else
    {
        IUnknown* pUnk = m_pListItr->GetItem();

        if (pUnk)
        {
            IHXBuffer* pValue = NULL;

            if (HXR_OK == pUnk->QueryInterface(IID_IHXBuffer, (void**)&pValue))
            {
                if (pValue->GetSize() != pExpected->GetSize())
                {
                    DPRINTF(D_ERROR, ("CHXAltGroupSelectorTest::HandleListItrGetBufferCmd : size expected %lu, got %lu\n",
                                      pExpected->GetSize(),
                                      pValue->GetSize()));
                }
                else if (memcmp(pValue->GetBuffer(), pExpected->GetBuffer(), 
                                pValue->GetSize()))
                {
                    DPRINTF(D_ERROR, ("CHXAltGroupSelectorTest::HandleListItrGetBufferCmd : buffers don't match\n"));
                    
                }
                else
                {
                    ret = true;
                }
            }
            else
            {
                DPRINTF(D_ERROR, ("CHXAltGroupSelectorTest::HandleListItrGetBufferCmd : IHXBuffer QI failed\n"));
            }

            HX_RELEASE(pValue);
        }
        else
        {
            DPRINTF(D_ERROR, ("CHXAltGroupSelectorTest::HandleListItrGetBufferCmd : failed to get item\n"));
        }
        HX_RELEASE(pUnk);
    }

    HX_RELEASE(pExpected);

    return ret;
}

bool 
CHXAltGroupSelectorTest::HandleListItrNextCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    bool expected;

    if (!UTParamUtil::GetBool(info[1], expected))
    {
	DPRINTF(D_ERROR, ("CHXAltGroupSelectorTest::HandleListItrNextCmd : failed to convert parameter\n"));	
    }
    else if (!m_pListItr)
    {
        DPRINTF(D_ERROR, ("CHXAltGroupSelectorTest::HandleListItrNextCmd : m_pList not set\n"));
    }
    else
    {
        bool bVal = (m_pListItr->MoveNext()) ? true : false;

        if (bVal != expected)
        {
            DPRINTF(D_ERROR, ("CHXAltGroupSelectorTest::HandleListItrNextCmd : got %u expected %u\n",
                              bVal, expected));
        }
        else
        {
            ret = true;
        }
    }

    return ret;
}

static int Char2Num(char ch)
{
    int ret = -1;

    if ((ch >= '0') && (ch <= '9'))
	ret = ch - '0';
    else if ((ch >= 'a') && (ch <= 'f'))
	ret = ch - 'a' + 10;
    else if ((ch >= 'A') && (ch <= 'F'))
	ret = ch - 'A' + 10;

    return ret;
}

bool CHXAltGroupSelectorTest::GetBuffer(const char* pStr, 
				       IHXBuffer*& pValue) const
{
    bool ret = false;

    int length = strlen(pStr);

    if (m_pCCF && (length > 0) && ((length & 0x1) == 0) &&
	(HXR_OK == m_pCCF->CreateInstance(CLSID_IHXBuffer, (void**)&pValue))&&
	(HXR_OK == pValue->SetSize(length / 2)))
    {
	ret = true;

	unsigned char* pDest = pValue->GetBuffer();

	while(ret && *pStr)
	{
	    int high = Char2Num(*pStr++);
	    int low = Char2Num(*pStr++);

	    if ((high == -1) || (low == -1))
	    {
		ret = false;
	    }
	    else
	    {
		*pDest++ = (unsigned char)((high << 4) | low);
	    }
	}
    }

    if (!ret)
    {
	HX_RELEASE(pValue);
    }

    return ret;
}

void CHXAltGroupSelectorTest::PrintBuffer(const char* pLabel, 
					 IHXBuffer* pBuf) const
{
    IHXBuffer* pStrBuf = 0;

    if (m_pCCF &&
	(HXR_OK == m_pCCF->CreateInstance(CLSID_IHXBuffer, (void**)&pStrBuf))&&
	(HXR_OK == pStrBuf->SetSize(pBuf->GetSize() * 2 + 1)))
    {
	static const char z_hexChars[] = "0123456789abcdef";
	
	UINT8* pSrc = pBuf->GetBuffer();
	char* pDest = (char*)pStrBuf->GetBuffer();
	
	for (ULONG32 i = 0; i < pBuf->GetSize(); i++)
	{
	    *pDest++ = z_hexChars[*pSrc >> 4];
	    *pDest++ = z_hexChars[*pSrc++ & 0xf];
	}
	*pDest = '\0';

	DPRINTF (D_ERROR, ("%s '%s'\n", 
			   pLabel, (const char*)pStrBuf->GetBuffer()));
    }

    HX_RELEASE(pStrBuf);
}

HX_RESULT 
CHXAltGroupSelectorTest::getIHXValues(const char* pID, REF(IHXValues*) pHdr)
{
    HX_RESULT res = HXR_INVALID_PARAMETER;

    if (pID)
    {
        unsigned int valueIndex;

        if (!strcmp(pID, "A"))
        {
            pHdr = m_pA;
            res = HXR_OK;
        }
        else if (!strcmp(pID, "B"))
        {
            pHdr = m_pB;
            res = HXR_OK;
        }
        else if (!UTParamUtil::GetUInt(pID, valueIndex))
        {
            DPRINTF(D_ERROR, ("CHXAltGroupSelectorTest::getIHXValues : failed to convert index parameter\n"));	
        }
        else if (valueIndex >= m_altTestHlpr.HeaderCount())
        {
            DPRINTF(D_ERROR, ("CHXAltGroupSelectorTest::getIHXValues : invalid value index\n"));
        }
        else
        {
            pHdr = m_altTestHlpr.Headers()[valueIndex];
            res = HXR_OK;
        }
    }

    return res;
}

HX_RESULT 
CHXAltGroupSelectorTest::getGroupIDBuffer(IHXValues* pHdr, 
                                         const char* pGroupType,
                                         const char* pGroupID,
                                         REF(IHXBuffer*) pIDBuffer) const
{
    HX_RESULT res = HXR_INVALID_PARAMETER;

    if (pHdr && pGroupType && pGroupID)
    {
        IHXValues* pAltGroups = NULL;

        res = getIHXValuesProp(pHdr, "Alt-Group", pAltGroups);

        if (HXR_OK == res)
        {
            IHXValues* pGroup = NULL;

            res = getIHXValuesProp(pAltGroups, pGroupType, pGroup);

            if (HXR_OK == res)
            {
                res = pGroup->GetPropertyBuffer(pGroupID, pIDBuffer);

                if (HXR_OK != res)
                {
                    DPRINTF(D_ERROR, ("CHXAltGroupSelectorTest::getGroupIDBuffer : failed to the buffer for %s\n", pGroupID));
                }
            }
            else
            {
                DPRINTF(D_ERROR, ("CHXAltGroupSelectorTest::getGroupIDBuffer : failed to get %s values\n", pGroupType));
            }

            HX_RELEASE(pGroup);
        }
        else
        {
            DPRINTF(D_ERROR, ("CHXAltGroupSelectorTest::getGroupIDBuffer : failed to get Alt-Group values\n"));
        }

        HX_RELEASE(pAltGroups);
    }

    return res;
}

HX_RESULT 
CHXAltGroupSelectorTest::getIHXValuesProp(IHXValues* pHdr, const char* pKey,
                                         REF(IHXValues*) pValue) const
{
    HX_RESULT res = HXR_INVALID_PARAMETER;

    if (pHdr)
    {
        IHXValues2* pHdr2 = NULL;
        
        res = pHdr->QueryInterface(IID_IHXValues2, (void**)&pHdr2);

        if (HXR_OK == res)
        {
            IUnknown* pUnk = NULL;
            
            res = pHdr2->GetPropertyObject(pKey, pUnk);
            
            if (HXR_OK == res)
            {
                res = pUnk->QueryInterface(IID_IHXValues, 
                                           (void**)&pValue);

                if (HXR_OK != res)
                {
                    DPRINTF(D_ERROR, ("CHXAltGroupSelectorTest::getIHXValuesProp : Alt-Group property is not an IHXValues object\n"));
                }
            }
            else
            {
                DPRINTF(D_ERROR, ("CHXAltGroupSelectorTest::getIHValuesProp : %s property is not present\n", pKey));
            }
            
            HX_RELEASE(pUnk);

        }
        else
        {
            DPRINTF(D_ERROR, ("CHXAltGroupSelectorTest::getIHXValuesProp : header does not implement IHXValues2\n"));
        }

        HX_RELEASE(pHdr2);
    }

    return res;
}
