/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: audsymbian.cpp,v 1.46 2008/10/19 05:13:35 gajia Exp $
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

#include <e32base.h>
#include <e32std.h>

#include "hxassert.h"
#include "hxresult.h"
#include "hxslist.h"
#include "hxcom.h"
#include "hxtick.h"
#include "hxausvc.h"
#include "ihxpckts.h"
#include "hxprefutil.h"
#include "hxtlogutil.h"

#include "CHXSymbianAudioDevice.h"

#include "audsymbian.h"
#include "hxerror.h"

static UINT32 Scale(UINT32 v, UINT32 f0, UINT32 f1, UINT32 t0, UINT32 t1);

// interval between generated calls to OnTimeSync
const UINT32 TIMESYNC_CALLBACK_INTERVAL_MS = 30;

CHXAudioDevice::CHXAudioDevice()
    : CActive(EPriorityHigh),
      m_lRefCount(0),
      m_deviceOpen(false),
      m_pDeviceResponse(NULL),
      m_pAudioStream(NULL),
      m_paused(false),
      m_uMinPlayerVolume(0),
      m_uMaxPlayerVolume(100),
      m_uMinDevVolume(0),
      m_uMaxDevVolume(100),
      m_pPrioritySettings(NULL),
      m_pContext(NULL),
      m_pErrorMessages(NULL),
	  m_bSecureOutputChanged(FALSE),
	  m_uSecureOutputChangeTime(0),
	  m_pPropWatch(NULL)
{
    CActiveScheduler::Add(this);
    TInt err = KErrNone;
    err = m_iTimer.CreateLocal();
    HX_ASSERT(KErrNone==err);
}

CHXAudioDevice::~CHXAudioDevice()
{
    Close(TRUE);
    m_iTimer.Close();
    HX_DELETE(m_pPrioritySettings);
    HX_RELEASE(m_pContext);
    HX_RELEASE(m_pErrorMessages);
}

ULONG32 CHXAudioDevice::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

ULONG32 CHXAudioDevice::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

