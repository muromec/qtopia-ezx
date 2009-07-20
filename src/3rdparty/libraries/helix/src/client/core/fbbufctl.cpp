/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: fbbufctl.cpp,v 1.17 2008/03/07 20:33:13 rrajesh Exp $
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

#include "hxtypes.h"
#include "hxcom.h"
#include "fbbufctl.h"
#include "hlxclib/math.h"
#include "hxprefutil.h"
#include "hxtlogutil.h"
#include "ihxrateadaptctl.h"

//#define HELIX_FEATURE_DBG_LOG
//#define BUFFER_CONTROL_TESTING

#include "hxtick.h"
#include "errdbg.h"
#include "hxstring.h"
#include "hxurl.h"
#include "ihxpckts.h" // IHXValues
#include "hxfiles.h" // IHXRequest

HXFeedbackControl::HXFeedbackControl() :
    m_delt(0),
    m_a1(0),
    m_a2(0),
    m_b1(0),
    m_b2(0),
    m_ulSetPoint(0),
    m_ulMin(0),
    m_ulMax(0xffffffff),
    m_e1(0),
    m_e2(0),
    m_c1(0),
    m_c2(0)
{}

void HXFeedbackControl::Init(double c, double wn, double Kv, double delt)
{
    // calculate coefficients
    double d = exp(-wn * c);
    double wd = wn * sqrt(1.0 - c *c);
    double pReal = d * cos(wd);
    double pImag = d * sin(wd);

    m_b1 = -2.0 * pReal;
    m_b2 = pReal * pReal + pImag * pImag;
    m_a1 = 2.0 + m_b1 - (1.0 + m_b1 + m_b2)/(Kv * delt);
    m_a2 = 1.0 + m_b1 + m_b2 - m_a1;

    m_delt = delt;
}
    
void HXFeedbackControl::Reset(UINT32 ulSetPoint, 
                              INT32 e1, INT32 e2,
                              UINT32 c1, UINT32 c2)
{
    m_ulSetPoint = ulSetPoint;
    m_e1 = e1;
    m_e2 = e2;
    m_c1 = c1;
    m_c2 = c2;
}

void HXFeedbackControl::SetLimits(UINT32 ulMin, UINT32 ulMax)
{
    m_ulMin = ulMin;
    m_ulMax = ulMax;
}

UINT32 HXFeedbackControl::Control(UINT32 ulCurrentBits)
{
    INT32 error =  (INT32)m_ulSetPoint - (INT32)ulCurrentBits;
    UINT32 ulControlBw = 0;
    
    double u1 = (m_a1 * (double)error + 
                 (m_a2 - m_a1) * (double)m_e1 -
                 m_a2 * (double)m_e2) / m_delt;
    
    double u2 = -((m_b1 - m_a1) * (double)m_c1 +
                  (m_b2 - m_a2) * (double)m_c2);
    
    ulControlBw = RoundAndClip(u1 + u2);

    Enqueue(error, ulControlBw);

    return ulControlBw;
}

UINT32 HXFeedbackControl::RoundAndClip(double value) const
{
    UINT32 ulRet = m_ulMin;

    if (value > m_ulMax)
    {
        // Saturate to max
        ulRet = m_ulMax;
    }
    else if (value > m_ulMin)
    {
        // Value within range.
        // Round to nearest integer
        ulRet = (UINT32)(value + 0.5);
    }

    return ulRet;
}

void HXFeedbackControl::Enqueue(INT32 error, UINT32 ulBandwidth)
{
    m_e2 = m_e1;
    m_e1 = error;

    m_c2 = m_c1;
    m_c1 = ulBandwidth;
}

