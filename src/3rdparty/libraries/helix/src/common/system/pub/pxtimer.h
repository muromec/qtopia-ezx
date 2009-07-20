/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: pxtimer.h,v 1.5 2005/03/14 19:35:27 bobclark Exp $
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

#ifndef PXTIMER_H
#define PXTIMER_H

// Forward declarations
typedef _INTERFACE IHXCallback  IHXCallback;
typedef _INTERFACE IHXScheduler IHXScheduler;
class CHXBaseCountingObject;

class PXTimerResponse : public IUnknown
{
public:
    // IUnknown methods
    STDMETHOD(QueryInterface)   (THIS_ REFIID riid, void **ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)  (THIS) PURE;
    STDMETHOD_(ULONG32,Release) (THIS) PURE;

    // PXTimerResponse methods
    STDMETHOD(TimerFire) (THIS_ UINT32 ulInstance) PURE;
};

class PXTimer : public CHXBaseCountingObject,
                public IHXCallback
{
public:
    PXTimer();
    virtual ~PXTimer();

    // IUnknown methods
    STDMETHOD(QueryInterface)   (THIS_ REFIID riid, void **ppvObj);
    STDMETHOD_(ULONG32,AddRef)  (THIS);
    STDMETHOD_(ULONG32,Release) (THIS);

    // IHXCallback methods
    STDMETHOD(Func)             (THIS);

    // PXTimer methods
    HX_RESULT Init(UINT32           ulInstance,
                   IUnknown*        pContext,
                   PXTimerResponse* pResponse);
    HX_RESULT StartTimer(UINT32 ulInterval);
    void      StopTimer();
    HXBOOL      IsStarted();
protected:
    enum
    {
        kStateConstructed,
        kStateInitialized,
        kStateActive,
        kMinInterval = 33
    };

    INT32            m_lRefCount;
    UINT32           m_ulState;
    UINT32           m_ulInstance;
    UINT32           m_ulInterval;
    IHXScheduler*   m_pScheduler;
    PXTimerResponse* m_pResponse;
    UINT32           m_ulCallbackHandle;
    HXBOOL             m_bCallbackPending;
    HXBOOL             m_bInsideTimerFire;

    void Deallocate();
    HXBOOL IsCallbackPending();
    void RemovePendingCallback();
};

inline HXBOOL PXTimer::IsCallbackPending()
{
    return m_bCallbackPending;
}

inline void PXTimer::RemovePendingCallback()
{
    if (m_bCallbackPending && m_pScheduler)
    {
        m_pScheduler->Remove(m_ulCallbackHandle);
        m_bCallbackPending = FALSE;
        m_ulCallbackHandle = 0;
    }
}

inline HXBOOL PXTimer::IsStarted()
{
    return (m_ulState == kStateActive ? TRUE : FALSE);
}

#endif
