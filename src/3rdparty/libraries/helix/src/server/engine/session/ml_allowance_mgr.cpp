/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: ml_allowance_mgr.cpp,v 1.6 2005/08/19 15:44:53 seansmith Exp $ 
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
#include <stdlib.h>
#include <sys/types.h>

#include "hxtypes.h"
#include "hxstring.h"
#include "hxcom.h"
#include "hxerror.h"

#include "server_context.h"
#include "hxmap.h"
#include "player.h"

#include "allowance_mgr.h"
#include "ml_allowance_mgr.h"


MLAllowanceMgr::MLAllowanceMgr(Process* pProc, 
                               Player::Session* pSession,
                               IHXPlayerConnectionAdviseSink* pPCAdviseSink)
    : AllowanceMgr(pProc, pPCAdviseSink)
    , m_lRefCount(0)
    , m_pSession(pSession)
    , m_pMidBoxNotify(NULL)
{
    if (pPCAdviseSink)
    {
        pPCAdviseSink->AddRef();
    }

    if (pSession)
    {
        pSession->AddRef();
    }
}

MLAllowanceMgr::~MLAllowanceMgr()
{
    HX_RELEASE(m_pMidBoxNotify);
    HX_RELEASE(m_pPCAdviseSink);
    HX_RELEASE(m_pSession);
}
    
STDMETHODIMP 
MLAllowanceMgr::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)(IHXPlayerConnectionResponse*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXPlayerConnectionResponse))
    {
        AddRef();
        *ppvObj = (IHXPlayerConnectionResponse*)this;
        return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32) 
MLAllowanceMgr::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32) 
MLAllowanceMgr::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

STDMETHODIMP
MLAllowanceMgr::AllowanceMgrDone()
{
    if (m_pPCAdviseSink)
    {
        m_pPCAdviseSink->OnDone();
    }

    HX_RELEASE(m_pPCAdviseSink);
    HX_RELEASE(m_pSession);
    HX_RELEASE(m_pMidBoxNotify);

    return HXR_OK;
}


// *** IHXPlayerConnectionAdviseSink methods *** //

STDMETHODIMP 
MLAllowanceMgr::OnConnection(THIS_ IHXPlayerConnectionResponse* pResponse)
{
    IHXPlayerConnectionResponse* pPCR = NULL;
    IHXPlayerController*          pPC  = NULL;

    // If HXR_OK is returned from the OnConnection() call, we will notify
    // this plugin whenever player events occur. Otherwise, we will release
    // the interface and the plugin will receive no notifications whatsoever.
    QueryInterface(IID_IHXPlayerConnectionResponse, (void**)&pPCR);
    if (HXR_OK != m_pPCAdviseSink->OnConnection(pPCR))
    {
        HX_RELEASE(m_pPCAdviseSink);
    }
    HX_RELEASE(pPCR);

    if (m_pPCAdviseSink)
    {
        // Provide the allowance plugin with an interface by which it can
        // affect the player (disconnect, alert, etc.)
        m_pSession->QueryInterface(IID_IHXPlayerController, (void**)&pPC);
        m_pPCAdviseSink->SetPlayerController((IHXPlayerController*)pPC);
        HX_RELEASE(pPC);

        // Does this plugin support the midbox interface?
        m_pPCAdviseSink->QueryInterface(IID_IHXMidBoxNotify,
                                        (void**)&m_pMidBoxNotify);
    }

    return HXR_OK;
}

STDMETHODIMP
MLAllowanceMgr::SetPlayerController(THIS_ IHXPlayerController* pPlayerController)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP
MLAllowanceMgr::SetRegistryID(THIS_ UINT32 ulPlayerRegistryID)
{
    if (m_pPCAdviseSink)
    {
        m_pPCAdviseSink->SetRegistryID(ulPlayerRegistryID);
    }

    return HXR_OK;
}

STDMETHODIMP 
MLAllowanceMgr::OnURL(THIS_ IHXRequest* pRequest)
{
    m_bPlaybackAllowed = FALSE;

    if (m_pMidBoxNotify != NULL)
    {
        m_pMidBoxNotify->SetMidBox(m_bIsMidBox);
    }
    else
    {
        // If the client is a midbox and the plugin doesn't support
        // midbox notification, release the advise sink to prevent
        // calling any of its methods in the future.  Playback is
        // always allowed so call OnURLDone() now.
        if (m_bIsMidBox)
        {
            HX_RELEASE(m_pPCAdviseSink);
            OnURLDone(HXR_OK);
        }
    }

    if (!m_bPlaybackAllowed && m_pPCAdviseSink)
    {
        m_pPCAdviseSink->OnURL(pRequest);
    }

    return HXR_OK;
}

STDMETHODIMP 
MLAllowanceMgr::OnBegin(void)
{
    if (m_pPCAdviseSink != NULL)
    {
        m_pPCAdviseSink->OnBegin();
    }

    return HXR_OK;
}

STDMETHODIMP 
MLAllowanceMgr::OnStop(void)
{
    if (m_pPCAdviseSink != NULL)
    {
        m_pPCAdviseSink->OnStop();
    }

    return HXR_OK;
}

STDMETHODIMP 
MLAllowanceMgr::OnDone(void)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP 
MLAllowanceMgr::OnPause(void)
{
    if (m_pPCAdviseSink != NULL)
    {
        m_pPCAdviseSink->OnPause();
    }

    return HXR_OK;
}


/* IHXPlayerConnectionResponse methods */

STDMETHODIMP
MLAllowanceMgr::OnConnectionDone(THIS_ HX_RESULT status)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP 
MLAllowanceMgr::OnURLDone(THIS_ HX_RESULT status)
{
    if (HXR_OK == status)
    {
        m_bPlaybackAllowed = TRUE;
    }

    // Notify the Player object
    if (m_pSession != NULL)
    {
        m_pSession->OnURLDone(status);
    }

    return HXR_OK;
}

STDMETHODIMP
MLAllowanceMgr::OnBeginDone(THIS_ HX_RESULT status)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP
MLAllowanceMgr::OnStopDone(THIS_ HX_RESULT status)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP
MLAllowanceMgr::OnPauseDone(THIS_ HX_RESULT status)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP_(void)
MLAllowanceMgr::PrintDebugInfo(THIS)
{
    IHXPlugin*  pPlugin = NULL;

    if (m_pPCAdviseSink)
    {
    if (HXR_OK == m_pPCAdviseSink->QueryInterface(IID_IHXPlugin, (void **)&pPlugin))
    {
        const char* pDescription;
        const char* pCopyright;
        const char* pMoreInfoURL;
        UINT32      unVersionNumber;
        BOOL        bLoadMultiple;

        pPlugin->GetPluginInfo(bLoadMultiple, pDescription, pCopyright
                                , pMoreInfoURL, unVersionNumber);

        printf("AO: SessionID: %s -- \"%s\" (%s - NULL) -- "
            , (const char *)(m_pSession->m_sessionID)
            , pDescription
            , bLoadMultiple ? "Multiload" : "Non-Multiload");
    }
}
}

STDMETHODIMP_(BOOL)
MLAllowanceMgr::AcceptMountPoint(THIS_ const char* pszMountPoint, INT32 uLen)
{
    return TRUE;
}

