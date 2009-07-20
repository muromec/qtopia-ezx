/* ***** BEGIN LICENSE BLOCK *****
* Source last modified: $Id: chxmedpltfmsched.cpp,v 1.7 2007/04/17 12:48:26 gahluwalia Exp $
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

#include "hxcom.h"
#include "hxresult.h"
#include "hxtypes.h"
#include "hxthread.h"
#include "thrdutil.h"
#include "chxmedpltfmkicker.h"
#include "chxmedpltfmsched.h"

IMPLEMENT_COM_CREATE_FUNCS(CHXMediaPlatformScheduler)

BEGIN_INTERFACE_LIST_NOCREATE(CHXMediaPlatformScheduler)
    INTERFACE_LIST_ENTRY_SIMPLE(IHXScheduler)
    INTERFACE_LIST_ENTRY_SIMPLE(IHXScheduler2)
    INTERFACE_LIST_ENTRY_DELEGATE(IID_IHXMutex, _InternalQIMutex)
END_INTERFACE_LIST

CHXMediaPlatformScheduler::CHXMediaPlatformScheduler()
			  :m_pKicker(NULL)
			  ,m_pScheduler(NULL)
{   
}

CHXMediaPlatformScheduler::~CHXMediaPlatformScheduler(void)
{
    if (m_pKicker)
    {
	m_pKicker->RemoveScheduer(this);
    }
	HX_RELEASE(m_pScheduler);
}

HX_RESULT
CHXMediaPlatformScheduler::Init(IUnknown* pContext)
{
    m_pScheduler = new HXScheduler(pContext);
    m_pScheduler->AddRef();
    
    return HXR_OK;
}

HX_RESULT
CHXMediaPlatformScheduler::AttachKicker(CHXMediaPlatformKicker* pKicker)
{
    HX_RESULT	rc = HXR_OK;

    m_pKicker = pKicker;
    if (m_pKicker)
    {
	m_pKicker->AddScheduler(this);
    }
    else
    {
	rc = HXR_INVALID_PARAMETER;
    }

    return rc;
}

HX_RESULT
CHXMediaPlatformScheduler::DetachKicker(CHXMediaPlatformKicker* pKicker)
{
    HX_RESULT	rc = HXR_OK;

    HX_ASSERT(pKicker == m_pKicker);
    m_pKicker = NULL;

    return rc;
}

STDMETHODIMP_(CallbackHandle)
CHXMediaPlatformScheduler::RelativeEnter(IHXCallback* pCallback, ULONG32 ulTime)
{
    return m_pScheduler ? m_pScheduler->RelativeEnter(pCallback, ulTime) : NULL;
}

STDMETHODIMP_(CallbackHandle)
CHXMediaPlatformScheduler::AbsoluteEnter(IHXCallback* pCallback, HXTimeval tVal)
{
    return m_pScheduler ? m_pScheduler->AbsoluteEnter(pCallback, tVal) : NULL;
}

STDMETHODIMP 
CHXMediaPlatformScheduler::Remove(CallbackHandle Handle)
{
    return m_pScheduler ? m_pScheduler->Remove(Handle): HXR_FAILED;
}

STDMETHODIMP_(HXTimeval) 
CHXMediaPlatformScheduler::GetCurrentSchedulerTime(void)
{
    HXTimeval ret = {0, 0};
    return m_pScheduler ? m_pScheduler->GetCurrentSchedulerTime() : ret;
}

STDMETHODIMP
CHXMediaPlatformScheduler::StartScheduler()
{
    return m_pScheduler ? m_pScheduler->StartScheduler() : HXR_FAILED;
}

STDMETHODIMP
CHXMediaPlatformScheduler::StopScheduler()
{
    return m_pScheduler ? m_pScheduler->StopScheduler() : HXR_FAILED;
}

STDMETHODIMP
CHXMediaPlatformScheduler::OnTimeSync(HXBOOL bAtInterrupt)
{
    return m_pScheduler ? m_pScheduler->OnTimeSync(bAtInterrupt) : HXR_FAILED;
}

STDMETHODIMP
CHXMediaPlatformScheduler::SetInterrupt(HXBOOL bInterruptenable)
{
    return m_pScheduler ? m_pScheduler->SetInterrupt(bInterruptenable) : HXR_FAILED;
}

STDMETHODIMP_(HXBOOL)
CHXMediaPlatformScheduler::IsAtInterruptTime()
{
    return m_pScheduler ? m_pScheduler->IsAtInterruptTime() : FALSE;
}

STDMETHODIMP
CHXMediaPlatformScheduler::SetMutex(IHXMutex* pMutex)
{
    return m_pScheduler ? m_pScheduler->SetMutex(pMutex) : HXR_FAILED;
}

STDMETHODIMP
CHXMediaPlatformScheduler::GetMutex(REF(IHXMutex*) pMutex)
{
    return m_pScheduler ? m_pScheduler->GetMutex(pMutex) : HXR_FAILED;
}

STDMETHODIMP
CHXMediaPlatformScheduler::NotifyPlayState(HXBOOL bInPlayingState)
{
    return m_pScheduler ? m_pScheduler->NotifyPlayState(bInPlayingState) : HXR_FAILED;
}

STDMETHODIMP_(HXBOOL)
CHXMediaPlatformScheduler::GetNextEventDueTimeDiff(ULONG32 &ulEarliestDueTimeDiff)
{
    return m_pScheduler ? m_pScheduler->GetNextEventDueTimeDiff(ulEarliestDueTimeDiff) : HXR_FAILED;
}

STDMETHODIMP
CHXMediaPlatformScheduler::WaitForNextEvent(ULONG32 ulTimeout)
{
    return m_pScheduler ? m_pScheduler->WaitForNextEvent(ulTimeout) : HXR_FAILED;
}

STDMETHODIMP_(ULONG32)
CHXMediaPlatformScheduler::GetThreadID(void)
{
    return m_pScheduler ? m_pScheduler->GetThreadID() : 0;
}

STDMETHODIMP_(HXBOOL)
CHXMediaPlatformScheduler::AreImmediatesPending(void)
{
    return m_pScheduler ? m_pScheduler->AreImmediatesPending() : FALSE; 
}

HX_RESULT
CHXMediaPlatformScheduler::_InternalQIMutex(REFIID riid, void** ppvObj)
{
    HX_RESULT retVal;
    IHXMutex* pMutex = NULL;

    HX_ASSERT(IsEqualIID(riid, IID_IHXMutex));
    HX_ASSERT(ppvObj);

    retVal = GetMutex(pMutex);

    *ppvObj = (void*) pMutex;

    return retVal;
}

