/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: sdpmdgen.cpp,v 1.74 2009/06/05 05:31:55 ckarusala Exp $
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

#include "sdptypes.h"
#include "sdppyldinfo.h"
#include "hxstrutl.h"
#include "hxstring.h"
#include "nptime.h"
#include "netbyte.h"
#include "rtsputil.h"
#include "rtptypes.h"
#include "hxslist.h"

#include "sdpchunk.h"
#include "sdpmdgen.h"
#include "defslice.h"

// Packet overhead in bits
#define RTP_HEAD_SZ 96  // 12 bytes
#define RDT_HEAD_SZ 128 // 16 bytes
#define UDP_HEAD_SZ 64  // 8 bytes
#define TCP_HEAD_SZ 160 // 20 bytes
#define IP4_HEAD_SZ 160 // 20 bytes
#define IP6_HEAD_SZ 320 // 40 bytes
#define ETH_HEAD_SZ 208 // 26 bytes: 8 preamble, 14 header, 4 CRC

#define SDP_DEFAULT_PACKET_RATE 16

#define SDP_CLIENT_HLX  0x00000001
#define SDP_CLIENT_REL6 0x00000010

#define NADU_REPORT_FREQUENCY_CFG "config.StreamAdaptation.NADUReportFrequency"
#define REG_RTSP_PREF_TCP "config.MediaDelivery.UserAgentSettings.Default.PreferTCP"

#define SDP_TITLE_CFG "config.SDPDefaultTitle"
#define SDP_TITLE_LEN_CFG "config.SDPTitleNumCharsOfFilename"

// Helix Rate Adaptation Configuration; default is TRUE
#define REGISTRY_GL_HRA_CFG "config.StreamAdaptation.AdvertiseHelixAdaptation"
BOOL    SDPMediaDescGenerator::g_bHelixRateAdaptationConfigured = TRUE;

// 3GPP adaptation report frequency. Default to 1.
UINT32  SDPMediaDescGenerator::g_ul3GPPNADUReportFreq = 1;
BOOL    SDPMediaDescGenerator::g_bGot3GPPNADUReportFreq = FALSE;

// Helix Rate Adaptation licensing
BOOL    SDPMediaDescGenerator::g_bHelixRateAdaptationLicensed = FALSE;
BOOL    SDPMediaDescGenerator::g_bCheckedHelixRateAdaptationLicense = FALSE;

// Maybe we should also add a config list so more
// can be added in the field
const char* SDPMediaDescGenerator::g_pHelixClients[] =
{ "RealMedia", "RealOne", "Helix", NULL };

// SDPData chunk lines to skip when writing to the SDP.
// All lines that are not a= will be skipped by default.
const SDPMediaDescGenerator::SDPDataLine
SDPMediaDescGenerator::g_pSDPDataPullTable[] =
{
    {"a=control:",                  sizeof("a=control:") - 1},
    {"a=range:",                    sizeof("a=range:") - 1},
    {"a=rtpmap:",                   sizeof("a=rtpmap:") - 1},
    {"a=X-predecbufsize:",          sizeof("a=X-predecbufsize:") - 1},
    {"a=X-initpredecbufperiod:",    sizeof("a=X-initpredecbufperiod:") - 1},
    {"a=X-initpostdecbufperiod:",   sizeof("a=X-initpostdecbufperiod:") - 1},
    {"a=X-decbyterate:",            sizeof("a=X-decbyterate:") - 1},
    {"a=3GPP-Adaptation-Support:",  sizeof("a=3GPP-Adaptation-Support:") - 1},
    {"a=Helix-Adaptation-Support",  sizeof("a=Helix-Adaptation-Support") - 1},

    {NULL,                          0}
};


SDPMediaDescGenerator::SDPMediaDescGenerator(ULONG32 ulVersion) :
    m_pContext(0),
    m_pCCF(0),
    m_pRegistry(0),
    m_ulVersion(ulVersion),
    m_bUseOldEOL(FALSE),
    m_bUseSessionGUID(FALSE),
    m_bUseAbsoluteURL(FALSE)
{}

SDPMediaDescGenerator::~SDPMediaDescGenerator()
{
    HX_RELEASE(m_pCCF);
    HX_RELEASE(m_pContext);
    HX_RELEASE(m_pRegistry);
}

HX_RESULT SDPMediaDescGenerator::Init(IUnknown* pContext)
{
    HX_RESULT res = HXR_FAILED;

    HX_RELEASE(m_pContext);
    HX_RELEASE(m_pCCF);

    if (pContext)
    {
        res = pContext->QueryInterface(IID_IHXCommonClassFactory,
                                       (void**)&m_pCCF);
        if (SUCCEEDED(res) && m_pCCF)
        {
            m_pContext = pContext;
            m_pContext->AddRef();

            res = pContext->QueryInterface(IID_IHXRegistry,
                (void**)&m_pRegistry);
        }
        if (!g_bGot3GPPNADUReportFreq && SUCCEEDED(res))
        {
            // Look up and store the 3GPP Adaptation Report Frequency
            // in the config if we haven't yet
            INT32 lFrequency;
            if (SUCCEEDED(m_pRegistry->GetIntByName(NADU_REPORT_FREQUENCY_CFG,
                lFrequency)))
            {
                g_ul3GPPNADUReportFreq = (UINT32)lFrequency;
            }

            g_bGot3GPPNADUReportFreq = TRUE;
        }
    }

    return res;
}

