/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: audUnix.cpp,v 1.16 2009/05/01 14:19:46 sfu Exp $
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
#include "pckunpck.h"
#include "hxmutex.h"
#include "debug.h"

#include "microsleep.h"
//me.
#include "audUnix.h"

#include <errno.h>

#if defined(_THREADED_AUDIO) && defined(_UNIX_THREADS_SUPPORTED)
#include "hxprefs.h"
#endif 

#include "hxtlogutil.h"
#include "ihxtlogsystem.h"
#include "baseobj.h"
#include "nestbuff.h"

//-1 is usually considered to be no file descriptor.
const int CAudioOutUNIX::NO_FILE_DESCRIPTOR = -1;

const int CAudioOutUNIX::MAX_VOLUME = 100;
    
//XXXgfw We need to clean up the return values. We need to return only PN result codes
//XXXgfw and not RA_AOE codes from interface methods.
CAudioOutUNIX::CAudioOutUNIX() :
    m_pCallback(NULL),
    m_wState( RA_AOS_CLOSED ),
    m_wLastError( RA_AOE_NOERR ),
    m_bMixerPresent(FALSE),
    m_wBlockSize(0),
    m_ulLastNumBytes (0),
    m_ulTotalWritten(0),
    m_bFirstWrite (TRUE),
    m_pPlaybackCountCBTime(0),
    m_PendingCallbackID (0),
    m_bCallbackPending(FALSE),
    m_pWriteList(NULL),
    m_ulDeviceBufferSize(0),
#if defined(_THREADED_AUDIO) && defined(_UNIX_THREADS_SUPPORTED)
    m_mtxWriteListPlayStateLock(NULL),
    m_mtxDeviceStateLock(NULL),
    m_audioThread(NULL),
    m_bUserWantsThreads(TRUE),
    m_ulSleepTime(0),
    m_pAvailableDataEvent(NULL),
#endif
    m_ulALSAPeriodSize(0),
#ifdef HELIX_FEATURE_ALSA_WRITE_PERIOD_SIZE
    m_ulByteCount(0),
    m_pCCF(NULL),
#endif //HELIX_FEATURE_ALSA_WRITE_PERIOD_SIZE    
    m_pRollbackBuffer(NULL)
{

    //Alloc a Timeval for use with the timer callback.
    m_pPlaybackCountCBTime = new Timeval;
    m_pCallback = new HXPlaybackCountCB(this);
    m_pCallback->AddRef();
    
    //Allco our write buffer list. Want to throw from here? You will, like
    //it or not.
    m_pWriteList = new CHXSimpleList();
}

void CAudioOutUNIX::_initAfterContext()
{

#ifdef HELIX_FEATURE_ALSA_WRITE_PERIOD_SIZE
    m_pContext->QueryInterface(IID_IHXCommonClassFactory, (void**)&m_pCCF);
#endif //HELIX_FEATURE_ALSA_WRITE_PERIOD_SIZE 

#if defined(_THREADED_AUDIO) && defined(_UNIX_THREADS_SUPPORTED)
    HX_ASSERT( m_pContext );
    
    //Find out if the user wants to use threads or not.
    IHXPreferences* pPreferences = NULL;   
    if( m_pContext && HXR_OK == m_pContext->QueryInterface( IID_IHXPreferences, (void **) &pPreferences))
    {
        IHXBuffer *pBuffer = NULL;
        pPreferences->ReadPref("ThreadedAudio", pBuffer);
        if (pBuffer)
        {
            m_bUserWantsThreads = (::atoi((const char*)pBuffer->GetBuffer()) == 1);
            HX_RELEASE(pBuffer);
        }
        HX_RELEASE( pPreferences );
    }    

    if( m_bUserWantsThreads )
    {
        //Initialize the write list and playstate mutex.
	CreateInstanceCCF(CLSID_IHXMutex, (void**)&m_mtxWriteListPlayStateLock, m_pContext);
	CreateInstanceCCF(CLSID_IHXMutex, (void**)&m_mtxDeviceStateLock, m_pContext);
	CreateInstanceCCF(CLSID_IHXThread, (void**)&m_audioThread, m_pContext);
	CreateInstanceCCF(CLSID_IHXEvent, (void**)&m_pAvailableDataEvent, m_pContext);
    }
	if(m_pAvailableDataEvent)
	{
		m_pAvailableDataEvent->Init("Audio_wait",0);
	}
#endif    

}

