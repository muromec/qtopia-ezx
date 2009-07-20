/****************************************************************************
 *
 *  $Id: mms_listenresp.cpp,v 1.9 2005/01/25 22:55:21 jzeng Exp $
 *
 *  Copyright (C) 1995-2002 RealNetworks, Inc.
 *  All rights reserved.
 *
 *  RealNetworks Confidential and Proprietary information.
 *  Do not redistribute.
 *
 */

#include "hxtypes.h"
#include "hxcom.h"
#include "hxerror.h"
#include "hxstring.h"

#include "debug.h"
#include "netbyte.h"

#include "proc.h"
#include "client.h"

#include "listenresp.h"
#include "mms_listenresp.h"
#include "server_engine.h"

//#include "hxprot.h"
//#include "hxmms.h"
//#include "mmsprot.h"
//#include "basepkt.h"
//#include "mmsserv.h"

MMSListenResponse::MMSListenResponse(Process* _proc) :
    ListenResponse(_proc)
{
    // Empty
}

MMSListenResponse::~MMSListenResponse(void)
{
    HX_RELEASE(m_pSock);
}

HX_RESULT
MMSListenResponse::Init(UINT16 uPort)
{
    HX_RESULT rc;

    rc = InitSocket(uPort);

    return rc;
}

STDMETHODIMP
MMSListenResponse::OnConnection(IHXSocket* pNewSock, IHXSockAddr* pSource)
{
    HXSocketConnection* pConn;
    pConn = new HXSocketConnection(pNewSock, HXPROT_MMS);
    // NB: We do not AddRef the connection, the socket does

    return HXR_OK;
}

STDMETHODIMP
MMSListenResponse::OnError(HX_RESULT err)
{
    //Remove the assert
    //This could happen with access control turned on.
    return HXR_OK;
}
