/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: rateadaptinfo.cpp,v 1.23 2008/05/01 20:36:41 rrajesh Exp $
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
#include "rateadaptinfo.h"
#include "hxasm.h"
#include "hxcore.h"
#include "hxcom.h"
#include "hxengin.h"
#include "ihxrateadaptctl.h"
#include "ihxpckts.h"
#include "hxccf.h"
#include "ihx3gpp.h"
#include "hxprefs.h"
#include "hxtlogutil.h"
#include "hxprefutil.h"
#include "pckunpck.h"
#include "3gpadapthdr.h"
#include "helixadapthdr.h"
#include "hxbufctl.h" // IHXTransportBufferLimit

static const char z_3GPPAdaptationHdr[]  = "3GPP-Adaptation";
static const char z_HelixAdaptationHdr[] = "Helix-Adaptation";


typedef enum {
    ahtUnknown,
    aht3GPP,
    ahtHelix
} AdaptationHeaderType;

class HXRAIStreamInfo
{
public:
    HXRAIStreamInfo();
    ~HXRAIStreamInfo();

    HX_RESULT Init(AdaptationHeaderType type,
                   UINT16 uStreamNumber,
                   UINT32 ulReportFreq,
                   IHXValues* pHdr,
                   IUnknown* pContext);
    
    AdaptationHeaderType Type() const { return m_type;}
    const char* HeaderName() const;
    UINT16 StreamNumber() const {return m_uStreamNumber;}
    UINT32 Get3gpBufferSize() const;
    UINT32 Get3gpTargetTime() const;
    UINT32 Get3gpReportFreq() const;

private:
    AdaptationHeaderType m_type;
    IUnknown*  m_pContext;
    IHXValues* m_pStreamHdr;
    UINT16 m_uStreamNumber;
    UINT32 m_ulBufferSize;
    mutable UINT32 m_ulTargetTime;
    UINT32 m_ulBaseTargetTime;
    HXBOOL m_bTargetTimeInPrefs;
    UINT32 m_ulReportFreq;
};

HXRAIStreamInfo::HXRAIStreamInfo() :
    m_type(ahtUnknown),
    m_pContext(NULL),
    m_pStreamHdr(0),
    m_uStreamNumber(0),
    m_ulBufferSize(0),
    m_ulTargetTime(0),
    m_ulBaseTargetTime(0),
    m_bTargetTimeInPrefs(FALSE),
    m_ulReportFreq(0)
{
}

HXRAIStreamInfo::~HXRAIStreamInfo()
{
    HX_RELEASE(m_pContext);
    HX_RELEASE(m_pStreamHdr);
}

inline
UINT32 HXMulDiv(UINT32 left, UINT32 right, UINT32 bottom)
{
    double res = double(left) * right / bottom;
    if (res > MAX_UINT32)
    {
        HX_ASSERT(FALSE); // overflow
        return MAX_UINT32;
    }
    return UINT32(res);
}

static 
UINT32 GetConnectionBW(IUnknown* pContext)
{
    HX_ASSERT(pContext);
    UINT32 unConnectBW = 0;
    IHXConnectionBWInfo* pConnBWInfo = NULL;
    if (HXR_OK == pContext->QueryInterface(IID_IHXConnectionBWInfo, (void**)&pConnBWInfo))
    {
        pConnBWInfo->GetConnectionBW(unConnectBW, FALSE);
    }
    else
    {
        IHXPreferences* pPrefs = NULL;
        pContext->QueryInterface(IID_IHXPreferences, (void**)&pPrefs);
        ReadPrefUINT32(pPrefs, "Bandwidth", unConnectBW);
        HX_RELEASE(pPrefs);
    }
    HX_RELEASE(pConnBWInfo);

    HX_ASSERT(unConnectBW);
    return unConnectBW;
}

