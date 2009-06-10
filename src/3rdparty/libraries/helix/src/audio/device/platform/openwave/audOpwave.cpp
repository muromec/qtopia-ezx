/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: audOpwave.cpp,v 1.6 2007/07/06 20:21:12 jfinnecy Exp $
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
#include "hxcom.h"
#include "hxresult.h"
#include "hxengin.h"
#include "ihxpckts.h"   // for IHXBuffer 
#include "hxslist.h"
#include "timeval.h"
#include "hxausvc.h"
#include "auderrs.h"
#include "hxaudev.h"
#include "hxaudses.h"
#include "hxtick.h"
#include "chxpckts.h"
#include "debug.h"

#include "audOpwave.h"

#include "hlxclib/errno.h"

#include "hxtick.h"

//-1 is usually considered to be no file descriptor.
const int CAudioOutOpenwave::NO_FILE_DESCRIPTOR = -1;

const int CAudioOutOpenwave::MAX_VOLUME = 100;

static const unsigned long BLOCK_SIZE = 176400;   // ~ 1s of audio, size of the write buffer

//**************************************************************************
// Prototypes.
//
static void
iodone_callback (
	op_sound_handle const	*handle,
	S32			msg,
	op_sound_buffer const	*sndbuf,
	void			*data
);

static void
iodone_callback (
op_sound_handle const	*handle,
S32			msg,
op_sound_buffer const	*sndbuf,
void			*data
)
{
	CAudioOutOpenwave *app = (CAudioOutOpenwave *) data;
	app->iodone();
}

void
CAudioOutOpenwave::iodone()
{
	if (m_bWriteDone) // huh? double buffering out of sync, we should always be writing
	{
		//OpDPRINTF("iodone: double buffering ERROR!!!\n");
		//_PushBits();
	}
	m_bWriteDone = true;
	_PushBits();
}

    
//XXXgfw We need to clean up the return values. We need to return only PN result codes
//XXXgfw and not RA_AOE codes from interface methods.
CAudioOutOpenwave::CAudioOutOpenwave() :
    m_pCallback(NULL),
    m_wState( RA_AOS_CLOSED ),
    m_wLastError( RA_AOE_NOERR ),
    m_bMixerPresent(FALSE),
    m_wBlockSize(0),
    m_ulLastNumBytes (0),
    m_ulTotalWritten(0),
    m_bFirstWrite (TRUE),
    m_bInitCallback (TRUE),
    m_pPlaybackCountCBTime(0),
    m_PendingCallbackID (0),
    m_bCallbackPending(FALSE),
    m_pWriteList(NULL),
    m_ulDeviceBufferSize(0),
    m_ulLastTick(0),
    m_pSndDev(NULL),
    m_bWriteDone(true)
{

	//Alloc a Timeval for use with the timer callback.
    m_pPlaybackCountCBTime = new Timeval;
    m_pCallback = new HXPlaybackCountCB(this);
    m_pCallback->AddRef();

    // Initialize the Openwave write buf struct
    for (int i=0; i<10; i++)
    {
	m_SndBuf[i].fSampleBuffer = malloc(BLOCK_SIZE);
	m_SndBuf[i].fNSamples		= 0;
	m_SndBuf[i].fUserData		= NULL;
	m_SndBuf[i].fState		= OP_SNDBUF_STATE_READY;
	m_SndBuf[i].fFlags		= OP_SNDBUFF_CALLBACK_ON_IODONE;
    }
    m_nCurBuf = 0;
    m_ulSampleBufSize = BLOCK_SIZE;
    
    //Alloc our write buffer list. Want to throw from here? You will, like
    //it or not.
    m_pWriteList = new CHXSimpleList();
    
}

void CAudioOutOpenwave::_initAfterContext()
{    
}

CAudioOutOpenwave::~CAudioOutOpenwave()
{
    //We must assume that _Imp_Close has already been called. If not, we are 
    //in big trouble.
    if ( m_wState != RA_AOS_CLOSED ) 
    {
        HX_ASSERT( "Device not closed in dtor." == NULL );
    }

    //Clean up the scheduler.
    HX_RELEASE( m_pScheduler );
    HX_DELETE(m_pPlaybackCountCBTime);
    HX_RELEASE(m_pCallback);
    //OpDPRINTF("dtor: Freeing sample buffer, expect an exception\n");
    for (int i=0; i<10; i++)
    	HX_DELETE(m_SndBuf[i].fSampleBuffer);
    //OpDPRINTF("dtor: Done freeing sample buffers \n");
    if (m_pSndDev) 
    {
    	//OpDPRINTF("dtor : before freeing the channel\n");
	op_sound_unregister_callback(m_pSndDev, iodone_callback);
	op_sound_freechan(m_pSndDev);
    	//OpDPRINTF("dtor : freeing the channel\n");
    }
    HX_DELETE(m_pWriteList); // Remove all elements in the list, if any
}