HXFeedbackBufferControl::HXFeedbackBufferControl() :
    m_lRefCount(0),
    m_pScheduler(NULL),
    m_callbackHandle(0),
    m_pSource(NULL),
    m_pBufferStats(NULL),
    m_pThin(NULL),
    m_pPrefs(NULL),
    m_pStatus(NULL),
    m_pErrMsg(NULL),
    m_ulClipBw(0),
    m_nControlStream(0),
    m_ulControlBw(0),
    m_bControlTotal(FALSE),
    m_bPaused(TRUE),
    m_state(csNotInitialized),
    m_pRateAdaptCtl(NULL)
{
    m_control.Init(0.7, 1.25664, 0.2, 5.0);
}

HXFeedbackBufferControl::~HXFeedbackBufferControl()
{
    Close();
}

/*
 *      IUnknown methods
 */
STDMETHODIMP HXFeedbackBufferControl::QueryInterface(THIS_
                                                     REFIID riid,
                                                     void** ppvObj)
{
    QInterfaceList qiList[] =
    {
        { GET_IIDHANDLE(IID_IUnknown), (IUnknown*)(IHXBufferControl*)this }, 
        { GET_IIDHANDLE(IID_IHXBufferControl), (IHXBufferControl*)this },
        { GET_IIDHANDLE(IID_IHXCallback), (IHXCallback*)this },
    };
    
    return ::QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
}

STDMETHODIMP_(ULONG32) HXFeedbackBufferControl::AddRef(THIS)
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32) HXFeedbackBufferControl::Release(THIS)
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

/*
 * IHXBufferControl method
 */

/************************************************************************
 *      Method:
 *          IHXBufferControl::Init
 *      Purpose:
 *      Initialize the buffer control object with a context
 *      so it can find the interfaces it needs to do buffer
 *      control
 */
STDMETHODIMP HXFeedbackBufferControl::Init(THIS_ IUnknown* pContext)
{
    HX_RESULT res = HXR_FAILED;
    
    Close();

    if (pContext)
    {
        pContext->QueryInterface(IID_IHXErrorMessages, (void**)&m_pErrMsg);

        if ((HXR_OK == pContext->QueryInterface(IID_IHXScheduler, 
                                                (void**)&m_pScheduler)) &&
            (HXR_OK == pContext->QueryInterface(IID_IHXStreamSource, 
                                                (void**)&m_pSource)) &&
            (m_pSource) && 
            (HXR_OK == m_pSource->QueryInterface(IID_IHXThinnableSource, 
                                                 (void**)&m_pThin)) &&
            (HXR_OK == pContext->QueryInterface(IID_IHXSourceBufferingStats3,
                                                (void**)&m_pBufferStats)) &&
            (HXR_OK == pContext->QueryInterface(IID_IHXPendingStatus, 
                                                (void**)&m_pStatus)) &&
            (HXR_OK == pContext->QueryInterface(IID_IHXPreferences, 
                                                (void**)&m_pPrefs)) &&
            (HXR_OK == ReadPrefSettings()) &&
            (HXR_OK ==  pContext->QueryInterface(IID_IHXClientRateAdaptControl,
                                           (void**)&m_pRateAdaptCtl))

            )

        {
#ifdef BUFFER_CONTROL_TESTING
            res = InitTesting(pContext);
#endif
            res = HXR_OK;
        }
        else
        {
            ChangeState(csError);
        }
    }
    return res;
}

/************************************************************************
 *      Method:
 *          IHXBufferControl::OnBuffering
 *      Purpose:
 *          Called while buffering
 */
STDMETHODIMP HXFeedbackBufferControl::OnBuffering(UINT32 ulRemainingInMs,
                                                  UINT32 ulRemainingInBytes)
{
    HXLOGL2(HXLOG_BUFF, "HXFeedbackBufferControl::OnBuffering(%lu,%lu)",
            ulRemainingInMs, ulRemainingInBytes);
    
    if (Initialized() &&
        (csBuffering != m_state))
    {
        ChangeState(csBuffering);

        SetBandwidth(m_ulClipBw);
    }
    
    return HXR_OK;
}

