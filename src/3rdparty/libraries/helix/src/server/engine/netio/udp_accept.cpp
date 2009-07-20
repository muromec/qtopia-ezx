/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: udp_accept.cpp,v 1.3 2004/07/24 23:10:19 tmarshall Exp $
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

#include <stdlib.h>
#include <stdio.h>

#include "hxtypes.h"
#include "hxcom.h"
#include "hxerror.h"
#include "debug.h"
#include "udpio.h"
#include "engine.h"
#include "udp_accept.h"

UDPAcceptor::UDPAcceptor(Process* proc, Engine* engine)
{
    m_proc                      = proc;
    m_engine                    = engine;
    m_conn                      = 0;
    m_message_received_cb       = new MessageReceivedCallback;
    m_message_received_cb->AddRef();
    m_message_received_cb->m_udp_acceptor = this;
}

UDPAcceptor::~UDPAcceptor()
{
    if (m_conn)
    {
        m_engine->callbacks.remove(HX_READERS, m_conn);
        delete m_conn;
        m_engine->UnRegisterSock();
    }
    m_message_received_cb->Release();
}

int
UDPAcceptor::Init(UINT16 first_port, UINT16 last_port)
{
    int result = 0;

    if (first_port > last_port)
    {
        ASSERT(FALSE);
        return -1;
    }

    m_conn = new UDPIO();
    m_engine->RegisterSock();
    if (m_conn->error())
    {
        return result;
    }
    for(int i = first_port; i <= last_port; i++)
    {
        result = m_conn->init(INADDR_ANY, i, 0, 0);     // don't reuse addr
        if (result >= 0)
        {
            m_port = m_conn->port();
            break;
        }
    }
    DPRINTF(D_INFO, ("UDPAcceptor: assigned port %d\n", m_port));

    if (result != 0)
    {
        return result;
    }
    m_engine->callbacks.add(HX_READERS, m_conn, m_message_received_cb, TRUE);

    return result;
}

int
UDPAcceptor::Init(UINT16 port)
{
    int result = -1;
    m_conn = new UDPIO();
    m_engine->RegisterSock();
    if (m_conn->error())
    {
        return result;
    }

    result = m_conn->init(INADDR_ANY, port, 0, 0);      // don't reuse addr
    if (result >= 0)
    {
        m_port = m_conn->port();
    }
    else
    {
        return result;
    }

    DPRINTF(D_INFO, ("UDPAcceptor: assigned port %d\n", port));
    m_engine->callbacks.add(HX_READERS, m_conn, m_message_received_cb, TRUE);

    return result;
}

void
UDPAcceptor::Enable()
{
    m_engine->callbacks.enable(HX_READERS, m_conn, TRUE);
}

void
UDPAcceptor::Disable()
{
    m_engine->callbacks.disable(HX_READERS, m_conn);
}

int
UDPAcceptor::Error()
{
    if (m_conn)
    {
        return m_conn->error();
    }
    return -1;
}

STDMETHODIMP
UDPAcceptor::MessageReceivedCallback::Func()
{
    m_udp_acceptor->MessageReceived();
    return HXR_OK;
}
