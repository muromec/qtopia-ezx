/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: hxwsasocketselector.cpp,v 1.4 2006/02/23 22:31:01 ping Exp $ 
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


#include "hxtypes.h"
#include "nettypes.h"
#include "hlxclib/windows.h"
#include "hxcom.h"
#include "hxmsgs.h"
#include "hxthread.h"
#include "thrdutil.h"
#include "hxnet.h" //HX_SOCK_EVENT_XXX
#include "netdrv.h"
#include "hxwsasocketselector.h"

#if !defined(HELIX_CONFIG_STATIC_WINSOCK)
#include "hxwinsocklib.h"
#endif

#include "debug.h"
#include "hxassert.h"
#include "hxtlogutil.h"
#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif									 


HXSocketSelector* HXSocketSelector::AllocInstance()
{
    return new HXWSASocketSelector();
}



HXWSASocketSelector::HXWSASocketSelector()
: m_pMsgSink(0)
, m_wsaEventSemantics(false)
{
}

HXWSASocketSelector::~HXWSASocketSelector()
{
    if(m_pMsgSink)
    {
        m_pMsgSink->RemoveHandler(HXMSG_SOCKETSELECT);
        HX_RELEASE(m_pMsgSink);
    }
}

//
// event notifications will be called on the thread 
// that calls HXWSASocketSelector::Init()
//
HX_RESULT HXWSASocketSelector::Init(IUnknown* pContext)
{
    HXLOGL3(HXLOG_NETW, "HXWSASocketSelector[%p]::Init()", this);
    
    HX_RESULT hr = HXSocketSelector::Init(pContext);
    if (FAILED(hr))
    {
        return hr;
    }
    
    hr = m_netDrvLoader.EnsureLoaded(pContext);
    if (FAILED(hr))
    {
        return hr;
    }

    HX_ASSERT(!m_pMsgSink);
     
    // Fetch the message sink associated with the thread that
    // calls this function.
    // 
    // This ensures that WSAAsyncSelect notifications will be
    // posted posted to the message queue associated with the 
    // thread that calls this function.
    hr = HXThreadMessageSink::GetThreadInstance(m_pMsgSink, m_pContext);
    if(SUCCEEDED(hr))
    {
        hr = m_pMsgSink->AddHandler(HXMSG_SOCKETSELECT, this);
    }
    
    return hr;
}


// AddSocket helper
inline
long TranlateMaskValue(UINT32 mask, UINT32 test, long transValue )
{
    if( (mask & test) == test )
    {
        return transValue;
    }
    return 0;
}

//
// Re-select an existing socket already in the select pool.
//
HX_RESULT HXWSASocketSelector::UpdateSocket(sockobj_t fd, UINT32 eventMask)
{
    HX_ASSERT(m_pMsgSink);
    HX_ASSERT(m_pMsgSink->GetTID() == HXGetCurrentThreadID());

    HX_RESULT hr = HXSocketSelector::UpdateSocket(fd, eventMask);
    if (FAILED(hr))
    {
        return hr;
    }
   
    if (eventMask & HX_SOCK_EVENT_CONNECT && !(eventMask & HX_SOCK_EVENT_WRITE))
    {
        // force write event so we user can provide Helix-semantic connect event (which
        // is synthesized based on first write event, i.e., connect means socket is
        // writeable per Helix semantics)
        eventMask |= HX_SOCK_EVENT_WRITE;
    }

    // translate HX_SOCK mask to WSAAsyncSelect mask
    long wsaMask = 0;
    wsaMask |= TranlateMaskValue(eventMask, HX_SOCK_EVENT_CONNECT,    FD_CONNECT);
    wsaMask |= TranlateMaskValue(eventMask, HX_SOCK_EVENT_ACCEPT,     FD_ACCEPT);
    wsaMask |= TranlateMaskValue(eventMask, HX_SOCK_EVENT_READ,       FD_READ);
    wsaMask |= TranlateMaskValue(eventMask, HX_SOCK_EVENT_WRITE,      FD_WRITE);
    wsaMask |= TranlateMaskValue(eventMask, HX_SOCK_EVENT_CLOSE,      FD_CLOSE);

    // register to receive async socket event notifications
    int res = WSAAsyncSelect(fd, m_pMsgSink->GetSinkHandle(), HXMSG_SOCKETSELECT, wsaMask);
    HX_ASSERT(SOCKET_ERROR != res);
    if (SOCKET_ERROR == res)
    {
        hr = HXR_FAIL;
    }
    return hr;
}

void HXWSASocketSelector::RemoveSocket(sockobj_t fd)
{
    // Note: it is possible that there are lingering event notification messages
    // for the given socket in the owning thread's message queue. (These will be ignored.)
    HXSocketSelector::RemoveSocket(fd);
    HX_ASSERT(m_pMsgSink);

    // cancel async socket event notifications (note: closesocket() has same effect)
    int res = WSAAsyncSelect(fd, m_pMsgSink->GetSinkHandle(), 0, 0);
    
    // we will get WSAENOTSOCK if socket closed (fd now invalid)
    HX_ASSERT(SOCKET_ERROR != res || WSAENOTSOCK == WSAGetLastError()); 
}

