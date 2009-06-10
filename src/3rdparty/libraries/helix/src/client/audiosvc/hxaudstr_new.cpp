/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxaudstr_new.cpp,v 1.47 2009/05/01 14:09:35 sfu Exp $
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

#include "hlxclib/stdio.h" 
#include "hlxclib/string.h"



#include "hxresult.h"
#include "hxtypes.h"

#include "hxcom.h"
#include "hxengin.h"
#include "ihxpckts.h"
#include "hxbuffer.h"
#include "hxausvc.h"
#include "hxrasyn.h"
#include "hxprefs.h"
#include "hxerror.h"

#include "errdbg.h"
#include "chxpckts.h"
#include "hxaudply.h"
#include "hxaudstr.h"
#include "hxaudses.h"
#include "hxaudev.h"
#include "hxaudvol.h"   

#include "mixengine.h"

#include "hxslist.h"
#include "hxmap.h"
#include "auderrs.h"

#include "hxtick.h"
#include "hxtlogutil.h"
#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE             
static const char HX_THIS_FILE[] = __FILE__;
#endif

#define CACHE_INCREMENT_SIZE 2

//#define _TESTING    1
#ifdef _TESTING
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#if defined (_WINDOWS) || defined (_WIN32)
#include <io.h>
#endif

#endif //_TESTING

// XXXLCM We are currently truncating some 64-bit values in HXLOG statements. 
//        Consider following or similar.
#if defined(_SYMBIAN)
#define HXLOG_INT64_ARG(x) INT64_TO_UINT32(x)
#define HX_INT64_FORMAT lu
#else
#define HXLOG_INT64_ARG(x) (x)
#define HX_INT64_FORMAT I64d
#endif


/************************************************************************
 *  Method:
 *              IHXAudioStream::CHXAudioStream()
 *      Purpose:
 *              Constructor. 
 */
CHXAudioStream::CHXAudioStream(CHXAudioPlayer* owner, IUnknown* pContext)
    :m_bMayNeedToRollbackTimestamp(FALSE)
    ,m_lRefCount(0)
    ,m_wLastError(HXR_OK)
    ,m_bInited(FALSE)
    ,m_bSetupDone(FALSE)
    ,m_bAudioFormatKnown(FALSE)
    ,m_bIsResumed(FALSE)
    ,m_bStreamPause(FALSE)
    ,m_pResampler(NULL)
    ,m_pValues(0)
    ,m_bDisableWrite(FALSE)
    ,m_ulGranularity(0)
    ,m_ulInputBytesPerGran(0)
    ,m_ulOutputBytesPerGran(0)
    ,m_bFirstWrite(TRUE)
    ,m_ulMaxBlockSize(0)
    ,m_uVolume(HX_MAX_VOLUME)
    ,m_nSoundLevelOffset(0)
    ,m_bMute(FALSE)
    ,m_bGotHooks(FALSE)
    ,m_bIsOpaqueStream(FALSE)
    ,m_llLastWriteTime(0)
    ,m_ulLastAdjustedTimeDiff(0)
    ,m_ulFudge(5)
    ,m_bHooksInitialized(FALSE)
    ,m_pInDataPtr(0)
    ,m_pOutDataPtr(0)
    ,m_pDataList(0)
    ,m_pInstantaneousList(0)
    ,m_pRAByToTsInList(0)
    ,m_pRAByToTsAdjustedList(0)
    ,m_pCommonClassFactory(NULL)
    ,m_pPreferences(NULL)
    ,m_pAvailableBuffers(NULL)
    ,m_uCacheSize(CACHE_INCREMENT_SIZE)
    ,m_bDeterminedInitialCacheSize(FALSE)
    ,m_bCacheMayBeGrown(FALSE)
    ,m_bTobeTimed(TRUE)
    ,m_bIsFirstPacket(TRUE)
    ,m_bIsLive(FALSE)
    ,m_bSetupToBeDone(FALSE)
    ,m_ulBaseTime(0)
    ,m_bCrossFadingToBeDone(FALSE)
    ,m_pCrossFadeStream(NULL)
    ,m_llCrossFadeStartTime(0)
    ,m_ulCrossFadeDuration(0)
    ,m_bFadeToThisStream(FALSE)
    ,m_bFadeAlreadyDone(FALSE)
    ,m_bRealAudioStream(FALSE)
    ,m_ulLastInputStartTime(0)
    ,m_ulLastInputEndTime(0)
    ,m_llLastStartTimePlayed(0)
    ,m_ulTSRollOver(0)
    ,m_ulLiveDelay(0)
    ,m_bAudioDeviceReflushHint(FALSE)
    ,m_bLastWriteTimeUpdated(FALSE)
    ,m_bLastNMilliSecsToBeSaved(FALSE)
    ,m_ulLastNMilliSeconds(MINIMUM_AUDIO_PUSHDOWN)
    ,m_pLastNMilliSecsList(NULL)
    ,m_ulLastNHeadTime(0)
    ,m_ulLastNTailTime(0)
    ,m_ulStartTime(0)
    ,m_ulSeekTime(0)
    ,m_ulLiveJoiningTime(0)
    ,m_bLiveJoiningTimeSet(FALSE)
    ,m_eState(E_STOPPED)
    ,m_bIsRewound(TRUE)
    ,m_bBeyondStartTime(FALSE)
    ,m_bHasStartTime(FALSE)
    ,m_piPendingAudioData(NULL)
    ,m_pSilenceBuffer(NULL)
{
    m_Owner = owner;
    if (m_Owner)
    {
        m_Owner->AddRef();
    }

    if (pContext)
    {
        HX_VERIFY(HXR_OK == pContext->QueryInterface(IID_IHXCommonClassFactory, 
                                                     (void**) &m_pCommonClassFactory));
    }

#ifdef HELIX_FEATURE_VOLUME
    m_pStreamVolume = NULL;
#endif
    
#if defined(HELIX_FEATURE_PREFERENCES)
    if (pContext)
    {
        HX_VERIFY(HXR_OK == pContext->QueryInterface(IID_IHXPreferences, (void**) &m_pPreferences));
    }
#endif /* HELIX_FEATURE_PREFERENCES */

    m_DryNotificationMap = new CHXMapPtrToPtr;
    m_pInDataPtr    = new HXAudioData;
    m_pOutDataPtr   = new HXAudioData;
    m_pMixEngine    = new HXAudioSvcMixEngine() ;
};

/************************************************************************
 *  Method:
 *              IHXAudioStream::~CHXAudioStream()
 *      Purpose:
 *              Destructor. Clean up and set free.
 */
CHXAudioStream::~CHXAudioStream()
{
    HX_DELETE(m_DryNotificationMap);
    ResetStream();
    HX_RELEASE(m_piPendingAudioData);
    HX_RELEASE(m_pSilenceBuffer);
}
 
