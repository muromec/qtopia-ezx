/* ***** BEGIN LICENSE BLOCK *****
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

#include "hxtypes.h"
#include "hxcom.h"
#include "ihxpckts.h"
#include "hxasm.h"
#include "hxformt.h"
#include "hxprefs.h"
#include "hxccf.h"
#include "hxplayvelocity.h"
#include "rule2flg.h"
#include "hlxclib/string.h"
#include "hxmime.h"
#include "hxslist.h"
#include "hxpacketflags.h"
#include "hxprefutil.h"
#include "hxrecord.h"
#include "velproxy.h"
#include "hxassert.h"
//#define HELIX_FEATURE_LOGLEVEL_NONE
#include "hxtlogutil.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE
static const char HX_THIS_FILE[] = __FILE__;
#endif

#define VELPROXY_DEFAULT_FRAME_RATE         5
#define VELPROXY_KEYFRAMEARRAY_DEFAULT_SIZE 8192
#define VELPROXY_DISABLE_RETIMESTAMPING     "PlaybackVelocity\\DisableRTS"
#define VELPROXY_NUM_VELOCITY_TIERS_STR     "PlaybackVelocity\\NumVelocityTiers"
#define VELPROXY_VELOCITY_BASE_STR          "PlaybackVelocity\\Velocity"
#define VELPROXY_FRAME_RATE_BASE_STR        "PlaybackVelocity\\FrameRate"
#define VELPROXY_BACKWARDS_SEEK_STR         "PlaybackVelocity\\BackwardsSeekInterval"
#define VELPROXY_MAX_NUM_VELOCITY_TIERS     10
#define VELPROXY_MAX_FRAME_RATE             30
#define VELPROXY_DEFAULT_BACKWARDS_SEEK     15000

/*

  This class does three jobs:

     a) filters out non-keyframe packets on video streams
        when in keyframe-only mode;
     b) when in a reverse playback velocity, it reads
        packets out of the fileformat/recordsource in forward
        order into a queue, and then reads them in reverse
        order out of the queue;
     c) re-timestamps packets such that video keyframes
        are evenly spaced and that non-video packets in
        between the retimestamped keyframes are also
        re-timestamped such that all packets coming
        out of the proxy always come in increasing
        (or decreasing) timestamp order.

  These jobs can be thought of as layers:

  Top layer - the user of this class
  ReTimeStamp (RTS) layer
  Reversal layer
  KeyFrame filtering layer
  Bottom layer - the IHXFileFormatObject or IHXRecordSource

  GetPacket requests travel "down" through the layers,
  and PacketReady calls travel "up" through the layers.
  If we are normal playback (m_lPlaybackVelocity == HX_PLAYBACK_VELOCITY_NORMAL),
  then packets go directly from the Bottom layer to 
  the Top layer.

  Detail on each of the layers:

  a) KeyFrame Filtering layer. If we are NOT in keyframe-only
     mode OR the packet is not a video packet OR the video packet
     is a keyframe packet, then this layer is just a pass-through.
     If we are in keyframe-only mode and the packet passing through
     is a NON-keyframe video packet, then we will NOT pass it up to
     the higher layer. Instead will we drop this packet and issue another
     GetPacket() call to the bottom layer on that same stream

  b) Reversal layer. If we are in reverse playback, then
     this layer reads packets in forward order from the 
     KeyFrame Filtering layer. Then it feeds the packets in reverse
     order to the layer above. Once we've run out of buffered packets,
     then it will issue a backward seek to the lower layer, and then
     read in more packets in forward order. It also has to take care
     to note the timestamp of the first packet that comes out of the
     lower layer after a reverse seek, since that will be the 
     limit of the packets buffered after the next seek.

  c) ReTimeStamp (RTS) layer. For some datatypes such as RealVideo,
     keyframes are irregularly spaced. However, for a pleasant
     fast-forward "experience", we want to see regular keyframes.
     Therefore, this layer fills in keyframes at a regular rate.
     Since packets coming out of the proxy must always be incrasing
     (or decreasing if in reverse playback) across all streams, then
     the other streams must be also be re-timestamped piecewise between
     each video keyframe.

 */

const UINT32 CHXPlaybackVelocityProxy::m_ulDefaultVelocity[VELPROXY_DEFAULT_NUM_VELOCITY_TIERS]    =
{600, 1200, 2000};
const UINT32 CHXPlaybackVelocityProxy::m_ulDefaultFrameRate[VELPROXY_DEFAULT_NUM_VELOCITY_TIERS+1] =
{2, 3, 4, 5};

CHXStreamInfo::CHXStreamInfo(UINT32 ulStreamNumber)
{
    m_bIsVideo                        = FALSE;
    m_bIsRealVideo                    = FALSE;
    m_bIsSparseStream                 = FALSE;
    m_bUseOffsetHandler               = FALSE;
    m_bKeyFrameModeTransition         = FALSE;
    m_ulStreamNumber                  = ulStreamNumber;
    m_ulLastASMSwitchOnTimeStamp      = 0;
    m_ulLastOriginalTimeStamp         = 0;
    m_ulLastWarpedTimeStamp           = 0;
    m_ulInitialPacketTime             = 0;
    m_ulLastInitialPacketTime         = 0;
    m_bFirstPacketAfterInternalSeek   = FALSE;
    m_bExternalPacketRequested        = FALSE;
    m_bReversalPacketRequested        = FALSE;
    m_bReTimeStampPacketRequested     = FALSE;
    m_bInternalPacketRequested        = FALSE;
    m_bReversalBufferingInProgress    = FALSE;
    m_pReversalQueue                  = NULL;
    m_pRuleToFlagMap                  = NULL;
}

CHXStreamInfo::~CHXStreamInfo()
{
    ClearPacketQueue(m_pReversalQueue);
    HX_DELETE(m_pReversalQueue);
    HX_DELETE(m_pRuleToFlagMap);
}

void CHXStreamInfo::ClearPacketQueue(CHXSimpleList* pQueue)
{
    if (pQueue)
    {
        while (pQueue->GetCount() > 0)
        {
            IHXPacket* pPacket = (IHXPacket*) pQueue->RemoveHead();
            HX_RELEASE(pPacket);
        }
    }
}

CHXPlaybackVelocityProxy::CHXPlaybackVelocityProxy(IUnknown* pContext)
{
    m_lRefCount                    = 0;
    m_pContext                     = pContext;
    m_pCommonClassFactory          = NULL;
    m_pResponse                    = NULL;
    m_bKeyFrameMode                = FALSE;
    m_bExternalSeekRequested       = FALSE;
    m_bInternalSeekRequested       = FALSE;
    m_ulFrameRate                  = VELPROXY_DEFAULT_FRAME_RATE;
    m_ulFrameInterval              = 1000 / m_ulFrameRate;
    m_ulBackwardsSeekInterval      = VELPROXY_DEFAULT_BACKWARDS_SEEK;
    m_lPlaybackVelocity            = HX_PLAYBACK_VELOCITY_NORMAL;
    m_ulNumStreams                 = 0;
    m_ppStreamInfo                 = NULL;
    m_pOffsetHandler               = NULL;
    m_pVelocityResponse            = NULL;
    m_pTimeRegulator               = NULL;
    m_ulLastReverseSeekTime        = 0;
    m_ulExternalSeekTime           = 0;
    m_ulNumPrefVelocityTiers       = 0;
    m_pulPrefFrameRate             = NULL;
    m_pulPrefVelocity              = NULL;
    m_bSendingReversalPackets      = FALSE;
    m_bDisableReTimeStamping       = FALSE;
    m_bReversalBufferingInProgress = FALSE;
    m_bSourceCanDoReverse          = FALSE;
    if (m_pContext)
    {
        m_pContext->AddRef();
        // QI for IHXCommonClassFactory
        m_pContext->QueryInterface(IID_IHXCommonClassFactory,
                                   (void**) &m_pCommonClassFactory);
        // QI for the IHXPlaybackVelocityTimeRegulator interface
        m_pContext->QueryInterface(IID_IHXPlaybackVelocityTimeRegulator,
                                   (void**) &m_pTimeRegulator);
        // Read the playback velocity preferences
        ReadVelocityPrefs(m_pContext);
    }
}

CHXPlaybackVelocityProxy::~CHXPlaybackVelocityProxy()
{
    Close();
}

STDMETHODIMP CHXPlaybackVelocityProxy::QueryInterface(REFIID riid, void** ppvObj)
{
    HX_RESULT retVal = HXR_NOINTERFACE;

    if (ppvObj)
    {
        // Set default
        *ppvObj = NULL;
        // Switch on riid
        // We intentionally do not check for IUnknown, since
        // we want to let the IUnknown QI pass all the way
        // through to the underlying IHXFileFormatObject
        // or IHXRecordSource.
        if (IsEqualIID(riid, IID_IHXFormatResponse))
        {
            *ppvObj = (IHXFormatResponse*) this;
            retVal  = HXR_OK;
        }
        else if (IsEqualIID(riid, IID_IHXBackChannel))
        {
            *ppvObj = (IHXBackChannel*) this;
            retVal  = HXR_OK;
        }
        else if (IsEqualIID(riid, IID_IHXPlaybackVelocity))
        {
            *ppvObj = (IHXPlaybackVelocity*) this;
            retVal  = HXR_OK;
        }
        else if (IsEqualIID(riid, IID_IHXPacketTimeOffsetHandlerResponse))
        {
            *ppvObj = (IHXPacketTimeOffsetHandlerResponse*) this;
            retVal  = HXR_OK;
        }
        if (SUCCEEDED(retVal))
        {
            AddRef();
        }
    }

    return retVal;
}

STDMETHODIMP_(ULONG32) CHXPlaybackVelocityProxy::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32) CHXPlaybackVelocityProxy::Release()
{
    HX_ASSERT(m_lRefCount > 0);
    INT32 lRet = InterlockedDecrement(&m_lRefCount);
    if (lRet == 0)
    {
        delete this;
    }
    return lRet;
}

STDMETHODIMP CHXPlaybackVelocityProxy::InitDone(HX_RESULT status)
{
    return (m_pResponse ? m_pResponse->InitDone(status) : HXR_UNEXPECTED);
}

STDMETHODIMP CHXPlaybackVelocityProxy::PacketReady(HX_RESULT status, IHXPacket* pPacket)
{
    HXLOGL4(HXLOG_TRIK, "Proxy PacketReady(0x%08x,) strm=%u pts=%lu rule=%u flags=0x%02x",
            status,
            (pPacket ? pPacket->GetStreamNumber() : 0),
            (pPacket ? pPacket->GetTime() : 0),
            (pPacket ? pPacket->GetASMRuleNumber() : 0),
            (pPacket ? pPacket->GetASMFlags() : 0));
    HX_RESULT retVal = HXR_UNEXPECTED;

    if (pPacket)
    {
        UINT32 ulStreamNumber = (UINT32) pPacket->GetStreamNumber();
        if (ulStreamNumber < m_ulNumStreams &&
            m_ppStreamInfo && m_ppStreamInfo[ulStreamNumber] &&
            m_pResponse)
        {
            // Clear the return value
            retVal = HXR_OK;
            // Clear the internal packet request flag
            m_ppStreamInfo[ulStreamNumber]->m_bInternalPacketRequested = FALSE;
            // Are we playing at non-1x speed?
            if (SUCCEEDED(status) && m_lPlaybackVelocity != HX_PLAYBACK_VELOCITY_NORMAL)
            {
                // Filter out video keyframes if we are in keyframe-only mode
                retVal = PacketReadyFilterKeyFrame(pPacket, m_ppStreamInfo[ulStreamNumber]);
            }
            else
            {
                // We are in normal 1x playback (or PacketReady returned
                // failure), so just forward the packet on
                retVal = _PacketReady(status, pPacket, m_ppStreamInfo[ulStreamNumber]);
            }
        }
    }

    return retVal;
}

