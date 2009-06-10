#include "shutdown.h"
#include "chxmaplongtoobj.h"
#include "hxslist.h"
#include "proc.h"
#include "core_container.h"
#include "hxshutdown.h"
#include "streamer_container.h"
#include "dispatchq.h"
#include "inetwork_acceptor.h"
#include "_main.h"
#include "listenresp.h"
#include "servsked.h"
#include "globals.h"

#define CONFIG_ROOT                         "config"
#define CONFIG_SERVER_SHUTDOWN              "config.ServerShutdown"
#define CONFIG_DELAYED_SHUTDOWN_ENABLED     "config.ServerShutdown.DelayedShutdownEnabled"
#define CONFIG_CLIENT_DISCONNECT_INTERVAL   "config.ServerShutdown.ClientDisconnectInterval"
#define CONFIG_SHUTDOWN_PROCEED_TIME        "config.ServerShutdown.ShutdownProceedTime"
#define CONFIG_NEW_CLIENT_CONN_ALLOWED      "config.ServerShutdown.NewClientConnectionsAllowed"
#define CONFIG_LOG_PLAYER_TERMINATION       "config.ServerShutdown.LogPlayerTermination"

#define CLIENT_DISCONNECT_INTERVAL_DEFAULT  0
#define SHUTDOWN_PROCEED_TIME_DEFAULT       0

BOOL*   g_pNewConnAllow = NULL;
UINT32* g_pSecondsToShutdown = NULL;
BOOL*   g_bEnableClientConnCleanup = NULL;

UINT32* g_pClientTerminationBySelf = NULL;
UINT32* g_pClientTerminationForced = NULL;
UINT32* g_pClientIgnoreTermReq = NULL;

void killme(int code);

void
ShutdownCallback::func(Process* proc)
{  
   
    if(proc->procnum() == PROC_RM_CONTROLLER)
    {
        if(*g_pStreamerCount == 0)
        {
            if(*g_bEnableClientConnCleanup)
            {
                char szMsg[4096];
                sprintf(szMsg,
                        "Server shutdown: "
                        "%d clients terminated on their own following Server Shutdown Request, "
                        "%d clients terminated by Server Shutdown, "
                        "%d clients ignored server shutdown request",
                        *g_pClientTerminationBySelf,
                        *g_pClientTerminationForced,
                        *g_pClientIgnoreTermReq);

                proc->pc->error_handler->Report(HXLOG_WARNING,
				        HXR_OK,
				        0,
				        szMsg,
				        NULL);

                // XXXJJ we can't shutdown immediately, or the above msg can't be written to the
                // log file in time.
                DelayedShutdownCallback* pCB = new DelayedShutdownCallback();
                pCB->m_bRestart = m_bRestart;

                proc->pc->scheduler->RelativeEnter(pCB, 1000);
            }
            else
            {
		if(m_bRestart)
		{
		    perform_restart();
		}
		else
		{
		    killme(0);
		}
	    }
	}
    }
    else if(proc->pc->process_type == PROC_RM_CORE)
    {
        if(*g_bEnableClientConnCleanup)
        {
            printf("Server shutdown Initiated\n");
            printf("New Client Connection allowed: %s\n", *g_pNewConnAllow ? "Yes" : "No");
            fflush(0);

            proc->pc->error_handler->Report(HXLOG_WARNING,
				        HXR_OK,
				        0,
				        "Server Shutdown Initiated",
				        NULL);

            if((*g_pNewConnAllow) == FALSE)
            {
                CHXSimpleList* pList = ((CoreContainer *)proc->pc)->m_pListenResponseList;
                ListenResponse* pResponse = NULL;
                while(!pList->IsEmpty())
                {
                    pResponse = (ListenResponse*)pList->RemoveHead();
                    pResponse->Close();
                    pResponse->Release();
                }
            }
        }
    }
    else if(proc->pc->process_type == PTStreamer)
    {
        HXAtomicDecUINT32(g_pStreamerCount);
        if(*g_bEnableClientConnCleanup)
        {
            CServerShutdown* pShutdown = ((StreamerContainer*)proc->pc)->m_pServerShutdown;
            pShutdown->StartShutdown(m_bRestart);
        }
            
        else
        {
            //ccc is disabled.  Notify core to restart immediately.
            ShutdownCallback* cb = new ShutdownCallback;
            cb->m_bRestart = m_bRestart;
            proc->pc->dispatchq->send(proc, cb, PROC_RM_CONTROLLER);
        }
    }

    return;
}