CAudioOutUNIX::~CAudioOutUNIX()
{
    //We must assume that _Imp_Close has already been called. If not, we are 
    //in big trouble.
    if ( m_wState != RA_AOS_CLOSED ) 
    {
        HX_ASSERT( "Device not closed in dtor." == NULL );
    }

    
#if defined(_THREADED_AUDIO) && defined(_UNIX_THREADS_SUPPORTED)
    //Sanity check that the audio thread has exited.
    //XXXgfw ADD a thread join to HXThread and use it instead
    //       of the cond var below in _close and we won't have
    //       to check here at all.
#ifdef _DEBUG    
    UINT32 unThreadID = 0;
    if( m_audioThread )
    {
        m_audioThread->GetThreadId(unThreadID);
        HX_ASSERT( unThreadID == 0 );
    }
#endif    
#endif

    //Clean up the scheduler.
    HX_RELEASE( m_pScheduler );
    
    //Just in case it isn't empty at this point.
    while( m_pWriteList && !m_pWriteList->IsEmpty() )
    {
        IHXBuffer* pBuffer = (IHXBuffer *)(m_pWriteList->RemoveHead());
        HX_RELEASE( pBuffer );
    }

    HX_DELETE(m_pPlaybackCountCBTime);
    HX_RELEASE(m_pCallback);
    HX_DELETE(m_pWriteList);

    HX_VECTOR_DELETE(m_pRollbackBuffer);
    
#if defined(_THREADED_AUDIO) && defined(_UNIX_THREADS_SUPPORTED)
    if( m_bUserWantsThreads )
    {
        HX_RELEASE( m_mtxWriteListPlayStateLock );
        HX_RELEASE( m_mtxDeviceStateLock );
        HX_RELEASE( m_audioThread );
        HX_RELEASE( m_pAvailableDataEvent );
    }
#endif    

#ifdef HELIX_FEATURE_ALSA_WRITE_PERIOD_SIZE
    HX_RELEASE(m_pCCF);
#endif //HELIX_FEATURE_ALSA_WRITE_PERIOD_SIZE    
}

UINT16 CAudioOutUNIX::_Imp_GetVolume()
{
    if (!m_bMixerPresent)
        _OpenMixer();

    if ( m_bMixerPresent ) 
    {
        m_uCurVolume = _GetVolume();
    }
    return m_uCurVolume;
}

HX_RESULT CAudioOutUNIX::_Imp_SetVolume( const UINT16 uVolume )
{
    HX_RESULT retCode = RA_AOE_NOERR;
    
    //Mixer methods can be called before _Imp_Open
    if( !m_bMixerPresent )
        _OpenMixer();

    //m_uCurVolume is set up at the pnaudev level.
    if( m_bMixerPresent )
    {
        retCode =_SetVolume( uVolume );
    }

    m_wLastError = retCode;
    return m_wLastError;
}


HXBOOL CAudioOutUNIX::_Imp_SupportsVolume()
{
    return TRUE;
}


HX_RESULT CAudioOutUNIX::_Imp_Open( const HXAudioFormat* pFormat )
{
    HX_RESULT retCode = RA_AOE_NOERR;
    
    //Schedule the timer callback...
    if(m_pContext && !m_pScheduler)
    {
        m_pContext->QueryInterface(IID_IHXScheduler, (void**) &m_pScheduler );
    }

    // Check state. Could already be opened.
    if( !IsOpen() && m_wState != RA_AOS_OPENING )
    {
        retCode = _OpenAudio();
        if( retCode == RA_AOE_NOERR ) 
        {
            m_wBlockSize     = m_ulBytesPerGran;
            m_uSampFrameSize
                = pFormat->uBitsPerSample / 8;
            
            //Set device state
            LOCK(m_mtxWriteListPlayStateLock);
            m_wState = RA_AOS_OPENING;
            UNLOCK(m_mtxWriteListPlayStateLock);

            //Configure the audio device.
            retCode = _SetDeviceConfig( pFormat );

            if (retCode != RA_AOE_NOERR) 
            {
                _CloseAudio();
                m_wState=RA_AOS_CLOSED;
            }
            else
            {
#if defined(_THREADED_AUDIO) && defined(_UNIX_THREADS_SUPPORTED)
                //We want to sleep as a function of device buffer size.
                //If we have a small m_ulDeviceBufferSize we can only 
                //afford to sleep just a little while.
                HX_ASSERT( m_ulDeviceBufferSize != 0 );
                m_ulSleepTime = (((float)m_ulDeviceBufferSize/(float)m_uSampFrameSize)/
                                 (float)m_unSampleRate) * 1000 / (float)m_unNumChannels;
#endif
                if (!m_bMixerPresent)
                    _OpenMixer();   

                if( _IsSelectable() )
                {
                    IHXAsyncIOSelection* pAsyncIO = NULL;
                    if( m_pContext && HXR_OK == m_pContext->QueryInterface( IID_IHXAsyncIOSelection, (void**)&pAsyncIO))
                    {
                        pAsyncIO->Add( new HXPlaybackCountCB(this, FALSE), _Imp_GetAudioFd() , PNAIO_WRITE);
                        HX_RELEASE( pAsyncIO );
                    }
                }
            }
        }
        else
        {
            //Couldn't open the audio device.
            //Just pass through and return the error.
            m_wState=RA_AOS_CLOSED;
        }
        
    }

    // If we're optimizing/minimizing heap, we don't want to do this malloc
    // yet, because we're going to change/minize the value of 
    // m_ulDeviceBufferSize later on.
#ifndef HELIX_CONFIG_MIN_ROLLBACK_BUFFER
    //_SetDeviceConfig should have set the m_ulDeviceBufferSize var.
    if( RA_AOE_NOERR == retCode && !_HardwarePauseSupported() )
    {
        HX_ASSERT( m_ulDeviceBufferSize != 0 );
        if( NULL == m_pRollbackBuffer)
        {
            m_pRollbackBuffer = new UCHAR[m_ulDeviceBufferSize];
            memset( m_pRollbackBuffer, '0', m_ulDeviceBufferSize );
        }
    }
#endif // HELIX_CONFIG_MIN_ROLLBACK_BUFFER


#if defined(_THREADED_AUDIO) && defined(_UNIX_THREADS_SUPPORTED)
    //Start up the audio thread if it isn't going allready from a previous open
    //which sometimes happens.
    
    //Make sure we only have one thread going at a time.
    if( m_bUserWantsThreads && RA_AOE_NOERR==retCode )
    {
        UINT32 unThreadID = 0;
        m_audioThread->GetThreadId(unThreadID);
        
        if( unThreadID == 0 )
            m_audioThread->CreateThread( CAudioOutUNIX::AudioThread, this, 0 );
        
        m_audioThread->GetThreadId(unThreadID);
    }
#endif
    m_wLastError = retCode;

#ifdef HELIX_FEATURE_ALSA_WRITE_PERIOD_SIZE
	m_ulALSAPeriodSize = _GetPeriodSize();
#endif //HELIX_FEATURE_ALSA_WRITE_PERIOD_SIZE
    return m_wLastError;
}