HX_RESULT SDPMediaDescGenerator::Generate(UINT16 nValues,
                                          IHXValues** pValueArray,
                                          REF(IHXBuffer*) pDescription)
{
    CHXString mDesc;

    char pszEOL[3]; /* Flawfinder: ignore */
    HX_RESULT rc;
    UINT32 propValue;
    const char* pPropName = 0;
    IHXBuffer* pPropBuffer = 0;
    char psz256[256]; /* Flawfinder: ignore */
    UINT32 ulLen = 0;
    HXBOOL bIsIN6 = FALSE;
    HXBOOL bTCP = FALSE;

    mDesc.SetMinBufSize(12000);

    if (m_bUseOldEOL)
    {
        pszEOL[0] = '\n';
        pszEOL[1] = '\0';
    }
    else
    {
        pszEOL[0] = '\r';
        pszEOL[1] = '\n';
        pszEOL[2] = '\0';
    }

    // Check for Helix Rate Adaptation license if
    // we haven't already
    if (!g_bCheckedHelixRateAdaptationLicense && m_pRegistry)
    {
        // Check if license file includes key for Helix
        // Rate Adaptation
        INT32 lHRAEnabled;
        if (SUCCEEDED(m_pRegistry->GetIntByName(REGISTRY_HRA_ENABLED,
            lHRAEnabled)))
        {
            g_bHelixRateAdaptationLicensed = lHRAEnabled;
        }
        else
        {
            g_bHelixRateAdaptationLicensed = LICENSE_HRA_ENABLED;
        }

        g_bCheckedHelixRateAdaptationLicense = TRUE;
    }

    // Check for global Helix Rate Adaptation configuration. If it's present, it may
    // affect whether or not HRA is used to init MDP.
    INT32 lHRAConfigured = 0;
    if (m_pRegistry)
    {
        if (SUCCEEDED(m_pRegistry->GetIntByName(REGISTRY_GL_HRA_CFG,
                        lHRAConfigured)))
        {
            g_bHelixRateAdaptationConfigured = lHRAConfigured;
        }

        INT32 lTCPPref = 0;
        if (SUCCEEDED(m_pRegistry->GetIntByName(REG_RTSP_PREF_TCP, lTCPPref))
                && lTCPPref)
        {
            // If PreferClientTCP is set, we will assume TCP unless
            // otherwise specified in the optional header
            // (from UA config)
            bTCP = TRUE;
        }
    }

    // get local host address from registry

    /*  XXX
     *  Interop:  Our old system is expecting a media type in m= to be either
     *  "audio", "video", or "data" and anything else will be ignored.
     *  The spec dectates otherwise...It should be whatever the type of mimetype
     *  is.  So, depending on the setting, we will have to change how we create
     *  m= line for interop and backward comp.
     *
     *  by default, we will do the spec complient way.
     */
    BOOL bUseOldSdp = FALSE;

#if 0
    IHXBuffer* pHostIBuf = NULL;

    if(m_pRegistry)
    {
        m_pRegistry->GetStrByName("server.sdpconfig.hostaddr", pHostIBuf);
    }

    const char* pszHost = "";

    if (pHostIBuf)
    {
        pszHost = (const char *)pHostIBuf->GetBuffer();
    }

    UINT32 ulHostAddr = HXinet_addr(pszHost);
#endif
    /*
     * default sessionID, else get LastModified from optional values
     */

    char pszVersion[40]; /* Flawfinder: ignore */
    char pszSessionID[40]; /* Flawfinder: ignore */
    IHXBuffer* pControlStringBuff = NULL;
    IHXBuffer* pUserAgent = NULL;
    IHXBuffer* pPSSVersion = NULL;

    // munge current time with IP
    NPTime timeNow;
    //XXXDPL is the hostaddr necessary here?
    //SafeSprintf(pszSessionID, 40, "%lu", ulHostAddr + timeNow.m_lMicroSecond);
    SafeSprintf(pszSessionID, 40, "%lu", timeNow.m_lMicroSecond);
    SafeSprintf(pszVersion, 40, "%lu", timeNow.m_lMicroSecond);

    IHXBuffer* pReqURL = NULL;
    IHXBuffer* pContentBase = NULL;
    IHXBuffer* pQueryParams = NULL;
    IHXBuffer* pHostname = NULL;
    double dBWMult = 1.0;
    HXBOOL bUnspecifiedRange = FALSE;

    // pValueArray[1] - optional session properties
    if(pValueArray[1])
    {
        rc = pValueArray[1]->GetFirstPropertyULONG32(pPropName, propValue);
        while(rc == HXR_OK)
        {
            if (strcasecmp(pPropName, "LastModified") == 0)
            {
                SafeSprintf(pszVersion, 40,"%lu", propValue);
                SafeSprintf(pszSessionID, 40,"%lu", propValue);
            }
            else if (strcasecmp(pPropName, "BandwidthMultiplier") == 0)
            {
                dBWMult =  (double)propValue / 100.0;
            }
            else if (strcasecmp(pPropName, "OpenDuration") == 0)
            {
                bUnspecifiedRange = (HXBOOL) propValue;
            }
            rc = pValueArray[1]->GetNextPropertyULONG32(pPropName, propValue);
        }

        rc = pValueArray[1]->GetFirstPropertyCString(pPropName,
            pPropBuffer);
        while(rc == HXR_OK)
        {
            if (m_bUseAbsoluteURL &&
                strcasecmp(pPropName, "AbsoluteBaseURL") == 0)
            {
                pContentBase = pPropBuffer;
            }
            else if (m_bUseAbsoluteURL && strcasecmp(pPropName,
                "URLQueryParams") == 0)
            {
                pQueryParams = pPropBuffer;
            }
            else if (strcasecmp(pPropName, "RequestURL") == 0)
            {
                pReqURL = pPropBuffer;
            }
            else if (strcasecmp(pPropName, "Control") == 0)
            {
                pControlStringBuff = pPropBuffer;
            }

            else if (strcasecmp(pPropName, "UserAgent") == 0)
            {
                pUserAgent = pPropBuffer;
            }
            else if (strcasecmp(pPropName, "PSSVersion") == 0)
            {
                pPSSVersion = pPropBuffer;
            }
            else if (strcasecmp(pPropName, "SockFamily") == 0)
            {
                if (pPropBuffer &&
                    !strcasecmp((const char*)pPropBuffer->GetBuffer(), "IN6"))
                {
                    bIsIN6 = TRUE;
                }

                HX_RELEASE(pPropBuffer);
            }
            else if (strcasecmp(pPropName, "BandwidthProtocol") == 0)
            {
                if (pPropBuffer &&
                    !strcasecmp((const char*)pPropBuffer->GetBuffer(), "TCP"))
                {
                    bTCP = TRUE;
                }
                else
                {
                    // If this is present, it overrides any global
                    // protocol setting
                    bTCP = FALSE;
                }
                HX_RELEASE(pPropBuffer);
            }
            else if (strcasecmp(pPropName, "Hostname") == 0)
            {
                pHostname = pPropBuffer;
            }
            else
            {
                HX_RELEASE(pPropBuffer);
            }
            rc = pValueArray[1]->GetNextPropertyCString(pPropName,
                                                    pPropBuffer);
        }
    }

    const char* szHostname = NULL;
    if (pHostname)
    {
        szHostname = (const char*)pHostname->GetBuffer();
    }
    else if (bIsIN6)
    {
        HX_ASSERT(!"No Hostname set in OptionalHeaders!");
        szHostname = "::0";
    }
    else
    {
        HX_ASSERT(!"No Hostname set in OptionalHeaders!");
        szHostname = "0.0.0.0";
    }

    HX_ASSERT(szHostname);

    ulLen = strlen(pszSessionID) + strlen(pszVersion) + strlen(szHostname) + 64;
    NEW_FAST_TEMP_STR(pszTmpString, 256, ulLen);

    SafeSprintf(pszTmpString, ulLen,
                "v=0%so=- %s %s IN %s %s%s",
                pszEOL,
                pszSessionID,
                pszVersion,
                bIsIN6 ? "IP6" : "IP4",
                szHostname,
                pszEOL);

#if 0
    HX_RELEASE(pHostIBuf);
#endif

    mDesc = pszTmpString;

    DELETE_FAST_TEMP_STR(pszTmpString);

    char* pszTitle = NULL;
    char* pszAuthor = NULL;
    char* pszCopyright = NULL;

    CHXString headerAttributes;
    headerAttributes.SetMinBufSize(8192);

    BOOL   bAddToHeader = 0;
    UINT32 ulSessionDuration = 0;
    BOOL   bDefaultDurationFound = FALSE;
    BOOL   bIsLive = FALSE;
    BOOL   bIsRealDatatype = FALSE;
    UINT32 ulStreamGroupCount = 0;
    BOOL   bFoundStreamGroupCount = FALSE;
    UINT32 ulStreamCount = 0;
    BOOL   bFoundStreamCount = FALSE;
    UINT32 ulClientType = 0;
    UINT32 ulPktHead = 0;
    UINT32 ulASBitRate = 0;

    MediaInfo sessionInfo;
    memset(&sessionInfo, 0, sizeof(MediaInfo));

    /*
     * Add session-level a=control field only for clients that can handle
     * interop SDP.
     */

    // Figure out if the client can handle interop SDP
    UINT32 ulSdpFileType = NONE_SDP;
    if (HXR_OK == pValueArray[0]->GetPropertyULONG32("SdpFileType", ulSdpFileType))
    {
        // default for bUseOldSdp is FALSE
        if (BACKWARD_COMP_SDP == ulSdpFileType)
        {
            bUseOldSdp = TRUE;
        }
    }

    if(!bUseOldSdp)
    {
        if (!pControlStringBuff)
        {
            const char* pszFieldPrefix = "a=control:";
            UINT32 ulBuffLen = 0;

            if (pContentBase)
            {
                char *pszBaseURL = (char*)pContentBase->GetBuffer();
                UINT32 ulBaseLen = strlen(pszBaseURL);
                UINT32 ulBuffLen = strlen(pszFieldPrefix) + ulBaseLen +
                                   ((pQueryParams) ?
                                   pQueryParams->GetSize() : 0);
                char cPathDelim = '\0';

                // Strip off the trailing slash from the aggregate control URL
                char *pszPathDelim = pszBaseURL + (ulBaseLen-1);
                if (*pszPathDelim == '/')
                {
                    cPathDelim = *pszPathDelim;
                    *pszPathDelim = '\0';
                }

                ulBuffLen += strlen(pszEOL) + 1;
                NEW_FAST_TEMP_STR(pszSessionCtrl, 512, ulBuffLen);
                SafeSprintf(pszSessionCtrl, ulBuffLen, "%s%s%s%s",
                         pszFieldPrefix, pszBaseURL,
                         (pQueryParams)?((char*)pQueryParams->GetBuffer()) : "",
                         pszEOL);

                if (cPathDelim)
                {
                    *pszPathDelim = cPathDelim;
                }
                headerAttributes += pszSessionCtrl;
                DELETE_FAST_TEMP_STR(pszSessionCtrl);
            }
            else
            {
                ulBuffLen = strlen(pszFieldPrefix) + strlen(pszEOL) + 2;
                NEW_FAST_TEMP_STR(pszSessionCtrl, 512, ulBuffLen);
                SafeSprintf(pszSessionCtrl, ulBuffLen, "%s*%s", pszFieldPrefix, pszEOL);
                headerAttributes += pszSessionCtrl;
                DELETE_FAST_TEMP_STR(pszSessionCtrl);
            }
        }
        else
        {
            UINT32 ulBuffLen = pControlStringBuff->GetSize();
            ulBuffLen += (pContentBase) ? pContentBase->GetSize() : 0;
            ulBuffLen += (pQueryParams) ? pQueryParams->GetSize() : 0;
            ulBuffLen += 64;

            NEW_FAST_TEMP_STR(pszControlString, 512, ulBuffLen);
            SafeSprintf(pszControlString, ulBuffLen, "a=control:%s%s%s%s",
                     (pContentBase ? ((char*)pContentBase->GetBuffer()) : ""),
                     pControlStringBuff->GetBuffer(),
                     (pQueryParams) ?
                     ((char*)pQueryParams->GetBuffer()) : "", pszEOL);

            headerAttributes += pszControlString;
            DELETE_FAST_TEMP_STR(pszControlString);
        }
    }

    // Find out what type of client we've got
    ulClientType = CheckClient(pUserAgent, pPSSVersion);

    ulSessionDuration = GetSessionDuration(nValues, pValueArray);


    HXBOOL bStandardSDP = FALSE;

    rc = pValueArray[0]->GetFirstPropertyULONG32(pPropName, propValue);

    while(rc == HXR_OK)
    {
        bAddToHeader = FALSE;

        if(strcasecmp(pPropName, "MulticastTTL") == 0)
        {
            sessionInfo.ulConnTTL = propValue;
            sessionInfo.bConnTTLFound = TRUE;
        }
        else if(strcasecmp(pPropName, "MulticastRange") == 0)
        {
            sessionInfo.ulConnRange = propValue;
            sessionInfo.bConnRangeFound = TRUE;
        }
        else if(strcasecmp(pPropName, "Duration") == 0)
        {
            bDefaultDurationFound = TRUE;
        }
        else if(strcasecmp(pPropName, "LiveStream") == 0)
        {
            // is this live?
            bIsLive = propValue;
            bAddToHeader = TRUE;
        }
        else if(strcasecmp(pPropName, "SdpFileType") == 0)
        {
            // don't add it to attribute.
            bAddToHeader = FALSE;
        }
        else if (strcasecmp(pPropName, "IsRealDatatype") == 0)
        {
            bIsRealDatatype = propValue ? TRUE : FALSE;
            bAddToHeader = bIsRealDatatype;
        }
        else if (strcasecmp(pPropName, "StreamGroupCount") == 0)
        {
            ulStreamGroupCount = propValue;
            bFoundStreamGroupCount = TRUE;
        }
        else if (strcasecmp(pPropName, "StreamCount") == 0)
        {
            ulStreamCount = propValue;
            bFoundStreamCount = TRUE;
        }
        else if (strcasecmp(pPropName, "StandardSDP") == 0)
        {
            bStandardSDP = propValue ? TRUE : FALSE;                    
        }
        else
        {
            //If a standard SDP has to be generated then don't
            //add unknown values to SDP.
            bAddToHeader =  bStandardSDP ? FALSE : TRUE;
        }

        if(bAddToHeader)
        {
            ulLen = strlen(pPropName) + 64;
            NEW_FAST_TEMP_STR(pszAttBuf, 256, ulLen);
            SafeSprintf(pszAttBuf, ulLen, "a=%s:integer;%d%s", pPropName,
                                                    propValue, pszEOL);
            headerAttributes += pszAttBuf;
            DELETE_FAST_TEMP_STR(pszAttBuf);
        }

        /* make sure to reset SdpFileType header */
        if (strcasecmp(pPropName, "SdpFileType") == 0)
        {
            pValueArray[0]->SetPropertyULONG32(pPropName, NONE_SDP);
        }

        rc = pValueArray[0]->GetNextPropertyULONG32(pPropName,
            propValue);
    }

    rc = pValueArray[0]->GetFirstPropertyBuffer(pPropName, pPropBuffer);
    while(rc == HXR_OK)
    {
        INT32 dataSize = pPropBuffer->GetSize();
        char* pString = (char*) pPropBuffer->GetBuffer();
        bAddToHeader = FALSE;

        if (dataSize > 0 && pString != NULL && *pString != '\0')
        {
            if(strcasecmp(pPropName, "Title") == 0)
            {
                bAddToHeader = TRUE;
                pszTitle = new char [ dataSize + 1 ];
                memcpy(pszTitle, (const char*)pPropBuffer->GetBuffer(), dataSize); /* Flawfinder: ignore */
                pszTitle[dataSize] = '\0';
            }
            else if(strcasecmp(pPropName, "Author") == 0)
            {
                bAddToHeader = TRUE;
                pszAuthor = new char [ dataSize + 1 ];
                memcpy(pszAuthor, (const char*)pPropBuffer->GetBuffer(), dataSize); /* Flawfinder: ignore */
                pszAuthor[dataSize] = '\0';
            }
            else if(strcasecmp(pPropName, "Copyright") == 0)
            {
                bAddToHeader = TRUE;
                pszCopyright = new char [ dataSize + 1 ];
                memcpy(pszCopyright, (const char*)pPropBuffer->GetBuffer(), dataSize); /* Flawfinder: ignore */
                pszCopyright[dataSize] = '\0';
            }
            else
            {
                bAddToHeader = bStandardSDP ? FALSE : TRUE;
            }

            if (bAddToHeader)
            {
                ulLen = dataSize * 2 + strlen(pPropName) + 64;
                NEW_FAST_TEMP_STR(pszAttBuf, 4096, ulLen);
                NEW_FAST_TEMP_STR(pszPropString, 4096, dataSize * 2 + 64);

                (void)BinTo64((const BYTE*)pPropBuffer->GetBuffer(),
                    dataSize, pszPropString);

                SafeSprintf(pszAttBuf, ulLen, "a=%s:buffer;\"%s\"%s",pPropName,
                                                          pszPropString, pszEOL);

                headerAttributes += pszAttBuf;
                DELETE_FAST_TEMP_STR(pszPropString);
                DELETE_FAST_TEMP_STR(pszAttBuf);
            }
        }

        HX_RELEASE(pPropBuffer);

        rc = pValueArray[0]->GetNextPropertyBuffer(pPropName,
            pPropBuffer);
    }

    rc = pValueArray[0]->GetFirstPropertyCString(pPropName,
        pPropBuffer);
    while(rc == HXR_OK)
    {
        char* pString = (char*) pPropBuffer->GetBuffer();
        char* pszData=NULL;
        BOOL bDeleteString=FALSE;

        bAddToHeader = FALSE;

        INT32 dataSize = pPropBuffer->GetSize();

        if (dataSize > 0 && pString != NULL && *pString != '\0')
        {
            if(strcasecmp(pPropName, "Title") == 0)
            {
                pszTitle = EscapeBuffer(pString, pPropBuffer->GetSize());
                pszData = pszTitle;

                bAddToHeader = TRUE;
            }
            else if(strcasecmp(pPropName, "Author") == 0)
            {
                pszAuthor = EscapeBuffer(pString, pPropBuffer->GetSize());
                pszData = pszAuthor;

                bAddToHeader = TRUE;
            }
            else if(strcasecmp(pPropName, "Copyright") == 0)
            {
                pszCopyright = EscapeBuffer(pString, pPropBuffer->GetSize());
                pszData = pszCopyright;

                bAddToHeader = TRUE;
            }
            else if(strcasecmp(pPropName, "MulticastAddress") == 0)
            {
                sessionInfo.pszConnAddr =
                    EscapeBuffer(pString, pPropBuffer->GetSize());
            }
            else if(strcasecmp(pPropName, "SDPData") == 0)
            {
                GetSDPData(pPropBuffer, &sessionInfo);
            }
            else if(strcasecmp(pPropName, "Information") == 0)
            {
                // "i="
                sessionInfo.pszInfo =
                    EscapeBuffer(pString, pPropBuffer->GetSize());
            }
            else if (strcasecmp(pPropName, "ASMRuleBook") == 0)
            {
                bAddToHeader = TRUE;

                if (bAddToHeader)
                {
                    pszData = EscapeBuffer(pString, pPropBuffer->GetSize());
                    bDeleteString = TRUE;
                }
            }
            else
            {
                bAddToHeader = bStandardSDP ? FALSE : TRUE;
                if (bAddToHeader)
                {
                pszData = EscapeBuffer(pString, pPropBuffer->GetSize());
                bDeleteString = TRUE;
            }
            }

            if(bAddToHeader)
            {
                HX_ASSERT(pszData);
                ulLen = dataSize + strlen(pPropName) + 64;
                NEW_FAST_TEMP_STR(pszAttBuf, 4096, ulLen);
                SafeSprintf(pszAttBuf, ulLen, "a=%s:string;\"%s\"%s",
                    pPropName, pszData, pszEOL);
                headerAttributes += pszAttBuf;
                DELETE_FAST_TEMP_STR(pszAttBuf);
            }

            if (bDeleteString)
            {
                HX_VECTOR_DELETE(pszData);
            }
        }

        HX_RELEASE(pPropBuffer);
        rc = pValueArray[0]->GetNextPropertyCString(pPropName,
                                                    pPropBuffer);
    }

    // Determine the expected packet overhead size for b=AS
    ulPktHead = !bTCP ? UDP_HEAD_SZ + ETH_HEAD_SZ :
                TCP_HEAD_SZ + ETH_HEAD_SZ;

    ulPktHead += !bIsIN6 ? IP4_HEAD_SZ : IP6_HEAD_SZ;

    // Use RDT for bIsRealDatatype (since RDT required) or if
    // the request came from a helix client
    ulPktHead += (bIsRealDatatype || (ulClientType & SDP_CLIENT_HLX)) ?
                RDT_HEAD_SZ : RTP_HEAD_SZ;

    if (bIsLive)
    {
        bUnspecifiedRange = TRUE;
    }

    /*
     *  Session level a=range
     */
    headerAttributes += "a=range:npt=0-";
    if (!bUnspecifiedRange)
    {
        // Add finite range for normal on-demand content
        NPTime npTime(ulSessionDuration);
        const char* pszTime = (const char*)npTime;
        headerAttributes += pszTime;

    }
    else if (bUseOldSdp)
    {
        // 0-0 for ancient clients
        headerAttributes += "0";
    }
    // Leave open-ended 0- range for live or unspecified duration
    headerAttributes += pszEOL;


    // Add a title in s=
    const char* pActualTitle = pszTitle;
    INT32 lTitleLen = 0;
    IHXBuffer* pDefaultTitle = NULL;

    // if there was no Title, check the registry for a default
    if (!pActualTitle && m_pRegistry)
    {
        if (SUCCEEDED(m_pRegistry->GetStrByName(SDP_TITLE_CFG, pDefaultTitle)))
        {
            pActualTitle = (char*)pDefaultTitle->GetBuffer();
        }

        // if no default, look for a file name size limit
        else if (SUCCEEDED(m_pRegistry->GetIntByName(SDP_TITLE_LEN_CFG,
                lTitleLen)) && lTitleLen < 0)
        {
            lTitleLen = 0;
        }
    }

    // If no title from file header or SDPDefaultTitle
    // configured default then get the file name
    if (!pActualTitle && (pReqURL || pContentBase))
    {
        if (pReqURL)
        {
            pActualTitle = (char*)pReqURL->GetBuffer();
        }
        else if (pContentBase)
        {
            pActualTitle = (char*)pContentBase->GetBuffer();
        }

        const char* pTitleStart;
        INT32 lLen = 0;
        const char* pTitleEnd = strchr(pActualTitle, '?');

        // If we don't have query params, it goes to the end
        // minus a trailing '/' if present
        if (!pTitleEnd)
        {
            lLen = strlen(pActualTitle);
            if (lLen > 1 && pActualTitle[lLen-1] == '/')
            {
                lLen--;
            }
            pTitleEnd = pActualTitle + lLen;
        }

        // Search back for the preceding '/' to find the start
        // of the file name
        for (pTitleStart = pTitleEnd;
             pTitleStart > pActualTitle && *(pTitleStart-1) != '/';
             --pTitleStart)
        {
        }

        // Calculate the length, but if we have a max length
        // from the config, don't go past that.
        lLen = pTitleEnd - pTitleStart;
        if (!lTitleLen || lLen < lTitleLen)
        {
            lTitleLen = lLen;
        }

        pActualTitle = pTitleStart;
    }

    // If still no title, we will just use a single space
    if (!pActualTitle)
    {
        pActualTitle = (char*)" ";
        lTitleLen = 1;
    }

    if (!lTitleLen)
    {
        lTitleLen = strlen(pActualTitle);
    }

    ulLen = 2 + lTitleLen + sizeof(pszEOL);
    NEW_FAST_TEMP_STR(pszTitleStr, 256, ulLen);
    sprintf(pszTitleStr, "s=%.*s%s", lTitleLen, pActualTitle, pszEOL); /* Flawfinder: ignore */
    mDesc += pszTitleStr;
    DELETE_FAST_TEMP_STR(pszTitleStr);

    HX_RELEASE(pDefaultTitle);

    if (sessionInfo.pszInfo)
    {
        ulLen = strlen(sessionInfo.pszInfo) + 64;
        NEW_FAST_TEMP_STR(pszTmpStr, 256, ulLen);
        SafeSprintf(pszTmpStr, ulLen, "i=%s%s", sessionInfo.pszInfo, pszEOL);
        mDesc += pszTmpStr;
        DELETE_FAST_TEMP_STR(pszTmpStr);
    }
    else
    {
        const char* pszDefaultAuthor = "<No author>";
        const char* pszDefaultCopyright = "<No copyright>";
        ulLen = 64;

        ulLen += pszAuthor ? strlen(pszAuthor) : 16;
        ulLen += pszCopyright ? strlen(pszCopyright) : 16;

        NEW_FAST_TEMP_STR(pszTmpString, 512, ulLen);

        SafeSprintf(pszTmpString, ulLen, "i=%s %s%s",
            pszAuthor ? pszAuthor : pszDefaultAuthor,
            pszCopyright ? pszCopyright : pszDefaultCopyright,
            pszEOL);

        mDesc += pszTmpString;
        DELETE_FAST_TEMP_STR(pszTmpString);
    }

    if (pszTitle)
        HX_VECTOR_DELETE(pszTitle);

    if (pszAuthor)
        HX_VECTOR_DELETE(pszAuthor);

    if (pszCopyright)
        HX_VECTOR_DELETE(pszCopyright);

    /* Format the connection line only if MulticastAddress and MulticastTTL
     * exist in the file header
     */
    if (sessionInfo.pszConnAddr && strlen(sessionInfo.pszConnAddr) &&
        sessionInfo.bConnTTLFound)
    {
        ulLen = strlen(sessionInfo.pszConnAddr) + 64;
        NEW_FAST_TEMP_STR(pszTmpString, 256, ulLen);
        if (sessionInfo.bConnRangeFound)
        {
            SafeSprintf(pszTmpString, ulLen, "c=IN %s %s/%d/%d%s",
                bIsIN6 ? "IP6" : "IP4",
                sessionInfo.pszConnAddr, sessionInfo.ulConnTTL,
                sessionInfo.ulConnRange, pszEOL);
        }
        else
        {
            SafeSprintf(pszTmpString, ulLen, "c=IN %s %s/%d%s",
                bIsIN6 ? "IP6" : "IP4",
                sessionInfo.pszConnAddr, sessionInfo.ulConnTTL, pszEOL);
        }

        mDesc += pszTmpString;

        DELETE_FAST_TEMP_STR(pszTmpString);
    }
    else if (!bUseOldSdp)
    {
        // we still need a c= line with a NULL value (RFC 2326 C.1.7)
        // XXXGo
        // Since adding this line w/o TTL (i.e. 0.0.0.0/ttl) will cause
        // older sdpplin to skip the next line (due to a wrong parsing),
        // add this only when we are using new sdp type.
        if (bIsIN6)
        {
            mDesc += "c=IN IP6 ::0";
        }
        else
        {
            mDesc += "c=IN IP4 0.0.0.0";
        }
        mDesc += pszEOL;
    }

    mDesc += "t=0 0";
    mDesc += pszEOL;

    /* add sdpplin version */
    SafeSprintf(psz256, 256, "a=SdpplinVersion:%u%s", m_ulVersion, pszEOL);
    mDesc += psz256;

    // a=StreamCount needed for legacy splitting
    // should be set to the number of stream groups
    // when in use, otherwise just the number of logical streams
    UINT32 ulActualStreams = bFoundStreamGroupCount ? ulStreamGroupCount :
        ulStreamCount;

    SafeSprintf(psz256, 256, "a=StreamCount:integer;%u%s", ulActualStreams, pszEOL);
    mDesc += psz256;

    mDesc += headerAttributes;

    // expand SDPData if it's there...
    if (sessionInfo.pSDPData)
    {
        IHXBuffer* pLine;
        LISTPOSITION pos = sessionInfo.pSDPData->GetHeadPosition();
        while (pos)
        {
            pLine = (IHXBuffer*) sessionInfo.pSDPData->GetNext(pos);
            if (pLine)
            {
                mDesc += (char*)pLine->GetBuffer();
                mDesc += pszEOL;
            }
        }
    }

    // Clean up the session info object
    sessionInfo.Cleanup();

    BOOL bUseStreamGroups = FALSE;
    UINT32 ulNumMediaLines = nValues - 2;
    CHXSimpleList** pStreamGroups = NULL;

    // If we have stream groups, pull them and generate alt streams
    if (bFoundStreamGroupCount)
    {
        rc = GetStreamGroups(nValues, pValueArray, ulStreamGroupCount,
            pStreamGroups);

        if (SUCCEEDED(rc))
        {
            bUseStreamGroups = TRUE;
            ulNumMediaLines = ulStreamGroupCount;
        }
    }


    IHXValues* pStreamHeader = NULL;
    BOOL bDefault = TRUE;
    char szAltPrefix[32]; // "a=alt:<alt-id>:" /* Flawfinder: ignore */
    MediaInfo defaultInfo;
    MediaInfo altInfo;
    MediaInfo* pStreamInfo = NULL;

    UINT32 ulStreamNumber = 0;
    HXBOOL bFoundStreamNum = FALSE;

    HXBOOL bEndTimeFound = FALSE;

    for(UINT32 i = 0; i < ulNumMediaLines; )
    {
        pStreamInfo = &defaultInfo;
        if (bUseStreamGroups)
        {
            pStreamHeader = (IHXValues*)pStreamGroups[i]->RemoveHead();
            if (!bDefault)
            {
                pStreamInfo = &altInfo;
            }
        }
        else
        {
            pStreamHeader = pValueArray[i + 2];
        }

        if(pStreamHeader)
        {
            CHXString streamAttributes;
            streamAttributes.SetMinBufSize(8192);

            // Initialize everything to 0/FALSE/NULL
            memset(pStreamInfo, 0, sizeof(MediaInfo));

            rc = pStreamHeader->GetFirstPropertyULONG32(pPropName, propValue);
            while(rc == HXR_OK)
            {
                bAddToHeader = FALSE;

                if(strcasecmp(pPropName, "StreamNumber") == 0)
                {
                   bFoundStreamNum = TRUE;
                   ulStreamNumber = propValue;

                    // Do not use StreamNumber as ControlID if we
                    // have found TrackID already
                    if (!pStreamInfo->bControlIDFound)
                    {
                        pStreamInfo->ulControlID = propValue;
                        pStreamInfo->bControlIDFound = TRUE;
                    }
                }
                else if(strcasecmp(pPropName, "Duration") == 0)
                {
                    pStreamInfo->ulDuration = propValue;
                    pStreamInfo->bDurationFound = TRUE;
                }
                else if(strcasecmp(pPropName, "RTPPayloadType") == 0)
                {
                    pStreamInfo->ulPayloadType = propValue;
                    pStreamInfo->bPayloadTypeFound = TRUE;
                }
                else if(strcasecmp(pPropName, "SamplesPerSecond") == 0)
                {
                    pStreamInfo->ulSampleRate = propValue;
                    pStreamInfo->bSampleRateFound = TRUE;
                }
                else if(strcasecmp(pPropName, "Channels") == 0)
                {
                    pStreamInfo->ulChannels = propValue;
                    pStreamInfo->bChannelsFound = TRUE;
                }
                else if(strcasecmp(pPropName, "MulticastTTL") == 0)
                {
                    pStreamInfo->ulConnTTL = propValue;
                    pStreamInfo->bConnTTLFound = TRUE;
                }
                else if(strcasecmp(pPropName, "MulticastRange") == 0)
                {
                    pStreamInfo->ulConnRange = propValue;
                    pStreamInfo->bConnRangeFound = TRUE;
                }
                else if(strcasecmp(pPropName, "Port") == 0)
                {
                    pStreamInfo->ulPort = propValue;
                }
                else if(strcasecmp(pPropName, "ptime") == 0)
                {
                    pStreamInfo->ulPtime = propValue;
                    pStreamInfo->bPtimeFound = TRUE;
                }
                else if(strcasecmp(pPropName, "AvgBitRate") == 0)
                {
                    pStreamInfo->ulAvgBitRate = propValue;
                    pStreamInfo->bAvgBitRateFound = TRUE;

                    // add it to default description only
                    bAddToHeader = bDefault;
                }
                else if(strcasecmp(pPropName, "MaxBitRate") == 0)
                {
                    pStreamInfo->ulMaxBitRate = propValue;
                    pStreamInfo->bMaxBitRateFound = TRUE;

                    // add it to default description only
                    bAddToHeader = bDefault;
                }
                else if (strcasecmp(pPropName, "AvgPacketSize") == 0)
                {
                    pStreamInfo->ulAvgPacketSize = propValue;
                    pStreamInfo->bAvgPacketSizeFound = TRUE;

                    // add it to default description only
                    bAddToHeader = bDefault;
                }
                else if(strcasecmp(pPropName, "RtcpRRRate") == 0)
                {
                    pStreamInfo->bRTCPRRFound = TRUE;
                    pStreamInfo->ulRTCPRR = propValue;
                }
                else if(strcasecmp(pPropName, "RtcpRSRate") == 0)
                {
                    pStreamInfo->bRTCPRSFound = TRUE;
                    pStreamInfo->ulRTCPRS = propValue;
                }
                else if(strcasecmp(pPropName, "UseAnnexG") == 0)
                {
                    pStreamInfo->bUseAnnexG = propValue ? TRUE : FALSE;
                }
                else if(strcasecmp(pPropName, "X-PreDecBufSize") == 0)
                {
                    pStreamInfo->bPreDecBufSizeFound = TRUE;
                    pStreamInfo->ulPreDecBufSize = propValue;
                }
                else if(strcasecmp(pPropName, "X-InitPreDecBufPeriod") == 0)
                {
                    pStreamInfo->bPreDecBufPeriodFound = TRUE;
                    pStreamInfo->ulPreDecBufPeriod = propValue;
                }
                else if(strcasecmp(pPropName, "X-InitPostDecBufPeriod") == 0)
                {
                    pStreamInfo->bPostDecBufPeriodFound = TRUE;
                    pStreamInfo->ulPostDecBufPeriod = propValue;
                }
                else if(strcasecmp(pPropName, "X-DecByteRate") == 0)
                {
                    pStreamInfo->bDecByteRateFound = TRUE;
                    pStreamInfo->ulDecByteRate = propValue;
                }
                else if(strcasecmp(pPropName, "TrackID") == 0)
                {
                    // Always use TrackID as ControlID if found
                    pStreamInfo->ulControlID = propValue;
                    pStreamInfo->bControlIDFound = TRUE;

                }
                else if (strcasecmp(pPropName, "FrameHeight") == 0)
                {
                    pStreamInfo->ulFrameHeight = propValue;
                    pStreamInfo->bFrameHeightFound = TRUE;
                }
                else if (strcasecmp(pPropName, "FrameWidth") == 0)
                {
                    pStreamInfo->ulFrameWidth = propValue;
                    pStreamInfo->bFrameWidthFound = TRUE;
                }
                else if (strcasecmp(pPropName, "EndTime") == 0)
                {
		    bEndTimeFound = TRUE;
                    bAddToHeader = FALSE;
                }
                else if(strcasecmp(pPropName, "StreamGroupNumber") == 0 ||
                        strcasecmp(pPropName, "SwitchGroupID") == 0 ||
                        strcasecmp(pPropName, "DefaultStream") == 0 ||
                        strcasecmp(pPropName, "ServerPreroll") == 0 ||
                        strcasecmp(pPropName, "BaseRule") == 0)
                {
                    // we don't need this but don't add it to the SDP
                }

                // Unknown header values - only one per m=
                //Add Unknown values only if it is not a standardSDP
                else if (bDefault && !bStandardSDP)
                {
                    bAddToHeader = TRUE;
                }

                if (bAddToHeader)
                {
                    // No alt values allowed here!
                    // If we need to add one, we have to make sure we have
                    // the TrackID first.
                    HX_ASSERT(bDefault);
                    ulLen = strlen(pPropName) + 64;
                    NEW_FAST_TEMP_STR(pszAttBuf, 4096, ulLen);
                    SafeSprintf(pszAttBuf, ulLen, "a=%s:integer;%d%s",
                        pPropName, propValue, pszEOL);
                    streamAttributes += pszAttBuf;
                    DELETE_FAST_TEMP_STR(pszAttBuf);
                }

                /* make sure to reset SdpFileType header */
                if (strncasecmp(pPropName, "SdpFileType", 11) == 0)
                {
                    pStreamHeader->SetPropertyULONG32(pPropName, NONE_SDP);
                }
                rc = pStreamHeader->GetNextPropertyULONG32(pPropName,
                    propValue);
            }

            // Create the alt attr prefix if this is an alt stream
            // "a=alt:<alt-id>:"
            if (bDefault)
            {
                szAltPrefix[0] = '\0';
            }
            else
            {
                SafeSprintf(szAltPrefix, 32, "a=alt:%u:",
                            pStreamInfo->ulControlID);
            }

            /**
             * Add the StreamNumber for every stream in the SDP.
             * This is beacuse the streamnumber should be in sync for both
             * proxy and the server. We are storing the stream numbers 
             * so that the same stream numbers can be used by the proxy while 
             * creating headers from SDP. (Part of LRA)
             **/
            const char* pBCNGUser = "RealNetworks Broadcast Receiver";
            if (pUserAgent && !(strncmp((const char*)pUserAgent->GetBuffer(), pBCNGUser, 
                                                         strlen(pBCNGUser))))
            {                       
                HX_ASSERT(bFoundStreamNum);
                char* pParamName = "X-Hlx-StreamNum";
                ulLen = strlen(pParamName) + 64;
                NEW_FAST_TEMP_STR(pszAttBuf, 4096, ulLen);
                SafeSprintf(pszAttBuf, ulLen, "%sa=%s:integer;%d%s", szAltPrefix, 
                                                   pParamName, ulStreamNumber, pszEOL);
                streamAttributes += pszAttBuf;
                DELETE_FAST_TEMP_STR(pszAttBuf);
            }

            rc = pStreamHeader->GetFirstPropertyBuffer(pPropName, pPropBuffer);
            while(rc == HXR_OK)
            {
                // Unknown header values - only one per m=
                if (bDefault && !bStandardSDP)
                {
                    INT32 dataSize = pPropBuffer->GetSize();
                    ulLen = dataSize * 2 + strlen(pPropName) + 64;
                    NEW_FAST_TEMP_STR(pszAttBuf, 4096, ulLen);
                    NEW_FAST_TEMP_STR(pszPropString, 4096, dataSize * 2 + 64);

                    (void)BinTo64((const BYTE*)pPropBuffer->GetBuffer(),
                        dataSize, pszPropString);

                    SafeSprintf(pszAttBuf, ulLen, "%sa=%s:buffer;\"%s\"%s",
                                szAltPrefix, pPropName, pszPropString, pszEOL);

                    streamAttributes += pszAttBuf;
                    DELETE_FAST_TEMP_STR(pszPropString);
                    DELETE_FAST_TEMP_STR(pszAttBuf);
                }
                HX_RELEASE(pPropBuffer);

                rc = pStreamHeader->GetNextPropertyBuffer(pPropName,
                    pPropBuffer);
            }

            rc = pStreamHeader->GetFirstPropertyCString(pPropName,
                pPropBuffer);
            while(rc == HXR_OK)
            {
                char* pString = (char*) pPropBuffer->GetBuffer();
                char* pszData=NULL;
                BOOL bDeleteString = FALSE;
                int len = pPropBuffer->GetSize();

                bAddToHeader = FALSE;

                if(strcasecmp(pPropName, "MimeType") == 0)
                {
                    pStreamInfo->pszMimeType =
                        EscapeBuffer(pString, pPropBuffer->GetSize());

                    pStreamInfo->pszMimeLast =
                        strchr(pStreamInfo->pszMimeType, '/');
                    if(pStreamInfo->pszMimeLast)
                    {
                        int MFLen = pStreamInfo->pszMimeLast -
                                    pStreamInfo->pszMimeType;

                        pStreamInfo->pszMimeLast++;
                        pStreamInfo->pszMimeFirst = new char [ MFLen + 1 ];
                        memcpy(pStreamInfo->pszMimeFirst,
                               pStreamInfo->pszMimeType,
                               MFLen); /* Flawfinder: ignore */
                        pStreamInfo->pszMimeFirst[MFLen] = '\0';
                    }
                    else
                    {
                        // mimetype is not in the right format...
                    }
                }
                else if(strcasecmp(pPropName, "MulticastAddress") == 0)
                {
                    pStreamInfo->pszConnAddr =
                        EscapeBuffer(pString, pPropBuffer->GetSize());
                }
                else if(strcasecmp(pPropName, "Control") == 0)
                {
                    // don't allow control strings if we have
                    // stream groups, the control ID has to
                    // match the alt ID
                    HX_ASSERT(!bUseStreamGroups);
                    if (!bUseStreamGroups)
                    {
                        HX_RELEASE(pControlStringBuff);
                        pControlStringBuff = pPropBuffer;
                        pControlStringBuff->AddRef();
                    }
                }
                else if(strcasecmp(pPropName, "PayloadParameters") == 0)
                {
                    pStreamInfo->pszFmtp =
                        EscapeBuffer(pString, pPropBuffer->GetSize());
                }
                else if(strcasecmp(pPropName, "SDPData") == 0)
                {
                    GetSDPData(pPropBuffer, pStreamInfo);
                }
                else if(strcasecmp(pPropName, "Information") == 0)
                {
                    // "i="
                    pStreamInfo->pszInfo =
                        EscapeBuffer(pString, pPropBuffer->GetSize());
                }
                else if (strcasecmp(pPropName, "ASMRuleBook") == 0)
                {
                    bAddToHeader = bDefault;

                    if (bAddToHeader)
                    {
                        pszData = EscapeBuffer(pString, pPropBuffer->GetSize());
                        bDeleteString = TRUE;
                    }
                }
                else if (strcasecmp(pPropName, "PayloadWirePacket") == 0 &&
                         strcasecmp((const char*)pPropBuffer->GetBuffer(),
                                    "RTP") == 0)
                {
                    pStreamInfo->bPayloadWirePacket = TRUE;
                    bAddToHeader = bDefault;
                    if(bAddToHeader)
                    {
                        pszData = EscapeBuffer(pString, pPropBuffer->GetSize());
                        bDeleteString = TRUE;
                    }
                }
                // Unknown header values - only one per m=, no alts
                else if (bDefault && !bStandardSDP)
                {
                    pszData = EscapeBuffer(pString, pPropBuffer->GetSize());
                    bDeleteString = TRUE;
                    bAddToHeader = TRUE;
                }

                if(bAddToHeader)
                {
                    ulLen = strlen(pszData) + strlen(pPropName) + 64;
                    NEW_FAST_TEMP_STR(pszAttBuf, 4096, ulLen);
                    SafeSprintf(pszAttBuf, ulLen, "%sa=%s:string;\"%s\"%s",
                        szAltPrefix, pPropName, pszData, pszEOL);
                    streamAttributes += pszAttBuf;
                    DELETE_FAST_TEMP_STR(pszAttBuf);
                }

                if (bDeleteString)
                {
                    HX_VECTOR_DELETE(pszData);
                }

                HX_RELEASE(pPropBuffer);

                rc = pStreamHeader->GetNextPropertyCString(pPropName,
                                                            pPropBuffer);
            }

            if(!pStreamInfo->bPayloadTypeFound)
            {
                pStreamInfo->ulPayloadType = RTP_PAYLOAD_RTSP;
            }

            // m=
            if (bDefault)
            {
                const char* pszName = (const char*)pStreamInfo->pszMimeFirst;
                if (bUseOldSdp)
                {
                    // our old sdpplin expects m= to have either "audio",
                    // "video", or "data", and anything else would be BAD!
                    if (strcmp(pStreamInfo->pszMimeFirst, "audio") &&
                        strcmp(pStreamInfo->pszMimeFirst, "video"))
                    {
                        pszName = "data";
                    }
                }
                else if (!pszName)
                {
                    pszName = "data";
                }

                ulLen = strlen(pszName) + 128;
                NEW_FAST_TEMP_STR(pszTmpString, 256, ulLen);
                SafeSprintf(pszTmpString, ulLen, "m=%s %d RTP/AVP %d%s",
                    pszName, pStreamInfo->ulPort, pStreamInfo->ulPayloadType,
                    pszEOL);
                mDesc += pszTmpString;
                DELETE_FAST_TEMP_STR(pszTmpString);
            }

            // i=
            if (pStreamInfo->pszInfo &&
                (bDefault || !defaultInfo.pszInfo ||
                strcmp(pStreamInfo->pszInfo, defaultInfo.pszInfo) != 0))
            {
                ulLen = strlen(pStreamInfo->pszInfo) + 64;
                NEW_FAST_TEMP_STR(pszTmpStr, 256, ulLen);
                SafeSprintf(pszTmpStr, ulLen, "%si=%s%s",
                    szAltPrefix, pStreamInfo->pszInfo, pszEOL);
                mDesc += pszTmpStr;
                DELETE_FAST_TEMP_STR(pszTmpStr);
            }

            /* Format the connection line only if MulticastAddress and
             * MulticastTTL exist in the stream header
             */
            // c=
            if (pStreamInfo->bConnTTLFound && pStreamInfo->pszConnAddr
                && pStreamInfo->pszConnAddr[0])
            {
                pStreamInfo->pszConnection = new char[256];
                if (pStreamInfo->pszConnection)
                {
                    if (pStreamInfo->bConnRangeFound)
                    {
                        SafeSprintf(pStreamInfo->pszConnection, 256,
                            "c=IN IP4 %s/%d/%d%s",
                            pStreamInfo->pszConnAddr, pStreamInfo->ulConnTTL,
                            pStreamInfo->ulConnRange, pszEOL);
                    }
                    else
                    {
                        SafeSprintf(pszTmpString, 256,"c=IN IP4 %s/%d%s",
                            pStreamInfo->pszConnAddr, pStreamInfo->ulConnTTL,
                            pszEOL);
                    }
                }

                if (bDefault || !defaultInfo.pszConnection ||
                    strcmp(pStreamInfo->pszConnection,
                    defaultInfo.pszConnection) != 0)
                {
                    mDesc += szAltPrefix;
                    mDesc += pszTmpString;
                }
            }

            // Packet rate is AvgBitRate/AvgPacketSize packets/sec
            double dPacketRate = SDP_DEFAULT_PACKET_RATE;
            if (pStreamInfo->ulAvgPacketSize && 
                pStreamInfo->bAvgBitRateFound)
            {
                dPacketRate = (double)pStreamInfo->ulAvgBitRate / 
                    ((double)pStreamInfo->ulAvgPacketSize * 8.0);
            }

            // b=AS
            // If using PayloadWirePackets (RTP live), just pass through.
            // Only modify by the multiplier.
            if (pStreamInfo->bPayloadWirePacket)
            {
                pStreamInfo->ulASBitRate = pStreamInfo->ulAvgBitRate;
                UINT32 ulASRate = (UINT32)(dBWMult *
                        ((double)pStreamInfo->ulAvgBitRate / 1000.0) + .5);
                SafeSprintf(psz256, 256,"%sb=AS:%u%s",
                            szAltPrefix, ulASRate, pszEOL);
                mDesc += psz256;
            }

            // Otherwise, estimate packet overhead and add it to the
            // media bitrate to calculate b=AS
            else
            {
                if (pStreamInfo->bMaxBitRateFound)
                {
                    pStreamInfo->ulASBitRate = pStreamInfo->ulMaxBitRate;
                }
                else if (pStreamInfo->bAvgBitRateFound)
                {
                    pStreamInfo->ulASBitRate = pStreamInfo->ulAvgBitRate;
                }

                if (pStreamInfo->ulASBitRate && (bDefault ||
                    pStreamInfo->ulASBitRate != defaultInfo.ulASBitRate))
                {
                    // Add overhead and multiplier and convert to kbps
                    UINT32 ulASRate = (UINT32)(dBWMult *
                        (((double)pStreamInfo->ulASBitRate +
                        (double)ulPktHead * dPacketRate) / 1000.0) + .5);

                    SafeSprintf(psz256, 256,"%sb=AS:%u%s",
                            szAltPrefix, ulASRate, pszEOL);
                    mDesc += psz256;
                }
            }

            // add TIAS
            SafeSprintf(psz256, 256, "%sb=TIAS:%u%s", 
                szAltPrefix, pStreamInfo->ulASBitRate, pszEOL);
            mDesc += psz256;

            // b=RR
            if (!pStreamInfo->bRTCPRRFound)
            {
                if (pStreamInfo->bAvgBitRateFound)
                {
                    // Default to 3.75%
                    pStreamInfo->ulRTCPRR =
                        (UINT32)pStreamInfo->ulAvgBitRate * .0375;
                }
                else
                {
                    pStreamInfo->ulRTCPRR = 1024;
                }
            }
            if (bDefault || pStreamInfo->ulRTCPRR != defaultInfo.ulRTCPRR)
            {
                SafeSprintf(psz256, 256,"%sb=RR:%u%s",
                    szAltPrefix, pStreamInfo->ulRTCPRR, pszEOL);
                mDesc += psz256;
            }

            //b=RS
            if (!pStreamInfo->bRTCPRSFound)
            {
                if (pStreamInfo->bAvgBitRateFound)
                {
                    // Default to 1.25%
                    pStreamInfo->ulRTCPRS = pStreamInfo->ulAvgBitRate / 80;
                }
                else
                {
                    pStreamInfo->ulRTCPRS = 512;
                }
            }
            if (bDefault || pStreamInfo->ulRTCPRS != defaultInfo.ulRTCPRS)
            {
                SafeSprintf(psz256, 256,"%sb=RS:%u%s",
                    szAltPrefix, pStreamInfo->ulRTCPRS, pszEOL);
                mDesc += psz256;
            }

            // add maxprate
            SafeSprintf(psz256, 256, "%sa=maxprate:%f%s", 
                szAltPrefix, dPacketRate, pszEOL);
            mDesc += psz256;

            // a=control
            if (pControlStringBuff)
            {
                HX_ASSERT(!bUseStreamGroups);
                char* pString = (char*) pControlStringBuff->GetBuffer();

                if (pContentBase || pQueryParams)
                {
                    UINT32 ulBuffLen = 64;
                    ulBuffLen += pContentBase ? pContentBase->GetSize() : 0;
                    ulBuffLen += (pQueryParams) ? pQueryParams->GetSize() : 0;

                    NEW_FAST_TEMP_STR(pszControlString, 512, ulBuffLen);
                    SafeSprintf(pszControlString, ulBuffLen,
                        "%sa=control:%s%s%s%s", szAltPrefix,
                        pContentBase ? ((char*)pContentBase->GetBuffer()) : "",
                        pControlStringBuff->GetBuffer(),
                        (pQueryParams)?((char*)pQueryParams->GetBuffer()) : "",
                        pszEOL);

                    mDesc += pszControlString;
                    DELETE_FAST_TEMP_STR(pszControlString);
                }
                else
                {
                    char* pszData = EscapeBuffer(pString,
                        pControlStringBuff->GetSize());
                    UINT32 ulBuffLen = strlen(pszData) + 64;
                    NEW_FAST_TEMP_STR(pszControlString, 512, ulBuffLen);
                    SafeSprintf(pszControlString, ulBuffLen, "%sa=control:%s%s",
                        szAltPrefix, pszData, pszEOL);

                    mDesc += pszControlString;
                    DELETE_FAST_TEMP_STR(pszControlString);
                    HX_VECTOR_DELETE(pszData);
                }
            }
            else if (pStreamInfo->bControlIDFound)
            {
                const char* pszFieldPrefix = "a=control:";
                const char* pszStreamLabel = "streamid=";

                if (pContentBase || pQueryParams)
                {
                    UINT32 ulBuffLen = 64;
                    ulBuffLen += pContentBase ? (pContentBase->GetSize()) : 0;
                    ulBuffLen += (pQueryParams) ? (pQueryParams->GetSize()) : 0;

                    NEW_FAST_TEMP_STR(pszControlField, 256, ulBuffLen);
                    SafeSprintf(pszControlField, ulBuffLen, "%s%s%s%s%u%s%s",
                        szAltPrefix, pszFieldPrefix,
                        pContentBase ?((char*)pContentBase->GetBuffer()) : "",
                        pszStreamLabel, pStreamInfo->ulControlID,
                        pQueryParams ?((char*)pQueryParams->GetBuffer()) : "",
                        pszEOL);

                    mDesc += pszControlField;
                    DELETE_FAST_TEMP_STR(pszControlField);
                }
                else
                {
                    SafeSprintf(psz256, 256,"%s%s%s%d%s",
                        szAltPrefix, pszFieldPrefix, pszStreamLabel,
                        pStreamInfo->ulControlID, pszEOL);
                    mDesc += psz256;
                }
            }

            // a=length && a=range
            // add for default/lone streams always
            // add for alt streams only if it does not match the default
            if (bDefault ||
                (pStreamInfo->bDurationFound && defaultInfo.bDurationFound
                && pStreamInfo->ulDuration != defaultInfo.ulDuration))

            {
                NPTime npTime("0");
                mDesc += "a=range:npt=0-";
                const char* szLen = (const char*)npTime;
                if (!bUnspecifiedRange && pStreamInfo->bDurationFound)
                {
                    // Add stream's end range
                    npTime = NPTime(pStreamInfo->ulDuration);
                    szLen = (const char*)npTime;
                    mDesc += szLen;
                }
                else if(!bUnspecifiedRange && bDefaultDurationFound)
                {
                    // Add session's end range
                    npTime = NPTime(ulSessionDuration);
                    szLen = (const char*)npTime;
                    mDesc += szLen;
                }
                else if (bUseOldSdp)
                {
                    // Really old client needs "0-0"
                    mDesc += "0";
                }

                // Leave open-ended 0-0 range for live or unspecified duration
                mDesc += pszEOL;
                mDesc += (const char*) "a=length:npt=";
                mDesc += szLen;
                mDesc += pszEOL;

                if (bEndTimeFound)
                {
                    INT32 nEndTime = pStreamInfo->ulDuration;
                    SafeSprintf(psz256, 256,"a=EndTime:integer;%d%s",
                                                             nEndTime , pszEOL);
                    mDesc += psz256;
                }
            }

            // a=rtpmap:
            if (pStreamInfo->pszMimeLast)
            {
                // Get static payload type sample rate and channels
                if (SDPIsStaticPayload(pStreamInfo->ulPayloadType))
                {
                    if (!pStreamInfo->bSampleRateFound)
                    {
                        pStreamInfo->ulSampleRate =
                            SDPMapPayloadToSamplesPerSecond(
                            pStreamInfo->ulPayloadType);
                        if (pStreamInfo->ulSampleRate)
                        {
                            pStreamInfo->bSampleRateFound = TRUE;
                        }
                    }
                    if (!pStreamInfo->bChannelsFound)
                    {
                        pStreamInfo->ulChannels = SDPMapPayloadToChannels(
                            pStreamInfo->ulPayloadType);
                        if (pStreamInfo->ulChannels)
                        {
                            pStreamInfo->bChannelsFound = TRUE;
                        }
                    }
                }
            }
            if (!pStreamInfo->bSampleRateFound)
            {
                pStreamInfo->ulSampleRate = 1000;
            }

            // Need to add rtpmap if default stream or if payload type
            // sample rate, channels, or mime type has changed
            if (bDefault ||
                pStreamInfo->ulPayloadType != defaultInfo.ulPayloadType ||
                pStreamInfo->ulSampleRate != defaultInfo.ulSampleRate ||
                pStreamInfo->bChannelsFound != defaultInfo.bChannelsFound ||
                (pStreamInfo->bChannelsFound &&
                pStreamInfo->ulChannels != defaultInfo.ulChannels) ||
                strcmp(pStreamInfo->pszMimeLast, defaultInfo.pszMimeLast) != 0)
            {
                SafeSprintf(psz256, 256, "%sa=rtpmap:%d %s/%u",
                    szAltPrefix, pStreamInfo->ulPayloadType,
                    pStreamInfo->pszMimeLast, pStreamInfo->ulSampleRate);
                mDesc += psz256;

                if (pStreamInfo->bChannelsFound)
                {
                    SafeSprintf(psz256, 256,"/%d%s", pStreamInfo->ulChannels,
                        pszEOL);
                    mDesc += psz256;
                }
                else
                {
                    mDesc += pszEOL;
                }
            }

            // a=fmtp
            if (pStreamInfo->pszFmtp && (bDefault || !defaultInfo.pszFmtp ||
                strcmp(pStreamInfo->pszFmtp, defaultInfo.pszFmtp) != 0))
            {
                SafeSprintf(psz256, 256, "%sa=fmtp:%d %s%s", szAltPrefix,
                    pStreamInfo->ulPayloadType, pStreamInfo->pszFmtp, pszEOL);
                mDesc += psz256;
            }
            else if (!bUseOldSdp && bDefault)
            {
                // 3GPP requires an fmtp field in every SDP file
                // Include this only if the client supports interop SDP
                // and SDPData doesn't already contain a=fmtp:
                pStreamInfo->pszFmtp = new char[32];
                if (pStreamInfo->pszFmtp)
                {
                    SafeSprintf(pStreamInfo->pszFmtp, 32, "a=fmtp:%u ",
                        pStreamInfo->ulPayloadType);
                    mDesc += pStreamInfo->pszFmtp;
                    mDesc += pszEOL;
                }
            }

            // a=ptime
            if (pStreamInfo->bPtimeFound &&
                (bDefault || !defaultInfo.bPtimeFound ||
                pStreamInfo->ulPtime != defaultInfo.ulPtime))
            {
                SafeSprintf(psz256, 256, "%sa=ptime:%u%s",
                    szAltPrefix, pStreamInfo->ulPtime, pszEOL);
                mDesc += psz256;
            }

            // a=mimetype needed for legacy splitting
            if (pStreamInfo->pszMimeType && (bDefault ||
                strcmp(pStreamInfo->pszMimeType, defaultInfo.pszMimeType)))
            {
                ulLen = strlen(pStreamInfo->pszMimeType) + 128;
                NEW_FAST_TEMP_STR(pszTmpStr, 256, ulLen);

                SafeSprintf(pszTmpStr, ulLen, "%sa=mimetype:string;\"%s\"%s",
                    szAltPrefix, pStreamInfo->pszMimeType, pszEOL);

                mDesc += pszTmpStr;
                DELETE_FAST_TEMP_STR(pszTmpStr);
            }

            // 3GPP AnnexG values
            // a=X-predecbufsize
            if (pStreamInfo->bPreDecBufSizeFound &&
                (bDefault || !defaultInfo.bPreDecBufSizeFound ||
                pStreamInfo->ulPreDecBufSize !=
                defaultInfo.ulPreDecBufSize))
            {
                SafeSprintf(psz256, 256, "%sa=X-predecbufsize:%u%s",
                    szAltPrefix, pStreamInfo->ulPreDecBufSize, pszEOL);
                mDesc += psz256;
            }
            // a=X-initpredecbufperiod
            if (pStreamInfo->bPreDecBufPeriodFound &&
                (bDefault || !defaultInfo.bPreDecBufPeriodFound ||
                pStreamInfo->ulPreDecBufPeriod !=
                defaultInfo.ulPreDecBufPeriod))
            {
                SafeSprintf(psz256, 256, "%sa=X-initpredecbufperiod:%u%s",
                    szAltPrefix, pStreamInfo->ulPreDecBufPeriod, pszEOL);
                mDesc += psz256;
            }
            // a=X-initpostdecbufperiod
            if (pStreamInfo->bPostDecBufPeriodFound &&
                (bDefault || !defaultInfo.bPostDecBufPeriodFound ||
                pStreamInfo->ulPostDecBufPeriod !=
                defaultInfo.ulPostDecBufPeriod))

            {
                SafeSprintf(psz256, 256, "%sa=X-initpostdecbufperiod:%u%s",
                    szAltPrefix, pStreamInfo->ulPostDecBufPeriod, pszEOL);
                mDesc += psz256;
            }
            // a=X-decbyterate
            if (pStreamInfo->bDecByteRateFound &&
                (bDefault || !defaultInfo.bDecByteRateFound ||
                pStreamInfo->ulDecByteRate != defaultInfo.ulDecByteRate))
            {
                SafeSprintf(psz256, 256, "%sa=X-decbyterate:%u%s",
                    szAltPrefix, pStreamInfo->ulDecByteRate, pszEOL);
                mDesc += psz256;
            }

            // a=framesize
            if (pStreamInfo->bFrameHeightFound && pStreamInfo->bFrameWidthFound && 
                (bDefault || !defaultInfo.bFrameHeightFound || 
                pStreamInfo->ulFrameHeight != defaultInfo.ulFrameHeight ||
                !defaultInfo.bFrameWidthFound || 
                pStreamInfo->ulFrameWidth != defaultInfo.ulFrameWidth) &&
                pStreamInfo->bPayloadTypeFound)
            {
                SafeSprintf(psz256, 256, "%sa=framesize:%u %u-%u%s",
                    szAltPrefix, pStreamInfo->ulPayloadType, pStreamInfo->ulFrameWidth,
                    pStreamInfo->ulFrameHeight, pszEOL);
                mDesc += psz256;
            }

            // a=3GPP-Adaptation-Support
            // Do not offer 3GPP-Adaptation-Support for Real Datatypes
            if (!bIsRealDatatype && g_ul3GPPNADUReportFreq && bDefault)
            {
                SafeSprintf(psz256, 256, "a=3GPP-Adaptation-Support:%u%s",
                    g_ul3GPPNADUReportFreq, pszEOL);
                mDesc += psz256;
            }

            // a=Helix-Adaptation-Support
            // always signal
            if (bDefault && g_bHelixRateAdaptationLicensed
                && g_bHelixRateAdaptationConfigured)
            {
                SafeSprintf(psz256, 256, "a=Helix-Adaptation-Support:1%s",
                    pszEOL);
                mDesc += psz256;
            }

            mDesc += streamAttributes;

            // expand SDPData if it's there...
            if (pStreamInfo->pSDPData)
            {
                IHXBuffer* pLine;
                char* szLine;
                LISTPOSITION pos = pStreamInfo->pSDPData->GetHeadPosition();
                while (pos)
                {
                    pLine = (IHXBuffer*) pStreamInfo->pSDPData->GetNext(pos);
                    if (pLine)
                    {
                        szLine = (char*)pLine->GetBuffer();

                        // if this line does not match one in the default
                        // SDPData then add it
                        if (bDefault || !defaultInfo.pSDPData ||
                            !HasSDPLine(defaultInfo.pSDPData, szLine))
                        {
                            if (!bDefault)
                            {
                                mDesc += szAltPrefix;
                            }
                            mDesc += szLine;
                            mDesc += pszEOL;
                        }
                    }
                }
            }
        }

        if (bUseStreamGroups)
        {
            if (!bDefault)
            {
                pStreamInfo->Cleanup();
            }

            // If this is the last header in this group, move
            // to the next stream group and cleanup this group's
            // default info
            if (pStreamGroups[i]->IsEmpty())
            {
                HX_DELETE(pStreamGroups[i]);

                i++;
                bDefault = TRUE;
                defaultInfo.Cleanup();
            }
            else
            {
                // a=alt-default-id
                if (bDefault)
                {
                    SafeSprintf(psz256, 256, "a=alt-default-id:%u%s",
                        pStreamInfo->ulControlID, pszEOL);
                    mDesc += psz256;
                }
                bDefault = FALSE;
            }
        }
        else
        {
            // Move to the next header
            i++;
            pStreamInfo->Cleanup();
        }
    }

    HX_RELEASE(pControlStringBuff);
    HX_RELEASE(pReqURL);
    HX_RELEASE(pContentBase);
    HX_RELEASE(pQueryParams);
    HX_RELEASE(pUserAgent);
    HX_RELEASE(pPSSVersion);
    HX_RELEASE(pHostname);

    HX_VECTOR_DELETE(pStreamGroups);

    if (mDesc.IsEmpty())
    {
        pDescription = NULL;
        return HXR_FAIL;
    }

    IHXBuffer* pBuffer = NULL;
    m_pCCF->CreateInstance(CLSID_IHXBuffer, (void**)&pBuffer);
    pBuffer->Set((BYTE*)(const char*)mDesc, mDesc.GetLength());

    pDescription = pBuffer;

    return HXR_OK;
}