STDMETHODIMP
DelayedShutdownCallback::Func()
{
    if(m_bRestart)
    {
        perform_restart();
    }
    else
    {
        killme(0);
    }
    return HXR_OK;
}

//CServerShutdown::CServerShutdown
CServerShutdown::CServerShutdown():
        m_bRestart(FALSE)
        ,m_state(NORMAL_STATE)
        ,m_bDelayedShutdownEnabled(FALSE)
{
    m_pClientTable = new CHXMapLongToObj;
}

//CServerShutdown::~CServerShutdown
CServerShutdown::~CServerShutdown()
{
    HX_DELETE(m_pClientTable);
    HX_RELEASE(m_pScheduler);
    HX_RELEASE(m_pRegistry);
    HX_RELEASE(m_pPropWatch);
}

//CServerShutdown::Init
void CServerShutdown::Init(Process* proc)
{
    HX_RESULT hr;
    IUnknown* pContext = (IUnknown*)proc->pc->server_context;

    m_proc = proc;

    pContext->QueryInterface(IID_IHXScheduler, (void**)&m_pScheduler);
    pContext->QueryInterface(IID_IHXRegistry, (void**)&m_pRegistry);

    hr = m_pRegistry->CreatePropWatch(m_pPropWatch);
    if (hr == HXR_OK)
    {
        hr = m_pPropWatch->Init((IHXPropWatchResponse*)this);
    }

    if(m_pRegistry->GetId(CONFIG_SERVER_SHUTDOWN) == 0)
    {
        m_pPropWatch->SetWatchByName(CONFIG_ROOT);
        *g_bEnableClientConnCleanup = FALSE;
    }

    ProcessConfig();

    *g_pSecondsToShutdown = 0;
    *g_pClientTerminationBySelf = 0;
    *g_pClientTerminationForced = 0;
    *g_pClientIgnoreTermReq = 0;
}

void CServerShutdown::ProcessConfig()
{
    if(m_pRegistry->GetId(CONFIG_SERVER_SHUTDOWN) != 0)
    {
        *g_bEnableClientConnCleanup = TRUE;
    }

    if(*g_bEnableClientConnCleanup)
    {
        m_DelayedShutdownEnabledPropID = m_pRegistry->GetId(CONFIG_DELAYED_SHUTDOWN_ENABLED);
        if (m_DelayedShutdownEnabledPropID)
        {
            INT32 ulTemp = 0;
            m_pRegistry->GetIntById(m_DelayedShutdownEnabledPropID, ulTemp);
            m_bDelayedShutdownEnabled = (ulTemp != 0);
            m_pPropWatch->SetWatchById(m_DelayedShutdownEnabledPropID);
        }
        else
        {
            m_bDelayedShutdownEnabled = FALSE;
        }
    }
    *g_bEnableClientConnCleanup = m_bDelayedShutdownEnabled;

    if(*g_bEnableClientConnCleanup)
    {
        m_ClientDisconnectIntervalPropID = m_pRegistry->GetId(CONFIG_CLIENT_DISCONNECT_INTERVAL);
        if (m_ClientDisconnectIntervalPropID)
        {
            m_pRegistry->GetIntById(m_ClientDisconnectIntervalPropID, m_ulClientDisconnectInterval);
            m_pPropWatch->SetWatchById(m_ClientDisconnectIntervalPropID);
        }
        else
        {
            m_ulClientDisconnectInterval = CLIENT_DISCONNECT_INTERVAL_DEFAULT;
        }

        //m_ulShutdownProceedTime
        m_ShutdownProceedTimePropID = m_pRegistry->GetId(CONFIG_SHUTDOWN_PROCEED_TIME);
        if (m_ShutdownProceedTimePropID)
        {
            m_pRegistry->GetIntById(m_ShutdownProceedTimePropID, m_ulShutdownProceedTime);
            m_pPropWatch->SetWatchById(m_ShutdownProceedTimePropID);
        }
        else
        {
            m_ulShutdownProceedTime = SHUTDOWN_PROCEED_TIME_DEFAULT;
        }

        //m_bNewClientConnAllowed
        m_NewClientConnectionAllowedPropID = m_pRegistry->GetId(CONFIG_NEW_CLIENT_CONN_ALLOWED);
        if (m_NewClientConnectionAllowedPropID)
        {
            INT32 ulTemp = 0;
            m_pRegistry->GetIntById(m_NewClientConnectionAllowedPropID, ulTemp);
            m_bNewClientConnAllowed = (ulTemp != 0);
            m_pPropWatch->SetWatchById(m_NewClientConnectionAllowedPropID);
        }
        else
        {
            m_bNewClientConnAllowed = FALSE;
        }

        //m_bLogPlayerTermination
        m_logPlayerTerminationPropID = m_pRegistry->GetId(CONFIG_LOG_PLAYER_TERMINATION);
        if (m_logPlayerTerminationPropID)
        {
            INT32 ulTemp = 0;
            m_pRegistry->GetIntById(m_logPlayerTerminationPropID, ulTemp);
            m_bLogPlayerTermination = (ulTemp != 0);
            m_pPropWatch->SetWatchById(m_logPlayerTerminationPropID);
        }
        else
        {
            m_bLogPlayerTermination = FALSE;
        }
    }

    *g_pNewConnAllow = m_bNewClientConnAllowed;
}

