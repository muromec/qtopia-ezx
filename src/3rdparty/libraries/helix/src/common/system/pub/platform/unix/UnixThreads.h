/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: UnixThreads.h,v 1.11 2006/08/16 17:30:13 gwright Exp $
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

#ifndef _UNIXTHREADSH
#define _UNIXTHREADSH

#include "hxthread.h"
#include "hxslist.h"  //for simplelist
#include "hxmap.h"    //for CHXMapLongToObj


//=======================================================================
//
//                      HXUnixMutex
//                   ------------------
//
// NOTE: Mutexs must be recursive.
//=======================================================================
class HXUnixMutex : public HXMutex
{
  public:
    //Ctors and dtors.
    HXUnixMutex();
    virtual ~HXUnixMutex();

    static HX_RESULT MakeMutex( HXMutex*& pMutex );
    
    //inherited methods.
    virtual HX_RESULT Lock();
    virtual HX_RESULT Unlock();
    virtual HX_RESULT Trylock();
        
protected:

    //Must be recursive
    virtual HX_RESULT _Lock() = 0;
    virtual HX_RESULT _Unlock() = 0;
    virtual HX_RESULT _TryLock() = 0;
    
    
private:

    //Prevent unintentional ctors
    HXUnixMutex( const HXUnixMutex& ); //copy ctor
    HXUnixMutex& operator=(const HXUnixMutex& ); //assignment operator.
};


//=======================================================================
//
//                      HXUnixSemaphore
//                   ------------------
//
//=======================================================================
class HXUnixSemaphore
{
  public:
    //Ctors and dtors.
    HXUnixSemaphore(UINT32 unInitialCount=0);
    virtual ~HXUnixSemaphore();


    static HX_RESULT  MakeSemaphore(HXUnixSemaphore*& pSem);
    
    virtual HX_RESULT Post();
    virtual HX_RESULT Wait();
    virtual HX_RESULT TryWait();
    virtual HX_RESULT GetValue(int* pnCount);
        
protected:
    virtual HX_RESULT _Post() = 0;
    virtual HX_RESULT _Wait() = 0;
    virtual HX_RESULT _TryWait() = 0;
    virtual HX_RESULT _GetValue(int* pnCount ) = 0;

    UINT32 m_unInitialCount;

private:
    
    //Prevent unintentional ctors
    HXUnixSemaphore( const HXUnixSemaphore& ); //copy ctor
    HXUnixSemaphore& operator=(const HXUnixSemaphore& ); //assignment operator.
};




//=======================================================================
//
//                      HXUnixCondition
//                   ---------------------
//
//=======================================================================
class HXUnixCondition
{
public:
    
    virtual ~HXUnixCondition();

    static  HX_RESULT MakeCondition( HXUnixCondition*& pCond,
                                     HXUnixMutex*&     pMutex );
    virtual HX_RESULT Wait();
    virtual HX_RESULT TimedWait( UINT32 uTimeoutPeriod = ALLFS);
    virtual HX_RESULT Broadcast();
    virtual HX_RESULT Signal();

protected:
    
    virtual HX_RESULT _Wait() = 0;
    virtual HX_RESULT _TimedWait( UINT32 uTimeoutPeriod = ALLFS) = 0;
    virtual HX_RESULT _Broadcast() = 0;
    virtual HX_RESULT _Signal() = 0;

    HXUnixCondition();

private:

    //Protect unintentional ctors
    HXUnixCondition( const HXUnixCondition& ); //copy ctor.
    HXUnixCondition& operator=(const HXUnixCondition& ); //assignment oper.
};


//=======================================================================
//
//                      HXUnixEvent
//                   ------------------
//
//=======================================================================
class HXUnixEvent : public HXEvent
{
public:
    
    HXUnixEvent(const char* pEventName = NULL, HXBOOL bManualReset=TRUE);
    virtual ~HXUnixEvent();
    
    virtual HX_RESULT SignalEvent();
    virtual HX_RESULT ResetEvent();
    virtual void*     GetEventHandle();
    virtual HX_RESULT Wait( UINT32 uTimeoutPeriod = ALLFS );

protected:

private:

    HXBOOL m_bIsManualReset;
    HXBOOL m_bEventIsSet;
    
    //These two are used for non manual resets (true cond vars)
    HXUnixMutex*     m_pCondLock;
    HXUnixCondition* m_pCond;
    
    //Protect unintentional ctors
    HXUnixEvent( const HXUnixEvent& ); //copy ctor.
    HXUnixEvent& operator=(const HXUnixEvent& ); //assignment oper.
};


//=======================================================================
//
//                      HXUnixThread
//                   ------------------
//
//=======================================================================
class HXUnixThread : public HXThread
{
  public:
    HXUnixThread();
    virtual ~HXUnixThread();

    static HX_RESULT MakeThread(HXThread*& pThread );
    static ULONG32 GetCurrentThreadID();
    
