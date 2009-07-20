/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: sdpmdparse.cpp,v 1.42 2006/11/14 21:25:17 milko Exp $
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

#include "sdpmdparse.h"

#include "sdptypes.h"
#include "sdppyldinfo.h"
#include "rtptypes.h"
#include "nptime.h"
#include "rtsputil.h"
#include "safestring.h"
#include "chxfmtpparse.h"
#include "chxcharstack.h"
#include "chxpckts.h"
#include "netbyte.h"
#include "hxcore.h"
#include "hxtlogutil.h"

class SDPParseState
{
public:
    SDPParseState();

    HXBOOL m_bDefiniteDuration;
    ULONG32 m_ulDefaultDuration;
    CHXString m_mediaType;
    ULONG32 m_uPayloadType;
};

SDPParseState::SDPParseState() :
    m_bDefiniteDuration(FALSE),
    m_ulDefaultDuration(0),
    m_uPayloadType(0)
{}

SDPMediaDescParser::SDPMediaDescParser(ULONG32 ulVersion) :
    m_pContext(0),
    m_pCCF(0),
    m_ulVersion(ulVersion),
    m_pFileHeader(0)
{}

SDPMediaDescParser::~SDPMediaDescParser()
{
    HX_RELEASE(m_pCCF);
    HX_RELEASE(m_pContext);

    HX_RELEASE(m_pFileHeader);
    clearStreamList();
}

HX_RESULT SDPMediaDescParser::Init(IUnknown* pContext)
{
    HX_RESULT res = HXR_FAILED;

    HX_RELEASE(m_pContext);
    HX_RELEASE(m_pCCF);

    if (pContext)
    {
                HX_ENABLE_LOGGING(pContext);
        res = pContext->QueryInterface(IID_IHXCommonClassFactory,
                                       (void**)&m_pCCF);
        if (SUCCEEDED(res) && m_pCCF)
        {
            m_pContext = pContext;
            m_pContext->AddRef();
        }
    }

    return res;
}

HX_RESULT SDPMediaDescParser::Parse(IHXBuffer* pDescription,
                                    REF(UINT16) nValues,
                                    REF(IHXValues**) pValueArray)
{
    HX_RESULT theErr = HXR_OK;

    /*
     * pDescription may not be NULL terminated, so don't do ->GetBuffer()
     */
    char* pDescString = NULL;
    UINT32 ulDescStringLen = 0;
    theErr = pDescription->Get((BYTE*&)pDescString, ulDescStringLen);

    if (HXR_OK == theErr)
        theErr = fromExternalRep(pDescString, ulDescStringLen);

    if (HXR_OK == theErr)
    {
        nValues = (UINT16)(m_streams.GetCount() + 1);
        IHXValues** ppHeaders = new IHXValues*[nValues];
        ppHeaders[0] = m_pFileHeader;
        ppHeaders[0]->AddRef();
        CHXSimpleList::Iterator i;
        UINT16 j=1;
        for(i = m_streams.Begin();i != m_streams.End();++i,++j)
        {
            ppHeaders[j] = (IHXValues*)(*i);
            ppHeaders[j]->AddRef();
            TakeCareOfDefaults(ppHeaders[j]);
        }
        pValueArray = ppHeaders;
    }

    return theErr;
}

/* right now there is only ASMRuleBook */
void SDPMediaDescParser::TakeCareOfDefaults(IHXValues* pHeader)
{
    HX_ASSERT(pHeader);
    IHXBuffer* pBuf = NULL;
    if (pHeader->GetPropertyCString("ASMRuleBook", pBuf) != HXR_OK)
    {
        // there is no asmrulebook...
        UINT32 ul = 0;
        if (pHeader->GetPropertyULONG32("AvgBitRate", ul) == HXR_OK)
        {
            AddRuleBook(pHeader, ul);
        }
        else
        {
            AddRuleBook(pHeader, 0);
            // add 0 bandwidth...
            pHeader->SetPropertyULONG32("AvgBitRate", 0);
        }
    }

    // Add 3GPP Annex G defaults if any of the Annex G headers
    // are present
    ULONG32 ulTmp;
    if ((HXR_OK == pHeader->GetPropertyULONG32("x-initpredecbufperiod",
                                               ulTmp)) ||
        (HXR_OK == pHeader->GetPropertyULONG32("x-initpostdecbufperiod",
                                               ulTmp)) ||
        (HXR_OK == pHeader->GetPropertyULONG32("x-predecbufsize",
                                               ulTmp)) ||
        (HXR_OK == pHeader->GetPropertyULONG32("x-decbyterate",
                                               ulTmp)))
    {
        if (HXR_OK != pHeader->GetPropertyULONG32("x-initpredecbufperiod",
                                                  ulTmp))
        {
            pHeader->SetPropertyULONG32("x-initpredecbufperiod", 90000);
        }

        if (HXR_OK != pHeader->GetPropertyULONG32("x-predecbufsize",
                                                  ulTmp))
        {
            ULONG32 ulBitrate;
            ULONG32 ulBufSize = 51200;

            if (((HXR_OK == pHeader->GetPropertyULONG32("MaxBitRate",
                                                       ulBitrate)) ||
                 (HXR_OK == pHeader->GetPropertyULONG32("AvgBitRate",
                                                       ulBitrate))) &&
                ulBitrate)
            {
                if (ulBitrate <= 65536)
                {
                    ulBufSize = 20480;
                }
                else if (ulBitrate <= 131072)
                {
                    ulBufSize = 40960;
                }
            }

            pHeader->SetPropertyULONG32("x-predecbufsize", ulBufSize);
        }

        if (HXR_OK != pHeader->GetPropertyULONG32("Preroll", ulTmp))
        {
            // We don't have a preroll so we should use the Annex G
            // headers to create one
            ULONG32 ulPreDec = 0;
            ULONG32 ulPostDec = 0;
            pHeader->GetPropertyULONG32("x-initpredecbufperiod", ulPreDec);
            pHeader->GetPropertyULONG32("x-initpostdecbufperiod", ulPostDec);

            // Add x-initpredecbufperiod to x-initpostdecbufperiod and
            // convert the value to milliseconds.
            ULONG32 ulPreroll = (ulPreDec  + ulPostDec) / 90;

            if (ulPreroll)
            {
                pHeader->SetPropertyULONG32("Preroll", ulPreroll);
            }
        }
    }

    if (IsClient())
    {
        // If we are being used by the client we should try to override
        // the Preroll field with a value that is likely more accurate
        ULONG32 ulPreroll = 0;

        pHeader->GetPropertyULONG32("Preroll", ulPreroll);
        if (HXR_OK == pHeader->GetPropertyULONG32("ActualPreroll", ulTmp))
        {
            // Replace Preroll with ActualPreroll
            ulPreroll = ulTmp;
        }
        else if (HXR_OK == pHeader->GetPropertyULONG32("x-initpredecbufperiod",
                                                       ulTmp))
        {
            // Replace Preroll with the Annex G headers

            ULONG32 ulPostDec = 0;
            pHeader->GetPropertyULONG32("x-initpostdecbufperiod", ulPostDec);

            ulPreroll = (ulTmp  + ulPostDec) / 90;
        }

        if (ulPreroll)
        {
            pHeader->SetPropertyULONG32("Preroll", ulPreroll);
        }
    }

    HX_RELEASE(pBuf);
}

/* if ulBW is 0, we will assume TSD */
void SDPMediaDescParser::AddRuleBook(IHXValues* pHeader, UINT32 ulBW)
{
    IHXBuffer* pBuf = NULL;
    m_pCCF->CreateInstance(CLSID_IHXBuffer, (void**)&pBuf);
    if (!pBuf)
    {
        // nothing we can do...
        return;
    }

    if (ulBW)
    {
        char rulebook[256] = {0}; /* Flawfinder: ignore */

        UINT32 ulBWHalf = ulBW / 2;

        SafeSprintf(rulebook,256, "marker=0,AverageBandwidth=%d;marker=1,AverageBandwidth=%d;",
                    ulBW - ulBWHalf, ulBWHalf);
        pBuf->Set((BYTE*)rulebook, strlen(rulebook) + 1);
    }
    else
    {
        pBuf->Set((const BYTE*)"marker=0,timestampdelivery=1;marker=1,timestampdelivery=1;", 59);
    }

    pHeader->SetPropertyCString("ASMRuleBook", pBuf);
    HX_RELEASE(pBuf);
}