HX_RESULT CAudioOutUNIX::_Imp_Close()
{
    HX_RESULT retCode = RA_AOE_NOERR;

    LOCK(m_mtxWriteListPlayStateLock);
    m_wState = RA_AOS_CLOSING;
    UNLOCK(m_mtxWriteListPlayStateLock);


#if defined(_THREADED_AUDIO) && defined(_UNIX_THREADS_SUPPORTED)
    //The audio thread should exit when the state changes to closed.
    //Wait for it to do so and clean up.
    if( m_bUserWantsThreads )
    {
        HXLOGL4 (HXLOG_ADEV, "CAudioUnixOUT::_Imp_Close signaling event..."); 
        m_pAvailableDataEvent->SignalEvent();
        m_audioThread->Exit(0);
    }
    
#endif    


    //Reset the device and empty the write buffer.
    //Don't protect this reset with a lock as the audio thread
    //has alread exited.
    retCode = _Imp_Reset();
    
    if( retCode != RA_AOE_DEVNOTOPEN )
    {
        if( _IsSelectable() )
        {
            IHXAsyncIOSelection* pAsyncIO = NULL;
            
            if( HXR_OK == m_pContext->QueryInterface( IID_IHXAsyncIOSelection,
                                                      (void**)&pAsyncIO)
                )
            {
                pAsyncIO->Remove(_Imp_GetAudioFd() , PNAIO_WRITE);
                HX_RELEASE( pAsyncIO );
            }
        }
    }

    //Close the audio device.
    retCode = _CloseAudio();
    
    //Close the mixer device.
    _CloseMixer();

    //Set state.
    LOCK(m_mtxWriteListPlayStateLock);
    m_wState = RA_AOS_CLOSED;
    UNLOCK(m_mtxWriteListPlayStateLock);

    //Remove callback from scheduler
    if (m_bCallbackPending)
    {
        m_pScheduler->Remove(m_PendingCallbackID);
        m_bCallbackPending = FALSE;
    }

    HX_VECTOR_DELETE(m_pRollbackBuffer);

    //return
    m_wLastError = retCode;
    return m_wLastError;
}


//So far no UN*X platforms support this.
HX_RESULT CAudioOutUNIX::_Imp_Seek(ULONG32 ulSeekTime)
{
    m_wLastError = HXR_OK;
    return m_wLastError;
}