UINT16 CAudioOutOpenwave::_Imp_GetVolume()
{
 
    return m_uCurVolume;
}

HX_RESULT CAudioOutOpenwave::_Imp_SetVolume( const UINT16 uVolume )
{
    HX_RESULT retCode = RA_AOE_NOERR;
    
	m_uCurVolume = uVolume;

    m_wLastError = retCode;
    return m_wLastError;
}


HXBOOL CAudioOutOpenwave::_Imp_SupportsVolume()
{
    return TRUE;
}


HX_RESULT CAudioOutOpenwave::_Imp_Open( const HXAudioFormat* pFormat )
{
    HX_RESULT retCode = RA_AOE_NOERR;
    m_ulLastTick = GetTickCount();
	m_ulCurrentTime = 0;
	m_wState = RA_AOS_OPENING;
	m_wLastError = retCode;

	//////////////////////////////
	// open the channel
	//////////////////////////////
	
	OpError	retval;

	if (m_pSndDev) 
	{
		//_Imp_Close();	// Close last channel
		//op_sound_unregister_callback(m_pSndDev, iodone_callback);
		op_sound_freechan(m_pSndDev);
	}

	//  Procure channels.  
	if (!(m_pSndDev = op_sound_allocchan (pFormat->uChannels)))
		return HXR_FAIL;

	m_unNumChannels = pFormat->uChannels;
	m_unSampleRate = pFormat->ulSamplesPerSec;
	m_unBitsPerSample = pFormat->uBitsPerSample;
    	m_unBytesPerSec = (m_unNumChannels * (m_unBitsPerSample/8) * m_unSampleRate);

	// Convert params into Openwave enums
	op_sound_pcm_format pcmfmt = OP_PCM_FMT_INVALID;
	switch (pFormat->uBitsPerSample)
	{
	case 8 : pcmfmt = OP_PCM_FMT_U8;	break;
	case 16: pcmfmt = OP_PCM_FMT_U16_LE;	break;
	}

	op_sound_pcm_rate pcmrate = OP_PCM_RATE_44100;
	switch (pFormat->ulSamplesPerSec)
	{
	case 8000 : pcmrate = OP_PCM_RATE_8000;		break;
	case 11025: pcmrate = OP_PCM_RATE_11025;	break;
	case 16000: pcmrate = OP_PCM_RATE_16000;	break;
	case 22050: pcmrate = OP_PCM_RATE_22050;	break;
	case 32000: pcmrate = OP_PCM_RATE_32000;	break;
	case 44100: pcmrate = OP_PCM_RATE_44100;	break;
	case 48000: pcmrate = OP_PCM_RATE_48000;	break;
	case 64000: pcmrate = OP_PCM_RATE_64000;	break;
	}

	//  Configure channels for playback.  
	retval = op_sound_set_params_args
	          (m_pSndDev,
	           OP_AUDIOTAG_VOLUME, 0xFFFF, // m_uCurVolume ?
	           OP_AUDIOTAG_PCMFORMAT, pcmfmt,
		   OP_AUDIOTAG_PCMRATE, pcmrate,
	           OP_AUDIOTAG_INTERLEAVESAMPLES, true,
	           OP_AUDIOTAG_END);
	if (retval < 0)
		return HXR_FAIL;

	//  Register our callback.  
	retval = op_sound_register_callback
	          (m_pSndDev, iodone_callback, this);
	if (retval < 0)
		return HXR_FAIL;

	// Start the device
	retval = op_sound_start (m_pSndDev);
	if (retval < 0)
		return HXR_FAIL;

    //Schedule the timer callback...
    if(m_pContext && !m_pScheduler)
    {
        m_pContext->QueryInterface(IID_IHXScheduler, (void**) &m_pScheduler );
    }

    return m_wLastError;
}

