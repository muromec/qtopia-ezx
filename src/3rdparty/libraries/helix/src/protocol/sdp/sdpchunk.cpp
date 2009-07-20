/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: sdpchunk.cpp,v 1.16 2006/03/06 21:03:00 tknox Exp $
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

/****************************************************************************
 *  Defines
 */
#define DFLT_PAYLOAD_MIME_TYPE   "data/x-unknown"

#define CHAR_LF	0x0a
#define CHAR_CR	0x0d

#define BAD_PULL_TABLE_IDX  0xFFFFFFFF

#define MAX_INT_TEXT_LENGTH 10

#define N_CLIPRECT_COORDS   4

#define FMPT_PREFIX	    "FMTP"
#define FMPT_PREFIX_SIZE    (sizeof(FMPT_PREFIX) - 1)


/****************************************************************************
 *  Includes
 */
//#include "hlxclib/stdlib.h"
#include "hxstrutl.h"
#include "hxassert.h"
#include "sdpchunk.h"

#include "hxstring.h"
#include "hxslist.h"
#include "sdppyldinfo.h"


/****************************************************************************
 *  Locals
 */
/****************************************************************************
 *  Pull Functions
*/
inline HX_RESULT PullUINT32(char* pData, 
                            ULONG32 ulLength, 
                            IHXValues* pSDPValues,
                            IHXCommonClassFactory* pClassFactory,
                            const char* pValName);

static HX_RESULT PullSessionName(char* pData,
                                 ULONG32 ulLength,
                                 IHXValues* pSDPValues,
                                 IHXCommonClassFactory* pClassFactory);
static HX_RESULT PullMediaDesc	(char* pData,
                                 ULONG32 ulLength, 
                                 IHXValues* pSDPValues,
                                 IHXCommonClassFactory* pClassFactory);
static HX_RESULT PullRTPMap	(char* pData,
                                 ULONG32 ulLength, 
                                 IHXValues* pSDPValues,
                                 IHXCommonClassFactory* pClassFactory);
#if 0
static HX_RESULT PullControl	(char* pData,
                                 ULONG32 ulLength, 
                                 IHXValues* pSDPValues,
                                 IHXCommonClassFactory* pClassFactory);
#endif

static HX_RESULT PullClipRect	(char* pData, 
                                 ULONG32 ulLength, 
                                 IHXValues* pSDPValues,
                                 IHXCommonClassFactory* pClassFactory);

static HX_RESULT PullBufferDelay(char* pData, 
                                 ULONG32 ulLength, 
                                 IHXValues* pSDPValues,
                                 IHXCommonClassFactory* pClassFactory);

static HX_RESULT PullFormatParams(char* pData, 
                                 ULONG32 ulLength, 
                                 IHXValues* pSDPValues,
                                 IHXCommonClassFactory* pClassFactory);

static HX_RESULT PullGroupID	(char* pData, 
                                 ULONG32 ulLength, 
                                 IHXValues* pSDPValues,
                                 IHXCommonClassFactory* pClassFactory);

static HX_RESULT PullGroupBitrate(char* pData, 
                                 ULONG32 ulLength, 
                                 IHXValues* pSDPValues,
                                 IHXCommonClassFactory* pClassFactory);
static HX_RESULT PullBandwidth	(char* pData,
                                 ULONG32 ulLength, 
                                 IHXValues* pSDPValues,
                                 IHXCommonClassFactory* pClassFactory);

static HX_RESULT PullPreDecBufSize  (char* pData,
                                     ULONG32 ulLength, 
                                     IHXValues* pSDPValues,
                                     IHXCommonClassFactory* pClassFactory);
static HX_RESULT PullInitPreDecBufPeriod    (char* pData,
                                     ULONG32 ulLength, 
                                     IHXValues* pSDPValues,
                                     IHXCommonClassFactory* pClassFactory);
static HX_RESULT PullInitPostDecBufPeriod   (char* pData,
                                     ULONG32 ulLength, 
                                     IHXValues* pSDPValues,
                                     IHXCommonClassFactory* pClassFactory);
static HX_RESULT PullVideoPostDecBufSize    (char* pData,
                                     ULONG32 ulLength, 
                                     IHXValues* pSDPValues,
                                     IHXCommonClassFactory* pClassFactory);
static HX_RESULT PullDecByteRate    (char* pData,
                                     ULONG32 ulLength, 
                                     IHXValues* pSDPValues,
                                     IHXCommonClassFactory* pClassFactory);

/****************************************************************************
 *  Local Utilities
 */
inline static ULONG32 GetPullTableIdx(const SDPRecordPuller* pTable,
                                      char* pData, 
                                      ULONG32 ulRecordSize);

inline static char* FindSDPFieldByIdx(char* pData, 
                                      ULONG32 ulLength, 
                                      ULONG32 ulIdx);

inline static char* FindSDPFieldEnd(char* pData, 
                                    ULONG32 ulLength);

inline static char* FindCRLF(char* pData, 
                             ULONG32 ulLength);

inline static char* SkipSDPFieldEnd(char* pData,
                                    ULONG32 ulLength);


inline static void OrderUp(LONG32 &l1, LONG32 &l2);

/****************************************************************************
 *  Pull Tables - must be NULL terminated
 */
const SDPRecordPuller SessionPullTable[] =
{
    {"s=",	    sizeof("s=") - 1,		PullSessionName},
    {"b=",	    sizeof("b=") - 1,		PullBandwidth},
    {NULL,	    0,				NULL}
};

