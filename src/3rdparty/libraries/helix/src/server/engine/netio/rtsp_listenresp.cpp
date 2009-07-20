/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: rtsp_listenresp.cpp,v 1.14 2005/01/25 22:55:21 jzeng Exp $
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

#include "hxtypes.h"
#include "hxcom.h"
#include "hxerror.h"
#include "hxstring.h"

#include "debug.h"
#include "netbyte.h"

#include "proc.h"
#include "client.h"

#include "listenresp.h"
#include "rtsp_listenresp.h"
#include "server_engine.h"

RTSPListenResponse::RTSPListenResponse(Process* _proc) :
    ListenResponse(_proc)
{
    // Empty
}

RTSPListenResponse::~RTSPListenResponse(void)
{
    HX_RELEASE(m_pSock);
}

HX_RESULT
RTSPListenResponse::Init(UINT16 uPort)
{
    HX_RESULT rc;

    rc = InitSocket(uPort);
    if (SUCCEEDED(rc))
    {
        g_pProtMgr->SetDefaultProtocol(HXPROT_RTSP, m_pAddr);
    }

    return rc;
}

STDMETHODIMP
RTSPListenResponse::OnConnection(IHXSocket* pNewSock, IHXSockAddr* pSource)
{
    HXSocketConnection* pConn;
    pConn = new HXSocketConnection(pNewSock, HXPROT_UNKNOWN);
    // NB: We do not AddRef the connection, the socket does

    return HXR_OK;
}

STDMETHODIMP
RTSPListenResponse::OnError(HX_RESULT err)
{
    //Remove the assert
    //This could happen with access control turned on.
    return HXR_OK;
}