//
//      This is to fix a problem that causes the optimized builds
//      to cause the macintosh to crash!!!!!!!!!!!! <seanh>
//
#ifdef _MACINTOSH
#pragma peephole off
#pragma global_optimizer off
#pragma scheduling off
#endif


/*
 *  pData needs to be NULL terminated for we use strlen...
 */
HX_RESULT
SDPMediaDescParser::fromExternalRep(char* pData)
{
    return fromExternalRep(pData, strlen(pData));
}

HX_RESULT
SDPMediaDescParser::fromExternalRep(char* pData, UINT32 ulDataLen)
{
    const char* pCur = pData;
    const char* pEnd = pData + ulDataLen;

    ULONG32 nStreams = 0;
    CHXString extraFields;

    SDPParseState state;

    m_pFileHeader = CreateHeader();

    IHXValues* pHdr = m_pFileHeader;

    HX_RESULT res = (pHdr) ? HXR_OK : HXR_OUTOFMEMORY;

    while((HXR_OK == res) && *pCur && (pCur < pEnd))
    {
        // Skip \n or '\r' characters
        for(; *pCur && (pCur < pEnd) && strchr("\r\n", *pCur); pCur++)
            ;

        if (*pCur && (pCur < pEnd))
        {
            char lineType = *pCur++;

            if (*pCur == '=')
            {
                pCur++; // skip '='

                IHXBuffer* pSDPLine = 0;
                res = GetLine(pCur, pEnd, pSDPLine, lineType);

                if (HXR_OK == res)
                {
                    char* pLine = (char*)pSDPLine->GetBuffer();
                    //printf ("line %c '%s'\n", lineType, pLine);

                    switch(lineType) {
                    case 'v':
                        res = HandleVLine(pLine);
                        break;
                    case 'm':
                        if (!extraFields.IsEmpty())
                        {
                            AddString(pHdr, "SDPData",
                                      (const char*)extraFields);
                            extraFields.Empty();
                        }

                        pHdr = CreateHeader();
                        m_streams.AddTail(pHdr);

                        AddULONG32(pHdr, "StreamNumber", nStreams);
                        nStreams++;

                        // Propagate default duration to new stream
                        if (0 != state.m_ulDefaultDuration)
                        {
                            AddULONG32(pHdr, "Duration",
                                       state.m_ulDefaultDuration);
                        }

                        res = HandleMLine(&state, pLine, pHdr);

                        break;
                    case 'a':
                        res = HandleALine(&state, pLine, pHdr);
                        break;

                    case 'c':
                        res = HandleCLine(pLine, pHdr);
                        break;

                    case 'b':
                        res = HandleBLine(pLine, pHdr);
                        break;

                    case 'i':
                        AddString(pHdr, "Information", pLine);
                        res = HXR_OK;
                        break;

                    case 's':
                        AddBuffer(pHdr, "Title", (const UINT8 *)pLine, strlen(pLine));
                        res = HXR_OK;
                        break;
                    case 'o':
                    case 't':
                        // ignore these lines
                        res = HXR_OK;
                        break;

                    default:
                        res = HXR_NOT_SUPPORTED;
                        break;
                    };

                    if (HXR_NOT_SUPPORTED == res)
                    {
                        extraFields += lineType;
                        extraFields += '=';
                        extraFields += pLine;
                        extraFields += "\n";

                        res = HXR_OK;
                    }

                    pSDPLine->Release();
                    pSDPLine = 0;
                }
            }
            else
            {
                res = HXR_FAILED;
            }
        }
    }

    if (HXR_OK == res)
    {
        if (!extraFields.IsEmpty())
        {
            AddString(pHdr, "SDPData",
                      (const char*)extraFields);
            extraFields.Empty();
        }

        /* Only add StreamCount and LiveStream fields
         * if we have at least 1 stream. This prevents
         * the fields from appearing when we are using
         * the SDP plugin to parse partial SDP chunks.
         */
        if (nStreams)
        {
            AddULONG32(m_pFileHeader, "StreamCount", nStreams);

            ULONG32 ulLive;
            if (!state.m_bDefiniteDuration &&
                (state.m_ulDefaultDuration == 0) &&
                (!SUCCEEDED(m_pFileHeader->GetPropertyULONG32("LiveStream",
                                                              ulLive))))
            {
                AddULONG32(m_pFileHeader, "LiveStream", 1);
            }
        }
    }

    return res;
}

#ifdef _MACINTOSH
#pragma peephole reset
#pragma global_optimizer reset
#pragma scheduling reset
#endif


/*
* Payload table contains reserved/unassigned types as well.  So, this func
* returns if the table contains a valid info
*/
HXBOOL
SDPMediaDescParser::IsPayloadTableValid(UINT32 ulPayloadType) const
{
    HX_ASSERT(SDPIsStaticPayload(ulPayloadType));

    if (!SDPMapPayloadToEncodingName(ulPayloadType))
    {
        return FALSE;
    }
    return TRUE;
}

HX_RESULT
SDPMediaDescParser::getRTPMapInfo(const char*    pMapInfo,
                                  UINT32         ulRTPPayloadType,
                                  REF(CHXString) strMimeType,
                                  IHXValues*     pStream) const
{
    if (!pMapInfo || !strMimeType.GetLength() || !pStream)
    {
        return HXR_FAIL;
    }

    /*
     *  It could be either:
     *  EncodingName
     *  EncodingName/SamplesPerSecond
     *  EncodingName/SamplesPerSecond/Channels
     */

    strMimeType += "/";

    char* pTok = strchr((char*)pMapInfo, ' ');
    if (pTok)
    {
        *pTok = '\0';
    }
    pTok = (char*)pMapInfo;
    char* pTokNext = strchr(pTok, '/');


    UINT32 ulSampleRate = 0;
    UINT32 ulChannels   = 0;
    if (pTokNext)
    {
        *pTokNext++ = '\0';

        strMimeType += pTok;
        pTok = pTokNext;
        pTokNext = strchr((char*)pTok, '/');
        if (pTokNext)
        {
            *pTokNext++ = '\0';
            // Both sample rate and channels are specified
            ulSampleRate = (UINT32) strtol(pTok, 0, 10);
            ulChannels   = (UINT32) strtol(pTokNext, 0, 10);
        }
        else
        {
            // Only the sample rate is specified
            ulSampleRate = (UINT32) strtol(pTok, 0, 10);
        }
    }
    else
    {
        // this is just an encoding name
        strMimeType += pMapInfo;
    }
    // Do we have a sample rate?
    if (!ulSampleRate)
    {
        // No sample rate was specified, so see if we
        // have a default sample rate based on the payload
        // type and the mime type. If we cannot infer
        // the sample rate from the payload type or
        // mime type, then GetDefaultSampleRate() returns 0.
        ulSampleRate = GetDefaultSampleRate(ulRTPPayloadType, strMimeType);
    }
    // NOW do we have a sample rate?
    if (ulSampleRate)
    {
        // Whether specified or by default, we have a sample
        // rate, so add it to the stream header
        AddULONG32(pStream, "SamplesPerSecond", ulSampleRate);
    }
    // Do we have channels?
    if (!ulChannels)
    {
        // No channels were specified, so see if we
        // have a default channels based on the payload
        // type and mime type. If we cannot infer the
        // number of channels from the payload type
        // or the mime type, then GetDefaultChannels
        // returns 0.
        ulChannels = GetDefaultChannels(ulRTPPayloadType, strMimeType);
    }
    // NOW do we have channels?
    if (ulChannels)
    {
        // Whether specified or by default, we have a number
        // of channels, so add it to the stream header
        AddULONG32(pStream, "Channels", ulChannels);
    }

    return HXR_OK;
}