BOOL SDPMediaDescGenerator::GetUseOldEOL() const
{
    return m_bUseOldEOL;
}

void SDPMediaDescGenerator::SetUseOldEOL(BOOL bUseOldEOL)
{
    m_bUseOldEOL = bUseOldEOL;
}

/*
 *  If there is no Duration in a file header and all streams have a
 *  duration, take the largest stream duration.
 *  If live, it is left unknown.
 */
UINT32
SDPMediaDescGenerator::GetSessionDuration(UINT16 nValues,
                                          IHXValues** ppValueArray)
{
    HX_RESULT theErr;
    UINT32 ulDuration = 0;

    // sanity check
    if (nValues < 3 || !ppValueArray)
    {
        HX_ASSERT(!"unexpected headers");
        return 0;
    }

    // Get duration from file header
    theErr = ppValueArray[0]->GetPropertyULONG32("Duration", ulDuration);
    if (SUCCEEDED(theErr))
    {
        return ulDuration;
    }

    UINT32 ulTmp = 0;

    /* find out if this is a live session */
    theErr = ppValueArray[0]->GetPropertyULONG32("LiveStream", ulTmp);
    if (SUCCEEDED(theErr) && 1 == ulTmp)
    {
        return MAX_UINT32;
    }

    BOOL bHasDuration = TRUE;
    for (UINT16 i = 2; i < nValues; ++i)
    {
        theErr = ppValueArray[i]->GetPropertyULONG32("Duration", ulTmp);
        if (SUCCEEDED(theErr))
        {
            // take the largest
            if (ulDuration < ulTmp)
            {
                ulDuration = ulTmp;
            }
        }
        else
        {
            // If any stream has no duration, leave duration as unknown
            ulDuration = MAX_UINT32;
            break;
        }
    }

    return ulDuration;
}

