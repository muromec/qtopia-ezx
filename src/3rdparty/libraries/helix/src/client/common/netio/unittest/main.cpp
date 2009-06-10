/* ***** BEGIN LICENSE BLOCK ***** 
 * Version: RCSL 1.0/RPSL 1.0 
 *  
 * Portions Copyright (c) 1995-2004 RealNetworks, Inc. All Rights Reserved. 
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

#include "hxtypes.h"
#include "hxcom.h"
#include "hlxclib/stdio.h"
#include "hlxclib/string.h"
#include "hxnet.h"
#include "dllpath.h"
#include "hxstring.h"
#include "minicntx.h"
#include "sockimp.h"
#include "netdrv.h"
#include "hxclientsockimp.h"
#include "hxclientsockimpshim.h"
#include "testbase.h"

ENABLE_DLLACCESS_PATHS(clntnettest_main);

void print_usage(const char* pszApp)
{
    fprintf(stderr, "Usage: %s <test>\n\n", pszApp);
    fprintf(stderr, "       <test> can be the following:\n\n");
    fprintf(stderr, "          \"httpget\": Test HTTP GET\n");
    fprintf(stderr, "\n");
};

int main(int argc, char *argv[])
{
    HX_RESULT retVal = HXR_OK;
    UINT32    ulTest = kNumTests;

    if (argc >= 2)
    {
        if (!strcmp(argv[1], "httpget"))
        {
            ulTest = kTestHTTPGet;
        }
        else if (!strcmp(argv[1], "echoserver"))
        {
            ulTest = kTestEchoServer;
        }
    }

    if (ulTest < kNumTests)
    {
        // Open the net driver
        hx_netdrv_open();
        // Create the mini-context
        MiniContext* pMiniContext = new MiniContext();
        if (pMiniContext)
        {
            // AddRef the mini-context
            pMiniContext->AddRef();
            // Init the mini-context
            retVal = pMiniContext->Init();
            if (SUCCEEDED(retVal))
            {
                // Get the IUnknown from the context
                IUnknown* pUnk = NULL;
                retVal = pMiniContext->QueryInterface(IID_IUnknown, (void**) &pUnk);
                if (SUCCEEDED(retVal))
                {
                    // Create the client net services shim
                    CHXClientNetServicesShim* pShim = new CHXClientNetServicesShim();
                    if (pShim)
                    {
                        // Set the context into the shim
                        retVal = pShim->SetContext(pUnk);
                        if (SUCCEEDED(retVal))
                        {
                            // Get the IHXNetServices interface
                            IHXNetServices* pNetServices = NULL;
                            retVal = pShim->QueryInterface(IID_IHXNetServices, (void**) &pNetServices);
                            if (SUCCEEDED(retVal))
                            {
                                // Create the test class
                                TestBase* pTest = TestBase::CreateTest(ulTest);
                                if (pTest)
                                {
                                    // Run the test
                                    retVal = pTest->RunTest(pMiniContext, pNetServices, argc, argv);
                                }
                                else
                                {
                                    retVal = HXR_OUTOFMEMORY;
                                }
                                HX_RELEASE(pTest);
                            }
                            HX_RELEASE(pNetServices);
                        }
                    }
                    else
                    {
                        retVal = HXR_OUTOFMEMORY;
                    }
                }
                HX_RELEASE(pUnk);
                // Close the MiniContext
                pMiniContext->Close();
            }
        }
        HX_RELEASE(pMiniContext);
        // Close the network driver
        hx_netdrv_close();
    }
    else
    {
        print_usage((const char*) argv[0]);
    }

    return retVal;
}