void CServerShutdown::StartShutdown(BOOL bRestart)
{
    m_pScheduler->RelativeEnter(this, m_ulClientDisconnectInterval*1000);
    m_state = SHUTDOWN_START;
    m_bRestart = bRestart;
    if(*g_pSecondsToShutdown == 0)
    {
        HXTimeval tVal = m_pScheduler->GetCurrentSchedulerTime();
        *g_pSecondsToShutdown = tVal.tv_sec + m_ulClientDisconnectInterval 
                        + m_ulShutdownProceedTime;
    }

}


/////////////////////////////////////////////////////////////////////////
//  Method:
//      IUnknown::QueryInterface
//  Purpose:
//      Implement this to export the interfaces supported by your
//      object.
//
STDMETHODIMP
CServerShutdown::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)(IHXCallback*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXCallback))
    {
    AddRef();
    *ppvObj = (IHXCallback*)this;
    return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXPropWatchResponse))
    {
    AddRef();
    *ppvObj = (IHXPropWatchResponse*)this;
    return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}   

/////////////////////////////////////////////////////////////////////////
//  Method:
//      IUnknown::AddRef
//  Purpose:
//      Everyone usually implements this the same... feel free to use
//      this implementation.
//
STDMETHODIMP_(ULONG32)
CServerShutdown::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}   

/////////////////////////////////////////////////////////////////////////
//  Method:
//      IUnknown::Release
//  Purpose:
//      Everyone usually implements this the same... feel free to use
//      this implementation.
//
STDMETHODIMP_(ULONG32)
CServerShutdown::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }
    
    delete this;
    return 0;
}   

////////////////////////////////////////////////////////////////////////
//            CServerShutdown::ModifiedProp
////////////////////////////////////////////////////////////////////////
//
// Description:        Callback for watched-property modified event.
//            We're watching the config var for the reclaim 
//            interval and age threshold.
//
// Parameters:
//
// Returns:
//
// Implementation:
//
//
//
////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CServerShutdown::ModifiedProp
(
    const UINT32      id,
    const HXPropType propType,
    const UINT32      ulParentID
)
{
    if(id == m_ClientDisconnectIntervalPropID)
    {
        m_pRegistry->GetIntById(m_ClientDisconnectIntervalPropID, m_ulClientDisconnectInterval);
    }
    else if(id == m_ShutdownProceedTimePropID)
    {
        m_pRegistry->GetIntById(m_ShutdownProceedTimePropID, m_ulShutdownProceedTime);
    }
    else if(id == m_NewClientConnectionAllowedPropID)
    {
        INT32 ulTemp = 0;
        m_pRegistry->GetIntById(m_NewClientConnectionAllowedPropID, ulTemp);
        m_bNewClientConnAllowed = (ulTemp != 0);
    }
    else if(id == m_logPlayerTerminationPropID)
    {
        INT32 ulTemp = 0;
        m_pRegistry->GetIntById(m_logPlayerTerminationPropID, ulTemp);
        m_bLogPlayerTermination = (ulTemp != 0);
    }
    else if(id == m_DelayedShutdownEnabledPropID)
    {
        INT32 ulTemp = 0;
        m_pRegistry->GetIntById(m_DelayedShutdownEnabledPropID, ulTemp);
        m_bDelayedShutdownEnabled = (ulTemp != 0);
        *g_bEnableClientConnCleanup = m_bDelayedShutdownEnabled;
    }

    *g_pNewConnAllow = m_bNewClientConnAllowed;

    return HXR_OK;
}


