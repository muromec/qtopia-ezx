/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: CHXMMFDevSound.cpp,v 1.3 2008/03/11 05:42:29 gahluwalia Exp $
 * 
 * Copyright Notices: 
 *  
 * Portions Copyright (c) 1995-2006 RealNetworks, Inc. All Rights Reserved. 
 *  
 * Patent Notices: This file may contain technology protected by one or  
 * more of the patents listed at www.helixcommunity.org 
 *  
 * 1.   The contents of this file, and the files included with this file, 
 * are protected by copyright controlled by RealNetworks and its  
 * licensors, and made available by RealNetworks subject to the current  
 * version of the RealNetworks Public Source License (the "RPSL")  
 * available at  http://www.helixcommunity.org/content/rpsl unless  
 * you have licensed the file under the current version of the  
 * RealNetworks Community Source License (the "RCSL") available at 
 * http://www.helixcommunity.org/content/rcsl, in which case the RCSL 
 * will apply.  You may also obtain the license terms directly from 
 * RealNetworks.  You may not use this file except in compliance with 
 * the RPSL or, if you have a valid RCSL with RealNetworks applicable 
 * to this file, the RCSL.  Please see the applicable RPSL or RCSL for 
 * the rights, obligations and limitations governing use of the 
 * contents of the file. 
 *  
 * 2.  Alternatively, the contents of this file may be used under the 
 * terms of the GNU General Public License Version 2 (the 
 * "GPL") in which case the provisions of the GPL are applicable 
 * instead of those above.  Please note that RealNetworks and its  
 * licensors disclaim any implied patent license under the GPL.   
 * If you wish to allow use of your version of this file only under  
 * the terms of the GPL, and not to allow others 
 * to use your version of this file under the terms of either the RPSL 
 * or RCSL, indicate your decision by deleting Paragraph 1 above 
 * and replace them with the notice and other provisions required by 
 * the GPL. If you do not delete Paragraph 1 above, a recipient may 
 * use your version of this file under the terms of any one of the 
 * RPSL, the RCSL or the GPL. 
 *  
 * This file is part of the Helix DNA Technology.  RealNetworks is the 
 * developer of the Original Code and owns the copyrights in the 
 * portions it created.   Copying, including reproducing, storing,  
 * adapting or translating, any or all of this material other than  
 * pursuant to the license terms referred to above requires the prior  
 * written consent of RealNetworks and its licensors 
 *  
 * This file, and the files included with this file, is distributed 
 * and made available by RealNetworks on an 'AS IS' basis, WITHOUT  
 * WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED, AND REALNETWORKS  
 * AND ITS LICENSORS HEREBY DISCLAIM  ALL SUCH WARRANTIES, INCLUDING  
 * WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS  
 * FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT. 
 *  
 * Technology Compatibility Kit Test Suite(s) Location:  
 *    http://www.helixcommunity.org/content/tck 
 * 
 * Contributors: Nokia Inc
 *
 *
 * ***** END LICENSE BLOCK ***** */

#include <e32std.h>

#include "debug.h"
#include "hxtlogutil.h"
#include "CHXMMFDevSound.h"
#include "hxglobalmgr.h"
#include "hxassert.h"
#include "symbian_gm_inst.h"

//
//  Constructor
//
CHXMMFDevSound::CHXMMFDevSound()
: m_pStream(NULL)
, m_pActiveSchedulerWait(NULL)
, m_bDevInitCompleted(FALSE)	
, m_pDevSoundObserver(NULL)	
{

}

//
//  Destructor
//
CHXMMFDevSound::~CHXMMFDevSound()
{
    Close();
}

//
//  CHXMMFDevSound::Create()
//  Static function used to create and initialize the 
//  CHXMMFDevSound object
//
CHXMMFDevSound* CHXMMFDevSound::Create()
{
    CHXMMFDevSound* pDevSound = new CHXMMFDevSound();
    if(pDevSound != NULL)
    {
        TInt lRetval = KErrNone;
        lRetval = pDevSound->Init();
        if(lRetval != KErrNone)
        {
            HX_DELETE(pDevSound);
        }
    }
    return pDevSound;
}

//
//  CHXMMFDevSound::Get()
//  Fetches HXMMFDevSound from global manager
//
CHXMMFDevSound* CHXMMFDevSound::Get()
{
    CHXMMFDevSound* pRet = NULL;
    CHXMMFDevSound** pInstance = NULL;
    HXGlobalManager* pGM = HXGlobalManager::Instance();
    
    if(pGM != NULL)
    {
        pInstance = reinterpret_cast<CHXMMFDevSound**>(pGM->Get((const void*)SYMBIAN_GLOBAL_AUDIO_DEVSOUND_ID));
        if(pInstance != NULL)
        {
            pRet = *pInstance;
        }
    }
    
    HXLOGL3(HXLOG_ADEV, "CHXMMFDevSound::Get(): DevSound:%x", pRet);
    return pRet;
}