char*
SDPMediaDescGenerator::EscapeBuffer(const char* pBuffer, UINT32 len)
{
    UCHAR* buf;
    UINT32 newlen;
    UINT32 i = 0;
    UCHAR* tmp;
    UCHAR* newbuf;

    if(!pBuffer)
    {
        return 0;
    }

    buf = (UCHAR*)pBuffer;
    newlen = len;

    /*
     *  We have to do this in two passes.  One to alloc new mem to
     *  copy into, and the next to do the escape/ copy.
     *  First, count unescaped quotes and alloc.
     */
    tmp = buf;
    while(i < len)
    {
        if(*tmp == '\"')
        {
            newlen++;
        }
        if(*tmp == '\\')
        {
            tmp++;
            i++;
        }
        tmp++;
        i++;
    }

    newbuf = new UCHAR[newlen];
    tmp = newbuf;

    /*
     * Now copy into the new buffer and unescape as you go along.
     */
    i = 0;
    while(i < len)
    {
        if(*buf == '\"')
        {
            *tmp++ = '\\';
        }
        if(*buf == '\\')
        {
            *tmp++ = *buf++;
            i++;
        }
        *tmp++ = *buf++;
        i++;
    }

    /*
     * Set the buffer and return.
     */
    return (char*)newbuf;
}