HX_RESULT CAudioOutUNIX::_Imp_Pause()
{
    //XXXgfw We really should be closing the device instead of keeping it open to be nice to
    //other procs.
    LOCK(m_mtxWriteListPlayStateLock);
    m_wState = RA_AOS_OPEN_PAUSED;
    UNLOCK(m_mtxWriteListPlayStateLock);
    
    
    if( !_HardwarePauseSupported() )
    {
        // Strategy.
        //--Find out how much we have left in the device's buffer.
        ULONG32 ulBytesPlayed      = _GetBytesActualyPlayed();
        ULONG32 ulNumBytesToRewind = m_ulTotalWritten-ulBytesPlayed;
        
        //Sometimes we can get a value for bytesPlayed that is slightly
        //more then the amount we have actually written. This results
        //in a huge number.
        if( ulBytesPlayed > m_ulTotalWritten )
            ulNumBytesToRewind = 0;
        
        //--and reset player and discard all the data in the device's buffer
        //We will just ignore an error here. That means the buffer will just drain
        //and we will hear it again when they unpause.
        LOCK(m_mtxDeviceStateLock);
        _Reset();
        UNLOCK(m_mtxDeviceStateLock);

        //Make sure we only deal with full samples. Bytes-per-sample*num-channels.
        int nRem = ulNumBytesToRewind%(m_uSampFrameSize*m_unNumChannels);
        ulNumBytesToRewind = ulNumBytesToRewind>nRem ? ulNumBytesToRewind-nRem : 0;
        // In heap-optimized mode, we accept that we're going to lose a 
        // little bit of data after a pause/resume because we are using as
        // small a rollback buffer as we can get away with.
#ifdef HELIX_CONFIG_MIN_ROLLBACK_BUFFER
	if( ulNumBytesToRewind > m_ulDeviceBufferSize )
	{
	   ulNumBytesToRewind = m_ulDeviceBufferSize;
	}
#endif // HELIX_CONFIG_MIN_ROLLBACK_BUFFER

        //  and add it to the front of the write buffer.
        IHXBuffer* pNewBuffer = NULL;
	if (HXR_OK == CreateAndSetBufferCCF(pNewBuffer,
					    m_pRollbackBuffer+m_ulDeviceBufferSize-ulNumBytesToRewind,
					    ulNumBytesToRewind,
					    m_pContext))
	{
	    LOCK(m_mtxWriteListPlayStateLock);
	    m_pWriteList->AddHead(pNewBuffer);        
	    UNLOCK(m_mtxWriteListPlayStateLock);
	}

        //--Subtract that from m_ulTotalWritten.
        m_ulTotalWritten -= ulNumBytesToRewind;
        
        _Pause();

    }
    else
    {
        //The hardware device handles the Pause/Resume.
        LOCK(m_mtxDeviceStateLock);
        _Pause();
        UNLOCK(m_mtxDeviceStateLock);
    }
    
    m_wLastError = HXR_OK;
    return m_wLastError;
}

HX_RESULT CAudioOutUNIX::_Imp_Resume()
{
    //XXXgfw We really should be closing and re-opening the device to be nice to other procs.
    LOCK(m_mtxWriteListPlayStateLock);
    m_wState = RA_AOS_OPEN_PLAYING;
    UNLOCK(m_mtxWriteListPlayStateLock);

    //XXXgfw If the two branches of the if are the same maybe get rid of one????
    if( !_HardwarePauseSupported() )
    {
        _Resume();
        _Imp_Write(NULL);
    }
    else
    {
        //The hardware device handles the Pause/Resume.
        _Resume();
        _Imp_Write(NULL);
    }
    
    m_wLastError = HXR_OK;
    return m_wLastError;
}

HX_RESULT CAudioOutUNIX::_Imp_Reset()
{
    HX_RESULT retCode = RA_AOE_NOERR;
    
    if ( m_wState != RA_AOS_CLOSED )
    {
        LOCK(m_mtxDeviceStateLock);
        retCode = _Reset();
        UNLOCK(m_mtxDeviceStateLock);
        LOCK(m_mtxWriteListPlayStateLock);
        while (m_pWriteList && m_pWriteList->GetCount() > 0)
        {
            IHXBuffer* pBuffer = (IHXBuffer *)(m_pWriteList->RemoveHead());
            HX_RELEASE( pBuffer );
        }
        UNLOCK(m_mtxWriteListPlayStateLock);

        m_ulTotalWritten  = 0;
        m_bFirstWrite     = TRUE;
        m_ulLastNumBytes  = 0;
    }
    
    m_wLastError = retCode;
    return m_wLastError;
}

HX_RESULT CAudioOutUNIX::_Imp_Drain()
{
    HX_RESULT retCode = RA_AOE_NOERR;

    LOCK(m_mtxWriteListPlayStateLock);

    if( m_wState != RA_AOS_CLOSED )
    {
        retCode = _Drain();
    }
    while( m_pWriteList && !m_pWriteList->IsEmpty() )
    {
        IHXBuffer* pBuffer = (IHXBuffer *)(m_pWriteList->RemoveHead());
        HX_RELEASE( pBuffer );
    }

    UNLOCK(m_mtxWriteListPlayStateLock);

    
    m_wLastError = retCode;
    return m_wLastError;
}


HX_RESULT CAudioOutUNIX::_Imp_CheckFormat( const HXAudioFormat* pFormat )
{
    HX_RESULT retCode = RA_AOE_NOERR;
    m_wLastError       = HXR_OK;

    retCode = _CheckFormat( pFormat );
    if( RA_AOE_NOERR != retCode && RA_AOE_DEVBUSY != retCode )
    {
        m_wLastError = HXR_FAILED;
    }
    else
    {
        m_wLastError = HXR_OK;
    }
    

    return m_wLastError;
}

HX_RESULT CAudioOutUNIX::_Imp_GetCurrentTime( ULONG32& ulCurrentTime )
{
    ULONG32 ulTime   = 0;
    UINT64  ulBytes  = 0;

    LOCK(m_mtxWriteListPlayStateLock);
    ulBytes = _GetBytesActualyPlayed();
    UNLOCK(m_mtxWriteListPlayStateLock);
    
    ulTime = (ULONG32)(( ( (double)ulBytes/(double)m_uSampFrameSize)/(double)m_unSampleRate) * 1000.0 / (double)m_unNumChannels);
    
    //Not used anywhere but belongs to CHXAudioDevice so we must set it.
    m_ulCurrentTime  = ulTime;

    //Set the answer.
    ulCurrentTime = ulTime;

    m_wLastError = HXR_OK;
    return HXR_OK;

}