HX_RESULT CAudioOutOpenwave::_Imp_Close()
{
    HX_RESULT retCode = RA_AOE_NOERR;

	if (m_pSndDev != NULL)
	{
	    // Stop the callback
	    op_sound_unregister_callback(m_pSndDev, iodone_callback);
		// Openwave error, can't free a channel when it's busy. Just free it in the destructor
		//op_sound_freechan(m_pSndDev);
		//m_pSndDev = NULL;
	}

    //Remove callback from scheduler
    if (m_bCallbackPending)
    {
        m_pScheduler->Remove(m_PendingCallbackID);
        m_bCallbackPending = FALSE;
    }

    m_wLastError = retCode;

    return m_wLastError;
}


// no support this.
HX_RESULT CAudioOutOpenwave::_Imp_Seek(ULONG32 ulSeekTime)
{
    m_wLastError = HXR_OK;
    return m_wLastError;
}

HX_RESULT CAudioOutOpenwave::_Imp_Pause()
{
    m_wLastError = HXR_OK;
	m_wState = RA_AOS_OPEN_PAUSED;

	op_sound_unregister_callback(m_pSndDev, iodone_callback);
	op_sound_pause(m_pSndDev);

    return m_wLastError;
}

HX_RESULT CAudioOutOpenwave::_Imp_Resume()
{
    //XXXgfw We really should be closing and re-opening the device to be nice to other procs.
    m_wLastError = HXR_OK;
	m_wState = RA_AOS_OPEN_PLAYING;
	m_ulLastTick = GetTickCount();

	op_sound_register_callback(m_pSndDev, iodone_callback, this);
	if (!m_bFirstWrite)
	{
		// Start the double buffering again
		m_bWriteDone = TRUE;
		_PushBits(); 
		m_bWriteDone = TRUE;
		_PushBits(); 
	}
	op_sound_resume(m_pSndDev);

    return m_wLastError;
}

HX_RESULT CAudioOutOpenwave::_Imp_Reset()
{
    HX_RESULT retCode = RA_AOE_NOERR;
    m_ulCurrentTime = 0;
	m_ulLastTick = GetTickCount();
    m_wLastError = retCode;
    return m_wLastError;
}

HX_RESULT CAudioOutOpenwave::_Imp_Drain()
{
    HX_RESULT retCode = RA_AOE_NOERR;

    m_wLastError = retCode;
    return m_wLastError;
}


HX_RESULT CAudioOutOpenwave::_Imp_CheckFormat( const HXAudioFormat* pFormat )
{
    HX_RESULT retCode = RA_AOE_NOERR;
    m_wLastError       = HXR_OK;

    return m_wLastError;
}

HX_RESULT CAudioOutOpenwave::_Imp_GetCurrentTime( ULONG32& ulCurrentTime )
{
    ULONG32 ulTime   = 0;
    INT64  ulBytes  = 0;
    /*
  
	if (RA_AOS_OPEN_PLAYING == m_wState)
	{
		ulTime = GetTickCount();
    
		HX_ASSERT(ulTime >= m_ulLastTick);

		//Not used anywhere but belongs to CHXAudioDevice so we must set it.
		m_ulCurrentTime  += (ulTime - m_ulLastTick);
		m_ulLastTick = ulTime;
	}
	ulCurrentTime = m_ulCurrentTime;
	*/

//// New implementation

    ulBytes = _GetBytesActualyPlayed();

    ulTime = (ULONG32) ((ulBytes * 1000) / m_unBytesPerSec);
    
//    OpDPRINTF("cur %d, new %d\n", m_ulCurrentTime, ulTime);
    //Not used anywhere but belongs to CHXAudioDevice so we must set it.
    m_ulCurrentTime  = ulTime;

    //Set the answer.
    ulCurrentTime = ulTime;

    m_wLastError = HXR_OK;
    return HXR_OK;
}

void CAudioOutOpenwave::DoTimeSyncs()
{
    ReschedPlaybackCheck();
    OnTimeSync();
    return;
}


HX_RESULT CAudioOutOpenwave::ReschedPlaybackCheck()
{
    HX_RESULT retCode = HXR_OK;
    
    if(!m_bCallbackPending)
    {
        HX_ASSERT( m_pCallback );
        if(m_pCallback)
        {
            *m_pPlaybackCountCBTime += (int)(500*m_ulGranularity);
            m_bCallbackPending = TRUE;
            m_PendingCallbackID = m_pScheduler->AbsoluteEnter( m_pCallback,*((HXTimeval*)m_pPlaybackCountCBTime));
        }
        else
        {
            retCode = HXR_OUTOFMEMORY;
        }
    }

    m_wLastError = retCode;
    return m_wLastError;
}