HX_RESULT CHXAudioDevice::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IHXAudioDevice))
    {
        AddRef();
        *ppvObj = (IHXAudioDevice*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = this;
        return HXR_OK;
    }
	else if (IsEqualIID(riid, IID_IHXPropWatchResponse))
    {
        AddRef();
        *ppvObj = this;
        return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

HX_RESULT CHXAudioDevice::Open(const HXAudioFormat* pFormat,
                               IHXAudioDeviceResponse* pDeviceResponse)
{
    HX_RESULT res = HXR_OK;

    HX_ASSERT(!m_pDeviceResponse);
    HX_RELEASE(m_pDeviceResponse);
    if( pDeviceResponse )
    {
        m_pDeviceResponse = pDeviceResponse;
        m_pDeviceResponse->AddRef();
    }

    if (HXR_OK != OpenDevice() || HXR_OK != InitDevice(pFormat))
    {
        res = HXR_FAIL;
    }

    return res;
}

HX_RESULT CHXAudioDevice::Close(const HXBOOL bFlush)
{
    Reset();

    if (m_pAudioStream)
    {
        CancelAudDeviceErrNotification();
        m_pAudioStream->CloseDevice();
        m_deviceOpen = false;

        HX_DELETE(m_pAudioStream);
        HX_RELEASE(m_pDeviceResponse);
    }

    return HXR_OK;
}

HX_RESULT CHXAudioDevice::Write(const HXAudioData* pAudioData)
{
    HX_RESULT res = HXR_FAIL;
    if (pAudioData)
    {
        // add the buffer to the pending write list
        IHXBuffer* pAudioBuf = pAudioData->pData;
        if( m_pAudioStream )
        {
            pAudioBuf->AddRef();
            TInt err = m_pAudioStream->Write(pAudioBuf);
            if (KErrNone == err)
            {
                res = HXR_OK;
            }
        }
    }

    return res;
}

HX_RESULT CHXAudioDevice::Reset()
{
    //reset SecureOutput settings
    m_bSecureOutputChanged = FALSE;
    
    if (m_pAudioStream)
    {
        if (!m_pAudioStream->Stopped())
        {
            m_pAudioStream->Stop();
        }
    }

    if (IsActive())
    {
        Cancel();
    }

    return HXR_OK;
}

HX_RESULT CHXAudioDevice::Drain()
{
    HX_ASSERT("Not implemented"==NULL);
    return HXR_FAIL;
}

HX_RESULT CHXAudioDevice::SetVolume( const UINT16 uVolume )
{
    HX_RESULT res = HXR_FAIL;

    if( m_pAudioStream )
    {
        m_pAudioStream->SetVolume(TInt(Scale(UINT32(uVolume),
                                             m_uMinPlayerVolume, m_uMaxPlayerVolume,
                                             m_uMinDevVolume, m_uMaxDevVolume)));
        res = HXR_OK;
    }

    return res;
}

HX_RESULT CHXAudioDevice::GetCurrentAudioTime( ULONG32& ulCurrentTime )
{
    HX_RESULT res = HXR_FAIL;

    HX_ASSERT(m_pAudioStream);
    if( m_pAudioStream )
    {
        ulCurrentTime = m_pAudioStream->GetTime();
		TInt rv = 0;
#if defined(HELIX_FEATURE_DRM_SECURE_OUTPUT)
		if(m_bSecureOutputChanged) 
		{
			if (ulCurrentTime>=m_uSecureOutputChangeTime) 
			{
				//Send new secure output settings to audio device
				rv = m_pAudioStream->SetSecureOutput(m_lSecureOutputSetting & 0x1);
				m_bSecureOutputChanged = FALSE;
			}
		}
#endif
		if(rv == KErrNotSupported)
		{
			res = HXR_SET_SECURE_OUT_FAIL;
			if(m_pErrorMessages != NULL)
			{
				m_pErrorMessages->Report(HXLOG_INFO, res, 0, NULL, NULL);
			}
		}
		else
		{
        res = HXR_OK;
		}
    }

    return res;
}

HXBOOL CHXAudioDevice::SupportsVolume()
{
    return TRUE;
}

UINT16 CHXAudioDevice::GetVolume()
{
    UINT16 vol = 0;
    if (m_pAudioStream)
    {
        vol = UINT16(Scale(UINT32(m_pAudioStream->GetVolume()),
                           m_uMinDevVolume, m_uMaxDevVolume,
                           m_uMinPlayerVolume, m_uMaxPlayerVolume));
    }

    return vol;
}

short CHXAudioDevice::GetAudioFd( void )
{
    //We don't have file descriptors on symbian for the
    //audio device.
    return 0;
}

HX_RESULT CHXAudioDevice::Seek(ULONG32 ulSeekTime)
{
    HX_ASSERT( "Not implemented"==NULL);
    return HXR_OK;
}

//Ensure timer is active. This timer drives calls to OnTimeSync().
void CHXAudioDevice::DoSetTimer()
{
    if( !IsActive())
    {
        m_iTimer.After(iStatus, TIMESYNC_CALLBACK_INTERVAL_MS * 1000);
        SetActive();
    }
}

HX_RESULT CHXAudioDevice::Resume()
{
    AddRef();	// Make sure OnTimeSync does not destroy us

    m_paused = false;

    //Return 0 time stamp for the beginning of playback.
    if( m_pDeviceResponse)
    {
        ULONG32 ulAudioTime = 0;
        GetCurrentAudioTime(ulAudioTime);
        m_pDeviceResponse->OnTimeSync(ulAudioTime);
    }

    if (m_pAudioStream)
    {
        m_pAudioStream->Play();
    }

    //Start OnTimeSync timer
    DoSetTimer();

    Release();
    
    return HXR_OK;
}

HX_RESULT CHXAudioDevice::Pause()
{
    HX_RESULT res = HXR_OK;

    m_paused = true;

    if (m_pAudioStream)
    {
        m_pAudioStream->Pause();
    }

    //Stop calling OnTimeSync while paused
    if (IsActive())
    {
        Cancel();
    }

    return res;
}

HX_RESULT CHXAudioDevice::CheckFormat(const HXAudioFormat* pFormat)
{
    HX_RESULT res = HXR_FAIL;

    //Symbian only supports 16-bit PCM.
    if (pFormat->uBitsPerSample == 16 &&
        HXR_OK == OpenDevice() &&
        HXR_OK == InitDevice(pFormat))
    {
        res = HXR_OK;
    }

    return res;
}

UINT16 CHXAudioDevice::NumberOfBlocksRemainingToPlay(void)
{
    return (UINT16)(m_pAudioStream->GetBlocksBuffered());
}

HXBOOL CHXAudioDevice::InitVolume(const UINT16 uMinVolume,
                                const UINT16 uMaxVolume )
{
    HX_ASSERT(m_uMaxPlayerVolume > m_uMinPlayerVolume);

    if (m_uMaxPlayerVolume > m_uMinPlayerVolume)
    {
        m_uMinPlayerVolume = uMinVolume;
        m_uMaxPlayerVolume = uMaxVolume;
    }
    return (m_uMaxPlayerVolume > m_uMinPlayerVolume) && SupportsVolume();
}

//
// CActive methods
//

void CHXAudioDevice::RunL()
{
    if (KErrNone == iStatus.Int())
    {
	AddRef();   // Make sure OnTimeSync does not destroy us

        //Check the time and call OnTimeSync
        if( m_pDeviceResponse && !m_paused)
        {
            ULONG32 ulAudioTime = 0;
            GetCurrentAudioTime(ulAudioTime);
            m_pDeviceResponse->OnTimeSync(ulAudioTime);
        }

        //Set timer for next OnTimeSync callback
        DoSetTimer();

	Release();
    }
    else if (KErrCancel != iStatus.Int())
    {
        //Errors (other than KErrCancel) not handled well. We need
        //a well-defined way to notify higher-level code.
        HX_ASSERT(FALSE);
        DoSetTimer();
    }
}

void CHXAudioDevice::DoCancel()
{
    m_iTimer.Cancel();
}

//////////////////////////////////////////////////////////////////////
// private methods ///////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

HX_RESULT CHXAudioDevice::OpenDevice()
{
    HX_RESULT res = HXR_FAIL;

    if (m_deviceOpen)
        return HXR_OK;

    m_pAudioStream = MHXSymbianAudioDevice::Create();
    
    if((m_pAudioStream != NULL)
        && (m_pAudioStream->Open() == KErrNone))
        {
            m_deviceOpen = true;
            res = RegisterAudDeviceErrNotification();
        }

    return res;
}

HX_RESULT CHXAudioDevice::InitDevice(const HXAudioFormat* pFormat)
{
    HX_RESULT res = HXR_FAIL;

    // retrieve MMF Priority Settings stored in prefs to init device
    GetPrioritySettings();

    INT32 lSecureAudio = 0;

#if defined(HELIX_FEATURE_DRM_SECURE_OUTPUT)
    //
    //  Determine if output must be secure from Registry
    //
    if (m_pContext)
    {
        IHXRegistry* pReg = NULL;

        if (m_pContext->QueryInterface(IID_IHXRegistry, (void**)&pReg) == HXR_OK)
        {
            HX_RELEASE(m_pPropWatch);
            pReg->GetIntByName("MMF.SecureOutput", lSecureAudio);
			res = pReg->CreatePropWatch(m_pPropWatch);  //watching for secure output changes on the fly
            if (SUCCEEDED(res))
            {
                m_pPropWatch->SetWatchByName("MMF.SecureOutput");
                m_pPropWatch->Init(this);
            }

            HX_RELEASE(pReg);
        }
    }

    HXLOGL2(HXLOG_ADEV, "CHXAudioDevice::InitDevice() Secure Audio %d", lSecureAudio);
#endif

    // attempt to init device with given audio format
    if (m_pAudioStream &&
        (KErrNone == m_pAudioStream->Init(pFormat->ulSamplesPerSec,
                                          pFormat->uChannels,
                                          m_pPrioritySettings,
                                          (HXBOOL)lSecureAudio)))
    {
        // grab device volume info (first opportunity)
        m_uMinDevVolume = m_pAudioStream->GetMinVolume();
        m_uMaxDevVolume = m_pAudioStream->GetMaxVolume();

        res = HXR_OK;
    }
    return res;
}

//
// CHXAudioDevice::Init()
// QI for ErrorMessage
//
void CHXAudioDevice::Init(IUnknown* pContext)
{
   HX_RELEASE(m_pContext);
   m_pContext = pContext;
   HX_ADDREF(m_pContext);

   HX_RELEASE(m_pErrorMessages);
   m_pContext->QueryInterface(IID_IHXErrorMessages, (void **)&m_pErrorMessages);
   HX_ASSERT(m_pErrorMessages != NULL);
}

//
// CHXAudDevStatusObserver::OnAudDevStatusChange()
// Handler invoked for Aud device status change
// (KErrDied, KErrAccessDenied)
//
void CHXAudioDevice::OnAudDevStatusChange(TInt AudDevStatus)
{
    // Report error to Error Messages
    if((AudDevStatus == KErrDied) ||
        (AudDevStatus == KErrAccessDenied) ||
        (AudDevStatus == KErrInUse))
    {
        HX_ASSERT(m_pErrorMessages != NULL);
        m_pErrorMessages->Report(HXLOG_INFO, HXR_AUDIODEVICETAKEN, 0, NULL, NULL);
    }
	else
	{
		HX_ASSERT(m_pErrorMessages != NULL);
        m_pErrorMessages->Report(HXLOG_ERR, HXR_AUDIO_DRIVER, 0, NULL, NULL);
	}
}

//
// CHXAudioDevice::RegisterAudDeviceErrNotification()
// Requests aud stream for notification.
//
HX_RESULT CHXAudioDevice::RegisterAudDeviceErrNotification()
{
    HX_ASSERT(m_pAudioStream != NULL);

    m_pAudioStream->RequestDeviceTakenNotification(this);
    return HXR_OK;
}

//
// CHXAudioDevice::CancelAudDeviceErrNotification()
// Cancels any outstanding aud dev notification req
//
void CHXAudioDevice::CancelAudDeviceErrNotification()
{
    HX_ASSERT(m_pAudioStream != NULL);

    m_pAudioStream->CancelDeviceTakenNotification();
}

//
// CHXAudioDevice::GetPrioritySettings()
// Retrieve priority settings from MMF and store in member
//
void CHXAudioDevice::GetPrioritySettings()
{
    HXLOGL2(HXLOG_ADEV, "CHXAudioDevice::GetPrioritySettings()");

    INT32 prio = 0;
    INT32 pref = 0;

    if (!m_pPrioritySettings)
    {
        m_pPrioritySettings = new TMMFPrioritySettings;
    }

    if (m_pContext)
    {
        //
        //  Get the IHXRegistry interface from the context
        //
        IHXRegistry* pReg = NULL;

        if (m_pContext->QueryInterface(IID_IHXRegistry, (void**)&pReg) == HXR_OK)
        {
            //
            //  read in audio priority and preference values
            //
            pReg->GetIntByName("MMF.Audio.Priority", prio);
            pReg->GetIntByName("MMF.Audio.PriorityPref", pref);

            HX_RELEASE(pReg);
        }
    }

    m_pPrioritySettings->iPriority = prio;
    m_pPrioritySettings->iPref = (TMdaPriorityPreference)pref;

    HXLOGL2( HXLOG_ADEV, "    -- priority values (%d, %d) from registry",
        m_pPrioritySettings->iPriority, m_pPrioritySettings->iPref);
}

//IHXPropWatchResponse methods
STDMETHODIMP
CHXAudioDevice::AddedProp(const UINT32 ulId, const HXPropType propType, const UINT32 ulParentID)
{
	//No need to implement
    return HXR_OK;
}

STDMETHODIMP
CHXAudioDevice::ModifiedProp(const UINT32 ulId, const HXPropType propType, const UINT32 ulParentID)
{
	IHXRegistry* pReg = NULL;
	if (m_pContext->QueryInterface(IID_IHXRegistry, (void**)&pReg) == HXR_OK)
	{
		INT32 lSecureOutputChangeTime;
		pReg->GetIntByName("MMF.SecureOutputTime", lSecureOutputChangeTime);
		m_uSecureOutputChangeTime = (UINT32)lSecureOutputChangeTime;
		pReg->GetIntByName("MMF.SecureOutput", m_lSecureOutputSetting);
		HX_RELEASE(pReg);
		m_bSecureOutputChanged = TRUE;
	}

    return HXR_OK;
}

STDMETHODIMP
CHXAudioDevice::DeletedProp(const UINT32	ulId, const UINT32 ulParentID)
{
    HX_RELEASE(m_pPropWatch);
    return HXR_OK;
}

//////////////////////////////////////////////////////////////////////
// convenience methods ///////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

UINT32 Scale(UINT32 v, UINT32 f0, UINT32 f1, UINT32 t0, UINT32 t1)
{
    HX_ASSERT(f1 > f0);
    HX_ASSERT(t1 >= t0);
    HX_ASSERT(v >= f0);

    if (f1 > f0 && v > f0)
    {
        UINT64 tr = t1 - t0;
        UINT64 fr = f1 - f0;
        UINT64 n  = v - f0;
        UINT64 q  = n / fr;
        UINT64 r  = n % fr;

        return t0 + INT64_TO_UINT32((q * tr + (r * tr + (fr >> 1)) / fr));
    }
    return 0;
}
