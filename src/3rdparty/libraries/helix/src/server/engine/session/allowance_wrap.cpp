/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: allowance_wrap.cpp,v 1.10 2006/10/03 23:19:07 tknox Exp $
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
#include "hxtypes.h"
#include "hxcom.h"
#include "hxerror.h"
#include "hxfiles.h"
#include "hxauth.h"
#include "hxplugn.h"
#include "ihxpckts.h"
#include "hxassert.h"
#include "proc.h"
#include "dispatchq.h"
#include "simple_callback.h"
#include "player.h"
#include "allowance_mgr.h"
#include "allowance_wrap.h"

/*
 * NOTE!!!...The m_pPCAdviseSink member variable is only valid in the
 *           plugin's process space. It is a foreign pointer in the
 *           streamer's process space and therefore MAY NOT BE USED
 */

void
AllowanceWrapperSinkCreateCallback::func(Process* proc)
{
    m_pPCAdviseSinkManager->CreatePlayerConnectionAdviseSink(
        m_pWrapper->m_pPCAdviseSink);

    /*
     * This matches up with AddRef() in OnConnection()
     */

    m_pWrapper->Release();

    delete this;
}

void
AllowanceWrapperSinkCallback::func(Process* proc)
{
    switch(m_Type)
    {
        case SINK_RELEASE:
        {
            if (m_pWrapper->m_pPCAdviseSink)
            {
                m_pWrapper->m_pPCAdviseSink->OnDone();
                m_pWrapper->m_pPCAdviseSink->Release();
                m_pWrapper->m_pPCAdviseSink = 0;
            }

            break;
        }

        case SINK_ON_URL:
        {
            HX_RESULT hresult = m_pWrapper->HandleOnURL();

            if (HXR_OK != hresult)
            {
                m_pWrapper->DispatchResponseCallback(RESPONSE_ON_URL_DONE,
                                                     hresult);
            }

            break;
        }

        case SINK_ON_BEGIN:
            m_pWrapper->m_pPCAdviseSink->OnBegin();
            break;

        case SINK_ON_STOP:
            m_pWrapper->m_pPCAdviseSink->OnStop();
            break;

        case SINK_ON_PAUSE:
            m_pWrapper->m_pPCAdviseSink->OnPause();
            break;

        default:
            DPRINTF(D_ERROR, ("Unexpected allowance sink type %d\n", m_Type));
    }

    /*
     * This matches up with AddRef() in DispatchSinkCallback()
     */

    m_pWrapper->Release();

    delete this;
}

void
AllowanceWrapperResponseCallback::func(Process* proc)
{
    if (m_Type == RESPONSE_ON_URL_DONE) {
      m_pWrapper->HandleOnURLDone(m_status);
    }

    /*
     * This matches up with AddRef() in DispatchResponseCallback()
     */

    m_pWrapper->Release();

    delete this;

}

AllowanceWrapperControllerCallback::AllowanceWrapperControllerCallback
(
    IHXPlayerController* pPlayerController,
    ControllerCallbackType Type,
    void* pParameter
) : m_pPlayerController(pPlayerController)
  , m_Type(Type)
  , m_pParameter(pParameter)
{
    if (m_pPlayerController)
    {
        m_pPlayerController->AddRef();
    }
}

AllowanceWrapperControllerCallback::~AllowanceWrapperControllerCallback()
{
    HX_RELEASE(m_pPlayerController);
}