UINT32
SDPMediaDescParser::GetDefaultSampleRate(UINT32 ulRTPPayloadType, const char* pszMimeType) const
{
    // Try the payload type first
    UINT32 ulRet = SDPMapPayloadToSamplesPerSecond(ulRTPPayloadType);
    if (!ulRet && pszMimeType)
    {
        // Didn't get it by payload type, try mime type
        ulRet = SDPMapMimeTypeToSampleRate(pszMimeType);
    }

    return ulRet;
}

UINT32
SDPMediaDescParser::GetDefaultChannels(UINT32 ulRTPPayloadType, const char* pszMimeType) const
{
    // Try the payload type first
    UINT32 ulRet = (UINT32) SDPMapPayloadToChannels(ulRTPPayloadType);
    if (!ulRet && pszMimeType)
    {
        // Didn't get it by payload type, try mime type
        ulRet = SDPMapMimeTypeToChannels(pszMimeType);
    }

    return ulRet;
}

/*
*   Compare ulVersion against m_ulVersion.  and returns HXR_FAIL iff
*   ulVersion < m_ulVersion  ---> need auto update.
*/
HX_RESULT
SDPMediaDescParser::checkVersion(UINT32 ulVersion) const
{
    HX_ASSERT(m_ulVersion);
    UINT32 ulPluginMajor = HX_GET_MAJOR_VERSION(m_ulVersion);
    UINT32 ulPluginMinor = HX_GET_MINOR_VERSION(m_ulVersion);
    UINT32 ulDataMajor = HX_GET_MAJOR_VERSION(ulVersion);
    UINT32 ulDataMinor = HX_GET_MINOR_VERSION(ulVersion);

    if((ulDataMajor >  ulPluginMajor) ||
       (ulDataMajor == ulPluginMajor && ulDataMinor > ulPluginMinor))
    {
        return HXR_FAIL;
    }
    else
    {
        return HXR_OK;
    }
}

void SDPMediaDescParser::clearStreamList()
{
    LISTPOSITION pos = m_streams.GetHeadPosition();
    while(pos)
    {
        IHXValues* pStream = (IHXValues*)m_streams.GetNext(pos);
        HX_RELEASE(pStream);
    }
    m_streams.RemoveAll();
}

IHXValues* SDPMediaDescParser::CreateHeader() const
{
    IHXValues* pRet = 0;

    if (m_pCCF)
        m_pCCF->CreateInstance(IID_IHXValues, (void**)&pRet);

    return pRet;
}

void SDPMediaDescParser::AddULONG32(IHXValues* pHeader,
                                    const char* pKey, ULONG32 ulValue) const
{
    pHeader->SetPropertyULONG32(pKey, ulValue);
}

void SDPMediaDescParser::AddString(IHXValues* pHeader,
                                   const char* pKey,
                                   const char* pValue) const
{
    if ((pKey == NULL) || (pValue == NULL))
    {
        return;
    }

    IHXBuffer* pBuf = CopyBuffer((const UINT8*)pValue, strlen(pValue) + 1);

    if (pBuf)
        pHeader->SetPropertyCString(pKey, pBuf);

    HX_RELEASE(pBuf);
}

void SDPMediaDescParser::AddBuffer(IHXValues* pHeader,
                                   const char* pKey,
                                   const UINT8* pValue,
                                   ULONG32 ulLength) const
{
    IHXBuffer* pBuf = CopyBuffer(pValue, ulLength);

    if (pBuf)
        pHeader->SetPropertyBuffer(pKey, pBuf);

    HX_RELEASE(pBuf);
}

IHXBuffer* SDPMediaDescParser::CopyBuffer(const UINT8* pBuf,
                                          ULONG32 ulLength) const
{
    IHXBuffer* pRet = 0;
    if (SUCCEEDED(m_pCCF->CreateInstance(IID_IHXBuffer, (void**)&pRet)))
        pRet->Set(pBuf, ulLength);

    return pRet;
}

HX_RESULT SDPMediaDescParser::GetLine(const char*& pData,
                                      const char* pEnd,
                                      IHXBuffer*& pLine,
                                      char lineType) const
{
    HX_RESULT res = HXR_OK;

    CHXCharStack tok(m_pCCF);
    HXBOOL bInQuote = FALSE;
    HXBOOL bLastWasEscape = FALSE;

    for (; (HXR_OK == res) && *pData && (pData < pEnd) && (bInQuote || !strchr("\r\n", *pData)); pData++)
    {
        if ( lineType == 'a' )
        {
            if (bLastWasEscape)
            {
                bLastWasEscape = FALSE;
            }
            else
            {
                if (*pData == '"')
                {
                    bInQuote = !bInQuote;
                }
                else if (*pData == '\\')
                {
                    bLastWasEscape = TRUE;
                }
            }
        }

        res = tok.AddChar(*pData);
    }

    if (HXR_OK == res)
    {
        res = tok.Finish(pLine);
    }

    return res;
}

void SDPMediaDescParser::SkipSpaces(char*& pData) const
{
    for (; *pData && *pData == ' '; pData++)
        ;
}

HXBOOL SDPMediaDescParser::ScanForDelim(char*& pData, char delim) const
{
    HXBOOL bRet = FALSE;

    while(*pData && !bRet)
    {
        if (*pData == delim)
        {
            bRet = TRUE;
        }
        else
        {
            pData++;
        }
    }

    return bRet;
}

HX_RESULT SDPMediaDescParser::HandleVLine(char* pLine) const
{
    HX_RESULT res = HXR_FAILED;

    char* pEnd = 0;

    unsigned long version = strtoul(pLine, &pEnd, 10);

    if (*pLine && !*pEnd && (version == 0))
    {

        res = HXR_OK;
    }

    return res;
}

HX_RESULT SDPMediaDescParser::HandleMLine(SDPParseState* pState, char* pLine,
                                          IHXValues* pHdr) const
{
    HX_RESULT res = HXR_FAILED;

    if (pState && *pLine)
    {
        int state = 0;
        const char* pMediaType = pLine;
        ULONG32 ulPort = 0;
        ULONG32 ulPayloadType = 0;


        if (ScanForDelim(pLine, ' '))
        {
            // Null terminate mimetype
            *pLine++ = '\0';

            // Save media type for later
            pState->m_mediaType = pMediaType;

            res = HXR_OK;
        }

        while(*pLine && (HXR_OK == res))
        {
            char* pEnd = 0;

            SkipSpaces(pLine);

            if (*pLine)
            {
                switch(state) {
                case 0:
                    // Grab port number
                    ulPort = strtoul(pLine, &pEnd, 10);
                    if (*pEnd == ' ')
                    {
                        pLine = pEnd;
                        state = 1;
                    }
                    else
                    {
                        // No transport field or
                        // invalid port character
                        res = HXR_FAILED;
                    }
                    break;
                case 1:
                    // Skip transport. Usually RTP/AVP
                    if (ScanForDelim(pLine, ' '))
                    {
                        state = 2;
                    }
                    break;
                case 2:
                    // Grab the first payload type
                    ulPayloadType = strtoul(pLine, &pEnd, 10);
                    if ((*pEnd == ' ') || !*pEnd )
                    {
                        state = 3;

                        pState->m_uPayloadType = ulPayloadType;
                    }
                    else
                    {
                        // There was an unexpected character
                        // the the payload type
                        res = HXR_FAILED;
                    }

                case 3:
                    // Consume the rest of the payload types
                    for (; *pLine; pLine++)
                        ;
                    break;
                }
            }
        }

        if (state == 3)
        {
            AddULONG32(pHdr, "RTPPayloadType", ulPayloadType);

            if (ulPort)
            {
                AddULONG32(pHdr, "Port", ulPort);
            }

            if (SDPIsStaticPayload(ulPayloadType))
            {
                if (IsPayloadTableValid(ulPayloadType))
                {
                    if (!SDPIsTimestampDeliverable(ulPayloadType))
                    {
                        ULONG32 ulBitrate =
                            SDPMapPayloadToBitrate(ulPayloadType);

                        HX_ASSERT(ulBitrate);
                        // we have a bandwidth info in a table...
                        AddULONG32(pHdr, "AvgBitRate", ulBitrate);
                    }

                    // static payload type
                    AddString(pHdr, "MimeType",
                              SDPMapPayloadToMimeType(ulPayloadType));
                    AddULONG32(pHdr,
                               "RTPTimestampConversionFactor",
                               SDPMapPayloadToRTPFactor(ulPayloadType));
                    AddULONG32(pHdr,
                               "HXTimestampConversionFactor",
                               SDPMapPayloadToRMAFactor(ulPayloadType));
                    AddULONG32(pHdr,
                               "SamplesPerSecond",
                               SDPMapPayloadToSamplesPerSecond(ulPayloadType));
                    AddULONG32(pHdr,
                               "Channels",
                               (ULONG32)SDPMapPayloadToChannels(ulPayloadType));

                    // deal with opaque hack...
                    const BYTE* pOpaqueData = NULL;
                    ULONG32 ulOpaqueDataSize = 0;
                    switch(ulPayloadType)
                    {
                    case RTP_PAYLOAD_GSM:
                        pOpaqueData =
                            SDPMapPayloadToHeaderData(RTP_PAYLOAD_GSM,
                                                      ulOpaqueDataSize);
                        break;
                    }

                    if (pOpaqueData)
                    {
                        AddBuffer(pHdr, "OpaqueData",
                                  pOpaqueData, ulOpaqueDataSize);
                    }
                }
                else
                {
                    res = HXR_REQUEST_UPGRADE;
                }
            }
        }
        else if (HXR_OK == res)
        {
            res = HXR_FAILED;
        }
    }

    return res;
}