STDMETHODIMP CHXPlaybackVelocityProxy::SeekDone(HX_RESULT status)
{
    HXLOGL4(HXLOG_TRIK, "Proxy SeekDone(0x%08x)", status);

    HX_RESULT retVal = HXR_OK;

    // Clear the flag saying an internal seek was requested
    m_bInternalSeekRequested = FALSE;
    // Was an external seek requested?
    if (m_bExternalSeekRequested)
    {
        retVal = _SeekDone(status);
    }
    else
    {
        // This must have been an internal seek due
        // to reverse playback
        HX_ASSERT(m_lPlaybackVelocity < 0 && !m_bSourceCanDoReverse);
        // We need to build up the reversal queue of packets, so we will
        // issue GetPackets() on all streams
        if (m_ppStreamInfo)
        {
            for (UINT32 i = 0; i < m_ulNumStreams; i++)
            {
                if (m_ppStreamInfo[i])
                {
                    // Set the flag saying that reversal buffering
                    // is in progress for this stream
                    m_ppStreamInfo[i]->m_bReversalBufferingInProgress = TRUE;
                }
                // Call GetPacket on this stream
                _GetPacket(m_ppStreamInfo[i]);
            }
        }
    }

    return retVal;
}

STDMETHODIMP CHXPlaybackVelocityProxy::FileHeaderReady(HX_RESULT status, IHXValues* pHeader)
{
    HX_RESULT retVal = HXR_OK;

    // Initialize from the file header. We don't
    // check the return value since we are going
    // to pass the header on anyway.
    InitFileHeader(pHeader);
    // Pass on to the response interface
    if (m_pResponse)
    {
        retVal = m_pResponse->FileHeaderReady(status, pHeader);
    }

    return retVal;
}

STDMETHODIMP CHXPlaybackVelocityProxy::StreamHeaderReady(HX_RESULT status, IHXValues* pHeader)
{
    HX_RESULT retVal = HXR_OK;

    // Initialize the stream header. We don't check
    // the return value since we are going to pass
    // the header on anyway.
    InitStreamHeader(pHeader);
    // Pass on to the response interface
    if (m_pResponse)
    {
        retVal = m_pResponse->StreamHeaderReady(status, pHeader);
    }

    return retVal;
}

STDMETHODIMP CHXPlaybackVelocityProxy::StreamDone(UINT16 unStreamNumber)
{
    return (m_pResponse ? m_pResponse->StreamDone(unStreamNumber) : HXR_UNEXPECTED);
}

STDMETHODIMP CHXPlaybackVelocityProxy::PacketReady(IHXPacket* pPacket)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pPacket)
    {
        // Get the packet buffer
        IHXBuffer* pBuffer = pPacket->GetBuffer();
        if (pBuffer)
        {
            // Get the buffer string
            const char* pszStr = (const char*) pBuffer->GetBuffer();
            if (pszStr)
            {
                const char* pszMsg  = "SetKeyFrameMode: TRUE";
                if (!strncmp(pszMsg, pszStr, pBuffer->GetSize()))
                {
                    // This is a go-to-keyframe-mode message, so 
                    // if we have a velocity response interface,
                    // then update the keyframe mode
                    if (m_pVelocityResponse)
                    {
                        retVal = m_pVelocityResponse->UpdateKeyFrameMode(TRUE);
                    }
                }
                else
                {
                    // This is not a go-to-keyframe-mode-message,
                    // so just pass it along. It's OK if it fails
                    // (i.e. - record source or fileformat may not
                    // support IHXBackChannel).
                    _BackChannelPacketReady(pPacket);
                    // Clear the return value
                    retVal = HXR_OK;
                }
            }
        }
        HX_RELEASE(pBuffer);
    }

    return retVal;
}

STDMETHODIMP CHXPlaybackVelocityProxy::InitVelocityControl(IHXPlaybackVelocityResponse* pResponse)
{
    if (pResponse)
    {
        HX_RELEASE(m_pVelocityResponse);
        m_pVelocityResponse = pResponse;
        m_pVelocityResponse->AddRef();
    }
    return HXR_OK;
}

STDMETHODIMP CHXPlaybackVelocityProxy::QueryVelocityCaps(REF(IHXPlaybackVelocityCaps*) rpCaps)
{
    return _QueryVelocityCaps(rpCaps);
}

STDMETHODIMP CHXPlaybackVelocityProxy::SetVelocity(INT32 lVelocity, HXBOOL bKeyFrameMode, HXBOOL bAutoSwitch)
{
    _SetVelocity(lVelocity, bKeyFrameMode, bAutoSwitch);
    m_lPlaybackVelocity = lVelocity;
    if (bKeyFrameMode != m_bKeyFrameMode)
    {
        m_bKeyFrameMode = bKeyFrameMode;
        SetKeyFrameModeTransitionFlag();
    }
    // Get the frame rate from the velocity
    UINT32  ulNumTiers = VELPROXY_DEFAULT_NUM_VELOCITY_TIERS;
    UINT32* pVelArr    = (UINT32*) &m_ulDefaultVelocity[0];
    UINT32* pRateArr   = (UINT32*) &m_ulDefaultFrameRate[0];
    // If we have preference settings, use them
    if (m_ulNumPrefVelocityTiers && m_pulPrefVelocity && m_pulPrefFrameRate)
    {
        ulNumTiers = m_ulNumPrefVelocityTiers;
        pVelArr    = m_pulPrefVelocity;
        pRateArr   = m_pulPrefFrameRate;
    }
    // Get the velocity
    UINT32 ulTmp = GetFrameRateFromVelocity(lVelocity, ulNumTiers, pVelArr, pRateArr);
    // Sanity check
    if (ulTmp)
    {
        m_ulFrameRate = ulTmp;
    }
    // Recompute the frame interval based on the playback velocity and frame rate
    UINT32 ulVelocity = (UINT32) (m_lPlaybackVelocity < 0 ? -m_lPlaybackVelocity : m_lPlaybackVelocity);
    m_ulFrameInterval = ulVelocity * 10 / m_ulFrameRate;

    return HXR_OK;
}

STDMETHODIMP_(INT32) CHXPlaybackVelocityProxy::GetVelocity()
{
    return m_lPlaybackVelocity;
}

STDMETHODIMP CHXPlaybackVelocityProxy::SetKeyFrameMode(HXBOOL bKeyFrameMode)
{
    if (bKeyFrameMode != m_bKeyFrameMode)
    {
        m_bKeyFrameMode = bKeyFrameMode;
        SetKeyFrameModeTransitionFlag();
        _SetKeyFrameMode(bKeyFrameMode);
    }
    return HXR_OK;
}

STDMETHODIMP_(HXBOOL) CHXPlaybackVelocityProxy::GetKeyFrameMode()
{
    return m_bKeyFrameMode;
}

STDMETHODIMP_(UINT32) CHXPlaybackVelocityProxy::GetKeyFrameInterval()
{
    return m_ulFrameInterval;
}

STDMETHODIMP CHXPlaybackVelocityProxy::CloseVelocityControl()
{
    HX_RELEASE(m_pVelocityResponse);
    return HXR_OK;
}

STDMETHODIMP CHXPlaybackVelocityProxy::TimeOffsetPacketReady(IHXPacket* pPacket)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pPacket)
    {
        // Get the stream number
        UINT32 ulStreamNumber = (UINT32) pPacket->GetStreamNumber();
        // Sanity check the stream number
        if (ulStreamNumber < m_ulNumStreams &&
            m_ppStreamInfo && m_ppStreamInfo[ulStreamNumber])
        {
            // Clear the return value
            retVal = HXR_OK;
            // Get the CHXStreamInfo object
            CHXStreamInfo* pInfo = m_ppStreamInfo[ulStreamNumber];
            // Now pass it on to our response interface
            _PacketReady(HXR_OK, pPacket, pInfo);
        }
    }

    return retVal;
}

STDMETHODIMP CHXPlaybackVelocityProxy::Init(IHXFormatResponse* pResponse)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pResponse)
    {
        HX_RELEASE(m_pResponse);
        m_pResponse = pResponse;
        m_pResponse->AddRef();
        // Attempt to get the offset handler interface (OK if it fails)
        HX_RELEASE(m_pOffsetHandler);
        _GetOffsetHandler(m_pOffsetHandler);
        retVal = HXR_OK;
    }

    return retVal;
}

STDMETHODIMP CHXPlaybackVelocityProxy::Close()
{
    ClearStreamInfoArray();
    HX_RELEASE(m_pContext);
    HX_RELEASE(m_pCommonClassFactory);
    HX_RELEASE(m_pResponse);
    HX_RELEASE(m_pOffsetHandler);
    HX_RELEASE(m_pVelocityResponse);
    HX_RELEASE(m_pTimeRegulator);
    HX_VECTOR_DELETE(m_pulPrefVelocity);
    HX_VECTOR_DELETE(m_pulPrefFrameRate);
    return HXR_OK;
}

void CHXPlaybackVelocityProxy::ClearStreamInfoArray()
{
    if (m_ulNumStreams && m_ppStreamInfo)
    {
        for (UINT32 i = 0; i < m_ulNumStreams; i++)
        {
            HX_DELETE(m_ppStreamInfo[i]);
        }
        HX_VECTOR_DELETE(m_ppStreamInfo);
    }
}

