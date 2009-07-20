/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: threngin.h,v 1.9 2006/08/16 17:28:19 gwright Exp $
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

#ifndef _THREADENGINE_
#define _THREADENGINE_

#if defined(HELIX_FEATURE_NETWORK_USE_SELECT)
#include "hxthread.h"
#endif // HELIX_FEATURE_NETWORK_USE_SELECT

#if defined(_UNIX_THREADED_NETWORK_IO)
#  include "hxcbobj.h"
#endif

class  ThreadedConn;
struct IHXCallback;
class  HXThread;
class  HXMutex;
class  CHXMapPtrToPtr;
class  HXEvent;

class ThreadEngine
{
public:
    static ThreadEngine*    GetThreadEngine(IUnknown* pContext);
    static void		    DestroyThreadEngine(void);

    IHXThread* GetNetworkThread(void)
    {
        HX_ADDREF(m_pNetworkThread);
        return m_pNetworkThread;
    };
    IHXThread* GetMainAppThread(void)
    {
        HX_ADDREF(m_pMainAppThread);
        return m_pMainAppThread;
    };
    IHXMutex*	    GetMutex(void) {return m_pMutex;};
    CHXMapPtrToPtr* GetSockMap() {return m_pSockMap;};
    HX_RESULT	    AttachSocket(ThreadedConn* pConn);
    HX_RESULT	    DetachSocket(ThreadedConn* pConn);

protected:
    ThreadEngine(IUnknown* pContext);
    ~ThreadEngine();
    void	    DoAsyncCallback(ThreadedConn* pConn, IHXCallback* pCallback);
    void	    DoNetworkIO(ThreadedConn* pConn = NULL);
#if defined(HELIX_FEATURE_NETWORK_USE_SELECT)
    ThreadedConn*   m_pLocalListener; // local loopback for msgs
    ThreadedConn*   m_pLocalReader; // rcv msg for network thread
    ThreadedConn*   m_pLocalWriter; // send msg to network thread
    HX_RESULT	    WaitforSelect();
#endif // HELIX_FEATURE_NETWORK_USE_SELECT

    friend void*    NetworkThreadMainLoop(void* pArg);

    IHXThread*		    m_pNetworkThread;
    IHXThread*		    m_pMainAppThread;
    IHXMutex*		    m_pMutex;
    CHXMapPtrToPtr*	    m_pSockMap;
    IUnknown*		    m_pContext;
    IHXEvent*		    m_pQuitEvent;
    HXBOOL		    m_bInDestructor;
    static  ThreadEngine*   m_pzThreadEngine;

#if defined(_MACINTOSH) && defined(_CARBON) && defined(THREADS_SUPPORTED)
    EventLoopTimerUPP		m_ThreadedNetworkingCarbonTimerUPP;
    EventLoopTimerRef		m_ThreadedNetworkingCarbonTimerRef;
    static void			FauxMainAppCarbonTimer(EventLoopTimerRef, void*);
#endif

#if defined(_UNIX_THREADED_NETWORK_IO)
    IHXScheduler*       m_pScheduler;
    CHXGenericCallback* m_pUnixMessageLoop;
    friend void UnixMessageLoop(void*);
    
#endif

};

class DestructEngine
{
public:
    DestructEngine() {};
    ~DestructEngine() {ThreadEngine::DestroyThreadEngine();};
};

#endif /*_THREADENGINE_*/