void
SDPMediaDescGenerator::GetSDPData(IHXBuffer* pSDPDataBuf,
                                  MediaInfo* pStreamInfo)
{
    IHXBuffer* pLine = NULL;
    UINT32 ulBufSize = 0;
    UINT32 ulLength = 0;
    char* szSDPData = NULL;

    HX_RESULT rc = HXR_OUTOFMEMORY;

    pStreamInfo->pSDPData = new CHXSimpleList();
    if (pStreamInfo->pSDPData)
    {
        rc = pSDPDataBuf->Get((UCHAR*&)szSDPData, ulBufSize);
    }

    while (SUCCEEDED(rc) && ulBufSize)
    {
        // Get line
        rc = SDPPullLine(szSDPData, ulBufSize, pLine, m_pCCF);
        if (SUCCEEDED(rc) && pLine)
        {
            // Add the line to the list if we do not
            // have any special handling for it
            if (!HandleSDPLine(pLine, pStreamInfo))
            {
                pStreamInfo->pSDPData->AddTail((void*)pLine);
                pLine->AddRef();
            }

            // Increment the buffer to the next line
            // SDPPullLine success ensures size >= 1 and null terminated
            ulLength = pLine->GetSize() - 1;
            ulBufSize -= ulLength;
            szSDPData += ulLength;
            while(ulBufSize && (*szSDPData == '\r' || *szSDPData == '\n'))
            {
                ulBufSize--;
                szSDPData++;
            }
        }
        HX_RELEASE(pLine);
    }
}

