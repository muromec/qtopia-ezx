/* ***** BEGIN LICENSE BLOCK *****
 * Version: RCSL 1.0/RPSL 1.0
 *
 * Portions Copyright (c) 1995-2002 RealNetworks, Inc. All Rights Reserved.
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
#include "hxcomm.h"
#include "rtspclnt.h"
#include "sessionlingertimeout.h"

#include "hxtlogutil.h"
#include "hxassert.h"
#include "debug.h"
#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE
static const char HX_THIS_FILE[] = __FILE__;
#endif


// IUnknown
BEGIN_INTERFACE_LIST_NOCREATE(SessionLingerTimeout)
    INTERFACE_LIST_ENTRY_SIMPLE(IHXCallback)
END_INTERFACE_LIST


// static constuct
HX_RESULT SessionLingerTimeout::Create(IUnknown* pContext, 
                                      RTSPClientSessionManager* pMgr,
                                      SessionLingerTimeout*& pObject)
{
    HX_RESULT res = HXR_OK;
    pObject = new SessionLingerTimeout();
    if (pObject)
    {
        pObject->AddRef();
        res = pObject->Init(pContext, pMgr);
        if (FAILED(res))
        {
            HX_RELEASE(pObject);
        }
    }
    return res;
}

SessionLingerTimeout::SessionLingerTimeout()
: m_pSessionMgr(NULL)
, m_pScheduler(NULL)
, m_pLingerSession(NULL)
, m_hCallback(0)
{
}


SessionLingerTimeout::~SessionLingerTimeout()
{
    if (m_hCallback)
    {
        m_pScheduler->Remove(m_hCallback);
        m_hCallback = 0;
    }
    HX_RELEASE(m_pSessionMgr);
    HX_RELEASE(m_pLingerSession);
    HX_RELEASE(m_pScheduler);
}

HX_RESULT
SessionLingerTimeout::Init(IUnknown* pContext, RTSPClientSessionManager* pMgr)
{
    HXLOGL3(HXLOG_RTSP, "SessionLingerTimeout[%p]::Init()", this);
    HX_ASSERT(pContext);
    HX_ASSERT(pMgr);
    m_pSessionMgr = pMgr;
    m_pSessionMgr->AddRef();
    return pContext->QueryInterface(IID_IHXScheduler, (void**)&m_pScheduler);
}

// Called when a session drops its last protocol. We keep the
// session alive for a short timeout period in case a re-connect
// to the same server:port is requested, in which case we can
// re-use the the session (and the connection).
//
void
SessionLingerTimeout::Start(RTSPClientSession* pSession, UINT32 msLinger)
{
    HX_ASSERT(pSession);
    HX_ASSERT(msLinger > 0);

    HX_ASSERT(!m_pLingerSession);
    m_pLingerSession = pSession;
    m_pLingerSession->AddRef();
    m_hCallback = m_pScheduler->RelativeEnter(this, msLinger);
}


// Called after session is found or created anew for new protocol attempt.
// At that time we know if the linger session is being re-used (because
// it no longer will be empty).
//
void
SessionLingerTimeout::Cancel()
{
    if (m_hCallback)
    {
        HXLOGL3(HXLOG_RTSP, "SessionLingerTimeout[%p]::Cancel(): aborting callback", this, m_hCallback);

        // Cancel pending timeout.
        m_pScheduler->Remove(m_hCallback);
        m_hCallback = 0;

        HX_ASSERT(m_pLingerSession);

        // Remove lingering session if is still empty.
        if (m_pLingerSession->isEmpty())
        {
            HXLOGL3(HXLOG_RTSP, "SessionLingerTimeout[%p]::Cancel(): removing last sess [%p]", this, m_pLingerSession);
            m_pSessionMgr->OnLingerTimeout(m_pLingerSession);
        }
        HX_RELEASE(m_pLingerSession);
    }
}

STDMETHODIMP
SessionLingerTimeout::Func()
{
    HXLOGL3(HXLOG_RTSP, "SessionLingerTimeout[%p]::Func(): linger timeout sess [%p]", this, m_pLingerSession);
    m_pSessionMgr->OnLingerTimeout(m_pLingerSession);
    HX_RELEASE(m_pLingerSession);
    m_hCallback = 0;
    return HXR_OK;
}

