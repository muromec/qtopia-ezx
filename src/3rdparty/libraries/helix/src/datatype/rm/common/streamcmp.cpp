/* ***** BEGIN LICENSE BLOCK ***** 
 * Version: RCSL 1.0/RPSL 1.0 
 *  
 * Portions Copyright (c) 1995-2002 RealNetworks, Inc. All Rights Reserved. 
 *      
 * The contents of this file, and the files included with this file, are 
 * subject to the current version of the RealNetworks Public Source License 
 * Version 1.0 (the "RPSL") available at 
 * http://www.helixcommunity.org/content/rpsl unless you have licensed 
 * the file under the RealNetworks Community Source License Version 1.0 
 * (the "RCSL") available at http://www.helixcommunity.org/content/rcsl, 
 * in which case the RCSL will apply. You may also obtain the license terms 
 * directly from RealNetworks.  You may not use this file except in 
 * compliance with the RPSL or, if you have a valid RCSL with RealNetworks 
 * applicable to this file, the RCSL.  Please see the applicable RPSL or 
 * RCSL for the rights, obligations and limitations governing use of the 
 * contents of the file.  
 *  
 * This file is part of the Helix DNA Technology. RealNetworks is the 
 * developer of the Original Code and owns the copyrights in the portions 
 * it created. 
 *  
 * This file, and the files included with this file, is distributed and made 
 * available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER 
 * EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS ALL SUCH WARRANTIES, 
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS 
 * FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT. 
 * 
 * Technology Compatibility Kit Test Suite(s) Location: 
 *    http://www.helixcommunity.org/content/tck 
 * 
 * Contributor(s): 
 *  
 * ***** END LICENSE BLOCK ***** */ 

#include "hxtypes.h"
#include "hxcom.h"
#include "hlxclib/string.h"
#include "ihxpckts.h"
#include "rmfftype.h"
#include "netbyte.h"
#include "hxmime.h"
#include "raparser.h"
#include "streamcmp.h"

HX_RESULT VerifyRMStreamCompatibility(IHXValues* pHdr1, IHXValues* pHdr2)
{
    IHXBuffer* pBuffer1 = NULL;
    IHXBuffer* pBuffer2 = NULL;
    HXBOOL bMimeTypesDiffer = TRUE;
    HXBOOL bRuleBooksDiffer = TRUE;
    HXBOOL bOpaqueDatasDiffer = TRUE;
    IHXBuffer* pMimeType = NULL;

    // Compare the Mime Types
    if (HXR_OK == pHdr1->GetPropertyCString("MimeType", pBuffer1))
    {
        if (HXR_OK == pHdr2->GetPropertyCString("MimeType", pBuffer2))
        {
            if (!strcmp((const char*)pBuffer1->GetBuffer(),
                        (const char*)pBuffer2->GetBuffer()))
            {
                bMimeTypesDiffer = FALSE;
            }
        }
    }
    HX_RELEASE(pBuffer1);
    HX_RELEASE(pBuffer2);

    if (bMimeTypesDiffer)
    {
        return HXR_FAIL;
    }

    // Compare the RuleBooks
    if (HXR_OK == pHdr1->GetPropertyCString("ASMRuleBook",
        pBuffer1))
    {
        if (HXR_OK == pHdr2->GetPropertyCString("ASMRuleBook",
            pBuffer2))
        {
            if (!strcmp((const char*)pBuffer1->GetBuffer(),
                        (const char*)pBuffer2->GetBuffer()))
            {
                bRuleBooksDiffer = FALSE;
            }
        }
    }
    HX_RELEASE(pBuffer1);
    HX_RELEASE(pBuffer2);

    // XXXDPS - For now, we will not declare these streams incompatible
    // just because the ASMRuleBooks differ
    if (FALSE/*bRuleBooksDiffer*/)
    {
        return HXR_FAIL;
    }

    // Compare the Opaque Data
    if (HXR_OK == pHdr1->GetPropertyBuffer("OpaqueData",
        pBuffer1))
    {
        if (HXR_OK == pHdr2->GetPropertyBuffer("OpaqueData",
            pBuffer2))
        {
            pHdr1->GetPropertyCString("MimeType", pMimeType);

            if (HXR_OK == CompareRMOpaqueData(pBuffer1, pBuffer2, pMimeType))
            {
                bOpaqueDatasDiffer = FALSE;
            }

            HX_RELEASE(pMimeType);
        }
    }
    else if (HXR_OK != pHdr2->GetPropertyBuffer("OpaqueData",
        pBuffer2))
    {
        // If neither stream has any Opaque Data, that's fine too
        bOpaqueDatasDiffer = FALSE;
    }
    HX_RELEASE(pBuffer1);
    HX_RELEASE(pBuffer2);

    if (bOpaqueDatasDiffer)
    {
        return HXR_FAIL;
    }

    return HXR_OK;
}