BOOL
SDPMediaDescGenerator::HandleSDPLine(IHXBuffer* pLine, MediaInfo* pStreamInfo)
{
    const char* szLine = (const char*)pLine->GetBuffer();
    UINT32 ulSize = pLine->GetSize();

    // Only a= lines are allowed to be carried over.
    // Anything else must be parsed out and added earlier
    // to retain proper sdp entity order.
    if (strncmp(szLine, "a=", 2) != 0)
    {
        return TRUE;
    }

    // Store fmtp to handle separately
    if (strncasecmp(szLine, "a=fmtp:", 7) == 0)
    {
        if (!pStreamInfo->pszFmtp && ulSize > 8)
        {
            // Look for a=fmtp:<format><space> and skip it.
            UINT32 ulPos;
            for (ulPos = 7; ulPos < ulSize && isdigit(szLine[ulPos]);
                    ulPos++);

            if (ulPos + 1 < ulSize && szLine[ulPos] == ' ')
            {
                pStreamInfo->pszFmtp = EscapeBuffer(szLine + ulPos + 1,
                    ulSize - ulPos - 1);
            }
        }
        return TRUE;
    }

    for (UINT32 i = 0; g_pSDPDataPullTable[i].pName != NULL; i++)
    {
        if (ulSize >= g_pSDPDataPullTable[i].ulLen &&
            strncasecmp(szLine, g_pSDPDataPullTable[i].pName,
            g_pSDPDataPullTable[i].ulLen) == 0)
        {
            return TRUE;
        }
    }

    return FALSE;
}

