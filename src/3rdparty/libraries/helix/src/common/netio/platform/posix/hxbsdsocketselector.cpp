/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: hxbsdsocketselector.cpp,v 1.6 2008/09/02 16:35:45 dcollins Exp $ 
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
 * terms of the GNU General Public License Version 2 (the 
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


#include "hxtypes.h"
#include "nettypes.h"
#include "hxcom.h"
#include "hxnet.h" //HX_SOCK_EVENT_XXX
#include "hxbsdsocketselector.h"
#include "hxmsgs.h" //for HXMSG message.

#include "debug.h"
#include "hxassert.h"
#include "hxtlogutil.h"
#include "hxheap.h"
#include "pckunpck.h"
#include "hxclassdispatchtasks.h"
#include "hxthreadtaskdriver.h"
#include "chxclientsocket.h"
#include "hxthread.h"
#include "thrdutil.h"
#include "microsleep.h"
#include "hxtick.h"

#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif									 


HXSocketSelector* HXSocketSelector::AllocInstance()
{
    return new HXBSDSocketSelector();
}



HXBSDSocketSelector::HXBSDSocketSelector()
    : m_pQuitEvent(NULL)
    , m_pSelectThread(NULL)
    , m_bThreadCreated(FALSE)
{
}

HXBSDSocketSelector::~HXBSDSocketSelector()
{
    CHXMapLongToObj::Iterator itr = m_SockEventPool.Begin();

    for (;itr != m_SockEventPool.End(); ++itr)
    {
        UINT32* pEventMask = (UINT32*)*itr;
        HX_DELETE(pEventMask);
        pEventMask = NULL;		
    }
	
    m_SockEventPool.RemoveAll();

    if (m_pSelectThread)
    {
        HXThreadMessage msg(HXMSG_QUIT, NULL, NULL);
        if (m_pSelectThread->PostMessage(&msg, NULL) == HXR_OK)
        {
            m_pQuitEvent->Wait(ALLFS);
        }
        m_pSelectThread->Exit(0);
        HX_RELEASE(m_pSelectThread);
    }
    HX_RELEASE(m_pQuitEvent);
}

// [Select thread]
void* HXBSDSocketSelector::ThreadProc_(void* pv)
{
    HXBSDSocketSelector* pThis = reinterpret_cast<HXBSDSocketSelector*>(pv);
    HX_ASSERT(pThis);
    return pThis->ThreadProc();  
}

void* HXBSDSocketSelector::ThreadProc()
{
    HXLOGL3(HXLOG_NETW, "HXBSDSocketSelector[%p]::ThreadProc() START", this);
	
    HXBOOL bDone = FALSE;
    microsleep(30000);
    //
    // Do the select stuff
    //
    HXThreadMessage msg;
    while (!bDone)
    {
        m_pSelectThread->PeekMessage(&msg, 0, 0,1);
        if(msg.m_ulMessage == HXMSG_QUIT)
        {
            // exit thread
            bDone = TRUE;
            HXLOGL3(HXLOG_NETW, "HXBSDSocketSelector[%p]::ThreadProc(): got HXMSG_QUIT", this);
            break;
        }
        ProcessIdle();
    }
    m_pQuitEvent->SignalEvent();
    HXLOGL3(HXLOG_NETW, "HXBSDSocketSelector[%p]::ThreadProc(): exiting...", this);
	
    return 0;
}

HX_RESULT HXBSDSocketSelector::Init(IUnknown* pContext)
{
    HXLOGL3(HXLOG_NETW, "HXBSDSocketSelector[%p]::Init()", this);
    
    HX_RESULT hr = HXSocketSelector::Init(pContext);

    if (SUCCEEDED(hr))
    {
        hr = HXThreadTaskDriver::GetInstance(m_pDriver, m_pContext);
        // create Select thread
        HX_ASSERT(!m_pSelectThread);
        hr = CreateInstanceCCF(CLSID_IHXThread, (void**)&m_pSelectThread, pContext);
        if(FAILED(hr))
        {
	      goto exit;
        }
        hr = CreateEventCCF((void**)&m_pQuitEvent, m_pContext, NULL, TRUE);
    }

exit:
    return hr;
}

// Called periodically. Do select processing.
void HXBSDSocketSelector::ProcessSelect()
{
    // XXXLCM see unix_net::process_select(); similar logic needed
}

HX_RESULT HXBSDSocketSelector::ProcessIdle()
{
    fd_set read_fds;
    HX_RESULT hxResult = HXR_OK;
    FD_ZERO(&read_fds);
    int maxfd = 0;
    int sock = -1;
    UINT32 hxEvent = HX_SOCK_EVENT_NONE;
    EventHandler* pHandler = 0;
    void* pv = 0;
    struct timeval tv;
    UINT32 ulEventMask = 0;
    CHXMapPtrToPtr::Iterator begin;
	
    // Make it to block till 60ms only
    tv.tv_sec = 0;
    tv.tv_usec = 60000;
    CHXMapPtrToPtr::Iterator end = m_pool.End();
    for(begin = m_pool.Begin(); begin != end; ++begin)
    {
        sock = reinterpret_cast<int>(begin.get_key());
        if(sock > 2)
        {
            if(sock > maxfd)
            {
                maxfd = sock;
            }
            FD_SET(sock, &read_fds);
        }
    }
    INT32 lSelectResult = -1;
    lSelectResult = select(maxfd + 1, &read_fds,NULL, NULL,&tv);
    
    if( lSelectResult > 0) 
    {
        // Got a read event

        // Check the socket that got set
        sock = -1;
        CHXMapPtrToPtr::Iterator end = m_pool.End();
        for(begin = m_pool.Begin(); begin != end; ++begin)
        {
            sock = reinterpret_cast<int>(begin.get_key());
            if(FD_ISSET(sock, &read_fds))
            {
                pHandler = NULL;
                hxEvent = HX_SOCK_EVENT_READ;
                if(m_pool.Lookup(reinterpret_cast<void*>(sock), pv))
                {
                    pHandler = reinterpret_cast<EventHandler*>(pv);
                }
                if(pHandler)
                {                
                    // Got the handler, send this event on the network thread to be called from there
                    SendWait(AllocDispatch3(pHandler, &HXSocketSelector::EventHandler::OnSelectEvent,sock,hxEvent,hxResult)); 
                }    
            }
        }
    }
	
    // lets see if we can write
    sock = -1;
    end = m_pool.End();
    for(begin = m_pool.Begin(); begin != end; ++begin)
    {
        sock = reinterpret_cast<int>(begin.get_key());
        	
        pHandler = NULL;
        // set the event with this socket
        hxEvent = HX_SOCK_EVENT_WRITE;

        void* pEventMask = NULL;

        if (m_SockEventPool.Lookup((LONG32)sock, pEventMask) && pEventMask)
        {
            ulEventMask = *((UINT32*)pEventMask);
            if(!(ulEventMask & HX_SOCK_EVENT_WRITE))
            {
                continue;
            }
        }

        if (m_pool.Lookup(reinterpret_cast<void*>(sock), pv))
        {
            pHandler = reinterpret_cast<EventHandler*>(pv);
        }

        if (pHandler)
        {
            HXLOGL3(HXLOG_NETW, "HXBSDSocketSelector[%p]::ProcessIdle() Writing on the socket", this);

            SendWait(AllocDispatch3(pHandler, &HXSocketSelector::EventHandler::OnSelectEvent,sock, hxEvent, hxResult)); 
        }
        
    }

    return hxResult;
}

HX_RESULT HXBSDSocketSelector::UpdateSocket(sockobj_t fd, UINT32 eventMask)
{
    HX_RESULT hr = HXR_OK;
    hr =  HXSocketSelector::UpdateSocket(fd, eventMask);
    if (SUCCEEDED(hr))
    {
        if (0 == m_SockEventPool.Lookup(fd))
        {
            UINT32* pEventMask = new UINT32;
            if (pEventMask)
            {
                *pEventMask = eventMask;
                if (m_SockEventPool.SetAt((LONG32)fd, pEventMask))
                {
                    hr = HXR_OK;
                }	
                else
                {
                    // Failed to insert
                    delete pEventMask;
                    pEventMask = NULL;
                }
            }
        }
        else
        {
            UINT32*& pEventMask = reinterpret_cast<UINT32*&> (m_SockEventPool[fd]);
            *pEventMask = eventMask;    
        }
		
        if(!m_bThreadCreated)
        {
            hr = m_pSelectThread->CreateThread(ThreadProc_, this, 0);
            m_pSelectThread->SetThreadName("HXBSDSocketSelector Thread");	
			
            if(SUCCEEDED(hr))
            {
                m_bThreadCreated = TRUE;
            }			
        }
    }
    return hr;
}

HX_RESULT HXBSDSocketSelector::ReEnableSocket(sockobj_t fd, UINT32 eventMask)
{
    HX_RESULT hr = HXR_OK;
    // Do some checking
    hr =  HXSocketSelector::ReEnableSocket(fd, eventMask);
    if(SUCCEEDED(hr))
    {
        HX_ASSERT(0 != m_SockEventPool.Lookup(fd));
        if(0 != m_SockEventPool.Lookup(fd))
        {
            UINT32*& pEventMask = reinterpret_cast<UINT32*&> (m_SockEventPool[fd]);
            *pEventMask = eventMask;    
        }
        else
        {
            hr = HXR_FAIL;
        }
    }
    return hr;
}

void HXBSDSocketSelector::RemoveSocket(sockobj_t fd)
{
    HXSocketSelector::RemoveSocket(fd); 

    HX_ASSERT(0 != m_SockEventPool.Lookup(fd));

    UINT32*& pEventMask = reinterpret_cast<UINT32*&> (m_SockEventPool[fd]);
    HX_DELETE(pEventMask);
    m_SockEventPool.RemoveKey(fd);
}

// send message to network thread and wait for completion
//
// for tasks that return HX_RESULT
HX_RESULT HXBSDSocketSelector::SendWait(HXClassDispatchTask<HX_RESULT>* pTask)
{
    HXLOGL4(HXLOG_NETW, "HXBSDSocketSelector[%p]::SendWait()", this);

    HX_RESULT hr = HXR_FAIL;
    HX_ASSERT(pTask);
    if (pTask)
    {
        pTask->AddRef();
        HX_ASSERT(m_pDriver);
        if (m_pDriver)
        {
            // send message to net thread and wait for completion
            hr = m_pDriver->SendAndWait(pTask);
            if (HXR_OK == hr)
            {
                // get result from completed task
                hr = pTask->GetReturnValue();
            }
        }
        else
        {
            HXLOGL4(HXLOG_NETW, "HXBSDSocketSelector[%p]::SendWait(): no driver (closed?)", this);
            hr = HXR_UNEXPECTED;
        }
        HX_RELEASE(pTask);
    }
    else
    {
        hr = HXR_OUTOFMEMORY;
    }
    return hr;
}