/************************************************************************
 *      Method:
 *          IHXBufferControl::OnBufferingDone
 *      Purpose:
 *      Called when buffering is done
 */
STDMETHODIMP HXFeedbackBufferControl::OnBufferingDone(THIS)
{
    HXLOGL1(HXLOG_BUFF, "HXFeedbackBufferControl::OnBufferingDone()");

    if (Initialized())
    {
        HX_ASSERT((csBuffering == m_state) || (csClipEnd == m_state));

        UINT32 ulTotalBits = 0;
        UINT32 ulControlBits = 0;

        GetControlData(ulTotalBits, ulControlBits);

        HXLOGL1(HXLOG_BUFF, "HXFeedbackBufferControl::OnBufferingDone() %lu %lu",
                ulTotalBits, ulControlBits);

        UINT32 ulSetPoint = ulControlBits;

        if (ulSetPoint < m_control.SetPoint())
        {
            // If the new setpoint is less than a 
            // previous value, ignore it and use
            // the old setpoint.
            ulSetPoint = m_control.SetPoint();
        }

        HXLOGL1(HXLOG_BUFF, "HXFeedbackBufferControl::OBD reset control %lu", ulSetPoint);

        m_control.Reset(ulSetPoint,
                        0, 0,                          // Error state
                        m_ulControlBw, m_ulControlBw); // control state

        if (m_bPaused)
        {
            ChangeState(csPaused);
        }
        else
        {
            ChangeState(csPlaying);
        }
    }

    return HXR_OK;
}

/************************************************************************
 *      Method:
 *          IHXBufferControl::OnResume
 *      Purpose:
 *          Called when playback is resumed
 */
STDMETHODIMP HXFeedbackBufferControl::OnResume()
{
    HXLOGL1(HXLOG_BUFF, "HXFeedbackBufferControl::OnResume");

    m_bPaused = FALSE;

    if (csError != m_state)
    {
        if (HXR_OK == GetBwInfo(m_ulClipBw, m_nControlStream, m_ulControlBw))
        {
            HXLOGL1(HXLOG_BUFF, "HXFeedbackBufferControl::OnResume : clipBw %lu ctlStr %u ctlBw %lu",
                    m_ulClipBw, m_nControlStream, m_ulControlBw);

            SetTransportByteLimits(m_ulClipBw);

            if (m_pSource && !m_pSource->IsLive() && 
                m_ulClipBw && m_ulControlBw)
            {
                /* Only activate the algorithm for on-demand clips that
                 * have bitrate information
                 */
                if (csNotInitialized == m_state)
                {
                    StartCallback();
                }
            
                ChangeState(csPlaying);
            }

            SetBandwidth(m_ulClipBw);
        }
    }

    return HXR_OK;
}
    
/************************************************************************
 *      Method:
 *          IHXBufferControl::OnPause
 *      Purpose:
 *          Called when playback is paused
 */
STDMETHODIMP HXFeedbackBufferControl::OnPause()
{
    HXLOGL1(HXLOG_BUFF, "HXFeedbackBufferControl::OnPause");
    
    m_bPaused = TRUE;

    if (Initialized())
    {
        ChangeState(csPaused);
    }

    return HXR_OK;
}

/************************************************************************
 *      Method:
 *          IHXBufferControl::OnSeek
 *      Purpose:
 *          Called when a seek occurs
 */
STDMETHODIMP HXFeedbackBufferControl::OnSeek(THIS)
{
    HXLOGL1(HXLOG_BUFF, "HXFeedbackBufferControl::OnSeek");

    if (Initialized())
    {
        ChangeState(csSeeking);
    }

    return HXR_OK;
}

/************************************************************************
 *      Method:
 *          IHXBufferControl::OnClipEnd
 *      Purpose:
 *          Called when we get the last packet in the clip
 */