HX_RESULT 
HXRAIStreamInfo::Init(AdaptationHeaderType type,
                      UINT16 uStreamNumber,
                      UINT32 ulReportFreq,
                      IHXValues* pHdr,
                      IUnknown* pContext)
{
    HX_ASSERT(pContext);
    HX_ASSERT(pHdr);
    m_pContext = pContext;
    HX_ADDREF(m_pContext);

    m_pStreamHdr = pHdr;
    HX_ADDREF(m_pStreamHdr);

    const UINT32 BITS_PER_BYTE  = 8;
    const UINT32 KByte          = 1024;
    const UINT32 MByte          = KByte * KByte;

    IHXPreferences* pPrefs = NULL;
    pContext->QueryInterface(IID_IHXPreferences,(void**)&pPrefs);

    ULONG32 ulAvgBitRate    = 0;
    ULONG32 ulPreroll       = 0;
    ULONG32 ulMaxBitRate    = 0;
    HXBOOL    bIsLive         = FALSE;
    pHdr->GetPropertyULONG32("AvgBitRate",ulAvgBitRate);
    pHdr->GetPropertyULONG32("MaxBitRate", ulMaxBitRate);
    if (0 == ulMaxBitRate)
    {
        ulMaxBitRate = ulAvgBitRate;
    } 
    else if (0 == ulAvgBitRate)
    {
        HX_ASSERT(FALSE); // abnormal case
        ulAvgBitRate = ulMaxBitRate;
    }

    if (0 == ulAvgBitRate)
    {
        HX_ASSERT(FALSE); // abnormal case
        ulAvgBitRate = GetConnectionBW(pContext);
        ulMaxBitRate = ulAvgBitRate;
        if (0 == ulAvgBitRate)
        {
            // Use arbitrary 1M default
            ulAvgBitRate = 1 * MByte;
            ulMaxBitRate = ulAvgBitRate;
        }
    }

    pHdr->GetPropertyULONG32("Preroll", ulPreroll);
    if (0 == ulPreroll)
    {
        // Use default 1 sec preroll per 3gpp
        ulPreroll = 1000;
    }


    ULONG32 ulTemp = 0;
    pHdr->GetPropertyULONG32("LiveStream", ulTemp);
    bIsLive = (ulTemp != 0);

    // Compute target time and buffer size.
    //
    // The "size" parameter is used to specify a limit on the amount of data the server should send
    // to the the client beyond that which it would otherwise need to send to deliver at the stream
    // bitrate. It is the total reception capacity of client.
    // 
    // Summary of pref values:
    //
    // AdaptationBufferSize  - Overrides "size" computation with smaller value (can only reduce)
    // AdaptationTargetTime  - Overrides computed target time
    // TransportByteLimit    - Specifies hard-limit on the transport buffer size (mobile client uses this)
    //
    // SuperBufferLength     - size of SuperBuffer in ms (5 min to 12 hours)
    // ODSuperBuffer         - use SuperBuffer for on-demand content (file size is used as length)
    // LiveSuperBuffer       - use SuperBuffer for live conten.
    //
   

    // Determine a target time
    if (pPrefs)
    {
        ReadPrefUINT32(pPrefs, "AdaptationTargetTime", m_ulTargetTime);
    }
    if (0 == m_ulTargetTime)
    {
        ULONG32 ulPostDecodeDelay = 0;
        pHdr->GetPropertyULONG32("PostDecodeDelay", ulPostDecodeDelay);
        if (pPrefs)
        {
            ReadPrefUINT32(pPrefs, "BaseAdaptationTargetTime", m_ulBaseTargetTime);
        }
        m_ulTargetTime = m_ulBaseTargetTime + ulPreroll + ulPostDecodeDelay;
    }
    else
    {
        // m_ulTargetTime is set to a preferred value in Prefs. with this flag,
        // we won't need to re-read from Prefs or recalculate target time later
        m_bTargetTimeInPrefs = TRUE;
    }

    // Compute the buffer size
    //
    
    // Add TransportByteLimit (or default)
    ReadPrefUINT32(pPrefs, "TransportByteLimit", m_ulBufferSize);
    if (0 == m_ulBufferSize)
    {
        // Transport buffers have no limit and can grow indefinitely (see RTSPClientProtocol::SetByteLimit)

        // Pick an arbitrary default: 32 secs of data at max bitrate, but no more than 64M
        UINT32 unDefaultTransBufSize = HXMulDiv(ulMaxBitRate, 32, BITS_PER_BYTE);
        unDefaultTransBufSize = HX_MIN(HX_MAX(unDefaultTransBufSize, 128 * KByte), 64 * MByte);
        m_ulBufferSize += unDefaultTransBufSize;
    }

    // Add bytes = preroll/1000 * max_bitrate/8
    m_ulBufferSize += HXMulDiv(ulMaxBitRate, ulPreroll, BITS_PER_BYTE * 1000);
 
    // Add super buffer if enabled
    HXBOOL bEnableODSuperBuffer = FALSE;
    HXBOOL bEnableLiveSuperBuffer = FALSE;
    ReadPrefBOOL(pPrefs, "ODSuperBuffer", bEnableODSuperBuffer);
    ReadPrefBOOL(pPrefs, "LiveSuperBuffer", bEnableLiveSuperBuffer);
    if (bIsLive)
    {
        if (bEnableLiveSuperBuffer)
        {
            UINT32 unSuperBuffer = 0;
            ReadPrefUINT32(pPrefs, "SuperBufferLength", unSuperBuffer); // in ms
            // Compute bytes = length/1000 * avg_bitrate/8
            unSuperBuffer = HXMulDiv(unSuperBuffer, ulAvgBitRate, BITS_PER_BYTE * 1000); 

            // The actual super-buffer limit is computed based on the playback time and the duration
            // of the data in the super-buffer. The size of this buffer (in bytes) therefore changes
            // depending on the average bitrate of the data currently in the buffer. To handle this
            // variation we reduce the size to 80% and hope that will cover the majority of cases.
            unSuperBuffer = HXMulDiv(unSuperBuffer, 8, 10);
            m_ulBufferSize += unSuperBuffer;
        }
    }
    else if (bEnableODSuperBuffer)
    {
        // OD uses size of file for super buffer. Set huge size by default.
        m_ulBufferSize = MAX_UINT32;
    }
  
    // Constrain default buffer size computation if limit is specified in prefs.
    UINT32 unBuffSizeLimit = 0;
    ReadPrefUINT32(pPrefs, "AdaptationBufferSize", unBuffSizeLimit);
    if (unBuffSizeLimit > 0)
    {
        m_ulBufferSize = HX_MIN(unBuffSizeLimit, m_ulBufferSize);
    }

    // Constrain to 9 digit max per 26.234 (buffer-size and target-time)
    const UINT32 MAX_9DIGIT = 999999999;
    m_ulBufferSize = HX_MIN(m_ulBufferSize, MAX_9DIGIT);
    m_ulTargetTime = HX_MIN(m_ulTargetTime, MAX_9DIGIT);


    m_ulReportFreq = ulReportFreq;
    m_uStreamNumber = uStreamNumber;
    m_type = type;

    HX_RELEASE(pPrefs);

    return HXR_OK;
   
}