    virtual HX_RESULT CreateThread( void*(pExecAddr(void*)),
                                    void* pArg,
                                    ULONG32 ulCreationFlags=0);
    virtual HX_RESULT Suspend();
    virtual HX_RESULT Resume();
    virtual HX_RESULT Cancel();
    virtual HX_RESULT Terminate();

    virtual HX_RESULT SetPriority( UINT32 ulPriority);
    virtual HX_RESULT GetPriority( UINT32& ulPriority);
    virtual HX_RESULT YieldTimeSlice();
    virtual HX_RESULT Exit(UINT32 unExitCode);

    //This call returns the CREATED THREAD's ID, not the calling thread's.
    virtual HX_RESULT GetThreadId(UINT32& ulThreadId);

    //DEPRICATED Use GetCurrentThreadID instead.
    virtual ULONG32 Self()
    {
        return GetCurrentThreadID();
    }
            
    virtual HX_RESULT PostMessage(HXThreadMessage* pMsg, void* pWindowHandle=NULL);
    virtual HX_RESULT GetMessage( HXThreadMessage* pMsg,
                                  UINT32 ulMsgFilterMin = 0, //Not used for now.
                                  UINT32 ulMsgFilterMax = 0  //Not used for now.
                                  );
    virtual HX_RESULT PeekMessageMatching( HXThreadMessage* pMsg,
                                           HXThreadMessage* pMatch,
                                           HXBOOL           bRemoveMessage = 1
                                           );
    virtual HX_RESULT PeekMessage( HXThreadMessage* pMsg,
                                   UINT32 ulMsgFilterMin = 0, //Not used for now.
                                   UINT32 ulMsgFilterMax = 0, //Not used for now.
                                   HXBOOL   bRemoveMessage = 1
                                   );
    virtual HX_RESULT DispatchMessage(HXThreadMessage* pMsg);
    
protected:

    //
    // Pure virtuals that implement the OS specific chores.
    //

    //Create a thread and return the ID.
    virtual HX_RESULT _thread_create( ULONG32& ulThreadID,
                                      void*(pfExecFunc(void*)),
                                      void* pArg ) = 0;

    //Exit the calling thread passing back unExitCode as the return
    //value.
    virtual void _thread_exit( UINT32 unExitCode ) = 0;

    //Cancel the thread passed in.
    virtual void _thread_cancel(ULONG32 ulThreadID) = 0;

    //Wait for ulThreadID to exit and return the ret code.
    virtual ULONG32  _thread_join( ULONG32 ulThreadID ) = 0;
    
private:

    ULONG32        m_threadID;        //The created thread ID.
    CHXSimpleList  m_messageQue;      //Our Message Que.
    
    //Used to make GetMessage blocking and to protect access to the que.
    HXUnixCondition*  m_pCond;
    HXUnixMutex*      m_pCondLock;
    
    //Prevent unintentional ctors
    HXUnixThread( const HXUnixThread& ); //copy ctor
    HXUnixThread& operator=(const HXUnixThread&); //assignment operator.
};


class HXUnixAsyncTimer
{
public:

    //
    // These two methods are the main interface into this class. It 
    // make it work just like the windows ::SetTimer and ::KillTimer
    // functions.
    static UINT32 SetTimer(ULONG32 ulTimeOut, HXThread* pReceivingThread );
    static UINT32 SetTimer(ULONG32 ulTimeOut, TIMERPROC pfExecFunc );
    static HXBOOL   KillTimer(UINT32 ulTimerID );
        
protected:
    
    //This starts the message pump sending HXMSG_ASYNC_TIMER messages
    //to pReceivingThread's queue every ulThreadId milliseconds.
    HXUnixAsyncTimer( ULONG32 ulTimeOut, HXThread* pReceivingThread );

    //This starts the timer and calls pfExecFunc every ulTimeOut milliseconds.
    HXUnixAsyncTimer( ULONG32 ulTimeOut, TIMERPROC pfExecFunc );
    
    //this kills the timer and stops pumping messages/func calls.
    ~HXUnixAsyncTimer();

    //This returns that ID of this timer (threadID).
    inline ULONG32 GetID();

    //This is the actual message pump.
    static void* _ActualMessagePump(void* pArg);
    
private:

    ULONG32           m_ulTimeOut;
    HXThread*        m_pReceivingThread;
    HXUnixThread*    m_pMessagePump;
    HXThreadMessage*  m_pMsg;
    HXThreadMessage   m_msgTmp;  //This is a per-class msg to be used by the message pump.
    TIMERPROC         m_pfExecFunc;
    
    //Support for setting/killing timers by ID.
    static HXMutex*       m_pmtxMapLock;
    static CHXMapLongToObj m_mapTimers;
    
    //Don't allow default/copy ctors or assignment oper. You can't dup this class.
    HXUnixAsyncTimer();
    HXUnixAsyncTimer( const HXUnixAsyncTimer& it );
    HXUnixAsyncTimer& operator=( const HXUnixAsyncTimer& it );
};


#endif //_UNIXTHREADSH