void
AllowanceWrapperControllerCallback::func(Process* proc)
{
    switch(m_Type)
    {
        case CONTROLLER_PAUSE:
            m_pPlayerController->Pause();
            break;

        case CONTROLLER_RESUME:
            m_pPlayerController->Resume();
            break;

        case CONTROLLER_DISCONNECT:
            m_pPlayerController->Disconnect();
            break;

        case CONTROLLER_ALERT_AND_DISCONNECT:
        {
            IHXBuffer* pAlert = (IHXBuffer*)m_pParameter;
            m_pPlayerController->AlertAndDisconnect(pAlert);
            pAlert->Release();
            break;
        }

        case CONTROLLER_REDIRECT:
        {
            IHXBuffer* pURL = (IHXBuffer*)m_pParameter;
            m_pPlayerController->Redirect(pURL);
            pURL->Release();
            break;
        }

        case CONTROLLER_HOST_REDIRECT:
        {
            AllowanceHostRedirectWrapperParameters* pARWP =
                (AllowanceHostRedirectWrapperParameters*)m_pParameter;

            m_pPlayerController->HostRedirect(pARWP->m_pHost,
                (UINT16)pARWP->m_unPort);
            pARWP->m_pHost->Release();
            delete pARWP;
            break;
        }

        case CONTROLLER_NETWORK_REDIRECT:
        {
            AllowanceNetworkRedirectWrapperParameters* pARWP =
                (AllowanceNetworkRedirectWrapperParameters*)m_pParameter;

            m_pPlayerController->NetworkRedirect(pARWP->m_pBuffer,
                pARWP->m_ulSecsFromNow);
            pARWP->m_pBuffer->Release();
            delete pARWP;
            break;
        }

        case CONTROLLER_PROXY_REDIRECT:
        {
            IHXBuffer* pURL = (IHXBuffer*)m_pParameter;
            IHXPlayerControllerProxyRedirect* pPCPR;
            m_pPlayerController->QueryInterface(
                IID_IHXPlayerControllerProxyRedirect, (void **)&pPCPR);
            pPCPR->NetworkProxyRedirect(pURL);
            pPCPR->Release();
            pURL->Release();
            break;
        }

        default:
            DPRINTF(D_ERROR, ("Unexpected allowance controller type %d\n",
                m_Type));
    }
    delete this;
}

/*
 * NOTE: I'm not calling AddRef()/Release() on m_pPCAdviseSinkManager as
 *       that is a persisent object and should always be valid
 */

AllowanceWrapper::AllowanceWrapper
(
    Player::Session* pSession,
    Process* pProc,
    Process* pPluginProc,
    const char* pszFileSysMountPoint,
    IHXPlayerConnectionAdviseSinkManager* pPCAdviseSinkManager
) : AllowanceMgr(pProc, 0)
  , m_pSession(pSession)
  , m_pPluginProc(pPluginProc)
  , m_pPCAdviseSinkManager(pPCAdviseSinkManager)
  , m_pRequest(0)
  , m_ulRegistryID(0)
  , m_pszMountPoint(NULL)
{
    if (pszFileSysMountPoint && strlen(pszFileSysMountPoint))
    {
        m_pszMountPoint = new_string(pszFileSysMountPoint);

    }

    if (m_pSession)
    {
        m_pSession->AddRef();
    }
}

AllowanceWrapper::~AllowanceWrapper()
{
    HX_VECTOR_DELETE(m_pszMountPoint);
    HX_RELEASE(m_pSession);
    HX_RELEASE(m_pRequest);
}

STDMETHODIMP AllowanceWrapper::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)(IHXPlayerConnectionAdviseSink*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXPlayerConnectionAdviseSink))
    {
        AddRef();
        *ppvObj = (IHXPlayerConnectionAdviseSink*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXPlayerConnectionResponse))
    {
        AddRef();
        *ppvObj = (IHXPlayerConnectionResponse*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXPlayerController))
    {
        AddRef();
        *ppvObj = (IHXPlayerController*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXPlayerControllerProxyRedirect))
    {
        AddRef();
        *ppvObj = (IHXPlayerControllerProxyRedirect*)this;
        return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32) AllowanceWrapper::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32) AllowanceWrapper::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

STDMETHODIMP AllowanceWrapper::AllowanceMgrDone()
{
    // Release allowance plugin here...
    DispatchSinkCallback(SINK_RELEASE);

    return HXR_OK;
}


/* IHXPlayerConnectionAdviseSink methods */

STDMETHODIMP
AllowanceWrapper::OnConnection(IHXPlayerConnectionResponse* pResponse)
{
    /*
     * The pResponse should be NULL because this object is the end point
     * for response calls
     */

    ASSERT(!pResponse);

    AllowanceWrapperSinkCreateCallback* pCallback =
        new AllowanceWrapperSinkCreateCallback(this, m_pPCAdviseSinkManager);

    /*
     * AddRef() here to avoid being destructed while callback is outstanding
     */

    AddRef();

    m_pProc->pc->dispatchq->send(m_pProc,
                                 pCallback,
                                 m_pPluginProc->procnum());
    return HXR_OK;
}

STDMETHODIMP
AllowanceWrapper::SetPlayerController(IHXPlayerController* pPlayerController)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP
AllowanceWrapper::SetRegistryID(UINT32 ulPlayerRegistryID)
{
    m_ulRegistryID = ulPlayerRegistryID;

    return HXR_OK;
}

