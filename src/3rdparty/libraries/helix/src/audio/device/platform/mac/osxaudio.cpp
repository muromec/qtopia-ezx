/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: osxaudio.cpp,v 1.7 2008/05/02 23:06:41 bobclark Exp $
 * 
 * Portions Copyright (c) 1995-2006 RealNetworks, Inc. All Rights Reserved.
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

#include "osxaudio.h"
#include "hxthread.h"

HXMutex* CAudioOutOSX::zm_pMutex = NULL;

CAudioOutOSX::CAudioOutOSX()
 : mAudioDevice(0)
 , mDeviceNumberOfChannels(0)
 , mInputNumberOfChannels(0)
 , mStaleBytesInFirstBuffer(0)
 , mCurrentTime(0)
 , mResetTimeNanos(0)
 , mElapsedNanos(0)
 , mElapsedNanosAtPause(0)
 , mNanoSecondsThatCoreAudioDrynessOccurred(0)
 , mAccumulatedNanoSecondsOfCoreAudioDryness(0)
 , mCurrentVolume(75.0)
{
    OSStatus err = noErr;
    UInt32 dataSize;
    
    dataSize = sizeof(mAudioDevice);
    
    if (err == noErr)
    {
        err = ::AudioHardwareGetProperty(kAudioHardwarePropertyDefaultOutputDevice, &dataSize, &mAudioDevice);
    }
    
    if (err == noErr)
    {
        err = ::AudioDeviceAddIOProc(mAudioDevice, HALIOProc, (void*)this);
    }
    
    if (err == noErr)
    {
        err = ::AudioDeviceGetPropertyInfo(mAudioDevice, 0, false, kAudioDevicePropertyStreamConfiguration, &dataSize, NULL);
    }
    if (err == noErr)
    {
        AudioBufferList* buflist = (AudioBufferList*)new char[dataSize];
        err = ::AudioDeviceGetProperty(mAudioDevice, 0, false, kAudioDevicePropertyStreamConfiguration, &dataSize, buflist);
        int i;
        for (i = 0; i < buflist->mNumberBuffers; i++)
        {
            mDeviceNumberOfChannels += buflist->mBuffers[i].mNumberChannels;
        }
        delete[] (char*)buflist;
    }
    
    if (!zm_pMutex)
    {
        HXMutex::MakeMutex(zm_pMutex);
    }
}

CAudioOutOSX::~CAudioOutOSX()
{
    OSStatus err = noErr;
    
    if (err == noErr)
    {
        err = ::AudioDeviceRemoveIOProc(mAudioDevice, HALIOProc);
    }
}

HX_RESULT
CAudioOutOSX::_Imp_Open( const HXAudioFormat* pFormat )
{
    OSStatus err = noErr;
    
    mInputNumberOfChannels = pFormat->uChannels;
    
    HX_ASSERT(mInputNumberOfChannels > 0);
    if (mInputNumberOfChannels == 0)
    {
        mInputNumberOfChannels = 1;
    }
    
    Float64 sampleRate = (Float64)pFormat->ulSamplesPerSec;
    UInt32 dataSize = sizeof(sampleRate);
    err = ::AudioDeviceSetProperty(mAudioDevice, NULL, 0, false, kAudioDevicePropertyNominalSampleRate, dataSize, &sampleRate);
    
    if (err == noErr)
    {
        err = ::AudioDeviceStart(mAudioDevice, HALIOProc);
    }
    
    mStaleBytesInFirstBuffer = 0;
    
    if (err == noErr)
    {
        return HXR_OK;
    }
    
    return HXR_AUDIO_DRIVER;
}

HX_RESULT
CAudioOutOSX::_Imp_Close(void)
{
    OSStatus err = noErr;
    
    if (err == noErr)
    {
        err = ::AudioDeviceStop(mAudioDevice, HALIOProc);
    }
    
    return HXR_OK;
}

HX_RESULT
CAudioOutOSX::_Imp_Seek(ULONG32 ulSeekTime)
{
    return HXR_OK;
}

HX_RESULT
CAudioOutOSX::_Imp_Pause(void)
{
    HX_RESULT retVal = HXR_OK;
    
    OSStatus err = noErr;
    
    if (err == noErr)
    {
        err = ::AudioDeviceStop(mAudioDevice, HALIOProc);
    }
    
    if (err != noErr)
    {
        retVal = HXR_AUDIO_DRIVER;
    }
    
    mElapsedNanosAtPause = mElapsedNanos;
    
    return retVal;
}