HX_RESULT CHXPlaybackVelocityProxy::ReTimeStampAndSendPacket(IHXPacket* pPacket, CHXStreamInfo* pInfo, UINT32 ulNewTime)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pPacket && pInfo)
    {
        // Save the original and warped timestamp. We will pass these
        // values to the time regulator interface
        // value to the time regulator interface
        UINT32 ulOldTime = pPacket->GetTime();
        pInfo->m_ulLastOriginalTimeStamp = ulOldTime;
        pInfo->m_ulLastWarpedTimeStamp   = ulNewTime;
        // Are the new and old times the same?
        if (ulOldTime != ulNewTime)
        {
            // Do we have an offset handler?
            if (m_pOffsetHandler && pInfo->m_bUseOffsetHandler)
            {
                // Yes, we have IHXPacketTimeOffsetHandler support,
                // so use this interface to re-timestamp. This will
                // update any internal timestamps.
                //
                // First compute the time difference and whether
                // we are adding or subtracting
                HXBOOL   bAdd   = FALSE;
                UINT32 ulDiff = 0;
                if (ulNewTime > ulOldTime)
                {
                    ulDiff = ulNewTime - ulOldTime;
                    bAdd   = TRUE;
                }
                else
                {
                    ulDiff = ulOldTime - ulNewTime;
                    bAdd   = FALSE;
                }
                // Now set the time offset
                retVal = m_pOffsetHandler->SetTimeOffset(ulDiff, bAdd);
                if (SUCCEEDED(retVal))
                {
                    // Now make the call to offset the packet. This will
                    // return in TimeOffsetPacketReady().
                    retVal = m_pOffsetHandler->HandlePacket(pPacket);
                }
            }
            else
            {
                // We do not have an offset handler interface, so 
                // just change the packet timestamp ourselves
                //
                // Get the packet info
                IHXBuffer* pBuffer         = NULL;
                UINT32     ulTime          = 0;
                UINT16     usStreamNumber  = 0;
                UINT8      ucASMFlags      = 0;
                UINT16     usASMRuleNumber = 0;
                retVal = pPacket->Get(pBuffer,
                                      ulTime,
                                      usStreamNumber,
                                      ucASMFlags,
                                      usASMRuleNumber);
                if (SUCCEEDED(retVal))
                {
                    // Create a new packet
                    IHXPacket* pNewPacket = NULL;
                    retVal = m_pCommonClassFactory->CreateInstance(CLSID_IHXPacket, (void**) &pNewPacket);
                    if (SUCCEEDED(retVal))
                    {
                        // Set the new packet
                        retVal = pNewPacket->Set(pBuffer,
                                                 ulNewTime,
                                                 usStreamNumber,
                                                 ucASMFlags,
                                                 usASMRuleNumber);
                        if (SUCCEEDED(retVal))
                        {
                            // Pass the packet on through
                            retVal = TimeOffsetPacketReady(pNewPacket);
                        }
                    }
                    HX_RELEASE(pNewPacket);
                }
                HX_RELEASE(pBuffer);
            }
        }
        else
        {
            // The new and old times are the same, so just pass
            // the packet through without modifying it
            retVal = TimeOffsetPacketReady(pPacket);
        }
    }

    return retVal;
}

HX_RESULT CHXPlaybackVelocityProxy::_PacketReady(HX_RESULT status, IHXPacket* pPacket, CHXStreamInfo* pInfo)
{
    HXLOGL4(HXLOG_TRIK, "Proxy _PacketReady(0x%08x,) strm=%u pts=%lu rule=%u flags=0x%02x",
            status,
            (pPacket ? pPacket->GetStreamNumber() : 0),
            (pPacket ? pPacket->GetTime() : 0),
            (pPacket ? pPacket->GetASMRuleNumber() : 0),
            (pPacket ? pPacket->GetASMFlags() : 0));
    if (pInfo)
    {
        pInfo->m_bReTimeStampPacketRequested = FALSE;
        pInfo->m_bExternalPacketRequested    = FALSE;
    }
    return (m_pResponse ? m_pResponse->PacketReady(status, pPacket) : HXR_UNEXPECTED);
}

HX_RESULT CHXPlaybackVelocityProxy::_SeekDone(HX_RESULT status)
{
    HX_RESULT retVal = HXR_UNEXPECTED;

    // If this was an external seek request, then we
    // need to call SeekDone() on our response interface
    if (m_bExternalSeekRequested)
    {
        // Clear the flag
        m_bExternalSeekRequested = FALSE;
        // Call the response's SeekDone
        if (m_pResponse)
        {
            retVal = m_pResponse->SeekDone(status);
        }
    }

    return retVal;
}

HX_RESULT CHXPlaybackVelocityProxy::AddPacketToQueue(CHXSimpleList*& rpQueue, IHXPacket* pPacket, HXBOOL bHead)
{
    HX_RESULT retVal = HXR_OUTOFMEMORY;

    if (!rpQueue)
    {
        rpQueue = new CHXSimpleList();
    }
    if (rpQueue)
    {
        // AddRef the packet
        pPacket->AddRef();
        // Add the packet at the head or tail?
        if (bHead)
        {
            // Put it on the queue at the HEAD
            rpQueue->AddHead((void*) pPacket);
        }
        else
        {
            // Put it on the queue at the TAIL
            rpQueue->AddTail((void*) pPacket);
        }
        // Clear the return value
        retVal = HXR_OK;
    }

    return retVal;
}

HXBOOL CHXPlaybackVelocityProxy::SendPacketFromAnyReversalQueue()
{
    HXBOOL bRet = FALSE;

    if (m_ppStreamInfo)
    {
        // Find the stream which has the maximum timestamp
        UINT32 ulMaxIndex  = 0;
        UINT32 ulMax       = 0;
        HXBOOL   bHavePacket = FALSE;
        for (UINT32 i = 0; i < m_ulNumStreams; i++)
        {
            if (m_ppStreamInfo[i] &&
                m_ppStreamInfo[i]->m_pReversalQueue &&
                m_ppStreamInfo[i]->m_pReversalQueue->GetCount() > 0)
            {
                // Check the timestamp of the next packet
                // we would take off this queue.
                IHXPacket* pPacket = (IHXPacket*) m_ppStreamInfo[i]->m_pReversalQueue->GetHead();
                if (pPacket)
                {
                    UINT32 ulTime = pPacket->GetTime();
                    if (!bHavePacket || ulTime > ulMax)
                    {
                        ulMax      = ulTime;
                        ulMaxIndex = i;
                    }
                    bHavePacket = TRUE;
                }
            }
        }
        if (bHavePacket)
        {
            // Is there an outstanding request on this stream?
            if (m_ppStreamInfo[ulMaxIndex]->m_bReversalPacketRequested)
            {
                // We have a packet, and there was a request for this stream,
                // so send the packet
                bRet = SendPacketFromThisReversalQueue(m_ppStreamInfo[ulMaxIndex]);
            }
        }
    }

    return bRet;
}

HXBOOL CHXPlaybackVelocityProxy::SendPacketFromThisReversalQueue(CHXStreamInfo* pInfo)
{
    HXBOOL bRet = FALSE;

    if (pInfo && pInfo->m_pReversalQueue && pInfo->m_pReversalQueue->GetCount() > 0)
    {
        IHXPacket* pPacket = (IHXPacket*) pInfo->m_pReversalQueue->RemoveHead();
        if (pPacket)
        {
            // Send the packet
            HX_RESULT rv = PacketReadyReTimeStamp(pPacket, pInfo);
            if (SUCCEEDED(rv))
            {
                bRet = TRUE;
            }
        }
        // Release the reversal queue's ref on the packet
        HX_RELEASE(pPacket);
    }

    return bRet;
}

UINT32 CHXPlaybackVelocityProxy::GetNumQueuedReversalPackets()
{
    UINT32 ulRet = 0;

    if (m_ppStreamInfo)
    {
        for (UINT32 i = 0; i < m_ulNumStreams; i++)
        {
            if (m_ppStreamInfo[i] &&
                m_ppStreamInfo[i]->m_pReversalQueue)
            {
                ulRet += m_ppStreamInfo[i]->m_pReversalQueue->GetCount();
            }
        }
    }

    return ulRet;
}

UINT32 CHXPlaybackVelocityProxy::ComputeBackwardSeekTime(UINT32 ulStopTime)
{
    return (ulStopTime > m_ulBackwardsSeekInterval ? ulStopTime - m_ulBackwardsSeekInterval : 0);
}

HX_RESULT CHXPlaybackVelocityProxy::ExternalGetPacket(UINT32 ulStreamNumber)
{
    HXLOGL4(HXLOG_TRIK, "Proxy ExternalGetPacket(%lu)", ulStreamNumber);

    HX_RESULT retVal = HXR_UNEXPECTED;

    // Is this a legal stream number
    if (ulStreamNumber < m_ulNumStreams &&
        m_ppStreamInfo && m_ppStreamInfo[ulStreamNumber])
    {
        // Clear the return value
        retVal = HXR_OK;
        // Get the CHXStreamInfo object
        CHXStreamInfo* pInfo = m_ppStreamInfo[ulStreamNumber];
        // Set the flag saying we got a GetPacket
        // request from the user of the proxy
        pInfo->m_bExternalPacketRequested = TRUE;
        // Are we in non-1x playback
        if (m_lPlaybackVelocity != HX_PLAYBACK_VELOCITY_NORMAL)
        {
            // Get a packet from the retimestamp queue
            retVal = GetPacketReTimeStamp(pInfo);
        }
        else
        {
            // Issue the internal GetPacket
            retVal = _GetPacket(pInfo);
        }
        if (retVal != HXR_OK)
        {
            // Error returned, so don't expect a callback
            pInfo->m_bExternalPacketRequested = FALSE;
        }
    }

    return retVal;
}

HX_RESULT CHXPlaybackVelocityProxy::ExternalSeek(UINT32 ulSeekTime)
{
    HXLOGL4(HXLOG_TRIK, "Proxy ExternalSeek(%lu)", ulSeekTime);

    HX_RESULT retVal = HXR_OK;
    // XXXMEH - This assumes that we always seek everytime
    // we change velocities AND that the SetVelocity has happened
    // BEFORE this seek occurs. If we ever change this assumption,
    // then for reverse playback we will have to add more
    // complicated logic here.
    //
    // Set the flag saying the user of the proxy
    // requested a seek
    m_bExternalSeekRequested = TRUE;
    // Clear any external packet requests, since a seek clears them
    UINT32 i = 0;
    if (m_ppStreamInfo)
    {
        for (i = 0; i < m_ulNumStreams; i++)
        {
            if (m_ppStreamInfo[i])
            {
                m_ppStreamInfo[i]->m_ulLastInitialPacketTime         = ulSeekTime;
                m_ppStreamInfo[i]->m_ulInitialPacketTime             = ulSeekTime;
                m_ppStreamInfo[i]->m_bFirstPacketAfterInternalSeek   = TRUE;
                m_ppStreamInfo[i]->m_bReversalBufferingInProgress    = FALSE;
                m_ppStreamInfo[i]->m_bReversalPacketRequested        = FALSE;
                m_ppStreamInfo[i]->m_bExternalPacketRequested        = FALSE;
                m_ppStreamInfo[i]->m_bReTimeStampPacketRequested     = FALSE;
                m_ppStreamInfo[i]->m_ulLastASMSwitchOnTimeStamp      = 0;
                m_ppStreamInfo[i]->m_ulLastOriginalTimeStamp         = ulSeekTime;
                m_ppStreamInfo[i]->m_ulLastWarpedTimeStamp           = ulSeekTime;
                m_ppStreamInfo[i]->ClearPacketQueue(m_ppStreamInfo[i]->m_pReversalQueue);
            }
        }
    }
    // Reset proxy members
    m_ulExternalSeekTime           = ulSeekTime;
    m_ulLastReverseSeekTime        = ulSeekTime;
    m_bReversalBufferingInProgress = FALSE;
    // Call the base seek
    retVal = _Seek(ulSeekTime);

    return retVal;
}