void CAudioOutUNIX::DoTimeSyncs()
{
    ReschedPlaybackCheck();
    OnTimeSync();
    return;
}


HX_RESULT CAudioOutUNIX::ReschedPlaybackCheck()
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

UINT16 CAudioOutUNIX::_NumberOfBlocksRemainingToPlay(void)
{
    UINT32 bytesBuffered = 0;

    //We have to go through all of the buffers and count the size. Even though
    //we always write m_wBlockSize buffers, we can get off-sized buffs because
    //of the pause/resume code. When we pause we rewind however many bytes have
    //not been played yet in the buffer.
    
    LOCK(m_mtxWriteListPlayStateLock);
    if( m_pWriteList )
    {
        LISTPOSITION i = m_pWriteList->GetHeadPosition();
        
        while( i )
        {
            bytesBuffered += ((IHXBuffer *)m_pWriteList->GetAt(i)) -> GetSize();
            m_pWriteList->GetNext(i);
        }
    }
    bytesBuffered += (m_ulTotalWritten - _GetBytesActualyPlayed());    
    UNLOCK(m_mtxWriteListPlayStateLock);

    return bytesBuffered / m_wBlockSize + 1;
}


CAudioOutUNIX::HXPlaybackCountCB::~HXPlaybackCountCB()
{
}

STDMETHODIMP CAudioOutUNIX::HXPlaybackCountCB::QueryInterface(	REFIID riid, void** ppvObj )
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

STDMETHODIMP_(ULONG32) CAudioOutUNIX::HXPlaybackCountCB::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32) CAudioOutUNIX::HXPlaybackCountCB::Release()
{
    
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }
    
    delete this;
    return HXR_OK;
}

STDMETHODIMP CAudioOutUNIX::HXPlaybackCountCB::Func(void)
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
            m_pAudioDeviceObject->_Imp_Write(NULL);
            m_pAudioDeviceObject->DoTimeSyncs();
        }
    }

    return HXR_OK;
}


HX_RESULT CAudioOutUNIX::_Pause()
{
    return RA_AOE_NOTSUPPORTED;
}

HX_RESULT CAudioOutUNIX::_Resume()
{
    return RA_AOE_NOTSUPPORTED;
}

HXBOOL CAudioOutUNIX::_IsSelectable() const
{
    return TRUE;
}

HXBOOL CAudioOutUNIX::_HardwarePauseSupported() const
{
    return FALSE;
}











#if defined(_THREADED_AUDIO) && defined(_UNIX_THREADS_SUPPORTED)
void* CAudioOutUNIX::AudioThread(void *thisPointer )
{
    HXBOOL bReadyToExit=FALSE;
    CAudioOutUNIX* that = (CAudioOutUNIX*)thisPointer;

    while(!bReadyToExit)
    {
        //If you want to destroy frame rate just sit on this lock for 
        //a while. If you don't, make sure this is a *very* fast loop.
        that->m_mtxWriteListPlayStateLock->Lock();
        that->m_mtxDeviceStateLock->Lock();
        if(that->m_wState!=RA_AOS_CLOSED && that->m_wState!=RA_AOS_CLOSING)
        {
            if( that->m_pWriteList->GetCount() > 0 && that->m_wState == RA_AOS_OPEN_PLAYING )
            {
                that->_PushBits();
            }
        }
        else
        {
            bReadyToExit=TRUE;
        }
        that->m_mtxDeviceStateLock->Unlock();
        that->m_mtxWriteListPlayStateLock->Unlock();

        if(bReadyToExit == FALSE) 
	{
	    if (that->m_pWriteList->GetCount() == 0 || that->m_wState == RA_AOS_OPEN_PAUSED)
	    {
	        HXLOGL4 (HXLOG_ADEV, "CAudioUnixOUT::AudioThread() waiting for audio data..."); 
	        that->m_pAvailableDataEvent->Wait(ALLFS);
            } 
            else
	    {
#if !defined(HELIX_FEATURE_ALSA)
	        // OK, sleep the amount of time it takes to play 1/4 of the device's buffer.
	        microsleep(that->m_ulSleepTime/4); 
#endif
	    }
	}
    }

    //Signal the parent thread that we are done.
    that->m_audioThread->Exit(0);

    return (void*)0;
}
#endif





