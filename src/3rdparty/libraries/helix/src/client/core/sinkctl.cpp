/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: sinkctl.cpp,v 1.24 2007/10/22 21:08:20 ehyche Exp $
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
#include "hxtypes.h"
#include "hxclsnk.h"
#include "hxengin.h"
#include "hxcore.h"
#include "hxengin.h"
#include "hxsched.h"
#include "hxerror.h"
#include "hxescapeutil.h"
#include "hxslist.h"
#include "hxcbobj.h"
#include "cringbuf.h"
#include "hxplayvelocity.h"
#include "baseobj.h"
#include "errdbg.h"
#include "sinkctl.h"
#define HELIX_FEATURE_LOGEVEL_NONE
#include "hxtlogutil.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE
static const char HX_THIS_FILE[] = __FILE__;
#endif

#define MAX_NUM_TIME_ENTRIES 4096 // each ring buffer will use 16k

#if defined(HELIX_FEATURE_LOGGING_TRANSLATOR)

const EHXTLogFuncArea CHXErrorSinkTranslator::m_ulCoreDebugTo4cc[NUM_DOL_CODES] =
{
    HXLOG_GENE, // DOL_GENERIC             -> HXLOG_GENE
    HXLOG_TRAN, // DOL_TRANSPORT           -> HXLOG_TRAN
    HXLOG_ASMX, // DOL_ASM                 -> HXLOG_ASMX
    HXLOG_BAND, // DOL_BWMGR               -> HXLOG_BAND
    HXLOG_TRAN, // DOL_TRANSPORT_EXTENDED  -> HXLOG_TRAN
    HXLOG_AUDI, // DOL_REALAUDIO           -> HXLOG_AUDI
    HXLOG_AUDI, // DOL_REALAUDIO_EXTENDED  -> HXLOG_AUDI
    HXLOG_VIDE, // DOL_REALVIDEO           -> HXLOG_VIDE
    HXLOG_PIXX, // DOL_REALPIX             -> HXLOG_PIXX
    HXLOG_PIXX, // DOL_REALPIX_EXTENDED    -> HXLOG_PIXX
    HXLOG_JPEG, // DOL_JPEG                -> HXLOG_JPEG
    HXLOG_JPEG, // DOL_JPEG_EXTENDED       -> HXLOG_JPEG
    HXLOG_GIFX, // DOL_GIF                 -> HXLOG_GIFX
    HXLOG_GIFX, // DOL_GIF_EXTENDED        -> HXLOG_GIFX
    HXLOG_SWFX, // DOL_FLASH               -> HXLOG_SWFX
    HXLOG_SWFX, // DOL_FLASH_EXTENDED      -> HXLOG_SWFX
    HXLOG_SMIL, // DOL_SMIL                -> HXLOG_SMIL
    HXLOG_SMIL, // DOL_SMIL_EXTENDED       -> HXLOG_SMIL
    HXLOG_TURB, // DOL_TURBOPLAY           -> HXLOG_TURB
    HXLOG_TURB, // DOL_TURBOPLAY_EXTENDED  -> HXLOG_TURB
    HXLOG_SITE, // DOL_SITE                -> HXLOG_SITE
    HXLOG_AUTO, // DOL_AUTOUPDATE          -> HXLOG_AUTO
    HXLOG_RECO, // DOL_RECONNECT           -> HXLOG_RECO
    HXLOG_AUTH, // DOL_AUTHENTICATION      -> HXLOG_AUTH
    HXLOG_CORE, // DOL_CORELOADTIME        -> HXLOG_CORE
    HXLOG_RTSP, // DOL_RTSP                -> HXLOG_RTSP
    HXLOG_STRE, // DOL_STREAMSOURCEMAP     -> HXLOG_STRE
    HXLOG_EVEN, // DOL_REALEVENTS          -> HXLOG_EVEN
    HXLOG_EVEN, // DOL_REALEVENTS_EXTENDED -> HXLOG_EVEN
    HXLOG_BUFF  // DOL_BUFFER_CONTROL      -> HXLOG_BUFF
};

const EHXTLogCode CHXErrorSinkTranslator::m_ulCoreDebugToLevel[NUM_DOL_CODES] =
{
    LC_CLIENT_LEVEL3, // DOL_GENERIC
    LC_CLIENT_LEVEL3, // DOL_TRANSPORT
    LC_CLIENT_LEVEL3, // DOL_ASM
    LC_CLIENT_LEVEL3, // DOL_BWMGR
    LC_CLIENT_LEVEL4, // DOL_TRANSPORT_EXTENDED
    LC_CLIENT_LEVEL3, // DOL_REALAUDIO
    LC_CLIENT_LEVEL4, // DOL_REALAUDIO_EXTENDED
    LC_CLIENT_LEVEL3, // DOL_REALVIDEO
    LC_CLIENT_LEVEL3, // DOL_REALPIX
    LC_CLIENT_LEVEL4, // DOL_REALPIX_EXTENDED
    LC_CLIENT_LEVEL3, // DOL_JPEG
    LC_CLIENT_LEVEL4, // DOL_JPEG_EXTENDED
    LC_CLIENT_LEVEL3, // DOL_GIF
    LC_CLIENT_LEVEL4, // DOL_GIF_EXTENDED
    LC_CLIENT_LEVEL3, // DOL_FLASH
    LC_CLIENT_LEVEL4, // DOL_FLASH_EXTENDED
    LC_CLIENT_LEVEL3, // DOL_SMIL
    LC_CLIENT_LEVEL4, // DOL_SMIL_EXTENDED
    LC_CLIENT_LEVEL3, // DOL_TURBOPLAY
    LC_CLIENT_LEVEL4, // DOL_TURBOPLAY_EXTENDED
    LC_CLIENT_LEVEL3, // DOL_SITE
    LC_CLIENT_LEVEL3, // DOL_AUTOUPDATE
    LC_CLIENT_LEVEL3, // DOL_RECONNECT
    LC_CLIENT_LEVEL3, // DOL_AUTHENTICATION
    LC_CLIENT_LEVEL3, // DOL_CORELOADTIME
    LC_CLIENT_LEVEL3, // DOL_RTSP
    LC_CLIENT_LEVEL3, // DOL_STREAMSOURCEMAP
    LC_CLIENT_LEVEL3, // DOL_REALEVENTS
    LC_CLIENT_LEVEL4, // DOL_REALEVENTS_EXTENDED
    LC_CLIENT_LEVEL3  // DOL_BUFFER_CONTROL
};

#endif /* #if defined(HELIX_FEATURE_LOGGING_TRANSLATOR) */

CHXAdviseSinkControl::PlayerAdviseSink::PlayerAdviseSink(
                                    IHXClientAdviseSink* pAdviseSink,
                                    HXBOOL bInterruptSafe)
{
    m_pAdviseSink           = pAdviseSink;
    m_pAdviseSink->AddRef();
    m_bInterruptSafe        = bInterruptSafe;
    m_pPendingAdviseList    = NULL;
}

CHXAdviseSinkControl::PlayerAdviseSink::~PlayerAdviseSink()
{
    while (m_pPendingAdviseList && m_pPendingAdviseList->GetCount() > 0)
    {
        PendingAdvise* pPendingAdvise = 
                    (PendingAdvise*) m_pPendingAdviseList->RemoveHead();
        delete pPendingAdvise;
    }

    HX_RELEASE(m_pAdviseSink);
    HX_DELETE(m_pPendingAdviseList);
}