HX_RESULT SDPMediaDescParser::HandleALine(SDPParseState* pState,
                                          char* pLine, IHXValues* pHdr) const
{
    HX_RESULT res = HXR_FAILED;

    const char* pFieldName = pLine;
    char* pFieldValue = 0;
    FieldType fieldType = ftUnknown;

    if (*pLine)
    {
        // Collect the field name
        if (ScanForDelim(pLine, ':'))
        {
            char* pColon = pLine;

            // Must be the key/value form
            // a=foo:bar

            // Null terminate field name by replacing ':' with '\0'
            *pLine++ = '\0';

            pFieldValue = pLine;

            if ((HXR_OK == (res = ParseFieldValue(pFieldValue, fieldType))))
            {
                switch (fieldType) {
                case ftUnknown:
                    res = HandleSpecialFields(pState, pFieldName,
                                              pFieldValue, pHdr);
                    break;
                case ftULONG32:
                {
                    char* pEnd = 0;
                    ULONG32 ulValue = strtoul(pFieldValue, &pEnd, 10);

                    if (*pFieldValue && !*pEnd)
                    {
                        res = pHdr->SetPropertyULONG32(pFieldName, ulValue);
                    }
                } break;
                case ftString:
                    AddString(pHdr, pFieldName, pFieldValue);
                    res = HXR_OK;
                    break;
                case ftBuffer:
                {
                    int length = (int)strlen(pFieldValue);
                    BYTE* pDecodeBuf = new BYTE[length];
                    INT32 decodeLen = BinFrom64(pFieldValue, length,
                                                pDecodeBuf);
                    if( decodeLen != -1 )
                    {
                        AddBuffer(pHdr, pFieldName, pDecodeBuf, (ULONG32)decodeLen);
                    }

                    res = HXR_OK;

                    delete [] pDecodeBuf;
                }break;
                }
            }

            if (HXR_OK != res)
            {
                // Put back the ':'
                *pColon = ':';
            }
        }
        else
        {
            // Must be the key only form
            // a=foo
            res = HXR_NOT_SUPPORTED;
        }
    }

    return res;
}

HX_RESULT SDPMediaDescParser::HandleCLine(char* pLine, IHXValues* pHdr) const
{
    // Handles the following forms (see rfc3266)
    // c=IN IP4 xxx.xxx.xxx.xxx
    // c=IN IP4 xxx.xxx.xxx.xxx/xxx
    // c=IN IP4 xxx.xxx.xxx.xxx/xxx/xx
    // c=IN IP6 {IP6-multicast}
    HX_RESULT res = HXR_FAILED;

    char* pCur = pLine;

    if (ScanForDelim(pCur, ' '))
    {
        *pCur++ = '\0';

        SkipSpaces(pCur);

        char* pAddrType = pCur;

        if (!strcasecmp(pLine, "IN") && ScanForDelim(pCur, ' '))
        {
            *pCur++ = '\0';

            SkipSpaces(pCur);

            if (!strcasecmp(pAddrType, "IP4") && *pCur)
            {
                char* pAddr = pCur;

                if (ScanForDelim(pCur, '/'))
                {
                    *pCur++ = '\0';

                    if (*pCur)
                    {
                        char* pEnd = 0;
                        ULONG32 ulTTL = strtoul(pCur, &pEnd, 10);

                        if (*pCur && ((*pEnd == '/') || (!*pEnd)))
                        {
                            AddULONG32(pHdr, "MulticastTTL", ulTTL);
                            res = HXR_OK;

                            pCur = pEnd;
                            if (*pCur)
                            {
                                pCur++; // skip '/'
                                ULONG32 ulRange = strtoul(pCur, &pEnd, 10);

                                if (*pCur && !*pEnd)
                                {
                                    // c=IN IP4 xxx.xxx.xxx.xxx/xxx/xx
                                    AddULONG32(pHdr, "MulticastRange", ulRange);
                                    res = HXR_OK;
                                }
                            }
                        }
                    }
                }
                else
                {
                    // c=IN IP4 xxx.xxx.xxx.xxx
                    res = HXR_OK;
                }

                if (HXR_OK == res)
                {
                    AddString(pHdr, "MulticastAddress", pAddr);
                }
            }
            else if (!strcasecmp(pAddrType, "IP6") && *pCur)
            {
                // c=IN IP6 {IP6-multicast}
                AddString(pHdr, "MulticastAddress", pCur);
                res = HXR_OK;
            }
        }
    }

    return res;
}

HX_RESULT SDPMediaDescParser::HandleBLine(char* pLine, IHXValues* pHdr) const
{
    HX_RESULT res = HXR_FAILED;

    char* pCur = pLine;

    if (ScanForDelim(pCur, ':'))
    {
        char* pColon = pCur;
        *pCur++ = '\0';

        char* pEnd = 0;
        ULONG32 ulBwValue = strtoul(pCur, &pEnd, 10);

        if (*pCur && !*pEnd)
        {
            const char* pBwKey = 0;
            res = HXR_OK;

            if (!strcasecmp(pLine, "AS"))
            {
                // a=AvgBitRate has the higher precedence...
                ULONG32 ulTmp;
                if (!SUCCEEDED(pHdr->GetPropertyULONG32("AvgBitRate",
                                                        ulTmp)))
                {
                                // use this as a default.
                                // it's kilobits per sec....
                    pBwKey = "AvgBitRate";
                    ulBwValue *= 1000;
                }
            }
            else if (!strcasecmp(pLine, "RR"))
            {
                pBwKey = "RtcpRRRate";
            }
            else if (!strcasecmp(pLine, "RS"))
            {
                pBwKey = "RtcpRSRate";
            }
            else
            {
                res = HXR_NOT_SUPPORTED;
            }

            if (pBwKey)
            {
                AddULONG32(pHdr, pBwKey, ulBwValue);
            }
        }

        *pColon = ':';
    }

    return res;
}