const char* 
HXRAIStreamInfo::HeaderName() const
{
    const char* pRet = 
        (aht3GPP == m_type) ? z_3GPPAdaptationHdr :
        (ahtHelix == m_type) ? z_HelixAdaptationHdr :
        NULL;
    return pRet;
}

UINT32 HXRAIStreamInfo::Get3gpBufferSize() const
{
    // used for setting "size"
    return m_ulBufferSize;
}

UINT32 HXRAIStreamInfo::Get3gpTargetTime() const
{
    const UINT32 MAX_9DIGIT = 999999999;
    ULONG32 ulPostDecodeDelay = 0;
    ULONG32 ulPreroll         = 0;

    // We've already checked Prefs for AdaptationTargetTime in Init()
    // if it wasn't in Prefs, then we calculate target time.
    if (!m_bTargetTimeInPrefs)
    {
        m_pStreamHdr->GetPropertyULONG32("PostDecodeDelay", ulPostDecodeDelay);
        m_pStreamHdr->GetPropertyULONG32("Preroll",         ulPreroll);
        if (0 == ulPreroll)
        {
            // Use default 1 sec preroll per 3gpp
            ulPreroll = 1000;
        }
        // Base Target time would have been checked from Prefs and saved
        // previously in Init(). It won't change like Preroll and
        // PostDecodeDelay so no need to check again here.
        m_ulTargetTime = m_ulBaseTargetTime + ulPreroll + ulPostDecodeDelay;
    }
    m_ulTargetTime = HX_MIN(m_ulTargetTime, MAX_9DIGIT);

    return m_ulTargetTime;
}

UINT32 HXRAIStreamInfo::Get3gpReportFreq() const
{
    return m_ulReportFreq;
}

CHXRateAdaptationInfo::CHXRateAdaptationInfo() :
    m_pContext(NULL),
    m_pRateAdaptCtl(NULL),
    m_pCCF(NULL),
    m_pNADU(NULL),
    m_bHlxAdaptEnabled(FALSE),
    m_bRateAdaptationUsed(FALSE),
    m_bUseRTP(FALSE)
{}

