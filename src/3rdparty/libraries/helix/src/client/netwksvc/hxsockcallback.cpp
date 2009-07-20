/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxsockcallback.cpp,v 1.7 2007/07/06 21:58:22 jfinnecy Exp $
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

#include "hxsockcallback.h"

ScheduledSocketCallback::ScheduledSocketCallback(
	SocketCallbackInterface *owner,
	HXBOOL					interruptSafe) :
     m_pSocket (owner)
    ,m_Handle (0)
    ,m_bIsCallbackPending(FALSE)
    ,m_bInterruptSafe(interruptSafe)
    ,m_Error(HXR_OK)
    ,m_lRefCount (0)
{
}

ScheduledSocketCallback::~ScheduledSocketCallback()
{
}

/*
 * IUnknown methods
 */

/////////////////////////////////////////////////////////////////////////
//	Method:
//		IUnknown::QueryInterface
//	Purpose:
//		Implement this to export the interfaces supported by your 
//		object.
//
STDMETHODIMP ScheduledSocketCallback::QueryInterface(REFIID riid, void** ppvObj)
{
    QInterfaceList qiList[] =
        {
            { GET_IIDHANDLE(IID_IHXCallback), (IHXCallback*)this },
            { GET_IIDHANDLE(IID_IHXInterruptSafe), (IHXInterruptSafe*)this },
            { GET_IIDHANDLE(IID_IUnknown), (IUnknown*)(IHXCallback*)this },
        };
    
    return ::QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
}

/////////////////////////////////////////////////////////////////////////
//	Method:
//		IUnknown::AddRef
//	Purpose:
//		Everyone usually implements this the same... feel free to use
//		this implementation.
//
STDMETHODIMP_(ULONG32) ScheduledSocketCallback::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

/////////////////////////////////////////////////////////////////////////
//	Method:
//		IUnknown::Release
//	Purpose:
//		Everyone usually implements this the same... feel free to use
//		this implementation.
//
STDMETHODIMP_(ULONG32) ScheduledSocketCallback::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
	return m_lRefCount;
    }

    delete this;
    return 0;
}


/*
 *	UDPSchedulerCallback methods
 */
STDMETHODIMP_(HXBOOL) ScheduledSocketCallback::ScheduleCallback(
	INT32 			theCommand,
	IHXScheduler*	theScheduler,
	ULONG32			theTime,
	HX_RESULT		theError)
{
	if (m_pSocket && theScheduler && !m_bIsCallbackPending)
	{
 		m_bIsCallbackPending = TRUE;
  		m_Command = theCommand;
  		m_Error = theError;
		m_Handle = theScheduler->RelativeEnter(this, theTime);
		return TRUE;
	}
	
    return FALSE;
}

/*
 *	UDPSchedulerCallback methods
 */
void ScheduledSocketCallback::Unschedule(
	IHXScheduler*	theScheduler)
{
	if (m_bIsCallbackPending && theScheduler)
	{
		theScheduler->Remove(m_Handle);
		m_bIsCallbackPending = FALSE;
		m_Handle = 0;
    }
}

STDMETHODIMP ScheduledSocketCallback::Func(void)
{
    HX_RESULT theErr = HXR_OK;
    m_Handle = 0;
    m_bIsCallbackPending = FALSE;
    if (m_pSocket)
    	theErr = m_pSocket->HandleCallback(m_Command, m_Error);

    return theErr;
}

/*
 *  IHXInterruptSafe methods 
 */
STDMETHODIMP_(HXBOOL)
ScheduledSocketCallback::IsInterruptSafe(void) 
{
    return m_bInterruptSafe; 
}