const SDPRecordPuller MediaPullTable[] =
{
    {"m=",	    sizeof("m=") - 1,		PullMediaDesc},
    {"a=rtpmap:",    sizeof("a=rtpmap:") - 1,	PullRTPMap},
    {"a=control:",   sizeof("a=control:") - 1,	NULL},
    {"b=",	    sizeof("b=") - 1,		PullBandwidth},
    {"a=X-predecbufsize:",          sizeof("a=X-predecbufsize:") - 1,
                                                PullPreDecBufSize},
    {"a=X-initpredecbufperiod:",    sizeof("a=X-initpredecbufperiod:") - 1,
                                                PullInitPreDecBufPeriod},
    {"a=X-initpostdecbufperiod:",   sizeof("a=X-initpostdecbufperiod:") - 1,
                                                PullInitPostDecBufPeriod},
    {"a=3gpp-videopostdecbufsize:",   sizeof("a=3gpp-videopostdecbufsize:") - 1,
                                                PullVideoPostDecBufSize},
    {"a=X-decbyterate:",            sizeof("a=X-decbyterate:") - 1,
                                                PullDecByteRate},
    {NULL,	    0,				NULL}
};

const SDPRecordPuller TimePullTable[] =
{
    {NULL,	    0,				NULL}
};

const SDPRecordPuller GenericPullTable[] =
{
    {NULL,	    0,				NULL}
};

const SDPRecordPuller RendererPullTable[] =
{
    {"a=cliprect:",	sizeof("a=cliprect:") - 1,	PullClipRect},
    {"a=x-bufferdelay:", sizeof("a=x-bufferdelay:") - 1,	PullBufferDelay}, 
    {"a=fmtp:",		sizeof("a=fmtp:") - 1,		PullFormatParams},
    {NULL,		0,				NULL}
};

const SDPRecordPuller GroupPullTable[] =
{
    {"a=x-alternate:group=",	sizeof("a=x-alternate:group=") - 1,	PullGroupID},
    {"a=x-alternate:datarate=",	sizeof("a=x-alternate:datarate=") - 1,	PullGroupBitrate},
    {NULL,			0,					NULL}
};

const SDPRecordPuller BandwidthPullTable[] =
{
    {"b=",	    sizeof("b=") - 1,		PullBandwidth},
    {NULL,	    0,				NULL}
};

/****************************************************************************
 *  Global Utility fuinctions
 */
/****************************************************************************
 *  SDPParseChunk - use context
 */
HX_RESULT SDPParseChunk(char* pData,
                        ULONG32 ulDataLen,
                        IHXValues* &pSDPValues,
                        IHXCommonClassFactory *pClassFactory,
                        SDPChunkContext SDPContext,
                        HXBOOL bPullRecords)
{
    HX_RESULT retVal = HXR_OK;
    const SDPRecordPuller* pPullTable = NULL;

    switch (SDPContext)
    {
    case SDPCTX_Media:
        pPullTable = MediaPullTable;
        break;
    case SDPCTX_Session:
        pPullTable = SessionPullTable;
        break;
    case SDPCTX_Bandwidth:
	pPullTable = BandwidthPullTable;
	break;
    case SDPCTX_Group:
        pPullTable = GroupPullTable;
        break;
    case SDPCTX_Time:
        pPullTable = TimePullTable;
        break;
    case SDPCTX_Generic:
        pPullTable = GenericPullTable;
        break;
    case SDPCTX_Renderer:
        pPullTable = RendererPullTable;
        break;
    default:
        retVal = HXR_INVALID_PARAMETER;
        break;
    }

    if (SUCCEEDED(retVal))
    {
        retVal = SDPParseChunk(pData,
                               ulDataLen,
                               pSDPValues,
                               pClassFactory,
                               pPullTable,
                               bPullRecords);
    }

    return retVal;
}

/****************************************************************************
 *  SDPParseChunk - use custom pull table
 */
HX_RESULT SDPParseChunk(char* pData,
                        ULONG32 ulDataLen,
                        IHXValues* &pSDPValues,
                        IHXCommonClassFactory *pClassFactory,
                        const SDPRecordPuller* pPullTable,
                        HXBOOL bPullRecords)
{
    HX_RESULT retVal = HXR_OK;
    IHXBuffer* pSDPBuffer = NULL;
    char* pSDPData = NULL;
    char* pPattern;
    ULONG32 ulRecordSize;
    ULONG32 ulPullTableIdx;
    HXBOOL bSDPValuesMadeHere = FALSE;
    
    if ((pData == NULL) || (ulDataLen == 0))
    {
        if (pSDPValues == NULL)
        {
            retVal = HXR_FAIL;
        }

        return retVal;
    }

    if (!pSDPValues)
    {
        bSDPValuesMadeHere = TRUE;
        retVal = pClassFactory->CreateInstance(CLSID_IHXValues,
                                                 (void**) &pSDPValues);
    }

    if (bPullRecords)
    {
        if (SUCCEEDED(retVal))
        {
            retVal = pClassFactory->CreateInstance(CLSID_IHXBuffer,
                (void**) &pSDPBuffer);
        }
        
        if (SUCCEEDED(retVal))
        {
            retVal = pSDPBuffer->SetSize(ulDataLen + 1);
        }
    }

    if (SUCCEEDED(retVal))
    {
        if (pPullTable == NULL)
        {
            retVal = HXR_INVALID_PARAMETER;
        }
    }

    if (SUCCEEDED(retVal))
    {
        if (pSDPBuffer)
        {
            pSDPData = (char*) pSDPBuffer->GetBuffer();
        }
        
        do
        {
            pPattern = StrNChr(pData, CHAR_LF, ulDataLen);

            if (pPattern == NULL)
            {
                ulRecordSize = ulDataLen;
            }
            else
            {
                ulRecordSize = (ULONG32)(pPattern - pData);
                pPattern = SkipSDPFieldEnd(pPattern, 
                                           ulDataLen - ulRecordSize);
                ulRecordSize = (ULONG32)(pPattern - pData);
            }
            
            ulPullTableIdx = GetPullTableIdx(pPullTable, 
                                             pData,
                                             ulRecordSize);

            if (ulPullTableIdx == BAD_PULL_TABLE_IDX)
            {
                if (pSDPData)
                {
                    // keep this entry as SDP data
                    memcpy(pSDPData, pData, ulRecordSize); /* Flawfinder: ignore */
                    pSDPData += ulRecordSize;
                }
            }
            else
            {	
                // pull this record out of SDP data
                if (pPullTable[ulPullTableIdx].pPullFunc)
                {
                    retVal = (*(pPullTable[ulPullTableIdx].pPullFunc))(
                                    pData,
                                    ulRecordSize,
                                    pSDPValues,
                                    pClassFactory);
                }
            }

            pData +=  ulRecordSize;
            ulDataLen -= ulRecordSize;
        } while ((ulDataLen != 0) && SUCCEEDED(retVal));
    }
   
    if (SUCCEEDED(retVal) && pSDPData)
    {
        HX_ASSERT(((ULONG32) (pSDPData - ((char *) pSDPBuffer->GetBuffer())) )
                  < pSDPBuffer->GetSize());

        if (((char *) pSDPBuffer->GetBuffer()) != pSDPData)
        {
            *pSDPData = '\0';
            pSDPValues->SetPropertyCString("SDPData", pSDPBuffer);
        }
    }

    HX_RELEASE(pSDPBuffer);

    if (FAILED(retVal) && bSDPValuesMadeHere)
    {
        HX_RELEASE(pSDPValues);
    }

    return retVal;
}


