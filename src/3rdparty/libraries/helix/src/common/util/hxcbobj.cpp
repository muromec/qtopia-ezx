/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxcbobj.cpp,v 1.7 2007/10/04 15:28:54 ehyche Exp $
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
#include "hxcom.h"
#include "hxcbobj.h"
#include "hxassert.h"

CHXGenericCallback::CHXGenericCallback(void* pParam, fGenericCBFunc pFunc)
 :  m_lRefCount(0)
 ,  m_fpCB(pFunc)
 ,  m_pParam(pParam)
 ,  m_PendingHandle(0)
{
}

CHXGenericCallback::~CHXGenericCallback()
{
}

STDMETHODIMP CHXGenericCallback::QueryInterface(REFIID riid, void** ppvObj)
{
    QInterfaceList qiList[] =
    {
        { GET_IIDHANDLE(IID_IHXCallback), (IHXCallback*)this },
        { GET_IIDHANDLE(IID_IUnknown), (IUnknown*)(IHXCallback*)this },
    };
    
    return ::QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
}

STDMETHODIMP_(ULONG32) CHXGenericCallback::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32) CHXGenericCallback::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
	return m_lRefCount;
    }

    delete this;
    return 0;
}


STDMETHODIMP CHXGenericCallback::Func(void)
{
    HX_ASSERT(m_fpCB && m_pParam && m_PendingHandle);

    m_PendingHandle = 0;

    if (m_fpCB)
    {
	(*m_fpCB)(m_pParam);
    }

    return HXR_OK;
}

void CHXGenericCallback::ScheduleRelative(IHXScheduler* pScheduler, UINT32 ulMs)
{
    if (pScheduler && !IsCallbackPending())
    {
        // On some platforms, the callback that we are scheduling can
        // get scheduled AND executed before RelativeEnter() returns.
        // Therefore, we set m_PendingHandle to 0xFFFFFFFF before
        // we call RelativeEnter().
        m_PendingHandle = 0xFFFFFFFF;
        // Schedule a callback and assign the handle to a local variable
        CallbackHandle hCB = pScheduler->RelativeEnter(this, ulMs);
        // If m_PendingHandle is 0, that means that the callback already
        // got scheduled and executed before RelativeEnter() returned.
        // In that case, we DON'T want to assign the return value
        // of RelativeEnter() to m_PendingHandle, since that would indicate
        // that there is a pending callback when there really isn't.
        if (m_PendingHandle != 0)
        {
            m_PendingHandle = hCB;
        }
    }
}

void CHXGenericCallback::Cancel(IHXScheduler* pScheduler)
{
    if (pScheduler && IsCallbackPending())
    {
        pScheduler->Remove(GetPendingCallback());
        CallbackCanceled();
    }
}