CHXAdviseSinkControl::CHXAdviseSinkControl()
    : m_lRefCount(0) 
    , m_pInterruptState(NULL)
    , m_pScheduler(NULL)
    , m_lPlaybackVelocity(HX_PLAYBACK_VELOCITY_NORMAL)
    , m_bKeyFrameMode(FALSE)
    , m_pOrigTime(NULL)
    , m_pWarpTime(NULL)
    , m_ulEnabledFlags(HX_ADVISE_SINK_FLAG_ALL)
{
    m_pCallback = new CHXGenericCallback((void*)this, (fGenericCBFunc)AdviseSinkCallback);
    m_pCallback->AddRef();
#if defined(HELIX_FEATURE_PLAYBACK_VELOCITY)
    m_pOrigTime = new CRingBuffer(MAX_NUM_TIME_ENTRIES);
    m_pWarpTime = new CRingBuffer(MAX_NUM_TIME_ENTRIES);
#endif /* #if defined(HELIX_FEATURE_PLAYBACK_VELOCITY) */
}

CHXAdviseSinkControl::~CHXAdviseSinkControl()
{
    CHXSimpleList::Iterator lSinkerator = m_SinkList.Begin();
    for (;  lSinkerator != m_SinkList.End(); ++lSinkerator)
    {
        PlayerAdviseSink* pPlayerAdviseSink = 
                    (PlayerAdviseSink*) (*lSinkerator);

        HX_DELETE(pPlayerAdviseSink);
    }
    m_SinkList.RemoveAll();

    HX_RELEASE(m_pInterruptState);

    if (m_pCallback && m_pScheduler)
    {
        m_pScheduler->Remove(m_pCallback->GetPendingCallback());
        m_pCallback->CallbackCanceled();
    }

    HX_RELEASE(m_pCallback);
    HX_RELEASE(m_pScheduler);
#if defined(HELIX_FEATURE_PLAYBACK_VELOCITY)
    HX_DELETE(m_pOrigTime);
    HX_DELETE(m_pWarpTime);
#endif /* #if defined(HELIX_FEATURE_PLAYBACK_VELOCITY) */
}

void
CHXAdviseSinkControl::Init(IHXClientEngine* pEngine)
{
    pEngine->QueryInterface(IID_IHXInterruptState, 
                                (void**) &m_pInterruptState);
    
    pEngine->QueryInterface(IID_IHXScheduler, (void**) &m_pScheduler);
}

HX_RESULT   
CHXAdviseSinkControl::AddAdviseSink(IHXClientAdviseSink* pAdviseSink)
{
    IHXInterruptSafe*   pInterruptSafe  =NULL;
    HXBOOL                bInterruptSafe  =FALSE;
    pAdviseSink->QueryInterface(IID_IHXInterruptSafe,(void**) &pInterruptSafe);
    if (pInterruptSafe)
    {
        bInterruptSafe=pInterruptSafe->IsInterruptSafe();
        pInterruptSafe->Release();
    }

    PlayerAdviseSink* pPlayerAdviseSink = new PlayerAdviseSink(pAdviseSink, 
                                                        bInterruptSafe);
    m_SinkList.AddTail(pPlayerAdviseSink);

    return HXR_OK;
}

HX_RESULT   
CHXAdviseSinkControl::RemoveAdviseSink(IHXClientAdviseSink* pAdviseSink)
{
    PlayerAdviseSink* pPlayerAdviseSink;
    CHXSimpleList::Iterator lIterator   = m_SinkList.Begin();
    for (; lIterator != m_SinkList.End(); ++lIterator)
    {
        pPlayerAdviseSink = (PlayerAdviseSink*) (*lIterator);
        if (pPlayerAdviseSink->m_pAdviseSink == pAdviseSink)
        {
            LISTPOSITION pos = m_SinkList.Find(pPlayerAdviseSink);
            m_SinkList.RemoveAt(pos);      
            HX_DELETE(pPlayerAdviseSink);
            return HXR_OK;
        }
    }

    return HXR_FAIL;
}



//
//      This object is never interrupt safe
//      and thus cannot be called at interrupt time.
//
STDMETHODIMP_(HXBOOL) CHXAdviseSinkControl::IsInterruptSafe()
{
        return FALSE;
}

// IHXPlaybackVelocity methods
STDMETHODIMP CHXAdviseSinkControl::InitVelocityControl(IHXPlaybackVelocityResponse* pResponse)
{
    return HXR_OK;
}

STDMETHODIMP CHXAdviseSinkControl::QueryVelocityCaps(REF(IHXPlaybackVelocityCaps*) rpCaps)
{
    return HXR_NOTIMPL;
}

STDMETHODIMP CHXAdviseSinkControl::SetVelocity(INT32 lVelocity, HXBOOL bKeyFrameMode, HXBOOL bAutoSwitch)
{
    m_lPlaybackVelocity = lVelocity;
    m_bKeyFrameMode     = bKeyFrameMode;
    return HXR_OK;
}

STDMETHODIMP_(INT32) CHXAdviseSinkControl::GetVelocity()
{
    return m_lPlaybackVelocity;
}

STDMETHODIMP CHXAdviseSinkControl::SetKeyFrameMode(HXBOOL bKeyFrameMode)
{
    m_bKeyFrameMode = bKeyFrameMode;
    return HXR_OK;
}

STDMETHODIMP_(HXBOOL) CHXAdviseSinkControl::GetKeyFrameMode()
{
    return m_bKeyFrameMode;
}

STDMETHODIMP CHXAdviseSinkControl::CloseVelocityControl()
{
    return HXR_OK;
}

STDMETHODIMP CHXAdviseSinkControl::SetCurrentTimeWarp(UINT32 ulOrigTime, UINT32 ulWarpTime)
{
    HX_RESULT retVal = HXR_FAIL;

#if defined(HELIX_FEATURE_PLAYBACK_VELOCITY)
    if (m_pOrigTime && m_pWarpTime && m_bKeyFrameMode)
    {
        // Clear the return value
        retVal = HXR_OK;
        // m_pOrigTime and m_pWarpTime stay in lockstep with
        // the number of elements, so we only need to check
        // one of them
        if (m_pWarpTime->IsFull())
        {
            // Reallocate and copy the ring buffers
            CRingBuffer* pNewWarpTime = NULL;
            CRingBuffer* pNewOrigTime = NULL;
            retVal = ReallocRingBuffer(m_pWarpTime, pNewWarpTime);
            if (SUCCEEDED(retVal))
            {
                retVal = ReallocRingBuffer(m_pOrigTime, pNewOrigTime);
            }
            if (SUCCEEDED(retVal))
            {
                HX_DELETE(m_pWarpTime);
                m_pWarpTime = pNewWarpTime;
                HX_DELETE(m_pOrigTime);
                m_pOrigTime = pNewOrigTime;
            }
        }
        if (SUCCEEDED(retVal))
        {
            // Check to see if the last time we added is the same as 
            // this time. This will happen when there are multiple packets
            // for the same keyframe
            UINT32 ulLastWarpTime = (UINT32) m_pWarpTime->PeekHead();
            if (ulLastWarpTime != ulWarpTime)
            {
                // Put this time in the queue
                HXLOGL4(HXLOG_TRIK, "CHXAdviseSinkControl::SetCurrentTimeWarp(%lu,%lu) - new time pair added",
                        ulOrigTime, ulWarpTime);
                m_pWarpTime->Put((void*) ulWarpTime);
                m_pOrigTime->Put((void*) ulOrigTime);
            }
        }
    }
#endif /* #if defined(HELIX_FEATURE_PLAYBACK_VELOCITY) */

    return retVal;
}

STDMETHODIMP CHXAdviseSinkControl::GetLastTimeWarp(REF(UINT32) rulOrigTime, REF(UINT32) rulWarpTime)
{
    HX_RESULT retVal = HXR_FAIL;

    if (m_pOrigTime && m_pOrigTime->Count() > 0 &&
        m_pWarpTime && m_pWarpTime->Count() > 0)
    {
        rulOrigTime = (UINT32) m_pOrigTime->PeekHead(0);
        rulWarpTime = (UINT32) m_pWarpTime->PeekHead(0);
        retVal      = HXR_OK;
    }

    return retVal;
}