HX_RESULT CHXPlaybackVelocityProxy::PacketReadyFilterKeyFrame(IHXPacket* pPacket, CHXStreamInfo* pInfo)
{
    HXLOGL4(HXLOG_TRIK, "Proxy PacketReadyFilterKeyFrame() strm=%u pts=%lu rule=%u flags=0x%02x",
            (pPacket ? pPacket->GetStreamNumber() : 0),
            (pPacket ? pPacket->GetTime() : 0),
            (pPacket ? pPacket->GetASMRuleNumber() : 0),
            (pPacket ? pPacket->GetASMFlags() : 0));
    HX_RESULT retVal = HXR_FAIL;

    if (pPacket && pInfo)
    {
        // Is this a video packet? If so, look at it
        // further. If not, then just pass it on.
        if (pInfo->m_bIsVideo)
        {
            // Assume we will pass this packet on
            HXBOOL bPassOn = TRUE;
            // Is this packet a keyframe packet?
            if (IsKeyFramePacket(pPacket, pInfo))
            {
                // This packet IS a keyframe packet, so
                // we will pass this packet on, but we 
                // need to clear the keyframe transition flag.
                // This flag gets set whenever we transition
                // into or out of keyframe-only mode.
                pInfo->m_bKeyFrameModeTransition = FALSE;
            }
            else
            {
                // This is NOT a keyframe packet
                //
                // If we are transitioning INTO keyframe-only mode,
                // we want to keep sending non-keyframe packets
                // until we see the first keyframe. If we are
                // transitioning OUT of keyframe-only mode, then
                // we still need to see a keyframe before we 
                // start sending non-keyframe packets. So we 
                // don't want to send non-keyframe packets
                // if: a) we are in keyframe-only mode and
                // we are NOT transitioning; or b) we are NOT
                // in keyframe-only mode and we ARE transitioning.
                // So that operation is an XOR or keyframe-only mode
                // and the transitioning flag
                if (m_bKeyFrameMode ^ pInfo->m_bKeyFrameModeTransition)
                {
                    // Don't pass this packet on
                    bPassOn = FALSE;
                }
            }
            // Are we supposed to pass this packet on?
            if (bPassOn)
            {
                // Pass it on to the next processing layer
                retVal = PacketReadyReversal(pPacket, pInfo);
            }
            else
            {
                // We are NOT supposed to pass this packet on, 
                // so make another request to the IHXFileFormatObject
                // or the IHXRecordSource for another packet.
                retVal = _GetPacket(pInfo);
            }
        }
        else
        {
            // This is not a video packet, so pass it on
            // to the next processing layer.
            retVal = PacketReadyReversal(pPacket, pInfo);
        }
    }

    return retVal;
}

HXBOOL CHXPlaybackVelocityProxy::IsKeyFramePacket(IHXPacket* pPacket, CHXStreamInfo* pInfo)
{
    HXBOOL bRet = FALSE;

    if (pPacket && pInfo)
    {
        // Determine if this is a keyframe packet
        UINT16 usRule    = pPacket->GetASMRuleNumber();
        BYTE   ucFlags   = pPacket->GetASMFlags();
        HXBOOL   bSwitchOn = (ucFlags & HX_ASM_SWITCH_ON ? TRUE : FALSE);
        // Do we have a RuleToFlagMap object?
        if (pInfo->m_pRuleToFlagMap)
        {
            // Is this a legal rule number?
            if (usRule < pInfo->m_pRuleToFlagMap->num_rules)
            {
                // Get the flags
                UINT16 usRuleFlags = pInfo->m_pRuleToFlagMap->rule_to_flag_map[usRule];
                // Is the keyframe flag set?
                if (usRuleFlags & HX_KEYFRAME_FLAG)
                {
                    // It's a keyframe
                    bRet = TRUE;
                }
            }
        }
        else
        {
            // If we don't have a rule->flag map, then
            // we will look for packets with the 
            // HX_ASM_SWITCH_ON ASM flag. Any packets
            // with this flag set (as well as any packets
            // following this packet which have the same
            // timestamp) are keyframe packets
            //
            // Is the HX_ASM_SWITCH_ON flag set?
            if (bSwitchOn)
            {
                // The HX_ASM_SWITCH_ON flag is set for the first
                // packet which is part of a keyframe. So save
                // the timestamp of this packet
                pInfo->m_ulLastASMSwitchOnTimeStamp = pPacket->GetTime();
            }
            // If the timestamp of this packet is the same
            // as the last HX_ASM_SWITCH_ON packet, then this
            // packet contains all or part of a keyframe
            if (pPacket->GetTime() == pInfo->m_ulLastASMSwitchOnTimeStamp)
            {
                // It's a keyframe
                bRet = TRUE;
            }
        }
    }

    return bRet;
}

HX_RESULT CHXPlaybackVelocityProxy::PacketReadyReversal(IHXPacket* pPacket, CHXStreamInfo* pInfo)
{
    HXLOGL4(HXLOG_TRIK, "Proxy PacketReadyReversal() strm=%u pts=%lu rule=%u flags=0x%02x",
            (pPacket ? pPacket->GetStreamNumber() : 0),
            (pPacket ? pPacket->GetTime() : 0),
            (pPacket ? pPacket->GetASMRuleNumber() : 0),
            (pPacket ? pPacket->GetASMFlags() : 0));
    HX_RESULT retVal = HXR_FAIL;

    if (pPacket && pInfo)
    {
        // Are we in reverse playback?
        if (m_lPlaybackVelocity < 0 && !m_bSourceCanDoReverse)
        {
            // We are in reverse playback. When in reverse playback,
            // we de-couple the GetPacket/PacketReady calls from the
            // proxy to the underlying derived class. Packets returned
            // from internal GetPacket() calls go on into a reversal queue
            // and then external GetPacket() calls are serviced from this
            // reversal queue.
            //
            // Is this the first packet we've received on this stream
            // after a seek?
            if (pInfo->m_bFirstPacketAfterInternalSeek)
            {
                // Clear the flag
                pInfo->m_bFirstPacketAfterInternalSeek = FALSE;
                // Record the timestamp of this packet
                pInfo->m_ulInitialPacketTime = pPacket->GetTime();
                // Print out some debug info
                HXLOGL3(HXLOG_TRIK, "Proxy: REV buffering first packet after internal seek for stream %lu = %lu",
                        pInfo->m_ulStreamNumber, pInfo->m_ulInitialPacketTime);
            }
            // Are we still buffering this stream?
            UINT32 ulPacketTime = pPacket->GetTime();
            if (pInfo->m_bReversalBufferingInProgress)
            {
                // Is the timestamp of this packet less than the last initial
                // time for this stream? If so, then add it to the reversal queue
                if (ulPacketTime < pInfo->m_ulLastInitialPacketTime)
                {
                    // Add this packet to the reversal queue. TRUE
                    // means add it at the head of the queue
                    retVal = AddPacketToQueue(pInfo->m_pReversalQueue, pPacket, TRUE);
                }
                else
                {
                    // Print out some debug info
                    HXLOGL3(HXLOG_TRIK, "Proxy: REV buffering END on stream %lu because ts (%lu) >= m_ulLastInitialPacketTime (%lu)",
                            pInfo->m_ulStreamNumber, ulPacketTime, pInfo->m_ulLastInitialPacketTime);
                    // We have reached a packet that we don't need
                    // to send. So clear the reversal buffering flag
                    pInfo->m_bReversalBufferingInProgress = FALSE;
                }
            }
            // Determine if any non-sparse streams are still reverse buffering
            HXBOOL bAnyBuffering = FALSE;
            for (UINT32 i = 0; i < m_ulNumStreams; i++)
            {
                if (m_ppStreamInfo[i] &&
                    !m_ppStreamInfo[i]->m_bIsSparseStream &&
                    m_ppStreamInfo[i]->m_bReversalBufferingInProgress)
                {
                    bAnyBuffering = TRUE;
                    break;
                }
            }
            // Are any non-sparse streams still buffering?
            if (bAnyBuffering)
            {
                // Some non-sparse streams are still buffering, so we 
                // need to keep asking for packets on this stream, even though
                // we know we won't use any more packets for this stream in
                // the current reversal buffer. This is because many fileformats
                // only return packets in sequential timestamp order between streams
                // (interleaved RealMedia A/V streams, for instance)
                //
                // Internally call GetPacket() again for this stream
                retVal = _GetPacket(pInfo);
            }
            else
            {
                // No non-sparse streams are still buffering.
                // Are we still buffering?
                if (m_bReversalBufferingInProgress)
                {
                    // Print out some debug info
                    HXLOGL3(HXLOG_TRIK, "Proxy: REV buffering begin sending packets");
                    // Clear the flag saying that at least one stream
                    // is still reverse buffering
                    m_bReversalBufferingInProgress = FALSE;
                    // Clear the return value
                    retVal = HXR_OK;
                    // All non-sparse streams are finished reversal buffering.
                    // Therefore send a packet from the reversal queue
                    SendPacketFromAnyReversalQueue();
                }
                else
                {
                    // Print out some debug info
                    HXLOGL3(HXLOG_TRIK, "Proxy: Received REV buffering packet after finished REV buffering - IGNORING");
                }
            }
        }
        else
        {
            // We are in forward playback - just forward
            // the packet on to the next processing layer
            retVal = PacketReadyReTimeStamp(pPacket, pInfo);
        }
    }

    return retVal;
}

