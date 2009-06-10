/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: server_control.cpp,v 1.5 2005/02/08 01:57:31 jrmoore Exp $ 
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
#include "hxcomm.h"
#include "hxerror.h"
#include "servsked.h"

#include "proc.h"
#include "server_control.h"
#include "_main.h"
#include "xmlconfig.h"
#include "dispatchq.h"

extern void terminate(int);

/*
 * ReconfigCallback
 */
void
ReconfigCallback::func(Process* proc)
{
    m_pCall->ServerReconfig(m_pOrigControl);
    delete this;
}

void
ReconfigDoneCallback::func(Process* proc)
{
    m_pgsc->_EndReconfig();
    delete this;
}
/*
 * GlobalServerControl
 */
GlobalServerControl::GlobalServerControl(Process* p)
: m_paproc(p)
, m_pReconfigureResponse(0)
, m_pReconfigResponseProc(0)
, m_ulNotificationsSent(0)
, m_ulNumWants(0)
, m_ulWantNotificationsDone(0)
{
    m_pWants = new CHXSimpleList;
}

GlobalServerControl::~GlobalServerControl()
{
}

STDMETHODIMP
GlobalServerControl::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
	AddRef();
	*ppvObj = (IHXReconfigServerResponse*) this;
	return HXR_OK;
    }
    else if(IsEqualIID(riid, IID_IHXReconfigServerResponse))
    {
	AddRef();
	*ppvObj = (IHXReconfigServerResponse*)this;
	return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32)
GlobalServerControl::AddRef()
{
    return (ULONG32)-1;
}

STDMETHODIMP_(ULONG32)
GlobalServerControl::Release()
{
    return (ULONG32)-1;
}

/*
 * Reconfig steps
 * 1) ServerControl gets a ReconfigServer call, and calls this globa
 *	ReconfigServer
 * 2) Global ReconfigServer asks the config file thingy to reparse and
 *	set modified config variables.
 * 3) When config file thingy is done it calls back into ReconfigServerDone
 *	of GlobalServerControl
 * 4) GlobalServerControl sends ReconfigServer notifications to everyone who
 *	registered fro a notification through IHXWantReconfigNotifications,
 *	throught the disapatch queue to get them on the right process.
 * 5) Everytime someone finishes, they call ReconfigServerDone on this
 *	class.  If the count hits 0 and we have recieved finished notifications
 *	from everyone we notified, then we dispatch back to our original calling
 *	proc and tell the original caller we are done.
 */
HX_RESULT
GlobalServerControl::ReconfigServer(IHXReconfigServerResponse* pResp,
				    Process* proc)
{
    if (m_pReconfigureResponse)
    {
	return HXR_UNEXPECTED;
    }
    if (!m_paproc->pc->config_file)
    {
	return HXR_FAIL;
    }
    m_pReconfigResponseProc = proc;
    m_pReconfigureResponse = pResp;
    m_pReconfigureResponse->AddRef();
    HX_RESULT res;
    res = m_paproc->pc->config_file->Reconfigure(this);
    if (res != HXR_OK)
    {
	m_pReconfigureResponse->Release();
	m_pReconfigureResponse = 0;
    }
    return res;
}

STDMETHODIMP
GlobalServerControl::ReconfigServerDone(HX_RESULT res,
				IHXBuffer** pInfo,
				UINT32 ulNumInfo)
{

    if (m_ulNumWants == 0)
    {
	_EndReconfig();
	return HXR_OK;
    }
    /*
     * If m_ulReconfigNotificationsSent == 0 it
     * means that this call is coming from the config
     * so now it is time to send it to everyone who
     * registered want notification with us.
     */
    if (m_ulNotificationsSent == 0)
    {
	while (m_ulNotificationsSent < m_ulNumWants)
	{
	    m_ulNotificationsSent++;
	    CHXSimpleList::Iterator i;
	    for (i = m_pWants->Begin(); i != m_pWants->End(); ++i)
	    {
		WantNotification* pN = (WantNotification*)*i;
		ReconfigCallback* cb = new ReconfigCallback;
		cb->m_pCall = pN->m_pNot;
		cb->m_pOrigControl = pN->m_pOrigControl;
		m_paproc->pc->dispatchq->send(m_paproc, cb,
		    pN->m_pOrigControl->m_proc->procnum());
	    }
	}
    }
    else 
    {
	HX_ASSERT(0);
    }
    return HXR_OK;
}

HX_RESULT
GlobalServerControl::ReconfigServerDone(Process* proc)
{
    /*
     * Called back from the ServerControl for the process
     * of the wanter.
     */
    m_ulWantNotificationsDone++;
    if (m_ulNotificationsSent == m_ulNumWants &&
	m_ulWantNotificationsDone == m_ulNumWants)
    {
	if (proc == m_pReconfigResponseProc)
	{
	    _EndReconfig();
	}
	else
	{
	    ReconfigDoneCallback* cb = new ReconfigDoneCallback;
	    cb->m_pgsc = this;
	    proc->pc->dispatchq->send(proc, cb,
		m_pReconfigResponseProc->procnum());
	}
    }
    return HXR_OK;
}