// Don't protect anything in this method with mutexes. It has 
// already been done from where it is called from. But mostly
// because we aren't using recursive mutexes yet.
ULONG32 CAudioOutUNIX::_PushBits()
{
    IHXBuffer* pBuffer  = NULL;
    UCHAR*      pData    = 0;
    ULONG32     ulBufLen = 0;
    LONG32      lCount   = 0;

    //We are going to try and write. Grab the head and do it.
    pBuffer  = (IHXBuffer*)m_pWriteList->RemoveHead();
    pData    = pBuffer->GetBuffer();
    ulBufLen = pBuffer->GetSize();
    //
    // Now, in case there is no mixer present, do the volume manually.
    //

    UCHAR* pNoVolumeData = NULL;
    
    if ( !m_bMixerPresent )
    {
        //Save a copy of the non-volume mutated sound data. If we don't, our
        //rewind buffer will fill up with volume modified data and it will get
        //run through the manual sound code again.
        if( !_HardwarePauseSupported() )
        {
            pNoVolumeData = new UCHAR[ ulBufLen ];
            memcpy( pNoVolumeData, pData, ulBufLen ); /* Flawfinder: ignore */
        }
        
        int i, j = ulBufLen / sizeof(short);
        short *s = (short *)pData;
        for (i = 0 ; i < j ; i++) 
        {
            s[i] = ((int)s[i]) * (m_uCurVolume) / MAX_VOLUME;
        }
    }

    UNLOCK(m_mtxWriteListPlayStateLock);
    HXLOGL4 (HXLOG_ADEV, "CAudioUnixOUT::_PushBits() writing %i bits", (int)ulBufLen);
    _WriteBytes(pData, ulBufLen, lCount);
    LOCK(m_mtxWriteListPlayStateLock);

    //Make sure we wrote the whole buffer like we wanted to.
    if( lCount!=-1 && lCount != ulBufLen)
    {
        //In release we can ignore this...I guess. But why didn't we?
        HX_ASSERT( "We ALWAYS write full buffers!" == NULL );
    }
    
    // Check for bad write...then check errno; write could be interrupted
    // by setitimer and sigalarm. If so, loop and try writing again.
    if ( lCount == -1 )
    {
        //EAGAIN should *never* happen as we are checking to make sure we don't
        //block on this call....
        if ( errno == EAGAIN) 
        {
            HX_ASSERT( "We shouldn't be blocking here..."==NULL);
        }
    }

    if( lCount > 0 )
    {
        m_ulTotalWritten += lCount;

        //We only need to do this if the hardware doesn't support pause.
        if( !_HardwarePauseSupported() )
        {
            // If we wrote to the device we need to keep a copy of the 
            // data our device buffer. We use this to 'rewind' the data
            // in case we get paused.
            //If we could write lCount without blocking then there was at 
            //least that much room in the device and since m_pRollbackBuffer
            //is as large as the devices buffer, we can safely shift and copy.
            //Add the new stuff to the end pushing the rest of the data forward.

            //We need this sanity check to prevent a very hard bug to find.
            //Don't get rid of it! If this asserts it *really* means something.
            //The first thing to check is the __powerpc defines that determine
            //buffer size.
            HX_ASSERT( lCount <= m_ulDeviceBufferSize );

            // Ok, now that we've asserted to catch the
            // bug, let's prevent the crash.
            if (lCount > m_ulDeviceBufferSize) lCount = m_ulDeviceBufferSize;

            memmove( m_pRollbackBuffer, m_pRollbackBuffer+lCount, m_ulDeviceBufferSize-lCount); /* Flawfinder: ignore */
            
            //Now copy in the new data at the end of the buffer.
            if( NULL != pNoVolumeData )
                memcpy( m_pRollbackBuffer+m_ulDeviceBufferSize-lCount, pNoVolumeData, lCount ); /* Flawfinder: ignore */
            else
                memcpy( m_pRollbackBuffer+m_ulDeviceBufferSize-lCount, pData, lCount );  /* Flawfinder: ignore */
        }
    }

    HX_VECTOR_DELETE(pNoVolumeData);
    HX_RELEASE( pBuffer );
        
    return lCount;
}



