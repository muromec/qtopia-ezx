/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: symbian_async_timer_imp.cpp,v 1.7 2007/07/06 20:41:57 jfinnecy Exp $
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

#include "symbian_async_timer_imp.h"
#include "hxassert.h"
#include "hxmsgs.h"   //for HXMSG_ASYNC_TIMER message.
#include "hxtick.h"   //for GetTickCount()

HXSymbianAsyncTimerImp::HXSymbianAsyncTimerImp() :
    m_ulTimeOut(0),
    m_pReceivingThread(0),
    m_pMsg(0),
    m_pfExecFunc(0)
{}

HXSymbianAsyncTimerImp::~HXSymbianAsyncTimerImp()
{}

//This starts the message pump sending HXMSG_ASYNC_TIMER messages
//to pReceivingThread's queue every ulThreadId milliseconds.
void HXSymbianAsyncTimerImp::Init(ULONG32 ulTimeOut, 
				  IHXThread* pReceivingThread)
{
    HX_ASSERT( pReceivingThread != NULL );
    HX_ASSERT( m_ulTimeOut != 0 );

    m_ulTimeOut = ulTimeOut;
    m_pReceivingThread = pReceivingThread;
    m_pMsg = new HXThreadMessage( HXMSG_ASYNC_TIMER, 
				  (void*)m_ulTimeOut, NULL, NULL );

    // Let the derived class do any initialization it needs to
    DoExtraInit();
}

//This starts the timer and calls pfExecFunc every ulTimeOut milliseconds.
void HXSymbianAsyncTimerImp::Init( ULONG32 ulTimeOut, TIMERPROC pfExecFunc )
{
    //we need non-null pfExecFunc
    HX_ASSERT( pfExecFunc != NULL );
    HX_ASSERT( ulTimeOut != 0 );

    m_ulTimeOut = ulTimeOut;
    m_pfExecFunc = pfExecFunc;

    // Let the derived class do any initialization it needs to
    DoExtraInit();
}

ULONG32 HXSymbianAsyncTimerImp::GetTimeoutPeriod() const
{
    return m_ulTimeOut;
}

void HXSymbianAsyncTimerImp::OnTimeout()
{
    if( m_pMsg != NULL )
        m_pReceivingThread->PostMessage( m_pMsg, NULL );
    else
        m_pfExecFunc( 0, 0, GetID(), GetTickCount() );
}
