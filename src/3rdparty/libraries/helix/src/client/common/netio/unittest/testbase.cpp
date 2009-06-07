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
#include "hxnet.h"
#include "hxengin.h"
#include "hxstring.h"
#include "testbase.h"
#include "testhttpget.h"
#include "testechoserver.h"

TestBase* TestBase::CreateTest(UINT32 ulTest)
{
    TestBase* pRet = NULL;

    if (ulTest < kNumTests)
    {
        switch (ulTest)
        {
            case kTestHTTPGet:    pRet = new TestHTTPGet();    break;
            case kTestEchoServer: pRet = new TestEchoServer(); break;
        }
        if (pRet) pRet->AddRef();
    }

    return pRet;
}

void TestBase::MakeSockEventString(UINT32 ulEvent, CHXString& rStr)
{
    // Clear out the string
    rStr.Empty();
    // Init the string array
    const char* pszStr[] = {"HX_SOCK_EVENT_READ",
                            "HX_SOCK_EVENT_WRITE",
                            "HX_SOCK_EVENT_EXCEPT",
                            "HX_SOCK_EVENT_ACCEPT",
                            "HX_SOCK_EVENT_CONNECT",
                            "HX_SOCK_EVENT_CLOSE",
                            "HX_SOCK_EVENT_TIMEOUT"};
    for (UINT32 i = 0; i < 7; i++)
    {
        UINT32 ulEventMask = (1 << i);
        if (ulEvent & ulEventMask)
        {
            if (rStr.GetLength() > 0)
            {
                rStr += "| ";
            }
            rStr += pszStr[i];
        }
    }
}

void TestBase::MakeSockErrString(INT32 lErr, CHXString& rStr)
{
    switch (lErr)
    {
        case HX_SOCKERR_NONE:        rStr = "HX_SOCKERR_NONE";        break;
        case HX_SOCKERR_INTR:        rStr = "HX_SOCKERR_INTR";        break;
        case HX_SOCKERR_ALREADY:     rStr = "HX_SOCKERR_ALREADY";     break;
        case HX_SOCKERR_NOTCONN:     rStr = "HX_SOCKERR_NOTCONN";     break;
        case HX_SOCKERR_INPROGRESS:  rStr = "HX_SOCKERR_INPROGRESS";  break;
        case HX_SOCKERR_WOULDBLOCK:  rStr = "HX_SOCKERR_WOULDBLOCK";  break;
        case HX_SOCKERR_PROTOTYPE:   rStr = "HX_SOCKERR_PROTOTYPE";   break;
        case HX_SOCKERR_NOPROTOOPT:  rStr = "HX_SOCKERR_NOPROTOOPT";  break;
        case HX_SOCKERR_PROTONOSUPP: rStr = "HX_SOCKERR_PROTONOSUPP"; break;
        case HX_SOCKERR_AFNOSUPP:    rStr = "HX_SOCKERR_AFNOSUPP";    break;
        case HX_SOCKERR_CONNREFUSED: rStr = "HX_SOCKERR_CONNREFUSED"; break;
        default:                     rStr = "Unknown";                break;
    }
}