HX_RESULT CHXPlaybackVelocityProxy::PacketReadyReTimeStamp(IHXPacket* pPacket, CHXStreamInfo* pInfo)
{
    HXLOGL4(HXLOG_TRIK, "Proxy PacketReadyReTimeStamp() strm=%u pts=%lu rule=%u flags=0x%02x",
            (pPacket ? pPacket->GetStreamNumber() : 0),
            (pPacket ? pPacket->GetTime() : 0),
            (pPacket ? pPacket->GetASMRuleNumber() : 0),
            (pPacket ? pPacket->GetASMFlags() : 0));
    HX_RESULT retVal = HXR_UNEXPECTED;

    if (pPacket && pInfo)
    {
        HX_ASSERT(pInfo->m_bReversalPacketRequested);
        // Clear the reversal packet requested flag for this stream
        pInfo->m_bReversalPacketRequested = FALSE;
        // Are we in keyframe-only mode?
        if (m_bKeyFrameMode && !m_bDisableReTimeStamping && m_pTimeRegulator)
        {
            // Clear the return value
            retVal = HXR_OK;
            // By default, we will send the packet
            HXBOOL bSendPacket = TRUE;
            UINT32 ulOrigTime  = pPacket->GetTime();
            UINT32 ulWarpTime  = ulOrigTime;
            // Get the most recent time warp pair
            UINT32 ulLastOrig  = 0;
            UINT32 ulLastWarp  = 0;
            HXBOOL bHasLast    = FALSE;
            if (SUCCEEDED(m_pTimeRegulator->GetLastTimeWarp(ulLastOrig, ulLastWarp)))
            {
                bHasLast = TRUE;
            }
            // Is this a video packet?
            if (pInfo->m_bIsVideo)
            {
                // Is this a keyframe packet? In keyframe-mode, it will
                // almost certainly be, but there can be some non-keyframe
                // packets in between switching to keyframe-mode and seeing
                // the first keyframe packet.
                HXBOOL bKeyFrame = IsKeyFramePacket(pPacket, pInfo);
                if (bKeyFrame)
                {
                    // Get the last time warp pair that was added
                    // to the time regulator. This should only fail
                    // when we have not yet added any time pairs
                    // to the regulator
                    HXBOOL bAddNewPair = FALSE;
                    // Do we have the last time warp pair?
                    if (bHasLast)
                    {
                        // We got the last warp pair from the regulator
                        // Is this keyframe time later than the last keyframe?
                        if (HX_LATER_USING_VELOCITY(ulOrigTime, ulLastOrig, m_lPlaybackVelocity))
                        {
                            // Compute the time of the next warped keyframe
                            UINT32 ulNextTime = HX_ADD_USING_VELOCITY(ulLastWarp,
                                                                      m_ulFrameInterval,
                                                                      m_lPlaybackVelocity);
                            // Is this packet later or the same as the next warped
                            // keyframe time?
                            if (HX_LATER_OR_SAME_USING_VELOCITY(ulOrigTime, ulNextTime, m_lPlaybackVelocity))
                            {
                                // This will be the next keyframe pair
                                ulWarpTime  = ulNextTime;
                                bAddNewPair = TRUE;
                            }
                            else
                            {
                                // We will drop this keyframe packet
                                HXLOGL3(HXLOG_TRIK, "Proxy: Dropping keyframe %lu, looking for %lu",
                                        ulOrigTime, ulNextTime);
                                bSendPacket = FALSE;
                            }
                        }
                    }
                    else
                    {
                        // No time pairs have been added to the time
                        // regulator yet. Therefore, we will allow the
                        // original time and warped time to remain the
                        // equal to each other, unless we are in reverse
                        // playback.
                        bAddNewPair = TRUE;
                        // If we are in forward playback, then the first timestamp
                        // that we receive will likely be BEFORE the seek time. However,
                        // in reverse playback, the first keyframe time we get may not
                        // be until well before the seek time. If we make the first warped
                        // time be the same as the original time, then this will look
                        // like playback is not beginning until the first keyframe. Therefore,
                        // in reverse playback, we want to assign the first warped
                        // time to be the external seek time. (NOTE THAT THIS ASSUMES
                        // THAT WE WILL ALWAYS BE SEEKING WHEN CHANGING VELOCITY.)
                        if (m_lPlaybackVelocity < 0)
                        {
                            ulWarpTime = m_ulExternalSeekTime;
                        }
                    }
                    // Should we add a new pair to the time regulator?
                    if (bAddNewPair)
                    {
                        // Add the pair
                        m_pTimeRegulator->SetCurrentTimeWarp(ulOrigTime, ulWarpTime);
                        // If we just added a pair, then we can
                        // certainly set the flag saying we don't
                        // have the last pair available
                        bHasLast = TRUE;
                    }
                }
            }
            // Are we supposed to send this packet?
            if (bSendPacket)
            {
                // We are sending this packet. First, we need
                // to get the warped time from the time regulator
                ulWarpTime = m_pTimeRegulator->GetWarpedTime(ulOrigTime);
                // For non-video packets, enforce monotonically
                // inceasing/decreasing timestamps. Video timestamps automatically
                // are monotonically increasing/decreasing by the retimestamping
                // process. If audio packets are seen before any video
                // packets, then it's possible to get out-of-order
                // packets on non-video streams unless we check
                // for monotonically increasing/decreasing timestamps.
                if (!pInfo->m_bIsVideo)
                {
                    // Compute the difference between the current
                    // warped timestamp and the last one for this stream.
                    INT32 lDiff = ulWarpTime - pInfo->m_ulLastWarpedTimeStamp;
                    // If we are in forward playback and this number is negative,
                    // then we have an out-of-timestamp-order packet for this
                    // stream. If we are in reverse playback and this number
                    // is positive, then we have an out-of-timestamp-order
                    // packet for this stream. Values of 0 in both cases
                    // are OK.
                    if ((m_lPlaybackVelocity <  0 && lDiff > 0) ||
                        (m_lPlaybackVelocity >= 0 && lDiff < 0))
                    {
                        // Log this event
                        HXLOGL3(HXLOG_TRIK, "Out-of-timestamp order packet: ts = %lu last = %lu",
                                ulWarpTime, pInfo->m_ulLastWarpedTimeStamp);
                        // Enforce monotonically incrasing/decreasing
                        // timestamps by assiging this packet the last
                        // warped timestamp for this stream.
                        ulWarpTime = pInfo->m_ulLastWarpedTimeStamp;
                    }
                }
                // Re-timestamp the packet and send it
                retVal = ReTimeStampAndSendPacket(pPacket, pInfo, ulWarpTime);
            }
            else
            {
                // We are dropping this packet so request another
                // from the reversal layer
                retVal = GetPacketReversal(pInfo);
            }
        }
        else
        {
            // We are not in keyframe-only mode (or we have turned
            // off re-timestamping), so just pass the packet through.
            // We don't need to re-timestamp.
            retVal = _PacketReady(HXR_OK, pPacket, pInfo);
        }
    }

    return retVal;
}

HX_RESULT CHXPlaybackVelocityProxy::GetPacketReTimeStamp(CHXStreamInfo* pInfo)
{
    HXLOGL4(HXLOG_TRIK, "Proxy GetPacketReTimeStamp() strm=%lu", pInfo->m_ulStreamNumber);
    HX_RESULT retVal = HXR_UNEXPECTED;

    if (pInfo)
    {
        HX_ASSERT(!pInfo->m_bReTimeStampPacketRequested);
        // If we already have an outstanding request
        // on this stream, do nothing.
        if (!pInfo->m_bReTimeStampPacketRequested)
        {
            // Set the flag saying we've requested a re-timestamp packet
            pInfo->m_bReTimeStampPacketRequested = TRUE;
            // Pass on the GetPacket request
            retVal = GetPacketReversal(pInfo);
            if (retVal != HXR_OK)
            {
                // Error returned, don't expect a callback
                pInfo->m_bReTimeStampPacketRequested = FALSE;
            }
        }
    }

    return retVal;
}

HX_RESULT CHXPlaybackVelocityProxy::GetPacketReversal(CHXStreamInfo* pInfo)
{
    HXLOGL4(HXLOG_TRIK, "Proxy GetPacketReversal() strm=%lu", pInfo->m_ulStreamNumber);
    HX_RESULT retVal = HXR_UNEXPECTED;

    if (pInfo)
    {
        HX_ASSERT(!pInfo->m_bReversalPacketRequested);
        // If we already have an outstanding GetPacketReversal
        // request on this stream, then do nothing
        if (!pInfo->m_bReversalPacketRequested)
        {
            // Clear the return value
            retVal = HXR_OK;
            // Mark the flag saying a reversal packet has been
            // requested from this stream
            pInfo->m_bReversalPacketRequested = TRUE;
            // Are we in reverse playback?
            if (m_lPlaybackVelocity < 0 && !m_bSourceCanDoReverse)
            {
                // In reverse playback, we de-couple the GetPacket
                // calls between the fileformat and the proxy. Internal
                // calls to _GetPacket() wind up on the reversal queue,
                // and external GetPacket() calls are serviced from this
                // reversal queue.
                //
                // Are we finished reversal buffering for this stream?
                // If we have not, we'll just fall out of this method
                // with having set the flag saying we have an outstanding
                // request on this stream. Later when we finish buffering,
                // it will be fulfilled at that time.
                if (!pInfo->m_bReversalBufferingInProgress)
                {
                    // Do we have any packets left in the reversal queues?
                    if (GetNumQueuedReversalPackets() > 0)
                    {
                        // Handle re-entrancy. While we call IHXFormatResponse::PacketReady(),
                        // then we may get a call to GetPacket() again. If we do,
                        // then handle it here. This will cause the GetPacket()
                        // request to simply set the m_bReversalPacketRequested flag
                        // above and then exit
                        if (!m_bSendingReversalPackets)
                        {
                            // Set the flag
                            m_bSendingReversalPackets = TRUE;
                            // Fulfill all outstanding requests for packets.
                            // We need the check for !m_bReversalBufferingInProgress
                            // because sending the last packet may cause the
                            // seek in the else clause below. It could be that
                            // a packet could get added back to the reversal
                            // queue in the same call stack, so we need to 
                            // make sure we fall out of the loop in that case.
                            HXBOOL bSent = TRUE;
                            while(bSent && !m_bReversalBufferingInProgress)
                            {
                                bSent = SendPacketFromAnyReversalQueue();
                            }
                            // Clear the flag
                            m_bSendingReversalPackets = FALSE;
                        }
                    }
                    else
                    {
                        // We're out of reversal packets, so we need to seek backwards.
                        // Compute the new seek time
                        m_ulLastReverseSeekTime = ComputeBackwardSeekTime(m_ulLastReverseSeekTime);
                        // Set up the reverse playback state variables
                        if (m_ppStreamInfo)
                        {
                            for (UINT32 i = 0; i < m_ulNumStreams; i++)
                            {
                                if (m_ppStreamInfo[i])
                                {
                                    // m_ulInitialPacketTime is the first packet time
                                    // for this stream that we received after the last
                                    // seek. We assign it now to m_ulLastInitialPacketTime,
                                    // which marks when this packet should stop reverse buffering.
                                    m_ppStreamInfo[i]->m_ulLastInitialPacketTime       = m_ppStreamInfo[i]->m_ulInitialPacketTime;
                                    m_ppStreamInfo[i]->m_ulInitialPacketTime           = 0;
                                    m_ppStreamInfo[i]->m_bFirstPacketAfterInternalSeek = TRUE;
                                    m_ppStreamInfo[i]->m_bReversalBufferingInProgress  = TRUE;
                                    HXLOGL3(HXLOG_TRIK, "Proxy: Begin REV buffering on stream %lu", i);
                                }
                            }
                        }
                        HXLOGL3(HXLOG_TRIK, "Proxy: REV buffering internal seek to %lu", m_ulLastReverseSeekTime);
                        // Set the flag that says that at least one
                        // stream is still in reverse buffering
                        m_bReversalBufferingInProgress = TRUE;
                        // Seek to this new time
                        retVal = _Seek(m_ulLastReverseSeekTime);
                    }
                }
            }
            else
            {
                // Issue the internal GetPacket
                retVal = _GetPacket(pInfo);
            }
            if (retVal != HXR_OK)
            {
                // Error returned, so don't expect callback
                pInfo->m_bReversalPacketRequested = FALSE;
            }
        }
    }

    return retVal;
}

HX_RESULT CHXPlaybackVelocityProxy::GetPacketReversalAllStreams()
{
    HX_RESULT retVal = HXR_FAIL;

    if (m_ppStreamInfo && m_ulNumStreams)
    {
        // Clear the return value
        retVal = HXR_OK;
        // Loop through all the streams, calling
        // GetPacketReversal on each stream.
        UINT32 i = 0;
        for (i = 0; i < m_ulNumStreams && SUCCEEDED(retVal); i++)
        {
            if (!m_ppStreamInfo[i]->m_bReversalPacketRequested)
            {
                // Call GetPacketReversal on this stream
                retVal = GetPacketReversal(m_ppStreamInfo[i]);
            }
        }
    }

    return retVal;
}