CHXRateAdaptationInfo::~CHXRateAdaptationInfo()
{
    Close();
}


HX_RESULT CHXRateAdaptationInfo::Init(IUnknown* pContext)
{
    HX_RESULT res = HXR_POINTER;

    if (pContext)
    {
        m_pContext = pContext;
        m_pContext->AddRef();

        //Check to see if we want to disable server side rate
        //adaptation.
        IHXPreferences* pPrefs = NULL;
        HXBOOL          bSSRC  = TRUE;
        HXBOOL          bNADU = TRUE;

        m_pContext->QueryInterface(IID_IHXPreferences, (void**)&pPrefs);
        if( pPrefs )
        {
            ReadPrefBOOL( pPrefs, "HelixAdaptation", m_bHlxAdaptEnabled );
            ReadPrefBOOL( pPrefs, "3GPPAdaptation", bNADU );
            ReadPrefBOOL( pPrefs, "ServerSideRateControl", bSSRC );

            if (!bSSRC)
            {
                m_bHlxAdaptEnabled = FALSE;
                bNADU = FALSE;
            }
            HX_RELEASE(pPrefs);
        }
        if( m_bHlxAdaptEnabled || bNADU )
        {
            res = pContext->QueryInterface(IID_IHXClientRateAdaptControl,
                                           (void**)&m_pRateAdaptCtl);

            if (HXR_OK == res)
            {
                res = pContext->QueryInterface(IID_IHXCommonClassFactory,
                                               (void**)&m_pCCF);
            }

            if (HXR_OK == res && bNADU)
            {
                res = pContext->QueryInterface(IID_IHX3gppNADU,
                                               (void**)&m_pNADU);
            }
        }
    }

    return res;
}

HX_RESULT CHXRateAdaptationInfo::Close()
{
    while(!m_streamInfo.IsEmpty())
    {
        HXRAIStreamInfo* pInfo = 
            (HXRAIStreamInfo*)m_streamInfo.RemoveHead();
        HX_DELETE(pInfo);
    }
    
    HX_RELEASE(m_pRateAdaptCtl);
    HX_RELEASE(m_pCCF);
    HX_RELEASE(m_pNADU);
    HX_RELEASE(m_pContext);

    return HXR_OK;
}

// Called when we get a stream header
HX_RESULT 
CHXRateAdaptationInfo::OnStreamHeader(UINT16 uStreamNumber,
                                      IHXValues* pHdr)
{
    HXLOGL3(HXLOG_RTSP, "CHXRateAdaptationInfo[%p]::OnStreamHeader(): stream %u", this, uStreamNumber);

    HX_RESULT res = HXR_OK;
    if (!pHdr)
    {
        res = HXR_POINTER;
    }
    else if (m_pRateAdaptCtl)
    {
        ULONG32 ulReportFreq = 0;
        ULONG32 ulHelixAdaptation = 0;
        AdaptationHeaderType hdrType = ahtUnknown;

        if (m_bHlxAdaptEnabled && !GetUseRTPFlag() &&
            HXR_OK == pHdr->GetPropertyULONG32("Helix-Adaptation-Support",
                                               ulHelixAdaptation) &&
            (1 == ulHelixAdaptation))
        {
            ulReportFreq = 1;
            hdrType = ahtHelix;
        }
        else if (m_pNADU && 
                 HXR_OK == pHdr->GetPropertyULONG32("3GPP-Adaptation-Support",
                                                    ulReportFreq) && (ulReportFreq != 0))
        {
            hdrType = aht3GPP;
        }
        
        if (ahtUnknown != hdrType)
        {
            HXRAIStreamInfo* pInfo = new HXRAIStreamInfo;
            
            if (pInfo)
            {
                res = pInfo->Init(hdrType,
                                  uStreamNumber,
                                  ulReportFreq,
                                  pHdr, m_pContext);
                
                if ((HXR_OK == res) &&
                    (!m_streamInfo.AddTail(pInfo)))
                {
                    res = HXR_OUTOFMEMORY;
                }
                
                if (HXR_OK != res)
                {
                    HX_DELETE(pInfo);
                }
            }
            else
            {
                res = HXR_OUTOFMEMORY;
            }
        }
    }
    
    return res;
}