/****************************************************************************
 *  SDPMapPayloadToMime
 */
HX_RESULT SDPMapPayloadToMime(ULONG32 ulPayloadType,
                              IHXBuffer* &pMimeType,
                              IHXCommonClassFactory *pClassFactory)
{
    HX_RESULT retVal = HXR_OK;

    HX_ASSERT(pClassFactory);

    if (!pMimeType)
    {
        retVal = pClassFactory->CreateInstance(CLSID_IHXBuffer,
                                               (void**) &pMimeType);
    }

    if (SUCCEEDED(retVal))
    {
        const char* pPayloadMime = DFLT_PAYLOAD_MIME_TYPE;

        if (SDPIsKnownPayload(ulPayloadType))
        {
            pPayloadMime = SDPMapPayloadToMimeType(ulPayloadType);
        }

        retVal = pMimeType->Set((UCHAR*) pPayloadMime, 
                                strlen(pPayloadMime) + 1);
    }

    return retVal;
}


/****************************************************************************
 *  SDPIsFixedRatePayload
 */
HXBOOL SDPIsFixedRatePayload(ULONG32 ulPayloadType)
{
    if (SDPIsStaticPayload(ulPayloadType))
    {
        return !SDPIsTimestampDeliverable(ulPayloadType);
    }

    return FALSE;
}


/****************************************************************************
 *  SDPMapMimeToSamplesPerSecond
 */
ULONG32 SDPMapMimeToSamplesPerSecond(IHXBuffer* pMimeTypeBuffer)
{
    ULONG32 ulPayload = 0;
    ULONG32 ulSamplesPerSecond = 0;

    if (SUCCEEDED(SDPMapMimeToPayload(pMimeTypeBuffer, ulPayload)))
    {
        ulSamplesPerSecond = SDPMapPayloadToSamplesPerSecond(ulPayload);
    }
    else
    {
        const char* pTargetMimeType = (const char*)pMimeTypeBuffer->GetBuffer();
        ulSamplesPerSecond = SDPMapMimeTypeToSampleRate(pTargetMimeType);
    }

    return ulSamplesPerSecond;
}

/****************************************************************************
 *  SDPIsKnownPayload
 */
HXBOOL SDPIsKnownPayload(ULONG32 ulPayloadType)
{
    return (SDPIsStaticPayload(ulPayloadType) &&
            (SDPMapPayloadToMimeType(ulPayloadType) != NULL));
}


/****************************************************************************
 *  SDPMapMimeToPayloadID
 */
HX_RESULT SDPMapMimeToPayload(IHXBuffer* pMimeTypeBuffer, ULONG32 &ulPayload)
{
    ULONG32 ulId;
    HX_RESULT retVal = HXR_FAIL;
    char* pMimeType = NULL;
    char* pEncodingName;

    if (pMimeTypeBuffer)
    {
        pMimeType = (char *) pMimeTypeBuffer->GetBuffer();
    }

    if (pMimeType &&
        (pEncodingName = strchr((char*) pMimeType, '/')))
    {
        pEncodingName++;
        
        for (ulId = 0; SDPIsStaticPayload(ulId); ulId++)
        {
            if (SDPMapPayloadToEncodingName(ulId) &&
                !strcasecmp(SDPMapPayloadToEncodingName(ulId),
                            pEncodingName))
            {
                ulPayload = ulId;
                retVal = HXR_OK;
                break;
            }
        }
    }

    return retVal;
}


/****************************************************************************
 *  Pull Functions
 */

/* 
 * PullUINT32: Generic function to pull a UINT32 from a simple
 * a=<name>:<value>
 */
inline HX_RESULT PullUINT32(char* pData, 
                            ULONG32 ulLength, 
                            IHXValues* pSDPValues,
                            IHXCommonClassFactory* pClassFactory,
                            const char* pValName)
{
    char* pPattern;
    ULONG32 ulPatternLength;
    char pNumBuffer[MAX_INT_TEXT_LENGTH + 1]; /* Flawfinder: ignore */
    char* pNumEnd;
    ULONG32 ulValue;
    HX_RESULT retVal = HXR_OK;

    pPattern = StrNChr((char *) pData, ':', ulLength);

    retVal = HXR_FAIL;
    if (pPattern)
    {
        ulPatternLength = (ulLength - ((++pPattern) - pData));
        
        if (ulPatternLength > MAX_INT_TEXT_LENGTH)
        {
            ulPatternLength = MAX_INT_TEXT_LENGTH;
        }
        
        memcpy(pNumBuffer, pPattern, ulPatternLength); /* Flawfinder: ignore */
        pNumBuffer[ulPatternLength] = '\0';
        
        ulValue = strtoul(pNumBuffer, &pNumEnd, 10);
        
        if (pNumEnd > pNumBuffer)
        {
            retVal = pSDPValues->SetPropertyULONG32(pValName, ulValue);
        }
    }

    return retVal;
}

/*
 * "s=" text CRLF
 */
