/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: ts_overlord.h,v 1.2 2003/01/23 23:42:55 damonlan Exp $ 
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

#include "proc.h"
#include "mutex.h"
#include "server_engine.h"

extern BOOL g_bForceThreadSafePlugins;

class ThreadSafeOverlord
{
public:
    inline ThreadSafeOverlord();
    virtual ~ThreadSafeOverlord(){};
    
protected:
    inline BOOL InitTSOverlord(IUnknown* pObj, Process* proc);
    enum LockState
    {
	TSO_NO_CHANGE, TSO_LOCKED, TSO_UNLOCKED
    };
    
    /*
     * These methods use the below primitives to
     * manage the locking and unlocking.  You can
     * usually just use these.
     */
    inline LockState PreMethod(UINT32 ulCondition);
    inline void PostMethod(LockState);
    
    /*
     * Use these to ensure protection
     */
    inline BOOL Protect(UINT32 ulCondition);
    inline void UnProtect(BOOL bDidProtect);
    /*
     * Use these to ensure no protection
     */
    inline BOOL RemoveProtect(UINT32 ulCondition);
    inline void ReProtect(BOOL bDidUnprotect);

    Process* m_pProc;
    UINT32 m_ulThreadSafeOptions;
};

inline
ThreadSafeOverlord::ThreadSafeOverlord()
: m_pProc(NULL)
, m_ulThreadSafeOptions(0)
{
}

inline BOOL
ThreadSafeOverlord::InitTSOverlord(IUnknown* pObj, Process* proc)
{
    m_pProc = proc;
    IHXThreadSafeMethods* pThreadSafe;
    m_ulThreadSafeOptions = 0;
    
    if (!pObj)
    {
	return FALSE;
    }

    if (HXR_OK == pObj->QueryInterface(IID_IHXThreadSafeMethods,
		(void**)&pThreadSafe))
    {
	m_ulThreadSafeOptions = pThreadSafe->IsThreadSafe();
	pThreadSafe->Release();
    }
    if (g_bForceThreadSafePlugins)
    {
	m_ulThreadSafeOptions = (UINT32)-1;
    }
    return TRUE;
}

inline ThreadSafeOverlord::LockState
ThreadSafeOverlord::PreMethod(UINT32 ulCondition)
{
    LockState lReturn = TSO_NO_CHANGE;
    
    if (Protect(ulCondition))
    {
	lReturn = TSO_LOCKED;
    }
    else if (RemoveProtect(ulCondition))
    {
	lReturn = TSO_UNLOCKED;
    }
    return lReturn;
}

inline void
ThreadSafeOverlord::PostMethod(LockState lState)
{
    if (lState == TSO_LOCKED)
    {
	UnProtect(TRUE);
    }
    else if (lState == TSO_UNLOCKED)
    {
	ReProtect(TRUE);
    }
}

inline BOOL
ThreadSafeOverlord::Protect(UINT32 ulCondition)
{
    BOOL bRet = FALSE;
    if (!(m_ulThreadSafeOptions & ulCondition) &&
	    m_pProc->pc->engine->m_bMutexProtection == FALSE)
    {
	HXMutexLock(g_pServerMainLock);
	m_pProc->pc->engine->m_bMutexProtection = TRUE;
	bRet = TRUE;
    }
    return bRet;
}

inline void
ThreadSafeOverlord::UnProtect(BOOL bDidProtect)
{
    if (bDidProtect)
    {
	m_pProc->pc->engine->m_bMutexProtection = FALSE;
	HXMutexUnlock(g_pServerMainLock);
    }
}

inline BOOL
ThreadSafeOverlord::RemoveProtect(UINT32 ulCondition)
{
    BOOL bRet = FALSE;
    if ((m_ulThreadSafeOptions & ulCondition) &&
	m_pProc->pc->engine->m_bMutexProtection == TRUE)
    {
	HXMutexUnlock(g_pServerMainLock);
	m_pProc->pc->engine->m_bMutexProtection = FALSE;
	bRet = TRUE;
    }
    return bRet;
}

inline void
ThreadSafeOverlord::ReProtect(BOOL bDidUnprotect)
{
    if (bDidUnprotect)
    {
	HXMutexLock(g_pServerMainLock);
	m_pProc->pc->engine->m_bMutexProtection = TRUE;
    }
}
