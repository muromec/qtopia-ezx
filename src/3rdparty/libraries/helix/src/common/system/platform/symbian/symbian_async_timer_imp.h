/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: symbian_async_timer_imp.h,v 1.5 2007/07/06 20:41:57 jfinnecy Exp $
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

#ifndef SYMBIAN_ASYNC_TIMER_IMP_H
#define SYMBIAN_ASYNC_TIMER_IMP_H

#include "hxtypes.h"
#include "hxthread.h"

class HXSymbianAsyncTimerImp
{
public: 
    HXSymbianAsyncTimerImp();
    virtual ~HXSymbianAsyncTimerImp();

    //This starts the message pump sending HXMSG_ASYNC_TIMER messages
    //to pReceivingThread's queue every ulThreadId milliseconds.
    void Init(ULONG32 ulTimeOut, IHXThread* pReceivingThread);

    //This starts the timer and calls pfExecFunc every ulTimeOut milliseconds.
    void Init( ULONG32 ulTimeOut, TIMERPROC pfExecFunc );

    ULONG32 GetTimeoutPeriod() const;

    void OnTimeout(); // Called when the timer fires

    virtual void DoExtraInit() = 0; // Allows derived object to do any
                                    // extra initialization. Called by
                                    // Init() functions.
    virtual ULONG32 GetID() = 0; // Returns the timer ID
    virtual void Destroy() = 0;  // Used to destroy the timer implementation

private:
    ULONG32           m_ulTimeOut;
    IHXThread*         m_pReceivingThread;
    HXThreadMessage*  m_pMsg;
    TIMERPROC         m_pfExecFunc;
};

#endif /* SYMBIAN_ASYNC_TIMER_IMP_H */