void
GlobalServerControl::_EndReconfig()
{
    m_ulNotificationsSent = 0;
    m_ulWantNotificationsDone = 0;
    IHXReconfigServerResponse* pResp = m_pReconfigureResponse;
    m_pReconfigureResponse = 0;
    /*
     * XXX PM Get the correct parameters down there.
     */
    pResp->ReconfigServerDone(HXR_OK,
	0, 0);
    pResp->Release();
}

HX_RESULT
GlobalServerControl::WantReconfigNotification(IHXWantServerReconfigNotification*
					    pNot, ServerControl* pOrig)
{
    CHXSimpleList::Iterator i;
    WantNotification* pN;
    for (i = m_pWants->Begin(); i != m_pWants->End(); ++i)
    {
	pN = (WantNotification*)*i;
	if (pN->m_pNot == pNot)
	{
	    pN->m_pOrigControl = pOrig;
	    /*
	     * Already there.
	     */
	    return HXR_OK;
	}
    }
    pN = new WantNotification;
    pN->m_pNot = pNot;
    pN->m_pNot->AddRef();
    pN->m_pOrigControl = pOrig;
    m_pWants->AddHead((void*)pN);
    m_ulNumWants++;
    return HXR_OK;
}

HX_RESULT
GlobalServerControl::CancelReconfigNotification(IHXWantServerReconfigNotification*
					  pNot, ServerControl* pOrig)
{
    LISTPOSITION lp;
    lp = m_pWants->GetHeadPosition();
    WantNotification* pN;
    while ((pN = (WantNotification*)m_pWants->GetAt(lp)))
    {
	if (pN->m_pNot == pNot)
	{
	    m_pWants->RemoveAt(lp);
	    pN->m_pNot->Release();
	    delete pN;
	    m_ulNumWants--;
	    break;
	}
	m_pWants->GetNext(lp);
    }
    return HXR_OK;
}


/*
 * ServerControl
 */

ServerControl::ServerControl(Process* pProc)
{
    m_proc = pProc;
    m_lRefCount = 1;
}

ServerControl::~ServerControl()
{
}

STDMETHODIMP
ServerControl::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
	AddRef();
	*ppvObj = (IHXServerControl*) this;
	return HXR_OK;
    }
    else if(IsEqualIID(riid, IID_IHXServerControl))
    {
	AddRef();
	*ppvObj = (IHXServerControl*)this;
	return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXReconfigServerResponse))
    {
	AddRef();
	*ppvObj = (IHXReconfigServerResponse*)this;
	return HXR_OK;
    }
    else if(IsEqualIID(riid, IID_IHXServerControl2))
    {
	AddRef();
	*ppvObj = (IHXServerControl2*)this;
	return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXServerReconfigNotification))
    {
	AddRef();
	*ppvObj = (IHXServerReconfigNotification*)this;
	return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32)
ServerControl::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32)
ServerControl::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
	return m_lRefCount;
    }
    ASSERT(FALSE);
    delete this;
    return 0;
}

STDMETHODIMP
ServerControl::ShutdownServer(UINT32 status)
{
    if (::ShutdownServer(TRUE))
    {
	return HXR_OK;
    }
    else
    {
	return HXR_FAIL;
    }
}


STDMETHODIMP
ServerControl::RestartServer()
{
    if (::RestartServer(TRUE))
    {
	return HXR_OK;
    }
    else
    {
	return HXR_FAIL;
    }
}

STDMETHODIMP
ServerControl::ReconfigServer(IHXReconfigServerResponse* pResp)
{
    return m_proc->pc->global_server_control->ReconfigServer(pResp,
	m_proc);
}

/*
 * IHXReconfigServerResponse
 */
STDMETHODIMP
ServerControl::ReconfigServerDone(HX_RESULT res,
				IHXBuffer** pInfo,
				UINT32 ulNumInfo)
{
    return m_proc->pc->global_server_control->ReconfigServerDone(m_proc);
}

/************************************************************************
 * IHXServerReconfigNotification::WantReconfigNotification
 *
 * Purpose:
 *
 *	    Tell the server that you want reconfig notification.
 */
STDMETHODIMP
ServerControl::WantReconfigNotification(IHXWantServerReconfigNotification*
					    pResponse)
{
    return m_proc->pc->global_server_control->
	WantReconfigNotification(pResponse, this);
}

/************************************************************************
 * IHXServerReconfigNotification::CancelReconfigNotification
 *
 * Purpose:
 *
 *	    Tell the server that you no longer want reconfig notification.
 */
STDMETHODIMP
ServerControl::CancelReconfigNotification(IHXWantServerReconfigNotification*
					  pResponse)
{
    return m_proc->pc->global_server_control->
	CancelReconfigNotification(pResponse, this);
}