HX_RESULT
CAudioOutOSX::_Imp_Resume(void)
{
    OSStatus err = noErr;
    
    mResetTimeNanos = ::AudioConvertHostTimeToNanos(::AudioGetCurrentHostTime());
    
    mCurrentTime = mResetTimeNanos;
    mResetTimeNanos -= mElapsedNanosAtPause;
    mElapsedNanosAtPause = 0;
    mNanoSecondsThatCoreAudioDrynessOccurred = 0;
    mAccumulatedNanoSecondsOfCoreAudioDryness = 0;
    
    err = ::AudioDeviceStart(mAudioDevice, HALIOProc);
    if (err != noErr)
    {
        return HXR_AUDIO_DRIVER;
    }
    
    // This is important!
    OnTimeSync();
    
    return HXR_OK;
}

HX_RESULT
CAudioOutOSX::_Imp_Write(const HXAudioData* pAudioOutHdr)
{
    HX_ADDREF(pAudioOutHdr->pData);
    
    HX_LOCK(zm_pMutex);
    mAudioBuffers.AddTail(pAudioOutHdr->pData);
    HX_UNLOCK(zm_pMutex);
    
    return HXR_OK;
}

HX_RESULT
CAudioOutOSX::_Imp_Reset(void)
{
    _Imp_Drain();
    mResetTimeNanos = ::AudioConvertHostTimeToNanos(::AudioGetCurrentHostTime());
    mCurrentTime = mResetTimeNanos;
    mElapsedNanos = 0;
    mElapsedNanosAtPause = 0;
    mNanoSecondsThatCoreAudioDrynessOccurred = 0;
    mAccumulatedNanoSecondsOfCoreAudioDryness = 0;
    return HXR_OK;
}

HX_RESULT
CAudioOutOSX::_Imp_Drain(void)
{
    HX_LOCK(zm_pMutex);
    while (mAudioBuffers.GetCount())
    {
        IHXBuffer* pHeadBuffer = (IHXBuffer*)mAudioBuffers.RemoveHead();
        HX_RELEASE(pHeadBuffer);
    }
    HX_UNLOCK(zm_pMutex);
    
    mStaleBytesInFirstBuffer = 0;
    return HXR_OK;
}

HXBOOL
CAudioOutOSX::_Imp_SupportsVolume(void)
{
    return TRUE;
}

UINT16
CAudioOutOSX::_Imp_GetVolume(void)
{
    return (UINT16)(mCurrentVolume * 100.0 + 0.5);
}

HX_RESULT
CAudioOutOSX::_Imp_SetVolume(const UINT16 uVolume)
{
    mCurrentVolume = (double)uVolume / 100.0;
    
    // mCurrentVolume must be between 0.0 and 1.0
    if (mCurrentVolume > 1.0)
    {
        mCurrentVolume = 1.0;
    }
    return HXR_OK;
}

HX_RESULT
CAudioOutOSX::_Imp_CheckFormat(const HXAudioFormat* pFormat)
{
    // first make sure that we have >= the number of channels requested.
    
    if (pFormat->uChannels > mDeviceNumberOfChannels)
    {
        // if it's asking for more channels than we have,
        // bail out so it asks again with fewer channels.
        return HXR_AUDIO_DRIVER;
    }
    
    // next bail out for single-channel requests: we want at least stereo!
    
    if (pFormat->uChannels < 2)
    {
        return HXR_AUDIO_DRIVER;
    }
    
    // next make sure that we support the sample rate requested.
    
    UInt32 dataSize;
    OSStatus err = ::AudioDeviceGetPropertyInfo(mAudioDevice, 0, false, kAudioDevicePropertyAvailableNominalSampleRates, &dataSize, NULL);
    if (err == noErr)
    {
        bool bSupportsSampleRate = false;
        
        AudioValueRange* pAudioValueRange = (AudioValueRange*)new char[dataSize];
        err = ::AudioDeviceGetProperty(mAudioDevice, 0, false, kAudioDevicePropertyAvailableNominalSampleRates, &dataSize, pAudioValueRange);
        int i;
        for (i = 0; i < dataSize / sizeof(AudioValueRange); i++)
        {
            if (((Float64)pFormat->ulSamplesPerSec >= pAudioValueRange[i].mMinimum)
                && ((Float64)pFormat->ulSamplesPerSec <= pAudioValueRange[i].mMaximum))
            {
                bSupportsSampleRate = true;
                break;
            }
        }
        delete[] (char*)pAudioValueRange;
        if (bSupportsSampleRate)
        {
            return HXR_OK;
        }
    }
    
    return HXR_AUDIO_DRIVER;
}