STDMETHODIMP HXFeedbackBufferControl::OnClipEnd(THIS)
{
    HXLOGL1(HXLOG_BUFF, "HXFeedbackBufferControl::OnClipEnd");

    if (Initialized())
    {
        ChangeState(csClipEnd);
    }

    return HXR_OK;
}

/************************************************************************
 *      Method:
 *          IHXBufferControl::Close()
 *      Purpose:
 *      Called when the owner of this object wishes to shutdown
 *      and destroy this object. This call causes the buffer control
 *      object to release all it's interfaces references.
 */
STDMETHODIMP HXFeedbackBufferControl::Close(THIS)
{
    HX_RELEASE(m_pErrMsg);

    StopCallback();
    HX_RELEASE(m_pScheduler);

    HX_RELEASE(m_pBufferStats);
    HX_RELEASE(m_pSource);
    HX_RELEASE(m_pThin);
    HX_RELEASE(m_pPrefs);
    HX_RELEASE(m_pStatus);
    HX_RELEASE(m_pRateAdaptCtl);

    ChangeState(csNotInitialized);

    return HXR_OK;
}

/************************************************************************
 *      Method:
 *          IHXCallback::Func
 *      Purpose:
 *          This is the function that will be called when a callback is
 *          to be executed.
 */
STDMETHODIMP HXFeedbackBufferControl::Func(THIS)
{
    HXLOGL3(HXLOG_BUFF, "HXFeedbackBufferControl::Func");

    if (Initialized())
    {
        if (csPlaying == m_state)
        {
            UINT32 ulTotalBits = 0;
            UINT32 ulControlBits = 0;
            UINT32 ulNewBandwidth = 0;
            
            GetControlData(ulTotalBits, ulControlBits);
            
            if (HXR_OK == Control(ulControlBits, ulNewBandwidth))
            {
                SetBandwidth(ulNewBandwidth);
            }
        }
        else if (csBuffering == m_state)
        {
            // Bandwidth used during buffering and seeking
            SetBandwidth(m_ulClipBw);
        }

        ScheduleCallback();
    }

    return HXR_OK;
}


void HXFeedbackBufferControl::ChangeState(ControlState newState)
{
    if (csError != m_state)
    {
        HXLOGL1(HXLOG_BUFF, "HXFeedbackBufferControl::ChangeState: %d -> %d", 
                m_state, newState);
        m_state = newState;
    }
    else
    {
        HXLOGL1(HXLOG_BUFF, "HXFeedbackBufferControl::ChangeState : Tried to leave error state");
        HX_ASSERT(csError == m_state);
    }
}

void HXFeedbackBufferControl::ScheduleCallback()
{
    if (m_pScheduler)
    {
        UINT32 uDelayInMs = (UINT32)(m_control.SamplePeriod() * 1000);
        m_lastTime.tv_sec += uDelayInMs / 1000;
        m_lastTime.tv_usec += (uDelayInMs * 1000) * 1000;
        
        m_callbackHandle = m_pScheduler->AbsoluteEnter(this, m_lastTime);
    }
}

void HXFeedbackBufferControl::StartCallback()
{
    if (m_pScheduler)
    {
        m_lastTime = m_pScheduler->GetCurrentSchedulerTime();
        
        ScheduleCallback();
    }
}

void HXFeedbackBufferControl::StopCallback()
{
    if (m_pScheduler && m_callbackHandle)
    {
        m_pScheduler->Remove(m_callbackHandle);
        m_callbackHandle = 0;
    }
}