void CHXPlaybackVelocityProxy::ReadVelocityPrefs(IUnknown* pContext)
{
    if (pContext)
    {
        IHXPreferences* pPrefs = NULL;
        pContext->QueryInterface(IID_IHXPreferences, (void**) &pPrefs);
        if (pPrefs)
        {
            // See if there is a pref to disable retimestamping
            HXBOOL bDisableRTS = FALSE;
            if (SUCCEEDED(ReadPrefBOOL(pPrefs, VELPROXY_DISABLE_RETIMESTAMPING, bDisableRTS)))
            {
                m_bDisableReTimeStamping = bDisableRTS;
            }
            // Read the backwards seek interval
            UINT32 ulTmp = 0;
            if (SUCCEEDED(ReadPrefUINT32(pPrefs, VELPROXY_BACKWARDS_SEEK_STR, ulTmp)))
            {
                m_ulBackwardsSeekInterval = ulTmp;
            }
            // Read the frame rate preferences
            UINT32 ulNumTiers = 0;
            HX_RESULT rv = ReadPrefUINT32(pPrefs, VELPROXY_NUM_VELOCITY_TIERS_STR, ulNumTiers);
            if (SUCCEEDED(rv) && ulNumTiers)
            {
                if (ulNumTiers > VELPROXY_MAX_NUM_VELOCITY_TIERS)
                {
                    ulNumTiers = VELPROXY_MAX_NUM_VELOCITY_TIERS;
                }
                // Allocate a velocity array
                UINT32* pulVelocity = NULL;
                pulVelocity = new UINT32 [ulNumTiers];
                if (pulVelocity)
                {
                    // NULL out the array
                    memset((void*) pulVelocity, 0, ulNumTiers * sizeof(UINT32));
                    // Allocate a frame rate array
                    UINT32  ulNumFrameRates = ulNumTiers + 1;
                    UINT32* pulFrameRate    = NULL;
                    pulFrameRate = new UINT32 [ulNumFrameRates];
                    if (pulFrameRate)
                    {
                        // NULL out the array
                        memset((void*) pulFrameRate, 0, ulNumFrameRates * sizeof(UINT32));
                        // Read in the velocities
                        rv = ReadPrefUINT32Array(pPrefs, VELPROXY_VELOCITY_BASE_STR, ulNumTiers, pulVelocity);
                        if (SUCCEEDED(rv))
                        {
                            // Read in the frame rates
                            rv = ReadPrefUINT32Array(pPrefs, VELPROXY_FRAME_RATE_BASE_STR, ulNumFrameRates, pulFrameRate);
                            if (SUCCEEDED(rv))
                            {
                                // Sanity check on velocities
                                UINT32 i = 0;
                                for (i = 0; i < ulNumTiers && SUCCEEDED(rv); i++)
                                {
                                    if (pulVelocity[i] == 0                        || // velocity must be > 0
                                        pulVelocity[i] >  HX_PLAYBACK_VELOCITY_MAX || // velocity must be <= max velocity
                                        (i && pulVelocity[i] <= pulVelocity[i-1]))    // velocities must be strictly increasing
                                    {
                                        rv = HXR_FAIL;
                                    }
                                }
                                if (SUCCEEDED(rv))
                                {
                                    // Sanity check on the frame rates
                                    for (i = 0; i < ulNumFrameRates; i++)
                                    {
                                        if (pulFrameRate[i] == 0 ||                     // frame rate must be > 0
                                            pulFrameRate[i] >  VELPROXY_MAX_FRAME_RATE) // frame rate must be <= max frame rate
                                        {
                                            rv = HXR_FAIL;
                                        }
                                    }
                                    if (SUCCEEDED(rv))
                                    {
                                        // Frame rates and velocities look good, so
                                        // assign them to the member variables
                                        HX_VECTOR_DELETE(m_pulPrefVelocity);
                                        HX_VECTOR_DELETE(m_pulPrefFrameRate);
                                        m_ulNumPrefVelocityTiers = ulNumTiers;
                                        m_pulPrefVelocity        = pulVelocity;
                                        m_pulPrefFrameRate       = pulFrameRate;
                                    }
                                }
                            }
                        }
                    }
                    if (FAILED(rv))
                    {
                        HX_VECTOR_DELETE(pulFrameRate);
                    }
                }
                else
                {
                    rv = HXR_OUTOFMEMORY;
                }
                if (FAILED(rv))
                {
                    HX_VECTOR_DELETE(pulVelocity);
                }
            }
        }
        HX_RELEASE(pPrefs);
    }
}

UINT32 CHXPlaybackVelocityProxy::GetFrameRateFromVelocity(INT32 lVelocity, UINT32 ulNumVelocityTiers,
                                                          UINT32* pulVelArr, UINT32* pulFrameRateArr)
{
    UINT32 ulRet = 0;

    if (ulNumVelocityTiers && pulVelArr && pulFrameRateArr)
    {
        // Get the absolute value of the velocity
        UINT32 ulVelocity = (UINT32) (lVelocity < 0 ? -lVelocity : lVelocity);
        // Find the right velocity tier
        //
        // pulVelArr should have ulNumVelocityTiers entries
        // pulFrameRateArr should have ulNumVelocityTiers + 1 entries
        //
        // We choose like this. Let N = ulNumVelocityTiers. If:
        //
        // ulVelocity < pulVelArr[0]                              , return pulFrameRateArr[0]
        // ulVelocity >= pulVelArr[0] && ulVelocity < pulVelArr[1], return pulFrameRateArr[1]
        // ulVelocity >= pulVelArr[1] && ulVelocity < pulVelArr[2], return pulFrameRateArr[2]
        // ...
        // ulVelocity >= pulVelArr[N-1],                            return pulFrameRateArr[N]
        //
        // Now pick the velocity
        if (ulVelocity < pulVelArr[0])
        {
            ulRet = pulFrameRateArr[0];
        }
        else if (ulVelocity >= pulVelArr[ulNumVelocityTiers-1])
        {
            ulRet = pulFrameRateArr[ulNumVelocityTiers];
        }
        else
        {
            for (UINT32 i = 1; i < ulNumVelocityTiers; i++)
            {
                if (ulVelocity >= pulVelArr[i-1] &&
                    ulVelocity <  pulVelArr[i])
                {
                    ulRet = pulFrameRateArr[i];
                    break;
                }
            }
        }
    }

    return ulRet;
}

HX_RESULT CHXPlaybackVelocityProxy::InitFileHeader(IHXValues* pHeader)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pHeader)
    {
        // Clear out any existing array
        ClearStreamInfoArray();
        // Get the stream count
        pHeader->GetPropertyULONG32("StreamCount", m_ulNumStreams);
        // Allocate an array of CHXStreamInfo*
        m_ppStreamInfo = new CHXStreamInfo* [m_ulNumStreams];
        if (m_ppStreamInfo)
        {
            // Clear return value
            retVal = HXR_OK;
            // Create the stream info objects
            for (UINT32 i = 0; i < m_ulNumStreams && SUCCEEDED(retVal); i++)
            {
                m_ppStreamInfo[i] = new CHXStreamInfo(i);
                if (!m_ppStreamInfo[i])
                {
                    retVal = HXR_OUTOFMEMORY;
                }
            }
        }
    }

    return retVal;
}

HX_RESULT CHXPlaybackVelocityProxy::InitStreamHeader(IHXValues* pHeader)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pHeader)
    {
        // Get the stream number
        UINT32 ulStreamNum = 0;
        pHeader->GetPropertyULONG32("StreamNumber", ulStreamNum);
        if (ulStreamNum < m_ulNumStreams &&
            m_ppStreamInfo && m_ppStreamInfo[ulStreamNum])
        {
            // Clear the return value
            retVal = HXR_OK;
            // Determine if it's a video mime type
            IHXBuffer* pMimeTypeStr = NULL;
            pHeader->GetPropertyCString("MimeType", pMimeTypeStr);
            if (pMimeTypeStr)
            {
                const char* pszPrefix   = "video/";
                UINT32      ulPrefixLen = (UINT32) strlen(pszPrefix);
                const char* pszMimeType = (const char*) pMimeTypeStr->GetBuffer();
                if (strlen(pszMimeType) > ulPrefixLen &&
                    !strncasecmp(pszMimeType, pszPrefix, ulPrefixLen))
                {
                    m_ppStreamInfo[ulStreamNum]->m_bIsVideo = TRUE;
                }
                else
                {
                     m_ppStreamInfo[ulStreamNum]->m_bIsVideo = FALSE;
                }
                // Determine if it's a RealVideo mimetype
                if (!strcasecmp(pszMimeType, REALVIDEO_MIME_TYPE) ||
                    !strcasecmp(pszMimeType, REALVIDEO_ENCRYPTED_MIME_TYPE) ||
                    !strcasecmp(pszMimeType, REALVIDEO_MULTIRATE_MIME_TYPE) ||
                    !strcasecmp(pszMimeType, REALVIDEO_ENCRYPTED_MULTIRATE_MIME_TYPE))
                {
                    m_ppStreamInfo[ulStreamNum]->m_bIsRealVideo = TRUE;
                }
                else
                {
                    m_ppStreamInfo[ulStreamNum]->m_bIsRealVideo = FALSE;
                }
                // Determine if it's a sparse stream or not.
                // XXXMEH - for now, we will only call event streams sparse.
                if (!strcasecmp(pszMimeType, REALEVENT_MIME_TYPE) ||
                    !strcasecmp(pszMimeType, REALEVENT_ENCRYPTED_MIME_TYPE) ||
                    !strcasecmp(pszMimeType, REALEVENT_MULTIRATE_MIME_TYPE) ||
                    !strcasecmp(pszMimeType, REALEVENT_ENCRYPTED_MULTIRATE_MIME_TYPE))
                {
                    m_ppStreamInfo[ulStreamNum]->m_bIsSparseStream = TRUE;
                }
                else
                {
                    m_ppStreamInfo[ulStreamNum]->m_bIsSparseStream = FALSE;
                }
            }
            HX_RELEASE(pMimeTypeStr);
            // See if we have an "RMFF 1.0 Flags" property
            IHXBuffer* pFlagBuf = NULL;
            if (SUCCEEDED(pHeader->GetPropertyBuffer(RULE_TO_FLAG_MAP_PROPERTY,pFlagBuf)))
            {
                // Create a RuleToFlagMap object
                HX_DELETE(m_ppStreamInfo[ulStreamNum]->m_pRuleToFlagMap);
                m_ppStreamInfo[ulStreamNum]->m_pRuleToFlagMap = new RuleToFlagMap;
                if (m_ppStreamInfo[ulStreamNum]->m_pRuleToFlagMap)
                {
                    m_ppStreamInfo[ulStreamNum]->m_pRuleToFlagMap->unpack(pFlagBuf->GetBuffer(),
                                                                          pFlagBuf->GetSize());
                }
            }
            HX_RELEASE(pFlagBuf);
            // If we don't yet have the offset handler, try again
            if (!m_pOffsetHandler)
            {
                _GetOffsetHandler(m_pOffsetHandler);
            }
            // See if we should use this offset handler for this
            // stream or not
            if (m_pOffsetHandler)
            {
                // Get our offset handler response interface
                IHXPacketTimeOffsetHandlerResponse* pResp = NULL;
                HX_RESULT rv = QueryInterface(IID_IHXPacketTimeOffsetHandlerResponse, (void**) &pResp);
                if (SUCCEEDED(rv))
                {
                    // Init the offset handler
                    rv = m_pOffsetHandler->Init(pResp, pHeader, m_pContext);
                    if (SUCCEEDED(rv))
                    {
                        m_ppStreamInfo[ulStreamNum]->m_bUseOffsetHandler = TRUE;
                    }
                }
                HX_RELEASE(pResp);
            }
        }
    }

    return retVal;
}