BOOL
SDPMediaDescGenerator::HasSDPLine(CHXSimpleList* pSDPList, const char* szLine)
{
    IHXBuffer* pLine;
    char* szCurLine;

    // I think this is usually a very small list but if
    // not this should probably be a hash instead
    LISTPOSITION pos = pSDPList->GetHeadPosition();
    while (pos)
    {
        pLine = (IHXBuffer*) pSDPList->GetNext(pos);
        if (pLine)
        {
            szCurLine = (char*)pLine->GetBuffer();
            if (strcmp(szCurLine, szLine) == 0)
            {
                return TRUE;
            }
        }
    }

    return FALSE;
}

HX_RESULT
SDPMediaDescGenerator::GetStreamGroups(UINT16 usNumHeaders,
                                        IHXValues** pHeaders,
                                        UINT32 ulNumStreamGroups,
                                        CHXSimpleList**& pStreamGroups)
{
    UINT32 i = 0;
    IHXValues* pStreamHeader = NULL;
    UINT32 ulTrackID = 0;
    UINT32 ulGroup = 0;
    BOOL* pDefaultSet = new BOOL[ulNumStreamGroups];
    UINT32* pDefaultTracks = new UINT32[ulNumStreamGroups];
    UINT32 ulDefault = 0;
    BOOL bAdded = FALSE;

    pStreamGroups = new CHXSimpleList*[ulNumStreamGroups];

    if (!pStreamGroups || !pDefaultTracks || !pDefaultSet)
    {
        HX_VECTOR_DELETE(pDefaultTracks);
        HX_VECTOR_DELETE(pStreamGroups);
        HX_VECTOR_DELETE(pDefaultSet);
        return HXR_OUTOFMEMORY;
    }

    for (i = 0; i < ulNumStreamGroups; i++)
    {
        pDefaultTracks[i] = MAX_UINT32;
        pDefaultSet[i] = FALSE;
        pStreamGroups[i] = new CHXSimpleList();
        if (!pStreamGroups[i])
        {
            HX_VECTOR_DELETE(pDefaultTracks);
            HX_VECTOR_DELETE(pStreamGroups);
            return HXR_OUTOFMEMORY;
        }
    }

    // XXXJDG combine this with pulling stream info from the stream
    // headers

    // Iterate the steam headers to create stream groups and determine
    // the default stream.
    // If a stream is set as DefaultStream it is the default, else
    // lowest TrackID.
    // The search for lowest TrackID should probably happen
    // before now.
    for(i = 2; i < usNumHeaders; i++)
    {
        pStreamHeader = pHeaders[i];
        HX_ASSERT(pStreamHeader);
        bAdded = FALSE;

        if (pStreamHeader &&
            SUCCEEDED(pStreamHeader->GetPropertyULONG32("StreamGroupNumber",
            ulGroup)) && ulGroup < ulNumStreamGroups)
        {
            if (!pDefaultSet[ulGroup])
            {
                // Check if this is set as the default stream
                if (SUCCEEDED(pStreamHeader->GetPropertyULONG32(
                    "DefaultStream", ulDefault)) && ulDefault)
                {
                    if (pStreamGroups[ulGroup]->AddHead((void*)pStreamHeader))
                    {
                        pDefaultSet[ulGroup] = TRUE;
                        bAdded = TRUE;
                    }
                }
                else if (SUCCEEDED(pStreamHeader->GetPropertyULONG32(
                    "TrackID", ulTrackID)))
                {
                    // If the track ID is the lowest for this group, set
                    // as the default by inserting at head, and store the ID
                    if (ulTrackID < pDefaultTracks[ulGroup])
                    {
                        if (pStreamGroups[ulGroup]->AddHead(
                            (void*)pStreamHeader))
                        {
                            pDefaultTracks[ulGroup] = ulTrackID;
                            bAdded = TRUE;
                        }
                    }
                }
            }

            // If not the default, just add to the end of the list
            if (!bAdded)
            {
                pStreamGroups[ulGroup]->AddTail((void*)pStreamHeader);
            }
        }
    }

    HX_VECTOR_DELETE(pDefaultTracks);
    HX_VECTOR_DELETE(pDefaultSet);

    return HXR_OK;
}