HX_RESULT HXFeedbackBufferControl::GetBwInfo(REF(UINT32) ulClipBw,
                                             REF(UINT16) nControlStream, 
                                             REF(UINT32) ulControlBw)
{
    HX_RESULT res = HXR_OK;

    if (!m_pSource)
    {
        res = HXR_FAILED;
    }
    else if (m_pSource->GetStreamCount() == 0)
    {
        res = HXR_NO_DATA;
    }
    else
    {
        ulClipBw = 0;
        nControlStream = 0;
        ulControlBw = 0;
        
        UINT32 ulHighestBw = 0;
        UINT16 nHighestBwStream = 0;

        HXBOOL bFoundAudio = FALSE;
        UINT32 ulLowAudioBw = 0;
        UINT16 nLowestBwAudioStream = 0;

        for (UINT16 i = 0; (HXR_OK == res) && (i < m_pSource->GetStreamCount()); i++)
        {
            UINT32 ulStreamBw = 0;
            if (HXR_OK == GetStreamBw(i, ulStreamBw))
            {
                if (ulStreamBw > ulHighestBw)
                {
                    ulHighestBw = ulStreamBw;
                    nHighestBwStream = i;
                }

                if (IsAudioStream(i) &&
                    (!bFoundAudio || (ulStreamBw < ulLowAudioBw)))
                {
                    bFoundAudio = TRUE;
                    ulLowAudioBw = ulStreamBw;
                    nLowestBwAudioStream = i;
                }

                ulClipBw += ulStreamBw;
            }
        }

        if (bFoundAudio)
        {
            nControlStream = nLowestBwAudioStream;
            ulControlBw = ulLowAudioBw;
        }
        else
        {
            nControlStream = nHighestBwStream;
            ulControlBw = ulHighestBw;
        }
    }

    return res;
}

HX_RESULT 
HXFeedbackBufferControl::GetStreamBw(UINT16 uStreamNumber, 
                                     REF(UINT32) ulBandwidth)
{
    HX_RESULT res = HXR_FAILED;

    if (m_pSource)
    {
        IUnknown* pUnk = NULL;
        res = m_pSource->GetStream(uStreamNumber, pUnk);
        
        if (HXR_OK == res)
        {
            IHXASMProps* pProps = NULL;
            
            res = pUnk->QueryInterface(IID_IHXASMProps, (void**)&pProps);
            if (HXR_OK == res)
            {
                res = pProps->GetBandwidth(ulBandwidth);
                
                pProps->Release();
            }
            pUnk->Release();
        }
    }

    return res;
}

HXBOOL HXFeedbackBufferControl::IsAudioStream(UINT16 uStreamNumber)
{
    HXBOOL bRet = FALSE;

    if (m_pSource)
    {
        IUnknown* pUnk = NULL;
        IHXStream* pStream = NULL;
        IHXValues* pHeader = NULL;
        IHXBuffer* pMimetype = NULL;
        if ((HXR_OK == m_pSource->GetStream(uStreamNumber, pUnk)) &&
            (HXR_OK == pUnk->QueryInterface(IID_IHXStream, (void**)&pStream))&&
            (NULL != (pHeader = pStream->GetHeader())) &&
            (HXR_OK == pHeader->GetPropertyCString("MimeType", pMimetype)) &&
            (!strncasecmp("audio", (char*)pMimetype->GetBuffer(), 5)))
        {
            bRet = TRUE;
        }
        HX_RELEASE(pMimetype);
        HX_RELEASE(pHeader);
        HX_RELEASE(pStream);
        HX_RELEASE(pUnk);
    }

    return bRet;
}

void HXFeedbackBufferControl::GetControlData(REF(UINT32) ulTotalBits,
                                             REF(UINT32) ulControlBits)
{
    ulTotalBits = 0;
    ulControlBits = 0;
    
    for (UINT16 i = 0; i < m_pSource->GetStreamCount(); i++)
    {
        UINT32 ulBytes;
        
        HX_RESULT res = HXR_FAILED;

        UINT32 ulLowTS;
        UINT32 ulHighTS;
        HXBOOL bDone;

        res = m_pBufferStats->GetTotalBuffering(i, 
                                                ulLowTS, ulHighTS,
                                                ulBytes,
                                                bDone);

        if (HXR_OK == res)
        {
            ulTotalBits += 8 * ulBytes;
            
            if (i == m_nControlStream)
            {
                ulControlBits = 8 * ulBytes;
            }
        }
    }
}