HX_RESULT SDPMediaDescParser::ParseFieldValue(char*& pValue,
                                              FieldType& fieldType) const
{
    HX_RESULT res = HXR_OK;

    char* pCur = pValue;

    // Look for the following forms
    // a=anInt:integer;43
    // a=aString:string;"this is a string"
    // a=aBuffer:buffer;"TWFjIFRWAA=="

    // Assume we don't know the type.
    fieldType = ftUnknown;

    if (ScanForDelim(pCur, ';'))
    {
        // Replace the ';' with a '\0' so we
        // can check to see if it matches the known
        // types
        char* pSemiColon = pCur;

        *pCur++ = '\0';

        if (!strcmp(pValue, "integer"))
        {
            // Update the value pointer to the
            // start of the integer value
            pValue = pCur;
            fieldType = ftULONG32;
        }
        else if (!strcmp(pValue, "string"))
        {
            fieldType = ftString;
        }
        else if (!strcmp(pValue, "buffer"))
        {
            fieldType = ftBuffer;
        }

        if ((fieldType == ftString) || (fieldType == ftBuffer))
        {
            HXBOOL bFailed = TRUE;

            // Look for starting '"'
            if (*pCur == '"')
            {
                pCur++; // skip '"'

                if (*pCur)
                {
                    // Store start of the string
                    char* pStrStart = pCur;

                    // Create temporary buffer for unescaping
                    char* pTmpBuf = new char[strlen(pStrStart) + 1];

                    if (pTmpBuf)
                    {
                        char* pDest = pTmpBuf;

                        // Copy string into pTmpBuf and
                        // unescape any escape sequences
                        while (*pCur && *pCur != '"')
                        {
                            if (*pCur == '\\')
                            {
                                pCur++; // skip '\\'
                            }

                            *pDest++ = *pCur++;
                        }

                        // Make sure the last character is a '"'
                        if (*pCur == '"')
                        {
                            pCur++; // skip '"'

                            if (*pCur == '\0')
                            {
                                // Replace ending '"' with '\0'
                                *pDest = '\0';

                                // Replace escaped string with
                                // unescaped string.
                                strcpy(pStrStart, pTmpBuf);

                                // Update the value pointer to the
                                // start of the string value
                                pValue = pStrStart;
                                bFailed = FALSE;
                            }
                            else
                            {
                                // This was likely an unescaped ".
                                // Return an error since this is an
                                // unexpected event
                                res = HXR_UNEXPECTED;
                            }
                        }

                        delete [] pTmpBuf;
                    }
                }
            }

            if (bFailed)
            {
                fieldType = ftUnknown;
            }
        }


        if (fieldType == ftUnknown)
        {
            // This is not a type we recognize.
            // Replace the '\0' with a ';' so the
            // value is the way it was before.
            *pSemiColon = ';';
        }
    }

    return res;
}

HX_RESULT SDPMediaDescParser::HandleSpecialFields(SDPParseState* pState,
                                                  const char* pFieldName,
                                                  char* pFieldValue,
                                                  IHXValues* pHdr) const
{
    HX_RESULT res = HXR_FAILED;

    if (!strcasecmp("Range", pFieldName))
    {
        res = HandleRangeField(pState, pFieldValue, pHdr);
    }
    else if (!strcasecmp("length", pFieldName))
    {
        res = HandleLengthField(pState, pFieldValue, pHdr);
    }
    else if (!strcasecmp("rtpmap", pFieldName))
    {
        res = HandleRTPMapField(pState, pFieldValue, pHdr);
    }
    else if (!strcasecmp("fmtp", pFieldName))
    {
        res = HandleFMTPField(pFieldValue, pHdr);
    }
    else if (!strcasecmp("ptime", pFieldName))
    {
        // a=ptime:43
        AddULONG32(pHdr, "Ptime", (ULONG32)strtol(pFieldValue, 0, 10));
        res = HXR_OK;
    }
    else if (!strcasecmp("x-bufferdelay", pFieldName))
    {
        // a=x-bufferdelay:234
        // x-bufferdelay units are 1/1000 of a sec
        res = HandlePrerollField(pFieldValue, 1000, pHdr);
    }
    else if (!strcasecmp("x-initpredecbufperiod", pFieldName))
    {
        // 3GPP 26.234 Annex G field
        // a=x-initpredecbufperiod:45000
        // x-initpredecbufperiod units are 1/90000 of a sec
        AddULONG32(pHdr, "x-initpredecbufperiod",
                   strtoul(pFieldValue, 0, 10));
        res = HandlePrerollField(pFieldValue, 90000, pHdr);
    }
    else if (!strcasecmp("x-initpostdecbufperiod", pFieldName))
    {
        // 3GPP 26.234 Annex G field
        // a=x-initpostdecbufperiod:45000
        // x-initpostdecbufperiod units are 1/90000 of a sec
        AddULONG32(pHdr, "x-initpostdecbufperiod",
                   strtoul(pFieldValue, 0, 10));
        res = HXR_OK;
    }
    else if (!strcasecmp("x-predecbufsize", pFieldName))
    {
        // 3GPP 26.234 Annex G field
        // a=x-predecbufsize:45000
        // x-predecbufsize units are bytes
        AddULONG32(pHdr, "x-predecbufsize", strtoul(pFieldValue, 0, 10));
        res = HXR_OK;
    }
    else if (!strcasecmp("x-decbyterate", pFieldName))
    {
        // 3GPP 26.234 Annex G field
        // a=x-decbyterate:45000
        // x-decbyterate units are bytes per second
        AddULONG32(pHdr, "x-decbyterate", strtoul(pFieldValue, 0, 10));
        res = HXR_OK;
    }
    else if (!strcasecmp("3GPP-Adaptation-Support", pFieldName))
    {
        // 3GPP 26.234 Rel6 field
        // a=3GPP-Adaptation-Support:3
        AddULONG32(pHdr, "3GPP-Adaptation-Support",
                   strtoul(pFieldValue, 0, 10));
        res = HXR_OK;
    }
    else if (!strcasecmp("Helix-Adaptation-Support", pFieldName))
    {
        // a=Helix-Adaptation-Support:1
        AddULONG32(pHdr, "Helix-Adaptation-Support",
                   strtoul(pFieldValue, 0, 10));
        res = HXR_OK;
    }
    else if (!strcasecmp("alt-default-id", pFieldName))
    {
        // 3GPP 26.234 Rel6 field
        // a=alt-default-id:3
        AddULONG32(pHdr, "alt-default-id", strtoul(pFieldValue, 0, 10));
        res = HXR_OK;
    }
    else if (!strcasecmp("alt-group", pFieldName))
    {
        // 3GPP 26.234 Rel6 field
        // a=alt-group:BW:AS:28=1,3;56=1,4;60=2,4;120=2,5
        // a=alt-group:LANG:RFC3066:en-US=1,2,4,5;se=3,4,5
        res = HandleAltGroupField(pFieldValue, pHdr);
    }
    else if (!strcasecmp("alt", pFieldName))
    {
        // 3GPP 26.234 Rel6 field
        // a=alt:2:b=AS:23
        // a=alt:2:a=control:trackID=2
        res = HandleAltField(pState, pFieldValue, pHdr);
    }
    else if (!strcasecmp("SdpplinVersion", pFieldName))
    {
        res = checkVersion((UINT32)strtol(pFieldValue, 0, 10));

        if (HXR_FAIL == res)
        {
            // need to update...
            // this flag causes to exit in "m=" case
            res = HXR_REQUEST_UPGRADE;
        }
    }
    else if (!strcasecmp("control", pFieldName))
    {
        AddString(pHdr, "Control", pFieldValue);
        res = HXR_OK;
    }
        else if (!strcasecmp("framesize", pFieldName))
        {
                //3GPP TS 26.234  a=framesize <payload> <width> <height>
                res = HandleFramesizeField( pFieldValue , pHdr);
        }
        else if (!strcasecmp("framerate", pFieldName))
        {
                res = HandleFramerateField( pFieldValue , pHdr);
        }
        else if (!strcasecmp("3GPP-Asset-Information", pFieldName))
        {
                
                res = Handle3GPPAssetInformationField( pFieldValue , pHdr);
        }
        else
    {
        res = HXR_NOT_SUPPORTED;
    }

    return res;
}

