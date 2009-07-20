/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: genthrd.cpp,v 1.9 2007/07/06 20:41:55 jfinnecy Exp $
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
#if defined (_WIN32) || defined (_WINDOWS)
#include "hlxclib/windows.h"
#endif

#include "hxresult.h"
#include "hxassert.h"

#include "hxthread.h"
#include "genthrd.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif									 

#define ALLFS	0xFFFFFFFF

HXGenThread::HXGenThread	(void)
{
}

HXGenThread::~HXGenThread	(void)
{
}


HX_RESULT	
HXGenThread::CreateThread(void* (pExecAddr(void*)), void* pArg, ULONG32 ulCreationFlags)
{
    return HXR_OK;
}


HX_RESULT	
HXGenThread::Suspend		(void)
{
    return HXR_OK;
}

HX_RESULT	
HXGenThread::Cancel		(void)
{
    return HXR_OK;
}

HX_RESULT	
HXGenThread::Terminate		(void)
{
    return HXR_OK;
}

HX_RESULT	
HXGenThread::Resume		(void)
{
    return HXR_OK;
}

HX_RESULT	
HXGenThread::SetPriority	(UINT32 ulPriority)
{
    return HXR_OK;
}

HX_RESULT	
HXGenThread::GetPriority	(UINT32& ulPriority)
{
    return HXR_OK;
}

HX_RESULT	
HXGenThread::YieldTimeSlice	(void)
{
    return HXR_OK;
}

HX_RESULT	
HXGenThread::Exit  (UINT32 ulExitCode)
{
    return HXR_OK;
}

HX_RESULT	
HXGenThread::GetThreadId (UINT32& ulThreadId)
{
    ulThreadId=1;
    return HXR_OK;
}

ULONG32 HXGenThread::GetCurrentThreadID()
{
    return 1;
}


HX_RESULT	
HXGenThread::PostMessage(HXThreadMessage* pMsg, void* pWindowHandle)
{
    return HXR_OK;
}

HX_RESULT	
HXGenThread::GetMessage(HXThreadMessage* pMsg, 
			UINT32 ulMsgFilterMix, 
			UINT32 ulMsgFilterMax)
{
    return HXR_OK;
}

HX_RESULT	
HXGenThread::PeekMessage(HXThreadMessage* pMsg, 
			 UINT32 ulMsgFilterMix, 
			 UINT32 ulMsgFilterMax,
			 HXBOOL bRemoveMessage)
{
    return HXR_OK;
}

HX_RESULT
HXGenThread::PeekMessageMatching(HXThreadMessage* pMsg,
                                 HXThreadMessage* pMatch,
			         HXBOOL bRemoveMessage)
{
    return HXR_OK;
}

HX_RESULT	
HXGenThread::DispatchMessage(HXThreadMessage* pMsg)
{
    return HXR_OK;
}

//===========================================================================

HXGenMutex::HXGenMutex    (void)
{
}

HXGenMutex::~HXGenMutex   (void)
{
}


HX_RESULT	
HXGenMutex::Lock	    (void)
{
    return HXR_OK;
}

HX_RESULT   
HXGenMutex::Unlock	    (void)
{
    return HXR_OK;
}

HX_RESULT   
HXGenMutex::Trylock	    (void)
{
    return HXR_NOTIMPL;
}

//===========================================================================

#ifdef _MACINTOSH
#ifdef _CARBON

HXGenMacMutex::HXGenMacMutex    (void)
{
	MPCreateCriticalRegion(&mCriticalRegion);
}

HXGenMacMutex::~HXGenMacMutex   (void)
{
	MPDeleteCriticalRegion(mCriticalRegion);
}


HX_RESULT	
HXGenMacMutex::Lock	    (void)
{
    MPEnterCriticalRegion(mCriticalRegion, kDurationForever);
    return HXR_OK;
}

HX_RESULT   
HXGenMacMutex::Unlock	    (void)
{
    MPExitCriticalRegion(mCriticalRegion);
    return HXR_OK;
}

HX_RESULT   
HXGenMacMutex::Trylock	    (void)
{
    return HXR_NOTIMPL;
}

#endif
#endif

HXGenEvent::HXGenEvent(const char* pEventName, HXBOOL bManualReset)
{
}

HXGenEvent::~HXGenEvent(void)
{
}

HX_RESULT	
HXGenEvent::SignalEvent(void)
{
    return HXR_UNEXPECTED;
}

HX_RESULT	
HXGenEvent::ResetEvent(void)
{
    return HXR_UNEXPECTED;
}

void*
HXGenEvent::GetEventHandle(void)
{
    return (void*) 0;
}

HX_RESULT	
HXGenEvent::Wait(UINT32 uTimeoutPeriod)
{
    return HXR_UNEXPECTED;
}