static HX_RESULT PullSessionName(char* pData,
                                 ULONG32 ulLength,
                                 IHXValues* pSDPValues,
                                 IHXCommonClassFactory* pClassFactory)
{
    HX_ASSERT(pData && !strncmp(pData, "s=", 2));

    HX_RESULT retVal = HXR_FAIL;
    IHXBuffer* pSessionName = NULL;

    retVal = SDPPullLine(pData + 2, ulLength - 2, pSessionName, pClassFactory);
    if (SUCCEEDED(retVal))
    {
      retVal = pSDPValues->SetPropertyCString("Title", pSessionName);
    }
    HX_RELEASE(pSessionName);
    return retVal;
}
static HX_RESULT PullMediaDesc	(char* pData, 
                                 ULONG32 ulLength, 
                                 IHXValues* pSDPValues,
                                 IHXCommonClassFactory* pClassFactory)
{
    char* pPattern;
    ULONG32 ulPayloadType = 0;
    IHXBuffer* pMimeType = NULL;
    HX_RESULT retVal = HXR_FAIL;

    // Extract Payload Type
    pPattern = FindSDPFieldByIdx(pData, ulLength, 3);
    if (pPattern != NULL)
    {
        char pNumBuffer[MAX_INT_TEXT_LENGTH + 1]; /* Flawfinder: ignore */
        char* pNumEnd;

        ULONG32 ulNumTextLength = (ulLength - (pPattern - pData));

        if (ulNumTextLength > MAX_INT_TEXT_LENGTH)
        {
            ulNumTextLength = MAX_INT_TEXT_LENGTH;
        }

        memcpy(pNumBuffer, pPattern, ulNumTextLength); /* Flawfinder: ignore */
        pNumBuffer[ulNumTextLength] = '\0';

        ulPayloadType = (ULONG32)strtol(pNumBuffer, &pNumEnd, 10);

        if (pNumEnd > pNumBuffer)
        {
            retVal = HXR_OK;
        }
    }

    // Prepare Mime Type
    if (SUCCEEDED(retVal))
    {
        const char* pPayloadMime = NULL;
        ULONG32 ulPayloadMimeLength = 0;
        char* pMimeStart = NULL;
        ULONG32 ulMimeLength = 0;

        if (SDPIsStaticPayload(ulPayloadType))
        {
            pPayloadMime = SDPMapPayloadToEncodingName(ulPayloadType);

            retVal = HXR_FAIL;
            if (pPayloadMime != NULL)
            {
                ulPayloadMimeLength = strlen(pPayloadMime) + 1;

                pSDPValues->SetPropertyULONG32(
                    "SamplesPerSecond",
                    SDPMapPayloadToSamplesPerSecond(ulPayloadType));

                UINT16 uChannels = 
                    SDPMapPayloadToChannels(ulPayloadType);
                if (uChannels > 0)
                {
                    pSDPValues->SetPropertyULONG32(
                        "Channels",
                        uChannels);
                }

                retVal = HXR_OK;
            }
        }

        if (SUCCEEDED(retVal))
        {
            // extract portion of mime type
            // - will be completed when rtp map encountered
            // if this is not a static payload
            pPattern = StrNChr((char *) pData, ' ', ulLength);
            pMimeStart = pData + 2;
            
            retVal = HXR_FAIL;
            
            if ((pPattern != NULL) &&
                ((pPattern - pData) > 2))
            {
                ulMimeLength = (ULONG32)(pPattern - pData - 2);
                
                retVal = pClassFactory->CreateInstance(
                    CLSID_IHXBuffer,
                    (void**) &pMimeType);
            }
            
            if (SUCCEEDED(retVal))
            {
                retVal = pMimeType->SetSize(ulMimeLength + 
                                            ulPayloadMimeLength + 
                                            1);
            }
            
            if (SUCCEEDED(retVal))
            {
                memcpy(pMimeType->GetBuffer(), /* Flawfinder: ignore */
                       pMimeStart, 
                       ulMimeLength);

                if (ulPayloadMimeLength > 0)
                {
                    memcpy(pMimeType->GetBuffer() + ulMimeLength + 1, /* Flawfinder: ignore */
                           pPayloadMime, 
                           ulPayloadMimeLength - 1);

                    pMimeType->GetBuffer()[ulMimeLength] = '/';
                }
                
                pMimeType->GetBuffer()[ulMimeLength + 
                                       ulPayloadMimeLength] = '\0';
            }
        }
    }

    if (SUCCEEDED(retVal))
    {
        pSDPValues->SetPropertyULONG32("RTPPayloadType", ulPayloadType);
        pSDPValues->SetPropertyCString("MimeType", pMimeType);
    }

    HX_RELEASE(pMimeType);

    return retVal;
}


