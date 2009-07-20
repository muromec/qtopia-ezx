/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: process_events.cpp,v 1.4 2004/07/09 18:21:00 hubbe Exp $
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

#include "platform/symbian/process_events.h"

#include <e32base.h>

class HXSymbianEventHandler : public CActive
{
public:
    HXSymbianEventHandler();
    ~HXSymbianEventHandler();

    void DoEvents(int timeoutMs);

    void RunL();
    void DoCancel();

private:
    RTimer m_timer;
    bool m_done;
};

HXSymbianEventHandler::HXSymbianEventHandler() :
    CActive(EPriorityStandard),
    m_done(false)
{
    CActiveScheduler::Add(this);
}

HXSymbianEventHandler::~HXSymbianEventHandler()
{
    Cancel();
    m_timer.Close();
}

void HXSymbianEventHandler::DoEvents(int timeoutMs)
{
    TInt err = m_timer.CreateLocal();
    
    if( KErrNone == err )
    {
	m_timer.After(iStatus, timeoutMs * 1000);
	SetActive();	

	m_done = false;

	while(!m_done)
	{
	    User::WaitForAnyRequest();
	    TInt aError;
	    CActiveScheduler::RunIfReady(aError, CActive::EPriorityIdle);
	}
    }
}

void HXSymbianEventHandler::RunL()
{
    m_done = true;
}

void HXSymbianEventHandler::DoCancel()
{
    m_timer.Cancel();
}

void SymbianProcessEvents(int timeoutMs)
{
    HXSymbianEventHandler* pEH = new HXSymbianEventHandler;

    if (pEH)
    {
	pEH->DoEvents(timeoutMs);

	delete pEH;
    }
}