STDMETHODIMP_(UINT32) CHXAdviseSinkControl::GetOriginalTime(UINT32 ulWarpTime)
{
    UINT32 ulRet = ulWarpTime;

#if defined(HELIX_FEATURE_PLAYBACK_VELOCITY)
    if (m_pWarpTime && m_pOrigTime && m_bKeyFrameMode)
    {
        UINT32 ulWarpedTime[2] = {0, 0};
        while (m_pWarpTime->Count() > 0)
        {
            ulWarpedTime[0] = (UINT32) m_pWarpTime->PeekTail(0);
            if (HX_LATER_OR_SAME_USING_VELOCITY(ulWarpTime, ulWarpedTime[0], m_lPlaybackVelocity))
            {
                if (m_pWarpTime->Count() > 1)
                {
                    ulWarpedTime[1] = (UINT32) m_pWarpTime->PeekTail(1);
                    if (!HX_LATER_OR_SAME_USING_VELOCITY(ulWarpTime, ulWarpedTime[1], m_lPlaybackVelocity))
                    {
                        // This time falls in between our first and second
                        // entries in our list, so now we can interpolate
                        //
                        // Get the original time entries
                        UINT32 ulOrigTime[2];
                        ulOrigTime[0] = (UINT32) m_pOrigTime->PeekTail(0);
                        ulOrigTime[1] = (UINT32) m_pOrigTime->PeekTail(1);
                        // Handle reverse playback
                        UINT32 ulMinIndex = (m_lPlaybackVelocity < 0 ? 1 : 0);
                        UINT32 ulMaxIndex = 1 - ulMinIndex;
                        // Interpolate
                        ulRet = ((ulWarpTime               - ulWarpedTime[ulMinIndex]) *
                                 (ulOrigTime[ulMaxIndex]   - ulOrigTime[ulMinIndex]) /
                                 (ulWarpedTime[ulMaxIndex] - ulWarpedTime[ulMinIndex])) +
                                ulOrigTime[ulMinIndex];
                        // Now break
                        break;
                    }
                    else
                    {
                        // This time is greater than or equal to
                        // our second entry on the list, so we can
                        // dump our first entries in both lists
                        UINT32 ulTmpWarp = (UINT32) m_pWarpTime->Get();
                        UINT32 ulTmpOrig = (UINT32) m_pOrigTime->Get();
                        HXLOGL4(HXLOG_TRIK, "CHXAdviseSinkControl::GetOriginalTime(%lu) removing (orig=%lu,warp=%lu)",
                                ulWarpTime, ulTmpOrig, ulTmpWarp);
                    }
                }
                else
                {
                    UINT32 ulOrigTime0 = (UINT32) m_pOrigTime->PeekTail(0);
                    if (m_lPlaybackVelocity < 0)
                    {
                        UINT32 ulWarpDiff = ulWarpedTime[0] - ulWarpTime;
                        ulRet = (ulOrigTime0 > ulWarpDiff ? ulOrigTime0 - ulWarpDiff : 0);
                    }
                    else
                    {
                        ulRet = ulOrigTime0 + (ulWarpTime - ulWarpedTime[0]);
                    }
                    break;
                }
            }
            else
            {
                break;
            }
        }
    }
#endif /* #if defined(HELIX_FEATURE_PLAYBACK_VELOCITY) */

    return ulRet;
}

STDMETHODIMP_(UINT32) CHXAdviseSinkControl::GetWarpedTime(UINT32 ulOrigTime)
{
    UINT32 ulRet   = ulOrigTime;

#if defined(HELIX_FEATURE_PLAYBACK_VELOCITY)
    UINT32 ulCount = 0;
    if (m_pWarpTime && m_pOrigTime && m_bKeyFrameMode)
    {
        // Get the number of pairs
        ulCount = (UINT32) m_pOrigTime->Count();
        if (ulCount)
        {
            // Get head original time
            UINT32 ulHeadTime = (UINT32) m_pOrigTime->PeekHead(0);
            if (HX_LATER_OR_SAME_USING_VELOCITY(ulOrigTime, ulHeadTime, m_lPlaybackVelocity))
            {
                // This time is "ahead" of the "latest" time we have.
                // Therefore, just make the warped time be the "latest"
                // warped time.
                ulRet = (UINT32) m_pWarpTime->PeekHead(0);
            }
            else
            {
                HXBOOL bFound = FALSE;
                UINT32 i      = 0;
                for (i = 1; i < ulCount; i++)
                {
                    UINT32 ulCurrOrigTime = (UINT32) m_pOrigTime->PeekHead(i);
                    UINT32 ulPrevOrigTime = (UINT32) m_pOrigTime->PeekHead(i-1);
                    if (HX_LATER_OR_SAME_USING_VELOCITY(ulOrigTime, ulCurrOrigTime, m_lPlaybackVelocity) &&
                        HX_EARLIER_USING_VELOCITY(ulOrigTime, ulPrevOrigTime, m_lPlaybackVelocity))
                    {
                        // Get the corresponding warped times
                        UINT32 ulCurrWarpTime = (UINT32) m_pWarpTime->PeekHead(i);
                        UINT32 ulPrevWarpTime = (UINT32) m_pWarpTime->PeekHead(i-1);
                        // Interpolate
                        INT32 lOrigDiff1 = ulOrigTime     - ulCurrOrigTime;
                        INT32 lOrigDiff2 = ulPrevOrigTime - ulCurrOrigTime;
                        INT32 lWarpDiff2 = ulPrevWarpTime - ulCurrWarpTime;
                        HX_ASSERT(lOrigDiff2 != 0);
                        INT32 lWarpDiff1 = 0;
                        if (lOrigDiff2 != 0)
                        {
                            lWarpDiff1 = lOrigDiff1 * lWarpDiff2 / lOrigDiff2;
                        }
                        // Make sure we don't wrap around 0 in reverse
                        if (lWarpDiff1 < 0)
                        {
                            if (((UINT32) -lWarpDiff1) > ulCurrWarpTime)
                            {
                                lWarpDiff1 = - ((INT32) ulCurrWarpTime);
                            }
                        }
                        // Compute the warped time
                        ulRet = ulCurrWarpTime + lWarpDiff1;
                        // Set the flag
                        bFound = TRUE;
                        // Break out
                        break;
                    }
                }
                // Did we find bounding pairs?
                if (!bFound)
                {
                    // In this case the time must be "earlier" than the
                    // "earliest" time we have. Therefore, make it equal
                    // to the earliest warp time we have.
                    ulRet = (UINT32) m_pWarpTime->PeekHead(ulCount - 1);
                }
            }
        }
    }
#if defined(HELIX_FEATURE_LOGLEVEL_4) || defined(HELIX_FEATURE_LOGLEVEL_ALL)
    // Logging-only code
    HXLOGL4(HXLOG_TRIK, "CHXAdviseSinkControl::GetWarpedTime(%lu) %lu entries",
            ulOrigTime, ulCount);
    for (UINT32 j = 0; j < ulCount; j++)
    {
        UINT32 ulOT = (UINT32) m_pOrigTime->PeekHead(j);
        UINT32 ulWT = (UINT32) m_pWarpTime->PeekHead(j);
        HXLOGL4(HXLOG_TRIK, "\tEntry %lu: (orig=%lu,warp=%lu)", j, ulOT, ulWT);
    }
    HXLOGL4(HXLOG_TRIK, "\tGetWarpedTime(%lu) returns %lu",
            ulOrigTime, ulRet);
#endif /* #if defined(HELIX_FEATURE_LOGLEVEL_4) || defined(HELIX_FEATURE_LOGLEVEL_ALL) */
#endif /* #if defined(HELIX_FEATURE_PLAYBACK_VELOCITY) */

    return ulRet;
}