HX_RESULT SDPMediaDescParser::HandleRangeField(SDPParseState* pState,
                                               char* pFieldValue,
                                               IHXValues* pHdr) const
{
    char* pCur = pFieldValue;

    UINT32 duration = 0;
    HX_RESULT rv = HXR_OK;

    HXLOGL4(HXLOG_SDPX, "SDPMediaDescParser::HandleRangeField: a=range:%s", pFieldValue);

    // BNF for Range (RFC 2326)
    // Range            = "Range" ":" 1\#ranges-specifier
    //                              [ ";" "time" "=" utc-time ]
    // ranges-specifier = npt-range | utc-range | smpte-range

    // Remove the optional ";time" portion.
    if(ScanForDelim(pCur, ';'))
    {
        // Not supported
        *pCur = '\0';
    }

    // Helix Supported formats
    // a=range:npt=npt-value
    // a=range:npt:npt-value
    // a=range:npt-value (assumed as npt)
    pCur = pFieldValue;
    if(strncasecmp(pCur, "npt", 3) == 0)
    {
        // replace delimiter with '\0'
        pCur += 3;
        *pCur++ = '\0';
    }
    else if( (strncasecmp(pCur, "smpte", 5) == 0) ||
        (strncasecmp(pCur, "clock", 5) == 0) )
    {
        // utc-range & smpte-range not supported
        HXLOGL3(HXLOG_SDPX, "SDPMediaDescParser::HandleRangeField: UnSupported Range-Specifier (%s)", pFieldValue);
        return HXR_NOT_SUPPORTED;
    }
    else
    {
        // a=range:npt-value
    }

    // Look for the following npt forms
    // a=range:npt=-xxx
    // a=range:npt=xxx-
    // a=range:npt=xxx-xxx
    char* pLeftVal = pCur;

    if (ScanForDelim(pCur, '-'))
    {
        // replace '-' with '\0'
        *pCur++ = '\0';

        NPTime left(pLeftVal);
        NPTime right(pCur);

        if (*pCur)
        {
            // a=range:npt=xxx-xxx
            duration = (ULONG32)(right - left);
            pState->m_bDefiniteDuration = TRUE;
        }
        else
        {
            // a=range:npt=xxx-
            // Treat open-ended play ranges as live streams
            // unless it is overridden by a media range.
        }
    }
    else
    {
        // This must be the following illegal form
        // a=range:npt=xxx or a=range:xxx
        rv = HXR_PARSE_ERROR;
        HXLOGL2(HXLOG_SDPX, "SDPMediaDescParser::HandleRangeField: Invalid Range");
    }



    if (SUCCEEDED(rv))
    {
        if (0 == pState->m_ulDefaultDuration)
        {
            pState->m_ulDefaultDuration = duration;
        }

        AddULONG32(pHdr, "Duration", duration);
    }

    return rv;
}

HX_RESULT SDPMediaDescParser::HandleLengthField(SDPParseState* pState,
                                                char* pFieldValue,
                                                IHXValues* pHdr) const
{
    HX_RESULT res = HXR_FAILED;

    char* pCur = pFieldValue;

    ULONG32 duration = 0;

    HXLOGL4(HXLOG_SDPX, "SDPMediaDescParser::HandleLengthField: a=Length:%s", pFieldValue);

    // Look for the following npt form
    // a=length:npt=xxx
    if (ScanForDelim(pCur, '='))
    {
        char* pEqual = pCur;

        // replace '=' with '\0'
        *pCur++ = '\0';

        if (!strcasecmp(pFieldValue, "npt") && *pCur)
        {
            NPTime dTime(pCur);

            duration = (ULONG32)dTime;
            res = HXR_OK;
        }
        else
        {
            // Put back '=' character
            *pEqual = '=';
        }
    }
    else
    {
        duration = (ULONG32)strtol(pFieldValue, 0, 10);
        res = HXR_OK;
    }

    if (duration)
    {
        pState->m_bDefiniteDuration = TRUE;
    }

    if (0 == pState->m_ulDefaultDuration)
    {
        pState->m_ulDefaultDuration = duration;
    }

    AddULONG32(pHdr, "Duration", duration);

    return res;
}

HX_RESULT SDPMediaDescParser::HandleRTPMapField(SDPParseState* pState,
                                                char* pFieldValue,
                                                IHXValues* pHdr) const
{
    HX_RESULT res = HXR_FAILED;

    // e.g. a=rtpmap:101 xxx/90000/2

    char* pCur = 0;
    UINT32 payload = (UINT32)strtol(pFieldValue, &pCur, 10);
    ULONG32 rtpPayloadType = pState->m_uPayloadType;
    HXBOOL bSetRTPPayloadType = FALSE;

    res = pHdr->GetPropertyULONG32("RTPPayloadType", rtpPayloadType);

    // If RTP Payload type is not present, we are likely processing
    // an "alt" SDP sub-section.   We set RTP payload type
    // to what is present in the RTP map since we need to have
    // a RTP payload type set.
    if (FAILED(res))
    {
	rtpPayloadType = payload;
	bSetRTPPayloadType = TRUE;
	res = HXR_OK;
    }

    if (*pFieldValue && (*pCur == ' '))
    {
        SkipSpaces(pCur);

        // there could be multiple of these...
        if (payload == rtpPayloadType)
        {
            CHXString mimeType(pState->m_mediaType);

            res = getRTPMapInfo(pCur, payload, mimeType, pHdr);
            /* make sure there is no mime type set!
             *  MimeType from m= && a=rtpmap has the lowest precedence.
             *  a=mimetype -> mimetype table -> this mimetype
             */
            IHXBuffer* pMimeType = 0;
            if (!SUCCEEDED(pHdr->GetPropertyCString(
                "MimeType", pMimeType)))
            {
                AddString(pHdr, "MimeType", mimeType);
            }
            HX_RELEASE(pMimeType);

	    if (bSetRTPPayloadType)
	    {
		res = pHdr->SetPropertyULONG32("RTPPayloadType", payload);
	    }
        }
    }

    return res;
}

HX_RESULT SDPMediaDescParser::HandleFMTPField(char* pFieldValue,
                                              IHXValues* pHdr) const
{
    // e.g. a=fmtp:101 emphasis=50/15;foo=bar

    char* pCur = 0;
    UINT32 payload = (UINT32)strtol(pFieldValue, &pCur, 10);

    ULONG32 rtpPayloadType = 0;
    HX_RESULT res = pHdr->GetPropertyULONG32("RTPPayloadType", rtpPayloadType);

    if (*pFieldValue && *pCur == ' ')
    {
        SkipSpaces(pCur);

        // If the RTPPayloadType field is present, compare it
        // to the value in the fmtp field.
        // There could be multiple of these...
        if ((HXR_OK != res) || (payload == rtpPayloadType))
        {
            AddString(pHdr, "PayloadParameters", pCur);

            CHXFMTPParser fmtp(m_pCCF);
            res = fmtp.Parse(pCur, pHdr);
        }
    }

    return res;
}

HX_RESULT SDPMediaDescParser::HandlePrerollField(char* pFieldValue,
                                                 ULONG32 ulPrerollUnits,
                                                 IHXValues* pHdr) const
{
    ULONG32 ulPreroll = 0;

    if (HXR_OK != pHdr->GetPropertyULONG32("Preroll", ulPreroll))
    {
        ULONG32 ulValue = strtoul(pFieldValue, 0, 10);

        // Convert Preroll value to milliseconds
        ulPreroll = (ulValue / ulPrerollUnits) * 1000;
        ulPreroll += ((ulValue % ulPrerollUnits) * 1000) / ulPrerollUnits;

        AddULONG32(pHdr, "Preroll", ulPreroll);
    }

    return HXR_OK;
}

