/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: chxkeepalive.cpp,v 1.8 2005/03/14 19:36:39 bobclark Exp $
 * 
 * Portions Copyright (c) 1995-2004 RealNetworks, Inc. All Rights Reserved.
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
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU General Public License Version 2 or later (the
 * "GPL") in which case the provisions of the GPL are applicable
 * instead of those above. If you wish to allow use of your version of
 * this file only under the terms of the GPL, and not to allow others
 * to use your version of this file under the terms of either the RPSL
 * or RCSL, indicate your decision by deleting the provisions above
 * and replace them with the notice and other provisions required by
 * the GPL. If you do not delete the provisions above, a recipient may
 * use your version of this file under the terms of any one of the
 * RPSL, the RCSL or the GPL.
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

#include "chxkeepalive.h"

#include "hxengin.h" // IHXCallback, IHXScheduler

#include "debug.h"

#define D_KEEPALIVE 0x20000000

class CHXKeepAliveImp : public IHXCallback
{
public:
    CHXKeepAliveImp(IHXScheduler* pSched, 
		    UINT32 timeoutMS, 
		    IHXCallback* pKeepAliveCB);
    ~CHXKeepAliveImp();

    void OnActivity();
    void Done();

    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)	(THIS);

    STDMETHOD_(ULONG32,Release)	(THIS);

    /*
     *  IHXCallback methods
     */

    /************************************************************************
     *	Method:
     *	    IHXCallback::Func
     *	Purpose:
     *	    This is the function that will be called when a callback is
     *	    to be executed.
     */
    STDMETHOD(Func)		(THIS);

private:
    HX_RESULT scheduleCallback();
    
    INT32 m_lRefCount;

    IHXScheduler* m_pSched;
    UINT32 m_timeoutMS;
    IHXCallback* m_pKeepAliveCB;

    HXBOOL m_bActivity;
    HXTimeval m_lastTime;
    HXBOOL m_bCBPending;
    CallbackHandle m_cbHandle;
};

CHXKeepAlive::CHXKeepAlive() :
    m_pImp(0)
{}

CHXKeepAlive::~CHXKeepAlive()
{
    reset();
}
    
HX_RESULT CHXKeepAlive::Init(IHXScheduler* pSched, 
			     UINT32 timeoutMS, 
			     IHXCallback* pKeepAliveCB)
{
    HX_RESULT res = HXR_FAILED;

    reset();

    if (pSched && (timeoutMS > 0) && pKeepAliveCB)
    {
	m_pImp = new CHXKeepAliveImp(pSched, timeoutMS, pKeepAliveCB);

	if (m_pImp)
	{
	    m_pImp->AddRef();

	    res = HXR_OK;
	}
    }

    return res;
}

void CHXKeepAlive::OnActivity()
{
    if (m_pImp)
    {
	m_pImp->OnActivity();
    }
}

void CHXKeepAlive::reset()
{
    if (m_pImp)
    {
	m_pImp->Done();

	m_pImp->Release();
	m_pImp = 0;
    }
}

CHXKeepAliveImp::CHXKeepAliveImp(IHXScheduler* pSched, 
				 UINT32 timeoutMS, 
				 IHXCallback* pKeepAliveCB) :
    m_lRefCount(0),
    m_pSched(pSched),
    m_timeoutMS(timeoutMS),
    m_pKeepAliveCB(pKeepAliveCB),
    m_bActivity(FALSE),
    m_lastTime(pSched->GetCurrentSchedulerTime()),
    m_bCBPending(FALSE),
    m_cbHandle(0)
{
    m_pSched->AddRef();
    m_pKeepAliveCB->AddRef();

    scheduleCallback();
}

CHXKeepAliveImp::~CHXKeepAliveImp()
{
    Done();
}

void CHXKeepAliveImp::OnActivity() // Called when any activity has occured
{
    DPRINTF(D_KEEPALIVE, ("CHXKeepAliveImp::OnActivity(%x)\n", (PTR_INT)this));

    m_bActivity = TRUE;
}

void CHXKeepAliveImp::Done()
{
    DPRINTF(D_KEEPALIVE, ("CHXKeepAliveImp::Done(%x)\n", (PTR_INT)this));

    if (m_bCBPending)
    {
	m_pSched->Remove(m_cbHandle);
	m_bCBPending = FALSE;
	m_cbHandle = 0;
    }
    
    HX_RELEASE(m_pSched);
    HX_RELEASE(m_pKeepAliveCB);
}

    /*
     *  IUnknown methods
     */
STDMETHODIMP CHXKeepAliveImp::QueryInterface(THIS_
					     REFIID riid,
					     void** ppvObj)
{
	QInterfaceList qiList[] =
	{
		{ GET_IIDHANDLE(IID_IUnknown), this },
		{ GET_IIDHANDLE(IID_IHXCallback), (IHXCallback*) this },
	};	
    return QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);   
}

STDMETHODIMP_(ULONG32) CHXKeepAliveImp::AddRef(THIS)
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32) CHXKeepAliveImp::Release(THIS)
{
    if(InterlockedDecrement(&m_lRefCount) > 0)
    {
	return m_lRefCount;
    }
    delete this;
    return 0;
}

    /*
     *  IHXCallback methods
     */

    /************************************************************************
     *	Method:
     *	    IHXCallback::Func
     *	Purpose:
     *	    This is the function that will be called when a callback is
     *	    to be executed.
     */
STDMETHODIMP CHXKeepAliveImp::Func(THIS)
{
    DPRINTF(D_KEEPALIVE, ("CHXKeepAliveImp::Func(%x)\n", (PTR_INT)this));

    m_bCBPending = FALSE;

    if (!m_bActivity && m_pKeepAliveCB)
    {
	DPRINTF(D_KEEPALIVE, ("CHXKeepAliveImp::Func(%x) : dispatching callback\n", (PTR_INT)this));

	m_pKeepAliveCB->Func();
    }

    m_bActivity = FALSE;

    scheduleCallback();

    return HXR_OK;
}

HX_RESULT CHXKeepAliveImp::scheduleCallback()
{
    HX_RESULT res = HXR_UNEXPECTED;

    if (!m_bCBPending && m_pSched)
    {
	// Update timeout
	m_lastTime.tv_sec += m_timeoutMS / 1000;
	m_lastTime.tv_usec += (m_timeoutMS % 1000) * 1000;

	m_cbHandle = m_pSched->AbsoluteEnter(this, m_lastTime);
	m_bCBPending = TRUE;

	res = HXR_OK;
    }

    return res;
}