STDMETHODIMP CHXAdviseSinkControl::UpdateVelocity(INT32 lVelocity)
{
    CallAllAdviseSinks(HX_ADVISE_SINK_FLAG_UPDATEVELOCITY, (UINT32) lVelocity, 0, NULL);
    return HXR_OK;
}

STDMETHODIMP CHXAdviseSinkControl::UpdateKeyFrameMode(HXBOOL bKeyFrameMode)
{
    CallAllAdviseSinks(HX_ADVISE_SINK_FLAG_UPDATEKEYFRAMEMODE, (UINT32) bKeyFrameMode, 0, NULL);
    return HXR_OK;
}

// *** IUnknown methods ***

/////////////////////////////////////////////////////////////////////////
//  Method:
//      IUnknown::QueryInterface
//  Purpose:
//      Implement this to export the interfaces supported by your 
//      object.
//
STDMETHODIMP CHXAdviseSinkControl::QueryInterface(REFIID riid, void** ppvObj)
{
    QInterfaceList qiList[] =
        {
            { GET_IIDHANDLE(IID_IHXClientAdviseSink), (IHXClientAdviseSink*)this },
            { GET_IIDHANDLE(IID_IHXInterruptSafe), (IHXInterruptSafe*)this },
#if defined(HELIX_FEATURE_PLAYBACK_VELOCITY)
            { GET_IIDHANDLE(IID_IHXPlaybackVelocity), (IHXPlaybackVelocity*)this },
            { GET_IIDHANDLE(IID_IHXPlaybackVelocityTimeRegulator), (IHXPlaybackVelocityTimeRegulator*)this },
            { GET_IIDHANDLE(IID_IHXPlaybackVelocityResponse), (IHXPlaybackVelocityResponse*) this },
#endif /* #if defined(HELIX_FEATURE_PLAYBACK_VELOCITY) */
            { GET_IIDHANDLE(IID_IUnknown), (IUnknown*)(IHXClientAdviseSink*)this },
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
STDMETHODIMP_(ULONG32) CHXAdviseSinkControl::AddRef()
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
STDMETHODIMP_(ULONG32) CHXAdviseSinkControl::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}


void
CHXAdviseSinkControl::AddToPendingList(PlayerAdviseSink* pPlayerAdviseSink,
                                       UINT32            ulType,
                                       UINT32            ulArg1, 
                                       UINT32            ulArg2, 
                                       char*             pHostName)
{
    if (!pPlayerAdviseSink->m_pPendingAdviseList)
    {
        pPlayerAdviseSink->m_pPendingAdviseList = new CHXSimpleList;
    }

    PendingAdvise* pPendingAdvise  = new PendingAdvise;
    if (pPendingAdvise)
    {
        pPendingAdvise->m_ulAdviseType = ulType;
        pPendingAdvise->m_ulArg1       = ulArg1;
        pPendingAdvise->m_ulArg2       = ulArg2;
        pPendingAdvise->m_pHostName    = NULL;

        if (pHostName)
        {
            pPendingAdvise->m_pHostName = new char[strlen(pHostName) + 1];
            if (pPendingAdvise->m_pHostName)
            {
                strcpy(pPendingAdvise->m_pHostName, pHostName); /* Flawfinder: ignore */
            }
        }

        pPlayerAdviseSink->m_pPendingAdviseList->AddTail((void*) pPendingAdvise);

        if (!m_pCallback->GetPendingCallback())
        {
            m_pCallback->CallbackScheduled(m_pScheduler->RelativeEnter(m_pCallback, 0));
        }
    }
}

void CHXAdviseSinkControl::ProcessAllRequests(void)
{
    CHXSimpleList::Iterator iItr = m_SinkList.Begin();
    for (;  iItr != m_SinkList.End(); ++iItr)
    {
        PlayerAdviseSink* pSink = (PlayerAdviseSink*) (*iItr);
        ProcessPendingRequests(pSink);
    }
}

void CHXAdviseSinkControl::ProcessPendingRequests(PlayerAdviseSink* pSink)
{
    if (pSink)
    {
        while (pSink->m_pPendingAdviseList && 
               pSink->m_pPendingAdviseList->GetCount() > 0)
        {
            PendingAdvise* pAdv = (PendingAdvise*)
                pSink->m_pPendingAdviseList->RemoveHead();
            if (pAdv)
            {
                IssueAdviseSinkCall(pSink->m_pAdviseSink, pAdv->m_ulAdviseType,
                                    pAdv->m_ulArg1, pAdv->m_ulArg2, pAdv->m_pHostName);
            }
            HX_DELETE(pAdv);
        }
    }
}

/*
 *      IHXClientAdviseSink methods
 */

/************************************************************************
 *      Method:
 *          IHXClientAdviseSink::OnPosLength
 *      Purpose:
 *          Called to advise the client that the position or length of the
 *          current playback context has changed.
 */
STDMETHODIMP CHXAdviseSinkControl::OnPosLength(UINT32 ulPosition, UINT32 ulLength)
{
    UINT32 ulOrigPos = ulPosition;
#if defined(HELIX_FEATURE_PLAYBACK_VELOCITY)
    if (m_lPlaybackVelocity != HX_PLAYBACK_VELOCITY_NORMAL && m_bKeyFrameMode)
    {
        ulOrigPos = GetOriginalTime(ulPosition);
        if (ulOrigPos > ulLength)
        {
            ulOrigPos = ulLength;
        }
    }
#endif /* #if defined(HELIX_FEATURE_PLAYBACK_VELOCITY) */
    CallAllAdviseSinks(HX_ADVISE_SINK_FLAG_ONPOSLENGTH, ulOrigPos, ulLength, NULL);

    return HXR_OK;
}

/************************************************************************
 *      Method:
 *          IHXClientAdviseSink::OnPresentationOpened
 *      Purpose:
 *          Called to advise the client a presentation has been opened.
 */
STDMETHODIMP CHXAdviseSinkControl::OnPresentationOpened()
{
    CallAllAdviseSinks(HX_ADVISE_SINK_FLAG_ONPRESENTATIONOPENED);

    return HXR_OK;
}


/************************************************************************
 *      Method:
 *          IHXClientAdviseSink::OnPresentationClosed
 *      Purpose:
 *          Called to advise the client a presentation has been closed.
 */
STDMETHODIMP CHXAdviseSinkControl::OnPresentationClosed()
{
    CallAllAdviseSinks(HX_ADVISE_SINK_FLAG_ONPRESENTATIONCLOSED);

    return HXR_OK;
}

/************************************************************************
 *      Method:
 *          IHXClientAdviseSink::OnStatisticsChanged
 *      Purpose:
 *          Called to advise the client that the presentation statistics
 *          have changed. 
 */
STDMETHODIMP CHXAdviseSinkControl::OnStatisticsChanged(void)
{
    CallAllAdviseSinks(HX_ADVISE_SINK_FLAG_ONSTATISTICSCHANGED);

    return HXR_OK;
}

/************************************************************************
 *      Method:
 *          IHXClientAdviseSink::OnPreSeek
 *      Purpose:
 *          Called by client engine to inform the client that a seek is
 *          about to occur. The render is informed the last time for the 
 *          stream's time line before the seek, as well as the first new
 *          time for the stream's time line after the seek will be completed.
 *
 */
STDMETHODIMP CHXAdviseSinkControl::OnPreSeek(ULONG32 ulOldTime, ULONG32 ulNewTime)
{
#if defined(HELIX_FEATURE_PLAYBACK_VELOCITY)
    // Whenever we seek, we need to clear out
    // all the regulator times
    ClearAllRegulatorTimes();
#endif /* #if defined(HELIX_FEATURE_PLAYBACK_VELOCITY) */
    CallAllAdviseSinks(HX_ADVISE_SINK_FLAG_ONPRESEEK, ulOldTime, ulNewTime);

    return HXR_OK;
}


/************************************************************************
 *      Method:
 *          IHXClientAdviseSink::OnPostSeek
 *      Purpose:
 *          Called by client engine to inform the client that a seek has
 *          just occured. The render is informed the last time for the 
 *          stream's time line before the seek, as well as the first new
 *          time for the stream's time line after the seek.
 *
 */
STDMETHODIMP CHXAdviseSinkControl::OnPostSeek(ULONG32 ulOldTime, ULONG32 ulNewTime)
{
    CallAllAdviseSinks(HX_ADVISE_SINK_FLAG_ONPOSTSEEK, ulOldTime, ulNewTime);

    return HXR_OK;
}


/************************************************************************
 *      Method:
 *          IHXClientAdviseSink::OnStop
 *      Purpose:
 *          Called by client engine to inform the client that a stop has
 *          just occured. 
 *
 */
STDMETHODIMP CHXAdviseSinkControl::OnStop(void)
{
    CallAllAdviseSinks(HX_ADVISE_SINK_FLAG_ONSTOP);

    return HXR_OK;
}

/************************************************************************
 *      Method:
 *          IHXClientAdviseSink::OnPause
 *      Purpose:
 *          Called by client engine to inform the client that a pause has
 *          just occured. The render is informed the last time for the 
 *          stream's time line before the pause.
 *
 */
STDMETHODIMP CHXAdviseSinkControl::OnPause(ULONG32 ulTime)
{
    CallAllAdviseSinks(HX_ADVISE_SINK_FLAG_ONPAUSE, ulTime);

    return HXR_OK;
}


/************************************************************************
 *      Method:
 *          IHXClientAdviseSink::OnBegin
 *      Purpose:
 *          Called by client engine to inform the client that a begin or
 *          resume has just occured. The render is informed the first time 
 *          for the stream's time line after the resume.
 *
 */
STDMETHODIMP CHXAdviseSinkControl::OnBegin(ULONG32 ulTime)
{
    CallAllAdviseSinks(HX_ADVISE_SINK_FLAG_ONBEGIN, ulTime);

    return HXR_OK;
}


/************************************************************************
 *      Method:
 *          IHXClientAdviseSink::OnBuffering
 *      Purpose:
 *          Called by client engine to inform the client that buffering
 *          of data is occuring. The render is informed of the reason for
 *          the buffering (start-up of stream, seek has occured, network
 *          congestion, etc.), as well as percentage complete of the 
 *          buffering process.
 *
 */
STDMETHODIMP CHXAdviseSinkControl::OnBuffering(ULONG32 ulFlags, UINT16 unPercentComplete)
{
    CallAllAdviseSinks(HX_ADVISE_SINK_FLAG_ONBUFFERING, ulFlags, (UINT32) unPercentComplete);

    return HXR_OK;
}


/************************************************************************
 *      Method:
 *          IHXClientAdviseSink::OnContacting
 *      Purpose:
 *          Called by client engine to inform the client is contacting
 *          hosts(s).
 *
 */
STDMETHODIMP CHXAdviseSinkControl::OnContacting(const char* pHostName)
{
    CallAllAdviseSinks(HX_ADVISE_SINK_FLAG_ONCONTACTING, 0, 0, (char*) pHostName);

    return HXR_OK;
}


void CHXAdviseSinkControl::AdviseSinkCallback(void* pParam)
{
    CHXAdviseSinkControl* pObj = (CHXAdviseSinkControl*)pParam;
    
    if (pObj)
    {
        pObj->ProcessAllRequests();
    }
}    

HX_RESULT CHXAdviseSinkControl::ReallocRingBuffer(CRingBuffer* pBuf, REF(CRingBuffer*) rpBuf)
{
    HX_RESULT retVal = HXR_FAIL;

#if defined(HELIX_FEATURE_PLAYBACK_VELOCITY)
    if (pBuf)
    {
        // Create a buffer that's twice as big
        CRingBuffer* pNewBuf = new CRingBuffer(pBuf->Size() * 2);
        if (pNewBuf)
        {
            // Copy the values from the old buffer to the new buffer
            while (pBuf->Count() > 0)
            {
                // CRingBuffer::Get() takes entries
                // off the ring buffer
                pNewBuf->Put(pBuf->Get());
            }
            // Assign the out parameter
            HX_DELETE(rpBuf);
            rpBuf = pNewBuf;
            // Clear the return value
            retVal = HXR_OK;
        }
    }
#endif /* #if defined(HELIX_FEATURE_PLAYBACK_VELOCITY) */

    return retVal;
}

void CHXAdviseSinkControl::ClearAllRegulatorTimes()
{
#if defined(HELIX_FEATURE_PLAYBACK_VELOCITY)
    if (m_pWarpTime)
    {
        while (m_pWarpTime->Count() > 0)
        {
            m_pWarpTime->Get();
        }
    }
    if (m_pOrigTime)
    {
        while (m_pOrigTime->Count() > 0)
        {
            m_pOrigTime->Get();
        }
    }
#endif /* #if defined(HELIX_FEATURE_PLAYBACK_VELOCITY) */
}

void CHXAdviseSinkControl::IssueAdviseSinkCall(IHXClientAdviseSink* pSink, UINT32 ulType,
                                               UINT32 ulArg1, UINT32 ulArg2, char* pszArg3)
{
    if (pSink)
    {
        switch (ulType)
        {
            case HX_ADVISE_SINK_FLAG_ONPOSLENGTH:
                pSink->OnPosLength(ulArg1, ulArg2);
                break;
            case HX_ADVISE_SINK_FLAG_ONPRESENTATIONOPENED:
                pSink->OnPresentationOpened();
                break;
            case HX_ADVISE_SINK_FLAG_ONPRESENTATIONCLOSED:
                pSink->OnPresentationClosed();
                break;
            case HX_ADVISE_SINK_FLAG_ONSTATISTICSCHANGED:
                pSink->OnStatisticsChanged();
                break;
            case HX_ADVISE_SINK_FLAG_ONPRESEEK:
                pSink->OnPreSeek(ulArg1, ulArg2);
                break;
            case HX_ADVISE_SINK_FLAG_ONPOSTSEEK:
                pSink->OnPostSeek(ulArg1, ulArg2);
                break;
            case HX_ADVISE_SINK_FLAG_ONSTOP:
                pSink->OnStop();
                break;
            case HX_ADVISE_SINK_FLAG_ONPAUSE:
                pSink->OnPause(ulArg1);
                break;
            case HX_ADVISE_SINK_FLAG_ONBEGIN:
                pSink->OnBegin(ulArg1);
                break;
            case HX_ADVISE_SINK_FLAG_ONBUFFERING:
                pSink->OnBuffering(ulArg1, (UINT16) ulArg2);
                break;
            case HX_ADVISE_SINK_FLAG_ONCONTACTING:
                pSink->OnContacting((const char*) pszArg3);
                break;
            case HX_ADVISE_SINK_FLAG_UPDATEVELOCITY:
            case HX_ADVISE_SINK_FLAG_UPDATEKEYFRAMEMODE:
                IHXPlaybackVelocityResponse* pResp = NULL;
                pSink->QueryInterface(IID_IHXPlaybackVelocityResponse, (void**) &pResp);
                if (pResp)
                {
                    if (ulType == HX_ADVISE_SINK_FLAG_UPDATEVELOCITY)
                    {
                        pResp->UpdateVelocity((INT32) ulArg1);
                    }
                    else
                    {
                        pResp->UpdateKeyFrameMode((HXBOOL) ulArg1);
                    }
                }
                HX_RELEASE(pResp);
                break;
        }
    }
}

void CHXAdviseSinkControl::CallAllAdviseSinks(UINT32 ulType, UINT32 ulArg1,
                                              UINT32 ulArg2, char* pszArg3)
{
    if (IsEnabled(ulType) && m_pInterruptState)
    {
        CHXSimpleList::Iterator iItr = m_SinkList.Begin();
        for (;  iItr != m_SinkList.End(); ++iItr)
        {
            PlayerAdviseSink* pSink = (PlayerAdviseSink*) (*iItr);
            if (!m_pInterruptState->AtInterruptTime() || pSink->m_bInterruptSafe)
            {
                ProcessPendingRequests(pSink);
                IssueAdviseSinkCall(pSink->m_pAdviseSink, ulType, ulArg1, ulArg2, pszArg3);
            }
            else
            {
                AddToPendingList(pSink, ulType, ulArg1, ulArg2, pszArg3);
            }
        }
    }
}

/************************************************************************
 CHXErrorSinkControl 
*/

CHXErrorSinkControl::CHXErrorSinkControl() :
     m_lRefCount (0)
    ,m_pInterruptState(NULL)
    ,m_pScheduler(NULL)
    ,m_pPendingErrorList(NULL)
    ,m_pErrorCallback(NULL)
{
    m_pErrorCallback = new CHXGenericCallback((void*)this, (fGenericCBFunc)ErrorCallback);
    m_pErrorCallback->AddRef();
}

CHXErrorSinkControl::~CHXErrorSinkControl()
{
    Close();
}

STDMETHODIMP
CHXErrorSinkControl::AddErrorSink       (
                                IHXErrorSink*   pErrorSink,
                                const UINT8     unLowSeverity,
                                const UINT8     unHighSeverity)
{
    if (pErrorSink)
    {
        PlayerErrorSink* pPlayerErrorSink = new PlayerErrorSink(pErrorSink, unLowSeverity, unHighSeverity);
        m_SinkList.AddTail(pPlayerErrorSink);
        pErrorSink->AddRef();
        
    }
    return HXR_OK;
}

STDMETHODIMP
CHXErrorSinkControl::RemoveErrorSink(IHXErrorSink* pErrorSink)
{
    PlayerErrorSink* pPlayerErrorSink;
    CHXSimpleList::Iterator lIterator   = m_SinkList.Begin();
    for (; lIterator != m_SinkList.End(); ++lIterator)
    {
        pPlayerErrorSink = (PlayerErrorSink *) (*lIterator);
        if (pPlayerErrorSink->m_pErrorSink == pErrorSink)
        {
            HX_RELEASE (pErrorSink);
            
            LISTPOSITION pos = m_SinkList.Find(pPlayerErrorSink);
            m_SinkList.RemoveAt(pos);      
            delete pPlayerErrorSink;
            return HXR_OK;
        }
    }
    return HXR_FAIL;
}

STDMETHODIMP 
CHXErrorSinkControl::QueryInterface(REFIID riid, void** ppvObj)
{
    QInterfaceList qiList[] =
        {
            { GET_IIDHANDLE(IID_IHXErrorSinkControl), (IHXErrorSinkControl*)this },
            { GET_IIDHANDLE(IID_IUnknown), (IUnknown*)(IHXErrorSinkControl*)this },
        };
    
    return ::QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
}

STDMETHODIMP_ (ULONG32) 
CHXErrorSinkControl::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_ (ULONG32) 
CHXErrorSinkControl::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

void 
CHXErrorSinkControl::GetSeverityRange(IHXErrorSink* pErrorSink, 
                                                        UINT8& unLowSeverity, 
                                                        UINT8& unHighSeverity)
{
    PlayerErrorSink* pPlayerErrorSink;
    CHXSimpleList::Iterator lIterator   = m_SinkList.Begin();
    for (; lIterator != m_SinkList.End(); ++lIterator)
    {
        pPlayerErrorSink = (PlayerErrorSink *) (*lIterator);
        if (pPlayerErrorSink->m_pErrorSink == pErrorSink)
        {
            unLowSeverity = pPlayerErrorSink->m_unLowSeverity;
            unHighSeverity = pPlayerErrorSink->m_unHighSeverity;
            break;
        }
    }
}

void
CHXErrorSinkControl::Init(IHXClientEngine* pEngine)
{
    pEngine->QueryInterface(IID_IHXInterruptState, 
                                (void**) &m_pInterruptState);
    
    pEngine->QueryInterface(IID_IHXScheduler, (void**) &m_pScheduler);
}

void 
CHXErrorSinkControl::Close()
{
    PlayerErrorSink* pPlayerErrorSink;
    CHXSimpleList::Iterator lIterator   = m_SinkList.Begin();
    for (; lIterator != m_SinkList.End(); ++lIterator)
    {
        pPlayerErrorSink = (PlayerErrorSink *) (*lIterator);
        HX_RELEASE (pPlayerErrorSink->m_pErrorSink);
        delete pPlayerErrorSink;
    }

    m_SinkList.RemoveAll();

    while (m_pPendingErrorList && m_pPendingErrorList->GetCount() > 0)
    {
        ErrorReport* pErrorReport = 
            (ErrorReport*) m_pPendingErrorList->RemoveHead();
        delete pErrorReport;
    }

    
    if (m_pErrorCallback)
    {
        m_pScheduler->Remove(m_pErrorCallback->GetPendingCallback());
        m_pErrorCallback->CallbackCanceled();
        HX_RELEASE(m_pErrorCallback);
    }

    HX_DELETE(m_pPendingErrorList);
    HX_RELEASE(m_pInterruptState);
    HX_RELEASE(m_pScheduler);
}

HX_RESULT 
CHXErrorSinkControl::ErrorOccurred( const UINT8 unSeverity,  
                                    const ULONG32       ulHXCode,
                                    const ULONG32       ulUserCode,
                                    const char* pUserString,
                                    const char* pMoreInfoURL)
{
    if (m_pInterruptState->AtInterruptTime())
    {
        if (!m_pPendingErrorList)
        {
            m_pPendingErrorList = new CHXSimpleList;
        }

        ErrorReport* pErrorReport = new ErrorReport;
        pErrorReport->m_unSeverity      = unSeverity;
        pErrorReport->m_ulHXCode        = ulHXCode;
        pErrorReport->m_ulUserCode      = ulUserCode;

        if (pUserString && *pUserString)
        {
            pErrorReport->m_pUserString = new char[strlen(pUserString) + 1];
            ::strcpy(pErrorReport->m_pUserString, pUserString); /* Flawfinder: ignore */
        }

        if (pMoreInfoURL && *pMoreInfoURL)
        {
            pErrorReport->m_pMoreInfoURL = new char[strlen(pMoreInfoURL) + 1];
            ::strcpy(pErrorReport->m_pMoreInfoURL, pMoreInfoURL); /* Flawfinder: ignore */
        }

        m_pPendingErrorList->AddTail((void*) pErrorReport);

        if (!m_pErrorCallback->GetPendingCallback())
        {
            m_pErrorCallback->CallbackScheduled(
             m_pScheduler->RelativeEnter(m_pErrorCallback, 0));
        }

        return HXR_OK;
    }

    ReportPendingErrors();

    CallReport(unSeverity,
               ulHXCode,
               ulUserCode,
               pUserString,
               pMoreInfoURL);

    return HXR_OK;
}

void
CHXErrorSinkControl::ReportPendingErrors()
{
    if (m_pErrorCallback)
    {
        m_pScheduler->Remove(m_pErrorCallback->GetPendingCallback());
        m_pErrorCallback->CallbackCanceled();
    }

    while (m_pPendingErrorList && m_pPendingErrorList->GetCount() > 0)
    {
        ErrorReport* pErrorReport = 
            (ErrorReport*) m_pPendingErrorList->RemoveHead();
        CallReport(pErrorReport->m_unSeverity,
                   pErrorReport->m_ulHXCode,
                   pErrorReport->m_ulUserCode,
                   pErrorReport->m_pUserString,
                   pErrorReport->m_pMoreInfoURL);
        delete pErrorReport;
    }
}

HX_RESULT       
CHXErrorSinkControl::CallReport
(
    const UINT8 unSeverity,  
    HX_RESULT   ulHXCode,
    const ULONG32       ulUserCode,
    const char* pUserString,
    const char* pMoreInfoURL
)
{
    CHXSimpleList::Iterator ndxSink;
    for (ndxSink = m_SinkList.Begin(); 
         ndxSink != m_SinkList.End(); ++ndxSink)
    {
        PlayerErrorSink* pPlayerErrorSink =
                        (PlayerErrorSink*) (*ndxSink);

        // error-reporting based on its severity          
        if (unSeverity >= pPlayerErrorSink->m_unLowSeverity &&
            unSeverity <= pPlayerErrorSink->m_unHighSeverity)
//          unSeverity <= pPlayerErrorSink->m_unHighSeverity && !m_pAltURLs->GetCount())
        {
            pPlayerErrorSink->m_pErrorSink->ErrorOccurred(unSeverity, ulHXCode,
                                                ulUserCode, pUserString, pMoreInfoURL);
        }
    }

    return HXR_OK;
}


void CHXErrorSinkControl::ErrorCallback(void* pParam)
{
    CHXErrorSinkControl* pObj = (CHXErrorSinkControl*)pParam;

    if (pObj)
    {
        pObj->ReportPendingErrors();
    }
}

#if defined(HELIX_FEATURE_LOGGING_TRANSLATOR)

CHXErrorSinkTranslator::CHXErrorSinkTranslator()
{
    m_lRefCount = 0;
}

CHXErrorSinkTranslator::~CHXErrorSinkTranslator()
{
}

STDMETHODIMP CHXErrorSinkTranslator::QueryInterface(REFIID riid, void** ppvObj)
{
    HX_RESULT retVal = HXR_NOINTERFACE;

    if (ppvObj)
    {
        // Set default
        *ppvObj = NULL;
        // Switch on riid
        if (IsEqualIID(riid, IID_IUnknown))
        {
            *ppvObj = (IUnknown*) (IHXErrorSink*) this;
            retVal  = HXR_OK;
        }
        else if (IsEqualIID(riid, IID_IHXErrorSink))
        {
            *ppvObj = (IHXErrorSink*) this;
            retVal  = HXR_OK;
        }
        if (SUCCEEDED(retVal))
        {
            AddRef();
        }
    }

    return retVal;
}

STDMETHODIMP_(ULONG32) CHXErrorSinkTranslator::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32) CHXErrorSinkTranslator::Release()
{
    HX_ASSERT(m_lRefCount > 0);
    INT32 lRet = InterlockedDecrement(&m_lRefCount);
    if (lRet == 0)
    {
        delete this;
    }
    return lRet;
}