HX_RESULT HXFeedbackBufferControl::Control(UINT32 ulControlBits, 
                                           REF(UINT32) ulNewBandwidth)
{
    UINT32 ulSetPoint = m_control.SetPoint();
    INT32 error = (INT32)ulSetPoint - (INT32)ulControlBits;
    
    double errorFactor = error;

    ulNewBandwidth = m_control.Control(ulControlBits);

    ulNewBandwidth = ControlToTotal(ulNewBandwidth);

    if (ulSetPoint)
    {
        errorFactor = (double)error / (double)ulSetPoint;
    }

    HXLOGL2(HXLOG_BUFF, "HXFeedbackBufferControl::Control %d %lu %lu %2.4f",
            error, ulNewBandwidth, ulControlBits, errorFactor);
    
    return HXR_OK;
}

void HXFeedbackBufferControl::SetBandwidth(UINT32 ulBandwidth)
{
    HXLOGL1(HXLOG_BUFF, "HXFeedbackBufferControl::SetBandwidth %lu", ulBandwidth);

    if (m_pThin)
    {
        HX_RESULT rv;
        rv = m_pThin->SetDeliveryBandwidth(ulBandwidth, 0);
        if(rv == HXR_NOT_SUPPORTED)
        {
            // Not supported is returned by protocol layer when
            // RateAdaptation is used or when server does not support
            // SetDeliveryBandwidth
            HXLOGL1(HXLOG_BUFF, "HXFeedbackBufferControl::SetBandwidth SDB NOT SUPPORTED Stopping FeedbackControl");

            StopCallback();
        }
    }
}

HX_RESULT HXFeedbackBufferControl::ReadPrefSettings()
{
    HX_RESULT res = HXR_FAILED;
    
    UINT32 ulMin = 1000;
    UINT32 ulMax = 0;
    ReadPrefUINT32(m_pPrefs, "MinDelBandwidth", ulMin);

    /* Get initial bandwidth guess from Prefs */
    if (HXR_OK == ReadPrefUINT32(m_pPrefs, "Bandwidth", ulMax))
    {
        UINT32 ulSustainFactor = 0;
        /* See if we have a sustainable bandwidth factor */
        if (HXR_OK == ReadPrefUINT32(m_pPrefs, "SustainableBwFactor", 
                                    ulSustainFactor))
        {
            /* clamp value to 0 <= ulTemp <= 100 range */
            if (ulSustainFactor > 100)
            {
                ulSustainFactor = 100;
            }
            
            /* Apply factor */
            ulMax = (((ulMax / 100) * ulSustainFactor) + 
                     (((ulMax % 100) * ulSustainFactor) / 100));
        }

        if (ulMax > 0)
        {
            if (ulMin > ulMax)
            {
                // Make the minimum bandwidh 1% of the max
                // rounded up to the next bit/second
                ulMin = (ulMax + 99) / 100;
            }
            
            m_control.SetLimits(ulMin, ulMax);
            
            res = HXR_OK;
        }
    }

    return res;
}

