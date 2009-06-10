/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: symbian_rtimer_at_imp.cpp,v 1.5 2007/07/06 20:41:57 jfinnecy Exp $
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

#include "symbian_rtimer_at_imp.h"

#include <string.h>

HXSymbianRTimerAsyncTimer::HXSymbianRTimerAsyncTimer() :
    CActive(EPriorityStandard),
    m_bInRunL(false),
    m_bDestroySelf(false)
{
    CActiveScheduler::Add(this);
}

HXSymbianRTimerAsyncTimer::~HXSymbianRTimerAsyncTimer()
{
    Cancel();
    m_timer.Close();
}

void HXSymbianRTimerAsyncTimer::DoExtraInit()
{
    TInt err = m_timer.CreateLocal();
    
    if( KErrNone == err )
    {
	m_timer.After(iStatus, GetTimeoutPeriod() * 1000);
	SetActive();	
    }
}

ULONG32 HXSymbianRTimerAsyncTimer::GetID()
{
    return (ULONG32)m_timer.Handle();
}

void HXSymbianRTimerAsyncTimer::Destroy()
{    
    if (m_bInRunL)
    {
	// We are being called from this timer callback.
	// Signal that we need to delay destruction
	m_bDestroySelf = true;
    }
    else
    {
	delete this;
    }
}

//For active object stuff.
void HXSymbianRTimerAsyncTimer::RunL()
{
    if (!m_bDestroySelf)
    {
	// Schedule next timeout
	m_timer.After(iStatus, GetTimeoutPeriod() * 1000);
	SetActive();
	m_bInRunL = true;
       OnTimeout();
       m_bInRunL = false;
    }
    else
    {
       m_bInRunL = true;
       OnTimeout();
       m_bInRunL = false;
	m_timer.Cancel();
	m_timer.Close();

	// Complete the delayed destruction
	delete this;
    }

    // Nothing should follow this line since the class may be destroyed
    // at this point
}

void HXSymbianRTimerAsyncTimer::DoCancel()
{
    m_timer.Cancel();
}