void CHXPlaybackVelocityProxy::SetKeyFrameModeTransitionFlag()
{
    if (m_ppStreamInfo && m_ulNumStreams)
    {
        UINT32 i = 0;
        for (i = 0; i < m_ulNumStreams; i++)
        {
            if (m_ppStreamInfo[i])
            {
                m_ppStreamInfo[i]->m_bKeyFrameModeTransition = TRUE;
            }
        }
    }
}

CHXPlaybackVelocityProxyFF::CHXPlaybackVelocityProxyFF(IUnknown* pContext, IHXFileFormatObject* pFileFormat) :
    CHXPlaybackVelocityProxy(pContext)
{
    m_pFileFormat = pFileFormat;
    if (m_pFileFormat)
    {
        m_pFileFormat->AddRef();
    }
}

CHXPlaybackVelocityProxyFF::~CHXPlaybackVelocityProxyFF()
{
    CHXPlaybackVelocityProxyFF::Close();
}

STDMETHODIMP CHXPlaybackVelocityProxyFF::QueryInterface(REFIID riid, void** ppvObj)
{
    HX_RESULT retVal = HXR_NOINTERFACE;

    if (ppvObj)
    {
        // Set default
        *ppvObj = NULL;
        // Switch on riid
        if (IsEqualIID(riid, IID_IHXFileFormatObject))
        {
            *ppvObj = (IHXFileFormatObject*) this;
            AddRef();
            retVal  = HXR_OK;
        }
        else
        {
            // Try our base class's QueryInterface
            retVal = CHXPlaybackVelocityProxy::QueryInterface(riid, ppvObj);
            if (FAILED(retVal) && m_pFileFormat)
            {
                // Try our file format object's QueryInterface
                retVal = m_pFileFormat->QueryInterface(riid, ppvObj);
            }
        }
    }

    return retVal;
}

STDMETHODIMP_(ULONG32) CHXPlaybackVelocityProxyFF::AddRef()
{
    return CHXPlaybackVelocityProxy::AddRef();
}

STDMETHODIMP_(ULONG32) CHXPlaybackVelocityProxyFF::Release()
{
    return CHXPlaybackVelocityProxy::Release();
}

STDMETHODIMP CHXPlaybackVelocityProxyFF::GetFileFormatInfo(REF(const char**) pFileMimeTypes,
                                                           REF(const char**) pFileExtensions,
                                                           REF(const char**) pFileOpenNames)
{
    return (m_pFileFormat ? m_pFileFormat->GetFileFormatInfo(pFileMimeTypes, pFileExtensions, pFileOpenNames) : HXR_UNEXPECTED);
}

STDMETHODIMP CHXPlaybackVelocityProxyFF::InitFileFormat(IHXRequest*        pRequest,
                                                        IHXFormatResponse* pFormatResponse,
                                                        IHXFileObject*     pFileObject)
{
    HX_RESULT retVal = HXR_UNEXPECTED;

    if (m_pFileFormat)
    {
        retVal = CHXPlaybackVelocityProxy::Init(pFormatResponse);
        if (SUCCEEDED(retVal))
        {
            // Get our own format response interface
            IHXFormatResponse* pResponse = NULL;
            retVal = QueryInterface(IID_IHXFormatResponse, (void**) &pResponse);
            if (SUCCEEDED(retVal))
            {
                // Init the fileformat
                retVal = m_pFileFormat->InitFileFormat(pRequest, pResponse, pFileObject);
            }
            HX_RELEASE(pResponse);
        }
    }

    return retVal;
}

STDMETHODIMP CHXPlaybackVelocityProxyFF::GetFileHeader()
{
    return (m_pFileFormat ? m_pFileFormat->GetFileHeader() : HXR_UNEXPECTED);
}

STDMETHODIMP CHXPlaybackVelocityProxyFF::GetStreamHeader(UINT16 unStreamNumber)
{
    return (m_pFileFormat ? m_pFileFormat->GetStreamHeader(unStreamNumber) : HXR_UNEXPECTED);
}

STDMETHODIMP CHXPlaybackVelocityProxyFF::GetPacket(UINT16 unStreamNumber)
{
    return ExternalGetPacket((UINT32) unStreamNumber);
}

STDMETHODIMP CHXPlaybackVelocityProxyFF::Seek(ULONG32 ulOffset)
{
    return ExternalSeek(ulOffset);
}

STDMETHODIMP CHXPlaybackVelocityProxyFF::Close()
{
    if (m_pFileFormat)
    {
        m_pFileFormat->Close();
    }
    HX_RELEASE(m_pFileFormat);
    return CHXPlaybackVelocityProxy::Close();
}

void CHXPlaybackVelocityProxyFF::_SetVelocity(INT32 lVelocity, HXBOOL bKeyFrameMode, HXBOOL bAutoSwitch)
{
    if (m_pFileFormat)
    {
        IHXPlaybackVelocity* pVel = NULL;
        m_pFileFormat->QueryInterface(IID_IHXPlaybackVelocity, (void**) &pVel);
        if (pVel)
        {
            pVel->SetVelocity(lVelocity, bKeyFrameMode, bAutoSwitch);
        }
        HX_RELEASE(pVel);
    }
}

void CHXPlaybackVelocityProxyFF::_SetKeyFrameMode(HXBOOL bKeyFrameMode)
{
    if (m_pFileFormat)
    {
        IHXPlaybackVelocity* pVel = NULL;
        m_pFileFormat->QueryInterface(IID_IHXPlaybackVelocity, (void**) &pVel);
        if (pVel)
        {
            pVel->SetKeyFrameMode(bKeyFrameMode);
        }
        HX_RELEASE(pVel);
    }
}

HX_RESULT CHXPlaybackVelocityProxyFF::_BackChannelPacketReady(IHXPacket* pPacket)
{
    HX_RESULT retVal = HXR_UNEXPECTED;

    if (m_pFileFormat)
    {
        IHXBackChannel* pBack = NULL;
        retVal = m_pFileFormat->QueryInterface(IID_IHXBackChannel, (void**) &pBack);
        if (SUCCEEDED(retVal))
        {
            retVal = pBack->PacketReady(pPacket);
        }
        HX_RELEASE(pBack);
    }

    return retVal;
}

HX_RESULT CHXPlaybackVelocityProxyFF::_GetOffsetHandler(REF(IHXPacketTimeOffsetHandler*) rpHandler)
{
    HX_RESULT retVal = HXR_FAIL;

    if (m_pFileFormat)
    {
        HX_RELEASE(rpHandler);
        retVal = m_pFileFormat->QueryInterface(IID_IHXPacketTimeOffsetHandler,
                                               (void**) &rpHandler);
    }

    return retVal;
}

HX_RESULT CHXPlaybackVelocityProxyFF::_Seek(UINT32 ulOffset)
{
    HX_RESULT retVal = HXR_UNEXPECTED;

    if (m_pFileFormat)
    {
        // Set the flag saying we are internally requesting a seek
        m_bInternalSeekRequested = TRUE;
        // Clear any internal packet requested flags, since
        // seeks cancel GetPacket() calls
        if (m_ppStreamInfo)
        {
            for (UINT32 i = 0; i < m_ulNumStreams; i++)
            {
                if (m_ppStreamInfo[i])
                {
                    m_ppStreamInfo[i]->m_bInternalPacketRequested = FALSE;
                }
            }
        }
        // Issue the fileformat seek
        m_pFileFormat->Seek(ulOffset);
        // Clear the return value
        retVal = HXR_OK;
    }

    return retVal;
}

HX_RESULT CHXPlaybackVelocityProxyFF::_GetPacket(CHXStreamInfo* pInfo)
{
    HXLOGL4(HXLOG_TRIK, "ProxyFF _GetPacket() strm=%lu", pInfo->m_ulStreamNumber);
    HX_RESULT retVal = HXR_UNEXPECTED;

    if (pInfo && m_pFileFormat)
    {
        // Set the flag saying we got a GetPacket
        // request from the user of the proxy
        pInfo->m_bInternalPacketRequested = TRUE;
        // Call the fileformat GetPacket
        retVal = m_pFileFormat->GetPacket((UINT16) pInfo->m_ulStreamNumber);
        if (retVal != HXR_OK)
        {
            // Error returned, don't expect a callback
            pInfo->m_bInternalPacketRequested = FALSE;
        }
    }

    return retVal;
}

HX_RESULT CHXPlaybackVelocityProxyFF::_QueryVelocityCaps(REF(IHXPlaybackVelocityCaps*) rpCaps)
{
    HX_RESULT retVal = HXR_NOTIMPL;

    if (m_pFileFormat)
    {
        // Assume that the source cannot give packets
        // in reverse
        m_bSourceCanDoReverse = FALSE;
        // Does the fileformat implement IHXPlaybackVelocity?
        IHXPlaybackVelocity* pVel = NULL;
        HX_RESULT rv = m_pFileFormat->QueryInterface(IID_IHXPlaybackVelocity, (void**) &pVel);
        if (SUCCEEDED(rv))
        {
            // Get the capabilities
            IHXPlaybackVelocityCaps* pCaps = NULL;
            rv = pVel->QueryVelocityCaps(pCaps);
            if (SUCCEEDED(rv))
            {
                // See if the fileformat can do reverse playback
                if (pCaps->IsCapable(HX_PLAYBACK_VELOCITY_REVERSE_1X))
                {
                    // Fileformat can do reverse
                    m_bSourceCanDoReverse = TRUE;
                }
            }
            HX_RELEASE(pCaps);
        }
        HX_RELEASE(pVel);
    }

    return retVal;
}

CHXPlaybackVelocityProxyRS::CHXPlaybackVelocityProxyRS(IUnknown* pContext, IHXRecordSource* pRecordSource) :
    CHXPlaybackVelocityProxy(pContext),
    m_pRecordSource(NULL),
    m_bExpectSortedOnPacket(TRUE),
    m_bSeenAnyPackets(FALSE),
    m_ulLastOnPacketTimestamp(0)
{
    m_pRecordSource = pRecordSource;
    if (m_pRecordSource)
    {
        m_pRecordSource->AddRef();
    }
}

CHXPlaybackVelocityProxyRS::~CHXPlaybackVelocityProxyRS()
{
    CHXPlaybackVelocityProxyRS::Close();
}

STDMETHODIMP CHXPlaybackVelocityProxyRS::QueryInterface(REFIID riid, void** ppvObj)
{
    HX_RESULT retVal = HXR_NOINTERFACE;

    if (ppvObj)
    {
        // Set default
        *ppvObj = NULL;
        // Switch on riid
        if (IsEqualIID(riid, IID_IHXRecordSource))
        {
            *ppvObj = (IHXRecordSource*) this;
            AddRef();
            retVal  = HXR_OK;
        }
        else
        {
            // Try our base class's QueryInterface
            retVal = CHXPlaybackVelocityProxy::QueryInterface(riid, ppvObj);
            if (FAILED(retVal) && m_pRecordSource)
            {
                // Try our record source object's QueryInterface
                retVal = m_pRecordSource->QueryInterface(riid, ppvObj);
            }
        }
    }

    return retVal;
}