HX_RESULT 
CHXRateAdaptationInfo::CreateRateAdaptHeaders(UINT16 uStreamNumber,
                                              const char* pStreamURL,
                                              REF(IHXValues*) pHdrs)
{
    HX_RESULT res = HXR_FAILED;
    HXRAIStreamInfo* pInfo = GetStreamInfo(uStreamNumber);

    pHdrs = NULL;

    if (pInfo)
    {
        IHXBuffer* pAdaptStr = NULL;

        if (aht3GPP == pInfo->Type())
        {
            pAdaptStr = Create3gpAdaptHdrs(pStreamURL, pInfo);
        }
        else if (ahtHelix == pInfo->Type())
        {
            pAdaptStr = CreateHelixAdaptHdrs(pStreamURL, pInfo);
        }
            
        if (pAdaptStr)
        {
            res = CreateValuesCCF(pHdrs, m_pCCF);
            
            if (HXR_OK == res)
            {
                res = pHdrs->SetPropertyCString(pInfo->HeaderName(),
                                                pAdaptStr);
            }
                
            if(HXR_OK != res)
            {
                HX_RELEASE(pHdrs);
            }
        }
        HX_RELEASE(pAdaptStr);
    }

    return res;
}

// Called when the server responds to our
// rate adaptation request
HX_RESULT 
CHXRateAdaptationInfo::OnRateAdaptResponse(UINT16 uStreamNumber,
                                           IHXValues* pReqHdrs,
                                           IHXValues* pRespHdrs)
{
    HXLOGL3(HXLOG_RTSP, "CHXRateAdaptationInfo[%p]::OnRateAdaptResponse(): stream %u", this, uStreamNumber);
    HX_RESULT res = HXR_FAILED;

    HXRAIStreamInfo* pInfo = GetStreamInfo(uStreamNumber);

    if (pInfo)
    {
        IHXBuffer* pReqField = NULL;
        IHXBuffer* pRespField = NULL;

        // The value in the request must match the value in the response
        HX_RESULT hr = pReqHdrs->GetPropertyCString(pInfo->HeaderName(), pReqField);
        if (HXR_OK == hr)
        {
            hr =  pRespHdrs->GetPropertyCString(pInfo->HeaderName(), pRespField);
            if (HXR_OK == hr)
            {
                HXLOGL3(HXLOG_RTSP, "HXRateAdaptationInfo[%p]::OnRateAdaptResponse(): request: %s", this, pReqField->GetBuffer());
                HXLOGL3(HXLOG_RTSP, "HXRateAdaptationInfo[%p]::OnRateAdaptResponse(): response: %s", this, pRespField->GetBuffer());

                hr = HXR_FAIL;
                if (pReqField->GetSize() == pRespField->GetSize() &&
                    !memcmp(pReqField->GetBuffer(), pRespField->GetBuffer(), pReqField->GetSize()) )
                {
                    m_bRateAdaptationUsed = TRUE;
                    hr = HXR_OK;

                    IHXTransportBufferLimit* pBufLimit = NULL;
                    IHXStreamSource* pSource = NULL;
                    if (HXR_OK == m_pContext->QueryInterface(IID_IHXStreamSource, (void**)&pSource))
                    {
                        if (pSource && (HXR_OK == pSource->QueryInterface(IID_IHXTransportBufferLimit, (void**)&pBufLimit)))
                        {
                            pBufLimit->SetByteLimit(pInfo->StreamNumber(), pInfo->Get3gpBufferSize());
                        }

                        HX_RELEASE(pBufLimit);
                    }

                    HX_RELEASE(pSource);
                }
            }
        }

        if (HXR_OK == hr)
        {
            // Disable client rate adaptation for this stream
            res = m_pRateAdaptCtl->Disable(uStreamNumber);

            if (HXR_OK == res && m_pNADU)
            {
                // Notify the IHX3gppNADU object of the negotiated
                // parameters
                res = m_pNADU->SetNADUParameters(uStreamNumber,
                                                 pInfo->Get3gpReportFreq(),
                                                 pInfo->Get3gpBufferSize());
            }
        }
        else
        {
            HXLOGL3(HXLOG_RTSP, "CHXRateAdaptationInfo[%p]::OnRateAdaptResponse(): mismatch; removing stream %u", this, uStreamNumber);
            RemoveStreamInfo(uStreamNumber);
            res = HXR_OK;
        }

        HX_RELEASE(pReqField);
        HX_RELEASE(pRespField);
    }

    return res;
}