HX_RESULT CAudioOutUNIX::_Imp_Write( const HXAudioData* pAudioOutHdr )
{
    IHXBuffer* pBuffer  = NULL;
    ULONG32     ulCount  = 0;

    
    //_Imp_Write won't be called if the device couldn't be opened. But,
    //Just in case.
    if( !IsOpen() )
    {
        return RA_AOE_DEVNOTOPEN;
    }

    //Schedule callbacks.
    if( m_bFirstWrite && pAudioOutHdr )
    {
        m_bFirstWrite = FALSE;

        //  Initialize the playback callback time.
        HXTimeval lTime = m_pScheduler->GetCurrentSchedulerTime();
        m_pPlaybackCountCBTime->tv_sec = lTime.tv_sec;
        m_pPlaybackCountCBTime->tv_usec = lTime.tv_usec;

        //  Scheduler playback callback.
        ReschedPlaybackCheck();
    }

    //Blindly add the incoming data to the tail of the writelist.
    //Some audio devices have very small device buffers. If this
    //buffer is smaller than our incoming block size we must break
    //up the incoming buffer into smaller chuncks. We want to be
    //able to fix *at least* two(1.5?) chunks into the buffer so we don't
    //block all the time.
    if( pAudioOutHdr != NULL )
    {
        LOCK(m_mtxWriteListPlayStateLock);
        int nBuffSize = pAudioOutHdr->pData->GetSize();
        // In heap-optimized mode, this is where we set the value of 
        // m_ulDeviceBufferSize, and malloc the buffer.
#ifdef HELIX_CONFIG_MIN_ROLLBACK_BUFFER

        //OPTIMIZED_MIXING conflicts with MIN_ROLLBACK_BUFFER feature.
        //This is because the logic here assumes that the audio device
        //Write() method is always called with a constant size -- the block size,
        //while in OPTIMIZED_MIXING mode, session will write partial blocks to avoid
        ///memory copying.
        //TODO: MIN_ROLLBACK_BUFFER logic should be improved so that it gets
        //the block size from audio session through a new API and reduce the rollback size 
        //to the block size, rather than second guessing it from Write() call here.
        //Error out for now if both features are defined.
#ifdef HELIX_FEATURE_OPTIMIZED_MIXING
#error "Can not have both HELIX_FEATURE_OPTIMIZED_MIXING and HELIX_CONFIG_MIN_ROLLBACK_BUFFER"
#endif

	if( m_ulDeviceBufferSize != nBuffSize )
	{
            m_ulDeviceBufferSize = nBuffSize;
    	    HX_VECTOR_DELETE(m_pRollbackBuffer);
            m_pRollbackBuffer = new UCHAR[m_ulDeviceBufferSize];
	}
#endif // HELIX_CONFIG_MIN_ROLLBACK_BUFFER
	// Shouldn't this be ">=" and not ">"?
        if (m_ulALSAPeriodSize == 0)
	{
        if( m_ulDeviceBufferSize >= nBuffSize )
        {
            //No need to break it up.
            IHXBuffer* pTmpBuff = pAudioOutHdr->pData;
            m_pWriteList->AddTail(pTmpBuff);
            pTmpBuff->AddRef();
        }
        else
        {
            //Break it up. First find the correct size.
            while( nBuffSize >= m_ulDeviceBufferSize )
                nBuffSize = nBuffSize>>1;

            //Now make sure we never break a buffer apart in the
            //middle of a Frame.
            int nRem = nBuffSize%(m_uSampFrameSize*m_unNumChannels);
            nBuffSize = nBuffSize>nRem ? nBuffSize-nRem : nBuffSize ;

            //March through the buffer, breaking it up.
            UCHAR* pData  = pAudioOutHdr->pData->GetBuffer();
            int    nLimit = pAudioOutHdr->pData->GetSize();
            int    nPtr   = 0;
            while( nPtr < nLimit )
            {
                IHXBuffer* pNewBuffer = NULL;
		if (HXR_OK == CreateBufferCCF(pNewBuffer, m_pContext))
		{
		    if( nPtr+nBuffSize <= nLimit )
		    {
			pNewBuffer->Set( pData+nPtr, nBuffSize );
		    }
		    else
		    {
			pNewBuffer->Set( pData+nPtr, (nLimit-nPtr) );
		    }
		    m_pWriteList->AddTail( pNewBuffer );
		}
                nPtr += nBuffSize;
            }
        }
	}
#ifdef HELIX_FEATURE_ALSA_WRITE_PERIOD_SIZE	
	else
	{
		if (nBuffSize % m_ulALSAPeriodSize == 0 && m_bufferList.IsEmpty())
		{
			//m_bufferList will be empty after creating a buffer equal to the m_ulALSAPeriodSize;
			//if next packet is just equal to the m_ulALSAPeriodSize then add it to the write list
			IHXBuffer* pTmpBuff = pAudioOutHdr->pData;
			m_pWriteList->AddTail(pTmpBuff);
			pTmpBuff->AddRef();
		}
		else
		{
			UCHAR* pData  = pAudioOutHdr->pData->GetBuffer();
			int  nLimit  = pAudioOutHdr->pData->GetSize();		
			if(nLimit > 0 && (nLimit + m_ulByteCount) < m_ulALSAPeriodSize)
			{
				AddToBufferList(pAudioOutHdr->pData);
			}
			else         
        		{
				//(((nLimit+m_ulByteCount) % m_ulALSAPeriodSize ) >= 0)
				IHXBuffer* pBuf = NULL;
				int ulremainder = (nLimit+m_ulByteCount) % m_ulALSAPeriodSize;
				int ulNewValue = (nLimit+m_ulByteCount) - ulremainder;
				if ((HXR_OK == m_pCCF->CreateInstance(CLSID_IHXBuffer, (void**)&pBuf)) &&
				(HXR_OK == pBuf->SetSize(ulNewValue)))
				{
					UCHAR* pCurPos = pBuf->GetBuffer();
            
					/* Copy the buffers in the buffer list */
					LISTPOSITION pos = m_bufferList.GetHeadPosition();
					while(pos)
					{
						IHXBuffer* pTmpBuf = (IHXBuffer*)m_bufferList.GetNext(pos);
						memcpy(pCurPos, pTmpBuf->GetBuffer(), pTmpBuf->GetSize());
						pCurPos += pTmpBuf->GetSize();
					}

					if (pData != NULL)
					{	
					    memcpy(pCurPos, pData, nLimit-ulremainder);  // nLimit-ulRemainder
					    pCurPos += nLimit;						
					}
					pBuf->AddRef();
					m_pWriteList->AddTail(pBuf);
					ClearBufferList();
				}	
				HX_RELEASE(pBuf);
				IHXBuffer* pRemBufffer = NULL;
				if (ulremainder && pAudioOutHdr->pData && HXR_OK == CHXNestedBuffer::CreateNestedBuffer(pAudioOutHdr->pData, (nLimit-ulremainder),ulremainder, pRemBufffer))
				{
					AddToBufferList(pRemBufffer);	
				}
				HX_RELEASE(pRemBufffer);
			}
		}
	}
#endif //HELIX_FEATURE_ALSA_WRITE_PERIOD_SIZE
        UNLOCK(m_mtxWriteListPlayStateLock);
    }


#if defined(_THREADED_AUDIO) && defined(_UNIX_THREADS_SUPPORTED)
    //If we are using threaded audio just return and let the audio thread
    //grab the data and write it to the device.
    if( m_bUserWantsThreads )
    {
        HXLOGL4 (HXLOG_ADEV, "CAudioUnixOUT::_Imp_Write signaling event..."); 
        m_pAvailableDataEvent->SignalEvent();
        return RA_AOE_NOERR;
    }
#endif    

    // ----------- Threaded audio note:
    //
    // We don't need to protect anything below here with the mutex
    // because we will never get here if we are running with threaded
    // audio.
    //


    //XXXgfw This doesn't seem to be doing any good anymore. So I am taking it
    //       out
//      //Check for underflow here and do what? Pause it?
//      if( m_wState!=RA_AOS_OPEN_PAUSED && m_pWriteList->GetCount() <= 0 )
//      {  
//          ULONG32 ulTmp  = 0;
        
//          _GetRoomOnDevice(ulTmp);
//          //there better be stuff in the device because we wouldn't get this
//          //far if we were not in an OPEN/Playing state.
//          if( ulTmp >= m_ulDeviceBufferSize )
//          {
//              //XXXgfw, read below...
//              //We aren't writting anything and there is no 
//              //data in the device buffer. Lions, tigers and bears..
//              //Maybe we should just do a _Pause here to keep
//              //the byte count from the audio device correct?
//              HX_ASSERT( "UNDERFLOW in AUDIO DEVICE" == NULL );
//          }
//      }

    //Just return if there is nothing to do 
    if( m_pWriteList->GetCount() <= 0 || m_wState==RA_AOS_OPEN_PAUSED )
    {
        return RA_AOE_NOERR;
    }

    // 
    // We only want to write to the device if we can send the
    // whole buffer. If there isn't that much room then just
    // put the buffer back in the front of the que.
    //
    ULONG32   ulBytesAvailable = 0;
    HX_RESULT code             = RA_AOE_NOERR;
    
    code = _GetRoomOnDevice(ulBytesAvailable);
    if( RA_AOE_NOERR != code )
    {
        //What? Can't get the room on the device for some reason.
        m_wLastError = code;
        return m_wLastError;
    }

    //Peek at the head.
    pBuffer = (IHXBuffer*)m_pWriteList->GetHead();
    if( NULL==pBuffer || ulBytesAvailable < pBuffer->GetSize())
    {
        m_wLastError = RA_AOE_NOERR;
        return m_wLastError;
    }
    
    ulCount = _PushBits();
    
    if ( m_bFirstWrite )
    {
        m_bFirstWrite = FALSE;
            
        /*  Initialize the playback callback time. */
        HXTimeval lTime = m_pScheduler->GetCurrentSchedulerTime();
        m_pPlaybackCountCBTime->tv_sec = lTime.tv_sec;
        m_pPlaybackCountCBTime->tv_usec = lTime.tv_usec;
            
        /*  Scheduler playback callback. */
        ReschedPlaybackCheck();
    }

    //make sure the device is kept full.
    if( m_pWriteList->GetCount()>0 && ulCount>0 )
        _Imp_Write(NULL);

    return m_wLastError;
    
}

HX_RESULT CAudioOutUNIX::_CheckFormat( const HXAudioFormat* pFormat )
{
    return RA_AOE_NOERR;
}

#ifdef HELIX_FEATURE_ALSA_WRITE_PERIOD_SIZE
HX_RESULT CAudioOutUNIX::AddToBufferList(IHXBuffer* pBuf)
{
    if (pBuf)
    {
        pBuf->AddRef();    
        m_ulByteCount += pBuf->GetSize();
        
        /* Transfering ownership here.
         * We don't need to call HX_RELEASE()
         * on pBuf
         */
        m_bufferList.AddTail(pBuf);
    }

    return HXR_OK;
}

void CAudioOutUNIX::ClearBufferList()
{
    while(!m_bufferList.IsEmpty())
    {
        IHXBuffer* pBuf = (IHXBuffer*)m_bufferList.RemoveHead();
        HX_RELEASE(pBuf);
    }

    m_ulByteCount = 0;
}
#endif // HELIX_FEATURE_ALSA_WRITE_PERIOD_SIZE

