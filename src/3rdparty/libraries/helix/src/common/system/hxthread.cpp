/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxthread.cpp,v 1.17 2006/12/06 10:35:07 gahluwalia Exp $
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
#if defined (_WIN32) || defined (_WINDOWS)
#include "hlxclib/windows.h"
#endif

#include "hxresult.h"
#include "hxassert.h"

#include "hxthread.h"
#include "genthrd.h"

#if defined (_WIN32)
#include "winthrd.h"
#endif

#if defined( _UNIX_THREADS_SUPPORTED )
#  include "UnixThreads.h"
#endif

#if defined( _SYMBIAN )
# include "hxslist.h"
# include "hxmap.h"
# include "symbianthreads.h"
#endif

#if defined (_MACINTOSH)
#ifdef THREADS_SUPPORTED
#include "platform/mac/carbthrd.h"
#else
#include "macthrd.h"
#endif
#endif

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE             
static const char HX_THIS_FILE[] = __FILE__;
#endif                                                                   

HXThread::~HXThread()
{
}

//XXXLCM consider moving static function definitions to
//      platform files to get rid of ifdefs
HX_RESULT       
HXThread::MakeThread(HXThread*& pThread)
{
    pThread = 0;
#ifdef _WIN32
    pThread = new HXWinThread;
#elif defined (_MACINTOSH)
# ifdef THREADS_SUPPORTED
    pThread = new HXCarbonThread;
# else
    pThread = new HXMacThread;
# endif
#elif defined( _UNIX_THREADS_SUPPORTED )
    HXUnixThread::MakeThread(pThread);
#elif defined _SYMBIAN
    pThread = new HXSymbianThread();
#else
    pThread = new HXGenThread;
#endif
    if (!pThread)
    {
        return HXR_OUTOFMEMORY;
    }

    return HXR_OK;
}

HX_RESULT       
HXThread::MakeStubThread(HXThread*& pThread)
{
    pThread = 0;
    pThread = new HXGenThread;
    if (!pThread)
    {
        return HXR_OUTOFMEMORY;
    }

    return HXR_OK;
}

HXMutex::~HXMutex()
{
}

HX_RESULT       
HXMutex::MakeNamedMutex(HXMutex*& pMutex, char* name)
{
    pMutex = NULL;
#ifdef _WIN32
    pMutex = new HXWinNamedMutex(name);
#else
    pMutex = new HXGenMutex;
#endif
    if (!pMutex)
    {
        return HXR_OUTOFMEMORY;
    }

    return HXR_OK;
}


HX_RESULT       
HXMutex::MakeMutex(HXMutex*& pMutex)
{
    pMutex = 0;
#ifdef _WIN32
    pMutex = new HXWinMutex;
#elif defined( _UNIX_THREADS_SUPPORTED )
    HXUnixMutex::MakeMutex(pMutex);
#elif defined _MACINTOSH

#ifdef THREADS_SUPPORTED
    pMutex = new HXCarbonMutex;
#else
    pMutex = new HXMacMutex;
#endif
#elif defined _SYMBIAN
    pMutex = new HXSymbianMutex;
#else
    pMutex = new HXGenMutex;
#endif
    if (!pMutex)
    {
        return HXR_OUTOFMEMORY;
    }

    return HXR_OK;
}

HX_RESULT       
HXMutex::MakeStubMutex(HXMutex*& pMutex)
{
    pMutex = 0;
#if defined(_CARBON) && !defined(THREADS_SUPPORTED)
        pMutex = new HXGenMacMutex;
#else
    pMutex = new HXGenMutex;
#endif
    if (!pMutex)
    {
        return HXR_OUTOFMEMORY;
    }

    return HXR_OK;
}

HXEvent::~HXEvent()
{
}


HX_RESULT       
HXEvent::MakeEvent(HXEvent*& pEvent, const char* pEventName, HXBOOL bManualReset)
{
    pEvent = 0;
#ifdef _WIN32
    pEvent = new HXWinEvent(pEventName, bManualReset);
#elif defined( _UNIX_THREADS_SUPPORTED )
    pEvent = new HXUnixEvent(pEventName, bManualReset );
#elif defined _MACINTOSH

#ifdef THREADS_SUPPORTED
    if (bManualReset)
    {
        pEvent = new HXCarbonManualEvent(pEventName);
    }
    else
    {
        pEvent = new HXCarbonEvent(pEventName, bManualReset);
    }
#else
    pEvent = new HXMacEvent(pEventName, bManualReset);
#endif
#elif defined _SYMBIAN
    pEvent = new HXSymbianEvent(pEventName, bManualReset);
#else
    pEvent = new HXGenEvent(pEventName, bManualReset);
#endif
    if (!pEvent)
    {
        return HXR_OUTOFMEMORY;
    }

    return HXR_OK;
}

HX_RESULT       
HXEvent::MakeStubEvent(HXEvent*& pEvent, const char* pEventName, HXBOOL bManualReset)
{
    pEvent = 0;
    pEvent = new HXGenEvent(pEventName, bManualReset);
    if (!pEvent)
    {
        return HXR_OUTOFMEMORY;
    }

    return HXR_OK;
}


//
// HXAsyncTimer methods.
//

//
// HXAsyncTimer mehtods.
//
UINT32 HXAsyncTimer::SetTimer(ULONG32 ulTimeOut, HXThread* pReceivingThread, IUnknown* pContext )
{
#ifdef _WIN32
    return ::SetTimer( NULL, NULL, ulTimeOut, NULL );
#elif defined( _UNIX_THREADS_SUPPORTED )
    return HXUnixAsyncTimer::SetTimer(ulTimeOut, pReceivingThread );
#elif defined _SYMBIAN
    return HXSymbianAsyncTimer::SetTimer(pContext, ulTimeOut, (IHXThread*)pReceivingThread );
#else
    return 0;
//#   error HXAsyncTimer::SetTimer not defined on this platform.
#endif        
}
    
UINT32 HXAsyncTimer::SetTimer(ULONG32 ulTimeOut, TIMERPROC pfExecFunc, IUnknown* pContext )
{
#ifdef _WIN32
    return ::SetTimer( NULL, NULL, ulTimeOut, pfExecFunc );
#elif defined( _UNIX_THREADS_SUPPORTED )
    return HXUnixAsyncTimer::SetTimer(ulTimeOut, pfExecFunc );
#elif defined _SYMBIAN
    return HXSymbianAsyncTimer::SetTimer(pContext, ulTimeOut, pfExecFunc );
#else
    return 0;
//#   error HXAsyncTimer::SetTimer not defined on this platform.
#endif        
}
    
HXBOOL HXAsyncTimer::KillTimer(UINT32 ulTimerID )
{
#ifdef _WIN32
    return ::KillTimer( NULL, ulTimerID );
#elif defined( _UNIX_THREADS_SUPPORTED )
    return HXUnixAsyncTimer::KillTimer(ulTimerID );
#elif defined _SYMBIAN
    return HXSymbianAsyncTimer::KillTimer(ulTimerID);
#else
    return TRUE;
//#   error HXAsyncTimer::KillTimer not defined on this platform.
#endif        
}