STDMETHODIMP CHXErrorSinkTranslator::ErrorOccurred(const UINT8   unSeverity,
                                                   const ULONG32 ulHXCode,
                                                   const ULONG32 ulUserCode,
                                                   const char*   pUserString,
                                                   const char*   pMoreInfoURL)
{
    HX_RESULT retVal = HXR_FAIL;

    // Only process HXLOG_DEBUG messages, which should
    // correspond to DEBUG_OUT() macros
    if (unSeverity == HXLOG_DEBUG && pUserString &&
        ulUserCode < NUM_DOL_CODES)
    {
        // Clear the return value
        retVal = HXR_OK;
        // Translate from CoreDebug error code to HXLOG 4cc
        EHXTLogFuncArea eFourCC = m_ulCoreDebugTo4cc[ulUserCode];
        // Translate from CoreDebug error code to HXLOG level
        EHXTLogCode eLevel = m_ulCoreDebugToLevel[ulUserCode];
        // Call appropriate HXLog function. We don't use
        // the HXLOGLx macros, since we want these calls to
        // be included regardless of whether we are a debug
        // or release build.
        
        switch (eLevel)
        {
            case LC_CLIENT_LEVEL1:
                HXLog1(eFourCC, "%s", pUserString);
                break;
            case LC_CLIENT_LEVEL2:
                HXLog2(eFourCC, "%s", pUserString);
                break;
            case LC_CLIENT_LEVEL3:
                HXLog3(eFourCC, "%s", pUserString);
                break;
            case LC_CLIENT_LEVEL4:
                HXLog4(eFourCC, "%s", pUserString);
                break;
            case LC_APP_DIAG:
                break;
            case LC_APP_INFO:
                break;
            case LC_APP_WARN:
                break;
            case LC_APP_ERROR:
                break;
            default:
               HX_ASSERT(0);
        }
    }

    return retVal;
}