void HXFeedbackBufferControl::SetTransportByteLimits(UINT32 ulClipBw)
{
    UINT32 ulTotalByteLimit = 0;
    IHXTransportBufferLimit* pBufLimit = NULL;

    ReadPrefUINT32(m_pPrefs, "TransportByteLimit", ulTotalByteLimit);

    if (m_pSource && 
        (HXR_OK == m_pSource->QueryInterface(IID_IHXTransportBufferLimit,
                                             (void**)&pBufLimit)))
    {
        // Divide the byte limit amoung the streams

        for (UINT16 i = 0; i < m_pSource->GetStreamCount(); i++)
        {
            UINT32 ulStreamBw = 0;

            if (HXR_OK == GetStreamBw(i, ulStreamBw))
            {
                // Byte limit for this stream is
                // (ulTotalByteLimit * ulStreamBw) / ulClipBw
                UINT32 ulByteLimit = MulDiv(ulTotalByteLimit,
                                            ulStreamBw, ulClipBw);
                
                if ((ulByteLimit == 0) &&
                    (ulTotalByteLimit != 0))
                {
                    // Make sure all streams have a byte limit
                    // if ulTotalByteLimit is non-zero. This is done
                    // because a byte limit of 0 means unlimited
                    // buffering is allowed

                    ulByteLimit = 1;
                }

                HXBOOL bEnabled = TRUE;
                m_pRateAdaptCtl->IsEnabled(i, bEnabled);

                if (bEnabled)
                {
                  pBufLimit->SetByteLimit(i, ulByteLimit);
                }
            }
        }
    }

    HX_RELEASE(pBufLimit);
}

HX_RESULT HXFeedbackBufferControl::InitTesting(IUnknown* pContext)
{
    HX_RESULT res = HXR_OK;

    IHXRequest* pRequest = NULL;
    const char* pURL = 0;
    if (HXR_OK == pContext->QueryInterface(IID_IHXRequest, 
                                           (void**)&pRequest) &&
        HXR_OK == pRequest->GetURL(pURL))
    {
        CHXURL url(pURL, pContext);
        
        HXLOGL1(HXLOG_BUFF, "HXFeedbackBufferControl : url %s", pURL);
        
        IHXValues* pOptions = url.GetOptions();
        ULONG32 ulTmp;
        
        if (pOptions)
        {
            if (HXR_OK == pOptions->GetPropertyULONG32("FBBC-total_control",
                                                       ulTmp))
            {
                HXLOGL1(HXLOG_BUFF, "HXFeedbackBufferControl : total control %lu", ulTmp);
                m_bControlTotal = (ulTmp > 0) ? TRUE : FALSE;
            }
        }
        
        HX_RELEASE(pOptions);
        HX_RELEASE(pRequest);
    }

    return res;
}

HXBOOL HXFeedbackBufferControl::IsBuffering()
{
    HXBOOL bRet = FALSE;

    IHXBuffer* pStatusDesc = NULL;
    UINT16 unStatusCode    = 0;
    UINT16 unPercentDone   = 0;
    if (m_pStatus &&
        (HXR_OK == m_pStatus->GetStatus(unStatusCode, 
                                        pStatusDesc, unPercentDone)) &&
        (unStatusCode == HX_STATUS_BUFFERING))
    {
        bRet = TRUE;
    }
    HX_RELEASE(pStatusDesc);

    return bRet;
}

UINT32 HXFeedbackBufferControl::ClampBandwidth(UINT32 ulBandwidth) const
{
    UINT32 ulRet = ulBandwidth;

    if (ulRet < m_control.Min())
    {
        ulRet = m_control.Min();
    }
    else if (ulRet > m_control.Max())
    {
        ulRet = m_control.Max();
    }

    return ulRet;
}

UINT32 HXFeedbackBufferControl::ControlToTotal(UINT32 ulValue)
{
    return MulDiv(ulValue, m_ulClipBw, m_ulControlBw);
}

UINT32 HXFeedbackBufferControl::TotalToControl(UINT32 ulValue)
{
    return MulDiv(ulValue, m_ulControlBw, m_ulClipBw);
}

UINT32 HXFeedbackBufferControl::MulDiv(UINT32 ulValue, 
                                       UINT32 ulMult, UINT32 ulDiv)
{
    INT64 temp = (CAST_TO_INT64(ulValue) * CAST_TO_INT64(ulMult))/CAST_TO_INT64(ulDiv);
    ULONG32 result = INT64_TO_ULONG32(temp);
    if( temp > MAX_UINT32 )
    {
        HX_ASSERT("Overflow in MulDiv"==NULL);
        result = MAX_UINT32;
    }
    return result;
}