STDMETHODIMP
AllowanceWrapper::OnURL(IHXRequest* pRequest)
{
    m_bPlaybackAllowed = FALSE;

    HX_RELEASE(m_pRequest);
    m_pRequest = pRequest;
    m_pRequest->AddRef();

    DispatchSinkCallback(SINK_ON_URL);

    return HXR_OK;
}

STDMETHODIMP
AllowanceWrapper::OnBegin()
{
    DispatchSinkCallback(SINK_ON_BEGIN);

    return HXR_OK;
}

STDMETHODIMP
AllowanceWrapper::OnPause()
{
    DispatchSinkCallback(SINK_ON_PAUSE);

    return HXR_OK;
}

STDMETHODIMP
AllowanceWrapper::OnStop()
{
    DispatchSinkCallback(SINK_ON_STOP);

    return HXR_OK;
}

STDMETHODIMP
AllowanceWrapper::OnDone()
{
    return HXR_NOTIMPL;
}

/* IHXPlayerConnectionResponse methods */

STDMETHODIMP
AllowanceWrapper::OnConnectionDone(HX_RESULT status)
{
    HX_RESULT hresult;

    if (HXR_OK != status)
    {
        hresult = status;
        goto OnConnectionDoneError;
    }

    hresult = m_pPCAdviseSink->SetPlayerController((IHXPlayerController*)this);

    if (HXR_OK != hresult)
    {
        goto OnConnectionDoneError;
    }

    hresult = m_pPCAdviseSink->SetRegistryID(m_ulRegistryID);

    if (HXR_OK != hresult)
    {
        goto OnConnectionDoneError;
    }

    hresult = m_pPCAdviseSink->OnURL(m_pRequest);

    if (HXR_OK != hresult)
    {
        goto OnConnectionDoneError;
    }

    return HXR_OK;

OnConnectionDoneError:

    DispatchResponseCallback(RESPONSE_ON_URL_DONE, hresult);

    return HXR_OK;
}

STDMETHODIMP
AllowanceWrapper::OnURLDone(HX_RESULT status)
{
    DispatchResponseCallback(RESPONSE_ON_URL_DONE, status);

    return HXR_OK;
}

STDMETHODIMP
AllowanceWrapper::OnBeginDone(HX_RESULT status)
{
    DispatchResponseCallback(RESPONSE_ON_BEGIN_DONE, status);

    return HXR_OK;
}

STDMETHODIMP
AllowanceWrapper::OnStopDone(HX_RESULT status)
{
    DispatchResponseCallback(RESPONSE_ON_STOP_DONE, status);

    return HXR_OK;
}

STDMETHODIMP
AllowanceWrapper::OnPauseDone(HX_RESULT status)
{
    DispatchResponseCallback(RESPONSE_ON_PAUSE_DONE, status);

    return HXR_OK;
}

/* IHXPlayerController methods */

STDMETHODIMP
AllowanceWrapper::Pause()
{
    DispatchControllerCallback(CONTROLLER_PAUSE, NULL);

    return HXR_OK;
}

STDMETHODIMP
AllowanceWrapper::Resume()
{
    DispatchControllerCallback(CONTROLLER_RESUME, NULL);

    return HXR_OK;
}

STDMETHODIMP
AllowanceWrapper::Disconnect()
{
    DispatchControllerCallback(CONTROLLER_DISCONNECT, NULL);

    return HXR_OK;
}

STDMETHODIMP
AllowanceWrapper::AlertAndDisconnect(IHXBuffer* pAlert)
{
    pAlert->AddRef();
    DispatchControllerCallback(CONTROLLER_ALERT_AND_DISCONNECT, (void*)pAlert);

    return HXR_OK;
}

STDMETHODIMP
AllowanceWrapper::HostRedirect(IHXBuffer* pHost, UINT16 nPort)
{
    AllowanceHostRedirectWrapperParameters* pARWP = new
        AllowanceHostRedirectWrapperParameters;

    pARWP->m_pHost = pHost;
    pHost->AddRef();
    pARWP->m_unPort = nPort;

    DispatchControllerCallback(CONTROLLER_HOST_REDIRECT, (void*)pARWP);

    return HXR_OK;
}

STDMETHODIMP
AllowanceWrapper::NetworkRedirect(IHXBuffer* pURL, UINT32 ulSecsFromNow)
{
    AllowanceNetworkRedirectWrapperParameters* pARWP = new
        AllowanceNetworkRedirectWrapperParameters;

    pARWP->m_pBuffer = pURL;
    pURL->AddRef();
    pARWP->m_ulSecsFromNow = ulSecsFromNow;

    DispatchControllerCallback(CONTROLLER_NETWORK_REDIRECT, (void*)pARWP);

    return HXR_OK;
}

