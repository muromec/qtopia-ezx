/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxaudstr.cpp,v 1.22 2007/07/06 21:57:40 jfinnecy Exp $
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
#include "hxausvc.h"
#include "hxrasyn.h"
#include "hxprefs.h"
#include "hxerror.h"

#include "errdbg.h"
#include "chxpckts.h"
#include "hxaudply.h"
#include "hxaudstr.h"
#include "hxaudses.h"
#include "hxmixer.h"
#include "hxaudvol.h"   

#include "hxslist.h"
#include "hxmap.h"
#include "auderrs.h"

#include "hxtick.h"

//#include "rmarsmp.h"
#ifdef _WINDOWS
#include "hxrsmp2.h"
#endif /* _WINDOWS */

//#include "resample.h"   
//#include "interp.h"   
#include "crosfade.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE             
static const char HX_THIS_FILE[] = __FILE__;
#endif

#define CACHE_INCREMENT_SIZE    2

//#define _TESTING    1
#ifdef _TESTING
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#if defined (_WINDOWS) || defined (_WIN32)

#include <io.h>

#endif

int g_log = -1;
#endif

/************************************************************************
 *  Method:
 *              IHXAudioStream::CHXAudioStream()
 *      Purpose:
 *              Constructor. 
 */
CHXAudioStream::CHXAudioStream(CHXAudioPlayer* owner, IUnknown* pContext)
:       m_lRefCount(0)
,       m_wLastError(HXR_OK)
,       m_pResampler(NULL)
,       m_pValues(0)
,       m_bDisableWrite(FALSE)
,       m_ulGranularity(0)
,       m_ulInputBytesPerGran(0)
,       m_ulOutputBytesPerGran(0)
,       m_ulExcessInterpBufferSize(0)
,       m_ulPreviousExcessInterpBufferSize(0)
,       m_pDataList(0)
,       m_pInstantaneousList(0)
,       m_pRAByToTsInList(0)
,       m_pRAByToTsAdjustedList(0)
,       m_bFirstWrite(TRUE)
,       m_bHooksInitialized(FALSE)
,       m_bInited(FALSE)
,       m_bSetupDone(FALSE)
,       m_bAudioFormatKnown(FALSE)
,       m_ulMaxBlockSize(0)
,       m_pResampleBuf(0)
,       m_pTmpResBuf(0)
,       m_pCrossFader(0)
,       m_pCrossFadeBuffer(0)
,       m_pExcessInterpBuffer(NULL)
,       m_pTempInterpBuffer(NULL)
,       m_fVolume((float)1.0)
,       m_bChannelConvert(FALSE)
,       m_pStreamVolume(0)
,       m_uVolume(HX_MAX_VOLUME)
,       m_bMute(FALSE)
,       m_bGotHooks(FALSE)
,       m_llLastWriteTime(0)
,       m_ulFudge(5)
,       m_pInDataPtr(0)
,       m_pOutDataPtr(0)
,       m_pVolumeAdviseSink(0)
,       m_bTobeTimed(TRUE)
,       m_bIsFirstPacket(TRUE)
,       m_bIsLive(FALSE)
,       m_ulBaseTime(0)
,       m_ulLiveDelay(0)
,       m_bSetupToBeDone(FALSE)
,       m_bCrossFadingToBeDone(FALSE)
,       m_pCrossFadeStream(NULL)
,       m_llCrossFadeStartTime(0)
,       m_ulCrossFadeDuration(0)
,       m_bFadeToThisStream(FALSE)
,       m_bFadeAlreadyDone(FALSE)
,       m_bRealAudioStream(FALSE)
,       m_ulLastInputStartTime(0)
,       m_ulLastInputEndTime(0)
,       m_llLastStartTimePlayed(0)
,       m_ulTSRollOver(0)
,       m_bLastWriteTimeUpdated(FALSE)
,       m_pCommonClassFactory(NULL)
,       m_pAvailableBuffers(NULL)
,       m_uCacheSize(CACHE_INCREMENT_SIZE)
,       m_bCacheMayBeGrown(FALSE)
,       m_bDeterminedInitialCacheSize(FALSE)
,       m_bLastNMilliSecsToBeSaved(FALSE)
,       m_ulLastNMilliSeconds(MINIMUM_AUDIO_PUSHDOWN)
,       m_pLastNMilliSecsList(NULL)
,       m_ulLastNHeadTime(0)
,       m_ulLastNTailTime(0)
,       m_eState(E_STOPPED)
,       m_bCanBeRewound(FALSE)
,       m_bAudioDeviceReflushHint(FALSE)
,       m_bIsResumed(FALSE)
,       m_bPlayerPause(FALSE)
,       m_pPreferences(NULL)
,       m_bMayNeedToRollbackTimestamp(FALSE)
{
/*
 *  Left around for future debugging
    static int fnum = 0;

    char fname[80];
 
    sprintf(fname, "c:\\temp\\before.%d", fnum);
    fdbefore = fopen(fname, "ab+");

    sprintf(fname, "c:\\temp\\before.%d.txt", fnum);
    fdbeforetxt = fopen(fname, "w+");

    sprintf(fname, "c:\\temp\\after.%d", fnum);
    fdafter = fopen(fname, "ab+");

    sprintf(fname, "c:\\temp\\after.%d.txt", fnum);
    fdaftertxt = fopen(fname, "w+");

    sprintf(fname, "c:\\temp\\in.%d", fnum);
    fdin = fopen(fname, "ab+");
    
    fnum++;
*/

    m_Owner         = owner;
    if (m_Owner)
    {
        m_Owner->AddRef();
    }

    if (pContext)
    {
        HX_VERIFY(HXR_OK == pContext->QueryInterface(IID_IHXCommonClassFactory, 
                                            (void**) &m_pCommonClassFactory));
    }

#if defined(HELIX_FEATURE_PREFERENCES)
    if (pContext)
    {
        HX_VERIFY(HXR_OK == pContext->QueryInterface(IID_IHXPreferences, (void**) &m_pPreferences));
    }
#endif /* HELIX_FEATURE_PREFERENCES */


    m_pInDataPtr    = new HXAudioData;
    m_pOutDataPtr   = new HXAudioData;
};

/************************************************************************
 *  Method:
 *              IHXAudioStream::~CHXAudioStream()
 *      Purpose:
 *              Destructor. Clean up and set free.
 */
CHXAudioStream::~CHXAudioStream()
{
    ResetStream();
/*
 *  Left around for future debugging
    if (fdbefore)
    {
        fclose(fdbefore);
    }
    if (fdbeforetxt)
    {
        fclose(fdbeforetxt);
    }
    if (fdafter)
    {
        fclose(fdafter);
    }
    if (fdaftertxt)
    {
        fclose(fdaftertxt);
    }
    if (fdin)
    {
        fclose(fdin);
    }
*/
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
		{ GET_IIDHANDLE(IID_IHXAudioStream), (IHXAudioStream*) this },
		{ GET_IIDHANDLE(IID_IHXRealAudioSync), (IHXRealAudioSync*) this },
		{ GET_IIDHANDLE(IID_IHXAudioStream2), (IHXAudioStream2*) this },
		{ GET_IIDHANDLE(IID_IHXCommonClassFactory), (IHXCommonClassFactory*) this },
		{ GET_IIDHANDLE(IID_IHXUpdateProperties), (IHXUpdateProperties*) this },
	};	
    return QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
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
    const HXAudioFormat*        pAudioFormat,
          IHXValues*            pValues
)
{
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
    }

    memcpy( &m_AudioFmt, pAudioFormat, sizeof(HXAudioFormat) ); /* Flawfinder: ignore */

    // Create the audio data list 
    m_pDataList = new CHXSimpleList;
    if ( !m_pDataList )
        theErr = HXR_OUTOFMEMORY;
    if(!theErr) // check if list constructor really succeeded
    {
        if(!m_pDataList->IsPtrListValid())
            theErr = HXR_OUTOFMEMORY;
    }

    m_pInstantaneousList = new CHXSimpleList;
    if ( !m_pInstantaneousList || !m_pInstantaneousList->IsPtrListValid())
        theErr = HXR_OUTOFMEMORY;

    // Reset this so that we init the hooks
    m_bFirstWrite       = TRUE;
    m_bHooksInitialized = FALSE;

#if defined(HELIX_FEATURE_VOLUME)
    // Create a volume object.
    if ( !theErr )
    {
        m_pVolumeAdviseSink = new HXStreamVolumeAdviseSink;
        if (!m_pVolumeAdviseSink)
        {
            theErr = HXR_OUTOFMEMORY;
        }
        else
        {
            m_pVolumeAdviseSink->AddRef();
            m_pVolumeAdviseSink->m_pCHXAudioStream = this;
        }

        m_pStreamVolume = (IHXVolume*) new CHXVolume;
        if ( !m_pStreamVolume )
            theErr = HXR_OUTOFMEMORY;
        else
        {
            m_pStreamVolume->AddRef();
            m_pStreamVolume->AddAdviseSink(m_pVolumeAdviseSink);
            m_pStreamVolume->SetVolume(HX_MAX_VOLUME);
        }
    }
#endif /* HELIX_FEATURE_VOLUME */