HXRAIStreamInfo* 
CHXRateAdaptationInfo::GetStreamInfo(UINT16 uStreamNumber) const
{
    HXRAIStreamInfo* pInfo = NULL;

    LISTPOSITION pos = m_streamInfo.GetHeadPosition();

    while (!pInfo && pos)
    {
        HXRAIStreamInfo* pTmp = 
            (HXRAIStreamInfo*)m_streamInfo.GetAt(pos);
        
        if (pTmp &&
            (pTmp->StreamNumber() == uStreamNumber))
        {
            pInfo = pTmp;
        }

        m_streamInfo.GetNext(pos);
    }
    return pInfo;
}

void 
CHXRateAdaptationInfo::RemoveStreamInfo(UINT16 uStreamNumber)
{
    HXBOOL bDone = FALSE;
    LISTPOSITION pos = m_streamInfo.GetHeadPosition();

    while (!bDone && pos)
    {
        HXRAIStreamInfo* pTmp = 
            (HXRAIStreamInfo*)m_streamInfo.GetAt(pos);
        
        if (pTmp &&
            (pTmp->StreamNumber() == uStreamNumber))
        {
            m_streamInfo.RemoveAt(pos);
            HX_DELETE(pTmp);
            bDone = TRUE;
        }
        else
        {
            m_streamInfo.GetNext(pos);
        }
    }
}

IHXBuffer* 
CHXRateAdaptationInfo::Create3gpAdaptHdrs(const char* pStreamURL,
                                          HXRAIStreamInfo* pInfo)
{
    IHXBuffer* pRet = NULL;

    if (pStreamURL && pInfo)
    {
        IHXValues* pValues = NULL;
        C3gpAdaptationHeader adaptHdr;
        
        HX_RESULT res = adaptHdr.Init(m_pCCF);
        ULONG32 ulTargetTime = 
            pInfo->Get3gpTargetTime();
        ULONG32 ulBufferSize = 
            pInfo->Get3gpBufferSize();

        if (HXR_OK == res)
        {
            res = CreateValuesCCF(pValues, m_pCCF);
        }

        if (HXR_OK == res)
        {
            res = SetCStringPropertyCCF(pValues,
                                        "url",
                                        pStreamURL,
                                        m_pCCF);
        }
        
        if (HXR_OK == res)
        {
            res = pValues->SetPropertyULONG32("target-time",
                                              ulTargetTime);
        }
        
        if (HXR_OK == res)
        {
            res = pValues->SetPropertyULONG32("size",
                                              ulBufferSize);
        }
        
        if (HXR_OK == res)
        {
            res = adaptHdr.SetValues(pValues);
        }

        if (HXR_OK == res)
        {
            res = adaptHdr.GetString(pRet);
        }
        
        HX_RELEASE(pValues);
    }

    return pRet;
}

IHXBuffer* 
CHXRateAdaptationInfo::CreateHelixAdaptHdrs(const char* pStreamURL,
                                            HXRAIStreamInfo* pInfo)
{
    IHXBuffer* pRet = NULL;

    if (pStreamURL && pInfo)
    {
        IHXValues* pValues = NULL;
        CHelixAdaptationHeader adaptHdr;
        
        HX_RESULT res = adaptHdr.Init(m_pCCF);
        ULONG32 ulTargetTime = 
            pInfo->Get3gpTargetTime();
        ULONG32 ulBufferSize = 
            pInfo->Get3gpBufferSize();

        if (HXR_OK == res)
        {
            res = CreateValuesCCF(pValues, m_pCCF);
        }

        if (HXR_OK == res)
        {
            res = SetCStringPropertyCCF(pValues,
                                        "url",
                                        pStreamURL,
                                        m_pCCF);
        }
        
        if (HXR_OK == res)
        {
            res = pValues->SetPropertyULONG32("target-time",
                                              ulTargetTime);
        }
        
        if (HXR_OK == res)
        {
            res = pValues->SetPropertyULONG32("size",
                                              ulBufferSize);
        }
        
        if (HXR_OK == res)
        {
            res = pValues->SetPropertyULONG32("stream-switch",
                                              1);
        }

        if (HXR_OK == res)
        {
            res = pValues->SetPropertyULONG32("feedback-level",
                                              1);
        }
        
        if (HXR_OK == res)
        {
            res = adaptHdr.SetValues(pValues);
        }

        if (HXR_OK == res)
        {
            res = adaptHdr.GetString(pRet);
        }
        
        HX_RELEASE(pValues);
    }

    return pRet;
}
