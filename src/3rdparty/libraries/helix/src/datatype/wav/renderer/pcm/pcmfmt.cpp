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
#include "ihxpckts.h"
#include "hxrendr.h"
#include "baseobj.h"
#include "netbyte.h"
#include "legacy.h"
#include "audhead.h"
#include "audrendf.h"
#include "pcmfmt.h"
#include "audrend.h"
#include "pcmrend.h"
#include "pcm_convert.h"

CPCMAudioFormat::CPCMAudioFormat(IHXCommonClassFactory* pCommonClassFactory,
                                 CPCMAudioRenderer*     pPCMAudioRenderer)
    : CAudioFormat(pCommonClassFactory, pPCMAudioRenderer)
{
    m_ucMimeType       = kMimeTypeUnknown;
    m_bZeroOffsetPCM   = FALSE;
    m_bSwapSampleBytes = FALSE;
}

CPCMAudioFormat::~CPCMAudioFormat()
{
}

HX_RESULT CPCMAudioFormat::Init(IHXValues* pHeader)
{
    HX_RESULT retVal = CAudioFormat::Init(pHeader);
    if (SUCCEEDED(retVal))
    {
        // Get the mime type
        IHXBuffer* pMimeTypeStr = NULL;
        retVal = pHeader->GetPropertyCString("MimeType", pMimeTypeStr);
        if (SUCCEEDED(retVal))
        {
            // Get the mime type string
            const char* pszMimeType = (const char*) pMimeTypeStr->GetBuffer();
            // Reset the mime type member variable
            m_ucMimeType = kMimeTypeUnknown;
            // Determine the mime type
            if (!strcmp(pszMimeType, "audio/L8"))
            {
                m_ucMimeType = kMimeTypeAudioL8;
                m_pAudioFmt->uBitsPerSample = 8;
            }
            else if (!strcmp(pszMimeType, "audio/L16"))
            {
                m_ucMimeType = kMimeTypeAudioL16;
                m_pAudioFmt->uBitsPerSample = 16;
            }
            else if (!strcmp(pszMimeType, "audio/x-pn-wav"))
            {
                m_ucMimeType = kMimeTypeAudioXPNWav;
            }
            else if (!strcmp(pszMimeType, "audio/PCMA") || !strcmp(pszMimeType, "audio/pcma"))
            {
                m_ucMimeType = kMimeTypeAudioPCMA;
                m_pAudioFmt->uBitsPerSample = 16;
            }
            else if (!strcmp(pszMimeType, "audio/PCMU"))
            {
                m_ucMimeType = kMimeTypeAudioPCMU;
                m_pAudioFmt->uBitsPerSample = 16;
            }
            // Get the endianness
            HXBOOL bBigEndian = TestBigEndian();
            if (m_ucMimeType == kMimeTypeAudioXPNWav)
            {
                // Get the opaque data
                IHXBuffer* pBuffer = NULL;
                retVal = pHeader->GetPropertyBuffer("OpaqueData", pBuffer);
                if (SUCCEEDED(retVal))
                {
                    AudioPCMHEADER audioPCMHEADER;
                    PCMHEADER*     pPcmFormatRecord;
                    PCMHEADER      tmpPcmFormatRecord;

                    HXBOOL bIsOldHeader = TRUE;
                    if(pBuffer->GetSize() == audioPCMHEADER.static_size())
                    {
                        bIsOldHeader = FALSE;
                        audioPCMHEADER.unpack(pBuffer->GetBuffer(), pBuffer->GetSize());
                    }
                    // Check if this is from an old (preview release of G2 / rmasdk beta-9
                    // or earlier) file format:
                    if(bIsOldHeader                   ||
                       audioPCMHEADER.usVersion != 0  ||
                       audioPCMHEADER.usMagicNumberTag != AUDIO_PCMHEADER_MAGIC_NUMBER)
                    {
                        pPcmFormatRecord = (PCMHEADER*) pBuffer->GetBuffer();

                        // Make sure endianness is ok by swapping bytes if and only if
                        // usChannels is > 0xff; don't just count on this being net-
                        // endian as some preview-release file formats did not send
                        // the data net-endian:
                        if(pPcmFormatRecord->usChannels > 0xff)
                        {
                            SwapWordBytes((UINT16*)&pPcmFormatRecord->usFormatTag, 1);
                            SwapWordBytes((UINT16*)&pPcmFormatRecord->usChannels, 1);
                            SwapDWordBytes((UINT32*)&pPcmFormatRecord->ulSamplesPerSec, 1);
                            SwapWordBytes((UINT16*)&pPcmFormatRecord->usBitsPerSample, 1);
                            SwapWordBytes((UINT16*)&pPcmFormatRecord->usSampleEndianness, 1);
                        }
                    }
                    else
                    {
                        // This is a valid pAudioPCMHEADER and is already in the correct
                        // byte-order as performed by audioPCMHEADER.unpack():
                        pPcmFormatRecord = &tmpPcmFormatRecord;
                        switch(audioPCMHEADER.usVersion)
                        {
                            case 0:
                            {
                                // PCM format stuff starts 4 bytes into the buffer (which is
                                // right after audioPCMHEADER.usVersion and
                                // audioPCMHEADER.usMagicNumberTag:
                                pPcmFormatRecord->usFormatTag        = audioPCMHEADER.usFormatTag;
                                pPcmFormatRecord->usChannels         = audioPCMHEADER.usChannels;
                                pPcmFormatRecord->ulSamplesPerSec    = audioPCMHEADER.ulSamplesPerSec;
                                pPcmFormatRecord->usBitsPerSample    = audioPCMHEADER.usBitsPerSample;
                                pPcmFormatRecord->usSampleEndianness = audioPCMHEADER.usSampleEndianness;
                                break;
                            }
                            default:
                                retVal = HXR_UNEXPECTED;
                        }
                    }
                    if (SUCCEEDED(retVal))
                    {
                        if (pPcmFormatRecord->usFormatTag     == PN_PCM_ZERO_OFFSET &&
                            pPcmFormatRecord->usBitsPerSample == 8)
                        {
                            m_bZeroOffsetPCM = TRUE;
                        }
                        else
                        {
                            m_bZeroOffsetPCM = FALSE;
                        }

                        if (pPcmFormatRecord->usBitsPerSample    == 16 &&
                            pPcmFormatRecord->usSampleEndianness != bBigEndian) 
                        {
                            m_bSwapSampleBytes = TRUE;
                        }
                        else
                        {
                            m_bSwapSampleBytes = FALSE;
                        }
                        // Get the "MaxPacketSize" property
                        UINT32 ulMaxPacketSize = 0;
                        pHeader->GetPropertyULONG32("MaxPacketSize", ulMaxPacketSize);
                        // Set the audio stream format parameters
                        // (m_pAudioFmt is a member variable of our parent class)
                        m_pAudioFmt->uChannels       = pPcmFormatRecord->usChannels;
                        m_pAudioFmt->uBitsPerSample  = pPcmFormatRecord->usBitsPerSample;
                        m_pAudioFmt->ulSamplesPerSec = pPcmFormatRecord->ulSamplesPerSec;
                        m_pAudioFmt->uMaxBlockSize   = (UINT16) ulMaxPacketSize;
                    }
                }
                HX_RELEASE(pBuffer);
            }
            else if(m_ucMimeType == kMimeTypeAudioL8 || 
                    m_ucMimeType == kMimeTypeAudioL16 ||
                    m_ucMimeType == kMimeTypeAudioPCMA ||
                    m_ucMimeType == kMimeTypeAudioPCMU)
            {
                // We are either audio/L8 or audio/L16. Since these
                // formats are ALWAYS net-endian (big-endian), then
                // we only need to swap if we are NOT running on 
                // a big-endian processor
                if (m_pAudioFmt->uBitsPerSample == 16 && !bBigEndian)
                {
                    m_bSwapSampleBytes = TRUE;
                }
                else
                {
                    m_bSwapSampleBytes = FALSE;
                }
                // We don't need to override the audio stream format
                // parameters for audio/L8 or audio/L16 since they
                // are covered correctly by CAudioFormat::Init().
            }
            else
            {
                retVal = HXR_FAIL;      
            }
        }
        HX_RELEASE(pMimeTypeStr);
    }

    return retVal;
}