UINT32
SDPMediaDescGenerator::CheckClient(IHXBuffer* pUserAgent,
                                   IHXBuffer* pPSSVersion)
{
    if (!pUserAgent)
    {
        return 0;
    }

    // Check if this is a known Helix client
    UINT32 ulClientType = 0;
    for (int i = 0; g_pHelixClients[i] && !(ulClientType & SDP_CLIENT_HLX);
        i++)
    {
        if (strncasecmp((const char*)pUserAgent->GetBuffer(),
            g_pHelixClients[i], strlen(g_pHelixClients[i])) == 0)
        {
            ulClientType |= SDP_CLIENT_HLX;
            break;
        }
    }

    // We only check for Rel6 if it's a Helix client!
    if (!(ulClientType & SDP_CLIENT_HLX))
    {
        return ulClientType;
    }

    // Check if it is 3GPP rel-6 compliant
    const char* szPSSName = "3GPP-R";
    const UINT32 ulPSSNameSize = 6;
    if (!pPSSVersion || pPSSVersion->GetSize() < ulPSSNameSize)
    {
        return ulClientType;
    }

    const char* szPssVer = (const char*)pPSSVersion->GetBuffer();
    if (strncasecmp(szPssVer, szPSSName, ulPSSNameSize) != 0)
    {
        return ulClientType;
    }

    if (atoi(&szPssVer[ulPSSNameSize]) < 6)
    {
        return ulClientType;
    }

    // Rel 6 client
    return (ulClientType | SDP_CLIENT_REL6);
}

BOOL SDPMediaDescGenerator::GetUseSessionGUID() const
{
    return m_bUseSessionGUID;
}

BOOL SDPMediaDescGenerator::GetUseAbsoluteURL() const
{
    return m_bUseAbsoluteURL;
}

void SDPMediaDescGenerator::SetUseSessionGUID(BOOL bOption)
{
    m_bUseSessionGUID = bOption;
}

void SDPMediaDescGenerator::SetUseAbsoluteURL(BOOL bOption)
{
    m_bUseAbsoluteURL = bOption;
}