#if defined(HELIX_FEATURE_LOGLEVEL_3) || defined(HELIX_FEATURE_LOGLEVEL_ALL)
//XXXLCM move
#define MAKE_ENTRY(x) {x, #x}
#define ARRAY_COUNT(x) (sizeof(x)/sizeof(x[0]))

static const char* GetSockEventString(UINT32 val)
{
    static struct
    {
        UINT32 key;
        const char* pszDesc;
    }
    const table[] =
    {
        MAKE_ENTRY(HX_SOCK_EVENT_NONE),
	MAKE_ENTRY(HX_SOCK_EVENT_WRITE),
	MAKE_ENTRY(HX_SOCK_EVENT_READ),
	MAKE_ENTRY(HX_SOCK_EVENT_CONNECT),
	MAKE_ENTRY(HX_SOCK_EVENT_CLOSE),
	MAKE_ENTRY(HX_SOCK_EVENT_ACCEPT)
    };

    for(INT32 idx = 0; idx < ARRAY_COUNT(table); ++idx)
    {
        if(val == table[idx].key)
        {
            return table[idx].pszDesc;
        }
    }
    return "HX_SOCK_???";
}
static const char* GetWSAEventString(UINT32 val)
{
    static struct
    {
        UINT32 key;
        const char* pszDesc;
    }
    const table[] =
    {
	MAKE_ENTRY(FD_WRITE),
	MAKE_ENTRY(FD_READ),
	MAKE_ENTRY(FD_CONNECT),
	MAKE_ENTRY(FD_CLOSE),
	MAKE_ENTRY(FD_ACCEPT)
    };

    for (INT32 idx = 0; idx < ARRAY_COUNT(table); ++idx)
    {
        if (val == table[idx].key)
        {
            return table[idx].pszDesc;
        }
    }
    return "FD_???";
}
static const char* GetWSAErrorString(UINT16 val)
{
    static struct
    {
        UINT16 key;
        const char* pszDesc;
    }
    const table[] =
    {
	MAKE_ENTRY(WSAEAFNOSUPPORT),
	MAKE_ENTRY(WSAECONNREFUSED),
	MAKE_ENTRY(WSAENETUNREACH),
	MAKE_ENTRY(WSAEFAULT),
	MAKE_ENTRY(WSAEINVAL),
        MAKE_ENTRY(WSAEISCONN),
	MAKE_ENTRY(WSAEMFILE),
        MAKE_ENTRY(WSAENOBUFS),
	MAKE_ENTRY(WSAENOTCONN),
        MAKE_ENTRY(WSAETIMEDOUT),
	MAKE_ENTRY(WSAENETDOWN),
        MAKE_ENTRY(WSAECONNRESET),
	MAKE_ENTRY(WSAECONNABORTED)
    };

    if (0 == val)
    {
        return "(no err)";
    }

    for (INT32 idx = 0; idx < ARRAY_COUNT(table); ++idx)
    {
        if (val == table[idx].key)
        {
            return table[idx].pszDesc;
        }
    }
    return "WSA_???";
}
#else

inline const char* GetSockEventString(UINT32 val) { return "";}
inline static const char* GetWSAEventString(UINT32 val) { return "";}
inline static const char* GetWSAErrorString(UINT16 val) { return "";}

#endif

HX_RESULT TranslateWSAAsyncSelectError(UINT16 wsaError)
{
    switch (wsaError)
    {
    case 0:                 return HXR_OK;
    case WSAEAFNOSUPPORT:   return HXR_SOCK_SOCKTNOSUPPORT;
    case WSAECONNREFUSED:   return HXR_SOCK_CONNREFUSED;
    case WSAENETUNREACH:    return HXR_SOCK_NETUNREACH;
    case WSAEFAULT:         return HXR_SOCK_FAULT;
    case WSAEINVAL:         return HXR_SOCK_INVAL;
    case WSAEISCONN:        return HXR_SOCK_ISCONN;
    case WSAEMFILE:         return HXR_SOCK_MFILE;
    case WSAENOBUFS:        return HXR_SOCK_NOBUFS;
    case WSAENOTCONN:       return HXR_SOCK_NOTCONN;
    case WSAETIMEDOUT:      return HXR_SOCK_TIMEDOUT;
    case WSAENETDOWN:       return HXR_SOCK_NETDOWN;
    case WSAECONNRESET:     return HXR_SOCK_CONNRESET;
    case WSAECONNABORTED:   return HXR_SOCK_CONNABORTED;
    }

    return HXR_FAIL;

}

