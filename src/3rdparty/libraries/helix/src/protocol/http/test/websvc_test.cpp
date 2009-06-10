/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: websvc_test.cpp,v 1.3 2007/07/06 20:51:34 jfinnecy Exp $
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

#include "websvc_test.h"
#include "websvc_response.h"

#include <unistd.h>

#include "hxhttp.h"
#include "hxccf.h"

#include "hx_ut_debug.h"
#include "ut_param_util.h"

#define MAX_HTTP_TIMEOUT 30000 // in ms. This is a minimum -- test will take longer.
#define SLEEP_TIME          10 // in ms

// extern const HXClientCallbacks g_kitCallbacks;

#include "websvc_kit.cpp"

HLXWebServicesTest::HLXWebServicesTest() :
    m_bInitialized(false),
    m_bTestCompleted(false),
    m_clientPlayerToken(0)
{
    memset(&m_clientWindow, 0, sizeof(m_clientWindow));
}

HLXWebServicesTest::~HLXWebServicesTest()
{
}

const char* HLXWebServicesTest::DefaultCommandLine() const
{
    return "websvc websvc.in";
}

void HLXWebServicesTest::GetCommandInfo(UTVector<HLXUnitTestCmdInfo*>& cmds)
{
    cmds.Resize(2);
    cmds[0] = new HLXUnitTestCmdInfoDisp<HLXWebServicesTest>(this, 
				    "Get",
				    &HLXWebServicesTest::HandleGetCmd,
				    3);

    cmds[1] = new HLXUnitTestCmdInfoDisp<HLXWebServicesTest>(this, 
                                    "Post",
                                    &HLXWebServicesTest::HandlePostCmd,
                                    4);

}

bool HLXWebServicesTest::HandleGetCmd(const UTVector<UTString>& info)
{
    const char* szUrl = NULL;
    const char* szExpectedBody = NULL;

    szUrl = info[1];
    szExpectedBody = info[2];

    m_bTestCompleted = false;    

    return TestHttp(szUrl, szExpectedBody);
}

bool HLXWebServicesTest::HandlePostCmd(const UTVector<UTString>& info)
{
    const char* szUrl = NULL;
    const char* szExpectedBody = NULL;
    const char* szPostBody = NULL;

    szUrl = info[1];
    szExpectedBody = info[2];
    szPostBody = info[3];
    
    m_bTestCompleted = false;    

    return TestHttp(szUrl, szExpectedBody, szPostBody);
}

bool HLXWebServicesTest::TestHttp(const char* szUrl,
                                  const char* szExpectedBody,
                                  const char* szPostBody)
{
    bool bRet = false;            
    IUnknown* pUnknown = NULL;
    IHXHttp* pHttp = NULL;
    HX_RESULT res;
    HLXHttpTestResponse* pTestResponse = NULL;

    res = GetWebServices(&pUnknown);        
        
    if(SUCCEEDED(res))
    {
        res = pUnknown->QueryInterface(IID_IHXHttp,
                                       (void**) &pHttp);
    }

    if(SUCCEEDED(res))
    {
        IUnknown* pContext;

        pTestResponse = new HLXHttpTestResponse(this, szPostBody);
        HX_ADDREF(pTestResponse);

        res = pTestResponse->QueryInterface(IID_IUnknown, (void**) &pContext);

        if(SUCCEEDED(res))
        {
            res = pHttp->Initialize(pContext);
            HX_RELEASE(pContext);
        }
    }


    if(SUCCEEDED(res))
    {
        if(szPostBody)
        {
            IHXHttp2* pHttp2;

            res = pHttp->QueryInterface(IID_IHXHttp2,
                                        (void**)&pHttp2);

            if(SUCCEEDED(res))
            {
                res = pHttp2->Post(szUrl,
                                   strlen(szPostBody));
            }

            HX_RELEASE(pHttp2);
        }
        else
        {
            res = pHttp->Get(szUrl);
        }
    }

    if(SUCCEEDED(res))
    {
        UINT32 i;

        for(i = 0; i < (MAX_HTTP_TIMEOUT / SLEEP_TIME); i++)
        {
            usleep(1000 * SLEEP_TIME);            

#ifdef _UNIX
            /* On Unix, we have to pump the client engine */
            ClientEngineProcessXEvent(NULL);    
#endif

            if(m_bTestCompleted)
            {
                /* Check if the returned body was what we're expecting */
                const char* szActualBody = pTestResponse->GetResponseBody();
                
                /* Allow trailing newlines -- apache seems to add them  */
                if(strncmp(szExpectedBody, szActualBody, strlen(szExpectedBody)) == 0)
                {
                    bRet = true;
                }
                break;
            }
        }
    }        

    HX_RELEASE(pTestResponse);
    HX_RELEASE(pHttp);
    HX_RELEASE(pUnknown);

    return bRet;
}

HLXCmdBasedTest* HLXWebServicesTest::Clone() const
{
    return new HLXWebServicesTest;
}

HX_RESULT HLXWebServicesTest::GetWebServices(IUnknown** ppUnknown)
{
    bool bResult;

    *ppUnknown = NULL;
    
    if(!m_bInitialized)
    {    
        bResult = ClientPlayerCreate(&m_clientPlayerToken,
                                     &m_clientWindow,
                                     this,
                                     &g_kitCallbacks);
        if(!bResult)
        {
            return HXR_FAIL;
        }
    }

    HX_RESULT res;
    IUnknown *pEngine;
    IHXCommonClassFactory* pCCF;
    IUnknown* pUnknown = NULL;
    IHXHttpInitialize* pHttpInitialize = NULL;
    
    bResult = ClientEngineGetUnknown ((void**) &pEngine);

    if(!bResult)
    {
        return HXR_FAIL;
    }    

    res = pEngine->QueryInterface(IID_IHXCommonClassFactory,
                                  (void**) &pCCF);

    if(SUCCEEDED(res))
    {        
        res = pCCF->CreateInstance(CLSID_IHXHttp, (void **) &pUnknown);
    }

    if(SUCCEEDED(res))
    {
        res = pUnknown->QueryInterface(IID_IHXHttpInitialize,
                                      (void**) &pHttpInitialize);
    }
    
    if(SUCCEEDED(res))
    {
        res = pHttpInitialize->Init(pEngine);
    }

    HX_RELEASE(pHttpInitialize);
    HX_RELEASE(pEngine);

    if(FAILED(res))
    {
        HX_RELEASE(pUnknown);
    }

    *ppUnknown = pUnknown;
    
    return res;
}