HX_RESULT
CAudioOutOSX::_Imp_GetCurrentTime(ULONG32& ulCurrentTime)
{
    if (mElapsedNanosAtPause)
    {
        // if we're paused, return the paused-at time
        ulCurrentTime = (ULONG32)(mElapsedNanosAtPause / (UInt64)1000000);
    }
    else
    {
        mElapsedNanos = mCurrentTime - mResetTimeNanos;
        ulCurrentTime = (ULONG32)(mElapsedNanos / (UInt64)1000000);
    }
    return HXR_OK;
}

OSStatus
CAudioOutOSX::CoreAudioCallback(const AudioTimeStamp* inCurTime, AudioBufferList* outOutputData)
{
    size_t bytesWritten = 0;
    size_t inputFrameSizeInBytes = mInputNumberOfChannels * sizeof(INT16);
    size_t outputFrameSizeInBytes = mDeviceNumberOfChannels * sizeof(Float32);
    
    IHXBuffer* pHeadDataBuffer = NULL;
    
    HX_LOCK(zm_pMutex);
    if (mAudioBuffers.GetCount() > 0)
    {
        pHeadDataBuffer = (IHXBuffer*)mAudioBuffers.GetHead();
    }
    HX_UNLOCK(zm_pMutex);
    
    while (pHeadDataBuffer
        && (bytesWritten < outOutputData->mBuffers[0].mDataByteSize))
    {
        INT16* src = (INT16*)(pHeadDataBuffer->GetBuffer() + mStaleBytesInFirstBuffer);
        Float32* dst = (Float32*)((char*)outOutputData->mBuffers[0].mData + bytesWritten);
        // write a frame
        
        int whichChan;
        for (whichChan = 0; whichChan < mInputNumberOfChannels; whichChan++)
        {
            dst[whichChan] = (Float32)src[whichChan] / 32768.0 * mCurrentVolume;
        }
        for (whichChan = mInputNumberOfChannels; whichChan < mDeviceNumberOfChannels; whichChan++)
        {
            dst[whichChan] = 0.0; // yikes, extra output channels!
        }
        
        mStaleBytesInFirstBuffer += inputFrameSizeInBytes;
        if (mStaleBytesInFirstBuffer >= pHeadDataBuffer->GetSize())
        {
            HX_RELEASE(pHeadDataBuffer);
            
            HX_LOCK(zm_pMutex);
            mAudioBuffers.RemoveHead();
            mStaleBytesInFirstBuffer = 0;
            if (mAudioBuffers.GetCount() > 0)
            {
                pHeadDataBuffer = (IHXBuffer*)mAudioBuffers.GetHead();
            }
            HX_UNLOCK(zm_pMutex);
        }
        
        bytesWritten += outputFrameSizeInBytes;
    }
    
    UInt64 inNano = ::AudioConvertHostTimeToNanos(inCurTime->mHostTime);
    
    if (outOutputData->mBuffers[0].mDataByteSize > bytesWritten)
    {
        // we're experiencing dryness
        
        UInt64 bytesPerSecond = (UInt64)(m_AudioFmt.uBitsPerSample/8)
                                * (UInt64)(m_AudioFmt.uChannels) * (UInt64)(m_AudioFmt.ulSamplesPerSec);
        
        UInt64 nanoSecondsPlayed = (UInt64)bytesWritten * (UInt64)1000000000 / bytesPerSecond;
        
        if (!mNanoSecondsThatCoreAudioDrynessOccurred)
        {
            mNanoSecondsThatCoreAudioDrynessOccurred = mCurrentTime + nanoSecondsPlayed;
        }
        
        mCurrentTime += nanoSecondsPlayed; // estimate timeline progression since this chunk may only be "partly dry"
    }
    else
    {
        if (mNanoSecondsThatCoreAudioDrynessOccurred)
        {
            // we're just coming out of "dry mode"
            mAccumulatedNanoSecondsOfCoreAudioDryness = inNano - mNanoSecondsThatCoreAudioDrynessOccurred;
            mNanoSecondsThatCoreAudioDrynessOccurred = 0;
        }
        
        mCurrentTime = inNano - mAccumulatedNanoSecondsOfCoreAudioDryness;
    }
    
    return noErr;
}

OSStatus
CAudioOutOSX::HALIOProc( AudioDeviceID theDevice,
                    const AudioTimeStamp* inCurTime,
                    const AudioBufferList* inInputData,
                    const AudioTimeStamp* inInputTime,
                    AudioBufferList* outOutputData,
                    const AudioTimeStamp* inOutputTime,
                    void* refcon
                    )
{
    CAudioOutOSX* pThis = (CAudioOutOSX*)refcon;
    
    return pThis->CoreAudioCallback(inCurTime, outOutputData);
}