#ifdef _TESTING
    g_log =  open("c:\\log\\stream.raw", O_WRONLY | O_CREAT
                  | O_BINARY | _O_SEQUENTIAL);
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
STDMETHODIMP CHXAudioStream::Write
( 
    HXAudioData*        pInData
)
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

    // Process any "hooks"; Add the data to the data list.
    /* If buffer is NULL, it means that the user just 
     * wants to know what timestamp should be placed in the next 
     * STREAMED/TIMED audio data
     */
    if ( !m_bGotHooks || !pInData->pData)
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
          IHXAudioHook*    pHook,
    const HXBOOL              bDisableWrite
)
{
#if defined(HELIX_FEATURE_AUDIO_PREMIXHOOK)
    void* pTmp = 0;
    
    /* Does one already exists */
    if (m_PreMixHookMap.Lookup((void*)pHook, pTmp))
    {
        return HXR_INVALID_PARAMETER;
    }

    HXAudioHookInfo* pPreMixHookInfo = (HXAudioHookInfo*) new HXAudioHookInfo;
    if(!pPreMixHookInfo)
    {
        return HXR_OUTOFMEMORY;
    }
    pPreMixHookInfo->pHook   	    = pHook;
    pPreMixHookInfo->bDisableWrite  = bDisableWrite;
    pPreMixHookInfo->bIgnoreAudioData = FALSE;

    IHXValues* pValues = NULL;
    if (pHook && pHook->QueryInterface(IID_IHXValues, (void**) &pValues) == HXR_OK)
    {
        UINT32 ulValue = 0;
        pValues->GetPropertyULONG32("IgnoreAudioData", ulValue);
        pPreMixHookInfo->bIgnoreAudioData = (ulValue == 1);
        HX_RELEASE(pValues);
    }

    pHook->AddRef();            // Released in destructor

    m_PreMixHookMap.SetAt(pHook, pPreMixHookInfo);

    m_bGotHooks = TRUE;

    /* If any one of them is Disabled, we do not write */
    if (bDisableWrite)
    {
        m_bDisableWrite = TRUE;
    }

    ProcessAudioHook(ACTION_ADD, pHook);

    /* If we are already initialized, send the audio format */
    if (m_bHooksInitialized)
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
#if defined(HELIX_FEATURE_AUDIO_PREMIXHOOK)
    HXAudioHookInfo* pPreMixHookInfo = 0;
    HXBOOL bCheckForDisableWrite        = FALSE;

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
    if (!pNotification)
    {
        return HXR_INVALID_PARAMETER;
    }

    void* pTmp = 0;
    
    /* Does one already exists */
    if (m_DryNotificationMap.Lookup((void*)pNotification, pTmp))
    {
        return HXR_INVALID_PARAMETER;
    }

    pNotification->AddRef();

    m_DryNotificationMap.SetAt((void*)pNotification, (void*)pNotification);

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
    HX_RESULT   hr = HXR_OK;

    void* pTmp = 0;

    if (!pNotification)
    {
        hr = HXR_INVALID_PARAMETER;
        goto cleanup;
    }

    // remove only if it is exists
    if (m_DryNotificationMap.Lookup((void*)pNotification, pTmp))
    {
        m_DryNotificationMap.RemoveKey((void*)pNotification);
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

    pAudioFormat->uChannels         = m_AudioFmt.uChannels;
    pAudioFormat->uBitsPerSample    = m_AudioFmt.uBitsPerSample;
    pAudioFormat->ulSamplesPerSec   = m_AudioFmt.ulSamplesPerSec;
    pAudioFormat->uMaxBlockSize     = m_AudioFmt.uMaxBlockSize;

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
    if ( m_pStreamVolume )
    {
        m_pStreamVolume->AddRef();
        return m_pStreamVolume;
    }
    else
    {
        return 0;
    }
}

#if defined(HELIX_FEATURE_VOLUME)

/*
 *      IHXVolumeAdviseSink methods
 */
STDMETHODIMP CHXAudioStream::OnVolumeChange
(
    const UINT16 uVolume
)
{
    m_uVolume = uVolume;
    return HXR_OK;
}

STDMETHODIMP CHXAudioStream::OnMuteChange
(
    const HXBOOL bMute
)
{
    m_bMute = bMute;
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
HX_RESULT CHXAudioStream::AddData
(
    HXAudioData* pAudioData
)
{
    HX_RESULT       theErr = HXR_OK;
    HXBOOL            bInTSRollOver = FALSE;
    HXAudioInfo*    pAinfo = 0;

    /* If buffer is NULL, it means that the user just 
     * wants to know what timestamp should be placed in the next 
     * STREAMED/TIMED audio data
     */
    if (!pAudioData->pData)
    {
        HXAudioInfo* pInfo = NULL;
        
        if (!m_pDataList->IsEmpty() && 
            NULL != (pInfo = (HXAudioInfo*) m_pDataList->GetTail()))
        {
            pAudioData->ulAudioTime = pInfo->ulStartTime + 
                                      CalcMs(pInfo->pBuffer->GetSize());
        }
        else
        {
            pAudioData->ulAudioTime = INT64_TO_UINT32(m_llLastWriteTime - CAST_TO_INT64 m_ulTSRollOver * CAST_TO_INT64 MAX_UINT32);
        }

        return HXR_OK;
    }

    // make sure the renderer does not pass NULL data!!
    HX_ASSERT(pAudioData->pData->GetBuffer() != NULL &&
              pAudioData->pData->GetSize() != 0);
    if (pAudioData->pData->GetBuffer()  == NULL ||
        pAudioData->pData->GetSize()    == 0)
    {
        return HXR_INVALID_PARAMETER;
    }

    if (m_bIsFirstPacket)
    {
        m_bIsFirstPacket = FALSE;
        
        IHXErrorMessages* pErrMsg = NULL;
        if (HXR_OK == m_Owner->m_pContext->QueryInterface(IID_IHXErrorMessages, (void**)&pErrMsg))
        {
            DEBUG_OUT(pErrMsg, DOL_GENERIC, (s,"AudioFormatIn: %lu channels %lu SamplesPerSec", m_AudioFmt.uChannels, m_AudioFmt.ulSamplesPerSec));
            DEBUG_OUT(pErrMsg, DOL_GENERIC, (s,"AudioFormatOut: %lu channels %lu SamplesPerSec", m_DeviceFmt.uChannels, m_DeviceFmt.ulSamplesPerSec));
        }
        HX_RELEASE(pErrMsg);

        if (m_bIsLive)
        {
            m_Owner->UpdateStreamLastWriteTime();
            UpdateStreamLastWriteTime(TRUE);
        }
    }
//{FILE* f1 = ::fopen("c:\\temp\\audio.txt", "a+"); ::fprintf(f1, "%lu\tAddData\t%p\t%lu\n", HX_GET_BETTERTICKCOUNT(), this, pAudioData->ulAudioTime);::fclose(f1);}
//    ::fwrite(pAudioData->pData->GetBuffer(), pAudioData->pData->GetSize(), 1, fdin);

    UINT32 ulDataTime = CalcMs(pAudioData->pData->GetSize());
    UINT32 ulEndTime = pAudioData->ulAudioTime + ulDataTime;

    if (m_pAvailableBuffers && !m_bDeterminedInitialCacheSize && ulDataTime > 0)
    {
        m_bDeterminedInitialCacheSize = TRUE;
        m_uCacheSize = (UINT16) (m_ulGranularity*2/ulDataTime) + 1;
        /* make sure it is atleast CACHE_INCREMENT_SIZE to begin with */
        m_uCacheSize = m_uCacheSize < CACHE_INCREMENT_SIZE ? 
                            CACHE_INCREMENT_SIZE : m_uCacheSize;
    }

    if (m_ulLastInputStartTime > pAudioData->ulAudioTime &&
        ((m_ulLastInputStartTime - pAudioData->ulAudioTime) > MAX_TIMESTAMP_GAP))
    {
        bInTSRollOver = TRUE;
        m_ulTSRollOver++;
    }

    m_ulLastInputStartTime  = pAudioData->ulAudioTime;
    m_ulLastInputEndTime    = ulEndTime;

    /* even in STREAMING_AUDIO case, it might happen, that the packets
     * written are late. e.g. packets received late on the network 
     */
    INT64 llActualTimestamp = CAST_TO_INT64 (pAudioData->ulAudioTime) + CAST_TO_INT64 m_ulTSRollOver * CAST_TO_INT64 MAX_UINT32;
    INT64 llActualEndTime = CAST_TO_INT64 (pAudioData->ulAudioTime) + CAST_TO_INT64 (ulDataTime) +
                            CAST_TO_INT64 m_ulTSRollOver * CAST_TO_INT64 MAX_UINT32;

    if ((pAudioData->uAudioStreamType == STREAMING_AUDIO ||
         pAudioData->uAudioStreamType == TIMED_AUDIO) &&
       !(llActualTimestamp >= m_llLastWriteTime ||
         llActualEndTime > m_llLastWriteTime)) 
    {
        /* Too late*/
        m_bTobeTimed    = TRUE;
//{FILE* f1 = ::fopen("e:\\audio.txt", "a+"); ::fprintf(f1, "%lu\t%p\t%d\t%lu\t%lu\tLATE\n", HX_GET_BETTERTICKCOUNT(), this, m_pDataList->GetCount(), pAudioData->ulAudioTime, (INT32)m_llLastWriteTime);::fclose(f1);}
        return HXR_LATE_PACKET; 
    }

    pAinfo = new HXAudioInfo;
    if(!pAinfo)
    {
        theErr = HXR_OUTOFMEMORY;
        goto exit;
    }

    pAudioData->pData->AddRef();
    pAinfo->pBuffer             = pAudioData->pData;
    pAinfo->ulStartTime         = pAudioData->ulAudioTime;
    pAinfo->pOffset             = pAudioData->pData->GetBuffer();  
    pAinfo->ulBytesLeft         = pAudioData->pData->GetSize();  
    pAinfo->uAudioStreamType    = pAudioData->uAudioStreamType;

    if (m_bTobeTimed && pAudioData->uAudioStreamType == STREAMING_AUDIO)
    {
        pAinfo->uAudioStreamType = TIMED_AUDIO;
        m_bTobeTimed             = FALSE;
    }
    else if (m_bTobeTimed && pAudioData->uAudioStreamType == TIMED_AUDIO)
    {
        m_bTobeTimed    = FALSE;
    }

//{FILE* f1 = ::fopen("c:\\temp\\audio.txt", "a+"); ::fprintf(f1, "AddData ulAudioTime: %lu\n", pAudioData->ulAudioTime);::fclose(f1);}

    if (pAinfo->uAudioStreamType == INSTANTANEOUS_AUDIO)
    {
	CHXSimpleList* pList = new CHXSimpleList;
        if(!pList)
        {
            theErr = HXR_OUTOFMEMORY;
            goto exit;
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
            if(!pList)
            {
                theErr = HXR_OUTOFMEMORY;
                goto exit;
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
        m_pDataList->AddTail((void*) pAinfo);
    }
    else
    {
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
            if ((llActualTimestamp <= llActualLastEndTime               &&
                 llActualLastEndTime - llActualTimestamp <= m_ulFudge)  || 
                (llActualTimestamp >= llActualLastEndTime               &&
                 llActualTimestamp - llActualLastEndTime <= m_ulFudge))
            {
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

    /* Make sure to discard any data that is prior to cross-fade time */
    if (!theErr && m_bCrossFadingToBeDone && m_bFadeToThisStream)
    {
        RemoveExcessCrossFadeData();
    }

//{FILE* f1 = ::fopen("e:\\audio.txt", "a+"); ::fprintf(f1, "%lu\t%p\t%d\t%lu\t%lu\n", HX_GET_BETTERTICKCOUNT(), this, m_pDataList->GetCount(), pAudioData->ulAudioTime, (UINT32)m_llLastWriteTime);::fclose(f1);}
    
    return theErr;
}

HX_RESULT CHXAudioStream::ProcessInfo(void)
{
    HX_RESULT theErr = HXR_OK;

    // Calculate the number of bytes per granularity.
    m_ulInputBytesPerGran = (ULONG32) 
                (((m_AudioFmt.uChannels * ((m_AudioFmt.uBitsPerSample==8)?1:2) *  m_AudioFmt.ulSamplesPerSec) 
                                / 1000.0) * m_ulGranularity);

    m_ulOutputBytesPerGran  = (ULONG32) 
                (((m_DeviceFmt.uChannels * ((m_DeviceFmt.uBitsPerSample==8)?1:2) *  m_DeviceFmt.ulSamplesPerSec) 
                                / 1000.0) * m_ulGranularity);

    /* Number of samples required at output should be a multiple of 8 if 
     * sampling rate is 8K/16K/32K...or a multiple of 11 for 11K/22K...
     * This is needed since the resamplerequires works reliably ONLY if
     * this condition is true. Ken is working on this problem. This is 
     * an interim fix 
     */
    ULONG32 ulExtraGranularity = 1;
    if (m_DeviceFmt.ulSamplesPerSec % 8 == 0)
    {
        ulExtraGranularity = 8;
    }
    else
    {
        ulExtraGranularity = 11;
    }

    // Make sure that number of bytes per granularity is an even number.
    if (m_ulInputBytesPerGran % (2*m_AudioFmt.uChannels*ulExtraGranularity) != 0)
    {
        m_ulInputBytesPerGran -= m_ulInputBytesPerGran % (2*m_AudioFmt.uChannels*ulExtraGranularity);
    }

    if (m_ulOutputBytesPerGran % (2*m_DeviceFmt.uChannels*ulExtraGranularity) != 0)
    {
        m_ulOutputBytesPerGran -= m_ulOutputBytesPerGran % (2*m_DeviceFmt.uChannels*ulExtraGranularity);
    }


    if (!theErr)
    {
        // Setup the resampler
        theErr = SetupResampler();
    }

    if (!theErr)
    {
        m_bInited = TRUE;
        
        if (m_eState == E_STOPPED)
        {
            m_eState = E_INITIALIZED;
        } 
    }

    if (!theErr && m_bInited && m_bCrossFadingToBeDone && 
        m_bFadeToThisStream)
    {
        InitializeCrossFader();
    }

    /* Get the current player time to set the last write audio time 
     * If someone creates a stream mid presentation, we ask the player 
     * object for the current write time.
     */
    // set last write time to be the current playback time since
    // this is what other system components(i.e. renderers) based on 
    // fixed b#69847 - loss of push-down-worth of data =
    // m_Owner->GetLastAudioWriteTime() - m_Owner->GetCurrentPlayBackTime()
//    m_llLastWriteTime = m_Owner->GetCurrentPlayBackTime();

    // XXXRA: It is necessary to use last audio write time for any delayed
    // audio streams to work that do not involve any Pause/Rewind logic.
    // To cover the case where a source (and an audio stream) has been added
    // mid-playback by SMIL renderer which has a delay equivalent to the 
    // current playback time, it should result in a player rewind which should 
    // reset the lastaudiowrite time accordingly...so we should be able
    // to use m_Owner->GetLastAudioWriteTime() value in such a use case as well.
    // this change is required to fix PR 79161 and PR 69780.
    // Henry, PR 69847 (the reason for the earlier change) is still busted. 
    // so I am reverting this code to the original code. you will have 
    // to come up with a different fix for PR 69847 since this was clearly not 
    // the correct fix.
    m_llLastWriteTime = m_Owner->GetLastAudioWriteTime();

    if (!theErr && m_bInited)
    {
        m_Owner->StreamInitialized(this);
    }

    return theErr;
}


/************************************************************************
 *  Method:
 *              IHXAudioStream::SetupResampler
 *      Purpose:
 */
HX_RESULT CHXAudioStream::SetupResampler()
{
    HX_RESULT theErr = HXR_OK;

#if defined(HELIX_FEATURE_RESAMPLER)
    // Create a resampler for this stream if we need one.
    // Current resampler code resamples these sampling rates:
    // 8000,11025,16000,22050,32000,44100.  The resample also
    // converts 8-bit to 16-bit. NOTE: We convert all 8-bit samples
    // to 16-bit before input into the resampler or mixer.

    /* Resampler does the following tasks:
     * 1. Conversion for sampling rates
     * 2. Conversion from 8->16 and will also do 16->8 (XXX TBD)
     * 3. Conversion from stereo to mono

     * We do not use resampler for conversion from mono to stereo. This
     * takes place in the Mixer.
     */

/*
    fprintf(fdbeforetxt, "Samples: %lu, Channels: %lu, Bits/Sample: %lu\n",
         m_AudioFmt.ulSamplesPerSec, m_AudioFmt.uChannels, m_AudioFmt.uBitsPerSample);

    fprintf(fdaftertxt, "Samples: %lu, Channels: %lu, Bits/Sample: %lu\n",
         m_DeviceFmt.ulSamplesPerSec, m_DeviceFmt.uChannels, m_DeviceFmt.uBitsPerSample);
*/

    if ((m_AudioFmt.ulSamplesPerSec != m_DeviceFmt.ulSamplesPerSec) ||
        (m_AudioFmt.uBitsPerSample != m_DeviceFmt.uBitsPerSample)   ||
        (m_AudioFmt.uChannels == 2 && m_DeviceFmt.uChannels == 1))
    {
	m_AudioFmt.uMaxBlockSize = (UINT16) (m_ulInputBytesPerGran*1.5);

        if (HXR_OK == m_Owner->GetOwner()->CreateResampler(m_AudioFmt, 
                                                           m_DeviceFmt, 
                                                           m_pResampler))
        {                               
            /* times 2 since resampler always returns data in 16 bit.
             * times 2 if i/p or o/p is stereo
             * May change in future when we do 16->8 conversion in resampler
             */
            m_ulMaxBlockSize = (ULONG32) m_DeviceFmt.uMaxBlockSize * 2 * 2;

            /* This may be TRUE in downsampling */
            if (m_ulMaxBlockSize < (ULONG32) (m_ulInputBytesPerGran*1.5))
            {
                m_ulMaxBlockSize = (ULONG32) (m_ulInputBytesPerGran*1.5);
            }
        }
        else
        {
            HX_RELEASE(m_pResampler);
            theErr = HX_RESAMPLER_ERROR;
        }

        if ( !theErr )
        {
            m_pResampleBuf = (UCHAR*) new char [ m_ulMaxBlockSize ];
            m_pTmpResBuf = (UCHAR*) new char [ m_ulMaxBlockSize ];

            if (!m_pResampleBuf || !m_pTmpResBuf)
            {
                theErr = HXR_OUTOFMEMORY;
            }
        }

    }
    else
    {
        m_ulMaxBlockSize =  m_DeviceFmt.uMaxBlockSize;
    }

    /* Do not rely on max block size specified by the user */
    if (!theErr)
    {
        m_AudioFmt.uMaxBlockSize = (UINT16) m_ulMaxBlockSize;
    }

    m_bChannelConvert = (m_AudioFmt.uChannels == 1 && m_DeviceFmt.uChannels == 2);
#endif /* HELIX_FEATURE_RESAMPLER */

    // Create the resampler output buffer. Size it to the largest needed.
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
    HXAudioFormat*      pAudioFormat
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
HX_RESULT CHXAudioStream::Setup(
        HXAudioFormat*  pFormat
,       ULONG32         ulGranularity
)
{
    HX_RESULT theErr = HXR_OK;

    m_DeviceFmt.uChannels       = pFormat->uChannels;
    m_DeviceFmt.uBitsPerSample  = pFormat->uBitsPerSample;
    m_DeviceFmt.ulSamplesPerSec = pFormat->ulSamplesPerSec;
    m_DeviceFmt.uMaxBlockSize   = pFormat->uMaxBlockSize;

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
    m_bInited           = FALSE;
    m_bCanBeRewound     = FALSE;
    m_bSetupDone        = FALSE;
    m_bAudioFormatKnown = FALSE;
    m_bIsResumed        = FALSE;
    
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
    
    // Delete resample buffer
    HX_VECTOR_DELETE(m_pResampleBuf);
    // Delete tmp resample buffer
    HX_VECTOR_DELETE(m_pTmpResBuf);

    HX_VECTOR_DELETE(m_pCrossFadeBuffer);
    HX_VECTOR_DELETE(m_pExcessInterpBuffer);
    HX_VECTOR_DELETE(m_pTempInterpBuffer);
    m_ulExcessInterpBufferSize = m_ulPreviousExcessInterpBufferSize = 0;
    
#if defined(HELIX_FEATURE_CROSSFADE)
    HX_DELETE(m_pCrossFader);
#endif /* HELIX_FEATURE_CROSSFADE */

    // Free the resampler
    HX_RELEASE(m_pResampler);

    m_bGotHooks = FALSE;

    m_llLastWriteTime = 0;
    m_ulTSRollOver = 0;

#ifdef _TESTING
    if ( g_log > 0 )
        close(g_log);
    g_log = -1;
#endif

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

#if defined(HELIX_FEATURE_VOLUME)
    // Delete IRMA volume object.
    if (m_pStreamVolume && m_pVolumeAdviseSink)
    {
        m_pStreamVolume->RemoveAdviseSink(m_pVolumeAdviseSink);
        m_pVolumeAdviseSink->m_pCHXAudioStream = 0;
    }
    HX_RELEASE(m_pVolumeAdviseSink);
#endif /* HELIX_FEATURE_VOLUME */

    HX_RELEASE(m_pStreamVolume);

    HX_DELETE(m_pInDataPtr);
    HX_DELETE(m_pOutDataPtr);

    if (m_DryNotificationMap.GetCount() > 0)
    {
        IHXDryNotification* pDryNotification = 0;
        CHXMapPtrToPtr::Iterator lIter = m_DryNotificationMap.Begin();
        for (; lIter != m_DryNotificationMap.End(); ++lIter)
        {
            pDryNotification = (IHXDryNotification*) (*lIter);
            pDryNotification->Release();
        }
        
        m_DryNotificationMap.RemoveAll();
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
CHXAudioStream::ProcessAudioHook(PROCESS_ACTION action, 
                                 IHXAudioHook* pAudioHook)
{
    return HXR_OK;
}

// Dummy version of this method - real version would be in hxaudstr_new.cpp
HXBOOL CHXAudioStream::ConvertIntoBuffer(tAudioSample* buffer, UINT32 nSamples, INT64 llStartTimeInSamples)
{
    return FALSE;
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
#if defined(HELIX_FEATURE_AUDIO_PREMIXHOOK)
    /* Iterate thru the hook list and call the hook's OnInit().
     * If any of the hooks have disabled write set to TRUE, then
     * we will let this override any set to FALSE.
     */
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
#endif /* HELIX_FEATURE_AUDIO_PREMIXHOOK */

    m_bHooksInitialized = TRUE;
}

/************************************************************************
 *  Method:
 *              IHXAudioStream::ProcessHooks
 *      Purpose:
 */
HX_RESULT CHXAudioStream::ProcessHooks
( 
    HXAudioData*        pInData,
    HXAudioData*        pOutData
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
                theErr = pPreMixHookInfo->pHook->OnBuffer( m_pInDataPtr, m_pOutDataPtr);

                /* Check to see if renderer changed the buffer. If so, then
                 * make this output as input to the next Hook.
                 */
                if (!theErr && m_pOutDataPtr->pData)
                {
                    m_pInDataPtr->pData->Release();
                    m_pInDataPtr->pData     = m_pOutDataPtr->pData;
                    m_pInDataPtr->ulAudioTime   = m_pOutDataPtr->ulAudioTime;

                    m_pOutDataPtr->pData        = 0;
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
 *  Method:
 *      CHXAudioStream::MixIntoBuffer
 *  Purpose:
 *      Mix stream data into this pPlayerBuf.
 *  Note:
 *      
 *      Resampler always works on 16 bit PCM. If the input is
 *      8 bit, it converts it first to 16 bit before making
 *      any resampling calculations. 
 *      We always try to open audio device in 16 bit stereo mode.
 *      This is because a new player/stream may be instantiated in the
 *      midst of a presentation and this new stream may be stereo. If we 
 *      earlier opened the device as mono, we will have to force this stereo
 *      stream to be played as mono! Not a good idea. Also any mono-streo
 *      conversion almost comes for free (some extra memory usage and 
 *      an extra assignment) since it can be done in the mixing loop in
 *      MixBuffer().
 *
 *      Any 8-16 and stereo-mono conversion, if required, SHOULD be done
 *      before resampling.
 *      Any mono-stereo conversion should be done after resampling.
 *
 *      Stereo-Mono conversion code resides in the resampler. 
 *      Mono-Stereo conversion code resides in the mixer. 
 *
 */
HX_RESULT CHXAudioStream::MixIntoBuffer
(
    UCHAR*   pPlayerBuf,
    ULONG32  ulBufSize,
    ULONG32& ulBufTime,
    HXBOOL&    bIsMixBufferDirty,
    HXBOOL     bGetCrossFadeData
)
{
    HXBOOL    bCrossFadeThisTime  = FALSE;
    UINT32  ulTimeActuallyFaded = m_ulGranularity;
    if (!m_bInited)
    {
        return HXR_NOT_INITIALIZED;
    }

//{FILE* f1 = ::fopen("c:\\temp\\rasync.txt", "a+"); ::fprintf(f1, "Call MixIntoBuffer: %lu\n", m_ulLastWriteTime);::fclose(f1);}
    /* If this is a *FROM* stream, we may have already mixed
     * data during cross-fade with *TO* stream
     */
    if (m_bFadeAlreadyDone && !m_bFadeToThisStream)
    {
        m_bFadeAlreadyDone = FALSE;
        return HXR_OK;
    }

    HX_ASSERT(!bGetCrossFadeData || !m_bFadeToThisStream);

    /* If we need to mix cross fade data from the *from* stream,
     * it better be available 
     */
    HX_ASSERT(!bGetCrossFadeData || m_pDataList->GetCount() > 0);

    /* If this stream needs to be cross-faded and is a
     * NOT a fade-to stream, it would have been already taken
     * care of by the fade-to stream in an earlier call to 
     * MixIntoBuffer()
     */
    if (m_bCrossFadingToBeDone && m_pDataList->GetCount() > 0)
    {
        HXAudioInfo* pInfo = (HXAudioInfo*) m_pDataList->GetHead();
        INT64 llActualStartTime  = 0;
        if (pInfo)
        {
            llActualStartTime   = CAST_TO_INT64 (pInfo->ulStartTime) + 
                                  CAST_TO_INT64 (CalcMs(pInfo->pBuffer->GetSize() - pInfo->ulBytesLeft)) +
                                  CAST_TO_INT64 m_ulTSRollOver * CAST_TO_INT64 MAX_UINT32;
            if (m_bFadeToThisStream)
            {
                /* Cool! It is time for cross-fading */
                if ((m_llLastWriteTime <= m_llCrossFadeStartTime                            &&
                     m_llCrossFadeStartTime - m_llLastWriteTime <= CAST_TO_INT64 m_ulGranularity)  ||
                    (m_llLastWriteTime > m_llCrossFadeStartTime                             &&
                     m_llLastWriteTime - m_llCrossFadeStartTime <= CAST_TO_INT64 m_ulFudge))
                {
                    bCrossFadeThisTime = TRUE;
                    if (m_llLastWriteTime <= m_llCrossFadeStartTime)
                    {
                        ulTimeActuallyFaded = m_ulGranularity - 
                                              INT64_TO_UINT32(m_llCrossFadeStartTime - m_llLastWriteTime);
                    }

//{FILE* f1 = ::fopen("c:\\raroot\\racross.txt", "a+"); ::fprintf(f1, "m_ulLastWriteTime: %lu ulStartTime: %lu m_ulCrossFadeStartTime: %lu pInfo->ulStartTime: %lu pInfo->pBuffer->GetSize(): %lu pInfo->ulBytesLeft: %lu\n", m_ulLastWriteTime, ulStartTime, m_ulCrossFadeStartTime, pInfo->ulStartTime, pInfo->pBuffer->GetSize(), pInfo->ulBytesLeft);::fclose(f1);}
                    HX_ASSERT(
                        (llActualStartTime >= m_llCrossFadeStartTime &&
                        (llActualStartTime <= m_llCrossFadeStartTime + 
                                              m_ulCrossFadeDuration)) ||
                        (llActualStartTime < m_llCrossFadeStartTime && 
                        (m_llCrossFadeStartTime - llActualStartTime <= CAST_TO_INT64 m_ulFudge)));
                }
            }
        }
    }

    UINT32  ulLastWriteTime = INT64_TO_UINT32(m_llLastWriteTime - CAST_TO_INT64 m_ulTSRollOver * CAST_TO_INT64 MAX_UINT32);

    if (!bGetCrossFadeData && ulBufTime < ulLastWriteTime)
    {
        ulBufTime = ulLastWriteTime;
    }

    /* If there are any DryNotifications and the data list is empty
     * we need to notify them so that they can write more data.
     */

    if (m_DryNotificationMap.GetCount() > 0 || m_Owner->GetAudioStreamCount() == 1)
    {
        UINT32 ulNumMsRequired = m_ulGranularity;
        if (m_pDataList->IsEmpty() || !EnoughDataAvailable(ulLastWriteTime, ulNumMsRequired))
        {
            if (!bIsMixBufferDirty && !bGetCrossFadeData && !m_Owner->m_Owner->ReallyNeedData())
            {
                return HXR_WOULD_BLOCK;
            }

            if (m_DryNotificationMap.GetCount() > 0)
            {
                IHXDryNotification* pDryNotification = 0;
                CHXMapPtrToPtr::Iterator lIter = m_DryNotificationMap.Begin();
                for (; lIter != m_DryNotificationMap.End(); ++lIter)
                {
                    pDryNotification = (IHXDryNotification*) (*lIter);
                    pDryNotification->OnDryNotification(ulLastWriteTime, ulNumMsRequired);
                }

                if (m_Owner->GetState() != E_PLAYING)
                {
                    return HXR_OK;
                }
            }
        }
    }

    m_Owner->DataInAudioDevice(TRUE);
    // /////////////////////////////////////////////////////////////
    // There may be no buffers in the list. No packets? Play silence.
    // Still need to increment time.
    if ( m_pDataList->IsEmpty() && m_pInstantaneousList->IsEmpty() )
    {
        m_llLastWriteTime += CAST_TO_INT64 m_ulGranularity;
//{FILE* f1 = ::fopen("e:\\MixIntoBuffer.txt", "a+"); ::fprintf(f1, "%lu\t%p\t%lu\n", HX_GET_BETTERTICKCOUNT(), this, (UINT32)m_llLastWriteTime);::fclose(f1);}
        return HXR_NO_DATA;
    }

    UCHAR*      pSourceBuffer = 0;
    ULONG32     ulMaxBytes = 0;
    ULONG32     ulMaxFramesIn = 0;
    ULONG32     ulMaxFramesOut = 0;
    ULONG32     ulNumBytesMixed = 0;
    HXBOOL        bMonoToStereoMayBeConverted = TRUE;
    HXBOOL        bResampleBufferDirty        = FALSE;


    // /////////////////////////////////////////////////////////////
    // If no  resampling, mix stream data directly into the player
    // buffer.
    if ( !m_pResampler)
    {
        pSourceBuffer   = pPlayerBuf;
        ulMaxBytes      = m_ulInputBytesPerGran;

        /* For only those sound cards which do not
         * support stereo - a RARE (non-existent) case 
         */
        if (m_AudioFmt.uChannels == 2 && m_DeviceFmt.uChannels == 1)
        {
            /* We should never reach here since this case
             * should be handled by the Resampler
             * Temporary ASSERT... 
             */
            HX_ASSERT(FALSE);
        }
        /* Mono->Stereo conversion*/
        else if (m_bChannelConvert)
        {
            HX_ASSERT(ulMaxBytes <= ulBufSize/2);
            
            /* Avoid GPF in retail builds! */
            if (ulMaxBytes > ulBufSize/2)
            {
                ulMaxBytes = ulBufSize/2;
            }
        }
        else
        {
            HX_ASSERT(ulMaxBytes <= ulBufSize);
            
            /* Avoid GPF in retail builds! */
            if (ulMaxBytes > ulBufSize)
            {
                ulMaxBytes = ulBufSize;
            }
        }
    }
#if defined(HELIX_FEATURE_RESAMPLER)
    // /////////////////////////////////////////////////////////////
    // If resampling, mix stream data into a tmp buffer. Then
    // resample this buffer and mix the final resampled buffer into
    // the player buffer.
    else
    {
        bMonoToStereoMayBeConverted = FALSE;

//      memset(m_pTmpResBuf, 0, HX_SAFESIZE_T(m_ulMaxBlockSize));
        pSourceBuffer = m_pTmpResBuf;

        /* 
         * Audio Session will always ask for m_ulOutputBytesPerGran bytes to be mixed
         * in MixIntoBuffer() call. So we need to produce these many number of bytes.
         * If there is any mono-stereo conversion that happens in the mixing, number
         * of output bytes required from the resampler are half the number of 
         * m_ulOutputBytesPerGran bytes.
         */

        if (m_pResampler)
        {
            ulMaxFramesOut = m_ulOutputBytesPerGran/(m_DeviceFmt.uBitsPerSample==8 ? 1 : 2);

            if (m_DeviceFmt.uChannels == 2)
            {
                ulMaxFramesOut /= 2;
            }

            ulMaxFramesIn = m_pResampler->Requires(ulMaxFramesOut);

            ulMaxBytes = ulMaxFramesIn *  ((m_AudioFmt.uBitsPerSample==8)? 1 : 2)
                                            * m_AudioFmt.uChannels;
        }
        else
        {
            ulMaxBytes = m_ulInputBytesPerGran;
        }

        HX_ASSERT(ulMaxBytes <= m_ulMaxBlockSize);
    }
#endif /* HELIX_FEATURE_RESAMPLER */

    // /////////////////////////////////////////////////////////////
    // Mix n bytes of data into buffer
    ulNumBytesMixed = MixData(pSourceBuffer, ulMaxBytes, bMonoToStereoMayBeConverted, 
        (!m_pResampler) ? bIsMixBufferDirty : bResampleBufferDirty);

/*
    if (ulNumBytesMixed > 0)
    {
        ::fwrite(pSourceBuffer, ulNumBytesMixed, 1, fdbefore);
    }
*/

#if defined(HELIX_FEATURE_RESAMPLER)
    // /////////////////////////////////////////////////////////////
    // If we need to resample , then do this and then mix data into
    // the player buffer.
    // Only resample and mix if volume is *not* zero and there 
    // are some bytes to mix.
    if (m_pResampler && ulNumBytesMixed > 0 && m_uVolume > 0 && !m_bMute)
    {
        if(ulNumBytesMixed < ulMaxBytes  &&  8==m_AudioFmt.uBitsPerSample)
        {
            //fill remainder with 128's (-1), silence:
            UCHAR* pTmp = &pSourceBuffer[ulNumBytesMixed];
            ULONG32 ulNumBytesLeftToSilence = ulMaxBytes-ulNumBytesMixed;
            do
            {
                *pTmp = 128;
                pTmp++;
            } while(--ulNumBytesLeftToSilence);
        }

        ULONG32 ulOutBytes      = 0;
        if (m_pResampler)
        {
            ulOutBytes = m_pResampler->Resample((UINT16*)pSourceBuffer,
                                                ulMaxBytes,
                                                (UINT16*)m_pResampleBuf);

/*
            FILE* fp = fopen("c:\\temp\\audio.txt", "w+");
            ::fwrite(m_pResampleBuf, ulOutBytes, 1, fp);
            fclose(fp);
*/
            /* Resampler will do stereo to mono conversion for us.*/
            HX_ASSERT(ulMaxFramesOut == (ulOutBytes / 2 / 
                      (m_AudioFmt.uChannels == 2 && m_DeviceFmt.uChannels == 1 ? 1 : m_AudioFmt.uChannels))) ;
        }

        if (m_bChannelConvert)
        {
            HX_ASSERT(ulOutBytes*2 <= ulBufSize);
            if ( ulOutBytes > ulBufSize/2 )
            {           
                ulOutBytes = ulBufSize/2;
            }
        }
        else
        {
            HX_ASSERT(ulOutBytes <= ulBufSize);
            ulOutBytes = ulBufSize;
        }

        HXBOOL bBeforeMixBufferDirty = bIsMixBufferDirty;
        UINT32 ulMixedNumBytes = 0;

#if defined(HELIX_FEATURE_MIXER)
        ulMixedNumBytes = CHXMixer::MixBuffer( m_pResampleBuf, pPlayerBuf, 
                             ulOutBytes, m_bChannelConvert,
                             m_uVolume, m_DeviceFmt.uBitsPerSample, bIsMixBufferDirty);
#else
        ::memcpy(pPlayerBuf, m_pResampleBuf, ulOutBytes); /* Flawfinder: ignore */
        bIsMixBufferDirty = TRUE;
        ulMixedNumBytes = ulOutBytes;
#endif /* HELIX_FEATURE_MIXER */

        // If we mixed only a partial buffer, make the remaining buffer silent
        if (!bBeforeMixBufferDirty && ulMixedNumBytes < ulBufSize)
        {
            ::memset(pPlayerBuf+ulMixedNumBytes, 0, ulBufSize-ulMixedNumBytes);
        }
    }

afterMixing:
#endif /* HELIX_FEATURE_RESAMPLER */

#ifdef _TESTING
    if ( g_log > 0 )
    {
        write( g_log, pPlayerBuf, ulNumBytesMixed);
    }

#endif

    /* This is for *FROM* stream */
    if (bGetCrossFadeData)
    {
        m_bFadeAlreadyDone = TRUE;
    }
    /* If we are cross-fading, we have data from this stream in pPlayerBuf
     * Now get data to be  cross-faded from *From* stream in 
     * m_pCrossFadeBuffer
     */
#if defined(HELIX_FEATURE_CROSSFADE) && defined(HELIX_FEATURE_MIXER)
    else if (bCrossFadeThisTime)
    {
        /* Allocate CrossFade Buffer */
        if (!m_pCrossFadeBuffer)
        {
            m_ulCrossFadeBufferSize = ulBufSize;
            m_pCrossFadeBuffer = new UCHAR[m_ulCrossFadeBufferSize];
        }

        memset(m_pCrossFadeBuffer, 0, HX_SAFESIZE_T(m_ulCrossFadeBufferSize));

        UINT32 ulCrossFadeLen = m_ulCrossFadeBufferSize;
        UINT32 ulTmpBufTime   = 0;
        HXBOOL bIsDrity = FALSE;
        m_pCrossFadeStream->MixIntoBuffer(m_pCrossFadeBuffer,
                                         ulCrossFadeLen, ulTmpBufTime, bIsDrity, TRUE);
        /* Now it is time to perform cross-fading between
         * pPlayerBuf and m_pCrossFadeBuffer
         */
        
        UINT32 ulStartByteToFade            = 0;
        UINT32 ulNumMsInThisBuffer          = CalcDeviceMs(ulBufSize);
        UINT32 ulNumBytesToBeCrossFaded = ulBufSize;
        UINT32 ulSampleSize = ((m_DeviceFmt.uBitsPerSample==8)? 1 : 2)
                            * m_DeviceFmt.uChannels;

        /* Make sure we have integral number of samples */
        HX_ASSERT(ulBufSize == (ulBufSize/ulSampleSize) * ulSampleSize);


        /* Only partial buffer needs to be cross-faded.
         *   -------------
         * ~~_____________|
         *
         *       -----------
         *      |___________~~
         *
         *      
         *     -----
         *    |_____|  <-- Granularity size block that is mixed.
         *
         *       <--> Only partial block needs to be faded
         */
        
        if (ulTimeActuallyFaded < ulNumMsInThisBuffer)
        {
            ulNumBytesToBeCrossFaded = (UINT32) (ulBufSize * 
                (ulTimeActuallyFaded*1./ulNumMsInThisBuffer)) ;

            UINT32 ulOutOfPhase = ulNumBytesToBeCrossFaded % ulSampleSize;
            if (ulOutOfPhase > 0)
            {
                ulNumBytesToBeCrossFaded = 
                    ulNumBytesToBeCrossFaded - ulOutOfPhase;
            }

            ulStartByteToFade = ulBufSize - ulNumBytesToBeCrossFaded;
        }

        if (ulTimeActuallyFaded > m_ulCrossFadeDuration)
        {
            ulNumBytesToBeCrossFaded = (UINT32) (ulBufSize * 
                (m_ulCrossFadeDuration*1./ulNumMsInThisBuffer)) ;

            UINT32 ulOutOfPhase = ulNumBytesToBeCrossFaded % ulSampleSize;
            if (ulOutOfPhase > 0)
            {
                ulNumBytesToBeCrossFaded = 
                    ulNumBytesToBeCrossFaded - ulOutOfPhase;
            }
        }

        UINT16 uNumSamples = (UINT16) (ulNumBytesToBeCrossFaded/
                                       ulSampleSize);

        HXBOOL bWasDirty = bIsMixBufferDirty;
        /* Mix the initial bytes that are not cross-faded*/
        if (ulStartByteToFade > 0)
        {
            CHXMixer::MixBuffer( m_pCrossFadeBuffer, pPlayerBuf, 
                        ulStartByteToFade, FALSE, 100, 8, bWasDirty);
        }

        m_pCrossFader->CrossFade((INT16*) (m_pCrossFadeBuffer+ulStartByteToFade), 
                                 (INT16*) (pPlayerBuf+ulStartByteToFade), 
                                 uNumSamples);

        /* make sure we have silence in bytes that were not touched */
        if (!bIsMixBufferDirty &&
            (ulStartByteToFade + (uNumSamples*ulSampleSize)) < ulBufSize)
        {
            ::memset(pPlayerBuf+ulStartByteToFade + (uNumSamples*ulSampleSize), 0, 
                ulBufSize - (ulStartByteToFade + (uNumSamples*ulSampleSize)));
        }

        bIsMixBufferDirty = TRUE;
    }

    if (bGetCrossFadeData || bCrossFadeThisTime)
    {
        if (bGetCrossFadeData)
        {
            HX_ASSERT(m_llLastWriteTime >= m_llCrossFadeStartTime);
            if (m_llLastWriteTime >= m_llCrossFadeStartTime)
            {
                HX_ASSERT(m_llLastWriteTime - m_llCrossFadeStartTime < MAX_TIMESTAMP_GAP);
                ulTimeActuallyFaded = INT64_TO_UINT32(m_llLastWriteTime - m_llCrossFadeStartTime);
            }
        }

        if (ulTimeActuallyFaded >= m_ulCrossFadeDuration)
        {
            m_bCrossFadingToBeDone = FALSE;
            HX_RELEASE(m_pCrossFadeStream);

            /* We should release any extra buffers if it is a
             * *from* stream
             */
            if (!m_bFadeToThisStream)
            {
                /* Do not remove any instantaenous buffers */
                FlushBuffers(FALSE);
            }
        }
        else
        {
            m_ulCrossFadeDuration   -= ulTimeActuallyFaded;
            m_llCrossFadeStartTime  += CAST_TO_INT64 ulTimeActuallyFaded;
        }
    }
    else if (m_bCrossFadingToBeDone && !m_bFadeToThisStream)
    {
        m_pCrossFadeStream->SyncStream(m_llLastWriteTime);
    }
#endif /* HELIX_FEATURE_CROSSFADE && HELIX_FEATURE_MIXER */

//{FILE* f1 = ::fopen("e:\\MixIntoBuffer.txt", "a+"); ::fprintf(f1, "%lu\t%p\t%lu\n", HX_GET_BETTERTICKCOUNT(), this, (UINT32)m_llLastWriteTime);::fclose(f1);}
    return HXR_OK;
}


/************************************************************************
 *  Method:
 *      CHXAudioStream::MixData
 *  Purpose:
 *      Mix all valid data in my auxilliary list into the buffer.
 *
 *  Thoughts:
 *      while there are buffers available
 *          if (buffertime is more than endtime) break;
 *          if (any part of buffer is >startime and < endtime)
 *              we are in business. 
 *              mix that part of the buffer, update offset, 
 *              update max of num bytes written in this round.
     * Looks like we need to keep LastWrite time and offsets for all buffers
     * that get written in one pass.
     * Consider this scenario:
     *  
     *
        ----------------- -----------------
       |________________| |_______________|  -> Skew  1
          ________ __________   
         |_______| |_________|              ->Skew    2  5
          ____________________________________
         |__________________|_|_______________| Skew in opposite direction 3
               _______
              |_______|                               4

         ____________
        |____________|      <- Buffer to be mixed currently
               
        
          Order of buffer processing will be in the order of numbers on the right.
          Since all the theree buffers have fudge within fudge limit, they need to 
          be written one after the other. This is possible only if we keep last
          write times and last written offsets in mixer buffer for each one of them.

        Is this extra processsing on every write worthed OR do we place limitations
        on the data that can be written.

     *  Hmmm... We are now going with STREAMED/INSTANTANEOUS/TIMED model since the
     *  above case is shows that users can really screw things up and it would be
     *  very difficult to handle this case. So instead, we do not support over-lapped
     *  buffers any more. i.e. If a renderer wants to have streamed and instantaneous
     *  behavior, it needs to use two audio streams.

 */
ULONG32 CHXAudioStream::MixData
(
        UCHAR*  pDestBuf
,       ULONG32 ulBufLen
,       HXBOOL    bMonoToStereoMayBeConverted
,       HXBOOL&   bIsMixBufferDirty
)
{
    HXAudioInfo*    pInfo                       = 0;
    ULONG32         ulNumBytes                  = 0;
    LISTPOSITION    lp                          = 0;
    LISTPOSITION    lastlp                      = 0;
    LISTPOSITION    lpList                      = 0;
    LISTPOSITION    lastlpList                  = 0;
    ULONG32         ulNumBytesWritten           = 0;
    HXBOOL            bLastWriteTimeToBeUpdated   = TRUE;
    ULONG32         ulDestinationOffset         = 0;

    HXBOOL            bWasMixBufferDirty          = bIsMixBufferDirty;

    // /////////////////////////////////////////////////////////////
    // Go thru the buffer list and mix in valid buffers.

    /* First all instantaneous buffers
     * All the instantaneous buffers get written at the start of 
     * the destination buffer.
     */

    lpList = lastlpList = 0;
    lpList = m_pInstantaneousList->GetHeadPosition();
    while(lpList)
    {
        lastlpList      = lpList;
        CHXSimpleList* pList = (CHXSimpleList*) m_pInstantaneousList->GetNext(lpList);

        lp = lastlp = 0;
        lp = pList->GetHeadPosition();

        ulDestinationOffset = 0;

        while( lp )
        {
            lastlp      = lp;
            pInfo       = (HXAudioInfo*) pList->GetNext(lp);

            /* just place it at the end of the last write position */
            ULONG32 ulNumMoreBytesToWrite = 0;

            if (m_bChannelConvert && bMonoToStereoMayBeConverted)
            {
                ulNumMoreBytesToWrite = (ulBufLen*2-ulDestinationOffset)/2;
            }
            else
            {
                ulNumMoreBytesToWrite = ulBufLen-ulDestinationOffset;
            }

            ulNumBytes = pInfo->ulBytesLeft > ulNumMoreBytesToWrite ? 
                         ulNumMoreBytesToWrite : pInfo->ulBytesLeft;

            // /////////////////////////////////////////////////////////////
            // Mix the data into the stream buffer, with stream volume. Don't 
            // do volume calculation if volume is 1. If volume is 0, then 
            // don't mix. Don't mix if there are no bytes to mix.
            if  (m_uVolume > 0 &&  ulNumBytes > 0)
            {
                if (m_bChannelConvert && bMonoToStereoMayBeConverted)
                {
                    HX_ASSERT(ulDestinationOffset + ulNumBytes*2 <= ulBufLen*2);
                }
                else
                {
                    HX_ASSERT(ulDestinationOffset + ulNumBytes <= ulBufLen);
                }

                /* we will actually make bIsMixBufferDirty dirty at the end of this function */
                HXBOOL bLocalDirty = bIsMixBufferDirty;
#if defined(HELIX_FEATURE_MIXER)
                CHXMixer::MixBuffer(pInfo->pOffset, pDestBuf+ulDestinationOffset, 
                                     ulNumBytes, m_bChannelConvert && bMonoToStereoMayBeConverted,
                                     m_uVolume, m_AudioFmt.uBitsPerSample, bLocalDirty);
#else
                ::memcpy(pDestBuf+ulDestinationOffset, pInfo->pOffset, ulNumBytes); /* Flawfinder: ignore */
                bLocalDirty = TRUE;
#endif /* HELIX_FEATURE_MIXER */
            }

            ULONG32 ulActualBytesWritten = ((m_bChannelConvert && bMonoToStereoMayBeConverted) ? 
                                        2*ulNumBytes : ulNumBytes);

            if (ulNumBytesWritten < ulActualBytesWritten)
            {
                ulNumBytesWritten = ulActualBytesWritten;
            }

            ulDestinationOffset += ulActualBytesWritten;

            pInfo->ulBytesLeft  -= ulNumBytes;
            pInfo->pOffset      += ulNumBytes;

            if (pInfo->ulBytesLeft == 0)
            {
                FreeInfo(pInfo, TRUE);
                pList->RemoveAt(lastlp);
            }

            /* Have we written enough for this time? */
            if (ulBufLen == ulDestinationOffset ||
                (m_bChannelConvert && bMonoToStereoMayBeConverted &&
                (ulBufLen*2 == ulDestinationOffset)))
            {
                break;
            }
        }

        if (ulNumBytesWritten > 0 && m_uVolume > 0)
        {
            bIsMixBufferDirty = TRUE;
        }

        if (pList->GetCount() == 0 && m_pInstantaneousList->GetCount() > 1)
        {
            m_pInstantaneousList->RemoveAt(lastlpList);
            HX_DELETE(pList);
        }
    }

    /* now timed buffers */
    /* We do not support over-lapped buffers any more */
    ulDestinationOffset = 0;
    lp = lastlp = 0;
    lp = m_pDataList->GetHeadPosition();
    while( lp )
    {
        INT64   llActualStartTime = 0;
        UINT32  ulActualTSRollOver = m_ulTSRollOver;

        lastlp  = lp;
        pInfo   = (HXAudioInfo*) m_pDataList->GetNext(lp);

        if (pInfo->ulStartTime > m_ulLastInputStartTime &&
            ((pInfo->ulStartTime - m_ulLastInputStartTime) > MAX_TIMESTAMP_GAP))
        {
            ulActualTSRollOver--;
        }

        llActualStartTime = CAST_TO_INT64 (pInfo->ulStartTime) + CAST_TO_INT64 ulActualTSRollOver * CAST_TO_INT64 MAX_UINT32;

        HX_ASSERT(llActualStartTime - m_llLastWriteTime < MAX_TIMESTAMP_GAP);

        /* only at the start of the buffer, we check whether
         * we are done or not for this round
         */
        if (pInfo->uAudioStreamType == STREAMING_AUDIO      &&
            pInfo->pOffset == pInfo->pBuffer->GetBuffer()   &&
            llActualStartTime > m_llLastWriteTime           &&
            llActualStartTime - m_llLastWriteTime > CAST_TO_INT64 m_ulFudge)
        {
            break;
        }

        if (pInfo->uAudioStreamType == TIMED_AUDIO && 
            (pInfo->ulBytesLeft == pInfo->pBuffer->GetSize() ||
             m_bCrossFadingToBeDone))
        {
            llActualStartTime = CAST_TO_INT64 (pInfo->ulStartTime) + 
                                CAST_TO_INT64 (CalcMs(pInfo->pBuffer->GetSize() - pInfo->ulBytesLeft)) +
                                CAST_TO_INT64 ulActualTSRollOver * CAST_TO_INT64 MAX_UINT32;

            HX_ASSERT(llActualStartTime - m_llLastWriteTime < MAX_TIMESTAMP_GAP);

            if (llActualStartTime >= m_llLastWriteTime && 
                llActualStartTime - m_llLastWriteTime >= CAST_TO_INT64 m_ulGranularity)
            {
//{FILE* f1 = ::fopen("e:\\MixData.txt", "a+"); ::fprintf(f1, "%lu\t%p\t%lu\t%lu\n", HX_GET_BETTERTICKCOUNT(), this, (UINT32)llActualStartTime, (UINT32)m_llLastWriteTime);::fclose(f1);}
                break;
            }

            /*Calculate the actual time where to write from*/
            ULONG32 ulBytesDiff = 0;
            if (llActualStartTime >= m_llLastWriteTime)
            {
                ulBytesDiff = CalcOffset(m_llLastWriteTime, llActualStartTime);
                ulBytesDiff = ulBytesDiff - (ulBytesDiff % (2*m_AudioFmt.uChannels));
                
                if (m_bChannelConvert && bMonoToStereoMayBeConverted)
                {
                    ulBytesDiff *= 2;

                    /* This check is needed to account for lost packets.
                     * If there are more than one packet missing in a row,
                     * we may get multiple packets with TIMED_AUDIO flag
                     * and they may be far apart in time to be written 
                     * in this block
                     */
                    if (ulDestinationOffset + ulBytesDiff > ulBufLen*2)
                    {
                        bLastWriteTimeToBeUpdated = FALSE;
                        m_llLastWriteTime += 
                            CalcMs((ulBufLen*2 - ulDestinationOffset)/2);
                        break;
                    }
                }
                else
                {
                    /* This check is needed to account for lost packets.
                     * If there are more than one packet missing in a row,
                     * we may get multiple packets with TIMED_AUDIO flag
                     * and they may be far apart in time to be written 
                     * in this block
                     */
                    if (ulDestinationOffset + ulBytesDiff > ulBufLen)
                    {
                        bLastWriteTimeToBeUpdated = FALSE;
                        m_llLastWriteTime += 
                            CalcMs(ulBufLen - ulDestinationOffset);
                        break;
                    }
                }

                ulDestinationOffset += ulBytesDiff;
            }
            else
            {
                ulBytesDiff = CalcOffset(llActualStartTime, m_llLastWriteTime);
                /* Make sure that it is at even byte boundary */
                ulBytesDiff     = ulBytesDiff - (ulBytesDiff % (2*m_AudioFmt.uChannels));

                if (pInfo->ulBytesLeft >= ulBytesDiff)
                {
                    pInfo->pOffset          += ulBytesDiff;
                    pInfo->ulBytesLeft  -= ulBytesDiff;
                }
                else
                {
                    pInfo->pOffset          += pInfo->ulBytesLeft;
                    pInfo->ulBytesLeft   = 0;
                }
            }
        }

        /* just place it at the end of the last write position */
        ULONG32 ulNumMoreBytesToWrite = 0;

        if (m_bChannelConvert && bMonoToStereoMayBeConverted)
        {
            ulNumMoreBytesToWrite = (ulBufLen*2-ulDestinationOffset)/2;
        }
        else
        {
            ulNumMoreBytesToWrite = ulBufLen-ulDestinationOffset;
        }

        ulNumBytes = pInfo->ulBytesLeft > ulNumMoreBytesToWrite ? 
                     ulNumMoreBytesToWrite : pInfo->ulBytesLeft;

        // /////////////////////////////////////////////////////////////
        // Mix the data into the stream buffer, with stream volume. Don't 
        // do volume calculation if volume is 1. If volume is 0, then 
        // don't mix. Don't mix if there are no bytes to mix.
        if  (m_uVolume > 0 &&  ulNumBytes > 0)
        {
            if (m_bChannelConvert && bMonoToStereoMayBeConverted)
            {
                HX_ASSERT(ulDestinationOffset + ulNumBytes*2 <= ulBufLen*2);
            }
            else
            {
                HX_ASSERT(ulDestinationOffset + ulNumBytes <= ulBufLen);
            }

            /* we will actually make bIsMixBufferDirty dirty at the end of this function */
            HXBOOL bLocalDirty = bIsMixBufferDirty;
#if defined(HELIX_FEATURE_MIXER)
            CHXMixer::MixBuffer(pInfo->pOffset, pDestBuf+ulDestinationOffset, 
                                 ulNumBytes, m_bChannelConvert && bMonoToStereoMayBeConverted,
                                 m_uVolume, m_AudioFmt.uBitsPerSample, bLocalDirty);
#else
            ::memcpy(pDestBuf+ulDestinationOffset, pInfo->pOffset, ulNumBytes); /* Flawfinder: ignore */
            bLocalDirty = TRUE;
#endif /* HELIX_FEATURE_MIXER */
        }

        ULONG32 ulActuallyBytesWritten = ulNumBytes;
        ulActuallyBytesWritten = ((m_bChannelConvert && bMonoToStereoMayBeConverted) ?
            ulActuallyBytesWritten*2 : ulActuallyBytesWritten);

        if (ulNumBytesWritten < ulActuallyBytesWritten + ulDestinationOffset)
        {
            ulNumBytesWritten = ulActuallyBytesWritten + ulDestinationOffset;
        }

        ulDestinationOffset += ulActuallyBytesWritten;

        pInfo->ulBytesLeft  -= ulNumBytes;
        pInfo->pOffset      += ulNumBytes;

        INT64 llLastWriteTime = 0;
        
        llLastWriteTime = CAST_TO_INT64 (pInfo->ulStartTime) + 
                          CAST_TO_INT64 (CalcMs(pInfo->pBuffer->GetSize() - pInfo->ulBytesLeft)) +
                          CAST_TO_INT64 ulActualTSRollOver * CAST_TO_INT64 MAX_UINT32;

        if (bLastWriteTimeToBeUpdated)
        {
            m_llLastStartTimePlayed = llLastWriteTime - CalcMs(ulNumBytes);
        }

        bLastWriteTimeToBeUpdated = FALSE;
        if (m_llLastWriteTime < llLastWriteTime)
        {
            m_llLastWriteTime = llLastWriteTime;
        }

        if (pInfo->ulBytesLeft == 0)
        {
            FreeInfo(pInfo);
            m_pDataList->RemoveAt(lastlp);
        }

        /* Have we written enough for this time? */
        if (ulBufLen == ulDestinationOffset ||
            (m_bChannelConvert && bMonoToStereoMayBeConverted &&
            (ulBufLen*2 == ulDestinationOffset)))
        {
            break;
        }
    }

    if (bLastWriteTimeToBeUpdated)
    {
        m_llLastWriteTime += m_ulGranularity;
    }
    else /* We mixed some input bytes */
    {
        if (m_bRealAudioStream)
        {
//{FILE* f1 = ::fopen("c:\\temp\\rasync.txt", "a+"); ::fprintf(f1, "Call MapFudgedTimestamps: %lu\n", m_ulLastStartTimePlayed);::fclose(f1);}
            MapFudgedTimestamps();
        }
    }

    if (ulNumBytesWritten > 0 && m_uVolume > 0)
    {
        bIsMixBufferDirty = TRUE;
    }

    if (!bWasMixBufferDirty && ulNumBytesWritten < ulBufLen && bIsMixBufferDirty)
    {
        ::memset(pDestBuf+ulNumBytesWritten, 0, ulBufLen-ulNumBytesWritten);
    }

    return ulNumBytesWritten;
}

/************************************************************************
 *  Method:
 *              CHXAudioStream::CalcMs
 *      Purpose:
 *              Calculate the duration in millisecs for this number of bytes.
 */
ULONG32 CHXAudioStream::CalcMs
(
    ULONG32     ulNumBytes
)
{
    return ( (ULONG32) (( 1000.0 
                / (m_AudioFmt.uChannels * ((m_AudioFmt.uBitsPerSample==8)?1:2) 
                *  m_AudioFmt.ulSamplesPerSec) ) 
                *  ulNumBytes) );

}

/************************************************************************
 *  Method:
 *              CHXAudioStream::CalcDeviceMs
 *      Purpose:
 *              Calculate the duration in millisecs for this number of 
 *              bytes in Device format.
 */
ULONG32 CHXAudioStream::CalcDeviceMs
(
    ULONG32     ulNumBytes
)
{
    return ( (ULONG32) (( 1000.0 
                / (m_DeviceFmt.uChannels * ((m_DeviceFmt.uBitsPerSample==8)?1:2) 
                *  m_DeviceFmt.ulSamplesPerSec) ) 
                *  ulNumBytes) );
}

/************************************************************************
 *  Method:
 *              CHXAudioStream::CalcOffset
 *      Purpose:
 *              Calculate the offset in bytes given time.
 */
UINT32 CHXAudioStream::CalcOffset
(
    INT64 llStartTime
,   INT64 llEndTime
)
{
    /* Using m_ulBytesPerMs may introduce cumulative error due 
     * to decimal cutoff 
     */
    HX_ASSERT(llEndTime - llStartTime < MAX_TIMESTAMP_GAP);

    return INT64_TO_UINT32((llEndTime - llStartTime) *  
                     (m_ulGranularity ? m_ulInputBytesPerGran*1./m_ulGranularity : 0));
}


void CHXAudioStream::FlushBuffers(HXBOOL bInstantaneousAlso)
{
    while (m_pDataList && m_pDataList->GetCount() > 0)
    {
        HXAudioInfo* pInfo = (HXAudioInfo*) m_pDataList->RemoveHead();
        FreeInfo(pInfo);
    }

    /* throw away any data in excess buffer for interpolator */
    m_ulPreviousExcessInterpBufferSize = 0;

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


HXBOOL
CHXAudioStream::EnoughDataAvailable(ULONG32& ulLastWriteTime, ULONG32& ulNumMsRequired)
{
    ULONG32 ulBytesNeeded = 0;
    HXBOOL    bAvailable = TRUE;
        // /////////////////////////////////////////////////////////////
    // If no  resampling, mix stream data directly into the player
    // buffer.
    if ( !m_pResampler)
    {
        ulBytesNeeded   = m_ulInputBytesPerGran;
    }
    // /////////////////////////////////////////////////////////////
    // If resampling, mix stream data into a tmp buffer. Then
    // resample this buffer and mix the final resampled buffer into
    // the player buffer.
    else
    {
        /* 
         * Audio Session will always ask for m_ulOutputBytesPerGran bytes to be mixed
         * in MixIntoBuffer() call. So we need to produce these many number of bytes.
         * If there is any mono-stereo conversion that happens in the mixing, number
         * of output bytes required from the resampler are half the number of 
         * m_ulOutputBytesPerGran bytes.
         */

        ULONG32 ulMaxFramesOut = m_ulOutputBytesPerGran/(m_DeviceFmt.uBitsPerSample==8 ? 1 : 2);
        
        if (m_DeviceFmt.uChannels == 2)
        {
            ulMaxFramesOut /= 2;
        }


        ULONG32 ulMaxFramesIn = m_pResampler->Requires(ulMaxFramesOut);

        ulBytesNeeded = ulMaxFramesIn *  ((m_AudioFmt.uBitsPerSample==8)? 1 : 2)
                                        * m_AudioFmt.uChannels;
    }

    ULONG32         ulBytesAvailable    = 0;
    LISTPOSITION    lp                  = m_pDataList->GetHeadPosition();

    while(lp)
    {
        HXAudioInfo* pInfo  = (HXAudioInfo*) m_pDataList->GetNext(lp);

        ulBytesAvailable += pInfo->ulBytesLeft;
        if ((pInfo->ulStartTime >= INT64_TO_UINT32(m_llLastWriteTime+m_ulGranularity)) ||
            ulBytesAvailable >= ulBytesNeeded)
        {
            return TRUE;
        }
    }

    ulNumMsRequired = CalcMs(ulBytesNeeded - ulBytesAvailable);

    UINT32  ulAdjustedLastWriteTime = INT64_TO_UINT32(m_llLastWriteTime - CAST_TO_INT64 m_ulTSRollOver * CAST_TO_INT64 MAX_UINT32);

    HX_ASSERT(ulAdjustedLastWriteTime + m_ulGranularity >= ulNumMsRequired);

    if (ulAdjustedLastWriteTime + m_ulGranularity >= ulNumMsRequired)
    {
        ulLastWriteTime = ulAdjustedLastWriteTime + m_ulGranularity - ulNumMsRequired;
    }
    else
    {
        ulLastWriteTime = 0;
    }

    return FALSE;
}


void            
CHXAudioStream::SetLive(HXBOOL bIsLive) 
{   
    if (m_bIsFirstPacket)
    {
        m_bIsLive = bIsLive;
    }
}

HX_RESULT    
CHXAudioStream::StartCrossFade(CHXAudioStream*  pFromStream, 
                               UINT32           ulCrossFadeStartTime,
                               UINT32           ulCrossFadeDuration, 
                               HXBOOL             bToStream)
{
#if defined(HELIX_FEATURE_CROSSFADE)
    if (m_bCrossFadingToBeDone)
    {
        return HXR_UNEXPECTED;
    }

    HX_RELEASE(m_pCrossFadeStream);

    m_bCrossFadingToBeDone  = TRUE;
    m_pCrossFadeStream      = pFromStream;
    m_pCrossFadeStream->AddRef();
    m_llCrossFadeStartTime  = CAST_TO_INT64 ulCrossFadeStartTime;
    m_ulCrossFadeDuration   = ulCrossFadeDuration;
    m_bFadeToThisStream     = bToStream;

    if (m_bInited && m_bFadeToThisStream)
    {
        InitializeCrossFader();
    }
    
    return HXR_OK;
#else
    return HXR_NOTIMPL;
#endif /* HELIX_FEATURE_CROSSFADE */
}

void
CHXAudioStream::InitializeCrossFader(void)
{
#if defined(HELIX_FEATURE_CROSSFADE)
    if (!m_pCrossFader)
    {
        m_pCrossFader = new CrossFader;
    }

    UINT16 uNumSamplesToFade = (UINT16)
        (m_DeviceFmt.ulSamplesPerSec * m_ulCrossFadeDuration/1000);

    /* Make cross-fade duration to land on a sample boundary */
    m_ulCrossFadeDuration = (uNumSamplesToFade * 1000)/
                                m_DeviceFmt.ulSamplesPerSec;

    HX_ASSERT(m_ulCrossFadeDuration > 0 && uNumSamplesToFade > 0);

    m_pCrossFader->Initialize(uNumSamplesToFade, m_DeviceFmt.uChannels);
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
STDMETHODIMP
CHXAudioStream::Register(void)
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
    m_Owner->RegisterRealAudioStream(this);

    if (!m_pRAByToTsInList)
    {
        m_pRAByToTsInList       = new CHXSimpleList;
        m_pRAByToTsAdjustedList = new CHXSimpleList;
    }
    return HXR_OK;
}

/************************************************************************
 *  Method:
 *      IHXRealAudioSync::UnRegister
 *  Purpose:
 */
STDMETHODIMP
CHXAudioStream::UnRegister(void)
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
    m_Owner->UnRegisterRealAudioStream(this);

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
CHXAudioStream::FudgeTimestamp(UINT32 /*IN*/ ulNumberofBytes,
                               UINT32 /*IN*/ ulTimestamp)
{
#if defined _DEBUG && defined HELIX_FEATURE_AUDIO_MULTIPLAYER_PAUSE 
    if (HXDebugOptionEnabled("zDoNotUseFudge"))
    {
        return HXR_OK;
    }
#endif

    RealAudioBytesToTimeStamp* pByToTs = 
        new RealAudioBytesToTimeStamp;

    pByToTs->m_ulTimestamp      = ulTimestamp;
    pByToTs->m_ulInTimestamp    = m_ulLastInputStartTime;
    pByToTs->m_ulInEndTime      = m_ulLastInputEndTime;

    if (m_bIsLive && m_ulBaseTime > 0)
    {
        pByToTs->m_ulTimestamp  += m_ulLiveDelay;

        if (pByToTs->m_ulTimestamp > m_ulBaseTime)
        {
            pByToTs->m_ulTimestamp -= m_ulBaseTime;
        }
        else
        {
            pByToTs->m_ulTimestamp  = 0;
        }
    }

    pByToTs->m_ulOrigTimestamp  = pByToTs->m_ulTimestamp;

    m_pRAByToTsInList->AddTail((void*) pByToTs);

//{FILE* f1 = ::fopen("d:\\temp\\audio.txt", "a+"); ::fprintf(f1, "Fudge:\t%lu\t%lu\n", ulTimestamp, m_ulLastInputStartTime);::fclose(f1);}

    return HXR_OK;
}

void
CHXAudioStream::CleanupRAByToTs(void)
{
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
}

HX_RESULT
CHXAudioStream::ConvertCurrentTime(double dBytesPlayed, 
                                   UINT32 ulCurrentTime, 
                                   UINT32& ulAdjustedTime)
{
//    return HXR_FAIL;

    HX_ASSERT(m_bRealAudioStream);

    ulAdjustedTime  = ulCurrentTime;

    LISTPOSITION posRABytes = m_pRAByToTsAdjustedList->GetHeadPosition();
    HXBOOL         bIsDone    = FALSE;
    RealAudioBytesToTimeStamp* pByToTsLower = NULL;
    RealAudioBytesToTimeStamp* pByToTsHigher = NULL;

    INT64   llActualByToTsHigherTimestamp = 0;
    INT64   llActualByToTsLowerTimestamp =0;

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
                /* It means that this stream was added mid-presentation and we have not yet 
                 * played any bits from this stream. Maintain the current time and do not
                 * fudge it.
                 */
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
//{FILE* f1 = ::fopen("d:\\temp\\rasync.txt", "a+"); ::fprintf(f1, "ConvertLowHigh: dBytesPlayed: %f LowTS: %lu HighTS: %lu LowBytes: %f HighBytes: %f\n", dBytesPlayed,pByToTsLower->m_ulTimestamp,pByToTsHigher->m_ulTimestamp, pByToTsLower->m_ulOutNumBytes,pByToTsHigher->m_ulOutNumBytes);::fclose(f1);}

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
//{FILE* f1 = ::fopen("d:\\temp\\multi.txt", "a+"); ::fprintf(f1, "ConvertLHINTER: %p %p\t%lu\t%lu\t%lu\t\t%lu\t%lu\t%lu\t%lu\n", this, m_Owner, (UINT32) dBytesPlayed, ulCurrentTime, ulAdjustedTime, pByToTsLower->m_ulTimestamp, (UINT32) pByToTsLower->m_ulOutNumBytes, pByToTsHigher->m_ulTimestamp, (UINT32) pByToTsHigher->m_ulOutNumBytes);::fclose(f1);}
        }
        else 
        {
            ulAdjustedTime = pByToTsLower->m_ulTimestamp;
//{FILE* f1 = ::fopen("d:\\temp\\multi.txt", "a+"); ::fprintf(f1, "ConvertLH: %p %p\t%lu\t%lu\t%lu\n", this, m_Owner, (UINT32) dBytesPlayed, ulCurrentTime, ulAdjustedTime);::fclose(f1);}
        }
//{FILE* f1 = ::fopen("d:\\temp\\rasync.txt", "a+"); ::fprintf(f1, "ConvertLowHigh: ulCurrentTime: %lu ulAdjustedTime: %lu dBytesPlayed: %f LowTS: %lu HighTS: %lu\n", ulCurrentTime, ulAdjustedTime, dBytesPlayed,pByToTsLower->m_ulTimestamp,pByToTsHigher->m_ulTimestamp);::fclose(f1);}
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
//{FILE* f1 = ::fopen("d:\\temp\\rasync.txt", "a+"); ::fprintf(f1, "ConvertLower: ulCurrentTime: %lu ulAdjustedTime: %lu dBytesPlayed: %f LowTS: %lu m_ulOutNumBytes: %f\n", ulCurrentTime, ulAdjustedTime, dBytesPlayed,pByToTsLower->m_ulTimestamp, pByToTsLower->m_ulOutNumBytes);::fclose(f1);}
    }

    /* Remove all maps before pByToTsLower */
    posRABytes = m_pRAByToTsAdjustedList->GetHeadPosition();
    while(posRABytes)
    {
        RealAudioBytesToTimeStamp* pByToTs = 
            (RealAudioBytesToTimeStamp*) m_pRAByToTsAdjustedList->GetAt(posRABytes);

        if (pByToTs != pByToTsLower)
        {
//{FILE* f1 = ::fopen("d:\\temp\\rasync.txt", "a+"); ::fprintf(f1, "Delete: OutBytes: %f OutTS: %lu\n", pByToTs->m_ulOutNumBytes, pByToTs->m_ulTimestamp);::fclose(f1);}
            delete pByToTs;
            posRABytes = m_pRAByToTsAdjustedList->RemoveAt(posRABytes);
        }
        else
        {
            break;
        }
    }

//{FILE* f1 = ::fopen("d:\\temp\\multi.txt", "a+"); ::fprintf(f1, "Convert: %p %p\t%lu\t%lu\t%lu\n", this, m_Owner, (UINT32) dBytesPlayed, ulCurrentTime, ulAdjustedTime);::fclose(f1);}
    return HXR_OK;
}

void
CHXAudioStream::MapFudgedTimestamps(void)
{
    LISTPOSITION    posRABytes = m_pRAByToTsInList->GetHeadPosition();
    INT64           llActualByToTsInEndTime = 0;
    INT64           llActualByToTsInStartTime = 0;

    while(posRABytes)
    {
        RealAudioBytesToTimeStamp* pByToTs = 
            (RealAudioBytesToTimeStamp*) m_pRAByToTsInList->GetAt(posRABytes);

        llActualByToTsInStartTime = CAST_TO_INT64 (pByToTs->m_ulInTimestamp) + CAST_TO_INT64 m_ulTSRollOver * CAST_TO_INT64 MAX_UINT32;
        llActualByToTsInEndTime = CAST_TO_INT64 (pByToTs->m_ulInEndTime) + CAST_TO_INT64 m_ulTSRollOver * CAST_TO_INT64 MAX_UINT32;

        /* Too late */
        if (llActualByToTsInEndTime < m_llLastStartTimePlayed)
        {
            posRABytes = m_pRAByToTsInList->RemoveAt(posRABytes);
            delete pByToTs;
        }
        else if (llActualByToTsInStartTime <= m_llLastStartTimePlayed
                /*&& pByToTs->m_ulInEndTime >= m_ulLastStartTimePlayed*/)
        {
            /* These two values will be used in determining what time it is */

            // Number of bytes that have been written to the audio device till now*/
            pByToTs->m_ulOutNumBytes = m_Owner->NumberOfBytesWritten();

            HX_ASSERT(m_llLastStartTimePlayed - llActualByToTsInStartTime < MAX_TIMESTAMP_GAP);

            /* Interpolate */
            UINT32 ulTimeDiff = INT64_TO_UINT32(m_llLastStartTimePlayed - llActualByToTsInStartTime);
            pByToTs->m_ulTimestamp += ulTimeDiff;
            
            pByToTs->m_ulDuration   = INT64_TO_UINT32((llActualByToTsInEndTime - 
                                       llActualByToTsInStartTime) - CAST_TO_INT64 ulTimeDiff);
                                      
            posRABytes = m_pRAByToTsInList->RemoveAt(posRABytes);
            m_pRAByToTsAdjustedList->AddTail(pByToTs);

//{FILE* f1 = ::fopen("d:\\temp\\multi.txt", "a+"); ::fprintf(f1, "Map: %p %p\t%lu\t%lu\t%lu\t%lu\t%lu\t\t%lu\n", this, m_Owner, (UINT32) pByToTs->m_ulOutNumBytes, pByToTs->m_ulInTimestamp, pByToTs->m_ulInEndTime, pByToTs->m_ulTimestamp, pByToTs->m_ulOrigTimestamp, m_llLastStartTimePlayed);::fclose(f1);}
        }
        else
        {
            break;
        }
    }
}


void
CHXAudioStream::RemoveExcessCrossFadeData()
{
#if defined(HELIX_FEATURE_CROSSFADE)
    HXAudioInfo*    pInfo = (HXAudioInfo*) m_pDataList->GetHead();
    INT64           llStartTime  = 0;
    INT64           llEndTime    = 0;
    LISTPOSITION lp = m_pDataList->GetHeadPosition();

    while(lp)
    {
        UINT32  ulActualTSRollOver = m_ulTSRollOver;

        pInfo = (HXAudioInfo*) m_pDataList->GetAt(lp);

        if (pInfo->ulStartTime > m_ulLastInputStartTime &&
            ((pInfo->ulStartTime - m_ulLastInputStartTime) > MAX_TIMESTAMP_GAP))
        {
            ulActualTSRollOver--;
        }

        llStartTime     = CAST_TO_INT64 (pInfo->ulStartTime) + 
                          CAST_TO_INT64 (CalcMs(pInfo->pBuffer->GetSize() - pInfo->ulBytesLeft)) +
                          CAST_TO_INT64 ulActualTSRollOver * CAST_TO_INT64 MAX_UINT32;
        llEndTime       = CAST_TO_INT64 (pInfo->ulStartTime) + 
                          CAST_TO_INT64 (CalcMs(pInfo->pBuffer->GetSize())) +
                          CAST_TO_INT64 ulActualTSRollOver * CAST_TO_INT64 MAX_UINT32;

        /* Do we have to remove this block completely */
        if (llEndTime < m_llCrossFadeStartTime)
        {
            lp = m_pDataList->RemoveAt(lp);
            FreeInfo(pInfo);
        }
        /* Remove some part of the block internally */
        else if (llStartTime <= m_llCrossFadeStartTime 
                 /* && ulEndTime >= m_ulCrossFadeStartTime */)
        {
            UINT32 ulBytesToDiscard = 
                    CalcOffset(llStartTime, m_llCrossFadeStartTime);

            /* Make sure that it is at even byte boundary */
            ulBytesToDiscard    -= (ulBytesToDiscard % (2*m_AudioFmt.uChannels));
            if (pInfo->ulBytesLeft > ulBytesToDiscard)
            {
                pInfo->ulBytesLeft -= ulBytesToDiscard;
                pInfo->pOffset     += ulBytesToDiscard;
                
                /* Make it TIME_AUDIO since this would be the first block 
                 * written during cross-fade for *TO* stream.
                 */
                if (m_bFadeToThisStream)
                {
                    pInfo->uAudioStreamType = TIMED_AUDIO;
                }
                break;
            }
            else
            {
                /* It is legal to happen with a *to* stream */
                if (!m_bFadeToThisStream)
                {
                    /* Something is screwed up here */
                    HX_ASSERT(FALSE);
                }

                pInfo->ulBytesLeft = 0;
                lp = m_pDataList->RemoveAt(lp);
                FreeInfo(pInfo);
            }
        }
        else
        {
            /* Not enough data */
            /* It is legal to happen with a *to* stream */
            if (!m_bFadeToThisStream)
            {
                /* Something is screwed up here */
                HX_ASSERT(FALSE);
            }
            break;
        }
    }
#endif /* HELIX_FEATURE_CROSSFADE */
}

void    
CHXAudioStream::SyncStream(INT64 llSyncTime)
{
    m_llLastWriteTime = llSyncTime;
}

void
CHXAudioStream::UpdateStreamLastWriteTime(HXBOOL bForceUpdate /*= FALSE*/)
{
    if (m_bLastWriteTimeUpdated)
    {
        return;
    }

    m_bLastWriteTimeUpdated = TRUE;

    if (m_bIsLive)
    {
        if (!m_pValues || m_pValues->GetPropertyULONG32("LiveSyncStartTime", m_ulBaseTime) != HXR_OK)
        {
            if (bForceUpdate)
            {
                m_bIsLive = FALSE;
                m_ulBaseTime = 0;
                m_llLastWriteTime = m_Owner->GetLastAudioWriteTime();
            }
            else
            {
                /* 
                 * do not set it yet.. we will wait till the first
                 * AddData call 
                 */
                m_bLastWriteTimeUpdated = FALSE;
            }
        }
        else
        {
            m_pValues->GetPropertyULONG32("Delay", m_ulLiveDelay);
            INT64 llLastPlayerWriteTime = m_Owner->GetLastAudioWriteTime();

            if (m_ulLiveDelay > 0 && 
                CAST_TO_INT64  m_ulLiveDelay > llLastPlayerWriteTime &&
                m_ulBaseTime > INT64_TO_UINT32(CAST_TO_INT64  m_ulLiveDelay-llLastPlayerWriteTime))
            {
                m_llLastWriteTime   = CAST_TO_INT64 (m_ulBaseTime - 
                                        INT64_TO_UINT32(CAST_TO_INT64  m_ulLiveDelay-llLastPlayerWriteTime));
            }
            else
            {
                m_llLastWriteTime   = CAST_TO_INT64  m_ulBaseTime;
            }
        }
    }
    else
    {
        // XXX HP
        //
        // Prolbem: 
        // when rewinding audio data upon resume, the audio-push-down worth of data
        // would be missing when the stream's first resume is at the middle of playback,
        // 
        // Solution: 
        // we need to adjust the m_llLastWriteTime to the last audio player write time
        if (m_Owner->IsResumed() && !m_bIsResumed)
        {
            m_llLastWriteTime = m_Owner->GetLastAudioWriteTime();
        }
    }
}

void
CHXAudioStream::SaveLastNMilliSeconds(HXBOOL bSave, UINT32 ulNMilliSeconds)
{
    m_bLastNMilliSecsToBeSaved  = bSave; // TRUE;       //
    m_ulLastNMilliSeconds       = ulNMilliSeconds; // 2000;//

    HX_ASSERT(!m_bLastNMilliSecsToBeSaved || m_ulLastNMilliSeconds > 0);

    // ensure we need to save for atleast 1 sec
    if (m_bLastNMilliSecsToBeSaved && m_ulLastNMilliSeconds < 1000)
    {
        m_ulLastNMilliSeconds = 1000;
    }

    if (!m_bLastNMilliSecsToBeSaved)
    {
        while (m_pLastNMilliSecsList && m_pLastNMilliSecsList->GetCount() > 0)
        {
            HXAudioInfo* pInfo = (HXAudioInfo*) m_pLastNMilliSecsList->RemoveHead();
            FreeInfo(pInfo);
        }

        HX_DELETE(m_pLastNMilliSecsList);
    }
}

void
CHXAudioStream::RewindStream(UINT32 ulTimeToRewind)
{
    HX_ASSERT(m_bLastNMilliSecsToBeSaved);

    if (!m_bCanBeRewound)
    {
        return;
    }

    if (m_bLastNMilliSecsToBeSaved && m_pLastNMilliSecsList)
    {
        HX_ASSERT(m_llLastWriteTime >= ulTimeToRewind);
        if (m_llLastWriteTime >= ulTimeToRewind)
        {
            m_llLastWriteTime -= ulTimeToRewind;
        }
        else
        {
            m_llLastWriteTime = 0;
        }

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

        UINT32 ulLastWriteTime = INT64_TO_UINT32(m_llLastWriteTime - CAST_TO_INT64 m_ulTSRollOver * CAST_TO_INT64 MAX_UINT32);
        HXBOOL bTimedToBeSet = (m_pLastNMilliSecsList->GetCount() > 0);

        while (m_pLastNMilliSecsList->GetCount() > 0)
        {
            pInfo = (HXAudioInfo*) m_pLastNMilliSecsList->RemoveTail();
            m_pDataList->AddHead(pInfo);

            if (pInfo->ulStartTime <= ulLastWriteTime)
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

//{FILE* f1 = ::fopen("d:\\temp\\multi.txt", "a+"); ::fprintf(f1, "%p %p RewindStream %lu %lu\n", this, m_Owner, ulFirstAudioTime, m_llLastWriteTime);::fclose(f1);}

        // put back stuff from adjusted list to in list
        while (m_pRAByToTsAdjustedList && m_pRAByToTsAdjustedList->GetCount() > 0)
        {
            RealAudioBytesToTimeStamp* pByToTs = 
                (RealAudioBytesToTimeStamp*) m_pRAByToTsAdjustedList->RemoveTail();
            // restore original fudge timestamp
            pByToTs->m_ulTimestamp = pByToTs->m_ulOrigTimestamp;
            m_pRAByToTsInList->AddHead(pByToTs);
        }
    }
}

void
CHXAudioStream::Pause(HXBOOL bPlayerPause)
{
    if (m_eState == E_PAUSED)
    {
        return;
    }

    m_eState = E_PAUSED;
    m_bCanBeRewound = FALSE;
    m_bPlayerPause = bPlayerPause;

    return;
}

void
CHXAudioStream::Resume(HXBOOL bPlayerResume)
{
    if (!m_bInited ||
        m_eState == E_PLAYING)
    {
        return;
    }

    UpdateStreamLastWriteTime();

    // add/resume audio stream on the fly
    if( m_Owner->IsResumed() )
    {
        
        if (m_eState != E_PAUSED        &&
            !m_bIsResumed               && 
            (!m_pDataList->IsEmpty() || !m_pInstantaneousList->IsEmpty()))
        {
            m_Owner->AudioStreamStateChanged(E_PLAYING);
            m_eState = E_PLAYING;
        }
        // whoever pause the stream is responsible for resuming the same
        // stream, the stream can either be paused specifically by the SMIL renderer
        // without pausing the playback or be paused by the AudioPlayer which
        // pauses the playback
        else if (!bPlayerResume || m_bPlayerPause)
        {
            m_eState = E_PLAYING;
        }
    }
    else
    {
        m_eState = E_PLAYING;
    }
    
    if (m_eState == E_PLAYING)
    {
        m_bCanBeRewound = TRUE;       
        m_bIsResumed = TRUE;
    }
 
    return;
}

void
CHXAudioStream::Seek(UINT32 ulSeekTime)
{
    m_llLastWriteTime   = CAST_TO_INT64 (m_ulBaseTime + ulSeekTime);

    m_bFirstWrite       = TRUE;
    m_bTobeTimed        = TRUE;
    m_ulTSRollOver      = 0;
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

    // XXX HP what happen if this is called from the client core on
    //        IHXTrack::Seek()
    return;
}

void
CHXAudioStream::Stop(void)
{
    if (m_eState == E_STOPPED)
    {
        return;
    }

    m_eState = E_STOPPED;
    ResetStream();

    return;
}

void    
CHXAudioStream::SetAudioDeviceReflushHint(HXBOOL bSupported)
{
    m_bAudioDeviceReflushHint = bSupported;
    return;
}

void
CHXAudioStream::FreeInfo(HXAudioInfo* pInfo, HXBOOL bInstantaneous /* = FALSE */)
{
    if (m_bLastNMilliSecsToBeSaved && !bInstantaneous)
    {
        if (!m_pLastNMilliSecsList)
        {
            m_pLastNMilliSecsList       = new CHXSimpleList;
            m_ulLastNHeadTime   = pInfo->ulStartTime;
            m_ulLastNTailTime   = pInfo->ulStartTime;
        }

        // reset members
        pInfo->pOffset      = pInfo->pBuffer->GetBuffer();
        pInfo->ulBytesLeft  = pInfo->pBuffer->GetSize();

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
//{FILE* f1 = ::fopen("d:\\temp\\cache.txt", "a+"); ::fprintf(f1, "Discard \n");::fclose(f1);}
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

    if (bAddToTail)
    {
        m_pAvailableBuffers->AddTail((void*) pBuffer);
    }
    else
    {
        m_pAvailableBuffers->AddHead((void*) pBuffer);
    }
#endif


//{FILE* f1 = ::fopen("d:\\temp\\cache.txt", "a+"); ::fprintf(f1, "Added %d\n", m_pAvailableBuffers->GetCount());::fclose(f1);}
    return;
}

/************************************************************************
 *  Method:
 *      IHXCommonClassFactory::CreateInstance
 */
STDMETHODIMP 
CHXAudioStream::CreateInstance
(
    REFCLSID    /*IN*/          rclsid,
    void**      /*OUT*/         ppUnknown
)
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

//{FILE* f1 = ::fopen("d:\\temp\\cache.txt", "a+"); ::fprintf(f1, "Cache Hit %d\n", m_pAvailableBuffers->GetCount());::fclose(f1);}
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
//{FILE* f1 = ::fopen("d:\\temp\\cache.txt", "a+"); ::fprintf(f1, "Cache increased to: %u \n", m_uCacheSize);::fclose(f1);}
            }
        }
        /* 
         * fall down to using the comonclass factory to allocate this buiffer since 
         * we do not have it in the cache
         */
    }

//{FILE* f1 = ::fopen("d:\\temp\\cache.txt", "a+"); ::fprintf(f1, "Cache Miss buffered blocks: %d\n", m_pDataList->GetCount());::fclose(f1);}
    theErr = m_pCommonClassFactory->CreateInstance(rclsid, ppUnknown);
exit:
    return theErr;
}

/************************************************************************
 *  Method:
 *      IHXCommonClassFactory::CreateInstanceAggregatable
 */
STDMETHODIMP 
CHXAudioStream::CreateInstanceAggregatable
(
    REFCLSID        /*IN*/      rclsid,
    REF(IUnknown*)  /*OUT*/     pUnknown,
    IUnknown*       /*IN*/      pUnkOuter
)
{
    return m_pCommonClassFactory->CreateInstanceAggregatable(rclsid, pUnknown, pUnkOuter);
}

/************************************************************************
 *      Method:
 *          IHXUpdateProperties::UpdatePacketTimeOffset
 *      Purpose:
 *          Call this method to update the timestamp offset of cached packets
 */
STDMETHODIMP
CHXAudioStream::UpdatePacketTimeOffset(INT32 lTimeOffset)
{
    HX_RESULT       rc = HXR_OK;
    HXAudioInfo*    pInfo = NULL;

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
    m_ulLastInputStartTime += lTimeOffset;
    m_ulLastInputEndTime += lTimeOffset;  
    m_llLastWriteTime += lTimeOffset;
    m_ulLastNHeadTime += lTimeOffset;  
    m_ulLastNTailTime += lTimeOffset;  
    m_llLastStartTimePlayed += lTimeOffset;

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

void
CHXAudioStream::RollBackTimestamp()
{
    if (m_llLastWriteTime > CAST_TO_INT64 m_ulGranularity)
    {
        m_llLastWriteTime -= CAST_TO_INT64 m_ulGranularity;
    }
}
 
#if defined(HELIX_FEATURE_VOLUME)  
/////////////////////////////////////////////////////////////////////////
// CHXAudioStream::HXVolumeAdviseSink
//
CHXAudioStream::HXStreamVolumeAdviseSink::HXStreamVolumeAdviseSink() :
     m_lRefCount (0)
,    m_pCHXAudioStream (0)
{
}

CHXAudioStream::HXStreamVolumeAdviseSink::~HXStreamVolumeAdviseSink()
{
}

/*
 * IUnknown methods
 */

/////////////////////////////////////////////////////////////////////////
//      Method:
//              IUnknown::QueryInterface
//      Purpose:
//              Implement this to export the interfaces supported by your 
//              object.
//
STDMETHODIMP CHXAudioStream::HXStreamVolumeAdviseSink::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IHXVolumeAdviseSink))
    {
        AddRef();
        *ppvObj = (IHXVolumeAdviseSink*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = this;
        return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

/////////////////////////////////////////////////////////////////////////
//      Method:
//              IUnknown::AddRef
//      Purpose:
//              Everyone usually implements this the same... feel free to use
//              this implementation.
//
STDMETHODIMP_(ULONG32) CHXAudioStream::HXStreamVolumeAdviseSink::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

/////////////////////////////////////////////////////////////////////////
//      Method:
//              IUnknown::Release
//      Purpose:
//              Everyone usually implements this the same... feel free to use
//              this implementation.
//
STDMETHODIMP_(ULONG32) CHXAudioStream::HXStreamVolumeAdviseSink::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}


/*
 *      IHXVolumeAdviseSink methods
 */
STDMETHODIMP CHXAudioStream::HXStreamVolumeAdviseSink::OnVolumeChange
(
    const UINT16        uVolume
)
{
    if (m_pCHXAudioStream)
    {
        // Set the audio device volume
        m_pCHXAudioStream->OnVolumeChange(uVolume);
    }
    return HXR_OK;
}

STDMETHODIMP CHXAudioStream::HXStreamVolumeAdviseSink::OnMuteChange
(
    const HXBOOL          bMute
)
{
    if (m_pCHXAudioStream)
    {
        m_pCHXAudioStream->OnMuteChange(bMute);
    }

    return HXR_OK;
}
#endif /* HELIX_FEATURE_VOLUME */

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
void * HXAudioMacQueue::RemoveHead()
{
        //
    // POINT A
    // 
    // You can look at the qHead anytime you want, but you can't USE a 
    // pointer unless it's OFF of the queue.  Basically you do a Dequeue,
    // and if it succeeds then you know nobody else has it.  If it fails,
    // an error is returned and you don't mess with it.
    //
    
        if (mQueueHeader.qHead)
    {
                HXAudioMacQueueElement * theElement = (HXAudioMacQueueElement *) mQueueHeader.qHead;

                if (theElement)
                {                       
                        OSErr e = ::Dequeue( (QElemPtr) theElement, &mQueueHeader );
                        
                        //
                        // Between points A and D, we can't be guaranteed that the queue header and theElement are valid.  But
                        // Dequeue will TELL us if that pointer is still valid by its return code.  If it can't remove the item
                        // from the queue, then somebody else did and the pointer is no longer ours.  If no error was returned
                        // from dequeue, then it's ours to mess with.
                        //
                                                
                        if (e == noErr)
                        {
                                // at this point we know that we can do whatever we need to with the object.  
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
