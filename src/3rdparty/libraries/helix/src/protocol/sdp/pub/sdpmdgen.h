/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: sdpmdgen.h,v 1.20 2009/03/14 01:35:32 ckarusala Exp $
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

#ifndef SDPMDESCPACK_H
#define SDPMDESCPACK_H

#include "hxcom.h"
#include "ihxpckts.h"
#include "hxccf.h"
#include "hxmon.h"

#include "mdescgen.h"

class CHXSimpleList;
class CHXString;

class SDPMediaDescGenerator : public MediaDescGenerator
{
public:
    SDPMediaDescGenerator(ULONG32 ulVersion);
    virtual ~SDPMediaDescGenerator();
    virtual HX_RESULT Init(IUnknown* pContext);
    
    virtual HX_RESULT Generate(UINT16 nValues, IHXValues** pValueArray,
			       REF(IHXBuffer*) pDescription);
    virtual HXBOOL GetUseOldEOL() const;
    virtual void SetUseOldEOL(HXBOOL bUseOldEOL);
    
    virtual HXBOOL GetUseSessionGUID() const;
    virtual HXBOOL GetUseAbsoluteURL() const;
    virtual void SetUseSessionGUID(HXBOOL bOption);
    virtual void SetUseAbsoluteURL(HXBOOL bOption);

private:
    struct MediaInfo;

    UINT32 GetSessionDuration(UINT16 nValues, IHXValues** ppValueArray);
    char* EscapeBuffer(const char* pBuffer, UINT32 len);

    HXBOOL HandleSDPLine(IHXBuffer* pLine, MediaInfo* pStreamInfo);
    HXBOOL HasSDPLine(CHXSimpleList* pSDPList, const char* szLine);
    void GetSDPData(IHXBuffer* pSDPDataBuf, MediaInfo* pStreamInfo);
    HX_RESULT GetStreamGroups(UINT16 usNumHeaders,
                                IHXValues** pHeaders,
                                UINT32 ulNumStreamGroups,
                                CHXSimpleList**& pStreamGroups);
    UINT32 CheckClient(IHXBuffer* pUserAgent, IHXBuffer* pPSSVersion);

    IUnknown* m_pContext;
    IHXCommonClassFactory* m_pCCF;
    IHXRegistry* m_pRegistry;
    ULONG32 m_ulVersion;
    HXBOOL m_bUseOldEOL;
    HXBOOL m_bUseSessionGUID;
    HXBOOL m_bUseAbsoluteURL;

    static UINT32   g_ul3GPPNADUReportFreq;
    static HXBOOL   g_bGot3GPPNADUReportFreq;
    static const char*      g_pHelixClients[];

    static HXBOOL     g_bHelixRateAdaptationConfigured;
    static HXBOOL     g_bHelixRateAdaptationLicensed;
    static HXBOOL     g_bCheckedHelixRateAdaptationLicense;

    struct SDPDataLine { const char* pName; UINT32 ulLen;};
    static const SDPDataLine g_pSDPDataPullTable[];

    struct MediaInfo
    {
        UINT32  ulControlID;
        HXBOOL  bControlIDFound;
        UINT32  ulDuration;
        HXBOOL  bDurationFound;
        UINT32  ulSampleRate;
        HXBOOL  bSampleRateFound;
        UINT32  ulChannels;
        HXBOOL  bChannelsFound;
        UINT32  ulRTCPRR;
        HXBOOL  bRTCPRRFound;
        UINT32  ulRTCPRS;
        HXBOOL  bRTCPRSFound;
        UINT32  ulPreDecBufSize;
        HXBOOL  bPreDecBufSizeFound;
        UINT32  ulPreDecBufPeriod;
        HXBOOL  bPreDecBufPeriodFound;
        UINT32  ulPostDecBufPeriod;
        HXBOOL  bPostDecBufPeriodFound;
        UINT32  ulDecByteRate;
        HXBOOL  bDecByteRateFound;
        UINT32  ulPtime;
        HXBOOL  bPtimeFound;
        UINT32  ulConnTTL;
        HXBOOL  bConnTTLFound;
        UINT32  ulConnRange;
        HXBOOL  bConnRangeFound;
        UINT32  ulAvgBitRate;
        HXBOOL  bAvgBitRateFound;
        UINT32  ulMaxBitRate;
        HXBOOL    bMaxBitRateFound;
        UINT32  ulAvgPacketSize;
        HXBOOL    bAvgPacketSizeFound;
        UINT32  ulPayloadType;
        HXBOOL  bPayloadTypeFound;
        HXBOOL  bUseAnnexG;
        UINT32  ulFrameHeight;
        HXBOOL  bFrameHeightFound;
        UINT32  ulFrameWidth;
        HXBOOL  bFrameWidthFound;

        HXBOOL    bPayloadWirePacket;
        UINT32  ulASBitRate;
        UINT32  ulPort;

        char*   pszMimeType;
        char*   pszMimeFirst;
        const char* pszMimeLast;

        char*   pszFmtp;        
        char*   pszConnAddr;
        char*   pszInfo;
        char*   pszConnection;

        CHXSimpleList* pSDPData;

        inline void Cleanup();
    };
};

inline void SDPMediaDescGenerator::MediaInfo::Cleanup()
{
    HX_VECTOR_DELETE(pszMimeType);
    HX_VECTOR_DELETE(pszMimeFirst);
    HX_VECTOR_DELETE(pszFmtp);
    HX_VECTOR_DELETE(pszConnAddr);
    HX_VECTOR_DELETE(pszInfo);
    HX_VECTOR_DELETE(pszConnection);

    if (pSDPData)
    {
        while(!pSDPData->IsEmpty())
        {
            IHXBuffer* pBuf = (IHXBuffer*)pSDPData->RemoveHead();
            HX_RELEASE(pBuf);
        }
    }
    HX_DELETE(pSDPData);
}

#endif /* SDPMDESCPACK_H */