UINT32 TranslateWSAAsyncSelectEvent(bool wsaEventSemantics, UINT16 wsaEvent, UINT16 err)
{
    UINT32 hxEvent = HX_SOCK_EVENT_NONE;
    switch (wsaEvent)
    {
    case FD_WRITE:
        //
        // Indicates that stream socket network buffers
        // are available for writing outgoing data. The event
        // re-enabled after a call to send() or sendto(); applies primarily to stream sockets since
        // datagram sockets are unreliable and data need not
        // be buffered.
        //
	hxEvent = HX_SOCK_EVENT_WRITE;
	break;

    case FD_READ:
        //
        // Indicates that stream/datagram socket data is present
        // in network buffers and available for reading. The event
        // is re-enabled after every call to read() or readfrom().
        //
	hxEvent = HX_SOCK_EVENT_READ;
	break;

    case FD_CONNECT:
        //
        // Indicates that stream/datagram socket has transitioned
        // to connected state after connect().
        //
        // This event occurs before the first write event. For most
        // applications you do not need to handle this; you just wait
        // for the write event.
        //
        if (wsaEventSemantics)
        {
            hxEvent = HX_SOCK_EVENT_CONNECT;
        }
        else if(err != 0)
        {
            // pass write event along with error code
            hxEvent = HX_SOCK_EVENT_WRITE;
        }
	break;

    case FD_CLOSE:
        //
        // Indicates that stream socket has transitioned to close-pending
        // state (either gracefully or abortively) from connected state
        //
        if(wsaEventSemantics)
        {
            hxEvent = HX_SOCK_EVENT_CLOSE;
        }
        else
        {
            // handler can synthesize HX_SOCK_EVENT_CLOSE based on HX_SOCK_EVENT_READ
	    hxEvent = HX_SOCK_EVENT_READ;
        }
	break;

    case FD_ACCEPT:
        //
        // Indicates arrival of incoming connection request for stream socket
        // that is in listening state.
        //
        // Call accept() to handle and re-enable the event.
        //
        // Helix synthesizes HX_SOCK_EVENT_ACCEPT based on HX_SOCK_EVENT_READ
        //
        if(wsaEventSemantics)
        {
            hxEvent = HX_SOCK_EVENT_ACCEPT;
        }
        else
        {
            // handler can synthesize HX_SOCK_EVENT_ACCEPT based on HX_SOCK_EVENT_READ
	    hxEvent = HX_SOCK_EVENT_READ;
        }
	break;
    default:
        HX_ASSERT(false);
        break;
    }

    return hxEvent;
}

//
// HXThreadMessageSink::MessageHandler
//
// Called from message window. Here we receive async HXMSG_SOCKETSELECT 
// notifications (from WSAAsyncSelect)
//
UINT32 HXWSASocketSelector::HandleMessage(const HXThreadMessage& msg)
{
    UINT32 msgid = msg.m_ulMessage;

    HX_ASSERT(HXMSG_SOCKETSELECT == msg.m_ulMessage);

    // get message data
    sockobj_t fd    = reinterpret_cast<sockobj_t>(msg.m_pParam1); //wParam;
    UINT16 event    = WSAGETSELECTEVENT(reinterpret_cast<LPARAM>(msg.m_pParam2)); //lParam
    UINT16 err      = WSAGETSELECTERROR(reinterpret_cast<WPARAM>(msg.m_pParam2)); //lParam

    HXLOGL4(HXLOG_NETW, "HXWSASocketSelector[%p]::HandleMessage(): fd = %lu; wsa event = %u; wsa err = %u", this, fd, event, err);

    // look up message handler associated with this socket
    EventHandler* pHandler = 0;
    void* pv = 0;
    if(m_pool.Lookup(reinterpret_cast<void*>(fd), pv))
    {
	pHandler = reinterpret_cast<EventHandler*>(pv);
    }
    
    if(pHandler)
    {
        UINT32 hxEvent = TranslateWSAAsyncSelectEvent(m_wsaEventSemantics, event, err);
        HX_RESULT hxResult = TranslateWSAAsyncSelectError(err);

        HXLOGL4(HXLOG_NETW, "HXWSASocketSelector[%p]::HandleMessage(): %s:%s translated as %s(= %u) for handler [%p]", this, 
            GetWSAEventString(event), GetWSAErrorString(err), GetSockEventString(hxEvent), hxEvent, pHandler);

        if (HX_SOCK_EVENT_NONE != hxEvent)
        {
            pHandler->OnSelectEvent(fd, hxEvent, hxResult);
        }
    }
    else
    {
        HXLOGL3(HXLOG_NETW, "HXWSASocketSelector[%p]::HandleMessage(): warning: no handler (lingering message?)", this);
        //HX_ASSERT(false); // most likely a lingering message posted before handler was removed
    }

    return UINT32(TRUE);
}