static HX_RESULT PullRTPMap(char* pData, 
                            ULONG32 ulLength, 
                            IHXValues* pSDPValues,
                            IHXCommonClassFactory* pClassFactory)
{
    char* pPattern = NULL;
    char* pPatternEnd = NULL;
    char pNumBuffer[MAX_INT_TEXT_LENGTH + 1]; /* Flawfinder: ignore */
    char* pNumEnd = NULL;
    ULONG32 ulPatternLength = 0;
    ULONG32 ulPayloadType = 0;
    ULONG32 ulMediaPayloadType = 0;
    HX_RESULT retVal = HXR_FAIL;

    // Extract the Payload Type
    pPattern = StrNChr((char *) pData, ':',  ulLength);
    if (pPattern)
    {
        ulPatternLength = (ulLength - ((++pPattern) - pData));

        if (ulPatternLength > MAX_INT_TEXT_LENGTH)
        {
            ulPatternLength = MAX_INT_TEXT_LENGTH;
        }

        memcpy(pNumBuffer, pPattern, ulPatternLength); /* Flawfinder: ignore */
        pNumBuffer[ulPatternLength] = '\0';

        ulPayloadType = (ULONG32)strtol(pNumBuffer, &pNumEnd, 10);
        
        if (pNumEnd > pNumBuffer)
        {
            retVal = pSDPValues->GetPropertyULONG32(
                "RTPPayloadType", ulMediaPayloadType);
        }
    }

    // Check if this is the payload type entry we are looking for
    if (SUCCEEDED(retVal) && 
        (ulMediaPayloadType == ulPayloadType))
    {
        IHXBuffer* pMimeType = NULL;
        ULONG32 ulOldMimeTextLength;
        HXBOOL bHasParams = TRUE;

        // Locate New and Old Mime Name Section
        pPattern = StrNChr((char *) pData, ' ',  ulLength);
        retVal = HXR_FAIL;

        if (pPattern)
        {
            pPattern++;
            
            pPatternEnd = StrNChr(pPattern, 
                                  '/',
                                  ulLength - (pPattern - pData));
            if (pPatternEnd == NULL)
            {
                bHasParams = FALSE;
                pPatternEnd = FindSDPFieldEnd(pPattern,
                                              ulLength - (pPattern - pData));
            }
            
            ulPatternLength = (ULONG32)(pPatternEnd - pPattern);
            
            if (ulPatternLength != 0)
            {
                retVal = pSDPValues->GetPropertyCString(
                    "MimeType", pMimeType);
            }
        }

        // Realocate memory for combined name
        if (SUCCEEDED(retVal))
        {
            pMimeType->Release();   // OK since pSDPValues has ref.
            ulOldMimeTextLength = pMimeType->GetSize() - 1;
            retVal = pMimeType->SetSize(
                ulOldMimeTextLength + ulPatternLength + 2);
        }

        // Copy in new Mime Name section
        if (SUCCEEDED(retVal))
        {
            char *pMimeData = (char*) pMimeType->GetBuffer();

            memcpy(pMimeData + ulOldMimeTextLength + 1, /* Flawfinder: ignore */
                   pPattern,
                   ulPatternLength);

            pMimeData[ulOldMimeTextLength] = '/';
            pMimeData[pMimeType->GetSize() - 1] = '\0';
        }

        // Check for parameters following mime type
        if (SUCCEEDED(retVal))
        {
            ULONG32 ulParamIdx = 0;

            while (bHasParams)
            {
                pPattern += ulPatternLength + 1;
                 
                pPatternEnd = StrNChr(pPattern, 
                                      '/',
                                      ulLength - (pPattern - pData));
                if (pPatternEnd == NULL)
                {
                    bHasParams = FALSE;
                    pPatternEnd = FindSDPFieldEnd(
                                    pPattern,
                                    ulLength - (pPattern - pData));
                }

                ulPatternLength = (ULONG32)(pPatternEnd - pPattern);
            
                if (ulPatternLength != 0)
                {
                    LONG32 lValue;

                    if (ulPatternLength > MAX_INT_TEXT_LENGTH)
                    {
                        ulPatternLength = MAX_INT_TEXT_LENGTH;
                    }
                    
                    memcpy(pNumBuffer, pPattern, ulPatternLength); /* Flawfinder: ignore */
                    pNumBuffer[ulPatternLength] = '\0';
                    
                    lValue = strtol(pNumBuffer, &pNumEnd, 10);

                    if (pNumEnd > pNumBuffer)
                    {
                        switch (ulParamIdx)
                        {
                        case 0:
                            pSDPValues->SetPropertyULONG32(
                                "SamplesPerSecond", (ULONG32) lValue);
                            break;
                        case 1:
                            pSDPValues->SetPropertyULONG32(
                                "Channels", (ULONG32) lValue);
                            break;
                        default:
                            // do nothing
                            break;
                        }
                    }
                }

                ulParamIdx++;
            }
        }
    }

    return retVal;
}

#if 0
static HX_RESULT PullControl(char* pData, 
                             ULONG32 ulLength, 
                             IHXValues* pSDPValues,
                             IHXCommonClassFactory* pClassFactory)
{
    char* pPattern = NULL;
    char* pPatternEnd = NULL;
    ULONG32 ulPatternLength = 0;
    IHXBuffer* pControl = NULL;
    HX_RESULT retVal = HXR_FAIL;

    pPattern = StrNChr((char *) pData, ':', ulLength);

    if (pPattern)
    {
        pPattern++;
        pPatternEnd = FindSDPFieldEnd(
            pPattern, ulLength - (pPattern - pData));

        ulPatternLength = pPatternEnd - pPattern;

        if (ulPatternLength > 0)
        {
            retVal = pClassFactory->CreateInstance(
                CLSID_IHXBuffer,
                (void**) &pControl);
        }
    }

    if (SUCCEEDED(retVal))
    {
        retVal = pControl->SetSize(ulPatternLength + 1);
        memcpy(pControl->GetBuffer(), /* Flawfinder: ignore */
               pPattern, 
               ulPatternLength);
        (pControl->GetBuffer())[ulPatternLength] = '\0';

        pSDPValues->SetPropertyCString("Control", pControl);

        HX_RELEASE(pControl);
    }

    return retVal;
}
#endif

static HX_RESULT PullClipRect(char* pData, 
                              ULONG32 ulLength, 
                              IHXValues* pSDPValues,
                              IHXCommonClassFactory* pClassFactory)
{
    char* pPattern;
    ULONG32 ulPatternLength;
    char pNumBuffer[MAX_INT_TEXT_LENGTH + 1]; /* Flawfinder: ignore */
    char* pNumEnd;
    ULONG32 ulIdx = 0;
    LONG32 lCoord[N_CLIPRECT_COORDS];
    HX_RESULT retVal = HXR_FAIL;

    pPattern = StrNChr((char *) pData, ':', ulLength);

    if (pPattern)
    {
        do
        {
            ulPatternLength = (ulLength - ((++pPattern) - pData));
            
            if (ulPatternLength > MAX_INT_TEXT_LENGTH)
            {
                ulPatternLength = MAX_INT_TEXT_LENGTH;
            }
            
            memcpy(pNumBuffer, pPattern, ulPatternLength); /* Flawfinder: ignore */
            pNumBuffer[ulPatternLength] = '\0';
            
            lCoord[ulIdx] = strtol(pNumBuffer, &pNumEnd, 10);

            if (pNumEnd == pNumBuffer)
            {
                break;
            }
        } while (((++ulIdx) < N_CLIPRECT_COORDS) &&
                 (pPattern = StrNChr((char *) pPattern, ',', ulLength)));

        if (ulIdx == N_CLIPRECT_COORDS)
        {
            retVal = HXR_OK;
        }
    }

    if (SUCCEEDED(retVal))
    {
        OrderUp(lCoord[0], lCoord[2]);
        OrderUp(lCoord[1], lCoord[3]);

        pSDPValues->SetPropertyULONG32("ClipFrameLeft", (ULONG32)lCoord[1]);
        pSDPValues->SetPropertyULONG32("ClipFrameRight", (ULONG32)lCoord[3]);
        pSDPValues->SetPropertyULONG32("ClipFrameTop", (ULONG32)lCoord[0]);
        pSDPValues->SetPropertyULONG32("ClipFrameBottom", (ULONG32)lCoord[2]);
    }

    return retVal;
}