#endif /* #if defined(HELIX_FEATURE_LOGGING_TRANSLATOR) */

////////////////////////////////////////
// CHXClientStateAdviseSink
CHXClientStateAdviseSink::PlayerClientStateAdviseSink::PlayerClientStateAdviseSink(
                                    IHXClientStateAdviseSink* pClientStateAdviseSink,
                                    HXBOOL bInterruptSafe)
{
    m_pClientStateAdviseSink           = pClientStateAdviseSink;
    m_pClientStateAdviseSink->AddRef();
    m_bInterruptSafe        = bInterruptSafe;
    m_pPendingAdviseList    = NULL;
}

CHXClientStateAdviseSink::PlayerClientStateAdviseSink::~PlayerClientStateAdviseSink()
{
    while (m_pPendingAdviseList && m_pPendingAdviseList->GetCount() > 0)
    {
        PendingAdvise* pPendingAdvise = 
                    (PendingAdvise*) m_pPendingAdviseList->RemoveHead();
        delete pPendingAdvise;
    }

    HX_RELEASE(m_pClientStateAdviseSink);
    HX_DELETE(m_pPendingAdviseList);
}

CHXClientStateAdviseSink::CHXClientStateAdviseSink()
    : m_lRefCount(0) 
    , m_pInterruptState(NULL)
    , m_ulEnabledFlags(HX_CLIENT_STATE_ADVISE_SINK_FLAG_ALL)
{
    m_pCallback = new CHXGenericCallback((void*)this, (fGenericCBFunc)ClientStateAdviseSinkCallback);
    m_pCallback->AddRef();
}