HX_RESULT
SDPMediaDescParser::HandleAltGroupField(char* pFieldValue,
                                        IHXValues* pHdr) const
{
    // 3GPP 26.234 Rel6 field
    // a=alt-group:BW:AS:28=1,3;56=1,4;60=2,4;120=2,5
    // a=alt-group:LANG:RFC3066:en-US=1,2,4,5;se=3,4,5
    HX_RESULT res = HXR_NOT_SUPPORTED;

    IHXValues2* pHdr2 = NULL;

    if (HXR_OK == pHdr->QueryInterface(IID_IHXValues2, (void**)&pHdr2))
    {
        // We can only handle these lines if we can use the IHXValues2
        // interface
        char* pCur = pFieldValue;
        const char* pGroupType = pCur;

        if ((*pCur != ':') && ScanForDelim(pCur, ':') &&
            IsToken(pGroupType, pCur))
        {
            pCur++; // Skip the colon

            const char* pGroupSubType = pCur;

            if ((*pCur != ':') && ScanForDelim(pCur, ':') &&
                IsToken(pGroupSubType, pCur))
            {
                char* pColon = pCur;

                // Replace the ':' with a null terminator
                *pCur++ = '\0';

                res = ParseAltGroups(pHdr2, pGroupType, pCur);

                if (HXR_OK != res)
                {
                    // Put back the colon
                    *pColon = ':';
                }
            }
        }

    }
    HX_RELEASE(pHdr2);

    return res;
}

HX_RESULT
SDPMediaDescParser::GetAltGroupType(IHXValues2* pHdr,
                                       const char* pGroupType,
                                       HXBOOL bCreateIfNeeded,
                                       REF(IHXValues2*) pAltGroupType) const
{
    HX_RESULT res = HXR_INVALID_PARAMETER;

    if (pHdr && pGroupType)
    {
        IHXValues2* pAltGroups = NULL;

        if (HXR_OK == GetIHXValue2(pHdr, "Alt-Group", pAltGroups,
                                   bCreateIfNeeded))
        {
            res = GetIHXValue2(pAltGroups, pGroupType,
                               pAltGroupType, bCreateIfNeeded);
        }

        HX_RELEASE(pAltGroups);
    }

    return res;
}

HX_RESULT
SDPMediaDescParser::ParseAltGroups(IHXValues2* pHdr, const char* pGroupType,
                                   char* pBuf) const
{
    HX_RESULT res = HXR_NOT_SUPPORTED;

    IHXValues2* pTypeValues = NULL;

    // We only want to parse this line if this pGroupType doesn't
    // already exist
    if (pHdr && pGroupType && pBuf && IsToken(pBuf, pBuf + 1) && m_pCCF &&
        (HXR_OK != GetAltGroupType(pHdr, pGroupType, FALSE, pTypeValues)))
    {
        char* pCur = pBuf;

        IHXValues2* pGroupInfo = NULL;

        res = m_pCCF->CreateInstance(CLSID_IHXValues2, (void**)&pGroupInfo);

        while((HXR_OK == res) && IsToken(pCur, pCur + 1))
        {
            char* pGroupID = pCur;

            if (ScanForDelim(pCur, '=') && IsToken(pGroupID, pCur))
            {
                CHXString groupID(pGroupID, pCur - pGroupID);

                pCur++; // Skip '='

                IHXBuffer* pAltIDBuf = NULL;

                res = ParseAltIDList(pCur, pAltIDBuf);

                if (HXR_OK == res)
                {
                    IHXList* pList = NULL;

                    // Get the IHXList object for this groupID.
                    res = GetIHXList(pGroupInfo, groupID, pList);

                    if (HXR_OK == res)
                    {
                        // Add the alt ID info to the list
                        res = pList->InsertTail(pAltIDBuf);
                    }

                    HX_RELEASE(pList);
                }

                if ((HXR_OK == res) && (*pCur == ';'))
                {
                    pCur++; // skip ';'

                    if (!IsToken(pCur, pCur + 1))
                    {
                        // The next character is not a token
                        // character
                        res = HXR_NOT_SUPPORTED;
                    }
                }

                HX_RELEASE(pAltIDBuf);
            }
            else
            {
                // We couldn't find the delimiter or
                // the groupID was not a valid token
                res = HXR_NOT_SUPPORTED;
            }
        }

        if (HXR_OK == res)
        {
            // All the parsing was successful. Now merge this
            // alt group data into the header
            res = GetAltGroupType(pHdr, pGroupType, TRUE,
                                  pTypeValues);

            if (HXR_OK == res)
            {
                CHXHeader::mergeHeaders(pTypeValues, pGroupInfo);
            }
        }

        HX_RELEASE(pGroupInfo);
    }

    HX_RELEASE(pTypeValues);

    return res;
}

HX_RESULT
SDPMediaDescParser::ParseAltIDList(char*& pBuf,
                                   REF(IHXBuffer*) pValues) const
{
    HX_RESULT res = HXR_NOT_SUPPORTED;

    if (pBuf && IsAltID(pBuf, pBuf + 1) && m_pCCF)
    {
        res = m_pCCF->CreateInstance(IID_IHXBuffer, (void**)&pValues);

        if (HXR_OK == res)
        {
            UINT32 uIDIndex = 0;

            while((HXR_OK == res) && IsAltID(pBuf, pBuf + 1))
            {
                char* pAltID = pBuf;
                char* pAltIDEnd = NULL;

                // Scan for the end of the field
                for(; *pBuf && !strchr(",;", *pBuf); pBuf++)
                    ;

                pAltIDEnd = pBuf;

                // Make sure this is a valid altID and that it won't
                // overflow the string conversion function
                if (IsAltID(pAltID, pAltIDEnd) && ((pAltIDEnd - pAltID) < 11))
                {
                    char* pEnd = NULL;
                    UINT32 uAltID = strtoul(pAltID, &pEnd, 10);

                    if (pEnd == pAltIDEnd)
                    {
                        // The conversion was successful. Time to append
                        // the new value.

                        res = pValues->SetSize((uIDIndex + 1) * 4);

                        if (HXR_OK == res)
                        {
                            UINT32* pAltIDs = (UINT32*)pValues->GetBuffer();

                            pAltIDs[uIDIndex] = DwToNet(uAltID);

                            uIDIndex++;
                        }
                    }
                    else
                    {
                        res = HXR_NOT_SUPPORTED;
                    }
                }
                else
                {
                    res = HXR_NOT_SUPPORTED;
                }

                if (*pBuf == ',')
                {
                    pBuf++; // Skip the ','

                    // Make sure the current character is
                    // a valid alt ID character
                    if (!IsAltID(pBuf, pBuf + 1))
                    {
                        res = HXR_NOT_SUPPORTED;
                    }
                }
            }
        }

        if (HXR_OK != res)
        {
            HX_RELEASE(pValues);
        }
    }

    return res;
}

HX_RESULT
SDPMediaDescParser::HandleAltField(SDPParseState* pState,
                                   char* pFieldValue,
                                   IHXValues* pHdr) const
{
    // 3GPP 26.234 Rel6 field
    // a=alt:2:b=AS:23
    // a=alt:2:a=control:trackID=2

    HX_RESULT res = HXR_NOT_SUPPORTED;

    IHXValues2* pHdr2 = NULL;

    if (HXR_OK == pHdr->QueryInterface(IID_IHXValues2, (void**)&pHdr2))
    {
        // We can only handle these lines if we can use the IHXValues2
        // interface

        char* pCur = pFieldValue;
        const char* pAltID = pFieldValue;

        if (ScanForDelim(pCur, ':') && IsAltID(pAltID, pCur))
        {
            char* pColon = pCur;

            // Replace the ':' with a null terminator
            *pCur++ = '\0';

            IHXValues* pAltData = CreateHeader();

            if (*pCur && pAltData)
            {
                // get SDP line type
                char lineType = *pCur++;

                if (*pCur == '=')
                {
                    // skip '='
                    pCur++;

                    // Copy the current state so we
                    // don't effect the global parse
                    // state
                    SDPParseState altState = *pState;

                    switch(lineType) {
                    case 'a':
                        res = HandleALine(&altState, pCur, pAltData);
                        break;

                    case 'c':
                        res = HandleCLine(pCur, pAltData);
                        break;

                    case 'b':
                        res = HandleBLine(pCur, pAltData);
                        break;

                    case 'i':
                        AddString(pAltData, "Information", pCur);
                        res = HXR_OK;
                        break;

                    case 'o':
                    case 's':
                    case 't':
                        // ignore these lines
                        res = HXR_OK;
                        break;

                    default:
                        res = HXR_NOT_SUPPORTED;
                        break;
                    };

                    if (HXR_OK == res)
                    {
                        res = AddAltData(pHdr2, pAltID, pAltData);
                    }
                    else
                    {
                        // Mask errors encountered as
                        // unsupported lines. This causes them to get
                        // put in the SDPData field
                        res = HXR_NOT_SUPPORTED;
                    }
                }
            }

            if (HXR_OK != res)
            {
                // Put back the colon
                *pColon = ':';
            }

            HX_RELEASE(pAltData);
        }
    }

    HX_RELEASE(pHdr2);

    return res;
}