static HX_RESULT PullFormatParams(char* pData, 
                                  ULONG32 ulLength, 
                                  IHXValues* pSDPValues,
                                  IHXCommonClassFactory* pClassFactory)
{
    char* pPattern = NULL;
    char* pPatternEnd = NULL;
    HXBOOL bParmNumeric = FALSE;
    ULONG32 ulParmValue = 0;
    ULONG32 ulPatternLength = 0;
    char* pParmName = NULL;
    IHXBuffer* pParmValue = NULL;
    HX_RESULT retVal = HXR_FAIL;

    // Find the start of name-value tuple
    pPattern = FindSDPFieldEnd((char*) pData, ulLength);
    if (pPattern)
    {
        ulPatternLength = (ulLength - (pPattern - pData));
        retVal = HXR_OK;
    }

    do
    {
        // Find the start of name-value tuple
        if (SUCCEEDED(retVal))
        {
            retVal = HXR_FAIL;
            
            pPattern = SkipSDPFieldEnd(pPattern, ulPatternLength);
            if (pPattern)
            {
                ulPatternLength = (ulLength - (pPattern - pData));
                retVal = HXR_OK;
            }
        }

        if (SUCCEEDED(retVal))
        {
            if (ulPatternLength == 0)
            {
                // Done: no more tuples
                break;
            }
        }

        // Parse the tuple name
        if (SUCCEEDED(retVal))
        {
            retVal = HXR_FAIL;
            pPatternEnd = StrNChr((char*) pPattern, '=', ulPatternLength);
            if (pPatternEnd)
            {
                retVal = HXR_OK;
            }
        }

        if (SUCCEEDED(retVal))
        {
            retVal = HXR_FAIL;
            ulPatternLength = (ULONG32)(pPatternEnd - pPattern);
            if (ulPatternLength > 0)
            {
                retVal = HXR_OK;
            }
        }

        if (SUCCEEDED(retVal))
        {
            retVal = HXR_OUTOFMEMORY;
            pParmName = new char [ulPatternLength + FMPT_PREFIX_SIZE + 1];
            if (pParmName)
            {
                retVal = HXR_OK;
            }
        }

        if (SUCCEEDED(retVal))
        {
            memcpy(pParmName, /* Flawfinder: ignore */
                   FMPT_PREFIX,
                   FMPT_PREFIX_SIZE);
            memcpy(pParmName + FMPT_PREFIX_SIZE, /* Flawfinder: ignore */
                   pPattern, 
                   ulPatternLength);
            pParmName[ulPatternLength + FMPT_PREFIX_SIZE] = '\0';

            pPattern = pPatternEnd;
            ulPatternLength = (ulLength - ((++pPattern) - pData));
        }

        // Parse the tuple value
        if (SUCCEEDED(retVal))
        {
            retVal = HXR_FAIL;

            pPatternEnd = StrNChr(pPattern, ';', ulPatternLength);

            if (!pPatternEnd)
            {
                pPatternEnd = FindSDPFieldEnd(pPattern, ulPatternLength);
            }

            if (pPatternEnd)
            {
                ulPatternLength = (ULONG32)(pPatternEnd - pPattern);
            }

            if (ulPatternLength > 0)
            {
                retVal = HXR_OK;
            }
        }

        bParmNumeric = FALSE;

        if (SUCCEEDED(retVal) && (ulPatternLength <= (MAX_INT_TEXT_LENGTH - 1)))
        {
            char pNumBuffer[MAX_INT_TEXT_LENGTH + 1]; /* Flawfinder: ignore */
            char* pNumEnd = NULL;

            memcpy(pNumBuffer, pPattern, ulPatternLength); /* Flawfinder: ignore */
            pNumBuffer[ulPatternLength] = '\0';

            ulParmValue = (ULONG32)strtol(pNumBuffer, &pNumEnd, 10);
            if (pNumEnd && (pNumEnd == (pNumBuffer + ulPatternLength)))
            {
                bParmNumeric = TRUE;
            }
        }

        if (bParmNumeric)
        {
            if (SUCCEEDED(retVal))
            {
                retVal = pSDPValues->SetPropertyULONG32(pParmName, 
                                                        ulParmValue);
            }
        }
        else
        {
            if (SUCCEEDED(retVal))
            {
                retVal = pClassFactory->CreateInstance(CLSID_IHXBuffer,
                    (void**) &pParmValue);
            }
            
            if (SUCCEEDED(retVal))
            {
                retVal = pParmValue->SetSize(ulPatternLength + 1);
            }
            
            if (SUCCEEDED(retVal))
            {
                UINT8* pBufferData = pParmValue->GetBuffer();
                memcpy(pBufferData, pPattern, ulPatternLength); /* Flawfinder: ignore */
                pBufferData[ulPatternLength] = '\0';
                
                retVal = pSDPValues->SetPropertyCString(pParmName, pParmValue);
            }
        }

        if (SUCCEEDED(retVal))
        {
            ulPatternLength = 0;
            pPattern = pPatternEnd;
            if (pPattern)
            {
                ulPatternLength = (ulLength - ((++pPattern) - pData));
            }
        }

        HX_VECTOR_DELETE(pParmName);
        HX_RELEASE(pParmValue);
    } while (SUCCEEDED(retVal) && (ulPatternLength != 0));

    return retVal;
}