////////////////////////////////////////////////////////////////////////
//            CServerShutdown::AddedProp
//            CServerShutdown::DeletedProp
////////////////////////////////////////////////////////////////////////
//
// Description:        Do-nothing minimal implementations for PropWatch.
//
////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CServerShutdown::AddedProp
(
    const UINT32      id,
    const HXPropType propType,
    const UINT32      ulParentID
)
{
    ProcessConfig();
    return HXR_OK;
}

STDMETHODIMP
CServerShutdown::DeletedProp(const UINT32 id,
                   const UINT32 ulParentID)
{
    return HXR_OK;
}

//CServerShutdown::Register
HX_RESULT CServerShutdown::Register(LONG32 connId, IHXServerShutdownResponse * pSserverShutdownResponse)
{
    pSserverShutdownResponse->AddRef();
    m_pClientTable->SetAt(connId, pSserverShutdownResponse);
    return HXR_OK;
}

//CServerShutdown::Unregister
HX_RESULT CServerShutdown::Unregister(LONG32 connId)
{
    BOOL bFound = FALSE;
    IHXServerShutdownResponse * pShutdownResp = NULL;
        
    bFound = m_pClientTable->Lookup(connId, (void*&)pShutdownResp);
    if(bFound)
    {
        pShutdownResp->Release();
        m_pClientTable->Remove(connId);
    }

    if(m_state == SHUTDOWN_START)
    {
        HXAtomicIncUINT32(g_pClientTerminationBySelf);
    }
    return HXR_OK;
}

////////////////////////////////////////////////////////////////////////
//            CServerShutdown::Func
////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CServerShutdown::Func()
{
    if(m_state == SHUTDOWN_START)
    {
        IHXServerShutdownResponse *pResp = NULL;
        CHXMapLongToObj::Iterator i;
        for (i = m_pClientTable->Begin(); i != m_pClientTable->End(); ++i)
        {
            pResp = (IHXServerShutdownResponse*)(*i);
            pResp->OnShutDownStart(m_bLogPlayerTermination);
            HXAtomicIncUINT32(g_pClientTerminationForced);
        }
        m_pScheduler->RelativeEnter(this, m_ulShutdownProceedTime*1000);
        m_state = SHUTDOWN_PROCEEDING;
    }
    else if(m_state == SHUTDOWN_PROCEEDING)
    {
        IHXServerShutdownResponse *pResp = NULL;
        CHXMapLongToObj::Iterator i;
        for (i = m_pClientTable->Begin(); i != m_pClientTable->End(); ++i)
        {
            pResp = (IHXServerShutdownResponse*)(*i);
            pResp->OnShutDownEnd();
            HXAtomicIncUINT32(g_pClientIgnoreTermReq);
        }
        if(*g_pClientIgnoreTermReq)
        {
            //give 1 sec for all clients to clean up
            m_pScheduler->RelativeEnter(this, 1000);
        }
        else
        {
            m_pScheduler->RelativeEnter(this, 5);
        }
        m_state = SHUTDOWN_FINISHING;
    }
    else if(m_state == SHUTDOWN_FINISHING)
    {
        ShutdownCallback* cb = new ShutdownCallback;
        cb->m_bRestart = m_bRestart;
        m_proc->pc->dispatchq->send(m_proc, cb, PROC_RM_CONTROLLER);
    }

    return HXR_OK;
}