STDMETHODIMP_(ULONG32) CHXPlaybackVelocityProxyRS::AddRef()
{
    return CHXPlaybackVelocityProxy::AddRef();
}

STDMETHODIMP_(ULONG32) CHXPlaybackVelocityProxyRS::Release()
{
    return CHXPlaybackVelocityProxy::Release();
}

STDMETHODIMP CHXPlaybackVelocityProxyRS::OnFileHeader(IHXValues* pValues)
{
    HX_RESULT retVal = InitFileHeader(pValues);
    if (SUCCEEDED(retVal) && m_pRecordSource)
    {
        retVal = m_pRecordSource->OnFileHeader(pValues);
        if (retVal != HXR_OK)
        {
            // This return value means that this is a record-only
            // record control and that we should not read packets
            // from it. Therefore, the core will send packets to
            // the record control differently, and we should not
            // expect or care whether they are in timestamp-sorted
            // order or not.
            m_bExpectSortedOnPacket = FALSE;
        }
    }

    return retVal;
}

STDMETHODIMP CHXPlaybackVelocityProxyRS::OnStreamHeader(IHXValues* pValues)
{
    HX_RESULT retVal = InitStreamHeader(pValues);
    if (SUCCEEDED(retVal) && m_pRecordSource)
    {
        retVal = m_pRecordSource->OnStreamHeader(pValues);
    }

    return retVal;
}

STDMETHODIMP CHXPlaybackVelocityProxyRS::OnPacket(IHXPacket* pPacket, INT32 nTimeOffset)
{
    HXLOGL4(HXLOG_TRIK, "ProxyRS OnPacket(0x%08x,%ld) strm=%u pts=%lu rule=%u flags=0x%02x",
            pPacket, nTimeOffset,
            (pPacket ? pPacket->GetStreamNumber() : 0),
            (pPacket ? pPacket->GetTime() : 0),
            (pPacket ? pPacket->GetASMRuleNumber() : 0),
            (pPacket ? pPacket->GetASMFlags() : 0));
    if (pPacket)
    {
        // Get this packet's timestamp
        UINT32 ulThisTime = pPacket->GetTime();
        // Have we seen our first packet yet?
        if (m_bSeenAnyPackets)
        {
            // If we see an out-of-order timestamp, log it
            if (ulThisTime < m_ulLastOnPacketTimestamp)
            {
                // Out-of-order timestamp
                HXLOGL3(HXLOG_TRIK, "ProxyRS OnPacket() OUT OF TIMESTAMP ORDER packet (ts=%lu,last=%lu)",
                        ulThisTime, m_ulLastOnPacketTimestamp);
                // If we are expecting sorted timestamps, then assert
                if (m_bExpectSortedOnPacket)
                {
                    HX_ASSERT(ulThisTime >= m_ulLastOnPacketTimestamp);
                }
            }
        }
        // Save the last timestamp
        m_ulLastOnPacketTimestamp = ulThisTime;
        // Set the seen-any-packets flag
        m_bSeenAnyPackets = TRUE;
    }
    return (m_pRecordSource ? m_pRecordSource->OnPacket(pPacket, nTimeOffset) : HXR_UNEXPECTED);
}

STDMETHODIMP CHXPlaybackVelocityProxyRS::OnEndOfPackets()
{
    return (m_pRecordSource ? m_pRecordSource->OnEndOfPackets() : HXR_UNEXPECTED);
}

STDMETHODIMP CHXPlaybackVelocityProxyRS::Flush()
{
    return (m_pRecordSource ? m_pRecordSource->Flush() : HXR_UNEXPECTED);
}

STDMETHODIMP CHXPlaybackVelocityProxyRS::SetFormatResponse(IHXFormatResponse* pFormatResponse)
{
    HX_RESULT retVal = HXR_UNEXPECTED;

    if (m_pRecordSource)
    {
        retVal = CHXPlaybackVelocityProxy::Init(pFormatResponse);
        if (SUCCEEDED(retVal))
        {
            // Get our own format response interface
            IHXFormatResponse* pResponse = NULL;
            retVal = QueryInterface(IID_IHXFormatResponse, (void**) &pResponse);
            if (SUCCEEDED(retVal))
            {
                // Init the fileformat
                retVal = m_pRecordSource->SetFormatResponse(pResponse);
            }
            HX_RELEASE(pResponse);
        }
    }

    return retVal;
}

STDMETHODIMP CHXPlaybackVelocityProxyRS::GetFormatResponse(REF(IHXFormatResponse*) pFormatResponse)
{
    HX_RESULT retVal = HXR_FAIL;

    if (m_pResponse)
    {
        HX_RELEASE(pFormatResponse);
        pFormatResponse = m_pResponse;
        pFormatResponse->AddRef();
        retVal = HXR_OK;
    }

    return retVal;
}

STDMETHODIMP CHXPlaybackVelocityProxyRS::GetFileHeader()
{
    return (m_pRecordSource ? m_pRecordSource->GetFileHeader() : HXR_UNEXPECTED);
}

STDMETHODIMP CHXPlaybackVelocityProxyRS::GetStreamHeader(UINT32 uStreamNumber)
{
    return (m_pRecordSource ? m_pRecordSource->GetStreamHeader(uStreamNumber) : HXR_UNEXPECTED);
}

STDMETHODIMP CHXPlaybackVelocityProxyRS::GetPacket(UINT16 nStreamNumber)
{
    return ExternalGetPacket((UINT32) nStreamNumber);
}

STDMETHODIMP CHXPlaybackVelocityProxyRS::Seek(UINT32 nPosition)
{
    return ExternalSeek(nPosition);
}

STDMETHODIMP CHXPlaybackVelocityProxyRS::Pause()
{
    return (m_pRecordSource ? m_pRecordSource->Pause() : HXR_UNEXPECTED);
}

STDMETHODIMP CHXPlaybackVelocityProxyRS::SetSource(IUnknown* pUnkSource)
{
    return (m_pRecordSource ? m_pRecordSource->SetSource(pUnkSource) : HXR_UNEXPECTED);
}

STDMETHODIMP CHXPlaybackVelocityProxyRS::Close()
{
    HX_RELEASE(m_pRecordSource);
    return CHXPlaybackVelocityProxy::Close();
}

void CHXPlaybackVelocityProxyRS::_SetVelocity(INT32 lVelocity, HXBOOL bKeyFrameMode, HXBOOL bAutoSwitch)
{
    if (m_pRecordSource)
    {
        IHXPlaybackVelocity* pVel = NULL;
        m_pRecordSource->QueryInterface(IID_IHXPlaybackVelocity, (void**) &pVel);
        if (pVel)
        {
            pVel->SetVelocity(lVelocity, bKeyFrameMode, bAutoSwitch);
        }
        HX_RELEASE(pVel);
    }
}

void CHXPlaybackVelocityProxyRS::_SetKeyFrameMode(HXBOOL bKeyFrameMode)
{
    if (m_pRecordSource)
    {
        IHXPlaybackVelocity* pVel = NULL;
        m_pRecordSource->QueryInterface(IID_IHXPlaybackVelocity, (void**) &pVel);
        if (pVel)
        {
            pVel->SetKeyFrameMode(bKeyFrameMode);
        }
        HX_RELEASE(pVel);
    }
}

HX_RESULT CHXPlaybackVelocityProxyRS::_BackChannelPacketReady(IHXPacket* pPacket)
{
    HX_RESULT retVal = HXR_UNEXPECTED;

    if (m_pRecordSource)
    {
        IHXBackChannel* pBack = NULL;
        retVal = m_pRecordSource->QueryInterface(IID_IHXBackChannel, (void**) &pBack);
        if (SUCCEEDED(retVal))
        {
            retVal = pBack->PacketReady(pPacket);
        }
        HX_RELEASE(pBack);
    }

    return retVal;
}

HX_RESULT CHXPlaybackVelocityProxyRS::_GetOffsetHandler(REF(IHXPacketTimeOffsetHandler*) rpHandler)
{
    HX_RESULT retVal = HXR_FAIL;

    if (m_pRecordSource)
    {
        HX_RELEASE(rpHandler);
        retVal = m_pRecordSource->QueryInterface(IID_IHXPacketTimeOffsetHandler,
                                               (void**) &rpHandler);
    }

    return retVal;
}

HX_RESULT CHXPlaybackVelocityProxyRS::_Seek(UINT32 ulOffset)
{
    HX_RESULT retVal = HXR_UNEXPECTED;

    if (m_pRecordSource)
    {
        // Set the flag saying we are internally requesting a seek
        m_bInternalSeekRequested = TRUE;
        // Clear any internal packet requested flags, since
        // seeks cancel GetPacket() calls
        if (m_ppStreamInfo)
        {
            for (UINT32 i = 0; i < m_ulNumStreams; i++)
            {
                if (m_ppStreamInfo[i])
                {
                    m_ppStreamInfo[i]->m_bInternalPacketRequested = FALSE;
                }
            }
        }
        // Issue the fileformat seek
        retVal = m_pRecordSource->Seek(ulOffset);
    }

    return retVal;
}

HX_RESULT CHXPlaybackVelocityProxyRS::_GetPacket(CHXStreamInfo* pInfo)
{
    HXLOGL4(HXLOG_TRIK, "ProxyRS _GetPacket() strm=%lu", pInfo->m_ulStreamNumber);
    HX_RESULT retVal = HXR_UNEXPECTED;

    if (pInfo && m_pRecordSource)
    {
        // Set the flag saying we got a GetPacket
        // request from the user of the proxy
        pInfo->m_bInternalPacketRequested = TRUE;
        // Issue the GetPacket
        retVal = m_pRecordSource->GetPacket((UINT16) pInfo->m_ulStreamNumber);
        if (retVal != HXR_OK)
        {
            // Error returned, so don't expect a callback
            pInfo->m_bInternalPacketRequested = FALSE;
        }
    }

    return retVal;
}

HX_RESULT CHXPlaybackVelocityProxyRS::_QueryVelocityCaps(REF(IHXPlaybackVelocityCaps*) rpCaps)
{
    HX_RESULT retVal = HXR_NOTIMPL;

    if (m_pRecordSource)
    {
        // Assume that the source cannot give packets
        // in reverse
        m_bSourceCanDoReverse = FALSE;
        // Does the fileformat implement IHXPlaybackVelocity?
        IHXPlaybackVelocity* pVel = NULL;
        HX_RESULT rv = m_pRecordSource->QueryInterface(IID_IHXPlaybackVelocity, (void**) &pVel);
        if (SUCCEEDED(rv))
        {
            // Get the capabilities
            IHXPlaybackVelocityCaps* pCaps = NULL;
            rv = pVel->QueryVelocityCaps(pCaps);
            if (SUCCEEDED(rv))
            {
                // See if the fileformat can do reverse playback
                if (pCaps->IsCapable(HX_PLAYBACK_VELOCITY_REVERSE_1X))
                {
                    // Fileformat can do reverse
                    m_bSourceCanDoReverse = TRUE;
                }
            }
            HX_RELEASE(pCaps);
        }
        HX_RELEASE(pVel);
    }

    return retVal;
}