//  CHXMMFDevSound::Destroy()
//  Static function to delete CHXMMFDevSound object
//  
void CHXMMFDevSound::Destroy(void* pObj)
{
    CHXMMFDevSound* pDevSound = (CHXMMFDevSound*)pObj;
    delete pDevSound;
}

//
//  CHXMMFDevSound::Init()
//  Creates the CMMFDevSound
//
TInt CHXMMFDevSound::Init()
{
    TInt lRetval = KErrNone;
    
    TRAP(lRetval, (m_pStream = CMMFDevSound::NewL()));
    m_pActiveSchedulerWait = new CActiveSchedulerWait();
    
    if( (m_pStream == NULL) || (m_pActiveSchedulerWait == NULL) )
    {
        HX_DELETE(m_pActiveSchedulerWait);
        HX_DELETE(m_pStream);
        lRetval = KErrNoMemory;
    }
    else
    {
        lRetval = Initialize();
    }

    return lRetval;
}

//
//  CHXMMFDevSound::Close()
//  Deletes the DevSound
//
void CHXMMFDevSound::Close()
{
    HX_DELETE(m_pStream);
    HX_DELETE(m_pActiveSchedulerWait);
}

//
//  CHXMMFDevSound::Reset()
//  re-Initializes with a default observer
//
TInt CHXMMFDevSound::Initialize()
{
    TInt lRetval = KErrNotReady;

    HXLOGL2(HXLOG_ADEV, "CHXMMFDevSound::Reset()");

    if( (m_pStream != NULL) &&
        (m_pActiveSchedulerWait != NULL) )
    {
        m_bDevInitCompleted = FALSE;
        TRAP(lRetval, (m_pStream->InitializeL(*this, EMMFStatePlaying)));
        
        if((m_bDevInitCompleted == FALSE) &&
            (lRetval == KErrNone))
        {
            m_pActiveSchedulerWait->Start();
        }
    }

    return lRetval;
}

//
//  CHXMMFDevSound::RegisterObserver()
//  Updates the CHXMMFDevSound's observer
//
TInt CHXMMFDevSound::RegisterObserver(MDevSoundObserver *pObserver)
{
    m_pDevSoundObserver = pObserver;
    return KErrNone;
}

//
//  CHXMMFDevSound::ReInitialize()
//  Updates the CHXMMFDevSound's observer
//  and initializes with the new fourcc
//
TInt CHXMMFDevSound::ReInitialize(MDevSoundObserver *pObserver, TFourCC fourcc)
{
    TInt lRetval = KErrNone;
    m_pDevSoundObserver = pObserver;
    HX_ASSERT(m_pStream);
    TRAP(lRetval, (m_pStream->InitializeL(*this, fourcc, EMMFStatePlaying)));
    return lRetval;
}

//
// CHXMMFDevSound::InitializeComplete
//
// Overrrides the devSound observer to inform client
// 
//
void CHXMMFDevSound::InitializeComplete(TInt aError)
{
    m_bDevInitCompleted = TRUE;    
    HXLOGL2(HXLOG_ADEV, "CHXMMFDevSound::InitializeComplete(): err = %d", aError);
    
    if( (m_pActiveSchedulerWait != NULL) &&
        (m_pActiveSchedulerWait->IsStarted()) )
    {
        m_pActiveSchedulerWait->AsyncStop();
    }
    else
    {
        if(m_pDevSoundObserver != NULL)
        {
            m_pDevSoundObserver->InitializeComplete(aError);
        }
    }
}

//
//  CHXMMFDevSound::BufferToBeFilled
//  Passes on the call to the observer
//
void CHXMMFDevSound::BufferToBeFilled(CMMFBuffer* aBuffer)
{
    if(m_pDevSoundObserver != NULL)
    {
        m_pDevSoundObserver->BufferToBeFilled(aBuffer);
    }
    else
    {
        HXLOGL1(HXLOG_ADEV, "CHXMMFDevSound::BufferToBeFilled(): ERROR ");
    }
}

//
//  CHXMMFDevSound::PlayError
//  Passes on the call to the observer
//
void CHXMMFDevSound::PlayError(TInt aError)
{
    HXLOGL1(HXLOG_ADEV, "CHXMMFDevSound::PlayError() Err:%d Obs:%x", aError, m_pDevSoundObserver);
    if(m_pDevSoundObserver != NULL)
    {
        m_pDevSoundObserver->PlayError(aError);
    }
}