UINT16 CAudioOutOpenwave::_NumberOfBlocksRemainingToPlay(void)
{
    //XXXctd total hack, to make sure there is always data to write
    if (m_pWriteList && !m_bFirstWrite && m_pWriteList->GetCount() > 10)
    {
	   return m_pWriteList->GetCount(); 
    }
    return 0;
}


CAudioOutOpenwave::HXPlaybackCountCB::~HXPlaybackCountCB()
{
}

STDMETHODIMP CAudioOutOpenwave::HXPlaybackCountCB::QueryInterface(	REFIID riid, void** ppvObj )
{
    HX_RESULT retCode = HXR_OK;
    
    if( IsEqualIID(riid, IID_IHXCallback) )
    {
        AddRef();
        *ppvObj = (IHXCallback*)this;
    }
    else if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = this;
    }
    else
    {
        *ppvObj = NULL;
        retCode = HXR_NOINTERFACE;
    }

    return retCode;
}

STDMETHODIMP_(ULONG32) CAudioOutOpenwave::HXPlaybackCountCB::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32) CAudioOutOpenwave::HXPlaybackCountCB::Release()
{
    
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }
    
    delete this;
    return HXR_OK;
}

STDMETHODIMP CAudioOutOpenwave::HXPlaybackCountCB::Func(void)
{
    if (m_pAudioDeviceObject)
    {
        if(!m_timed)
        {
            //m_pAudioDeviceObject->_Imp_Write(NULL);
        }
        else
        {
            m_pAudioDeviceObject->m_bCallbackPending  = FALSE;
            //m_pAudioDeviceObject->_Imp_Write(NULL);
            m_pAudioDeviceObject->DoTimeSyncs();
        }
    }

    return HXR_OK;
}


HX_RESULT CAudioOutOpenwave::_Pause()
{
    return RA_AOE_NOTSUPPORTED;
}

HX_RESULT CAudioOutOpenwave::_Resume()
{
    return RA_AOE_NOTSUPPORTED;
}

HXBOOL CAudioOutOpenwave::_IsSelectable() const
{
    return TRUE;
}

HXBOOL CAudioOutOpenwave::_HardwarePauseSupported() const
{
    return FALSE;
}


// Don't protect anything in this method with mutexes. It has 
// already been done from where it is called from. But mostly
// because we aren't using recursive mutexes yet.
ULONG32 CAudioOutOpenwave::_PushBits()
{
    if (!m_bWriteDone)
    {
    	//OpDPRINTF("_PushBits : ERROR Both buffers are writing?\n");
	return 0;
    }
    if (m_pWriteList == NULL || m_pWriteList->GetCount() <= 0)  
    {
    	//OpDPRINTF("_PushBits : ERROR Nothing to write\n");
    	//OpDPRINTF("_PushBits : m_pWriteList(%d) %d\n", m_pWriteList->GetCount(), m_nCurBuf);
	return 0;
    }
    if (m_SndBuf[m_nCurBuf].fState == OP_SNDBUF_STATE_NOWPLAYING ||
    	m_SndBuf[m_nCurBuf].fState == OP_SNDBUF_STATE_QUEUED)
    {
    	//OpDPRINTF("_PushBits : buf playing %d\n", m_nCurBuf);
	return 0;
    }
    //OpDPRINTF("_PushBits : m_pWriteList(%d)\n", m_pWriteList->GetCount());

    IHXBuffer* pBuffer  = NULL;
    UCHAR*      pData    = 0;
    ULONG32     nSize = 0;
    op_sound_buffer* sndbuf;

    sndbuf = &m_SndBuf[m_nCurBuf];
    m_nCurBuf = !m_nCurBuf;
    //m_nCurBuf = (m_nCurBuf + 1) % 10;  // use all ten buffers

    //We are going to try and write. Grab the head and do it.
    pBuffer = (IHXBuffer*)m_pWriteList->RemoveHead();
    pData   = pBuffer->GetBuffer();
    nSize   = pBuffer->GetSize();

    memcpy( (unsigned char*)sndbuf->fSampleBuffer, pData, nSize );
    HX_RELEASE( pBuffer );

    sndbuf->fNSamples = (nSize) / (m_unNumChannels * (m_unBitsPerSample/8));
    m_ulTotalWritten += nSize; //Keep track of how much we have written to the device.
    op_sound_write (m_pSndDev, sndbuf);

    m_bWriteDone = false;

    return nSize;
}