CHXClientStateAdviseSink::~CHXClientStateAdviseSink()
{
    CHXSimpleList::Iterator lSinkerator = m_SinkList.Begin();
    for (;  lSinkerator != m_SinkList.End(); ++lSinkerator)
    {
        PlayerClientStateAdviseSink* pPlayerClientStateAdviseSink = 
                    (PlayerClientStateAdviseSink*) (*lSinkerator);

        HX_DELETE(pPlayerClientStateAdviseSink);
    }
    m_SinkList.RemoveAll();

    HX_RELEASE(m_pInterruptState);

    if (m_pCallback && m_pScheduler)
    {
        m_pScheduler->Remove(m_pCallback->GetPendingCallback());
        m_pCallback->CallbackCanceled();
    }

    HX_RELEASE(m_pCallback);
    HX_RELEASE(m_pScheduler);
}

void
CHXClientStateAdviseSink::Init(IHXClientEngine* pEngine)
{
    pEngine->QueryInterface(IID_IHXInterruptState, 
                                (void**) &m_pInterruptState);
    
    pEngine->QueryInterface(IID_IHXScheduler, (void**) &m_pScheduler);
}

HX_RESULT
CHXClientStateAdviseSink::AddClientStateAdviseSink(IHXClientStateAdviseSink* pClientStateAdviseSink)
{
    IHXInterruptSafe*   pInterruptSafe  =NULL;
    HXBOOL                bInterruptSafe  =FALSE;
    pClientStateAdviseSink->QueryInterface(IID_IHXInterruptSafe,(void**) &pInterruptSafe);
    if (pInterruptSafe)
    {
        bInterruptSafe=pInterruptSafe->IsInterruptSafe();
        pInterruptSafe->Release();
    }

    PlayerClientStateAdviseSink* pPlayerClientStateAdviseSink = new PlayerClientStateAdviseSink(pClientStateAdviseSink, 
                                                        bInterruptSafe);
    m_SinkList.AddTail(pPlayerClientStateAdviseSink);

    return HXR_OK;
}