CMediaPacket* CPCMAudioFormat::CreateAssembledPacket(IHXPacket* pPacket)
{
    CMediaPacket* pRet = NULL;

    if (pPacket)
    {
        IHXBuffer* pBuffer = pPacket->GetBuffer();
        if (pBuffer)
        {
            UINT32 ulFlags = MDPCKT_USES_IHXBUFFER_FLAG;
            pRet = new CMediaPacket(pBuffer,
                                    (UINT8*) pBuffer->GetBuffer(),
                                    pBuffer->GetSize(),
                                    pBuffer->GetSize(),
                                    pPacket->GetTime(),
                                    ulFlags,
                                    NULL);
            pBuffer->Release();
        }
    }

    return pRet;
}

HX_RESULT CPCMAudioFormat::DecodeAudioData(HXAudioData& audioData,
                                           HXBOOL bFlushCodec)
{
    return DecodeAudioData(audioData,
                           bFlushCodec,
                           CAudioFormat::GetAudioPacket());
}

HX_RESULT CPCMAudioFormat::DecodeAudioData(HXAudioData& audioData,
                                           HXBOOL bFlushCodec,
                                           CMediaPacket *pPacket)
{
    HX_RESULT retVal = HXR_FAIL;
    
    if (pPacket && m_pCommonClassFactory)
    {
        // Create an IHXBuffer
        IHXBuffer* pBuffer = NULL;
        m_pCommonClassFactory->CreateInstance(CLSID_IHXBuffer, (void**) &pBuffer);
        if (pBuffer)
        {
            switch(m_ucMimeType)
            {
            case kMimeTypeAudioL8:
            case kMimeTypeAudioL16:
            case kMimeTypeAudioXPNWav:
                // Copy the media packet into this buffer
                retVal = pBuffer->Set(pPacket->m_pData, pPacket->m_ulDataSize);
                if(SUCCEEDED(retVal))
                {
                    // Byteswap the samples if necessary
                    if (m_bSwapSampleBytes)
                    {
                        SwapWordBytes((UINT16*) pBuffer->GetBuffer(), pBuffer->GetSize() / 2);
                    }
                    // Offset 8-bit samples if necessary
                    if (m_bZeroOffsetPCM)
                    {
                        UCHAR* pTmp = (UCHAR*) pBuffer->GetBuffer();
                        for (UINT32 i = 0; i < pBuffer->GetSize(); i++)
                        {
                            pTmp[i] -= 128;
                        }
                    }
                }
                break;
            case kMimeTypeAudioPCMA:
            case kMimeTypeAudioPCMU:    
            {
                UCHAR* pbCurSrc = (UCHAR*)(pPacket->m_pData);
                UINT16* pwCurDes = NULL;
                ULONG32 ulSampleNum = pPacket->m_ulDataSize;
                retVal = pBuffer->SetSize(ulSampleNum<<1);
                if(SUCCEEDED(retVal))
                {
                    pwCurDes = (UINT16*)(pBuffer->GetBuffer());
                    if(m_ucMimeType == kMimeTypeAudioPCMA)
                        PCM_CONVERTER_ALaw2Linear(pbCurSrc,pwCurDes,ulSampleNum);
                    else
                        PCM_CONVERTER_ULaw2Linear(pbCurSrc,pwCurDes,ulSampleNum);
                }
                break;  
            }
            default:
                retVal = HXR_FAIL;
            }

            if (SUCCEEDED(retVal))
            {
                audioData.pData            = pBuffer;
                audioData.ulAudioTime      = pPacket->m_ulTime;
                audioData.uAudioStreamType = STREAMING_AUDIO;
                audioData.pData->AddRef();
            }
        }
        HX_RELEASE(pBuffer);
        // Delete the packet
        CMediaPacket::DeletePacket(pPacket);
    }

    return retVal;
}

