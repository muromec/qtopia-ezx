/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: httpprotmgr.cpp,v 1.2 2004/08/24 21:37:31 tmarshall Exp $
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

#include <signal.h>

#include <stdio.h>
#include <string.h>

#include "hxtypes.h"
#include "hxcom.h"
#include "hxerror.h"
#include "hxprefs.h"
#include "hxstring.h"
#include "debug.h"
#include "client.h"

#include "base_errmsg.h"
#include "http.h"
#include "httpprotmgr.h"

const int MAX_HTTP_MSG = 32768;

struct HTTPError
{
    const char* pErrNo;
    const char* pErrMsg;
};

// XXXBAB - make this a dictionary lookup

static HTTPError HTTPErrorTable[] =
{
    { "200", "OK" },
    { "201", "Created" },
    { "202", "Accepted" },
    { "204", "No Content" },
    { "301", "Moved Permanently" },
    { "302", "Moved Temporarily" },
    { "304", "Not Modified" },
    { "400", "Bad Request" },
    { "401", "Unauthorized" },
    { "403", "Forbidden" },
    { "404", "Not Found" },
    { "415", "Unsupported Media Type" },
    { "500", "Internal Server Error" },
    { "501", "Not Implemented" },
    { "502", "Bad Gateway" },
    { "503", "Service Unavailable" }
};

HTTPProtocolMgr::HTTPProtocolMgr(IHXSocket* pSock) :
    HXProtocol(),
    m_nRefCount(0)
{
    // Fixme...
}

HTTPProtocolMgr::~HTTPProtocolMgr(void)
{
    // Empty
}

STDMETHODIMP
HTTPProtocolMgr::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = this;
        return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(UINT32)
HTTPProtocolMgr::AddRef(void)
{
    return InterlockedIncrement(&m_nRefCount);
}

STDMETHODIMP_(UINT32)
HTTPProtocolMgr::Release(void)
{
    if(InterlockedDecrement(&m_nRefCount) > 0)
    {
        return m_nRefCount;
    }
    delete this;
    return 0;
}