HX_RESULT
SDPMediaDescParser::Handle3GPPAssetInformationField(char* pFieldValue, IHXValues* pHdr) const
{
        HX_RESULT res = HXR_FAILED;
        if(pFieldValue)
        {
                char* pCur;
                char* pAssetName;
                do{     
                        pCur = pFieldValue;
                        if (ScanForDelim(pFieldValue, '{') && ScanForDelim(pCur, '}'))
                        {
                                *pCur = '\0';
                                pCur++;
                                pFieldValue++;
                                pAssetName = pFieldValue;
                                if (ScanForDelim(pFieldValue, '='))
                                {
                                        *pFieldValue = '\0';
                                        pFieldValue++;
                                        AddString(pHdr, pAssetName, pFieldValue);
                                        res = HXR_OK;
                                }
                                pFieldValue = pCur;
                        }                       
                }while (ScanForDelim(pFieldValue, ','));
        }
        return res;     
}

HX_RESULT
SDPMediaDescParser::HandleFramesizeField(char* pFieldValue, IHXValues* pHdr) const
{

         HX_RESULT res = HXR_FAILED;
        if(pFieldValue && *pFieldValue)
        {
                char* pFieldRemain    = NULL;
                char* pFieldRemainAux = NULL;
                char* pEnd            = NULL;
                //first number (payload) in a=framesize <payload> <width>-<height> not used now
                UINT32 ulPayload = (UINT32) strtoul(pFieldValue , &pFieldRemain , 10);

                //get <width> from the pFieldRemain
                UINT32 ulWidth = (UINT32) strtoul(pFieldRemain , &pFieldRemainAux , 10);

                //get <height> from pFieldRemainAux + 1. (use +1 to skip '-' symbol)
                UINT32 ulHeight = strtoul( pFieldRemainAux + 1 , &pEnd , 10);

                if ( ulWidth != 0 && ulHeight != 0 )
                {
                        //Add Values to Header
                        AddULONG32(pHdr, "FrameWidth",ulWidth);
                        AddULONG32(pHdr, "FrameHeight",ulHeight);
                        res = HXR_OK;
                }
        }

        return res;
        
}

HX_RESULT
SDPMediaDescParser::HandleFramerateField(char* pFieldValue, IHXValues* pHdr) const
{

    HX_RESULT res = HXR_INVALID_PARAMETER;
    if (pFieldValue && *pFieldValue )
    {
        double dFrameRate = atof(pFieldValue);
        if ( dFrameRate > 0 )
        {
            UINT32 ulFrameRatePerMSec = (UINT32) (dFrameRate*1000*1000 + 0.5);
            AddULONG32(pHdr, "FramesPerMSecond", ulFrameRatePerMSec);
            res = HXR_OK;
        }
    }
    return res;
}

HX_RESULT
SDPMediaDescParser::AddAltData(IHXValues2* pHdr, const char* pAltID,
                               IHXValues* pAltData) const
{
    HX_RESULT res = HXR_FAILED;

    if (pHdr)
    {
        IHXValues2* pAlt = NULL;

        if (HXR_OK == GetIHXValue2(pHdr, "Alt", pAlt, TRUE))
        {
            IHXValues2* pAltValues = NULL;

            if (HXR_OK == GetIHXValue2(pAlt, pAltID, pAltValues, TRUE))
            {
                CHXHeader::mergeHeaders(pAltValues, pAltData);

                res = HXR_OK;
            }
            HX_RELEASE(pAltValues);
        }

        HX_RELEASE(pAlt);
    }

    return res;
}

HX_RESULT
SDPMediaDescParser::GetIHXValue2(IHXValues2* pHdr, const char* pKey,
                                 REF(IHXValues2*) pVal,
                                 HXBOOL bCreateIfNeeded) const
{
    HX_RESULT res = HXR_INVALID_PARAMETER;

    if (pHdr)
    {
        IUnknown* pUnk = NULL;

        res = pHdr->GetPropertyObject(pKey, pUnk);

        if (HXR_OK == res)
        {
            res = pUnk->QueryInterface(IID_IHXValues2, (void**)& pVal);
        }
        else if (bCreateIfNeeded && m_pCCF)
        {
            // The object doesn't exist yet. Time to create and add it.
            res = m_pCCF->CreateInstance(CLSID_IHXValues2, (void**)&pVal);

            if (HXR_OK == res)
            {
                res = pHdr->SetPropertyObject(pKey, pVal);
            }
        }

        HX_RELEASE(pUnk);
    }

    return res;
}

HX_RESULT
SDPMediaDescParser::GetIHXList(IHXValues2* pHdr, const char* pKey,
                               REF(IHXList*) pVal) const
{
    HX_RESULT res = HXR_INVALID_PARAMETER;

    if (pHdr)
    {
        IUnknown* pUnk = NULL;

        res = pHdr->GetPropertyObject(pKey, pUnk);

        if (HXR_OK == res)
        {
            res = pUnk->QueryInterface(IID_IHXList, (void**)& pVal);
        }
        else if (m_pCCF)
        {
            // The object doesn't exist yet. Time to create and add it.
            res = m_pCCF->CreateInstance(CLSID_IHXList, (void**)&pVal);

            if (HXR_OK == res)
            {
                res = pHdr->SetPropertyObject(pKey, pVal);
            }
        }

        HX_RELEASE(pUnk);
    }

    return res;
}

HXBOOL
SDPMediaDescParser::IsToken(const char* pStart, const char* pEnd) const
{
    HXBOOL bRet = FALSE;

    if (pStart < pEnd)
    {
        bRet = TRUE;
        for (; bRet && (pStart < pEnd); pStart++)
        {
            bRet = ((*pStart == 0x21) ||
                    ((0x23 <= *pStart) && (*pStart <= 0x27)) ||
                    ((0x2A <= *pStart) && (*pStart <= 0x2B)) ||
                    ((0x2D <= *pStart) && (*pStart <= 0x2E)) ||
                    ((0x30 <= *pStart) && (*pStart <= 0x39)) ||
                    ((0x41 <= *pStart) && (*pStart <= 0x5A)) ||
                    ((0x5e <= *pStart) && (*pStart <= 0x7e)));
        }
    }

    return bRet;
}

HXBOOL
SDPMediaDescParser::IsAltID(const char* pStart, const char* pEnd) const
{
    HXBOOL bRet = FALSE;

    if (pStart < pEnd)
    {
        bRet = TRUE;
        for (; bRet && (pStart < pEnd); pStart++)
        {
            bRet = (('0' <= *pStart) && (*pStart <= '9'));
        }
    }

    return bRet;
}

HXBOOL SDPMediaDescParser::IsClient()
{
    HXBOOL bRet = FALSE;

    IHXClientEngine* pEngine = NULL;

    if (m_pContext &&
        (HXR_OK == m_pContext->QueryInterface(IID_IHXClientEngine,
                                              (void**)&pEngine)))
    {
        bRet = TRUE;
    }

    HX_RELEASE(pEngine);

    return bRet;
}
