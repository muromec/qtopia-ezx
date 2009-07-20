/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: audio_svr_cntxt.cpp,v 1.9 2006/08/04 15:06:51 shy_ward Exp $
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
 
/*
 *  Description: 
 *
 *  This file contains the implimentation of the HXSymbianAudioServerContext
 *  class. This class specialized bsThread and provides an exection
 *  context for the Audio Server.
 *
 */
#include "hlxclib/stdlib.h"
#include "hxglobalmgr_inst.h"
#include "audio_svr_cntxt.h"
#include "audio_svr.h"
#include "hxassert.h"
#include "hxtlogutil.h"
#include <e32math.h>


// arguments passed to audio server thread entry function
struct AudioServerThreadArgs
{
    HXSymbianAudioServerContext* pCtx;
    HXGlobalManager* pGM;
};

_LIT(kHXSymbianAudioServer, "HelixAudioServer");
static const TInt kDefaultStack = 0x4000;

//
// HXSymbianAudioServerContext::ctor
// 
HXSymbianAudioServerContext::HXSymbianAudioServerContext()
    : m_pServer(0),
      m_running(false),
      m_handleOpen(false),
      m_pServerName(NULL)
{}

//
// HXSymbianAudioServerContext::dtor
//
HXSymbianAudioServerContext::~HXSymbianAudioServerContext()
{
	// make sure server is stopped
    HXLOGL3(HXLOG_ADEV, "HXSymbianAudioServerContext::~HXSymbianAudioServerContext()");
    Stop();

    m_handle.Close();
    m_startSem.Close();

    delete m_pServerName;
}



//
// HXSymbianAudioServerContext::Start
//
// This call wraps the normal thread startup call (Run()) in order to
// wait on a startup semaphore. This ensures the server thread is 
// fully started before the calling thread proceeds.
//
void HXSymbianAudioServerContext::Start(const TDesC& ServerName)
{
    HXLOGL3(HXLOG_ADEV, "HXSymbianAudioServerContext::Start(): running = %ld\n", m_running ? 1 : 0);
    if (!m_running)
    {
        m_pServerName = HBufC::New(ServerName.Size());
        if(m_pServerName)
        {
            TPtr ptr = m_pServerName->Des();
            ptr.Copy(ServerName);
            m_startSem.CreateLocal(0);
            this->Run();
            m_startSem.Wait();
        }
    }
}

//
// HXSymbianAudioServerContext::Stop
//
// 
//
void HXSymbianAudioServerContext::Stop()
{
    HXLOGL3(HXLOG_ADEV, "HXSymbianAudioServerContext::Stop(): running = %ld\n", m_running ? 1 : 0);
    if (m_running)
    {
        m_startSem.Wait();
        m_running = false;
    }
}

bool HXSymbianAudioServerContext::Running()
{
    return m_running;
}

TInt HXSymbianAudioServerContext::Run()
{
    TInt err = KErrNone;
    if (!m_running)
    {
        HXLOGL3(HXLOG_ADEV, "HXSymbianAudioServerContext::Run(): creating");
        // close handle if previously opened
        if (m_handleOpen)
        {
            m_handle.Close();
            m_handleOpen = false;
        }
        
        AudioServerThreadArgs* pArgs = new AudioServerThreadArgs();
        if(pArgs)
        {
            pArgs->pCtx = this;
            pArgs->pGM = HXGlobalManInstance::GetInstance();
            
            // Create a Unique Thread Name
            TName ThreadName;
            ThreadName.Copy(kHXSymbianAudioServer);
            ThreadName.AppendNum(Math::Random(),EHex);
            
            err = m_handle.Create(ThreadName,
                HXSymbianAudioServerContext::_Main,
                kDefaultStack, 
                /*NULL*/&User::Heap(), pArgs, EOwnerThread);
                if (KErrNone == err)
                {
                    HXLOGL3(HXLOG_ADEV, "HXSymbianAudioServerContext::Run(): created audio server thread");
                    m_handleOpen = true;
                    m_handle.Resume();
                    m_running = true;
                }
                else
                {
                    HXLOGL3(HXLOG_ADEV, "HXSymbianAudioServerContext::Run(): creation ERROR : %d", err);
                    User::Panic(_L("HXSymbianAudioServerContext::Run"), err);
                }
        }
        else
        {
            err = KErrNoMemory;
        }

    } // End of if (!m_running)
    
    return err;
}

//
// HXSymbianAudioServerContext::Main
//
// This is the entry point for the new execution context. The call to
// StartServerL() blocks until the server shuts down.
//
void* HXSymbianAudioServerContext::Main()
{
    TRAPD(leaveCode, StartServerL());

    HXLOGL3(HXLOG_ADEV, "HXSymbianAudioServerContext::Main(): leave code = %ld\n", leaveCode);

    if (leaveCode != KErrNone)
    {
        User::Panic(_L("HXSymbianAudioServerContext"), leaveCode);
    }

    return (void*)0;
}    

//
// HXSymbianAudioServerContext::StartServerL
// 
// Create and start the server object and active scheduler. This call
// runs within the new execution context.
//
void HXSymbianAudioServerContext::StartServerL()
{
    if (m_pServerName)
    {
        m_pServer = new HXSymbianAudioServer;
        m_pServer->StartL(*m_pServerName);


        // signal waiter that server has started
        m_startSem.Signal();

        CActiveScheduler::Start();

    }

    // signal waiter that server has exited
    m_startSem.Signal();

    delete m_pServer;
    m_pServer = 0;
}

TInt HXSymbianAudioServerContext::_Main (TAny* obj)
{
    void* pstatus = 0;
    AudioServerThreadArgs* pArgs = (AudioServerThreadArgs*)obj;
    HX_ASSERT(pArgs);

    // install a handle to the global manager for this thread.
    HXGlobalManInstance::SetInstance(pArgs->pGM);

    // increase the priority of the audio thread
    RThread().SetPriority(EPriorityMore);

    // allocate a cleanup trap for use on this thread
    CTrapCleanup* pCleanupTrap = CTrapCleanup::New();

    // install active scheduler
    CActiveScheduler::Install(new CActiveScheduler);

    pstatus = pArgs->pCtx->Main();

    // fall out of main body
    //
    delete CActiveScheduler::Current();
    delete pCleanupTrap;

    // clean up memory allocated by stdlib functions
    CloseSTDLIB();

    // reset thread members in case this thread is restarted
    pArgs->pCtx->m_running = false;
    delete pArgs;

    return ((TInt)pstatus);
}
