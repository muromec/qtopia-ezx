/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxprot.cpp,v 1.7 2007/05/10 18:43:05 seansmith Exp $
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
#include "hxcomm.h"

#include "dict.h"
#include "debug.h"

#include "tcpio.h"
#include "fsio.h"

#include "rttp2.h"

#include "id.h"

#include "config.h"
#include "client.h"

#include "hxprot.h"

HXProtocol::HXProtocol(void) :
    m_pSock(NULL),
    m_pClient(NULL),
    m_proc(NULL)
{
    // Empty
}

HXProtocol::~HXProtocol(void)
{
    HX_RELEASE(m_pClient);
    HX_RELEASE(m_pSock);
}

int
HXProtocol::setupHeader(IHXValues* pHeader, ClientSession* pSession,
    HX_RESULT status)
{
    return 0;
}

int
HXProtocol::setupStreams(CHXSimpleList* headers, ClientSession* pSession,
    HX_RESULT result)
{
    return 0;
}

int
HXProtocol::playDone(const char* pSessionID)
{
    return 0;
}

int
HXProtocol::disconnect(const char* pSessionID)
{
    return 0;
}

int
HXProtocol::sendAlert(const char* pSessionID, StreamError err)
{
    return 0;
}

int
HXProtocol::sendAlert(const char* pSessionID, IHXBuffer* pAlert)
{
    return 0;
}

int
HXProtocol::sendRedirect(const char* pSessionID,
                         const char* pURL,
                         UINT32 ulSecsFromNow)
{
    return 0;
}

Transport*
HXProtocol::getTransport(ClientSession* pSession,
    UINT16 stream_number, UINT32 reliability)
{
    return 0;
}

UINT32
HXProtocol::controlBytesSent()
{
    return m_pClient->m_uBytesSent;
}

void
HXProtocol::Done(HX_RESULT status)
{
    if (m_pSock != NULL)
    {
        m_pSock->Close();
        HX_RELEASE(m_pSock);
    }
}

void
HXProtocol::dumpMsg(const Byte* pMsg, int nMsgLen)
{
    char* dumpBuf = new char[nMsgLen*3+1];

    for(int i=0; i<nMsgLen; i++)
        sprintf(&dumpBuf[i*3], "%02x ", pMsg[i]);
    DPRINTF(D_INFO, ("Dump: %s\n", dumpBuf));
    delete[] dumpBuf;
}