HX_RESULT CAudioOutOpenwave::_Imp_Write( const HXAudioData* pAudioOutHdr )
{
    ULONG32     ulCount  = 0;

    //Schedule callbacks.
    if( m_bInitCallback && pAudioOutHdr )
    {
        m_bInitCallback = FALSE;

        //  Initialize the playback callback time.
        HXTimeval lTime = m_pScheduler->GetCurrentSchedulerTime();
        m_pPlaybackCountCBTime->tv_sec = lTime.tv_sec;
        m_pPlaybackCountCBTime->tv_usec = lTime.tv_usec;

        //  Scheduler playback callback.
        ReschedPlaybackCheck();
    }

    //Blindly add the incoming data to the tail of the writelist.
    if( pAudioOutHdr != NULL)
    {
        IHXBuffer* pTmpBuff = pAudioOutHdr->pData;
        m_pWriteList->AddTail(pTmpBuff);
        pTmpBuff->AddRef();
    }

    //OpDPRINTF("_Imp_Write : m_pWriteList(%d) \n", m_pWriteList->GetCount());

    //Just return if there is nothing to do 
    if( m_pWriteList->GetCount() <= 0 || m_wState==RA_AOS_OPEN_PAUSED )
    {
        return RA_AOE_NOERR;
    }
    m_wState = RA_AOS_OPEN_PLAYING;

    if( m_bFirstWrite )
    {
	if (m_pWriteList->GetCount() >= 10)
	{
        	m_bFirstWrite = FALSE;
		for (int i=0; i<2; i++)
		{
			m_bWriteDone = TRUE;
			ulCount = _PushBits(); // Start the double buffering
		}
	}
    }
    else
    {
    	//ulCount = _PushBits();// causes two competing threads
    }
    
    if ( m_bInitCallback )
    {
        m_bInitCallback = FALSE;
            
        //  Initialize the playback callback time. 
        HXTimeval lTime = m_pScheduler->GetCurrentSchedulerTime();
        m_pPlaybackCountCBTime->tv_sec = lTime.tv_sec;
        m_pPlaybackCountCBTime->tv_usec = lTime.tv_usec;
            
        //  Scheduler playback callback. 
        ReschedPlaybackCheck();
    }

    //make sure the device is kept full.
    //if( m_pWriteList->GetCount()>0 && ulCount>0)
        //_PushBits();

    return m_wLastError;
    
}

//-------------------------------------------------------
// These Device Specific methods must be implemented 
// by the platform specific sub-classes.
//-------------------------------------------------------
INT16 CAudioOutOpenwave::_Imp_GetAudioFd(void)
{
    return 0;
}

//Device specific method to set the audio device characteristics. Sample rate,
//bits-per-sample, etc.
//Method *must* set member vars. m_unSampleRate and m_unNumChannels.
HX_RESULT CAudioOutOpenwave::_SetDeviceConfig( const HXAudioFormat* pFormat )
{
    return RA_AOE_NOERR;
}


//Device specific method to write bytes out to the audiodevice and return a
//count of bytes written. 
HX_RESULT CAudioOutOpenwave::_WriteBytes( UCHAR* buffer, ULONG32 ulBuffLength, LONG32& lCount )
{
    HX_RESULT retCode = RA_AOE_NOERR;
    return retCode;
}

//Device specific methods to open/close the mixer and audio devices.
HX_RESULT CAudioOutOpenwave::_OpenAudio()
{
    HX_RESULT retCode = RA_AOE_NOERR;
    
    m_wLastError = retCode;
    return m_wLastError;
}

HX_RESULT CAudioOutOpenwave::_CloseAudio()
{
    HX_RESULT retCode = RA_AOE_NOERR;
    
    m_wLastError = retCode;
    return m_wLastError;
}

HX_RESULT CAudioOutOpenwave::_OpenMixer()
{
    HX_RESULT retCode = RA_AOE_NOERR;
    m_wLastError = retCode;
    return m_wLastError;
}

HX_RESULT CAudioOutOpenwave::_CloseMixer()
{
    HX_RESULT retCode = RA_AOE_NOERR;

    m_wLastError = retCode;
    return m_wLastError;
}


UINT64 CAudioOutOpenwave::_GetBytesActualyPlayed(void) const
{
    /* Get current playback position in device DMA. */
    return  m_ulTotalWritten;
}




HX_RESULT CAudioOutOpenwave::_CheckSampleRate( ULONG32 ulSampleRate )
{
    ULONG32 ulTmp = ulSampleRate;

    m_wLastError = RA_AOE_NOERR;
    
    return m_wLastError;
}
