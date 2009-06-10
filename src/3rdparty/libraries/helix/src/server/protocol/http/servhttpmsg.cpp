/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: servhttpmsg.cpp,v 1.4 2009/04/14 19:21:47 svaidhya Exp $
 *
 * Portions Copyright (c) 1995-2003 RealNetworks, Inc. All Rights Reserved.
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

#include <stdio.h>

#include "hxcom.h"
#include "hxtypes.h"
#include "hxstrutl.h"
#include "hxstring.h"
#include "hxslist.h"
#include "mimehead.h"
#include "ihxpckts.h"
#include "hxauth.h"
#include "httpmsg.h"

#include "hxheap.h"

#include "servhttpmsg.h"


struct HTTPError
{
    const char* pErrNo;
    const char* pErrMsg;
};

static HTTPError HTTPErrorTable[] =
{
    { "200", "OK" },
    { "201", "Created" },
    { "202", "Accepted" },
    { "204", "No Content" },
    { "273", "No Session" },
    { "274", "Content Not Compatible" },
    { "275", "Content Not Switchable" },
    { "301", "Moved Permanently" },
    { "302", "Moved Temporarily" },
    { "304", "Not Modified" },
    { "400", "Bad Request" },
    { "401", "Unauthorized" },
    { "403", "Forbidden" },
    { "404", "Not Found" },
    { "405", "Method Not Allowed" },
    { "412", "Precondition Failed" },
    { "415", "Unsupported Media Type" },
    { "500", "Internal Server Error" },
    { "501", "Not Implemented" },
    { "502", "Bad Gateway" },
    { "503", "Service Unavailable" }
};

void 
ServerHTTPResponseMessage::setErrorCode(const char* eCode)
{
    HTTPResponseMessage::setErrorCode(eCode);
    setErrorMsg(lookupErrorText(eCode));
}


const char*
ServerHTTPResponseMessage::lookupErrorText(const char* pErrNo)
{
    const char* pErrMsg = "";
    int tabSize = sizeof(HTTPErrorTable)/sizeof(HTTPErrorTable[0]);
    for(int i=0; i<tabSize; ++i)
    {
        if(strcmp(pErrNo, HTTPErrorTable[i].pErrNo) == 0)
        {
            pErrMsg = HTTPErrorTable[i].pErrMsg;
            break;
        }
    }
    return pErrMsg;    
}
