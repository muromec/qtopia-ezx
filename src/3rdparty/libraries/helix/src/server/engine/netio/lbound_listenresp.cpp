/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: lbound_listenresp.cpp,v 1.11 2004/11/16 19:02:15 tmarshall Exp $
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

#include "debug.h"
#include "netbyte.h"

#include "proc.h"
#include "client.h"
#include "player.h"
#include "hxprot.h"
#include "rtspprot.h"

#include "lbound_listenresp.h"
#include "hxstring.h"
#include "hxslist.h"
#include "server_engine.h"

#include "servlbsock.h"

LBoundRTSPListenResponse::LBoundRTSPListenResponse(Process* _proc) :
    m_lRefCount(0),
    m_pProc(_proc)
{
    DPRINTF(0x00200000, ("%p LBoundRTSPListenResponse created.\n", this));
}

LBoundRTSPListenResponse::~LBoundRTSPListenResponse(void)
{
    DPRINTF(0x00200000, ("%p LBoundRTSPListenResponse destroyed\n", this));
}

STDMETHODIMP
LBoundRTSPListenResponse::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)(IHXListeningSocketResponse*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXListeningSocketResponse))
    {
        AddRef();
        *ppvObj = (IHXListeningSocketResponse*)this;
        return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32)
LBoundRTSPListenResponse::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32)
LBoundRTSPListenResponse::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}


HX_RESULT
LBoundRTSPListenResponse::OnConnection(CLBSocket* pNewSock, Process* pProc)
{
    RTSPProtocol* pProt = new RTSPProtocol();

    pProt->init(pProc, pNewSock);

    return HXR_OK;
}