static HX_RESULT PullBufferDelay(char* pData, 
                                 ULONG32 ulLength, 
                                 IHXValues* pSDPValues,
                                 IHXCommonClassFactory* pClassFactory)
{
    char* pPattern;
    ULONG32 ulPatternLength;
    char pNumBuffer[MAX_INT_TEXT_LENGTH + 1]; /* Flawfinder: ignore */
    char* pNumEnd;
    ULONG32 ulPreroll;
    double fPreroll;
    HX_RESULT retVal = HXR_FAIL;

    if (pSDPValues->GetPropertyULONG32("Preroll", ulPreroll) != HXR_OK)
    {
        pPattern = StrNChr((char *) pData, ':', ulLength);

        if (pPattern)
        {
            ulPatternLength = (ulLength - ((++pPattern) - pData));
            
            if (ulPatternLength > MAX_INT_TEXT_LENGTH)
            {
                ulPatternLength = MAX_INT_TEXT_LENGTH;
            }
            
            memcpy(pNumBuffer, pPattern, ulPatternLength); /* Flawfinder: ignore */
            pNumBuffer[ulPatternLength] = '\0';
            
            fPreroll = strtod(pNumBuffer, &pNumEnd);
            
            if (pNumEnd > pNumBuffer)
            {
                retVal = pSDPValues->SetPropertyULONG32(
                    "Preroll", 
                    (ULONG32) (fPreroll * 1000.0 + 0.5));
            }
        }
    }

    return retVal;
}

static HX_RESULT PullGroupID(char* pData, 
                             ULONG32 ulLength, 
                             IHXValues* pSDPValues,
                             IHXCommonClassFactory* pClassFactory)
{
    char* pPattern;
    ULONG32 ulPatternLength;
    char pNumBuffer[MAX_INT_TEXT_LENGTH + 1]; /* Flawfinder: ignore */
    char* pNumEnd;
    ULONG32 ulGroupID;
    HX_RESULT retVal = HXR_FAIL;

    pPattern = StrNChr((char *) pData, ':', ulLength);

    if (pPattern)
    {
        ulPatternLength = (ulLength - ((++pPattern) - pData));

        pPattern = StrNChr((char *) pPattern, '=', ulPatternLength);
    }
    
    if (pPattern)
    {
        ulPatternLength = (ulLength - ((++pPattern) - pData));
        
        if (ulPatternLength > MAX_INT_TEXT_LENGTH)
        {
            ulPatternLength = MAX_INT_TEXT_LENGTH;
        }
        
        memcpy(pNumBuffer, pPattern, ulPatternLength); /* Flawfinder: ignore */
        pNumBuffer[ulPatternLength] = '\0';
        
        ulGroupID = (ULONG32)strtol(pNumBuffer, &pNumEnd, 10);
                
        if (pNumEnd > pNumBuffer)
        {
            retVal = pSDPValues->SetPropertyULONG32(
                "AlternateGroupID",
                ulGroupID);
        }
    }

    return retVal;
}

static HX_RESULT PullGroupBitrate(char* pData, 
                                  ULONG32 ulLength, 
                                  IHXValues* pSDPValues,
                                  IHXCommonClassFactory* pClassFactory)
{
    char* pPattern;
    ULONG32 ulPatternLength;
    char pNumBuffer[MAX_INT_TEXT_LENGTH + 1]; /* Flawfinder: ignore */
    char* pNumEnd;
    ULONG32 ulGroupBitrate;
    HX_RESULT retVal = HXR_FAIL;

    pPattern = StrNChr((char *) pData, ':', ulLength);
   
    if (pPattern)
    {
        ulPatternLength = (ulLength - ((++pPattern) - pData));
        
        pPattern = StrNChr((char *) pPattern, '=', ulPatternLength);
    }

    if (pPattern)
    {
        ulPatternLength = (ulLength - ((++pPattern) - pData));
        
        if (ulPatternLength > MAX_INT_TEXT_LENGTH)
        {
            ulPatternLength = MAX_INT_TEXT_LENGTH;
        }
        
        memcpy(pNumBuffer, pPattern, ulPatternLength); /* Flawfinder: ignore */
        pNumBuffer[ulPatternLength] = '\0';
        
        ulGroupBitrate = (ULONG32)strtol(pNumBuffer, &pNumEnd, 10);
                
        if (pNumEnd > pNumBuffer)
        {
            retVal = pSDPValues->SetPropertyULONG32(
                "AlternateGroupBitrate",
                ulGroupBitrate);
        }
    }

    return retVal;
}


static HX_RESULT PullBandwidth	(char* pData,
                                 ULONG32 ulLength, 
                                 IHXValues* pSDPValues,
                                 IHXCommonClassFactory* pClassFactory)
{        
    char* pPattern;
    ULONG32 ulPatternLength;
    char pNumBuffer[MAX_INT_TEXT_LENGTH + 1]; /* Flawfinder: ignore */
    char* pNumEnd;
    HX_RESULT retVal = HXR_FAIL;

    char* pPropName = NULL;
    ULONG32 ulBandwidth = 0;
    
    pPattern = StrNChr((char *) pData, ':', ulLength);           
    if (pPattern)
    {
        ulPatternLength = (ulLength - ((++pPattern) - pData));
        
        if (ulPatternLength > MAX_INT_TEXT_LENGTH)
        {
            ulPatternLength = MAX_INT_TEXT_LENGTH;
        }
        
        memcpy(pNumBuffer, pPattern, ulPatternLength); /* Flawfinder: ignore */
        pNumBuffer[ulPatternLength] = '\0';
        
        ulBandwidth = (ULONG32)strtol(pNumBuffer, &pNumEnd, 10);

        if (pNumEnd > pNumBuffer)
        {	    
	    // skip b=
	    pData += 2;
	    ulLength -= 2;
	    if (StrNStr((char*)pData, "AS:", ulLength, 3))
	    {        
		pPropName = "AvgBitRate";
		ulBandwidth *= 1000;
	    }
	    else if (StrNStr((char*)pData, "RR:", ulLength, 3))
	    {        
		pPropName = "RtcpRRRate";
	    }
	    else if (StrNStr((char*)pData, "RS:", ulLength, 3))
	    {        
		pPropName = "RtcpRSRate";
	    }
	    else
	    {
		// fine, ignore
		retVal = HXR_OK;
	    }	   
        }

	if (pPropName)
	{
	    retVal = pSDPValues->SetPropertyULONG32(pPropName, ulBandwidth);	    
	}
    }

    return retVal;
}

