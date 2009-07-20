/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: symbianthreads.h,v 1.10 2006/06/21 06:35:46 pankajgupta Exp $
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

#ifndef _SYMBIANTHREADSH
#define _SYMBIANTHREADSH

#include "hxthread.h"
#include <e32std.h>
#include <e32base.h>
#include "hxslist.h" // for CHXSimpleList
#include "hxmap.h"    //for CHXMapLongToObj

//=======================================================================
//
//                      HXSymbianMutex
//                   ------------------
//
// NOTE: Mutexs must be recursive.
//=======================================================================
class HXSymbianMutex : public HXMutex
{
  public:
    //Ctors and dtors.
    HXSymbianMutex();
    virtual ~HXSymbianMutex();

    //inherited methods.
    virtual HX_RESULT Lock();
    virtual HX_RESULT Unlock();
    virtual HX_RESULT Trylock();
        
protected:

    HX_RESULT _Init();
    
private:

    RCriticalSection* m_pCritSec;
    HXBOOL              m_bInited;

    //Symbian mutexes, if that is what you want to call them, are
    //not recursive. We need them to be so we roll our own.
    RCriticalSection* m_pCritSecLck;
    ULONG32           m_ulOwnerThread;
    ULONG32           m_ulLockCount;

    //Prevent unintentional ctors
    HXSymbianMutex( const HXSymbianMutex& ); //copy ctor
    HXSymbianMutex& operator=(const HXSymbianMutex& ); //assignment operator.
};


//=======================================================================
//
//                      HXSymbianEvent
//                   ------------------
//
//=======================================================================
class HXSymbianEvent : public HXEvent
{
public:
    
    HXSymbianEvent(const char* pEventName = NULL, HXBOOL bManualReset=TRUE);
    virtual ~HXSymbianEvent();
    
    virtual HX_RESULT SignalEvent();
    virtual HX_RESULT ResetEvent();
    virtual void*     GetEventHandle();
    virtual HX_RESULT Wait( UINT32 uTimeoutPeriod = ALLFS );

protected:

private:

    HXBOOL            m_bIsManualReset;
    HXBOOL            m_bEventIsSet;
    HXSymbianMutex* m_pCondLock;
    RSemaphore*     m_pCond;
    
    //Protect unintentional ctors
    HXSymbianEvent( const HXSymbianEvent& ); //copy ctor.
    HXSymbianEvent& operator=(const HXSymbianEvent& ); //assignment oper.
};


//=======================================================================
//
//                     HXSymbsymbianisymbiananThread
//                   ------------------
//
//=======================================================================
class HXSymbianThread : public HXThread
{
  public:
    HXSymbianThread();
    virtual ~HXSymbianThread();

    static ULONG32     GetCurrentThreadID();

    virtual HX_RESULT CreateThread( void*(pExecAddr(void*)),
                                    void* pArg,
                                    ULONG32 ulCreationFlags=0);
    virtual HX_RESULT Suspend();
    virtual HX_RESULT Resume();
    virtual HX_RESULT SetPriority( UINT32 ulPriority);
    virtual HX_RESULT GetPriority( UINT32& ulPriority);
    virtual HX_RESULT YieldTimeSlice();
    virtual HX_RESULT Exit(UINT32 unExitCode);

    //This call returns the CREATED THREAD's ID, not the calling thread's.
    virtual HX_RESULT GetThreadId(UINT32& ulThreadId);

    virtual HX_RESULT PostMessage(HXThreadMessage* pMsg, void* pWindowHandle=NULL);
    virtual HX_RESULT GetMessage( HXThreadMessage* pMsg,
                                  UINT32 ulMsgFilterMin = 0, //Not used for now.
                                  UINT32 ulMsgFilterMax = 0  //Not used for now.
                                  );
    virtual HX_RESULT PeekMessageMatching( HXThreadMessage* pMsg,
                                           HXThreadMessage* pMatch,
                                           HXBOOL             bRemoveMessage = 1
                                           );
    virtual HX_RESULT PeekMessage( HXThreadMessage* pMsg,
                                   UINT32 ulMsgFilterMin = 0, //Not used for now.
                                   UINT32 ulMsgFilterMax = 0, //Not used for now.
                                   HXBOOL   bRemoveMessage = 1
                                   );
    virtual HX_RESULT DispatchMessage(HXThreadMessage* pMsg);

protected:

    typedef struct _execStruct
    {
        TThreadFunction  pfExecProc; //point to thread proc
        void*            pExecArg;
        HXGlobalManager* pGlobalManager;
    } st_execStruct;
    
    static TInt _ThreadWrapper(TAny* execStruct);

private:

    RThread*       m_pThread;
    CHXSimpleList  m_messageQue;      //Our Message Que.
    
    //Used to make GetMessage blocking and to protect access to the que.
    RSemaphore*    m_pSemMessageCount;  //our message counting semaphore
    HXMutex*       m_pmtxQue;

    //Request status used to join the thread.
    TRequestStatus m_reqStat;

    //Prevent unintentional ctors
    HXSymbianThread( const HXSymbianThread& ); //copy ctor
    HXSymbianThread& operator=(const HXSymbianThread&); //assignment operator.
};


class HXSymbianAsyncTimerImp;

class HXSymbianAsyncTimer 
{
public:

    // These two methods are the main interface into this class. It 
    // make it work just like the windows ::SetTimer and ::KillTimer
    // functions.
    static UINT32 SetTimer(IUnknown* pContext, ULONG32 ulTimeOut, IHXThread* pReceivingThread );
    static UINT32 SetTimer(IUnknown* pContext, ULONG32 ulTimeOut, TIMERPROC pfExecFunc );
    static HXBOOL KillTimer(UINT32 ulTimerID );

private:
    //Don't allow default/copy ctors or assignment oper. 
    // You can't dup this class.
    HXSymbianAsyncTimer();
    HXSymbianAsyncTimer( const HXSymbianAsyncTimer& it );
    HXSymbianAsyncTimer& operator=( const HXSymbianAsyncTimer& it );

    static HXSymbianAsyncTimerImp* CreateTimer();
    static CHXMapLongToObj& GetMapTimers();
};

#endif //_SYMBIANTHREADSH
