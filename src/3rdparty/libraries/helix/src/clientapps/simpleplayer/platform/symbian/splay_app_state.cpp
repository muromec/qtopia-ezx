/* ***** BEGIN LICENSE BLOCK ***** 
 * Version: RCSL 1.0/RPSL 1.0 
 *  
 * Portions Copyright (c) 1995-2002 RealNetworks, Inc. All Rights Reserved. 
 *      
 * The contents of this file, and the files included with this file, are 
 * subject to the current version of the RealNetworks Public Source License 
 * Version 1.0 (the "RPSL") available at 
 * http://www.helixcommunity.org/content/rpsl unless you have licensed 
 * the file under the RealNetworks Community Source License Version 1.0 
 * (the "RCSL") available at http://www.helixcommunity.org/content/rcsl, 
 * in which case the RCSL will apply. You may also obtain the license terms 
 * directly from RealNetworks.  You may not use this file except in 
 * compliance with the RPSL or, if you have a valid RCSL with RealNetworks 
 * applicable to this file, the RCSL.  Please see the applicable RPSL or 
 * RCSL for the rights, obligations and limitations governing use of the 
 * contents of the file.  
 *  
 * This file is part of the Helix DNA Technology. RealNetworks is the 
 * developer of the Original Code and owns the copyrights in the portions 
 * it created. 
 *  
 * This file, and the files included with this file, is distributed and made 
 * available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER 
 * EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS ALL SUCH WARRANTIES, 
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS 
 * FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT. 
 * 
 * Technology Compatibility Kit Test Suite(s) Location: 
 *    http://www.helixcommunity.org/content/tck 
 * 
 * Contributor(s): 
 *  
 * ***** END LICENSE BLOCK ***** */ 

#include "splay_app_state.h"
#include "splay_app_ui.h"

#include "ut_cmd_line.h"

extern "C" int main(int argc, char* argv[]);

CHXSplayAppState::CHXSplayAppState(CHXSplayAppUI* pParent) :
    CActive(EPriorityStandard),
    m_pParent(pParent),
    m_running(false),
    m_pTrapHandler(User::TrapHandler()),
    m_playbackResult(-1)
{
    CActiveScheduler::Add(this);
}

CHXSplayAppState::~CHXSplayAppState()
{
    if (m_running)
    {
	m_thread.LogonCancel(iStatus);
	m_thread.Kill(-1);
    }
    Cancel();
}

bool CHXSplayAppState::IsPlaying() const
{
    return m_running;
}

void CHXSplayAppState::StartPlayer(const TDesC& cmdLine)
{
    if (!m_running && GetCommandLine(cmdLine))
    {
	TInt err = m_thread.Create(_L("SPLAY_PLAYBACK"),
				   &CHXSplayAppState::PlaybackFunc,
				   8192, // Stack size
				   KMinHeapSize,
				   16 * 1024 * 1024, // Max Heap Size
				   (TAny*)this);

	if (err == KErrNone)
	{
	    if (m_pParent)
		m_pParent->ChangeState(ESplayPlaying);

	    m_thread.Logon(iStatus);
	    SetActive();
	    m_thread.Resume();
	    m_running = true;
	}
    }
}

void CHXSplayAppState::RunL()
{
    m_running = false;

    if (m_pParent)
    {
	if (m_playbackResult >= 0)
	    m_pParent->ChangeState(ESplayComplete);
	else
	    m_pParent->ChangeState(ESplayFailed);
    }
}

void CHXSplayAppState::DoCancel()
{}

bool CHXSplayAppState::GetCommandLine(const TDesC& cmdLine)
{
    bool ret = false;

    char* pTmp = new char[cmdLine.Length() + 1];
    
    for (int i = 0; i < cmdLine.Length(); i++)
	pTmp[i] = (char)cmdLine[i];
    pTmp[cmdLine.Length()] = '\0';
    
    ret = m_cmdLine.Parse(pTmp);
    delete [] pTmp;
    
    return ret;
}

TInt CHXSplayAppState::PlaybackFunc(TAny* pAny)
{
    CHXSplayAppState* pStateObj = (CHXSplayAppState*)pAny;

    CActiveScheduler* pSched = new CActiveScheduler();

    CActiveScheduler::Install(pSched);

    User::SetTrapHandler(pStateObj->m_pTrapHandler);

    if (pStateObj->m_cmdLine.Argc())
    {
	pStateObj->m_playbackResult = main(pStateObj->m_cmdLine.Argc(), 
					   pStateObj->m_cmdLine.Argv());
    }

    CActiveScheduler::Install(0);
    delete pSched;

    return 0;
}