static HX_RESULT PullPreDecBufSize  (char* pData,
                                     ULONG32 ulLength, 
                                     IHXValues* pSDPValues,
                                     IHXCommonClassFactory* pClassFactory)
{
    return PullUINT32(pData, ulLength, pSDPValues, pClassFactory, 
                        "X-PreDecBufSize");
}

static HX_RESULT PullInitPreDecBufPeriod    (char* pData,
                                     ULONG32 ulLength, 
                                     IHXValues* pSDPValues,
                                     IHXCommonClassFactory* pClassFactory)
{
    return PullUINT32(pData, ulLength, pSDPValues, pClassFactory, 
                        "X-InitPreDecBufPeriod");
}

static HX_RESULT PullInitPostDecBufPeriod   (char* pData,
                                     ULONG32 ulLength, 
                                     IHXValues* pSDPValues,
                                     IHXCommonClassFactory* pClassFactory)
{
    return PullUINT32(pData, ulLength, pSDPValues, pClassFactory, 
                        "X-InitPostDecBufPeriod");
}

static HX_RESULT PullVideoPostDecBufSize    (char* pData,
                                     ULONG32 ulLength, 
                                     IHXValues* pSDPValues,
                                     IHXCommonClassFactory* pClassFactory)
{
    return PullUINT32(pData, ulLength, pSDPValues, pClassFactory, 
                        "3GPP-VideoPostDecBufSize");
}

static HX_RESULT PullDecByteRate    (char* pData,
                                     ULONG32 ulLength, 
                                     IHXValues* pSDPValues,
                                     IHXCommonClassFactory* pClassFactory)
{
    return PullUINT32(pData, ulLength, pSDPValues, pClassFactory, 
                        "X-DecByteRate");
}

/****************************************************************************
 *  Local Utilities
 */
inline static ULONG32 GetPullTableIdx(const SDPRecordPuller* pTable,
                                      char* pData, 
                                      ULONG32 ulRecordSize)
{
    ULONG32 ulIdx = 0;

    HX_ASSERT(pTable);

    while (pTable->pSDPMatch != NULL)
    {
        if (pTable->ulSDPMatchSize <= ulRecordSize)
        {
            if (!strncmp(pTable->pSDPMatch, 
                         pData, 
                         pTable->ulSDPMatchSize))
            {
                return ulIdx;
            }
        }

        pTable++;
        ulIdx++;
    }

    return BAD_PULL_TABLE_IDX;
}


inline static char* FindSDPFieldByIdx(char* pData, 
                                       ULONG32 ulLength, 
                                       ULONG32 ulIdx)
{
    char* pField;

    while((ulIdx != 0) && (pData != NULL))
    {
        pField = StrNChr((char *) pData, ' ', ulLength);
        if (pField)
        {
            ulLength -= (ULONG32)((++pField) - pData);  
            ulIdx--;
        }
        pData = pField;
    } 

    if (ulLength == 0)
    {
        pData = NULL;
    }

    return pData;
}

inline static char* FindSDPFieldEnd(char* pData, 
                                     ULONG32 ulLength)
{
    while((ulLength != 0) &&
          (*pData != ' ') &&
          (*pData != CHAR_CR) &&
          (*pData != CHAR_LF))
    {
        pData++;
        ulLength--;
    } 

    return pData;
}

inline static char* FindCRLF(char* pData, 
                             ULONG32 ulLength)
{
    HX_ASSERT(pData && ulLength);
    
    UINT32 i;

    for (i = 0; i < ulLength; i++)
    {
        // SDP parsers SHOULD accept either '\r' or '\n' as a line ender. 
        // As specified in RFC2327 - Section 6.
        if ((*pData == CHAR_CR)
        ||  (*pData == CHAR_LF))
        {
            return pData;
        }
        pData++;
    }
   
    return NULL;
}

/*
 * saves what pData points to upto CRLF sequence in pLine ('\0' terminated)
 */
HX_RESULT SDPPullLine(char* pData, 
                      ULONG32 ulLength,
                      REF(IHXBuffer*) pLine,
                      IHXCommonClassFactory* pClassFactory)
{
    HX_ASSERT(pData && ulLength);
    
    char* pc = NULL;
    HX_RESULT retVal = HXR_FAIL;

    pc = FindCRLF((char*)pData, ulLength);
    if (pc)
    {
        HX_ASSERT((pc > pData) && (ulLength > (ULONG32)(pc - pData)));
        ulLength = (ULONG32)(pc - pData);
        retVal = HXR_OK;
    }

    if (SUCCEEDED(retVal))
    {
        retVal = pClassFactory->CreateInstance
            (CLSID_IHXBuffer, (void**) &pLine);
    }	

    if (SUCCEEDED(retVal))
    {
        retVal = pLine->SetSize(ulLength + 1);	
        BYTE* pcLine = pLine->GetBuffer();
        memcpy(pcLine, pData, ulLength); /* Flawfinder: ignore */
        pcLine[ulLength] = '\0';
    }

    return retVal;            
}


inline static char* SkipSDPFieldEnd(char* pData, ULONG32 ulSize)
{
    while((ulSize > 0) &&
          ((*pData == CHAR_CR) ||
           (*pData == CHAR_LF) ||
           (*pData == ' ')))
    {
        pData++;
        ulSize--;
    } 

    return pData;
}

inline static void OrderUp(LONG32 &l1, LONG32 &l2)
{
    LONG32 temp;

    if (l1 > l2)
    {
        temp = l1;
        l1 = l2;
        l2 = temp;
    }
}