HX_RESULT
CHXClientStateAdviseSink::RemoveClientStateAdviseSink(IHXClientStateAdviseSink* pClientStateAdviseSink)
{
    PlayerClientStateAdviseSink* pPlayerClientStateAdviseSink;
    CHXSimpleList::Iterator lIterator   = m_SinkList.Begin();
    for (; lIterator != m_SinkList.End(); ++lIterator)
    {
        pPlayerClientStateAdviseSink = (PlayerClientStateAdviseSink*) (*lIterator);
        if (pPlayerClientStateAdviseSink->m_pClientStateAdviseSink == pClientStateAdviseSink)
        {
            LISTPOSITION pos = m_SinkList.Find(pPlayerClientStateAdviseSink);
            m_SinkList.RemoveAt(pos);      
            HX_DELETE(pPlayerClientStateAdviseSink);
            return HXR_OK;
        }
    }

    return HXR_FAIL;
}



//
//      This object is never interrupt safe
//      and thus cannot be called at interrupt time.
//
STDMETHODIMP_(HXBOOL) CHXClientStateAdviseSink::IsInterruptSafe()
{
        return FALSE;
}


// *** IUnknown methods ***

/////////////////////////////////////////////////////////////////////////
//  Method:
//      IUnknown::QueryInterface
//  Purpose:
//      Implement this to export the interfaces supported by your 
//      object.
//
STDMETHODIMP CHXClientStateAdviseSink::QueryInterface(REFIID riid, void** ppvObj)
{
    QInterfaceList qiList[] =
        {
            { GET_IIDHANDLE(IID_IHXClientStateAdviseSink), (IHXClientStateAdviseSink*)this },
            { GET_IIDHANDLE(IID_IHXInterruptSafe), (IHXInterruptSafe*)this },
            { GET_IIDHANDLE(IID_IUnknown), (IUnknown*)(IHXClientStateAdviseSink*)this },
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
STDMETHODIMP_(ULONG32) CHXClientStateAdviseSink::AddRef()
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
STDMETHODIMP_(ULONG32) CHXClientStateAdviseSink::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}


void
CHXClientStateAdviseSink::AddToPendingList(PlayerClientStateAdviseSink* pPlayerClientStateAdviseSink,
                                       UINT32            ulType,
                                       UINT32            ulArg1, 
                                       UINT32            ulArg2, 
                                       char*             pHostName)
{
    if (!pPlayerClientStateAdviseSink->m_pPendingAdviseList)
    {
        pPlayerClientStateAdviseSink->m_pPendingAdviseList = new CHXSimpleList;
    }

    PendingAdvise* pPendingAdvise  = new PendingAdvise;
    if (pPendingAdvise)
    {
        pPendingAdvise->m_ulAdviseType = ulType;
        pPendingAdvise->m_ulArg1       = ulArg1;
        pPendingAdvise->m_ulArg2       = ulArg2;
        pPendingAdvise->m_pHostName    = NULL;

        if (pHostName)
        {
            pPendingAdvise->m_pHostName = new char[strlen(pHostName) + 1];
            if (pPendingAdvise->m_pHostName)
            {
                strcpy(pPendingAdvise->m_pHostName, pHostName); /* Flawfinder: ignore */
            }
        }

        pPlayerClientStateAdviseSink->m_pPendingAdviseList->AddTail((void*) pPendingAdvise);

	if (!m_pCallback->IsCallbackPending())
	{
	    m_pCallback->ScheduleRelative(m_pScheduler, 0);
	}
	
    }
}

void CHXClientStateAdviseSink::ProcessAllRequests(void)
{
    CHXSimpleList::Iterator iItr = m_SinkList.Begin();
    for (;  iItr != m_SinkList.End(); ++iItr)
    {
        PlayerClientStateAdviseSink* pSink = (PlayerClientStateAdviseSink*) (*iItr);
        ProcessPendingRequests(pSink);
    }
}

void CHXClientStateAdviseSink::ProcessPendingRequests(PlayerClientStateAdviseSink* pSink)
{
    if (pSink)
    {
        while (pSink->m_pPendingAdviseList && 
               pSink->m_pPendingAdviseList->GetCount() > 0)
        {
            PendingAdvise* pAdv = (PendingAdvise*)
                pSink->m_pPendingAdviseList->RemoveHead();
            if (pAdv)
            {
                IssueClientStateAdviseSinkCall(pSink->m_pClientStateAdviseSink, pAdv->m_ulAdviseType,
                                    pAdv->m_ulArg1, pAdv->m_ulArg2, pAdv->m_pHostName);
            }
            HX_DELETE(pAdv);
        }
    }
}

/*
 *      IHXClientStateAdviseSink methods
 */

/************************************************************************
 *      Method:
 *          IHXClientStateAdviseSink::OnStateChange
 *      Purpose:
 *          Called to advise the client state has changed state
 */
STDMETHODIMP CHXClientStateAdviseSink::OnStateChange(UINT16 uOldState, UINT16 uNewState)
{
    CallAllClientStateAdviseSinks(HX_CLIENT_STATE_ADVISE_SINK_FLAG_ONSTATECHANGE, uOldState, uNewState);

    return HXR_OK;
}

void CHXClientStateAdviseSink::ClientStateAdviseSinkCallback(void* pParam)
{
    CHXClientStateAdviseSink* pObj = (CHXClientStateAdviseSink*)pParam;
    
    if (pObj)
    {
        pObj->ProcessAllRequests();
    }
}    

void CHXClientStateAdviseSink::IssueClientStateAdviseSinkCall(IHXClientStateAdviseSink* pSink, UINT32 ulType,
                                               UINT32 ulArg1, UINT32 ulArg2, char* pszArg3)
{
    if (pSink)
    {
        switch (ulType)
        {
            case HX_CLIENT_STATE_ADVISE_SINK_FLAG_ONSTATECHANGE:
                pSink->OnStateChange((UINT16)ulArg1, (UINT16)ulArg2);
                break;
        }
    }
}

void CHXClientStateAdviseSink::CallAllClientStateAdviseSinks(UINT32 ulType, UINT32 ulArg1,
                                              UINT32 ulArg2, char* pszArg3)
{
    if (IsEnabled(ulType) && m_pInterruptState)
    {
        CHXSimpleList::Iterator iItr = m_SinkList.Begin();
        for (;  iItr != m_SinkList.End(); ++iItr)
        {
            PlayerClientStateAdviseSink* pSink = (PlayerClientStateAdviseSink*) (*iItr);
            if (!m_pInterruptState->AtInterruptTime() || pSink->m_bInterruptSafe)
            {
                ProcessPendingRequests(pSink);
                IssueClientStateAdviseSinkCall(pSink->m_pClientStateAdviseSink, ulType, ulArg1, ulArg2, pszArg3);
            }
            else
            {
                AddToPendingList(pSink, ulType, ulArg1, ulArg2, pszArg3);
            }
        }
    }
}