/////////////////////////////////////////////////////////////////////////
//  Method:
//      IUnknown::QueryInterface
//  Purpose:
//      Implement this to export the interfaces supported by your
//      object.
//
STDMETHODIMP CHXAudioStream::QueryInterface(REFIID riid, void** ppvObj)
{
    QInterfaceList qiList[] =
        {
            { GET_IIDHANDLE(IID_IHXAudioStream), (IHXAudioStream*)this },
            { GET_IIDHANDLE(IID_IHXRealAudioSync), (IHXRealAudioSync*)this },
            { GET_IIDHANDLE(IID_IHXAudioStream2), (IHXAudioStream2*)this },
	    { GET_IIDHANDLE(IID_IHXAudioStream3), (IHXAudioStream3*)this },
            { GET_IIDHANDLE(IID_IHXCommonClassFactory), (IHXCommonClassFactory*)this },
            { GET_IIDHANDLE(IID_IHXUpdateProperties), (IHXUpdateProperties*)this },
            { GET_IIDHANDLE(IID_IUnknown), (IUnknown*)(IHXAudioStream*)this },
#ifdef HELIX_FEATURE_VOLUME            
            { GET_IIDHANDLE(IID_IHXVolumeAdviseSink), (IHXVolumeAdviseSink*)this },
#endif            
        };
    
    return ::QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//      IUnknown::AddRef
//  Purpose:
//      Everyone usually implements this the same... feel free to use
//      this implementation.
//
STDMETHODIMP_(ULONG32) CHXAudioStream::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//      IUnknown::Release
//  Purpose:
//      Everyone usually implements this the same... feel free to use
//      this implementation.
//
STDMETHODIMP_(ULONG32) CHXAudioStream::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

/*
 *  IHXAudioStream methods
 */

/************************************************************************
 *  Method:
 *              IHXAudioStream::Init
 *      Purpose:
 *              Init the audio stream.
 */
STDMETHODIMP CHXAudioStream::Init
(
    const HXAudioFormat* pAudioFormat,
    IHXValues*           pValues
    )
{
    HXLOGL3(HXLOG_ADEV, "CHXAudioStream[%p]::Init()", this);

    if (m_bAudioFormatKnown)
    {
        return HXR_OK;
    }

    HX_RESULT theErr = HXR_OK;
    m_pValues = pValues;
    if (m_pValues)
    {
        m_pValues->AddRef();
        UINT32 ulVal = 0;
        m_pValues->GetPropertyULONG32("audioDeviceReflushHint", ulVal);
        if (ulVal == 1)
        {
            SetAudioDeviceReflushHint(TRUE);
            m_Owner->m_Owner->CheckIfLastNMilliSecsToBeStored();
        }

        ulVal = 0;
        m_pValues->GetPropertyULONG32("IsOpaqueAudioStream", ulVal);
        if (ulVal)
        {
            m_bIsOpaqueStream = TRUE;
            IHXBuffer* pOpaqueType = NULL;
            IHXBuffer* pOpaqueData = NULL;
            if( FAILED(m_pValues->GetPropertyCString("OpaqueAudioStreamType", pOpaqueType)) ||
                FAILED(m_pValues->GetPropertyBuffer("OpaqueAudioStreamData", pOpaqueData)) ||
                !m_Owner->m_Owner->SetOpaqueMode( 
                    (const char *)pOpaqueType->GetBuffer(), pOpaqueData))
            {
                HX_RELEASE(pOpaqueType);
                HX_RELEASE(pOpaqueData);
                return HXR_FAIL;
            }
            HX_RELEASE(pOpaqueType);
            HX_RELEASE(pOpaqueData);
        }
    }

    memcpy( &m_AudioFmt, pAudioFormat, sizeof(HXAudioFormat) );

    // Create the audio data list 
    m_pDataList = new CHXSimpleList;
    if ( !m_pDataList )
    {
        theErr = HXR_OUTOFMEMORY;
    }
    
    if(!theErr) // check if list constructor really succeeded
    {
        if(!m_pDataList->IsPtrListValid())
        {
            theErr = HXR_OUTOFMEMORY;
        }
    }

    m_pInstantaneousList = new CHXSimpleList;
    if ( !m_pInstantaneousList || !m_pInstantaneousList->IsPtrListValid())
    {
        theErr = HXR_OUTOFMEMORY;
    }

    // Reset this so that we init the hooks
    m_bFirstWrite       = TRUE;
    m_bHooksInitialized = FALSE;

#ifdef HELIX_FEATURE_VOLUME    
    if( !theErr )
    {
        m_pStreamVolume = (IHXVolume*)new CHXVolume;
        if( m_pStreamVolume )
        {
            m_pStreamVolume->AddRef();
            m_pStreamVolume->AddAdviseSink(this);
        }
        else
            theErr = HXR_OUTOFMEMORY;
    }
#endif    

    m_bAudioFormatKnown = TRUE;

    if (m_bSetupToBeDone)
    {
        m_bSetupToBeDone    = FALSE;
        m_Owner->AudioFormatNowKnown();
    }

    if (!theErr && m_bSetupDone && !m_bInited)
    {
        theErr = ProcessInfo();
    }

    return theErr;
}

/************************************************************************
 *  Method:
 *              IHXAudioStream::Write
 *  Purpose:
 *      Write audio data to Audio Services. 
 *
 *      NOTE: If the renderer loses packets and there is no loss
 *      correction, then the renderer should write the next packet 
 *      using a meaningful start time.  Audio Services will play 
 *      silence where packets are missing.
 */
STDMETHODIMP CHXAudioStream::Write( HXAudioData* pInData )
{
    HX_RESULT theErr = HXR_OK;

    if (!pInData)
    {
        return HXR_INVALID_PARAMETER;
    }

    if (!m_bInited)
    {
        return HXR_NOT_INITIALIZED;
    }

    // Init pre-mix hooks. Call this once to set up hook info. 
    if ( !m_bHooksInitialized )
    {
        InitHooks();
    }

#if defined(HELIX_FEATURE_AUDIO_INCOMPLETESAMPLE)
    /* make sure that we are always handing complete sample frames
     * down the chain by buffering up samples if we don't get complete frames.
     *
     * This is done by using a slush IHXBuffer that is just large enough
     * to hold one sample frame, and a second IHXBuffer to hold sample fragments.
     * This one does not own a buffer, but uses the incoming buffers.
     *
     * This way, we don't need any large memcpy()s. Unfortunately, we have to
     * create a new slush buffer every time because we don't know how long
     * this buffer will be stuck in the queue until it gets rendered.
     *
     * I guess we could try to be smart and only create a new one if the old one
     * is still AddRef()ed but I figure it's not worth it.
     */

    /* if there was a discontinuity in the audio stream, throw away pending bytes.
     * This also assumes that we're starting at a sample frame boundary, which might
     * be wrong -- but we really have no way to tell.
     */

    /* if pData is NULL, hand it through unchanged. See comment in Write2() */
    if (!pInData->pData)
    {
        return Write2(pInData) ;
    }

    if(pInData->uAudioStreamType != STREAMING_AUDIO)
    {
        m_ulPendingAudioBytes = 0 ;
        HX_RELEASE(m_piPendingAudioData) ;
    }

    // number of bytes in incoming sample
    UINT32 ulInBytes = pInData->pData->GetSize() ;
    
    // number of bytes that will be cut off incoming sample
    UINT32 ulCutoffBytes = 0 ; 

    /* first check if we have pending samples. */
    if (m_ulPendingAudioBytes)
    {
        // complete sample frames would have been sent the last time around
        HX_ASSERT(m_ulPendingAudioBytes < m_ulSampleFrameSize) ;
        HX_ASSERT(m_piPendingAudioData) ;

        /* append more bytes from the start of the incoming packet. */
        ulCutoffBytes = m_ulSampleFrameSize - m_ulPendingAudioBytes ;
        if (ulCutoffBytes > ulInBytes)
        {
            ulCutoffBytes = ulInBytes ;
        }

        memcpy(m_piPendingAudioData->GetBuffer() + m_ulPendingAudioBytes,
               pInData->pData->GetBuffer(),
               ulCutoffBytes ) ;

        m_ulPendingAudioBytes += ulCutoffBytes ;
        ulInBytes             -= ulCutoffBytes ;
    }

    // if we have a complete sample frame in the slush buffer, send it.
    if (m_ulPendingAudioBytes == m_ulSampleFrameSize)
    {
        HX_ASSERT(m_piPendingAudioData) ;

        HXAudioData audioData ;

        audioData.pData = m_piPendingAudioData ;
        audioData.pData->AddRef() ;

        // use time of incoming packet -- one sample frame is below the
        // ms resolution of time stamps.
        audioData.ulAudioTime = pInData->ulAudioTime;
        // stream type is incoming stream type
        audioData.uAudioStreamType = pInData->uAudioStreamType;
        theErr = Write2(&audioData) ;
        audioData.pData->Release() ;

        m_ulPendingAudioBytes = 0 ;

        // release the slush buffer
        HX_RELEASE(m_piPendingAudioData) ;

        if (FAILED(theErr))
        {
            return theErr ;
        }
    }

    // put partial sample frames from the end of the incoming buffer
    // into the slush buffer.

    if (ulInBytes % m_ulSampleFrameSize)
    {
        // the slush buffer should be empty here.
        HX_ASSERT(m_ulPendingAudioBytes == 0);
        HX_ASSERT(m_piPendingAudioData == 0) ;

        // reserve a new slush buffer
        theErr = CreateInstance(IID_IHXBuffer, (void**)&m_piPendingAudioData);

        if (SUCCEEDED(theErr))
        {
            theErr = m_piPendingAudioData->SetSize(m_ulSampleFrameSize) ;
        }

        if (SUCCEEDED(theErr))
        {
            m_ulPendingAudioBytes = ulInBytes % m_ulSampleFrameSize ;
            ulInBytes -= m_ulPendingAudioBytes ;

            memcpy(m_piPendingAudioData->GetBuffer(),
                   pInData->pData->GetBuffer() + ulCutoffBytes + ulInBytes,
                   m_ulPendingAudioBytes) ;
        }

        if (FAILED(theErr))
        {
            return theErr ;
        }
    }

    // send any leftover fragment of the incoming buffer.

    if (ulInBytes == pInData->pData->GetSize() && !ulCutoffBytes)
        /* this is the entire buffer, not a fragment. 
         * This is the normal case -- let's handle it efficiently. */
    {
        theErr = Write2(pInData) ;
    }
    else if (ulInBytes)
        /* if anything left in buffer, send it in a fragment */
    {
        HXAudioData audioData ;
        CHXBufferFragment* pFragment = new CHXBufferFragment(
            pInData->pData,
            pInData->pData->GetBuffer() + ulCutoffBytes,
            ulInBytes);

        theErr = pFragment->QueryInterface(IID_IUnknown, (void**)&audioData.pData) ;

        // this must always succeed, since we know it exports a IHXBuffer
        HX_ASSERT(SUCCEEDED(theErr)) ;

        // use time of incoming packet -- one sample frame is below the
        // ms resolution of time stamps.
        audioData.ulAudioTime = pInData->ulAudioTime;
        // stream type is incoming stream type if we did not cut anything away,
        // and STREAMED_AUDIO if we did (because in that case this is a continuation)
        audioData.uAudioStreamType = ulCutoffBytes ? STREAMING_AUDIO : pInData->uAudioStreamType ;
        theErr = Write2(&audioData) ;
        // we release our hold on pFragment here. When MixIntoBuffer() is done with
        // this fragment, it will also release its hold, and the fragment gets
        // deleted.
        audioData.pData->Release() ;
    }
#else
    theErr = Write2(pInData) ;
#endif /* HELIX_FEATURE_AUDIO_INCOMPLETESAMPLE */

    return theErr;
}

/************************************************************************
 *  Method:
 *              IHXAudioStream::Write2
 *  Purpose:
 *      Write audio data to Audio Services. This is a companion/backend
 *      function to IHXAudioStream::Write
 *
 */
HX_RESULT CHXAudioStream::Write2(HXAudioData* pInData)
{
    HX_RESULT theErr = HXR_OK;

    // Process any "hooks"; Add the data to the data list.  If buffer is NULL,
    // it means that the user just wants to know what timestamp should be placed
    // in the next STREAMED/TIMED audio data
    if ( !m_bGotHooks || !pInData->pData || m_bIsOpaqueStream)
    {
        theErr = AddData( pInData );
    }
    else
    {
        HXAudioData outData;
        
        outData.pData       = 0;
        outData.ulAudioTime = 0;

        theErr = ProcessHooks( pInData, &outData );
        if (!theErr && !m_bDisableWrite )
        {
            theErr = AddData( &outData );
        }

        if (outData.pData)
        {
            outData.pData->Release();
        }
    }
    
    return theErr;
}

/************************************************************************
 *  Method:
 *              IHXAudioStream::AddPreMixHook
 *      Purpose:
 *      Use this method to add a pre-mix audio data hook.
 */
STDMETHODIMP CHXAudioStream::AddPreMixHook
( 
    IHXAudioHook* pHook,
    const HXBOOL  bDisableWrite
    )
{
    HXLOGL3(HXLOG_ADEV, "CHXAudioStream[%p]::AddPreMixHook(): add [%p]", this, pHook);
#if defined(HELIX_FEATURE_AUDIO_PREMIXHOOK)
    void* pTmp = 0;
    
    /* Does one already exists */
    if (m_PreMixHookMap.Lookup((void*)pHook, pTmp))
    {
        return HXR_INVALID_PARAMETER;
    }

    HXAudioHookInfo* pPreMixHookInfo      = (HXAudioHookInfo*) new HXAudioHookInfo;
    pPreMixHookInfo->pHook                = pHook;
    pPreMixHookInfo->bDisableWrite        = bDisableWrite;
    pPreMixHookInfo->bFinal               = FALSE;
    pPreMixHookInfo->bIgnoreAudioData     = FALSE;
    pPreMixHookInfo->bMultiChannelSupport = FALSE;

    IHXValues* pValues = NULL;
    if (pHook && pHook->QueryInterface(IID_IHXValues, (void**) &pValues) == HXR_OK)
    {
        UINT32 ulValue = 0;
        pValues->GetPropertyULONG32("IgnoreAudioData", ulValue);
        pPreMixHookInfo->bIgnoreAudioData = (ulValue == 1);
        HX_RELEASE(pValues);
    }

    pHook->AddRef(); // Released in destructor

    IHXAudioMultiChannel* pMultiChannel = NULL;
    if (pHook && HXR_OK == pHook->QueryInterface(IID_IHXAudioMultiChannel, (void**) &pMultiChannel))
    {
        pPreMixHookInfo->bMultiChannelSupport = pMultiChannel->GetMultiChannelSupport();
    }
    HX_RELEASE(pMultiChannel);

    m_PreMixHookMap.SetAt(pHook, pPreMixHookInfo);

    m_bGotHooks = TRUE;

    /* If any one of them is Disabled, we do not write */
    if (bDisableWrite)
    {
        m_bDisableWrite = TRUE;
    }

    ProcessAudioHook(ACTION_ADD, pHook);

    /* If we are already initialized, send the audio format */
    if (m_bHooksInitialized && !m_bIsOpaqueStream)
    {
        if (pPreMixHookInfo->bIgnoreAudioData ||
            HXR_OK == ProcessAudioHook(ACTION_CHECK, pHook))
        {
            pHook->OnInit( &m_AudioFmt );
        }
    }

    return HXR_OK;
#else
    return HXR_NOTIMPL;
#endif /* HELIX_FEATURE_AUDIO_PREMIXHOOK */
}

/************************************************************************
 *  Method:
 *              IHXAudioStream::RemovePreMixHook
 *      Purpose:
 *      Use this method to remove a pre-mix audio data hook.
 */
STDMETHODIMP CHXAudioStream::RemovePreMixHook
( 
    IHXAudioHook*    pHook
    )
{
    HXLOGL3(HXLOG_ADEV, "CHXAudioStream[%p]::RemovePreMixHook(): remove [%p]", this, pHook);
#if defined(HELIX_FEATURE_AUDIO_PREMIXHOOK)
    HXAudioHookInfo* pPreMixHookInfo = 0;
    HXBOOL bCheckForDisableWrite              = FALSE;

    if (!m_PreMixHookMap.Lookup((void*)pHook, (void*&) pPreMixHookInfo))
    {
        return HXR_INVALID_PARAMETER;
    }

    m_PreMixHookMap.RemoveKey(pHook);

    /* If we are removing a hook which had disable write, 
     * we need to re-determine if any of the remaining hooks
     * has DisableWrite set to TRUE
     */
    if (pPreMixHookInfo->bDisableWrite)
    {
        bCheckForDisableWrite = TRUE;
        m_bDisableWrite       = FALSE;
    }

    ProcessAudioHook(ACTION_REMOVE, pHook);

    pPreMixHookInfo->pHook->Release();
    delete pPreMixHookInfo;

    if (m_PreMixHookMap.GetCount() == 0)
    {
        m_bGotHooks     = FALSE;
        m_bDisableWrite = FALSE;
    }
    else if (bCheckForDisableWrite)
    {
        CHXMapPtrToPtr::Iterator lIter = m_PreMixHookMap.Begin();
        for (; lIter != m_PreMixHookMap.End(); ++lIter)
        {
            HXAudioHookInfo* pPreMixHook = (HXAudioHookInfo*) (*lIter);
            
            /* atleast one has Disable Write ON */
            if (pPreMixHook->bDisableWrite)
            {
                m_bDisableWrite = TRUE;
                break;
            }
        }
    }
#endif /* HELIX_FEATURE_AUDIO_PREMIXHOOK */

    return HXR_OK;
}

/************************************************************************
 *  Method:
 *       IHXAudioStream::AddDryNotification
 *  Purpose:
 *       Use this to add a notification response object to get notifications
 *       when audio stream is running dry.
 */
STDMETHODIMP CHXAudioStream::AddDryNotification
(
    IHXDryNotification* /*IN*/ pNotification
    )
{
    HXLOGL3(HXLOG_ADEV, "CHXAudioStream[%p]::AddDryNotification(): add [%p]", this, pNotification);
    if (!pNotification)
    {
        return HXR_INVALID_PARAMETER;
    }

    void* pTmp = 0;
    
    /* Does one already exists */
    if (m_DryNotificationMap->Lookup((void*)pNotification, pTmp))
    {
        return HXR_INVALID_PARAMETER;
    }

    pNotification->AddRef();

    m_DryNotificationMap->SetAt((void*)pNotification, (void*)pNotification);

    return HXR_OK;
}


/************************************************************************
 *  Method:
 *      IHXAudioStream2::RemoveDryNotification
 *  Purpose:
 *           Use this to remove itself from the notification response object
 *           during the stream switching.
 */
STDMETHODIMP CHXAudioStream::RemoveDryNotification   
(
    IHXDryNotification* /*IN*/ pNotification
    )
{

    HXLOGL3(HXLOG_ADEV, "CHXAudioStream[%p]::RemoveDryNotification(): add [%p]", this, pNotification);
    HX_RESULT   hr = HXR_OK;

    void* pTmp = 0;

    if (!pNotification)
    {
        hr = HXR_INVALID_PARAMETER;
        goto cleanup;
    }

    // remove only if it is exists
    if (m_DryNotificationMap->Lookup((void*)pNotification, pTmp))
    {
        m_DryNotificationMap->RemoveKey((void*)pNotification);
        HX_RELEASE(pNotification);
    }
    else
    {
        hr = HXR_INVALID_PARAMETER;
        goto cleanup;
    }

  cleanup:

    return hr;
}

/************************************************************************
 *  Method:
 *      IHXAudioStream2::GetAudioFormat
 *  Purpose:
 *           Returns the input audio format of the data written by the 
 *           renderer. This function will fill in the pre-allocated 
 *           HXAudioFormat structure passed in.
 */
STDMETHODIMP
CHXAudioStream::GetAudioFormat(HXAudioFormat*   /*IN/OUT*/pAudioFormat)
{
    HX_ASSERT(pAudioFormat);
    if (!pAudioFormat)
    {
        return HXR_INVALID_PARAMETER;
    }

    if (!m_bInited)
    {
        return HXR_UNEXPECTED;
    }

    pAudioFormat->uChannels       = m_AudioFmt.uChannels;
    pAudioFormat->uBitsPerSample  = m_AudioFmt.uBitsPerSample;
    pAudioFormat->ulSamplesPerSec = m_AudioFmt.ulSamplesPerSec;
    pAudioFormat->uMaxBlockSize   = m_AudioFmt.uMaxBlockSize;

    return HXR_OK;
}


 
/************************************************************************
 *  Method:
 *              IHXAudioStream::GetAudioVolume
 *      Purpose:
 *              Return this stream's IRMA volume interface.
 */
STDMETHODIMP_(IHXVolume*) CHXAudioStream::GetAudioVolume()
{
    IHXVolume* pRet = NULL;
    
#ifdef HELIX_FEATURE_VOLUME    
    if( m_pStreamVolume )
    {
        m_pStreamVolume->AddRef();
        pRet = m_pStreamVolume;
    }
#endif 
   
    return pRet;
}

void CHXAudioStream::SetSoundLevelOffset(INT16 nSoundLevelOffset)
{
    m_nSoundLevelOffset = nSoundLevelOffset;

#if defined(HELIX_FEATURE_GAINTOOL) && defined(HELIX_FEATURE_AUDIO_LEVEL_NORMALIZATION)
    if (m_pMixEngine && m_bInited)
    {
        // if the volume is "very low", make the gain change immediate
        // this is so that fade-ins that are automated by the TLC can immediately
        // start out at zero volume
        // XXX wschildbach this is an ugly hack.
        m_pMixEngine->SetVolume(m_pMixEngine->HXVolume2TenthOfDB(m_uVolume) + m_nSoundLevelOffset, m_uVolume <= 3);
    }
#endif
}

#if defined(HELIX_FEATURE_VOLUME)
//
// IHXVolume methods
//
STDMETHODIMP CHXAudioStream::OnVolumeChange(const UINT16 uVolume)
{
    m_uVolume = uVolume;
#ifdef HELIX_FEATURE_GAINTOOL
    if (m_pMixEngine && m_bInited)
    {
        // if the volume is "very low", make the gain change immediate
        // this is so that fade-ins that are automated by the TLC can immediately
        // start out at zero volume
        // XXX wschildbach this is an ugly hack.
        m_pMixEngine->SetVolume(m_pMixEngine->HXVolume2TenthOfDB(m_uVolume) + m_nSoundLevelOffset, m_uVolume <= 3) ;
    }
#endif
    return HXR_OK;
}

STDMETHODIMP CHXAudioStream::OnMuteChange(const HXBOOL bMute)
{
    m_bMute = bMute;
#ifdef HELIX_FEATURE_GAINTOOL
    if (m_pMixEngine)
    {
        m_pMixEngine->SetVolume(
            m_pMixEngine->HXVolume2TenthOfDB(bMute ? HX_MIN_VOLUME : m_uVolume) + bMute ?
            0 :
            m_nSoundLevelOffset, TRUE
            ); // mute is immediate.
    }
    

#endif
    return HXR_OK;
}

#endif /* HELIX_FEATURE_VOLUME */





/************************************************************************
 *  Method:
 *              IHXAudioStream::AddData
 *      Purpose:
 *              Add audio data to list.
 *      NOTE: Mark Streamed data also as Timed data IF we don't write a streamed packet
 *       since it was LATE!!!
 */
HX_RESULT CHXAudioStream::AddData( HXAudioData* pAudioData )
{
    HX_RESULT    theErr        = HXR_OK;
    HXBOOL       bInTSRollOver = FALSE;
    HXAudioInfo* pAinfo        = 0;

    /* If buffer is NULL, it means that the user just 
     * wants to know what timestamp should be placed in the next 
     * STREAMED/TIMED audio data
     */
    if (!pAudioData->pData)
    {
        HXAudioInfo* pInfo = NULL;

        if (!m_pDataList->IsEmpty() && NULL != (pInfo = (HXAudioInfo*) m_pDataList->GetTail()))
        {
            pAudioData->ulAudioTime = pInfo->ulStartTime + 
                CalcMs(pInfo->pBuffer->GetSize());
        }
        else
        {
	    UpdateStreamLastWriteTime();
            pAudioData->ulAudioTime = INT64_TO_UINT32(m_llLastWriteTime);
        }

        HXLOGL4(HXLOG_ADEV, "CHXAudioStream[%p]::AddData(): no data (timestamp query): time = %lu", this, pAudioData->ulAudioTime);

        return HXR_OK;
    }

    // make sure the renderer does not pass NULL data!!
    HX_ASSERT(pAudioData->pData->GetBuffer() != NULL && pAudioData->pData->GetSize() != 0);
    if (pAudioData->pData->GetBuffer()  == NULL || pAudioData->pData->GetSize() == 0)
    {
        return HXR_INVALID_PARAMETER;
    }

    if (m_bIsFirstPacket)
    {
        HXLOGL2(HXLOG_ADEV, "AudioFormatIn: %lu channels %lu SamplesPerSec",
                m_AudioFmt.uChannels,
                m_AudioFmt.ulSamplesPerSec);
        HXLOGL2(HXLOG_ADEV, "AudioFormatOut: %lu channels %lu SamplesPerSec",
                m_DeviceFmt.uChannels,
                m_DeviceFmt.ulSamplesPerSec);
    
        if (m_bIsLive)
        {
            // We update last write time here for live streams since we may
	    // not have the live stream sync point until the first packet arrives.
            m_Owner->UpdateStreamLastWriteTime();
            UpdateStreamLastWriteTime(TRUE);
        }

	m_bIsFirstPacket = FALSE;
    }

    UINT32 ulDataTime = CalcMs(pAudioData->pData->GetSize());
    UINT32 ulEndTime  = pAudioData->ulAudioTime + ulDataTime;

    if ((!m_bBeyondStartTime) && m_bHasStartTime &&
	(((INT32) (m_ulStartTime - pAudioData->ulAudioTime)) > ((INT32) m_ulFudge)))
    {
	/* Packet prior to the start time */
        m_bTobeTimed = TRUE;
        HXLOGL3(HXLOG_ADEV,
                "CHXAudioStream[%p]::AddData(): BEFORE START: count = %ld; audio time = %lu; start time = %ld",
                this,
                m_pDataList->GetCount(),
                pAudioData->ulAudioTime,
                m_ulStartTime);
        
        return HXR_LATE_PACKET; 
    }

    HXLOGL4(HXLOG_ADEV,
            "CHXAudioStream[%p]::AddData(): StartTime=%lu; Duration=%lu; EndTime=%lu BufSize=%lu Mode=%u [%s]",
            this,
            pAudioData->ulAudioTime,
            ulDataTime,
            ulEndTime,
	    pAudioData->pData->GetSize(),
	    pAudioData->uAudioStreamType,
	    ((pAudioData->uAudioStreamType == STREAMING_AUDIO) ? 
		"STREAMING" :
		((pAudioData->uAudioStreamType == TIMED_AUDIO) ? "TIMED" : "OTHER")));

    if (m_pAvailableBuffers && !m_bDeterminedInitialCacheSize && ulDataTime > 0)
    {
        m_bDeterminedInitialCacheSize = TRUE;
        m_uCacheSize = (UINT16) (m_ulGranularity*2/ulDataTime) + 1;
        /* make sure it is atleast CACHE_INCREMENT_SIZE to begin with */
        m_uCacheSize = m_uCacheSize < CACHE_INCREMENT_SIZE ? 
            CACHE_INCREMENT_SIZE : m_uCacheSize;

        HXLOGL3(HXLOG_ADEV, "CHXAudioStream[%p]::AddData(): cache size = %u", this, m_uCacheSize);
    }

    if (m_ulLastInputStartTime > pAudioData->ulAudioTime &&
        ((m_ulLastInputStartTime - pAudioData->ulAudioTime) > MAX_TIMESTAMP_GAP))
    {
        bInTSRollOver = TRUE;
        m_ulTSRollOver++;
        HXLOGL3(HXLOG_ADEV,
                "CHXAudioStream[%p]::AddData(): timestamp rollover (rollover count => %lu)",
                this,
                m_ulTSRollOver);
    }

    m_ulLastInputStartTime = pAudioData->ulAudioTime; 
    m_ulLastInputEndTime   = ulEndTime;

    /* even in STREAMING_AUDIO case, it might happen, that the packets
     * written are late. e.g. packets received late on the network 
     */

    // XXX wschildbach : Why do we even care? Just insert the packet into the queue
    // and let the reaper take care of it!

    INT64 llActualTimestamp = CAST_TO_INT64 (pAudioData->ulAudioTime) + CAST_TO_INT64 m_ulTSRollOver * CAST_TO_INT64 MAX_UINT32;
    INT64 llActualEndTime = CAST_TO_INT64 (pAudioData->ulAudioTime) + CAST_TO_INT64 (ulDataTime) +
        CAST_TO_INT64 (m_ulTSRollOver) * CAST_TO_INT64 (MAX_UINT32);

#if 0 // testing sampling frequency estimation for "inaccurate resampling"
    if (pAudioData->uAudioStreamType == TIMED_AUDIO)
    {
        m_startMeasureTime = llActualTimestamp ;
        m_totSamples = 0 ;
    }
    else if (pAudioData->uAudioStreamType == STREAMING_AUDIO)
    {
        float diffTime = (llActualTimestamp - m_startMeasureTime) / 1000.0f ;
        if (diffTime)
        {
            float frEstimate = m_totSamples / m_AudioFmt.uChannels / diffTime ;
            float frEstErr = frEstimate * 0.001f / diffTime ; // 1 ms inaccuracy
            HXLOGL4(HXLOG_ADEV, "CHXAudioStream[%p]::AddData(): %I64d\t%I64d\t%f\t%f", this, llActualTimestamp,llActualEndTime,frEstimate,frEstErr);
        }
    }
    m_totSamples += CalcSamples(pAudioData->pData->GetSize()) ;
#endif

    if ((pAudioData->uAudioStreamType == STREAMING_AUDIO ||
         pAudioData->uAudioStreamType == TIMED_AUDIO) &&
        !(llActualTimestamp >= m_llLastWriteTime ||
          llActualEndTime > m_llLastWriteTime)) 
    {
        /* Too late*/
        m_bTobeTimed = TRUE;
        HXLOGL3(HXLOG_ADEV,
                "CHXAudioStream[%p]::AddData(): LATE: count = %ld; audio time = %lu; last write time = %ld",
                this,
                m_pDataList->GetCount(),
                pAudioData->ulAudioTime,
                INT64_TO_INT32(m_llLastWriteTime));
        
        return HXR_LATE_PACKET; 
    }

    pAinfo = new HXAudioInfo;
    if( !pAinfo )
    {
        return HXR_OUTOFMEMORY;
    }

    pAudioData->pData->AddRef();
    pAinfo->pBuffer          = pAudioData->pData;
    pAinfo->ulStartTime      = pAudioData->ulAudioTime;
    pAinfo->pOffset          = pAudioData->pData->GetBuffer();  
    pAinfo->ulBytesLeft      = pAudioData->pData->GetSize();  
    pAinfo->uAudioStreamType = pAudioData->uAudioStreamType;

    if (m_bTobeTimed && pAudioData->uAudioStreamType == STREAMING_AUDIO)
    {
        pAinfo->uAudioStreamType = TIMED_AUDIO;
        m_bTobeTimed             = FALSE;
    }
    else if (m_bTobeTimed && pAudioData->uAudioStreamType == TIMED_AUDIO)
    {
        m_bTobeTimed = FALSE;
    }


//////////////////////////////////////////////////////////////////////
    // XXX wschildbach
    // the start time of this packet in samples. This may be corrected later on.
    pAinfo->llStartTimeInSamples = CAST_TO_INT64(llActualTimestamp) * m_AudioFmt.ulSamplesPerSec / 1000 * m_AudioFmt.uChannels ;
    pAinfo->llEndTimeInSamples   = pAinfo->llStartTimeInSamples + Bytes2Samples(pAinfo->pBuffer->GetSize(), &m_AudioFmt) ;

//////////////////////////////////////////////////////////////////////

    if (pAinfo->uAudioStreamType == INSTANTANEOUS_AUDIO)
    {
        CHXSimpleList* pList = new CHXSimpleList;
        if( !pList )
        {
            HX_RELEASE(pAudioData->pData);
            HX_DELETE(pAinfo);
            return HXR_OUTOFMEMORY;
        }
        pList->AddHead((void*) pAinfo);
        m_pInstantaneousList->AddTail((void*) pList);
        m_Owner->m_Owner->ToBeRewound();
    }
    else if (pAinfo->uAudioStreamType == STREAMING_INSTANTANEOUS_AUDIO)
    {
        HX_ASSERT(m_pInstantaneousList && m_pInstantaneousList->GetCount() > 0);
        CHXSimpleList* pList = NULL;
        if (m_pInstantaneousList->GetCount() == 0)
        {
            pList = new CHXSimpleList;
            if( !pList )
            {
                HX_RELEASE(pAudioData->pData);
                HX_DELETE(pAinfo);
                return HXR_OUTOFMEMORY;
            }
            m_pInstantaneousList->AddTail((void*) pList);

            // fix for naive users!
            pAinfo->uAudioStreamType = INSTANTANEOUS_AUDIO;
            m_Owner->m_Owner->ToBeRewound();
        }
        pList = (CHXSimpleList*) m_pInstantaneousList->GetTail();
        pList->AddTail(pAinfo);
    }
    else if (m_pDataList->IsEmpty())
    {
        //HXLOGL4(HXLOG_ADEV, "CHXAudioStream[%p]::AddData(): add to empty list", this);
        m_pDataList->AddTail((void*) pAinfo);
    }
    else
    {
        //HXLOGL4(HXLOG_ADEV, "CHXAudioStream[%p]::AddData(): add to non-empty list", this);
        HXAudioInfo* pInfo = (HXAudioInfo*) m_pDataList->GetTail();

        UINT32  ulActualTSRollOver = m_ulTSRollOver;

        if (bInTSRollOver && ulActualTSRollOver)
        {
            ulActualTSRollOver--;
        }

        INT64 llActualLastEndTime = CAST_TO_INT64 (pInfo->ulStartTime) + CAST_TO_INT64 (CalcMs(pInfo->pBuffer->GetSize())) +
            CAST_TO_INT64 ulActualTSRollOver * CAST_TO_INT64 MAX_UINT32;
        INT64 llActualLastStartTime = CAST_TO_INT64 (pInfo->ulStartTime) + CAST_TO_INT64 ulActualTSRollOver * CAST_TO_INT64 MAX_UINT32;

        if (llActualTimestamp < llActualLastStartTime)
        {
            /* Not allowed */
            theErr = HXR_OUTOFORDER_PACKET; 
            /* something is wrong... figure out what?*/

            HX_ASSERT(!("Packets written out of order"));
            goto exit;
        }

        if (pAinfo->uAudioStreamType == STREAMING_AUDIO)
        {
            /* is it a resonable packet to add to the list */
            if ((llActualTimestamp <= llActualLastEndTime              &&
                 llActualLastEndTime - llActualTimestamp <= m_ulFudge) || 
                (llActualTimestamp >= llActualLastEndTime              &&
                 llActualTimestamp - llActualLastEndTime <= m_ulFudge)
                )
            {
                // XXX wschildbach
                // make 64-bit timestamps contiguous before adding into the queue.
                pAinfo->llEndTimeInSamples += pInfo->llEndTimeInSamples - pAinfo->llStartTimeInSamples ;
                pAinfo->llStartTimeInSamples  = pInfo->llEndTimeInSamples ;
                m_pDataList->AddTail((void*) pAinfo);
            }
            else
            {
                theErr = HXR_NONCONTIGUOUS_PACKET; //HX_LATE_PACKET;
                /* something is wrong... figure out what?*/
                HX_ASSERT(!("Streaming Audio: Non-Contigous Write"));
                m_bTobeTimed    = TRUE;
                goto exit;
            }
        }
        else
        {
            /* see if there is any overlap.. we do not allow any overlap */
            if (llActualTimestamp < llActualLastEndTime &&
                llActualLastEndTime - llActualTimestamp > m_ulFudge)
            {
                /* hmmm an overlapped packet */
                theErr = HXR_OVERLAPPED_PACKET;
                /* something is wrong... figure out what?*/
                HX_ASSERT(!("Timed Audio: Overlapping write"));
                m_bTobeTimed    = TRUE;
                goto exit;
            }
            else
            {
                m_pDataList->AddTail((void*) pAinfo);
            }
        }
    }

  exit:
    if (theErr != HXR_OK && pAinfo)
    {
        pAinfo->pBuffer->Release();
        delete pAinfo;
    }

    return theErr;
}

HX_RESULT CHXAudioStream::ProcessInfo(void)
{
    UINT32 ulResumed = 0;   // default state after init is paused
    HX_RESULT theErr = HXR_OK;

    m_ulSampleFrameSize = m_AudioFmt.uChannels * (m_AudioFmt.uBitsPerSample>>3) ;

    m_ulPendingAudioBytes = 0 ;

    // Calculate the number of bytes per granularity.

    // XXX wschildbach: These formulas are suspect. There is no feedback to the player
    // or audio session that this is what we assume for a size. I believe this is either
    // not needed or too complex.

    m_ulInputBytesPerGran = (ULONG32) 
        (((m_AudioFmt.uChannels * (m_AudioFmt.uBitsPerSample>>3) * m_AudioFmt.ulSamplesPerSec) 
          / 1000.0) * m_ulGranularity);

    m_ulOutputBytesPerGran  = (ULONG32) 
        (((m_DeviceFmt.uChannels * (m_DeviceFmt.uBitsPerSample>>3) * m_DeviceFmt.ulSamplesPerSec) 
          / 1000.0) * m_ulGranularity);

    
    // Make sure that number of bytes per granularity is an even number.
    m_ulInputBytesPerGran  -= m_ulInputBytesPerGran  % ((m_AudioFmt.uBitsPerSample>>3)*m_AudioFmt.uChannels);
    m_ulOutputBytesPerGran -= m_ulOutputBytesPerGran % ((m_DeviceFmt.uBitsPerSample>>3)*m_DeviceFmt.uChannels);

    if (!theErr)
    {
        // set up the mixing engine
        // XXX wschildbach
        theErr = m_pMixEngine->Init( m_AudioFmt.ulSamplesPerSec,
                                     m_DeviceFmt.ulSamplesPerSec,
                                     m_AudioFmt.uChannels,
                                     m_DeviceFmt.uChannels
                                     );
        if (SUCCEEDED(theErr))
        {
            theErr = m_pMixEngine->SetSampleConverter(this) ;
        }
        
        if (SUCCEEDED(theErr))
        {
            theErr = m_pMixEngine->SetOutputBytesPerSample(m_DeviceFmt.uBitsPerSample / 8) ;
        }
        
        // set the volume (somebody might have set it when we did not have an engine)
#ifdef HELIX_FEATURE_GAINTOOL
        if (SUCCEEDED(theErr))
        {
            m_pMixEngine->SetVolume(
                m_pMixEngine->HXVolume2TenthOfDB(m_bMute ? HX_MIN_VOLUME : m_uVolume) + m_bMute ?
                0 :
                m_nSoundLevelOffset,
                TRUE);
        }
        
#endif
    }

    if (!theErr)
    {
        m_bInited = TRUE;

        if (m_eState == E_STOPPED)
        {
            m_eState = E_INITIALIZED;
        } 
    }

    HX_ASSERT(m_bIsRewound);
    
    HXLOGL3(HXLOG_ADEV, "CHXAudioStream[%p]::ProcessInfo(): InputBytesPerGran=%lu OutputBytesPerGran=%lu", 
            this, 
            m_ulInputBytesPerGran, 
            m_ulOutputBytesPerGran
            );

    if (!theErr && m_bInited)
    {
        m_Owner->StreamInitialized(this);
    }

    // Caller can explicitly request the stream into resumed (E_PLAYING) state
    // or paused (E_PAUSED) state immediately on init. by indicating "Resumed" 
    // property in the heaader at non-zero and zero value respectively.
    // If property is not set, the stream will choose the initial state
    // to best accomodate the starting time for the stream.
    if (m_pValues->GetPropertyULONG32("Resumed", ulResumed) != HXR_OK)
    {
	// See if there is a basis for start-time setting.
	// If there is a start-time, we can judge well if we should resume
	// or pause the stream as we have something to compare agiainst
	// the current playback time.
	UINT32 ulDelay = 0;
	if (m_pValues->GetPropertyULONG32("Delay", ulDelay) == HXR_OK)
	{
	    UpdateStreamLastWriteTime(FALSE, TRUE, TRUE);
	    if (m_bHasStartTime && (!m_bIsRewound))
	    {
		// We determined this stream has start-time and it can be
		// created in resumed state since it need not be in rewound
		// state and will thus not cause rewinf of the session to
		// occur.
		ulResumed = 1;
	    }

	    // Reset the state affected by UpdateStreamLastWriteTime() call
	    m_bIsRewound = TRUE;
	    m_bHasStartTime = FALSE;
	}
    }

    // Place the stream into the proper state
    if (ulResumed)
    {
	IHXAudioStream* pThisAudioStreamI = (IHXAudioStream*) this;
	m_Owner->ManageAudioStreams(1, &pThisAudioStreamI, AUD_PLYR_STR_RESUME, NULL);
    }
    else
    {
	IHXAudioStream* pThisAudioStreamI = (IHXAudioStream*) this;
	m_Owner->ManageAudioStreams(1, &pThisAudioStreamI, AUD_PLYR_STR_PAUSE, NULL);
    }

    return theErr;
}


/************************************************************************
 *  Method:
 *              IHXAudioStream::GetFormat
 *      Purpose:
 *      Return the stream's audio format.
 */
HX_RESULT CHXAudioStream::GetFormat
( 
    HXAudioFormat* pAudioFormat
    )
{
    if (!m_bAudioFormatKnown)
    {
        return HXR_NOT_INITIALIZED;
    }

    pAudioFormat->uChannels       = m_AudioFmt.uChannels;
    pAudioFormat->uBitsPerSample  = m_AudioFmt.uBitsPerSample;
    pAudioFormat->ulSamplesPerSec = m_AudioFmt.ulSamplesPerSec;
    pAudioFormat->uMaxBlockSize   = m_AudioFmt.uMaxBlockSize;

    return HXR_OK;
}

/************************************************************************
 *  Method:
 *              IHXAudioStream::Setup
 *      Purpose:
 *              This is called by the player's Setup method. At this
 *              time the audio device format is set and we can now
 *              set up the streams pre-mixing buffer. This buffer
 *              contains data that has been resampled to match the
 *              audio device format.
 */
HX_RESULT CHXAudioStream::Setup( HXAudioFormat*  pFormat,
                                 ULONG32         ulGranularity
                                 )
{
    HXLOGL3(HXLOG_ADEV, "CHXAudioStream[%p]::Setup()", this);
    HX_RESULT theErr = HXR_OK;

    memcpy( &m_DeviceFmt, pFormat, sizeof(HXAudioFormat) );
    m_ulGranularity = ulGranularity;

    m_bSetupDone = TRUE;
    
    /* we have all the info now.. so setup the resampler */
    if (m_bAudioFormatKnown && !m_bInited)
    {
        theErr = ProcessInfo();
    }

    return theErr;
}

/************************************************************************
 *  Method:
 *              IHXAudioStream::ResetStream
 *      Purpose:
 */
void CHXAudioStream::ResetStream()
{
    HXLOGL3(HXLOG_ADEV, "CHXAudioStream[%p]::ResetStream()", this);
    m_bInited           = FALSE;
    m_bSetupDone        = FALSE;
    m_bAudioFormatKnown = FALSE;
    m_bIsResumed        = FALSE;
    m_bIsRewound	= TRUE;
    m_bStreamPause	= FALSE;
    m_bBeyondStartTime	= FALSE;
    m_bHasStartTime	= FALSE;
    m_bIsFirstPacket	= TRUE;
    
    UnRegister();

    while (m_pAvailableBuffers && m_pAvailableBuffers->GetCount() > 0)
    {
        IHXBuffer* pBuffer = (IHXBuffer*) m_pAvailableBuffers->RemoveHead();
        HX_RELEASE(pBuffer);
    }

    HX_DELETE(m_pAvailableBuffers);

    // Delete all entries in the audio data list
    FlushBuffers();
    HX_DELETE(m_pDataList);
    HX_DELETE(m_pInstantaneousList);

    CleanupRAByToTs();
    HX_DELETE(m_pRAByToTsInList);
    HX_DELETE(m_pRAByToTsAdjustedList);

    HX_DELETE(m_pMixEngine);

    m_bGotHooks = FALSE;

    m_ulStartTime = 0;
    m_ulSeekTime = 0;
    m_ulLiveJoiningTime = 0;
    m_bLiveJoiningTimeSet = FALSE;
    m_llLastWriteTime = 0;
    m_ulLastAdjustedTimeDiff = 0;
    m_ulTSRollOver = 0;

    HX_RELEASE(m_pValues);

#if defined(HELIX_FEATURE_AUDIO_PREMIXHOOK)
    // Delete all entries in the pre-mix hook list.
    if ( m_PreMixHookMap.GetCount() > 0)
    {
        HXAudioHookInfo* h = 0;
        CHXMapPtrToPtr::Iterator lIter = m_PreMixHookMap.Begin();
        for (; lIter != m_PreMixHookMap.End(); ++lIter)
        {
            h = (HXAudioHookInfo*) (*lIter);
            ProcessAudioHook(ACTION_REMOVE, h->pHook);
            h->pHook->Release();
            delete h;
        }
        
        m_PreMixHookMap.RemoveAll();
    }
#endif /* HELIX_FEATURE_AUDIO_PREMIXHOOK */

#ifdef HELIX_FEATURE_VOLUME
    if( m_pStreamVolume )
    {
        m_pStreamVolume->RemoveAdviseSink(this);
        m_pStreamVolume->Release();
        m_pStreamVolume=NULL;
    }
#endif    

    
    HX_DELETE(m_pInDataPtr);
    HX_DELETE(m_pOutDataPtr);

    if (m_DryNotificationMap && m_DryNotificationMap->GetCount() > 0)
    {
        IHXDryNotification* pDryNotification = 0;
        CHXMapPtrToPtr::Iterator lIter = m_DryNotificationMap->Begin();
        for (; lIter != m_DryNotificationMap->End(); ++lIter)
        {
            pDryNotification = (IHXDryNotification*) (*lIter);
            pDryNotification->Release();
        }
        
        m_DryNotificationMap->RemoveAll();
    }

    HX_RELEASE(m_pCrossFadeStream);
    HX_RELEASE(m_pCommonClassFactory);
#if defined(HELIX_FEATURE_PREFERENCES)
    HX_RELEASE(m_pPreferences);
#endif /* HELIX_FEATURE_PREFERENCES */
    HX_RELEASE(m_Owner);

    return;
}

HX_RESULT
CHXAudioStream::ProcessAudioHook( PROCESS_ACTION action, 
                                  IHXAudioHook* pAudioHook)
{
    return HXR_OK;
}

/************************************************************************
 *  Method:
 *              IHXAudioStream::InitHooks
 *      Purpose:
 *      Init any pre-mix hooks. Return TRUE if hooks exist else return
 *      FALSE.
 */
void CHXAudioStream::InitHooks()
{
    HXLOGL3(HXLOG_ADEV, "CHXAudioStream[%p]::InitHooks()", this);
#if defined(HELIX_FEATURE_AUDIO_PREMIXHOOK)
    /* Iterate thru the hook list and call the hook's OnInit().
     * If any of the hooks have disabled write set to TRUE, then
     * we will let this override any set to FALSE.
     */
    if(!m_bIsOpaqueStream)
    {
        if ( m_PreMixHookMap.GetCount() > 0 )
        {
            HXAudioHookInfo* h = 0;
            CHXMapPtrToPtr::Iterator lIter = m_PreMixHookMap.Begin();
            for (; lIter != m_PreMixHookMap.End(); ++lIter)
            {
                h = (HXAudioHookInfo*) (*lIter);
                if (h->bIgnoreAudioData ||
                    HXR_OK == ProcessAudioHook(ACTION_CHECK, h->pHook))
                {
                    h->pHook->OnInit( &m_AudioFmt );
                }
            }
        }
    }
#endif /* HELIX_FEATURE_AUDIO_PREMIXHOOK */

    m_bHooksInitialized = TRUE;
}

/************************************************************************
 *  Method:
 *              IHXAudioStream::ProcessHooks
 *      Purpose:
 */
HX_RESULT CHXAudioStream::ProcessHooks( HXAudioData* pInData,
                                        HXAudioData* pOutData
                                        )
{
    HX_RESULT theErr = HXR_OK;

#if defined(HELIX_FEATURE_AUDIO_PREMIXHOOK)
    m_pInDataPtr->pData         = pInData->pData;
    m_pInDataPtr->pData->AddRef();
    m_pInDataPtr->ulAudioTime   = pInData->ulAudioTime;
    
    m_pOutDataPtr->pData        = NULL;
    m_pOutDataPtr->ulAudioTime  = pInData->ulAudioTime;

    m_pInDataPtr->uAudioStreamType    = pInData->uAudioStreamType;
    m_pOutDataPtr->uAudioStreamType   = pInData->uAudioStreamType;

    if ( m_PreMixHookMap.GetCount() > 0 )
    {
        HXAudioHookInfo* pPreMixHookInfo = 0;
        CHXMapPtrToPtr::Iterator lIter = m_PreMixHookMap.Begin();
        for (; !theErr && lIter != m_PreMixHookMap.End(); ++lIter)
        {
            pPreMixHookInfo = (HXAudioHookInfo*) (*lIter);

            if (HXR_OK == ProcessAudioHook(ACTION_CHECK, pPreMixHookInfo->pHook))
            {
                // XXXHP, disable hooks when it doesn't support multi-channel
                if (m_AudioFmt.uChannels <= 2 || pPreMixHookInfo->bMultiChannelSupport)
                {
                    theErr = pPreMixHookInfo->pHook->OnBuffer( m_pInDataPtr, m_pOutDataPtr);

                    /* Check to see if renderer changed the buffer. If so, then
                     * make this output as input to the next Hook.
                     */
                    if (!theErr && m_pOutDataPtr->pData)
                    {
                        m_pInDataPtr->pData->Release();

                        m_pInDataPtr->pData       = m_pOutDataPtr->pData;
                        m_pInDataPtr->ulAudioTime = m_pOutDataPtr->ulAudioTime;
                        m_pOutDataPtr->pData      = 0;
                    }
                }
            }
            else if (pPreMixHookInfo->bIgnoreAudioData)
            {
                IHXBuffer* pTempBuf = m_pInDataPtr->pData;
                m_pInDataPtr->pData = NULL;
                theErr = pPreMixHookInfo->pHook->OnBuffer( m_pInDataPtr, m_pOutDataPtr);
                m_pInDataPtr->pData = pTempBuf;
            }
        }
    }

    /* Final output is always in InDataPtr*/
    pOutData->pData             = m_pInDataPtr->pData;
    pOutData->ulAudioTime       = m_pInDataPtr->ulAudioTime;
    pOutData->uAudioStreamType  = m_pInDataPtr->uAudioStreamType;
#endif /* HELIX_FEATURE_AUDIO_PREMIXHOOK */

    return theErr;
}

/************************************************************************
 *  IHXAudioStream3 methods
 */
/************************************************************************
 *  Method:
 *      IHXAudioStream3::Stop()
 *  Purpose:
 *	   Stops the mixing of the stream into the audio session, 
 *	   flushes all data present in the audio stream and de-initializes 
 *	   it.  Stream must be re-initailized before resuming.
 */
STDMETHODIMP CHXAudioStream::Stop()
{
    HXLOGL3(HXLOG_ADEV, "CHXAudioStream[%p]::Stop()", this);
    if (m_eState == E_STOPPED)
    {
        return HXR_OK;
    }
    
    m_eState = E_STOPPED;
    ResetStream();

    return HXR_OK;
}

/************************************************************************
 *  Method:
 *      IHXAudioStream3::Pause
 *  Purpose:
 *	   Pauses the playback of audio stream thus suspending mixing of
 *	   the stream data into the audio session while retaining all
 *	   data within the stream.  On resumption, mixing will resume at
 *	   same point it was suspended assuming no other intervening
 *	   commands were given.
 */
STDMETHODIMP CHXAudioStream::Pause()
{
    _Pause(FALSE);

    return HXR_OK;
}

/************************************************************************
 *  Method:
 *      IHXAudioStream3::Resume
 *  Purpose:
 *	   Resumes the playback of audio stream thus resuming mixing of
 *	   the stream data into the audio session while retaining all
 *	   data within the stream.  On resumption, mixing will resume at
 *	   same point it was suspended assuming no other intervening
 *	   commands were given.
 */
STDMETHODIMP CHXAudioStream::Resume()
{
    _Resume(FALSE);

    return HXR_OK;
}

/************************************************************************
 *  Method:
 *      IHXAudioStream3::Flush
 *  Purpose:
 *	   Flushes all data from the audio stream.  If the stream is paused
 *	   at the time of the call, the audio stream is also rewound to the
 *	   current playback time and made ready to receive new data for
 *	   mixing at or after the current playback time.
 */
STDMETHODIMP CHXAudioStream::Flush()
{
    HXLOGL3(HXLOG_ADEV,
            "CHXAudioStream[%p]::Flush()",
            this);

    m_ulLastAdjustedTimeDiff = 0;

    m_bFirstWrite           = TRUE;
    m_bTobeTimed            = TRUE;
    m_ulTSRollOver          = 0;
    m_ulLastInputStartTime  = 0;
    m_ulLastInputEndTime    = 0;
    // Remove all buffers from auxlliary list. This means that a 
    // renderer must send all buffers again including those buffers
    // that start at time t way out there in the future.
    FlushBuffers();
    CleanupRAByToTs();
    
    /* Remove any cross-fading */
    m_bCrossFadingToBeDone  = FALSE;
    HX_RELEASE(m_pCrossFadeStream);

    /* clear the pending bytes buffer */
    m_ulPendingAudioBytes = 0 ;
    HX_RELEASE(m_piPendingAudioData) ;

    // If we are in paused state, flush resets the stream back to 
    // initialized state so that data written after the flush can 
    // be mixed at or after current audio player time.
    // Otherwise, new data could only be mixed audio pushdown duration
    // after the current playback time.
    // 
    if (m_eState == E_PAUSED)
    {
	m_eState = E_INITIALIZED;
    }

    // If we are not playing, the stream is considered rewound and can
    // thus rejoin the session at head of the audio data queue.
    if (m_eState != E_PLAYING)
    {
	m_bIsRewound = TRUE;
	m_bBeyondStartTime = FALSE;
	m_ulStartTime = 0;
	m_ulSeekTime = 0;
    }

    return HXR_OK;
}

/************************************************************************
 *  Method:
 *      IHXAudioStream3::Rewind
 *  Purpose:
 *	   Rewinds the audio stream to the current playback time while
 *	   restoring any data that might have been stored in the audio
 *	   stream after being mixed into the mix buffer.
 */
STDMETHODIMP CHXAudioStream::Rewind()
{
    HX_RESULT retVal = HXR_FAIL;
    INT32 lTimeToRewind = 0;

    if (m_bIsRewound)
    {
	return HXR_OK;
    }

    if (m_Owner && m_Owner->m_Owner)
    {
	lTimeToRewind = m_Owner->m_Owner->GetCurrentRawAudioDevicePushdown();
	RewindStream(lTimeToRewind);
    }

    return retVal;
}

/************************************************************************
 *  Method:
 *      IHXAudioStream3::Reconfig
 *  Purpose:
 *	   Reconfigures already initialized audio stream with the indicated
 *	   parameters.  The following parameters are currently defined:
 *		"audioDeviceReflushHint" (UINT32):  0 = no data storage for
 *							rewind of the audio
 *							session should be kept
 *						    1 = data storage for
 *							rewind of the audio
 *							session should be kept
 */
STDMETHODIMP CHXAudioStream::Reconfig(IHXValues* pParams)
{
    HX_RESULT retVal = HXR_INVALID_PARAMETER;

    if (pParams)
    {
	UINT32 ulVal = 0;

	retVal = HXR_OK;

	retVal = pParams->GetPropertyULONG32("audioDeviceReflushHint", ulVal);
	if (SUCCEEDED(retVal))
	{
	    SetAudioDeviceReflushHint((ulVal != 0));
	}
	retVal = HXR_OK;
    }

    return retVal;
}

/************************************************************************
 *  Method:
 *      IHXAudioStream3::IsResumed
 *  Purpose:
 *	   Informs the caller if the audio stream is resumed (in playing
 *	   state) and thus if its data is being mixed into the audio session
 *	   given that the owning audio player and session are resumed as 
 *	   well.
 */
STDMETHODIMP_(HXBOOL) CHXAudioStream::IsResumed()
{
    return (m_eState == E_PLAYING);
}

/************************************************************************
 *  Method:
 *      IHXAudioStream3::IsRewound
 *  Purpose:
 *	   Informs the caller if the audio stream is rewound and thus is
 *	   capable of receiving data at or after the current playback time.
 *	   If stream is not rewound, it is capable of playing (mixing)
 *	   data timed for the tail of the mix-buffer which is pushdown worth
 *	   ahead of the current playback time 
 *	   (see IHXAudioPushdown2::GetCurrentAudioDevicePushdown).
 */
STDMETHODIMP_(HXBOOL) CHXAudioStream::IsRewound()
{
    return m_bIsRewound;
}

/************************************************************************
 *  CHXAudioStream methods
 */
/************************************************************************
 *  Method:
 *      CHXAudioStream::MixIntoBuffer
 *  Purpose:
 *      Mix stream data into this pPlayerBuf.
 *
 */
HX_RESULT CHXAudioStream::MixIntoBuffer( UCHAR*   pPlayerBuf,
                                         ULONG32  ulBufSize,
                                         ULONG32& ulBufTime,
                                         HXBOOL&  bIsMixBufferDirty,
                                         HXBOOL&  bOptimizedMixing,
                                         CHXSimpleList* pStreamBufferList,
                                         HXBOOL   bGetCrossFadeData
                                         )
{
    HX_RESULT res = HXR_OK;
    
    if (!m_bInited)
    {
        return HXR_NOT_INITIALIZED;
    }

    // bGetCrossFadeData should now be a thing of the past.
    HX_ASSERT(!bGetCrossFadeData) ;

    //HXLOGL4(HXLOG_ADEV, "CHXAudioStream[%p]::MixIntoBuffer(): last write time = %lu", this, m_ulLastWriteTime);

    /* If this is a *FROM* stream, we may have already mixed
     * data during cross-fade with *TO* stream
     */

    // update the outside world's sense of time.
    // XXX wschildbach: how to account for rollover?
    INT64 llNextMixTime = m_pMixEngine->GetNextMixTimeMillis();
    UINT32 ulLastWriteTime = INT64_TO_UINT32(llNextMixTime);

    // Below we set the mixed in buffer time in player time coordinates
    // if larger than time passed in.
    // This allows the coller to establish the maximum buffer time of all
    // streams.
    // Time 0 is treated specially (as unset case) and buffer time is 
    // always set in this case.
    if ((ulBufTime == 0) || (((LONG32) (ulLastWriteTime - ulBufTime)) > 0))
    {
        ulBufTime = ulLastWriteTime;
    }
    
    // If there are any DryNotifications and the data list is empty
    // we need to notify them so that they can write more data.
    
    // If EnoughDataAvailable() returns FALSE, it will have updated
    // the input values to point at where the buffer needs to be filled.
    INT64  llStartMix, llEndMix;

    m_pMixEngine->GetMixRange(ulBufSize, llStartMix, llEndMix) ;
    UINT32 nSamplesNeeded = INT64_TO_UINT32(llEndMix - llStartMix) ; // always fits into UINT32

    // Keep a copy since EnoughDataAvailable can change llStartMix and nSamplesNeeded
    INT64 llStartMix0 = llStartMix;
    UINT32 nSamplesNeeded0 = nSamplesNeeded;

    if (!EnoughDataAvailable(llStartMix, nSamplesNeeded))
    {
        HXLOGL3(HXLOG_ADEV,
                "CHXAudioStream[%p]::MixIntoBuffer(): not enough data; mix start = %ld; samps needed = %lu",
                this,
                INT64_TO_UINT32(llStartMix),
                nSamplesNeeded);

        // signal the renderer to send data faster, the renderer will relay this
        // to the source which sets m_rebufferStatus = REBUFFER_WARNING
        if (m_DryNotificationMap->GetCount() > 0)
        {
            HXBOOL bWouldBlock = FALSE;
            IHXDryNotification* pDryNotification = 0;
            CHXMapPtrToPtr::Iterator lIter = m_DryNotificationMap->Begin();
            for (; lIter != m_DryNotificationMap->End(); ++lIter)
            {
                pDryNotification = (IHXDryNotification*) (*lIter);
                UINT64 streamTime = Samples2Ms(llStartMix, &m_AudioFmt) -
                    CAST_TO_INT64(m_ulTSRollOver)*CAST_TO_INT64(MAX_UINT32);
                // Ask for ms worth od samples rounded up to nearest millisecond..
		HXLOGL3(HXLOG_ADEV,
			"CHXAudioStream[%p]::MixIntoBuffer(): Issuing Dry Notification: StreamTime=%lu AudioDurationNeeded=%lu",
			this,
			INT64_TO_UINT32(streamTime),
			INT64_TO_UINT32(Samples2Ms(nSamplesNeeded,
						   &m_AudioFmt,
						   TRUE)));

                HX_RESULT theErr =
                    pDryNotification->OnDryNotification( INT64_TO_UINT32(streamTime),
                                                         INT64_TO_UINT32(Samples2Ms(nSamplesNeeded,
                                                                                    &m_AudioFmt,
                                                                                    TRUE))
                                                         );
                if (theErr == HXR_OUTOFMEMORY)
                {
                    return theErr;
                }
                else if (theErr == HXR_WOULD_BLOCK)
                {
                    bWouldBlock = TRUE;
                }
            }

            if (bWouldBlock)
            {
                return HXR_WOULD_BLOCK;
            }

            if (m_Owner->GetState() != E_PLAYING)
            {
                return HXR_OK;
            }
        }
    }

    // this call does all the mixing.
    res = m_pMixEngine->MixIntoBuffer(pPlayerBuf, ulBufSize, 
                           bIsMixBufferDirty, bOptimizedMixing, m_bIsOpaqueStream);
                           
    if( m_wLastError == HXR_OUTOFMEMORY )
    {
        return m_wLastError;
    }
 
    if (FAILED(res))
    {
        return res ; 
    }

    // if we are still in optimized mode, it means that mix engine doesn't fill the pPlayerBuf
    // we'll create the pStreamBufferList directly
    if (bOptimizedMixing)
    {
        CreateDirectOutput(pStreamBufferList, llStartMix0, nSamplesNeeded0);
    }

#if defined(HELIX_FEATURE_AUDIO_INACCURATESAMPLING)
    if( m_bRealAudioStream )
    {
        MapFudgedTimestamps();
    }
#endif    
    
    // update the inside world's sense of time.
    m_llLastWriteTime = m_pMixEngine->GetNextMixTimeMillis();
    m_bIsRewound = FALSE;
    if (m_bHasStartTime && (!m_bBeyondStartTime))
    {
	INT64 llStartTime = CAST_TO_INT64 m_ulStartTime;
	if (m_llLastWriteTime >= llStartTime)
	{
	    m_bBeyondStartTime = TRUE;
	    HXLOGL3(HXLOG_ADEV,
			"CHXAudioStream[%p]::MixIntoBuffer(): Passed StartTime=%lu at LastWriteTime=%lu",
			this,
			m_ulStartTime,
			INT64_TO_UINT32(m_llLastWriteTime));
	}
    }
    
    // This notfies the player owning the audio stream that data has been 
    // written into the audio device.  This is done for purpose of 
    // determining the need for rewind of the audio session when the 
    // player is stopped (removed from the audio session).
    m_Owner->DataInAudioDevice(TRUE);

    HXLOGL4(HXLOG_ADEV,
            "CHXAudioStream[%p]::MixIntoBuffer(): buf time = %lu; m_llLastWriteTime = %lu",
            this,
            ulBufTime,
            INT64_TO_UINT32(m_llLastWriteTime)
            );

    return HXR_OK;
}

// helper method to remove old packets
HX_RESULT CHXAudioStream::RemoveOldPackets(INT64 llStartTimeInSamples)
{
    // remove old packets. Old packets are packets that have an end time that is
    // before our current mix time.
    HXAudioInfo* pInfo = NULL;
    LISTPOSITION lp = m_pDataList->GetHeadPosition();
    while( lp )
    {
        LISTPOSITION lastlp = lp;
        pInfo   = (HXAudioInfo*) m_pDataList->GetNext(lp);

        if (pInfo->llEndTimeInSamples < llStartTimeInSamples)
        {
            HXLOGL4(HXLOG_ADEV,
                    "CHXAudioStream[%p]::RemoveOldPackets: reaping packet: start = %lu; end = %lu",
                    this,
                    INT64_TO_UINT32(pInfo->llStartTimeInSamples),
                    INT64_TO_UINT32(pInfo->llEndTimeInSamples));

            FreeInfo(pInfo);
            m_pDataList->RemoveAt(lastlp);
            if( m_wLastError == HXR_OUTOFMEMORY )
            {
                return FALSE;
            }
        }
        else 
        {
            // if monotonous and non-overlapping
            break ;
        }
    }
    return HXR_OK;
}

HX_RESULT CHXAudioStream::AddSilenceBuffer(CHXSimpleList* pStreamBufferList, UINT32 ulSizeInBytes)
{
    //on-demand create/enlarge the silence buffer. this may not be
    //the best strategy
    if (!m_pSilenceBuffer)
    {
      HX_ASSERT(m_pCommonClassFactory);
      IUnknown* pUnk = NULL;
      if (HXR_OK == m_pCommonClassFactory->CreateInstance(CLSID_IHXBuffer, (void**)&pUnk))
      {
          pUnk->QueryInterface(IID_IHXBuffer, (void**)&m_pSilenceBuffer);
          HX_RELEASE(pUnk);
      }
      else
      {
          return HXR_OUTOFMEMORY;
      }
    }
    
    if (m_pSilenceBuffer->GetSize() < ulSizeInBytes)
    {
        m_pSilenceBuffer->SetSize(ulSizeInBytes);
        
        if (m_pSilenceBuffer->GetSize() != ulSizeInBytes)
        {
           //cannot resize because refcount > 1, create a new buffer
           HX_RELEASE(m_pSilenceBuffer);
           HX_ASSERT(m_pCommonClassFactory);
           IUnknown* pUnk = NULL;
           if (HXR_OK == m_pCommonClassFactory->CreateInstance(CLSID_IHXBuffer, (void**)&pUnk))
           {
               pUnk->QueryInterface(IID_IHXBuffer, (void**)&m_pSilenceBuffer);
               HX_RELEASE(pUnk);
               m_pSilenceBuffer->SetSize(ulSizeInBytes);
           }
           else
           {
               return HXR_OUTOFMEMORY;
           }
        }     
        HX_ASSERT(m_pSilenceBuffer->GetBuffer() && m_pSilenceBuffer->GetSize() == ulSizeInBytes);
        memset(m_pSilenceBuffer->GetBuffer(), 0, ulSizeInBytes);
    }	            
    
    IHXBuffer* pSilenceBuffer = (IHXBuffer*) m_pSilenceBuffer;     
    if (ulSizeInBytes < m_pSilenceBuffer->GetSize())
    {
        pSilenceBuffer = (IHXBuffer*) new CHXBufferFragment(m_pSilenceBuffer, 
                                                            m_pSilenceBuffer->GetBuffer(),
                                                            ulSizeInBytes);
        if (pSilenceBuffer == NULL)
        {
           return HXR_OUTOFMEMORY;
        }
    }
    pSilenceBuffer->AddRef();
    pStreamBufferList->AddTail(pSilenceBuffer);

    return HXR_OK;
}             

// This method creates a list of data buffers with the total requested size.
// It avoids memcpy by using CHXBufferFragment
HX_RESULT CHXAudioStream::CreateDirectOutput(CHXSimpleList* pStreamBufferList, INT64 llStartTimeInSamples, UINT32 nSamples)
{
    HXAudioInfo* pInfo              = 0;
    LISTPOSITION lp                 = 0;
    INT32        nBytesPerSample    = m_AudioFmt.uBitsPerSample>>3 ;

    HXLOGL4(HXLOG_ADEV, "CHXAudioStream[%p]::CreateDirectOutput(): start samples = %lu; sample count = %lu", this, INT64_TO_UINT32(llStartTimeInSamples), nSamples);
 
    // there are two lists of packets here: timed audio and instantaneous audio.
    // We only look into the list for timed buffers -- Instantaneoue audio is ignored
    // (it never properly worked anyway, so support is discontinued).

    // remove old packets. Old packets are packets that have an end time that is
    // before our current mix time.
    if (RemoveOldPackets(llStartTimeInSamples) != HXR_OK) 
    {
        return FALSE;
    }
    
    // now go through the entire list of packets
    // assume the packet timestamps are monotonously increase and non-overlapping
    // we'll fill the gaps with silent packets
    // the start and end points likely need 'partial' packets 

    lp = m_pDataList->GetHeadPosition();
    while( lp && nSamples > 0)
    {
        pInfo = (HXAudioInfo*) m_pDataList->GetNext(lp);
        
        if (pInfo->llStartTimeInSamples > llStartTimeInSamples)
        {
            //there is a gap, fill with a silence buffer
            UINT32 gap = INT64_TO_UINT32(pInfo->llStartTimeInSamples - llStartTimeInSamples);
            AddSilenceBuffer(pStreamBufferList, gap * nBytesPerSample);     
            llStartTimeInSamples += gap;
            nSamples -= gap;
        }
        
        IHXBuffer* pBuffer = pInfo->pBuffer;

        UINT32 ulStartSamples = 0;
        if (pInfo->llStartTimeInSamples < llStartTimeInSamples)
        {
            // skip some data from the beginning
            ulStartSamples = INT64_TO_UINT32(llStartTimeInSamples - pInfo->llStartTimeInSamples);
        }

        UINT32 ulSizeSamples = pInfo->pBuffer->GetSize()/nBytesPerSample - ulStartSamples;
        if (ulSizeSamples > nSamples)
        {
           //enough data, this is the last packet
           ulSizeSamples = nSamples;
        }
        
        if (ulStartSamples > 0 || ulSizeSamples * nBytesPerSample != pInfo->pBuffer->GetSize())
        {
           //we need only part of the packet
           pBuffer = (IHXBuffer*) new CHXBufferFragment(
                            pInfo->pBuffer,
                            pInfo->pBuffer->GetBuffer() + ulStartSamples * nBytesPerSample,
                            ulSizeSamples * nBytesPerSample);
        }
    
        pBuffer->AddRef();
        pStreamBufferList->AddTail(pBuffer);
        
        llStartTimeInSamples += ulSizeSamples;
        nSamples -= ulSizeSamples;
    }

    if (nSamples > 0)
    {
       //use used all the buffers, fill some silence data at the end
       AddSilenceBuffer(pStreamBufferList, nSamples * nBytesPerSample);
    }
    
    return HXR_OK;        
}

// This is the callback function that m_pMixEngine->MixIntoBuffer() will call to
// read new samples.
HXBOOL CHXAudioStream::ConvertIntoBuffer(tAudioSample* buffer, UINT32 nSamples, INT64 llStartTimeInSamples)
{
    HXAudioInfo* pInfo              = 0;
    LISTPOSITION lp                 = 0;
    INT32        nBytesPerSample    = m_AudioFmt.uBitsPerSample>>3 ;
    HXBOOL       didMix             = FALSE ; // set to TRUE if we have packets in this time range
    HXBOOL       bPacketsAfterRange = FALSE;

    HXLOGL4(HXLOG_ADEV, "CHXAudioStream[%p]::ConvertIntoBuffer(): start samples = %lu; sample count = %lu", this, INT64_TO_UINT32(llStartTimeInSamples), nSamples);
 
    // there are two lists of packets here: timed audio and instantaneous audio.
    // We only look into the list for timed buffers -- Instantaneoue audio is ignored
    // (it never properly worked anyway, so support is discontinued).

    // remove old packets. Old packets are packets that have an end time that is
    // before our current mix time.
    if (RemoveOldPackets(llStartTimeInSamples) != HXR_OK) 
    {
        return FALSE;
    }

    // now go through the entire list of packets, and look for overlap with the
    // convert buffer. Any packet with overlap will be at least partially converted
    // into the buffer.

    // If packets overlap, one packet will then take precedence over another -- not
    // much we can do about that.

    lp = m_pDataList->GetHeadPosition();
    while( lp )
    {
        pInfo = (HXAudioInfo*) m_pDataList->GetNext(lp);

        if (pInfo->llStartTimeInSamples < llStartTimeInSamples + nSamples &&
            pInfo->llEndTimeInSamples > llStartTimeInSamples)
        {
            // This packet has some overlap with what we are converting.

            // if this is the first packet to be mixed into this buffer,
            // silence out the entire buffer. This is inefficient, but safe
            if (!didMix)
            {
                CAudioSvcSampleConverter::silence(buffer, nSamples) ;
            }

            didMix = TRUE ;

            INT32 nMixbufferOffset = 0;
            INT32 pastPacketStart  = INT64_TO_INT32(llStartTimeInSamples - pInfo->llStartTimeInSamples) ;
            if (pastPacketStart < 0)
            {
                nMixbufferOffset = -pastPacketStart;
                pastPacketStart  = 0;
            }

            INT32 nn = Bytes2Samples(pInfo->pBuffer->GetSize(), &m_AudioFmt);
            INT32 nSamplesToUse = nn - pastPacketStart;
            if (nSamplesToUse > (INT32)(nSamples - nMixbufferOffset))
                nSamplesToUse = nSamples - nMixbufferOffset;

            const unsigned char *cvtin = pInfo->pBuffer->GetBuffer() + pastPacketStart * nBytesPerSample;
            tAudioSample *cvtout = buffer + nMixbufferOffset;

            HXLOGL4(HXLOG_ADEV, "CHXAudioStream[%p]::ConvertIntoBuffer(): mix packet: start = %lu; end = %lu; sample count = %lu", 
                    this, INT64_TO_UINT32(pInfo->llStartTimeInSamples), INT64_TO_UINT32(pInfo->llEndTimeInSamples), nn);
            HXLOGL4(HXLOG_ADEV, "CHXAudioStream[%p]::ConvertIntoBuffer(): into buffer: offset = %lu; sample count = %lu", 
                    this, nMixbufferOffset, nSamplesToUse);
       
            switch (nBytesPerSample)
            {
               case 1:
                   CAudioSvcSampleConverter::cvt8(cvtin, cvtout, nSamplesToUse);
                   break ;
               case 2:
                   CAudioSvcSampleConverter::cvt16(cvtin, cvtout, nSamplesToUse);
                   break ;
               case 4:
                   CAudioSvcSampleConverter::cvt32(cvtin, cvtout, nSamplesToUse);
                   break ;
            }
        }
        else if (pInfo->llStartTimeInSamples >= llStartTimeInSamples + nSamples)
        {
            // We've found audio data that is past the desired range
            bPacketsAfterRange = TRUE;
        }
    }

    if (!didMix && bPacketsAfterRange)
    {
        /* We do not have packets for this range, but we
         * do have packets after the range.
         * Create silence data and make it look like we
         * actually had data for this range.
         */
        CAudioSvcSampleConverter::silence(buffer, nSamples) ;
        didMix = TRUE;
    }

    return didMix ;
}



/************************************************************************
 *  Method:
 *              CHXAudioStream::Bytes2Samples
 *      Purpose:
 *              Translate from units of bytes to samples.
 */
UINT32 CHXAudioStream::Bytes2Samples( UINT64               ulNumBytes,
                                      const HXAudioFormat* fmt 
                                      )
{
    ASSERT(ulNumBytes % (fmt->uBitsPerSample >> 3) == 0) ;
    return INT64_TO_UINT32(ulNumBytes / (fmt->uBitsPerSample >> 3)) ;
}

/************************************************************************
 *  Method:
 *              CHXAudioStream::Samples2Ms
 *      Purpose:
 *              Calculate the duration in millisecs for this number of samples.
 */
UINT64 CHXAudioStream::Samples2Ms( INT64 nSamples,
                                   const HXAudioFormat *fmt,
                                   HXBOOL bRoundUp
                                   )
{
    INT32 lDenom = fmt->uChannels * fmt->ulSamplesPerSec;
    INT64 q = nSamples / lDenom;
    INT64 r = nSamples - q * lDenom;

    if (nSamples > 0)
    {
	return q * 1000 + (r * 1000 + (bRoundUp ? (lDenom - 1) : 0)) / lDenom;
    }
    else
    {
	return q * 1000 + (r * 1000 + (bRoundUp ? 0 : (1 - lDenom))) / lDenom;
    }
}


/************************************************************************
 *  Method:
 *              CHXAudioStream::CalcMs
 *      Purpose:
 *              Calculate the duration in millisecs for this number of
 *              bytes in input format.
 */
ULONG32 CHXAudioStream::CalcMs(ULONG32 ulNumBytes)
{
    return INT64_TO_ULONG32(Samples2Ms(Bytes2Samples(ulNumBytes, &m_AudioFmt), &m_AudioFmt));
}

/************************************************************************
 *  Method:
 *              CHXAudioStream::CalcDeviceMs
 *      Purpose:
 *              Calculate the duration in millisecs for this number of 
 *              bytes in Device format.
 */
ULONG32 CHXAudioStream::CalcDeviceMs(ULONG32 ulNumBytes)
{
    return INT64_TO_ULONG32(Samples2Ms(Bytes2Samples(ulNumBytes, &m_DeviceFmt), &m_DeviceFmt));
}

/************************************************************************
 *  Method:
 *              CHXAudioStream::CalcOffset
 *      Purpose:
 *              Calculate the offset in bytes given time.
 */
UINT32 CHXAudioStream::CalcOffset(INT64 llStartTime, INT64 llEndTime)
{
    /* Using m_ulBytesPerMs may introduce cumulative error due 
     * to decimal cutoff 
     */
    HX_ASSERT(llEndTime - llStartTime < MAX_TIMESTAMP_GAP);

    return m_ulGranularity ?
        INT64_TO_UINT32((llEndTime - llStartTime) * m_ulInputBytesPerGran / m_ulGranularity) :
        0 ;
}


void CHXAudioStream::FlushBuffers(HXBOOL bInstantaneousAlso)
{
    HXLOGL3(HXLOG_ADEV, "CHXAudioStream[%p]::FlushBuffers()", this);
    while (m_pDataList && m_pDataList->GetCount() > 0)
    {
        HXAudioInfo* pInfo = (HXAudioInfo*) m_pDataList->RemoveHead();
        FreeInfo(pInfo);
    }

    while (bInstantaneousAlso && m_pInstantaneousList && m_pInstantaneousList->GetCount() > 0)
    {
        CHXSimpleList* pList = (CHXSimpleList*) m_pInstantaneousList->RemoveHead();
        while (pList->GetCount() > 0)
        {
            HXAudioInfo* pInfo = (HXAudioInfo*) pList->RemoveHead();
            FreeInfo(pInfo, TRUE);
        }

        HX_DELETE(pList);
    }

    // reset m_bLastNMilliSecsToBeSaved so that we actually 
    // delete buffers in FreeInfo
    HXBOOL bLastNMilliSecsToBeSaved = m_bLastNMilliSecsToBeSaved;
    m_bLastNMilliSecsToBeSaved = FALSE;

    while (m_pLastNMilliSecsList && m_pLastNMilliSecsList->GetCount() > 0)
    {
        HXAudioInfo* pInfo = (HXAudioInfo*) m_pLastNMilliSecsList->RemoveHead();
        FreeInfo(pInfo);
    }

    m_bLastNMilliSecsToBeSaved = bLastNMilliSecsToBeSaved;

    HX_DELETE(m_pLastNMilliSecsList);
}

/*
  this routine checks if there are enough packets waiting in the queue
  to be mixed. It will return FALSE if not, or if there are packets missing
  in the middle of the queue.
*/

HXBOOL
CHXAudioStream::EnoughDataAvailable(INT64& llStartTimeInSamples, UINT32& nSamplesRequired)
{
    INT64           llEndTimeInSamples   = llStartTimeInSamples + nSamplesRequired ;

    HXBOOL            bDataAvailable  = FALSE;
    HXAudioInfo*    pInfoOld        = 0 ;
    HXAudioInfo*    pInfo           = 0;
    LISTPOSITION    lp              = 0;

    // if the list is completely empty, check if data requested
    // is prior to the start time.
    // If so, we have enough data (silence) for this request.
    if (m_pDataList->IsEmpty())
    {
	if (m_bHasStartTime && (!m_bBeyondStartTime))
	{
	    INT64 llEndTimeInMs = Samples2Ms(llEndTimeInSamples, &m_AudioFmt);
	    INT64 llStartTime = CAST_TO_INT64 m_ulStartTime;

	    if (llStartTime >= llEndTimeInMs)
	    {
		return TRUE;
	    }
	}
        return FALSE;
    }

    nSamplesRequired = 0 ;

    /* skip over old packets. Old packets are packets that have an end time that is before
       our current mix time. */
    lp = m_pDataList->GetHeadPosition();
    while( lp )
    {
        pInfoOld = (HXAudioInfo*) m_pDataList->GetNext(lp);

        if (pInfoOld->llEndTimeInSamples >= llStartTimeInSamples)
            break ;
    }

#if 0 // disabled missing packet detection
    // pInfoOld is the first packet to be mixed. To make sure it overlaps with the start
    // of the mix buffer, do this (disabled for now):
    
    if (pInfoOld->llStartTimeInSamples > llStartTimeInSamples)
    {
        return FALSE ;
    }
    
#endif    

    // now go through the rest of packets, and make sure they are contiguous until
    // the end of our mix time

    // If packets overlap, one packet will then take precedence over another -- not
    // much we can do about that.

    while( lp )
    {
        pInfo = (HXAudioInfo*) m_pDataList->GetNext(lp);

        // if we see a packet with a timestamp after the mix time ("future packet")
        // or one that does not abut with the previous one ("discontinuity"), stop.
        if (pInfo->llStartTimeInSamples >= llEndTimeInSamples)         // future packet
//            pInfo->llStartTimeInSamples != pInfoOld->llEndTimeInSamples) // discontinuity
        {
            bDataAvailable = TRUE;
            break ;
        }

        pInfoOld = pInfo ;
    }

    // XXX HP skip the discontinuity check if we know there is enough data available
    if (!bDataAvailable)
    {
        // pInfoOld is the last packet to be mixed (or the last before a discontinuity).
        // Make sure it overlaps with the end of the mix buffer.
        if (pInfoOld->llEndTimeInSamples >= llEndTimeInSamples)
        {
            bDataAvailable = TRUE;
        }
        else
        {
            llStartTimeInSamples = pInfoOld->llEndTimeInSamples ;
            nSamplesRequired = INT64_TO_UINT32(llEndTimeInSamples - llStartTimeInSamples) ;
        }
    }

    return bDataAvailable;
}

HX_RESULT    
CHXAudioStream::StartCrossFade(CHXAudioStream*  pFromStream, 
                               UINT32           ulCrossFadeStartTime,
                               UINT32           ulCrossFadeDuration, 
                               HXBOOL           bToStream)
{
    HXLOGL3(HXLOG_ADEV, "CHXAudioStream[%p]::StartCrossFade(): start = %lu; duration = %lu; to stream = %s", this, ulCrossFadeStartTime, ulCrossFadeDuration, bToStream ? "true" : "false");
#if defined(HELIX_FEATURE_CROSSFADE)
    // XXX wschildbach need to account for rollover.
    INT64 llStartTimeInSamples = CAST_TO_INT64(ulCrossFadeStartTime) * m_DeviceFmt.ulSamplesPerSec / 1000 * m_DeviceFmt.uChannels ;
    INT64 llEndTimeInSamples = (CAST_TO_INT64(ulCrossFadeStartTime)+ulCrossFadeDuration) * m_DeviceFmt.ulSamplesPerSec / 1000 * m_DeviceFmt.uChannels ;

    m_pMixEngine->SetCrossFade(bToStream ? HXAudioSvcMixEngine::FADE_IN : HXAudioSvcMixEngine::FADE_OUT,
                               llStartTimeInSamples, llEndTimeInSamples);

    HXLOGL3(HXLOG_ADEV, "CHXAudioStream[%p]::StartCrossFade(): samps start = %lu; end = %lu", this, INT64_TO_UINT32(llStartTimeInSamples), INT64_TO_UINT32(llEndTimeInSamples));

    return HXR_OK;
#else
    return HXR_NOTIMPL;
#endif /* HELIX_FEATURE_CROSSFADE */
}

/*
 *  IHXRealAudioSync methods
 */

/************************************************************************
 *  Method:
 *      IHXRealAudioSync::Register
 *  Purpose:
 */
STDMETHODIMP CHXAudioStream::Register(void) 
{
#if defined _DEBUG && defined HELIX_FEATURE_AUDIO_MULTIPLAYER_PAUSE 
    if (HXDebugOptionEnabled("zDoNotUseFudge"))
    {
        return HXR_OK;
    }
#endif

    if (m_bRealAudioStream)
    {
        return HXR_UNEXPECTED;
    }

    m_bRealAudioStream = TRUE;

#if defined(HELIX_FEATURE_AUDIO_INACCURATESAMPLING)
    if (!m_pRAByToTsInList)
    {
        m_pRAByToTsInList       = new CHXSimpleList;
        m_pRAByToTsAdjustedList = new CHXSimpleList;
    }
#endif /* HELIX_FEATURE_AUDIO_INACCURATESAMPLING */

    return HXR_OK;
}

/************************************************************************
 *  Method:
 *      IHXRealAudioSync::UnRegister
 *  Purpose:
 */
STDMETHODIMP CHXAudioStream::UnRegister(void)
{
#if defined _DEBUG && defined HELIX_FEATURE_AUDIO_MULTIPLAYER_PAUSE 
    if (HXDebugOptionEnabled("zDoNotUseFudge"))
    {
        return HXR_OK;
    }
#endif

    if (!m_bRealAudioStream)
    {
        return HXR_UNEXPECTED;
    }

    m_bRealAudioStream = FALSE;
    CleanupRAByToTs();

    return HXR_OK;
}

/************************************************************************
 *  Method:
 *      IHXRealAudioSync::FudgeTimestamp
 *  Purpose:
 *      Tell the audio stream about the relationship between the number 
 *      of bytes written to the actual timestamp.
 *          
 */
STDMETHODIMP
CHXAudioStream::FudgeTimestamp( UINT32 ulNumberofBytes,
                                UINT32 ulTimestamp)
{
    HXLOGL3(HXLOG_ADEV, "CHXAudioStream[%p]::FudgeTimestamp()", this);
#if defined(HELIX_FEATURE_AUDIO_INACCURATESAMPLING)
#if defined _DEBUG && defined HELIX_FEATURE_AUDIO_MULTIPLAYER_PAUSE 
    if (HXDebugOptionEnabled("zDoNotUseFudge"))
    {
        return HXR_OK;
    }
#endif

    RealAudioBytesToTimeStamp* pByToTs = new RealAudioBytesToTimeStamp();

    pByToTs->m_ulTimestamp   = ulTimestamp;
    pByToTs->m_ulInTimestamp = m_ulLastInputStartTime;
    pByToTs->m_ulInEndTime   = m_ulLastInputEndTime;

    pByToTs->m_ulTimestamp += m_ulLiveDelay;
    pByToTs->m_ulTimestamp -= m_ulBaseTime;
    
    pByToTs->m_ulOrigTimestamp = pByToTs->m_ulTimestamp;

    m_pRAByToTsInList->AddTail((void*) pByToTs);
#endif /* HELIX_FEATURE_AUDIO_INACCURATESAMPLING */

    HXLOGL4(HXLOG_ADEV, "CHXAudioStream[%p]::FudgeTimestamp(): time = %lu; last start time = %lu", this, ulTimestamp, m_ulLastInputStartTime);

    return HXR_OK;
}

void
CHXAudioStream::CleanupRAByToTs(void)
{
#if defined(HELIX_FEATURE_AUDIO_INACCURATESAMPLING)
    if (!m_pRAByToTsInList)
    {
        return;
    }

    CHXSimpleList::Iterator ndx = m_pRAByToTsInList->Begin();
    for (; ndx != m_pRAByToTsInList->End(); ++ndx)
    {
        RealAudioBytesToTimeStamp* pByToTs = 
            (RealAudioBytesToTimeStamp*) (*ndx);
        delete pByToTs;
    }

    m_pRAByToTsInList->RemoveAll();

    ndx = m_pRAByToTsAdjustedList->Begin();
    for (; ndx != m_pRAByToTsAdjustedList->End(); ++ndx)
    {
        RealAudioBytesToTimeStamp* pByToTs = 
            (RealAudioBytesToTimeStamp*) (*ndx);
        delete pByToTs;
    }

    m_pRAByToTsAdjustedList->RemoveAll();
#endif /* HELIX_FEATURE_AUDIO_INACCURATESAMPLING */
} 

HX_RESULT
CHXAudioStream::ConvertCurrentTime(double dBytesPlayed, 
                                   UINT32 ulCurrentTime, 
                                   UINT32& ulAdjustedTime)
{
#if defined(HELIX_FEATURE_AUDIO_INACCURATESAMPLING)
    //In the case of non-RA streams, we have a very simple way
    //to adjust the timeline. It just basically builds ontop of
    //any previous RA-Stream's fudged mapping that might have
    //taken place.
    ulAdjustedTime = ulCurrentTime;

    if( IsRealAudioStream() )
    {
        LISTPOSITION posRABytes = m_pRAByToTsAdjustedList->GetHeadPosition();
        RealAudioBytesToTimeStamp* pByToTsLower  = NULL;
        RealAudioBytesToTimeStamp* pByToTsHigher = NULL;

        INT64 llActualByToTsHigherTimestamp = 0;
        INT64 llActualByToTsLowerTimestamp  = 0;

        while(posRABytes)
        {
            RealAudioBytesToTimeStamp* pByToTs = (RealAudioBytesToTimeStamp*) 
                m_pRAByToTsAdjustedList->GetAt(posRABytes);

            if (dBytesPlayed >= pByToTs->m_ulOutNumBytes)
            {
                pByToTsLower = pByToTs;
            }
            else
            {
                if (pByToTsLower)
                {
                    pByToTsHigher = pByToTs; 
                }
                else
                {
                    //We do not have any samples yet. Until we do, add in any
                    //previous stream's adjustment.
                    ulAdjustedTime += m_ulLastAdjustedTimeDiff;
                    return HXR_OK;
                }
            }

            if (pByToTsLower && pByToTsHigher)
            {
                break;
            }

            m_pRAByToTsAdjustedList->GetNext(posRABytes);
        }

        /* We got a range, interpolate */
        if (pByToTsLower && pByToTsHigher)
        {
            HXLOGL4(HXLOG_ADEV,
                    "CHXAudioStream[%p]::ConvertCurrentTime(): bytes played: %f; low ts: %lu; hight ts: %lu; low bytes: %f; high bytes: %f",
                    this,
                    dBytesPlayed,
                    pByToTsLower->m_ulTimestamp,
                    pByToTsHigher->m_ulTimestamp, 
                    pByToTsLower->m_ulOutNumBytes,
                    pByToTsHigher->m_ulOutNumBytes);

            /* Need to re-visit this ASSERT. A check will do for now */
#if 0
            HX_ASSERT((pByToTsHigher->m_ulTimestamp >= 
                       pByToTsLower->m_ulTimestamp) &&
                      (pByToTsHigher->m_ulOutNumBytes >= 
                       pByToTsLower->m_ulOutNumBytes));
#endif

            llActualByToTsHigherTimestamp = CAST_TO_INT64 (pByToTsHigher->m_ulTimestamp) + CAST_TO_INT64 m_ulTSRollOver * CAST_TO_INT64 MAX_UINT32;
            llActualByToTsLowerTimestamp = CAST_TO_INT64 (pByToTsLower->m_ulTimestamp) + CAST_TO_INT64 m_ulTSRollOver * CAST_TO_INT64 MAX_UINT32;

            if ((llActualByToTsHigherTimestamp >= llActualByToTsLowerTimestamp) &&
                (pByToTsHigher->m_ulOutNumBytes >= pByToTsLower->m_ulOutNumBytes))
            {
                ulAdjustedTime = pByToTsLower->m_ulTimestamp +
                    (UINT32) (((dBytesPlayed - pByToTsLower->m_ulOutNumBytes)*1./
                               (pByToTsHigher->m_ulOutNumBytes - 
                                pByToTsLower->m_ulOutNumBytes)) *
                              INT64_TO_UINT32(llActualByToTsHigherTimestamp - 
                                              llActualByToTsLowerTimestamp));

                //HXLOGL4(HXLOG_ADEV, "CHXAudioStream[%p]::ConvertCurrentTime(): ConvertLHINTER: %p\t%lu\t%lu\t%lu\t\t%lu\t%lu\t%lu\t%lu", this, m_Owner, (UINT32) dBytesPlayed, ulCurrentTime, ulAdjustedTime, pByToTsLower->m_ulTimestamp, (UINT32) pByToTsLower->m_ulOutNumBytes, pByToTsHigher->m_ulTimestamp, (UINT32) pByToTsHigher->m_ulOutNumBytes);
            }
            else 
            {
                ulAdjustedTime = pByToTsLower->m_ulTimestamp;
                //HXLOGL4(HXLOG_ADEV, "CHXAudioStream[%p]::ConvertCurrentTime(): ConvertLH: %p\t%lu\t%lu\t%lu", this, m_Owner, (UINT32) dBytesPlayed, ulCurrentTime, ulAdjustedTime);
            }
            //HXLOGL4(HXLOG_ADEV, "CHXAudioStream[%p]::ConvertCurrentTime(): ConvertLowHigh: ulCurrentTime: %lu ulAdjustedTime: %lu dBytesPlayed: %f LowTS: %lu HighTS: %lu", this, ulCurrentTime, ulAdjustedTime, dBytesPlayed,pByToTsLower->m_ulTimestamp,pByToTsHigher->m_ulTimestamp);
        }
        /* The best we can do is return the time of the nearest map */
        else if (pByToTsLower)
        {
            ulAdjustedTime = pByToTsLower->m_ulTimestamp;
            double dBytesDiff = dBytesPlayed - pByToTsLower->m_ulOutNumBytes;

            if (dBytesDiff > 0)
            {
                double dNumBytes = m_Owner->ConvertMsToBytes(pByToTsLower->m_ulDuration);
                if (dBytesDiff >= dNumBytes)
                {
                    ulAdjustedTime += pByToTsLower->m_ulDuration;
                }
                else
                {
                    ulAdjustedTime += (UINT32) (pByToTsLower->m_ulDuration * dBytesDiff *1./dNumBytes);
                }
            }
            //HXLOGL4(HXLOG_ADEV, "CHXAudioStream[%p]::ConvertCurrentTime(): ConvertLower: ulCurrentTime: %lu ulAdjustedTime: %lu dBytesPlayed: %f LowTS: %lu m_ulOutNumBytes: %f", this, ulCurrentTime, ulAdjustedTime, dBytesPlayed,pByToTsLower->m_ulTimestamp, pByToTsLower->m_ulOutNumBytes);
        }

        /* Remove all maps before pByToTsLower */
        posRABytes = m_pRAByToTsAdjustedList->GetHeadPosition();
        while(posRABytes)
        {
            RealAudioBytesToTimeStamp* pByToTs =  (RealAudioBytesToTimeStamp*) m_pRAByToTsAdjustedList->GetAt(posRABytes);
            if (pByToTs != pByToTsLower)
            {
                //HXLOGL4(HXLOG_ADEV, "CHXAudioStream[%p]::ConvertCurrentTime(): Delete: OutBytes: %f OutTS: %lu", this, pByToTs->m_ulOutNumBytes, pByToTs->m_ulTimestamp);
                delete pByToTs;
                posRABytes = m_pRAByToTsAdjustedList->RemoveAt(posRABytes);
            }
            else
            {
                break;
            }
        }
    }
    else
    {
        //Add in any adjusted time difference a previous RA-Stream might
        //have had. 
        ulAdjustedTime += m_ulLastAdjustedTimeDiff;
    }
    
#else
    ulAdjustedTime = ulCurrentTime;
#endif /* HELIX_FEATURE_AUDIO_INACCURATESAMPLING */
    //HXLOGL4(HXLOG_ADEV, "CHXAudioStream[%p]::ConvertCurrentTime(): Convert: %p\t%lu\t%lu\t%lu", this, m_Owner, (UINT32) dBytesPlayed, ulCurrentTime, ulAdjustedTime);
    return HXR_OK;
}

void CHXAudioStream::MapFudgedTimestamps(void)
{
#if defined(HELIX_FEATURE_AUDIO_INACCURATESAMPLING)
    LISTPOSITION posRABytes = m_pRAByToTsInList->GetHeadPosition();
    UINT32 ulActualByToTsInEndTime   = 0;
    UINT32 ulActualByToTsInStartTime = 0;
    UINT32 ulLastWriteTime = INT64_TO_UINT32(m_llLastWriteTime);
    
    while (posRABytes)
    {
        RealAudioBytesToTimeStamp* pByToTs = 
            (RealAudioBytesToTimeStamp*) m_pRAByToTsInList->GetAt(posRABytes);

        ulActualByToTsInStartTime = pByToTs->m_ulInTimestamp;
        ulActualByToTsInEndTime = pByToTs->m_ulInEndTime;

        /* Too late */
        if (((LONG32) (ulActualByToTsInEndTime - ulLastWriteTime)) < 0)
        {
            posRABytes = m_pRAByToTsInList->RemoveAt(posRABytes);
            HX_DELETE(pByToTs);
        }
        else if (((LONG32) (ulActualByToTsInStartTime - ulLastWriteTime)) <= 0)
        {
            // These two values will be used in determining what time it is
            // Number of bytes that have been written to the audio device till
            // now
            pByToTs->m_ulOutNumBytes = m_Owner->NumberOfBytesWritten();

            //Interpolate
            UINT32 ulTimeDiff = ulLastWriteTime - ulActualByToTsInStartTime;
            pByToTs->m_ulTimestamp += ulTimeDiff;

            pByToTs->m_ulDuration = ulActualByToTsInEndTime - 
				    ulActualByToTsInStartTime - 
				    ulTimeDiff;
            
            posRABytes = m_pRAByToTsInList->RemoveAt(posRABytes);
            m_pRAByToTsAdjustedList->AddTail(pByToTs);

            //HXLOGL4(HXLOG_ADEV, "CHXAudioStream[%p]::MapFudgedTimestamps(): Map: %p\t%lu\t%lu\t%lu\t%lu\t%lu\t\t%lu", this, m_Owner, (UINT32) pByToTs->m_ulOutNumBytes, pByToTs->m_ulInTimestamp, pByToTs->m_ulInEndTime, pByToTs->m_ulTimestamp, pByToTs->m_ulOrigTimestamp, m_llLastWriteTime);
        }
        else
        {
            break;
        }
    }
#endif /* HELIX_FEATURE_AUDIO_INACCURATESAMPLING */
}

// XXX wschildbach: How to implement this with the 64-bit timestamps?

void
CHXAudioStream::UpdateStreamLastWriteTime(HXBOOL bForceUpdate /*= FALSE*/, HXBOOL bOnResume /*= FALSE*/, HXBOOL bNotRealResume /*= FALSE*/)
{
    UINT32 ulDelay = 0;
    HXBOOL bHasDelay = FALSE;
    HXBOOL bUpdateOccured = FALSE;

    if (!m_Owner)
    {
	return;
    }

    if (m_bIsRewound)
    {
	if (SUCCEEDED(m_pValues->GetPropertyULONG32("Delay", ulDelay)))
	{
	    bHasDelay = TRUE;
	}
    }

    // Determine the synchonization time (m_llLastWriteTime) and the
    // start time.
    if (m_bIsLive)
    {
	// To update stream time on a live stream, the stream must be rewound
	// or waiting for a first packet.
	// We wait for first packet since it will deliver LiveSyncStartTime needed
	// for proper synchronization of live streams.
	if ((!m_bIsFirstPacket) && (!m_bIsRewound))
	{
	    return;
	}

	UINT32 ulCurrentPlayBackTime = m_Owner->GetCurrentPlayBackTime();

        if (!m_pValues || m_pValues->GetPropertyULONG32("LiveSyncStartTime", m_ulBaseTime) != HXR_OK)
        {
            if (bForceUpdate)
            {
                m_bIsLive = FALSE;
                m_ulBaseTime = 0;
		m_bHasStartTime = FALSE;
            }
	    if (m_bIsRewound)
	    {
		m_ulStartTime = HX_MAX(ulDelay, ulCurrentPlayBackTime);
		m_llLastWriteTime = CAST_TO_INT64 ulCurrentPlayBackTime;
		m_bHasStartTime = bHasDelay;
		SetLastAdjustedTimeDiff();
		bUpdateOccured = TRUE;
	    }
        }
        else
        {
            UINT32 ulStartDelayFromNow = 0;
	    UINT32 ulCurrentPlayBackTime = m_Owner->GetCurrentPlayBackTime();

	    if (bHasDelay)
	    {
		// Delay is provided.  Assume strong synchronzation and thus
		// that steart of live stream is mapped to playback time of zero.
		if (!m_bLiveJoiningTimeSet)
		{
		    m_ulLiveJoiningTime = 0;
		    m_bLiveJoiningTimeSet = (bOnResume && (!bNotRealResume));
		}

		m_ulLiveDelay = ulDelay;
	    }
	    else
	    {
		// Mapping from live time-base to playback time is not
		// provided.  Assume, the stream is not strongly synchronized.
		// This is on the fly live stream addition.
		if (!m_bLiveJoiningTimeSet)
		{
		    m_ulLiveJoiningTime = ulCurrentPlayBackTime;
		    m_bLiveJoiningTimeSet = (bOnResume && (!bNotRealResume));
		}

		m_ulLiveDelay = m_ulLiveJoiningTime;

		// if we are not rewound, make it so we start playing at the
		// back of the pushdown queue so we do not clip any data.
		if (!m_bIsRewound)
		{
		    INT32 lCurrentAudioPushdown = 0;

		    if (m_Owner && m_Owner->m_Owner)
		    {
			lCurrentAudioPushdown = m_Owner->m_Owner->GetCurrentRawAudioDevicePushdown();
		    }

		    m_ulLiveDelay += lCurrentAudioPushdown;
		}
	    }
	    
	    // Adjust the base time based on current and joining times
	    HX_ASSERT(ulCurrentPlayBackTime >= m_ulLiveJoiningTime);
	    if (ulCurrentPlayBackTime > m_ulLiveJoiningTime)
	    {
		UINT32 ulTimeSinceJoining = ulCurrentPlayBackTime - m_ulLiveJoiningTime;

		m_ulBaseTime += ulTimeSinceJoining;
		m_ulLiveDelay += ulTimeSinceJoining;
	    }

	    // Determine how much we need to delay the audio stream
	    ulStartDelayFromNow = m_ulLiveDelay - ulCurrentPlayBackTime;

	    // If we are not rewound, we obtained the synhronization information
	    // late.  Reduce the delay by the current audio-pushdown as we are
	    // writing starting data to the back of the pushdown queue at this point
	    // rather than to the front of the pushdown queue.
	    if (!m_bIsRewound)
	    {
		INT32 lCurrentAudioPushdown = 0;

		if (m_Owner && m_Owner->m_Owner)
		{
		    lCurrentAudioPushdown = m_Owner->m_Owner->GetCurrentRawAudioDevicePushdown();
		}

		ulStartDelayFromNow -= lCurrentAudioPushdown;
	    }

            m_ulLastInputStartTime = m_ulBaseTime - ulStartDelayFromNow;
            m_llLastWriteTime = CAST_TO_INT64 m_ulLastInputStartTime;
	    m_ulTSRollOver = 0;
	    if ((((LONG32) ulStartDelayFromNow) < 0) &&
		(m_ulLastInputStartTime < m_ulBaseTime))
	    {
		m_llLastWriteTime += CAST_TO_INT64 MAX_UINT32;
	    }
	    m_ulStartTime = m_ulBaseTime;
	    m_bHasStartTime = TRUE;
            SetLastAdjustedTimeDiff();
	    bUpdateOccured = TRUE;
        }
    }
    else if (m_bIsRewound)
    {
	UINT32 ulCurrentPlayBackTime =  m_Owner->GetCurrentPlayBackTime();
	m_llLastWriteTime = ulCurrentPlayBackTime;
	m_ulStartTime = HX_MAX(ulDelay, ulCurrentPlayBackTime);
	m_bHasStartTime = TRUE;	// If the stream is not live, we always know the start-time
	SetLastAdjustedTimeDiff();
	bUpdateOccured = TRUE;
    }

    // See if the rewound can be undone based on the starting time of the
    // data and current playback position.
    if (bOnResume && m_bIsRewound)
    {
	INT32 lCurrentAudioPushdown = 0;
	UINT32 ulLastWriteTime = INT64_TO_UINT32(m_llLastWriteTime);

	if (m_Owner && m_Owner->m_Owner)
	{
	     lCurrentAudioPushdown = m_Owner->m_Owner->GetCurrentRawAudioDevicePushdown();
	}

	if (m_pDataList->IsEmpty())
	{
	    HXBOOL bMustStayRewound = FALSE;

	    // No data in the stream - no need to keep the rewound state which
	    // may cause the entire audio session to rewind.
	    if (m_bHasStartTime && (!m_bBeyondStartTime))
	    {
		if (((INT32) (m_ulStartTime - ulLastWriteTime)) < lCurrentAudioPushdown)
		{
		    bMustStayRewound = TRUE;
		}
	    }

	    if (!bMustStayRewound)
	    {
		if ((CAST_TO_INT64 (-lCurrentAudioPushdown)) <= m_llLastWriteTime)
		{
		    m_llLastWriteTime = m_llLastWriteTime + lCurrentAudioPushdown;
		}
		else
		{
		    m_llLastWriteTime = 0;
		}
		m_bIsRewound = FALSE;
	    }
	}
	else
	{
	    HXAudioInfo* pAudioInfo = (HXAudioInfo*) m_pDataList->GetHead();
	    HX_ASSERT(pAudioInfo);
	    if (pAudioInfo)
	    {
		UINT32 ulStartTimeForThisCase = pAudioInfo->ulStartTime;

		if (m_bHasStartTime && (!m_bBeyondStartTime))
		{
		    // If the start time for this stream is after the starting
		    // data packet, use it as the starting criteria instead of
		    // the starting data packet time.
		    // The data packets before the start time will be purged
		    // when audio stream starts playing.
		    if (((INT32) (m_ulStartTime - ulStartTimeForThisCase)) > 0)
		    {
			ulStartTimeForThisCase = m_ulStartTime;
		    }
		}

		if ((ulLastWriteTime + lCurrentAudioPushdown - ulStartTimeForThisCase) <= 0)
		{
		    // We have audio data but it is after the audio pushdown interval
		    // or the start time for this stream is after the audio pushdown
		    // interval.
		    // Thus, there is no need to keep the rewound state which may
		    // cause the entire audio session to rewind.
		    if ((CAST_TO_INT64 (-lCurrentAudioPushdown)) <= m_llLastWriteTime)
		    {
			m_llLastWriteTime = m_llLastWriteTime + (CAST_TO_INT64 lCurrentAudioPushdown);
		    }
		    else
		    {
			m_llLastWriteTime = 0;
		    }
		    m_bIsRewound = FALSE;
		}
	    }
	}
    }

    if (m_pMixEngine && bUpdateOccured)
    {
        m_pMixEngine->ResetTimeLineInMillis(m_llLastWriteTime) ;
    }

    HXLOGL3(HXLOG_ADEV,
	"CHXAudioStream[%p]::UpdateStreamLastWriteTime(bForceUpdate=%c bOnresume=%c): m_llLastWriteTime: %lu m_bIsRewound: %c", 
            this,
            bForceUpdate ? 'T' : 'F',
	    bOnResume ? 'T' : 'F',
            INT64_TO_UINT32(m_llLastWriteTime),
	    m_bIsRewound ? 'T' : 'F'
            );
}

void CHXAudioStream::SaveLastNMilliSeconds( HXBOOL bSave,
                                            UINT32 ulNMilliSeconds)
{
    m_bLastNMilliSecsToBeSaved = bSave;
    m_ulLastNMilliSeconds      = ulNMilliSeconds; 

    HX_ASSERT(!m_bLastNMilliSecsToBeSaved || m_ulLastNMilliSeconds > 0);

    // ensure we need to save for at least 1 sec
    if( m_bLastNMilliSecsToBeSaved )
    {
        m_ulLastNMilliSeconds = HX_MIN(1000, m_ulLastNMilliSeconds);
    }
    else
    {
        while (m_pLastNMilliSecsList && m_pLastNMilliSecsList->GetCount() > 0)
        {
            HXAudioInfo* pInfo = (HXAudioInfo*) m_pLastNMilliSecsList->RemoveHead();
            FreeInfo(pInfo);
        }

        HX_DELETE(m_pLastNMilliSecsList);
    }
}

void CHXAudioStream::RewindStream(INT32 lTimeToRewind)
{
    HXLOGL3(HXLOG_ADEV,
            "CHXAudioStream[%p]::RewindStream() START m_bIsRewound: %d",
            this,
            m_bIsRewound
            );

    if (m_bIsRewound)
    {
        return;
    }

    if (m_eState == E_PLAYING)
    {
	// This a rewind on the fly - reset now insted of deferring
	// the reset to Resume where it is normally done.
	m_llLastWriteTime -= lTimeToRewind;
	SetLastAdjustedTimeDiff();
	if (m_pMixEngine)
	{
	    m_pMixEngine->ResetTimeLineInMillis(m_llLastWriteTime) ;
	}
    }

    // It is nice to have m_bLastNMilliSecsToBeSaved since this allows us to
    // fill ine the data for the time that is rewound.  However, it is not
    // necessary tp have audio data to rewind.  Data that is absent will
    // be treated as silence.

    if (m_bLastNMilliSecsToBeSaved && m_pLastNMilliSecsList)
    {
        HXAudioInfo* pInfo = NULL;

        // reset any pInfo's in data list that may have been partially used.
        CHXSimpleList::Iterator ndx = m_pDataList->Begin();
        for (; ndx != m_pDataList->End(); ++ndx)
        {
            pInfo = (HXAudioInfo*) (*ndx);
            if (pInfo->ulBytesLeft  != pInfo->pBuffer->GetSize())
            {
                pInfo->pOffset      = pInfo->pBuffer->GetBuffer();
                pInfo->ulBytesLeft  = pInfo->pBuffer->GetSize();
            }
            else
            {
                break;
            }
        }

        UINT32 ulLastWriteTime = INT64_TO_UINT32(m_llLastWriteTime);
        HXBOOL bTimedToBeSet = (m_pLastNMilliSecsList->GetCount() > 0);

        HXLOGL3(HXLOG_ADEV,
                "CHXAudioStream[%p]::RewindStream() END TimeToRewind=%ld LastWriteTime=%lu TimedToBeSet=%c", 
                this, 
                lTimeToRewind,
                ulLastWriteTime,
                bTimedToBeSet ? 'T' : 'F');

        while (m_pLastNMilliSecsList->GetCount() > 0)
        {
            pInfo = (HXAudioInfo*) m_pLastNMilliSecsList->RemoveTail();
            m_pDataList->AddHead(pInfo);

            if (pInfo->ulStartTime <= m_llLastWriteTime)
            {
                break;
            }
        }

        if (bTimedToBeSet)
        {
            pInfo = (HXAudioInfo*) m_pDataList->GetHead();
            pInfo->uAudioStreamType = TIMED_AUDIO;
        }

        // remove remaining elements from the list
        while (m_pLastNMilliSecsList->GetCount() > 0)
        {
            pInfo = (HXAudioInfo*) m_pLastNMilliSecsList->RemoveHead();
            // delete the stored one
            HX_RELEASE(pInfo->pBuffer);
            HX_DELETE(pInfo);
        }

        //HXLOGL4(HXLOG_ADEV, "CHXAudioStream[%p]::RewindStream(); %p %lu %lu", this, m_Owner, ulFirstAudioTime, m_llLastWriteTime);

#if defined(HELIX_FEATURE_AUDIO_INACCURATESAMPLING)
        // put back stuff from adjusted list to in list
        while( m_pRAByToTsAdjustedList && m_pRAByToTsAdjustedList->GetCount() > 0 )
        {
            RealAudioBytesToTimeStamp* pByToTs = 
                (RealAudioBytesToTimeStamp*) m_pRAByToTsAdjustedList->RemoveTail();
            // restore original fudge timestamp
            pByToTs->m_ulTimestamp = pByToTs->m_ulOrigTimestamp;
            m_pRAByToTsInList->AddHead(pByToTs);
        }
#endif /* HELIX_FEATURE_AUDIO_INACCURATESAMPLING */
    }

    m_bIsRewound = TRUE;
}

HX_RESULT CHXAudioStream::Seek(UINT32 ulSeekTime)
{
    HX_RESULT retVal = HXR_UNEXPECTED;

    HXLOGL3(HXLOG_ADEV,
            "CHXAudioStream[%p]::Seek(ulSeekTime=%lu)",
            this,
	    ulSeekTime);

    if (m_eState != E_PLAYING)
    {
	retVal = HXR_OK;
    }

    if (SUCCEEDED(retVal))
    {
	retVal = Flush();
	m_ulSeekTime = ulSeekTime;
    }

    return retVal;
}

void CHXAudioStream::_Pause(HXBOOL bPlayerPause)
{
    HXLOGL3(HXLOG_ADEV, "CHXAudioStream[%p]::Pause(): is player = %s", this, bPlayerPause ? "true" : "false");
    if (m_eState == E_PAUSED)
    {
        return;
    }

    m_eState = E_PAUSED;
    if (!bPlayerPause)
    {
	m_bStreamPause = TRUE;
    }

    return;
}

void CHXAudioStream::_Resume(HXBOOL bPlayerResume)
{
    HXLOGL3(HXLOG_ADEV,
            "CHXAudioStream[%p]::Resume(): is player = %s",
            this,
            bPlayerResume ? "true" : "false" );

    if (!m_bInited || m_eState == E_PLAYING)
    {
        return;
    }

    if (!bPlayerResume)
    {
	m_bStreamPause = FALSE;
    }

    /* Stream can be paused due to audio player pausing or due to
      m audio stream specifically being paused regardless of the 
      audio player state.  We tesume the audio stream only when both
      reason for pausing are no longer present. */
    if ((!m_bStreamPause) && (!m_Owner || m_Owner->IsResumed()))
    {
	m_eState = E_PLAYING;
    }

    // resetting mixengine time line is done in UpdateStreamLastWriteTime()
    UpdateStreamLastWriteTime(FALSE, (m_eState == E_PLAYING));
    
    return;
}

void CHXAudioStream::SetAudioDeviceReflushHint(HXBOOL bSupported)
{
    m_bAudioDeviceReflushHint = bSupported;
}

void CHXAudioStream::FreeInfo( HXAudioInfo* pInfo,
                               HXBOOL bInstantaneous /* = FALSE */)
{
    if (m_bLastNMilliSecsToBeSaved && !bInstantaneous)
    {
        if (!m_pLastNMilliSecsList)
        {
            m_pLastNMilliSecsList = new CHXSimpleList;
            m_ulLastNHeadTime     = pInfo->ulStartTime;
            m_ulLastNTailTime     = pInfo->ulStartTime;
        }

        // reset members
        pInfo->pOffset     = pInfo->pBuffer->GetBuffer();
        pInfo->ulBytesLeft = pInfo->pBuffer->GetSize();

        // add it to the tail
        m_pLastNMilliSecsList->AddTail(pInfo);

        // Last m_ulLastNTailTime could have been invalidated by a rewind:
        // check it again here:
        m_ulLastNHeadTime = ((HXAudioInfo*) m_pLastNMilliSecsList->GetHead())->ulStartTime;
        m_ulLastNTailTime = pInfo->ulStartTime;

        // time to expire certain blocks?
        if (CALCULATE_ELAPSED_TICKS(m_ulLastNHeadTime, m_ulLastNTailTime) > m_ulLastNMilliSeconds)
        {
            // override pInfo. we will delete this block at the bottom
            pInfo = (HXAudioInfo*) m_pLastNMilliSecsList->RemoveHead();

            // update head time
            HXAudioInfo* pHeadInfo = (HXAudioInfo*) m_pLastNMilliSecsList->GetHead();

            // we should always have ATLEAST one nore in the list
            HX_ASSERT(pHeadInfo);
            m_ulLastNHeadTime = pHeadInfo->ulStartTime;
        }
        else
        {
            // early exit to save this block
            return;
        }
    }

    FreeBuffer(pInfo->pBuffer);
    delete pInfo;
}

void
CHXAudioStream::FreeBuffer(IHXBuffer* pBuffer)
{
    /* do we need to keep it around for reuse? */
    if (!m_pAvailableBuffers || m_pAvailableBuffers->GetCount() >= m_uCacheSize)
    {
        //HXLOGL4(HXLOG_ADEV, "CHXAudioStream[%p]::FreeBuffer(): remove [%p] from cache", this, pBuffer);
        /* 
         * now that we are full, we should grow the cache size, if we ever have
         * a cache miss
         */
        m_bCacheMayBeGrown = TRUE;
        pBuffer->Release();
        return;
    }

    /* 
     * check if we have the only reference, if so reuse it
     * else release our reference
     */
    pBuffer->AddRef();
    if (pBuffer->Release() > 1)
    {
        pBuffer->Release();
        return;
    }

#ifdef _MACINTOSH
    m_pAvailableBuffers->AddTail((void*) pBuffer);
#else
    HXBOOL bAddToTail = (HX_GET_BETTERTICKCOUNT() & 0x01) ? TRUE : FALSE;

    LISTPOSITION listRet = NULL;
    if (bAddToTail)
    {
        listRet = m_pAvailableBuffers->AddTail((void*) pBuffer);
    }
    else
    {
        listRet = m_pAvailableBuffers->AddHead((void*) pBuffer);
    }
    if( listRet == NULL )
    {
        m_wLastError = HXR_OUTOFMEMORY;
    }
#endif

    //HXLOGL4(HXLOG_ADEV, "CHXAudioStream[%p]::FreeBuffer(): added [%p] to cache; avail = %ld", this, pBuffer, m_pAvailableBuffers->GetCount());
    return;
}

/************************************************************************
 *  Method:
 *      IHXCommonClassFactory::CreateInstance
 */
STDMETHODIMP CHXAudioStream::CreateInstance( REFCLSID rclsid,
                                             void**   ppUnknown )
{
    HX_RESULT theErr = HXR_OK;

    if (IsEqualCLSID(rclsid, CLSID_IHXBuffer))
    {
        if (!m_pAvailableBuffers)
        {
#ifdef _MACINTOSH
            m_pAvailableBuffers = new HXAudioMacQueue;
#else
            m_pAvailableBuffers = new CHXSimpleList;
#endif
        }

        if (m_pAvailableBuffers->GetCount() > 0)
        {
#ifdef _MACINTOSH
            *ppUnknown = (IUnknown*) (IHXBuffer*) m_pAvailableBuffers->RemoveHead();
            if (!*ppUnknown) goto justincase;
#else
            HXBOOL bRemoveFromHead = (HX_GET_BETTERTICKCOUNT() & 0x01) ? TRUE : FALSE;

            if (bRemoveFromHead)
            {
                *ppUnknown = (IUnknown*) (IHXBuffer*) m_pAvailableBuffers->RemoveHead();
            }
            else
            {
                *ppUnknown = (IUnknown*) (IHXBuffer*) m_pAvailableBuffers->RemoveTail();
            }
#endif
            //HXLOGL4(HXLOG_ADEV, "CHXAudioStream[%p]::CreateInstance(): cache hit [%p]; %ld avail", this, *ppUnknown, m_pAvailableBuffers->GetCount());

            goto exit;
        }
        else
        {
#ifdef _MACINTOSH
          justincase:
#endif
            if (m_bCacheMayBeGrown)
            {
                m_bCacheMayBeGrown  = FALSE;
                m_uCacheSize        += CACHE_INCREMENT_SIZE;
                HXLOGL4(HXLOG_ADEV, "CHXAudioStream[%p]::CreateInstance(): cache size increased; now %u ", this, m_uCacheSize);
            }
        }
        /* 
         * fall down to using the comonclass factory to allocate this buiffer since 
         * we do not have it in the cache
         */
    }

    //HXLOGL4(HXLOG_ADEV, "CHXAudioStream[%p]::CreateInstance(): cache miss buffered blocks: %ld", this, m_pDataList->GetCount());
    theErr = m_pCommonClassFactory->CreateInstance(rclsid, ppUnknown);
  exit:
    return theErr;
}

/************************************************************************
 *  Method:
 *      IHXCommonClassFactory::CreateInstanceAggregatable
 */
STDMETHODIMP CHXAudioStream::CreateInstanceAggregatable( REFCLSID        /*IN*/      rclsid,
                                                         REF(IUnknown*)  /*OUT*/     pUnknown,
                                                         IUnknown*       /*IN*/      pUnkOuter
                                                         )
{
    return m_pCommonClassFactory->CreateInstanceAggregatable(rclsid, pUnknown, pUnkOuter);
}

// XXX wschildbach: What does this method do? How do we implement this with
// the 64-bit timestamps?

/************************************************************************
 *      Method:
 *          IHXUpdateProperties::UpdatePacketTimeOffset
 *      Purpose:
 *          Call this method to update the timestamp offset of cached packets
 */
STDMETHODIMP CHXAudioStream::UpdatePacketTimeOffset(INT32 lTimeOffset)
{
    HX_RESULT    rc = HXR_OK;
    HXAudioInfo* pInfo = NULL;

    // adjust the start time 
    CHXSimpleList::Iterator ndx = m_pDataList->Begin();
    for (; ndx != m_pDataList->End(); ++ndx)
    {
        pInfo = (HXAudioInfo*) (*ndx);
        pInfo->ulStartTime += lTimeOffset;
    }
  
    if (m_pLastNMilliSecsList)
    {
        ndx = m_pLastNMilliSecsList->Begin();
        for (; ndx != m_pLastNMilliSecsList->End(); ++ndx)
        {
            pInfo = (HXAudioInfo*) (*ndx);
            pInfo->ulStartTime += lTimeOffset;
        }
    }

    // Adjust more state:
    m_ulLastInputStartTime  += lTimeOffset;
    m_ulLastInputEndTime    += lTimeOffset;  
    m_llLastWriteTime       += lTimeOffset;
    m_ulLastNHeadTime       += lTimeOffset;  
    m_ulLastNTailTime       += lTimeOffset;  
    m_llLastStartTimePlayed += lTimeOffset;

    HXLOGL3(HXLOG_ADEV, "CHXAudioStream[%p]::UpdatePacketTimeOffset(): TimeOffset=%ld LastWriteTime=%lu", 
            this, 
            lTimeOffset, 
            INT64_TO_UINT32(m_llLastWriteTime)
            );

    return rc;
}

/************************************************************************
 *      Method:
 *          IHXUpdateProperties::UpdatePlayTimes
 *      Purpose:
 *          Call this method to update the playtime attributes
 */
STDMETHODIMP
CHXAudioStream::UpdatePlayTimes(IHXValues* pProps)
{
    return HXR_OK;
}

// XXX wschildbach: How do we implement this method with 64-bit timestamps?
void
CHXAudioStream::RollBackTimestamp()
{
    if (m_llLastWriteTime > CAST_TO_INT64 m_ulGranularity)
    {
        m_llLastWriteTime -= CAST_TO_INT64 m_ulGranularity;
    }
}

HXBOOL CHXAudioStream::IsRealAudioStream()
{
    return m_bRealAudioStream;
}

//This routine *DOES NOT* return the notion that the stream
//has been muted, instead, it returns if the stream is audible,
//that is, has a non-zero volume.
HXBOOL CHXAudioStream::IsAudible()
{
    return (m_bMute || (0==m_uVolume));
}

//Sets the difference between the player's last adjusted time
//and the session's unadjusted audio time.
void CHXAudioStream::SetLastAdjustedTimeDiff()
{
    HX_ASSERT(m_Owner );
    if( m_Owner )
    {
        m_ulLastAdjustedTimeDiff = m_Owner->GetLastAdjustedTimeDiff();
    }
}


#ifdef _MACINTOSH

/////////////////////////////////////////////////////////////////////////
//      
//      HXAudioMacQueue
//      
//      For passing data between an interrupt and anything else (mac only).
//
HXAudioMacQueue::HXAudioMacQueue()
{
    mQueueHeader.qFlags=0;  
    mQueueHeader.qHead=0;
    mQueueHeader.qTail=0;
    mDestructing = FALSE; // just a safety check
    m_nCount = 0;
}

/////////////////////////////////////////////////////////////////////////
//
HX_RESULT HXAudioMacQueue::AddTail(void* pObject)
{
    if (pObject && !mDestructing)
    {
        HXAudioMacQueueElement * theElement = new HXAudioMacQueueElement();  
            
        if (theElement)
        {  
            theElement->mNextElementInQueue = NULL;
            theElement->mObject = pObject;    
            ::Enqueue((QElem *)theElement, &mQueueHeader);
                    
            m_nCount++;
                    
            //
            // If someone interrupts and enters the destructor while we're in here,
            // then the pObject and the new node will be leaked.  This shouldn't 
            // happen since we should have shut down all interrupts that would
            // be adding items to the queue long before we start destructing it.
            //
                    
            HX_ASSERT(!mDestructing); // if we DID enter the destructor, let the programmer know...
        }
                    
        return HXR_OK;
    }
        
    return HXR_FAILED;
}

/////////////////////////////////////////////////////////////////////////
//
void* HXAudioMacQueue::RemoveHead()
{
    //
    // POINT A
    // 
    // You can look at the qHead anytime you want, but you can't USE a
    // pointer unless it's OFF of the queue.  Basically you do a
    // Dequeue, and if it succeeds then you know nobody else has it.
    // If it fails, an error is returned and you don't mess with it.
    //
    
    if (mQueueHeader.qHead)
    {
        HXAudioMacQueueElement * theElement = (HXAudioMacQueueElement *) mQueueHeader.qHead;

        if (theElement)
        {                       
            OSErr e = ::Dequeue( (QElemPtr) theElement, &mQueueHeader );
                        
            //
            // Between points A and D, we can't be
            // guaranteed that the queue header and
            // theElement are valid.  But Dequeue will
            // TELL us if that pointer is still valid by
            // its return code.  If it can't remove the
            // item from the queue, then somebody else did
            // and the pointer is no longer ours.  If no
            // error was returned from dequeue, then it's
            // ours to mess with.
            //
                                                
            if (e == noErr)
            {
                // at this point we know that we can
                // do whatever we need to with the
                // object.
                void* theObj = theElement->mObject;
                delete theElement; // delete the node
                m_nCount--;
                HX_ASSERT(m_nCount >= 0);
                return theObj;
            }
        }
    }
   
    return NULL;
}

/////////////////////////////////////////////////////////////////////////
//
UINT32 HXAudioMacQueue::GetCount()
{
    return m_nCount;
}

/////////////////////////////////////////////////////////////////////////
//
HXAudioMacQueue::~HXAudioMacQueue()
{
    mDestructing = TRUE; // don't add anything else to the queue
        
    void * theObject;

    while ((theObject = RemoveHead()) != 0)
    {
    }
    
    // and just to be safe...
    mQueueHeader.qHead=0;
    mQueueHeader.qTail=0;
}

#endif


