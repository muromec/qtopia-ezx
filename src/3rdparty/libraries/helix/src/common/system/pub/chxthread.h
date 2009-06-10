/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: chxthread.h,v 1.7 2007/07/06 20:41:59 jfinnecy Exp $
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

#ifndef _CHXTHREAD_H_
#define _CHXTHREAD_H_

#include "hxengin.h"
#include "hxthread.h"


class CHXThread	: public IHXThread, public IHXNativeThread
{
private:
    ~CHXThread();

    INT32       m_lRefCount;
    HXThread*	m_pThread;
  protected:

    
public:

    CHXThread();

    STDMETHOD(QueryInterface)   (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)  (THIS);
    STDMETHOD_(ULONG32,Release) (THIS);

    STDMETHOD_(HXThread*, GetHXThread) (THIS)
    {
        return m_pThread;
    }
    

    // IHXThread methods
    STDMETHOD(CreateThread)	(THIS_
				void* (pExecAddr(void*)), 
				void* pArg,
				ULONG32 ulCreationFlags);
    STDMETHOD(Suspend)		(THIS);
    STDMETHOD(Resume)		(THIS);
    STDMETHOD(Cancel)		(THIS);
    STDMETHOD(Terminate)	(THIS);
    STDMETHOD(SetPriority)	(THIS_	
				UINT32 ulPriority);
    STDMETHOD(GetPriority)	(THIS_	
				UINT32& ulPriority);
    STDMETHOD(YieldTimeSlice)	(THIS);
    STDMETHOD(Exit)		(THIS_
				UINT32 ulExitCode);
    STDMETHOD(GetThreadId)	(THIS_	
				UINT32& ulThreadId);
    STDMETHOD(SetThreadName)	(THIS_	
				const char* pszName);
    STDMETHOD(GetThreadName)	(THIS_	
				const char*& pszName);
    STDMETHOD(PostMessage)	(THIS_	
				HXThreadMessage* pMsg, 
				void* pWindowHandle);
    STDMETHOD(GetMessage)	(THIS_	
				HXThreadMessage* pMsg, 
				UINT32 ulMsgFilterMix, 
				UINT32 ulMsgFilterMax);
    STDMETHOD(PeekMessage)	(THIS_	
				HXThreadMessage* pMsg, 
				UINT32 ulMsgFilterMix, 
				UINT32 ulMsgFilterMax,
				HXBOOL bRemoveMessage);
    STDMETHOD(PeekMessageMatching)(HXThreadMessage* pMsg,
                                  HXThreadMessage* pMatch,
				  HXBOOL bRemoveMessage);
    STDMETHOD(DispatchMessage)	(THIS_	
				HXThreadMessage* pMsg);
};

// Unfortunately the name CHXEvent has been used for data packets'
// wrapper class defined in client/core/pub/chxeven.h
class CHelixEvent : public IHXEvent
{
private:
    ~CHelixEvent();

    INT32       m_lRefCount;
    HXEvent*	m_pEvent;
public:

    CHelixEvent();

    STDMETHOD(QueryInterface)   (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)  (THIS);
    STDMETHOD_(ULONG32,Release) (THIS);

    // IHXEvent methods
    STDMETHOD(Init)			(THIS_
					const char* pszName,
					HXBOOL	    bManualReset);
    STDMETHOD(SignalEvent)		(THIS);
    STDMETHOD(ResetEvent)		(THIS);
    STDMETHOD_(void*, GetEventHandle)	(THIS);
    STDMETHOD(Wait)			(THIS_
					UINT32 ulTimeoutPeriod);
};

class CHXAsyncTimer : public IHXAsyncTimer
{
public:

    CHXAsyncTimer(IUnknown* pContext = NULL);
    ~CHXAsyncTimer();

    STDMETHOD(QueryInterface)   (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)  (THIS);
    STDMETHOD_(ULONG32,Release) (THIS);

    // IHXAsyncTimer methods
    STDMETHOD_(ULONG32,SetTimer) (THIS_
                                  ULONG32 ulTimeOut,
                                  IHXThread* pReceivingThread);
    STDMETHOD_(ULONG32,SetTimer) (THIS_
                                  ULONG32 ulTimeOut,
                                  TIMERPROC pfExecFunc);
    STDMETHOD(KillTimer) (THIS_ ULONG32 ulID);

private:
    INT32 m_lRefCount;
    IUnknown* m_pContext;	
};

#endif /*_CHXTHREAD_H_*/