STDMETHODIMP
AllowanceWrapper::NetworkProxyRedirect(IHXBuffer* pURL)
{
    pURL->AddRef();
    DispatchControllerCallback(CONTROLLER_PROXY_REDIRECT, (void*)pURL);

    return HXR_OK;
}

STDMETHODIMP
AllowanceWrapper::Redirect(IHXBuffer* pPartialURL)
{
    pPartialURL->AddRef();
    DispatchControllerCallback(CONTROLLER_REDIRECT, (void*)pPartialURL);

    return HXR_OK;
}

void
AllowanceWrapper::DispatchSinkCallback
(
    SinkCallbackType Type
)
{
    AllowanceWrapperSinkCallback* pCallback;

    pCallback = new AllowanceWrapperSinkCallback(this,
                                                 Type);

    /*
     * AddRef() here to avoid being destructed while callback is outstanding
     */

    AddRef();

    m_pProc->pc->dispatchq->send(m_pProc,
                                 pCallback,
                                 m_pPluginProc->procnum());
}

void
AllowanceWrapper::DispatchResponseCallback
(
    ResponseCallbackType Type,
    HX_RESULT            status
)
{
    /*
     * This function should be called in the IHXPlayerConnectionAdviseSink's
     * address space
     */

    AllowanceWrapperResponseCallback* pCallback;

    pCallback = new AllowanceWrapperResponseCallback(this, Type, status);

    /*
     * AddRef() here to avoid being destructed while callback is outstanding
     */

    AddRef();

    m_pPluginProc->pc->dispatchq->send(m_pPluginProc,
                                       pCallback,
                                       m_pProc->procnum());
}

void
AllowanceWrapper::DispatchControllerCallback
(
    ControllerCallbackType Type,
    void*                  pParameter
)
{
    /*
     * This function should be called in the IHXPlayerConnectionAdviseSink's
     * address space
     */

    AllowanceWrapperControllerCallback* pCallback;

    pCallback = new AllowanceWrapperControllerCallback((IHXPlayerController*)m_pSession,
                                                       Type,
                                                       pParameter);

    m_pPluginProc->pc->dispatchq->send(m_pPluginProc,
                                       pCallback,
                                       m_pProc->procnum());
}

HX_RESULT
AllowanceWrapper::HandleOnURL()
{
    if (!m_pPCAdviseSink)
    {
        return HXR_UNEXPECTED;
    }

    /*
     * Start with the OnConnection
     */

    return m_pPCAdviseSink->OnConnection((IHXPlayerConnectionResponse*)this);
}

void
AllowanceWrapper::HandleOnURLDone(HX_RESULT status)
{
    if (HXR_OK == status)
    {
        m_bPlaybackAllowed = TRUE;
    }
    else if (HXR_NOT_AUTHORIZED == status)
    {
        // Request authentication again
    }

    m_pSession->OnURLDone(status);

    HX_RELEASE(m_pRequest);
}

STDMETHODIMP_(void)
AllowanceWrapper::PrintDebugInfo(THIS)
{
    IHXPlugin*  pPlugin = NULL;

    if (HXR_OK == m_pPCAdviseSinkManager->QueryInterface(IID_IHXPlugin, (void **)&pPlugin))
    {
        const char* pDescription;
        const char* pCopyright;
        const char* pMoreInfoURL;
        UINT32      unVersionNumber;
        BOOL        bLoadMultiple;

        pPlugin->GetPluginInfo(bLoadMultiple, pDescription, pCopyright
                                , pMoreInfoURL, unVersionNumber);

        printf("AO: SessionID: %s -- \"%s\" (%s - %s) -- "
            , (const char *)(m_pSession->m_sessionID)
            , pDescription
            , bLoadMultiple ? "Multiload" : "Non-Multiload"
            , (const char *)(m_pszMountPoint));
    }
}

STDMETHODIMP_(BOOL)
AllowanceWrapper::AcceptMountPoint(THIS_ const char* pszMountPoint, INT32 uLen)
{
    if (pszMountPoint && uLen &&
           !strncasecmp(pszMountPoint, (const char*)m_pszMountPoint, uLen))
    {
        return TRUE;
    }

    return FALSE;
}


