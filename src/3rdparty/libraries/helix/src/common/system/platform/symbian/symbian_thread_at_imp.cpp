/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: symbian_thread_at_imp.cpp,v 1.5 2006/02/24 01:53:35 ping Exp $
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

#include "symbian_thread_at_imp.h"
#include "hxmsgs.h"   //for HXMSG_ASYNC_TIMER message.
#include "hxtick.h" // GetTickCount()

HXSymbianThreadAsyncTimer::HXSymbianThreadAsyncTimer() :
    m_pMessagePump(0),
    m_destroySelf(false)
{}

HXSymbianThreadAsyncTimer::~HXSymbianThreadAsyncTimer()
{
    HX_DELETE( m_pMessagePump );
}

void HXSymbianThreadAsyncTimer::DoExtraInit()
{
    m_pMessagePump = new HXSymbianThread;

    m_pMessagePump->CreateThread( _ActualMessagePump, (void*)this );
}

ULONG32 HXSymbianThreadAsyncTimer::GetID()
{
    ULONG32 ulTmp=0;

    m_pMessagePump->GetThreadId(ulTmp);

    return ulTmp;
}
void HXSymbianThreadAsyncTimer::Destroy()
{
    if (m_pMessagePump)
    {
	//Tell the message pump to quit.
	HXThreadMessage msgQuit(HXMSG_QUIT, NULL, NULL);
	m_pMessagePump->PostMessage( &msgQuit, NULL );
	

	ULONG32 ulThreadID = 0;
	if ((m_pMessagePump->GetThreadId(ulThreadID) == HXR_OK) &&
	    (ulThreadID == m_pMessagePump->GetCurrentThreadID()))
	{
	    // The timer is be destroyed from the timer thread.
	    // Signal that delayed destruction needs to happen
	    m_destroySelf = true;
	}
	else
	{	    
	    // Wait for the thread to die
	    m_pMessagePump->Exit(0);
	    delete this;
	}

    }
}

void* HXSymbianThreadAsyncTimer::_ActualMessagePump(void* pArg)
{
    HXSymbianThreadAsyncTimer* pTimer = (HXSymbianThreadAsyncTimer*)pArg;

    while(1)
    {
	HXThreadMessage msgTmp;
        if( HXR_OK == pTimer->m_pMessagePump->PeekMessage(&msgTmp, 0, 0, TRUE))
        {
            //Got a message. If it is HXMSG_QUIT get our of here.
            if( msgTmp.m_ulMessage == HXMSG_QUIT )
            {
                break;
            }
        }

        User::After(pTimer->GetTimeoutPeriod() * 1000);

	pTimer->OnTimeout();
    }

    // Check for delayed destruction
    if (pTimer->m_destroySelf)
    {
	HX_DELETE(pTimer);
    }
    
    return NULL;
}