HX_RESULT CompareRMOpaqueData(IHXBuffer* pOp1, IHXBuffer* pOp2, IHXBuffer* pMimeType)
{
    HXBOOL bCompatible = TRUE;
    UINT32 uID1 = 0;
    UINT32 uID2 = 0;
    UCHAR* pCursor1 = NULL;
    UCHAR* pCursor2 = NULL;
    UINT32 ulHeaderSize1 = 0;
    UINT32 ulHeaderSize2 = 0;
    MultiStreamHeader msHeader1;
    MultiStreamHeader msHeader2;
    VideoTypeSpecificData v1;
    VideoTypeSpecificData v2;
    UINT32 ulBytesRead = 0;
    HX_RESULT hResult1 = HXR_OK;
    HX_RESULT hResult2 = HXR_OK;

    msHeader1.rule_to_header_map = NULL;
    msHeader2.rule_to_header_map = NULL;

    // Find out if these are multistream headers
    memcpy((char*)&uID1, (const char*)pOp1->GetBuffer(), sizeof(UINT32)); /* Flawfinder: ignore */
    uID1 = DwToHost(uID1);

    memcpy((char*)&uID2, (const char*)pOp2->GetBuffer(), sizeof(UINT32)); /* Flawfinder: ignore */
    uID2 = DwToHost(uID2);

    if (uID1 != uID2)
    {
        bCompatible = FALSE;
        goto exit;
    }

    // Compare Audio Type Specific Data
    if (!strcasecmp((const char*)pMimeType->GetBuffer(), REALAUDIO_MIME_TYPE) ||
        !strcasecmp((const char*)pMimeType->GetBuffer(), REALAUDIO_ENCRYPTED_MIME_TYPE))
    {
        if (RM_MULTIHEADER_OBJECT == uID1)
        {
            // Unpack the multistream headers
            pCursor1 = msHeader1.unpack(pOp1->GetBuffer(),
                pOp1->GetSize());
            pCursor2 = msHeader2.unpack(pOp2->GetBuffer(),
                pOp2->GetSize());

            if (msHeader1.num_headers != msHeader2.num_headers)
            {
                bCompatible = FALSE;
                goto exit;
            }

            for (UINT16 i = 0; i < msHeader1.num_headers; i++)
            {
                CStreamParam StreamParam1;
                CStreamParam StreamParam2;
                // Get the 1st type specific data for this physical stream
                memcpy((char*)&ulHeaderSize1, (char*)pCursor1, sizeof(UINT32)); /* Flawfinder: ignore */
                ulHeaderSize1 = DwToHost(ulHeaderSize1);
                pCursor1 += sizeof(UINT32);

                // Unpack the audio header
                hResult1 = StreamParam1.ReadOneRAHeader(pCursor1,
                                           ulHeaderSize1,
                                           &ulBytesRead);
                pCursor1 += ulHeaderSize1;

                // Get the 2nd type specific data for this physical stream
                memcpy((char*)&ulHeaderSize2, (char*)pCursor2, sizeof(UINT32)); /* Flawfinder: ignore */
                ulHeaderSize2 = DwToHost(ulHeaderSize2);
                pCursor2 += sizeof(UINT32);

                // Unpack the audio header
                hResult2 = StreamParam2.ReadOneRAHeader(pCursor2,
                                           ulHeaderSize2,
                                           &ulBytesRead);
                pCursor2 += ulHeaderSize2;

                if (hResult1 != hResult2)
                {
                    bCompatible = FALSE;
                    goto exit;
                }

                if (HXR_OK == hResult1)
                {
                    // Compare all of the relevant details...
                    if ((StreamParam1.uFlavorIndex != StreamParam2.uFlavorIndex)   ||
                        (StreamParam1.ulGranularity != StreamParam2.ulGranularity) ||
                        (strcmp(StreamParam1.codecID, StreamParam2.codecID))       ||
                        (StreamParam1.IsInterleaved != StreamParam2.IsInterleaved) ||
                        (StreamParam1.uChannels != StreamParam2.uChannels)         ||
                        (StreamParam1.ulSampleRate != StreamParam2.ulSampleRate)   ||
                        (StreamParam1.ulOpaqueDataSize !=
                            StreamParam2.ulOpaqueDataSize)                         ||
                        (memcmp(StreamParam1.opaqueData,
                                StreamParam2.opaqueData,
                                StreamParam1.ulOpaqueDataSize)))
                    {
                        bCompatible = FALSE;
                        goto exit;
                    }
                }
            }
        }
        else
        {
            CStreamParam StreamParam1;
            CStreamParam StreamParam2;
            // Unpack the audio header
            hResult1 = StreamParam1.ReadOneRAHeader(pOp1->GetBuffer(),
                                       pOp1->GetSize(),
                                       &ulBytesRead);

            // Unpack the audio header
            hResult2 = StreamParam2.ReadOneRAHeader(pOp2->GetBuffer(),
                                       pOp2->GetSize(),
                                       &ulBytesRead);

            if (hResult1 != hResult2)
            {
                bCompatible = FALSE;
                goto exit;
            }

            if (HXR_OK == hResult1)
            {
                // Compare all of the relevant details...
                if ((StreamParam1.uFlavorIndex != StreamParam2.uFlavorIndex)   ||
                    (StreamParam1.ulGranularity != StreamParam2.ulGranularity) ||
                    (strcmp(StreamParam1.codecID, StreamParam2.codecID))       ||
                    (StreamParam1.IsInterleaved != StreamParam2.IsInterleaved) ||
                    (StreamParam1.uChannels != StreamParam2.uChannels)         ||
                    (StreamParam1.ulSampleRate != StreamParam2.ulSampleRate)   ||
                    (StreamParam1.ulOpaqueDataSize !=
                        StreamParam2.ulOpaqueDataSize)                         ||
                    (memcmp(StreamParam1.opaqueData,
                            StreamParam2.opaqueData,
                            StreamParam1.ulOpaqueDataSize)))
                {
                    bCompatible = FALSE;
                    goto exit;
                }
            }
        }
    }
    // Compare Video Type Specific Data
    else if (!strcasecmp((const char*)pMimeType->GetBuffer(), REALVIDEO_MIME_TYPE) ||
             !strcasecmp((const char*)pMimeType->GetBuffer(), REALVIDEO_ENCRYPTED_MIME_TYPE))
    {
        if (RM_MULTIHEADER_OBJECT == uID1)
        {
            // Unpack the multistream header
            pCursor1 = msHeader1.unpack(pOp1->GetBuffer(),
                pOp1->GetSize());
            pCursor2 = msHeader2.unpack(pOp2->GetBuffer(),
                pOp2->GetSize());

            if (msHeader1.num_headers != msHeader2.num_headers)
            {
                bCompatible = FALSE;
                goto exit;
            }

            for (UINT16 i = 0; i < msHeader1.num_headers; i++)
            {
                // Get the 1st type specific data for this physical stream
                memcpy((char*)&ulHeaderSize1, (char*)pCursor1, sizeof(UINT32)); /* Flawfinder: ignore */
                ulHeaderSize1 = DwToHost(ulHeaderSize1);
                pCursor1 += sizeof(UINT32);

                // Unpack it
                v1.unpack(pCursor1, ulHeaderSize1);
                pCursor1 += ulHeaderSize1;

                // Get the 2nd type specific data for this physical stream
                memcpy((char*)&ulHeaderSize2, (char*)pCursor2, sizeof(UINT32)); /* Flawfinder: ignore */
                ulHeaderSize2 = DwToHost(ulHeaderSize2);
                pCursor2 += sizeof(UINT32);

                // Unpack it
                v2.unpack(pCursor2, ulHeaderSize2);
                pCursor2 += ulHeaderSize2;

                if ((v1.moftag != v2.moftag)           ||
                    (v1.submoftag != v2.submoftag)     ||
                    (v1.uiHeight != v2.uiHeight)       ||
                    (v1.uiWidth != v2.uiWidth)         ||
                    (v1.uiBitCount != v2.uiBitCount)   ||
                    (v1.uiPadWidth != v2.uiPadWidth)   ||
                    (v1.uiPadHeight != v2.uiPadHeight) ||
                    (v1.framesPerSecond != v2.framesPerSecond))
                {
                    bCompatible = FALSE;
                    goto exit;
                }
            }
        }
        else
        {
            // Unpack the type specific data
            v1.unpack(pOp1->GetBuffer(), pOp1->GetSize());
            v2.unpack(pOp2->GetBuffer(), pOp2->GetSize());

            if ((v1.moftag != v2.moftag)           ||
                (v1.submoftag != v2.submoftag)     ||
                (v1.uiHeight != v2.uiHeight)       ||
                (v1.uiWidth != v2.uiWidth)         ||
                (v1.uiBitCount != v2.uiBitCount)   ||
                (v1.uiPadWidth != v2.uiPadWidth)   ||
                (v1.uiPadHeight != v2.uiPadHeight) ||
                (v1.framesPerSecond != v2.framesPerSecond))
            {
                bCompatible = FALSE;
                goto exit;
            }
        }
    }
    // All other data types
    else
    {
        // The opaque datas must match exactly
        if (pOp1->GetSize() != pOp2->GetSize() ||
            (memcmp((char*)pOp1->GetBuffer(),
                    (char*)pOp2->GetBuffer(),
                    pOp1->GetSize())))
        {
            bCompatible = FALSE;
            goto exit;
        }
    }

exit:

    // Must delete this since PMC doesn't generate distructors
    HX_VECTOR_DELETE(msHeader1.rule_to_header_map);
    HX_VECTOR_DELETE(msHeader2.rule_to_header_map);

    return (bCompatible ? HXR_OK : HXR_FAIL);
}

